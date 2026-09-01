# Sub-Device State Management

Author: Eric Yang (fornchu110)

Other contributors: None

Created: April 17, 2026

## Problem Description

OpenBMC provides state management for BMC, Host, and Chassis, but has no
mechanism for per-device power control of sub-devices such as NVMe drives, NICs,
or PCIe cards. Platforms that need per-drive power cycling today must either
misuse chassis or host instances, which produces incorrect Redfish semantics, or
write ad-hoc platform daemons outside the state-manager framework.

The goal is to add a sub-device state management framework to
phosphor-state-manager: a single type-agnostic daemon that discovers devices
through entity-manager and maps Redfish reset actions to systemd targets. The
first supported type is NVMe drives.

## Background and References

[phosphor-state-manager][1] manages BMC, Host, and Chassis state. Each daemon
exposes a D-Bus interface from [phosphor-dbus-interfaces][2] and maps requested
transitions to systemd targets, monitoring `JobRemoved` signals to update
current state.

The existing `xyz.openbmc_project.State.Drive` interface defines
`RequestedDriveTransition` and `CurrentDriveState` with the same pattern used by
Host and Chassis, one interface per managed entity type.

This design instead uses a single `xyz.openbmc_project.State.Device` interface
for every sub-device type. It defines `RequestedDeviceTransition`,
`AllowedDeviceTransitions` and `CurrentDeviceState`, a `Transition` enum of
`On`, `Off`, `Reboot`, `HardReboot`, `PowerCycle` and `None`, a `DeviceState`
enum of `Off`, `TransitioningToOff`, `On`, `TransitioningToOn` and `Unknown`,
and a `paths:` metadata block for the object path convention. A single interface
matches Redfish, which reuses one `ResetType` enum across every device schema
rather than defining a reset action per type.

Type-specific state properties are out of scope for this design. Where a device
type genuinely needs one, it would be added as a separate device-specific
interface, and both that interface's namespace and which daemon hosts it are
undecided and require their own design discussion.

[entity-manager][3] is the standard mechanism for runtime hardware discovery. It
consumes probe signals from presence-detection daemons and publishes inventory
and configuration interfaces on D-Bus.

[dbus-sensors][4] organises multiple related daemons (adc, fan, nvme, psu, and
others) around a shared common source tree, with one binary plus one systemd
service per sensor type, each long-lived, single-instance and
entity-manager-driven. Every sensor type exposes the same
`xyz.openbmc_project.Sensor.Value` interface; what differs per type is the
hardware access path (I2C, hwmon, NVMe-MI) and its build dependencies. This
design has no equivalent hardware-access split, since the hardware operation is
left to a platform service; see Alternatives Considered for why it therefore
uses a single daemon rather than one per type.

DMTF defines a `Drive.Reset` action in the Redfish Drive schema (Drive v1.7.0,
DSP8010 2019.2). The `ResetType` values map to the D-Bus transitions.

[1]: https://github.com/openbmc/phosphor-state-manager
[2]: https://github.com/openbmc/phosphor-dbus-interfaces
[3]: https://github.com/openbmc/entity-manager
[4]: https://github.com/openbmc/dbus-sensors

## Requirements

1. Provide per-device power control (on, off, power cycle, reboot, hard reboot)
   via D-Bus interfaces that bmcweb and other consumers can use. The current
   state must be readable and updated as transitions complete.
2. The daemon must discover devices at runtime through entity-manager rather
   than hardcoding instances at build time. Slots without a device present must
   produce no D-Bus object and no Redfish resource.
3. Separate hardware-specific power control (GPIO toggling, I2C IO expander
   control, a terminus command, etc.) from state management logic. The daemon
   executes each transition as an ordered sequence of operations drawn from a
   closed, daemon-defined vocabulary (GPIO set/get/wait, D-Bus property
   get/wait, a terminus command, sleep, a named concurrency limit); a platform
   supplies which sequence template applies to an instance and that instance's
   parameters, but cannot introduce a new kind of operation.
4. Each state object must maintain a D-Bus association to its inventory object
   so that bmcweb can locate the state from inventory without relying on path
   naming conventions.
5. All sub-device types share a single `State.Device` interface for transition
   requests and current state, consistent with Redfish using one `ResetType`
   enum across all device schemas. Adding a new type (NIC, PCIe card) to this
   power-control mechanism must not require a new D-Bus interface, a new daemon,
   or changes to how existing types work: it supplies the inventory interface to
   watch, a systemd target name prefix, and whether a host-dependency check
   applies. This is a guarantee about the power-control mechanism only; a
   device-specific interface for properties outside it is a separate, additive
   possibility (see Background) and is not ruled out here. This guarantee is
   about adding a _type_; a new _sequence template_ that only recombines
   existing operations follows the same no-code path, but a genuinely new kind
   of operation is a phosphor-state-manager code change regardless of type (see
   Sequence Execution).
6. Per-type requirements such as host dependency are applied per device type
   through configuration rather than being imposed on every type by the
   framework. Host dependency itself is handled inside the daemon using the
   existing `Inventory.Decorator.ManagedHost` interface.
7. Expose the Redfish reset action for each device type in bmcweb. The action
   must only appear when a matching state D-Bus object exists.

## Proposed Design

Sub-device types share a single `State.Device` interface for power control
requests and current state. Existing `State.Host` and `State.Chassis` retain
their established transition interfaces; the same pattern could be adopted for
those types in a future revision.

### Daemon Architecture

A single long-lived, single-instance daemon serves every sub-device type. It is
type-agnostic: every type binds to the same interface, with one property set and
one pair of enums, so nothing in the daemon's D-Bus layer varies by type.

The daemon handles:

- entity-manager `InterfacesAdded` and `InterfacesRemoved` subscription plus an
  initial `GetManagedObjects` scan
- D-Bus state object lifecycle keyed on inventory path
- `Association.Definitions` publication with
  `(inventory, state, <inventory path>)` tuples
- operation sequence execution and systemd `StartUnit` invocation for
  transition-target notification (see Sequence Execution, below)
- state-query sequence execution for initial-state resolution on daemon startup
  (see Sequence Execution, below)
- host-dependency evaluation, using one `CurrentHostState` subscription shared
  across all device types rather than one per type

What remains type-specific is a table of configuration data in
phosphor-state-manager, not code. Each entry supplies:

- the inventory item interface to watch (e.g. `Inventory.Item.Drive`)
- the systemd target name prefix (e.g. `obmc-drive-`), from which the transition
  target names are derived. Deriving the full name from a prefix assumes every
  type uses the same fixed suffix set (poweron, poweroff, reboot, hard-reboot,
  powercycle); a type that genuinely needs a different target set is outside
  this table's shape.
- whether a host-dependency check applies to that type
- the set of transitions to advertise in `AllowedDeviceTransitions` for that
  type. Which operation sequence implements a given transition for a given
  instance is supplied per-instance through entity-manager (see Sequence
  Execution, below). `AllowedDeviceTransitions` itself is still fixed per type
  at build time, since it describes what the mechanism supports for a type in
  general, not per-instance configuration. `State.Host`'s existing
  `AllowedHostTransitions` is populated the same way for the same reason.

Discovery through entity-manager creates per-device state objects at runtime
within the single process, so there is no systemd template unit per device
instance. The first supported type is drives. Adding a new device type (NIC,
PCIe card) to power control adds a table entry and the corresponding target
units; it requires no new interface, no new binary, and no change to existing
types.

### Sequence Execution

A transition is implemented as an ordered sequence of operations drawn from a
closed set the daemon defines: setting or reading a GPIO, waiting for a GPIO to
reach a value within a timeout, getting or waiting on a D-Bus property, sending
a command to a terminus, sleeping, and acquiring or releasing a named
concurrency slot. The daemon interprets and runs a sequence directly; a platform
cannot add a new kind of operation, only compose the existing ones in a
different order.

A named concurrency slot bounds how many operations referencing the same group
may run at once, for reasons such as a platform-defined electrical or resource
limit. The boundary a group name represents is entirely a platform decision
expressed in its own configuration; the daemon only enforces the limit for
whatever grouping it is given.

A sequence definition is a named template stored in phosphor-state-manager, in a
format the daemon defines. Entity-manager supplies, per instance: which template
applies, and that instance's parameters (GPIO line names, terminus target,
timing values, concurrency group). Putting the step list directly into the
entity-manager schema instead was considered; it would remove the template layer
entirely, but it requires entity-manager's maintainers to accept an ordered step
list as configuration data, which this design has not confirmed, so it keeps the
step list in phosphor-state-manager's own repository instead.

For illustration, not as a final serialization format, a drive poweron template
might read:

```text
set_gpio(${reset}, 0)
set_gpio(${power_enable}, 1)
sleep(45ms, group: ${concurrency_group})
wait_gpio(${power_good}, 1, timeout: 500ms)
sleep(100ms)
set_gpio(${reset}, 1)
```

`${reset}`, `${power_enable}`, `${power_good}`, and `${concurrency_group}` are
placeholders the daemon resolves against the instance's entity-manager
parameters; the template itself carries no platform-specific value.

An operation that genuinely has no matching primitive may run a named
platform-provided unit as an explicit step; the daemon still owns the sequence
around it, tracks the result, and continues or fails accordingly.

`Transition.PowerCycle` has no template of its own: the daemon runs the
instance's poweroff sequence followed by its poweron sequence. Whatever a
device needs before it is safe to reapply power, whether that is polling a
real signal or simply a bounded wait, belongs in the poweron template as its
own first step, not in the poweroff template and not as separate
`PowerCycle`-only data. The poweron template is the one point every path to
powering the device back on runs through, whether the request was
`PowerCycle`, a standalone `On`, or independent `Off` and `On` writes from a
caller; a safety condition only enforced on the `PowerCycle` path would not
hold on the other two. A poweron template expresses this with `wait_gpio`
against a real settle signal where one exists, which resolves immediately if
the device was already off long enough and only waits when power was just
removed; a device with no such signal falls back to an unconditional `sleep`
at the same position, at the cost of that wait applying even when the device
had already been off for a while.

### Integration with entity-manager

The daemon discovers devices through entity-manager. Platforms provide Board
entities with the appropriate `Inventory.Item.<Type>` marker interface.

entity-manager supports multiple detection mechanisms that platforms choose
based on their hardware:

- GPIO presence pins via entity-manager's `gpio-presence` sub-binary, publishing
  `Inventory.Source.DevicePresence` interfaces
- FRU EEPROM data via FruDevice scanner

The daemon is agnostic to the detection mechanism. It only watches for the
resulting `Inventory.Item.<Type>` interfaces that entity-manager publishes after
a successful probe. Empty slots produce no detection signal, so entity-manager
creates no inventory object, the daemon creates no state object, and bmcweb
shows no Redfish resource.

Beyond the marker interface, the daemon reads only
`Inventory.Decorator.ManagedHost` from the Board entity, and only for types
whose table entry applies the host-dependency check.

### Inventory-to-State Mapping

Each state object publishes a D-Bus association:

```text
("inventory", "state", <inventory path>)
```

ObjectMapper creates the reverse endpoint. bmcweb follows the association from
the inventory path's `/state` endpoint to locate the state object. This is
consistent with existing bmcweb association patterns used for chassis-to-drive,
sensor-to-inventory, and PCIe device-to-slot lookups.

### Device Type: Drive

The drive entry in the daemon's type table watches `Inventory.Item.Drive`, uses
the `obmc-drive-` target prefix, and applies the host-dependency check. The
daemon creates one `State.Device` object per discovered drive.

**Configuration in entity-manager:**

Platforms provide a Board entity with `Inventory.Item.Drive`. All PDI-defined
properties are included so that entity-manager fully populates the D-Bus
interface.

```json
{
  "Name": "nvme0",
  "Probe": "xyz.openbmc_project.Inventory.Source.DevicePresence({'Name': 'MB E1S0'})",
  "Type": "Board",
  "xyz.openbmc_project.Inventory.Item.Drive": {
    "Capacity": 0,
    "Protocol": "NVMe",
    "Type": "SSD",
    "EncryptionStatus": "Unknown",
    "LockedStatus": "Unknown",
    "PredictedMediaLifeLeftPercent": 255,
    "Resettable": true
  },
  "xyz.openbmc_project.Inventory.Decorator.ManagedHost": {
    "HostIndex": 0
  },
  "xyz.openbmc_project.Configuration.PowerSequence": {
    "PowerOnTemplate": "drive-poweron-gpio-basic",
    "PowerOffTemplate": "drive-poweroff-gpio-basic",
    "Reset": "DRIVE0_RESET_N",
    "PowerEnable": "DRIVE0_PWR_EN",
    "PowerGood": "DRIVE0_PWR_GOOD",
    "ConcurrencyGroup": "drive_bank_0"
  }
}
```

`PowerOnTemplate` and `PowerOffTemplate` name templates defined in
phosphor-state-manager (the same kind of template shown under Sequence
Execution); this block does not repeat their step lists. `Reset`,
`PowerEnable`, `PowerGood`, and `ConcurrencyGroup` are this instance's values
for the `${reset}`, `${power_enable}`, `${power_good}`, and
`${concurrency_group}` placeholders those templates reference, shared by both
templates since poweron and poweroff act on the same physical lines in a
different order. `Reboot` and `HardReboot` are not shown here because a drive
does not need them: both default to the `PowerOnTemplate`/`PowerOffTemplate`
composition described under Systemd targets. A device whose reboot needs to
differ from a plain power cycle would add `RebootTemplate` and/or
`HardRebootTemplate` fields naming their own templates, the same way
`PowerOnTemplate` and `PowerOffTemplate` are named here.

A corresponding entity-manager schema change is needed to accept
`Inventory.Item.Drive` and `Configuration.PowerSequence` as Board interfaces.

**D-Bus object:**

```text
/xyz/openbmc_project/state/device/<instance>
  xyz.openbmc_project.State.Device
    RequestedDeviceTransition (enum: On, Off, Reboot, HardReboot, PowerCycle,
                               None)
    AllowedDeviceTransitions  (set[enum], const)
    CurrentDeviceState        (enum: Off, TransitioningToOff, On,
                               TransitioningToOn, Unknown; readonly)
  xyz.openbmc_project.Association.Definitions
    [("inventory", "state", <inventory path>)]
```

The object path namespace is shared by every device type, so `<instance>` must
be unique across all sub-devices on a platform rather than only within one type.
The daemon takes it from the entity-manager inventory object the state object
was created for, which makes platform inventory naming the single place that
uniqueness is enforced.

`AllowedDeviceTransitions` is const and describes the transitions a device is
capable of, with an empty set meaning all transitions are supported. It is not a
statement about whether a transition will succeed right now: host dependency and
transient conditions are evaluated when the request arrives, so a transition may
be advertised here and still be rejected with an error.

**Host dependency:**

Drives depend on host power state. When the host is off, the drive state must
reflect off, and a request to power on the drive must return an error.

A Board entity may include the existing `Inventory.Decorator.ManagedHost`
interface with a `HostIndex` property identifying which host the device belongs
to. This is the same interface already defined in entity-manager schemas and
consumed by bmcweb for system collection enumeration. Board entities without
`ManagedHost` receive no host-dependency check and behave as host-independent
devices.

The daemon monitors `CurrentHostState` for the host indicated by `HostIndex`.
When the host transitions to Off, the daemon initiates a poweroff transition for
all devices associated with that host, updating `CurrentDeviceState` to `Off`.
This is built-in behavior that requires no platform-specific systemd wiring.
When a `RequestedDeviceTransition` of `On`, `Reboot`, `HardReboot` or
`PowerCycle` is received while the associated host is off, the write is rejected
with `xyz.openbmc_project.Common.Error.NotAllowed`, which bmcweb surfaces as a
Redfish 4xx response. A transition refused only temporarily, for example while a
firmware update holds the device, is rejected with
`xyz.openbmc_project.Common.Error.Unavailable`, and a write arriving before the
BMC is ready with `xyz.openbmc_project.State.Device.Error.BMCNotReady`.

Systemd target dependencies alone cannot deliver a D-Bus error response because
a unit job failure does not surface as a property-set failure. The daemon-level
approach handles both automatic poweroff on host shutdown and transition
rejection in one place.

**Systemd targets:**

The following systemd targets are installed by the phosphor-state-manager recipe
for the drive type. All are template units parameterized by device instance
name. Target names keep the device type in the prefix, which gives platforms and
other units a stable per-type hook point for dependency ordering; the prefix
comes from the daemon's type table, so a new type follows the same naming
scheme.

Target-unit stub files and sequence templates are shipped unconditionally in
every image, the same way entity-manager already ships every platform's
configuration file in every image; runtime discovery, not a build-time recipe
list, determines which instances and templates are actually used.

Transition targets, one per `Transition` value, each started by the daemon
once that transition's own sequence completes (see Sequence Execution):

```text
obmc-drive-poweron@.target      Started on Transition.On
obmc-drive-poweroff@.target     Started on Transition.Off
obmc-drive-reboot@.target       Started on Transition.Reboot
obmc-drive-hard-reboot@.target  Started on Transition.HardReboot
obmc-drive-powercycle@.target   Started on Transition.PowerCycle
```

`Transition.PowerCycle` runs the poweroff sequence followed by the poweron
sequence (see Sequence Execution). `Transition.Reboot` and
`Transition.HardReboot` default to that same composition for the drive type,
resolved per instance from the daemon's type table, the same way
phosphor-state-manager's existing host daemon maps
`GracefulWarmReboot`/`ForceWarmReboot` onto the plain reboot target when a
platform does not support a distinct warm reboot: a table entry the daemon
resolves, not a systemd unit dependency. A device whose reboot genuinely
differs from a power cycle, for example a terminus command instead of
toggling power, supplies its own `RebootTemplate` or `HardRebootTemplate` in
entity-manager for that instance, overriding the default in the same table.
No target declares `Wants=` on another target for this: each target is
started only after the daemon has already run whatever sequence that
transition resolved to, so a dependency between targets would not do
anything the daemon's own resolution has not already done.

Unlike a platform-authored hook target, these are not where the hardware
operation happens. The daemon runs the operation sequence itself (see Sequence
Execution) and starts the matching target once the sequence completes, so other
units that depend on a target still see it become active at the right time. A
sequence step may declare a named hook slot (`on_complete`, `on_failure`) that
the daemon starts as part of the same job, which is where a platform-specific
companion action, such as a recovery service after a failure, still attaches
without becoming part of the default path.

The daemon updates `CurrentDeviceState` directly from the operation sequence's
own result rather than from a systemd job signal: `On` or `Off` once the
sequence completes successfully, an error state if it fails. While a sequence is
running the daemon reports `TransitioningToOn` or `TransitioningToOff`.

**Initial state on daemon startup:**

On startup, for each discovered instance the daemon runs that device type's
state-query sequence, an ordered list using the same operation vocabulary,
typically a single read such as a power-good line, through the same
concurrency-group broker a transition would use, so it cannot race a live
transition on the same resource. `CurrentDeviceState` is `Unknown` until that
read completes.

**bmcweb integration:**

The Drives collection continues to be enumerated from `Inventory.Item.Drive`
objects in the existing inventory subtree, so empty slots stay absent from it
for the reason given under Integration with entity-manager.

On a Drive GET, bmcweb follows the `/state` association from the inventory
object. If a state endpoint exists, it reads `CurrentDeviceState` and injects
the `Actions.#Drive.Reset` block into the Redfish response. If no state
association exists, the action is omitted, ensuring platforms that do not deploy
the daemon are unaffected.

`DeviceState` has the same shape as the existing `State.Chassis` `PowerState`
enum, so `Status.State` reuses the mapping bmcweb already applies to a chassis:

```text
CurrentDeviceState   Redfish Status.State
-------------------  --------------------
On                   Enabled
Off                  StandbyOffline
TransitioningToOff   StandbyOffline
TransitioningToOn    Starting
```

`Unknown` means the daemon could not determine the device state, so no
`Status.State` is derived from it and whatever bmcweb already reports from other
sources is left in place. Unlike Chassis, the Redfish Drive schema has no
`PowerState` property, so device state is reflected only through `Status.State`.

For `Drive.Reset` POST, bmcweb follows the association to the state object and
sets `RequestedDeviceTransition` on the `State.Device` interface. The Redfish
`ResetType` to D-Bus transition mapping:

```text
ResetType            D-Bus Transition
-------------------  -------------------------
On                   Transition.On
ForceOff             Transition.Off
GracefulRestart      Transition.Reboot
ForceRestart         Transition.HardReboot
PowerCycle           Transition.PowerCycle
```

A `ResetActionInfo` GET route returns `AllowableValues` derived from the
`AllowedDeviceTransitions` property on the state object, treating an empty set
as every transition being allowed. A listed `ResetType` can still be refused at
request time, as described above, surfacing as a Redfish 4xx rather than being
filtered out of `AllowableValues`.

## Alternatives Considered

**One binary per device type**, each built on a shared common layer, as
dbus-sensors does for sensor types.

The deciding question is whether a device type owns anything a separate process
needs to enclose.

- Per-type binaries: with a separate `State.*` interface per type, each type
  would carry its own generated D-Bus binding, with its own property names,
  transition enum and state enum, and the type-specific behaviour around that
  binding would live in the binary. The entity-manager subscription, event loop,
  association lifecycle and `CurrentHostState` subscription are replicated per
  process.
- Single type-agnostic daemon (chosen): with one `State.Device` interface for
  every type (see Background), no per-type binding remains, and the
  type-specific behaviour reduces to the configuration data in the daemon's type
  table (see Daemon Architecture), with the individual devices discovered from
  entity-manager at runtime. A process boundary per type would therefore not be
  separating distinct logic. One subscription, event loop and association
  lifecycle regardless of how many types are supported, which also reduces
  compressed image size (duplicated code defeats `-Osize`) and per-process
  overhead.

This choice may be revisited if a future device type needs its own
device-specific interface, since that type would then carry its own generated
bindings and the reasoning above would no longer hold for it.

**Compiled-in hardware control via abstract base class.** The trade-off is
whether platform hardware operations live inside the daemon binary, behind a
platform-authored systemd hook service, or as data interpreted by a closed,
daemon-defined operation vocabulary.

- Compiled-in: platform-specific operations (GPIO, I2C) are compiled directly
  into the daemon binary. Each platform implements an abstract base class in a
  per-platform sub-directory, and the common code calls into it at build time or
  via runtime detection. This gives full static analysis coverage, keeps the
  whole path refactorable in one tree, and avoids starting a process for an
  operation as small as a GPIO write. A similar pattern is used in
  `phosphor-bmc-code-mgmt`, where common code calls into device-specific update
  implementations and per-instance parameters such as bus, address and GPIO line
  names come from entity-manager configuration. The cost is a new compiled class
  for every new sequence shape, even one that only recombines operations that
  already exist for another shape.
- Platform-authored systemd hook services: the daemon starts a target with no
  logic of its own, and a platform attaches a service to it via `.wants/` in its
  meta-layer to perform the actual hardware sequence. This keeps platform
  hardware code out of the phosphor-state-manager source tree entirely, at the
  cost of moving the sequence itself into a platform-authored script or service
  outside the daemon's own control flow, error handling, and event logging, and
  of an extension point every platform must independently wire.
- A closed, daemon-defined operation vocabulary (chosen, see Sequence
  Execution): existing sub-device and chassis power paths decompose into a
  bounded set of primitives: assert or read a GPIO, wait for hardware to settle,
  poll status or presence with a retry and a timeout, serialize a group of
  operations against a platform-defined limit, and occasionally reach the device
  through a terminus rather than a local line. That is a bounded vocabulary, not
  arbitrary platform logic, so it can be expressed as an ordered list of
  operations rather than requiring either a new compiled class per sequence
  shape or a platform-authored service outside the daemon's control. The cost is
  that composing a sequence is schema-checked rather than compile-checked, so
  the maintainer overview a compiled implementation gives is not identical,
  though similar in practice for a flat, unbranching list.

This does not rule out a built-in mechanism later for devices whose power
control genuinely is a single line write or a single standard command. Such a
mechanism could be added as an additional path, configured by data, without
changing the interface or the target layout described here. This design only
declines to make it the required path for every platform.

**Map drives as additional chassis or host instances.** Reuses existing daemons
and services without new code, but produces incorrect Redfish semantics (a drive
is not a chassis) and requires bmcweb workarounds to filter the spurious
resources from Redfish collections.

**Per-type transition interface for each sub-device type.** Each sub-device type
would define its own transition property (e.g. `RequestedDriveTransition` in
`State.Drive`), following the existing per-type PDI convention (State.BMC,
State.Host, State.Chassis each with independent transition property names).
Redfish instead uses a single `ResetType` enum (defined in the Resource schema)
consistently across all device types (Drive, Port, Processor, PowerSupply,
Switch, and others), and a per-type interface would push that uniformity back
onto every consumer: bmcweb would need a branch per device type to pick the
right interface, property name and enum, and each new type would need a
phosphor-dbus-interfaces change before it could be managed at all. This design
therefore uses one `State.Device` interface.

A device type that genuinely needs its own state properties, such as the
existing `Rebuilding` on `State.Drive`, would be served by a separate
device-specific interface rather than by reintroducing per-type transition
properties (see Background).

**Build-time instance configuration via systemd template instances.** Needs no
entity-manager coupling, but cannot represent dynamic presence: empty slots
would still produce D-Bus objects and Redfish resources unless the platform
statically tracks population per build.

**Host dependency via systemd target dependencies alone.** Uses only the systemd
wiring layer and needs no entity-manager metadata, but cannot translate a failed
host-dependency gate into a D-Bus error: a unit job failure does not surface as
a property-set failure, so bmcweb clients receive a silently successful response
while the device state remains unchanged. The chosen design handles host
dependency entirely at the daemon level, as described in the Device Type: Drive
section.

## Impacts

phosphor-dbus-interfaces gains a new `State.Device` interface
(`RequestedDeviceTransition`, `AllowedDeviceTransitions`, `CurrentDeviceState`)
together with its `BMCNotReady` error. This design takes no position on the
future of `State.Drive`; that interface's lifecycle is phosphor-dbus-interfaces'
concern.

phosphor-state-manager gains a sub-device state daemon: entity-manager
discovery, association lifecycle, systemd orchestration, host-dependency
handling, and a table of supported device types with the drive entry as its
first member. No changes to existing host, chassis, or BMC daemons. Future
sub-device types (NIC, PCIe) add a table entry and target units to the same
daemon.

entity-manager schemas are extended to accept
`xyz.openbmc_project.Inventory.Item.Drive` as a top-level Board interface. Host
association uses the existing `Inventory.Decorator.ManagedHost` interface
(already defined in entity-manager schemas), so no new schema properties are
required for host association.

bmcweb gains Drive.Reset routes under the existing Storage hierarchy, resolving
the state object through the `/state` association on the drive inventory path.
The reset handler writes `RequestedDeviceTransition` on `State.Device`, so
future sub-device types reuse the same handler without new bmcweb code. No
existing Redfish routes are affected. The Drive.Reset action is absent on
systems that do not deploy the daemon.

Platforms provide entity-manager Board configuration for each device slot; a
platform whose hardware needs a sequence shape not already covered contributes a
new named template to phosphor-state-manager rather than authoring one in its
own meta-layer. Platform-specific early-boot initialization for drive power
hardware (e.g., GPIO direction setup) belongs in the platform's platform-init
implementation, not in this daemon.

### Organizational

- Does this proposal require a new repository? No.
- Which repositories are expected to be modified?
  - `phosphor-dbus-interfaces` (new `State.Device` interface and its
    `BMCNotReady` error)
  - `phosphor-state-manager` (sub-device state daemon, its systemd target units,
    and the sequence template set)
  - `bmcweb` (Drive.Reset action, association-based state lookup)
  - `entity-manager` (schema: allow `Inventory.Item.Drive` as Board interface;
    platform entity-manager configurations for device slots also live here)
  - `openbmc` (platform-init hardware setup where applicable)

## Testing

The phosphor-state-manager implementation will be verified on a single-host
platform, covering:

- entity-manager discovery creating a state object when a device is probed
  present and removing it when the device is removed, with no object and no
  Redfish resource for an unpopulated slot
- each transition running its operation sequence, with `CurrentDeviceState`
  reaching `On` or `Off` once the sequence completes
- a transition requested while the associated host is off being rejected with
  `xyz.openbmc_project.Common.Error.NotAllowed`, and the device being powered
  off automatically when the host transitions to off
- the Redfish `Drive.Reset` action round trip through bmcweb, and
  `ResetActionInfo` reporting `AllowableValues` matching
  `AllowedDeviceTransitions`

A multi-host platform additionally exercises the
`Inventory.Decorator.ManagedHost` path, where devices belonging to different
hosts must follow only their own host's power state.
