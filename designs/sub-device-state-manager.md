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
   control, NVMe-MI commands, etc.) from state management logic. The current
   design uses systemd targets as the extension point: platforms attach hardware
   control services via `.wants/` directories.
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
   possibility (see Background) and is not ruled out here.
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
- systemd `StartUnit` invocation and `JobRemoved` signal handling
- marker-target initial-state resolution on daemon startup
- host-dependency evaluation, using one `CurrentHostState` subscription shared
  across all device types rather than one per type

What remains type-specific is a table of configuration data in
phosphor-state-manager, not code. Each entry supplies:

- the inventory item interface to watch (e.g. `Inventory.Item.Drive`)
- the systemd target name prefix (e.g. `obmc-drive-`), from which the transition
  and marker target names are derived. Deriving the full name from a prefix
  assumes every type uses the same fixed suffix set (poweron, poweroff, reboot,
  hard-reboot, powercycle, powered-on, powered-off); a type that genuinely needs
  a different target set is outside this table's shape.
- whether a host-dependency check applies to that type
- the set of transitions to advertise in `AllowedDeviceTransitions` for that
  type. The actual hardware sequence for a transition is implemented by a
  platform-authored target hook (see Systemd targets, below), so whether a
  transition is meaningful for a type on a given platform is fixed once that
  platform's hooks are built, not something entity-manager can report or the
  daemon can probe at runtime. This value is therefore build-time and applies
  uniformly to every instance of that type in a given image, not read
  per-instance from entity-manager. `State.Host`'s existing
  `AllowedHostTransitions` is populated the same way for the same reason.

Discovery through entity-manager creates per-device state objects at runtime
within the single process, so there is no systemd template unit per device
instance. The first supported type is drives. Adding a new device type (NIC,
PCIe card) to power control adds a table entry and the corresponding target
units; it requires no new interface, no new binary, and no change to existing
types.

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
  }
}
```

A corresponding entity-manager schema change is needed to accept
`Inventory.Item.Drive` as a Board interface.

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

Which device types' target units a given image installs is still open; see
Alternatives Considered.

Transition targets (started by the daemon on `RequestedDeviceTransition`):

```text
obmc-drive-poweron@.target      Started on Transition.On
obmc-drive-poweroff@.target     Started on Transition.Off
obmc-drive-reboot@.target       Started on Transition.Reboot
obmc-drive-hard-reboot@.target  Started on Transition.HardReboot
obmc-drive-powercycle@.target   Started on Transition.PowerCycle
```

Marker targets (checked on daemon startup to determine initial state):

```text
obmc-drive-powered-on@.target   Active state -> CurrentDeviceState = On
obmc-drive-powered-off@.target  Active state -> CurrentDeviceState = Off
```

If neither marker is active, which is the expected situation on a first boot
before any transition has run, the initial state is `Unknown`.

Target dependencies:

```text
obmc-drive-reboot@.target
  Wants=obmc-drive-powercycle@%i.target         (default)

obmc-drive-hard-reboot@.target
  Wants=obmc-drive-powercycle@%i.target         (default)
```

The composite targets (`reboot`, `hard-reboot`, `powercycle`) are empty hook
targets. Platforms attach services via `.wants/` to implement the actual
hardware sequence. Platforms may attach additional services to the reboot
targets to differentiate graceful from hard reboot (e.g. an NVMe-MI shutdown
notification before power-off for graceful reboot).

The daemon monitors `JobRemoved` signals for `poweron` and `poweroff` targets to
update `CurrentDeviceState`: poweron completion sets `On`, poweroff completion
sets `Off`. While a transition job is outstanding the daemon reports
`TransitioningToOn` or `TransitioningToOff`. Composite transitions ultimately
trigger poweron through systemd dependency ordering, so no separate handler is
needed.

**Platform systemd target hooking:**

Platforms provide the actual power control by attaching services to targets in
their openbmc meta-layer:

```text
obmc-drive-poweron@.target
  .wants/drive-poweron@.service     (platform-specific: GPIO, I2C, etc.)

obmc-drive-poweroff@.target
  .wants/drive-poweroff@.service    (platform-specific: GPIO, I2C, etc.)
```

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
whether platform hardware operations live inside the daemon binary or behind
systemd targets.

- Compiled-in: platform-specific operations (GPIO, I2C, NVMe-MI) are compiled
  directly into the daemon binary. Each platform implements an abstract base
  class in a per-platform sub-directory, and the common code calls into it at
  build time or via runtime detection. This gives full static analysis coverage,
  keeps the whole path refactorable in one tree, and avoids starting a process
  for an operation as small as a GPIO write. A similar pattern is used in
  `phosphor-bmc-code-mgmt`, where common code calls into device-specific update
  implementations and per-instance parameters such as bus, address and GPIO line
  names come from entity-manager configuration.
- Systemd targets (chosen): platform hardware code stays out of the
  phosphor-state-manager source tree. Platforms attach services to
  framework-defined targets in their meta-layer without modifying
  phosphor-state-manager, at the cost of losing compile-time type safety across
  the boundary and of each platform having to author that wiring.

The deciding factor is what a real platform's sequence contains. Existing
sub-device and chassis power paths derive controller indices from board
topology, resolve the controlling GPIO chip by inspecting sysfs at runtime,
branch on board variant, wait for hardware to settle, read status back and log
an error record on failure, and in multi-host designs reach the device through a
satellite controller over an OEM protocol rather than a local line. That is
platform logic, not a parameterisation of a common operation, so moving it into
phosphor-state-manager would mean carrying per-platform behaviour upstream, and
expressing it as configuration data would mean inventing a sequencing language
in that configuration.

This does not rule out a built-in mechanism later for devices whose power
control genuinely is a single line write or a single standard command. Such a
mechanism could be added as an additional path, configured by data, without
changing the interface or the target layout described here. This design only
declines to make it the required path for every platform.

**Which device types' target units a given image installs.** This is an open
question this design does not resolve, on top of the per-type target naming
above: the person building a platform's firmware image is not always the person
who knows what hardware will eventually be attached, and how that should shape
the answer still needs discussion.

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
properties. Which namespace such an interface belongs in, and which daemon hosts
it, is undecided and out of scope here; it needs its own design discussion.

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

Platforms provide entity-manager Board configuration for each device slot and
systemd target hook services for hardware-specific power control.
Platform-specific early-boot initialization for drive power hardware (e.g., GPIO
direction setup) belongs in the platform's platform-init implementation, not in
this daemon.

### Organizational

- Does this proposal require a new repository? No.
- Which repositories are expected to be modified?
  - `phosphor-dbus-interfaces` (new `State.Device` interface and its
    `BMCNotReady` error)
  - `phosphor-state-manager` (sub-device state daemon and its systemd target
    units)
  - `bmcweb` (Drive.Reset action, association-based state lookup)
  - `entity-manager` (schema: allow `Inventory.Item.Drive` as Board interface;
    platform entity-manager configurations for device slots also live here)
  - `openbmc` (platform systemd target hook services, and platform-init hardware
    setup where applicable)

## Testing

The phosphor-state-manager implementation will be verified on a single-host
platform, covering:

- entity-manager discovery creating a state object when a device is probed
  present and removing it when the device is removed, with no object and no
  Redfish resource for an unpopulated slot
- each transition starting the matching target, with `CurrentDeviceState`
  reaching `On` or `Off` once the job completes
- a transition requested while the associated host is off being rejected with
  `xyz.openbmc_project.Common.Error.NotAllowed`, and the device being powered
  off automatically when the host transitions to off
- the Redfish `Drive.Reset` action round trip through bmcweb, and
  `ResetActionInfo` reporting `AllowableValues` matching
  `AllowedDeviceTransitions`

A multi-host platform additionally exercises the
`Inventory.Decorator.ManagedHost` path, where devices belonging to different
hosts must follow only their own host's power state.
