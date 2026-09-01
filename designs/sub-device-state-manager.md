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
through entity-manager and executes each transition itself through a closed
operation vocabulary, rather than delegating the hardware operation to a
platform-authored systemd hook service. The first supported type is NVMe drives.

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
design has no equivalent hardware-access split: hardware-specific bindings and
operation sequences are supplied by the platform as configuration, while the
common daemon interprets and executes them; see Alternatives Considered for why
it therefore uses a single daemon rather than one per type.

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
   closed, daemon-defined vocabulary (GPIO set/get/check, D-Bus property
   get/check, a terminus command, sleep, a named concurrency group, and a
   bounded retry around a sub-sequence); a platform supplies each instance's
   ordered sequence of operations directly in its entity-manager configuration,
   but cannot introduce a new kind of operation.
4. Each state object must maintain a D-Bus association to its inventory object
   so that bmcweb can locate the state from inventory without relying on path
   naming conventions.
5. All sub-device types share a single `State.Device` interface for transition
   requests and current state, consistent with Redfish using one `ResetType`
   enum across all device schemas. Adding a new type (NIC, PCIe card) to this
   power-control mechanism must not require a new D-Bus interface, a new daemon,
   or changes to how existing types work: it supplies the inventory interface to
   watch and whether a host-dependency check applies; the transition-
   notification targets (see Systemd targets) are generic across every type and
   need no per-type naming. This is a guarantee about the power-control
   mechanism only; a device-specific interface for properties outside it is a
   separate, additive possibility (see Background) and is not ruled out here.
   Put differently, this is about adding a _type_; an instance whose sequence
   only recombines existing operations needs only its own entity-manager
   configuration, but a genuinely new kind of operation is a
   phosphor-state-manager code change regardless of type (see Sequence
   Execution).
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
- whether a host-dependency check applies to that type
- the set of transitions to advertise in `AllowedDeviceTransitions` for that
  type. Which operation sequence implements a given transition for a given
  instance is supplied per-instance through entity-manager (see Sequence
  Execution, below). `AllowedDeviceTransitions` is fixed at build time, not
  per-instance, but not fixed once for every platform either: it is a meson
  option a platform's own recipe can override, the same mechanism `State.Host`'s
  existing `AllowedHostTransitions` already uses via `ENABLE_WARM_REBOOT` to let
  one platform declare a transition unsupported without affecting every other
  platform linking the same binary.

Discovery through entity-manager creates per-device state objects at runtime
within the single process, so there is no systemd template unit per device
instance. The first supported type is drives. Adding a new device type (NIC,
PCIe card) to power control adds a table entry and the corresponding target
units; it requires no new interface, no new binary, and no change to existing
types.

### Sequence Execution

A transition is implemented as an ordered sequence of operations drawn from a
closed set the daemon defines: setting or reading a GPIO, checking a GPIO
against an expected value, reading or checking a D-Bus property against an
expected value, sending a command to a terminus, sleeping, and acquiring or
releasing a named concurrency group. A check is a single read: whether it
matches is what a wrapping `retry` (below) responds to, there is no separate
operation that waits or polls internally. The daemon interprets and runs a
sequence directly; a platform cannot add a new kind of operation, only compose
the existing ones in a different order.

A named concurrency group bounds how many sequences may hold it at once, for
reasons such as a platform-defined electrical or resource limit (an over-current
condition, contention on a shared bus, and similar). `AcquireGroup` takes the
group name, a required wait timeout, and an effective maximum-concurrency value
that defaults to one when omitted. The daemon validates this across every
discovered instance's configuration up front, not at whichever transition
happens to run `AcquireGroup` first: every reference to the same group name must
declare the same effective maximum, and an inconsistent declaration is a
configuration error that prevents creating a state object only for the newly
discovered instance whose declaration disagrees, leaving already-created state
objects for that group unaffected, rather than something discovered only when
two sequences race at runtime. Omitting the maximum defaults it to one: the one
real precedent this design has checked (a board-wide drive-power lock) is
exactly this mutual-exclusion case, so a platform whose limit is genuinely
higher than one states it explicitly. A wait that exceeds its timeout is an
ordinary operation failure, handled the same fixed way as any other (see below),
not a platform-defined outcome. The boundary a group name represents beyond its
numeric limit is entirely a platform decision expressed in its own
configuration: a group can name a board-wide resource shared across instances,
or a single instance's own serialization, scoping it however the platform's own
sequences reference it; the daemon only enforces the limit for whatever grouping
it is given. When and how often a sequence explicitly acquires and releases a
group within its own steps is likewise the platform's choice; the one thing the
daemon guarantees on its own is that any group still held when a transition's or
state-query's top-level sequence ends, whether it succeeded, failed, or was
aborted, is released at that point, so a platform's configuration only chooses
when it releases early, never whether the daemon eventually gives a forgotten
acquisition back.

Each transition's sequence is entity-manager configuration data: an ordered list
of operations specific to that instance, in a format the daemon defines.
phosphor-state-manager's own repository holds only the closed operation
vocabulary and the engine that interprets whatever sequence an instance's
configuration supplies. The reasoning for keeping each instance's sequence with
its own configuration, rather than a shared definition kept elsewhere, is the
one given in Alternatives Considered: the same device type does not necessarily
use the same sequence on every platform, so a shared definition would either
need to be revisited per platform anyway or would not fit every platform that
used it. phosphor-state-manager remains the sole interpreter and executor
regardless, so control flow, error handling, and event logging stay in the
daemon no matter how many instances' configurations happen to look alike.

A bounded retry wraps a sub-list of operations, running it again after a fixed
delay up to a fixed count if it does not succeed, for cases such as repeatedly
waiting and rechecking a settle signal before failing outright. It is a fixed
loop over already-closed operations, not a general control-flow construct: a
platform cannot use it to introduce conditional branching on device-specific
data. Every retry iteration re-executes the entire sub-sequence, not only the
check that failed, including operations with side effects such as `SetGpio`; a
platform must place an operation inside a retry only when repeating it is safe
and intentional. A retry sub-sequence must not acquire a group a failed earlier
iteration may still be holding, since the daemon only releases a forgotten
acquisition when the top-level sequence ends, not between iterations; a group an
instance's sequence needs across every retry attempt belongs outside the retry,
acquired once before it and released once after. A retry that exhausts its count
is handled the same way any other operation failure is: the daemon logs it as an
error through its own fixed mapping, not a platform-supplied message, and, for a
transition sequence, resolves `CurrentDeviceState` as described under Systemd
targets, since a platform's configuration can only compose already-closed
operations, never define what a failure means or what state it leaves the device
in.

For illustration, not as a final serialization format, a drive poweron sequence,
as it would appear directly in that instance's entity-manager configuration,
might read:

```text
acquire_group(drive_bank_0, timeout: 30s)
set_gpio(reset, 0)
set_gpio(power_enable, 1)
retry(count: 10, backoff: 100ms) {
  sleep(45ms)
  check_gpio(power_good, 1)
}
release_group(drive_bank_0)
sleep(100ms)
set_gpio(reset, 1)
```

Every value here (GPIO line names, the concurrency group, timing) is this
instance's own physical binding, written for this platform. If an instance's
sequence genuinely needs an operation outside this vocabulary, that is a
phosphor-state-manager change to extend the vocabulary (see Requirement 5), not
a platform-supplied step.

`Transition.PowerCycle` has no sequence of its own by default: the daemon runs
the instance's poweroff sequence followed by its poweron sequence. Whatever a
device needs before it is safe to reapply power, whether that is polling a real
signal or simply a bounded wait, belongs in the poweron sequence as its own
first step, not in the poweroff sequence and not as separate `PowerCycle`-only
data. The poweron sequence is the one point every path to powering the device
back on runs through, whether the request was `PowerCycle`, a standalone `On`,
or independent `Off` and `On` writes from a caller; a safety condition only
enforced on the `PowerCycle` path would not hold on the other two. A poweron
sequence expresses this with a `retry`-wrapped `check_gpio` against a real
settle signal where one exists, which succeeds on the first check if the device
was already off long enough and only spends its retries when power was just
removed; a device with no such signal falls back to an unconditional `sleep` at
the same position, at the cost of that wait applying even when the device had
already been off for a while. An instance whose power cycle genuinely needs to
differ from that composition supplies its own `PowerCycleSequence` instead.

Unlike `PowerCycle`, `Reboot` and `HardReboot` have no daemon-applied default at
all: each instance's own `RebootSequence` and `HardRebootSequence` is the only
source for what those transitions do (see Systemd targets, below). A drive reset
is typically a short pulse on its own reset line, unrelated to removing and
reapplying power, so treating it as a variant of the poweroff-then-poweron
composition would be wrong for the common case, not an edge case an override
exists for.

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

The drive entry in the daemon's type table watches `Inventory.Item.Drive` and
applies the host-dependency check. The daemon creates one `State.Device` object
per discovered drive.

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
    "PowerOnSequence": [
      { "Op": "AcquireGroup", "Group": "drive_bank_0", "TimeoutMs": 30000 },
      { "Op": "SetGpio", "Pin": "DRIVE0_RESET_N", "Value": 0 },
      { "Op": "SetGpio", "Pin": "DRIVE0_PWR_EN", "Value": 1 },
      {
        "Op": "Retry",
        "Count": 10,
        "BackoffMs": 100,
        "Steps": [
          { "Op": "Sleep", "Ms": 45 },
          { "Op": "CheckGpio", "Pin": "DRIVE0_PWR_GOOD", "Value": 1 }
        ]
      },
      { "Op": "ReleaseGroup", "Group": "drive_bank_0" },
      { "Op": "Sleep", "Ms": 100 },
      { "Op": "SetGpio", "Pin": "DRIVE0_RESET_N", "Value": 1 }
    ],
    "PowerOffSequence": [
      { "Op": "SetGpio", "Pin": "DRIVE0_RESET_N", "Value": 0 },
      { "Op": "SetGpio", "Pin": "DRIVE0_PWR_EN", "Value": 0 }
    ]
  }
}
```

`PowerOnSequence` and `PowerOffSequence` are this instance's own ordered
operation lists (see Sequence Execution): every GPIO line name, timing value,
and concurrency group here is specific to this drive. `RebootSequence` and
`HardRebootSequence` are omitted because this example assumes the drive type's
build-time `AllowedDeviceTransitions` advertises only `On`, `Off` and
`PowerCycle`; a type that also advertises `Reboot` and/or `HardReboot` requires
every one of its instances to supply the matching `RebootSequence` and/or
`HardRebootSequence` field (see the `AllowedDeviceTransitions` description
above), the same way `PowerOnSequence` and `PowerOffSequence` are given here.

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
capable of, with an empty set meaning all of `On`, `Off`, `Reboot`, `HardReboot`
and `PowerCycle` are supported; `None` is never advertised here, since it is
only the idle value `RequestedDeviceTransition` reads back as before any
transition has been requested, not an actionable transition. It is not a
statement about whether a transition will succeed right now: host dependency and
transient conditions are evaluated when the request arrives, so a transition may
be advertised here and still be rejected with an error. It is, however, a
statement that some sequence exists for that transition, expanding an empty set
to the five transitions above first if needed: `On` requires `PowerOnSequence`,
`Off` requires `PowerOffSequence`, `Reboot` requires `RebootSequence`,
`HardReboot` requires `HardRebootSequence`, and `PowerCycle` requires either its
own `PowerCycleSequence` or both `PowerOnSequence` and `PowerOffSequence` (see
Sequence Execution for the default composition). The daemon rejects creating a
state object for an instance missing a sequence required by its type's
(expanded) `AllowedDeviceTransitions`, rather than advertising a transition it
cannot actually run.

**Host dependency:**

Drives associated with a host through `ManagedHost` are subject to that host's
power-state dependency: when the host transitions to Off, the daemon attempts to
power off every associated drive, and a request to power the drive back on
returns an error. Drives without `ManagedHost` are host-independent, as
described below. The host-triggered poweroff is, from the daemon's own
perspective, a sequence like any other: if it starts while an `On` sequence for
the same drive is still running, the same overlap rule from Sequence Execution
applies, and a platform whose in-flight `On` could otherwise outlast a
host-triggered `Off` should place both under a common group.

A Board entity may include the existing `Inventory.Decorator.ManagedHost`
interface with a `HostIndex` property identifying which host the device belongs
to. This is the same interface already defined in entity-manager schemas and
consumed by bmcweb for system collection enumeration. Board entities without
`ManagedHost` receive no host-dependency check and behave as host-independent
devices.

The daemon monitors `CurrentHostState` for the host indicated by `HostIndex`.
When the host transitions to Off, the daemon initiates a poweroff transition for
all devices associated with that host; a transition that completes successfully
sets `CurrentDeviceState` to `Off`, and one that fails follows the same
state-query reconciliation described under Systemd targets. This is built-in
behavior that requires no platform-specific systemd wiring. When a
`RequestedDeviceTransition` of `On`, `Reboot`, `HardReboot` or `PowerCycle` is
received while the associated host is off, the write is rejected with
`xyz.openbmc_project.Common.Error.NotAllowed`, which bmcweb surfaces as a
Redfish 4xx response. A transition refused only temporarily, for example while a
firmware update holds the device, is rejected with
`xyz.openbmc_project.Common.Error.Unavailable`, and a write arriving before the
BMC is ready with `xyz.openbmc_project.State.Device.BMCNotReady`.

Systemd target dependencies alone cannot deliver a D-Bus error response because
a unit job failure does not surface as a property-set failure. The daemon-level
approach handles both automatic poweroff on host shutdown and transition
rejection in one place.

A `RequestedDeviceTransition` write naming the same transition already running
for that instance is rejected with
`xyz.openbmc_project.Common.Error.Unavailable`, since it asks the daemon to run
a sequence that is already in flight for no additional effect. A write naming a
different transition while one is running is not rejected by the daemon itself:
the daemon starts that sequence too, and whatever ordering or exclusion the two
need, for example a plain `Off` request arriving mid-poweron, is left to the
same named concurrency group mechanism described in Sequence Execution, scoped
however the instance's own configuration chooses, including to the instance
itself. A `RequestedDeviceTransition` write of `On` or `Off` that already
matches `CurrentDeviceState`, with no sequence currently running for that
instance, is also a no-op: the daemon does not re-run the corresponding
sequence, and reports success immediately, without starting either the
transition target or (see below) the failure path, since no sequence was
attempted for this request. This applies only to `On` and `Off`, which are
requests to reach a state; `Reboot`, `HardReboot` and `PowerCycle` are requests
to perform an action and always run their sequence regardless of the device's
current state, since asking to reboot a device that happens to already be `On`
is not the same request as asking for it to be `On`. When an instance permits
overlapping sequences this way, by not placing them under a common group, each
sequence updates `CurrentDeviceState` according to its own progress and result
as it runs, and the most recent update wins, whichever sequence produced it; the
daemon does not impose per-instance serialization beyond the
identical-transition case. A platform is responsible for placing its sequences
under a shared group whenever overlapping them would leave either the physical
outcome or the reported state ambiguous.

**Systemd targets:**

The following systemd targets are installed by the phosphor-state-manager
recipe. All are template units parameterized by device instance name, and are
generic across every sub-device type rather than named per type: no confirmed
consumer depends on "any drive" or "any NIC" as a class through a type-specific
target name, only ever on one specific instance, so a type prefix would add a
namespace without a use. Every sub-device type, present or future, uses the same
five target names.

Target-unit stub files are shipped unconditionally in every image, the same way
entity-manager already ships every platform's configuration file in every image;
runtime discovery, not a build-time recipe list, determines which instances are
actually present.

Transition targets, one per actionable `Transition` value (`On`, `Off`,
`Reboot`, `HardReboot` and `PowerCycle`; `None` is never actionable and has no
target), each started by the daemon only once that transition's own sequence
completes successfully (see Sequence Execution). A sequence that fails does not
start the transition target; see below for what the daemon does instead.

```text
obmc-device-poweron@.target      Started on Transition.On
obmc-device-poweroff@.target     Started on Transition.Off
obmc-device-reboot@.target       Started on Transition.Reboot
obmc-device-hard-reboot@.target  Started on Transition.HardReboot
obmc-device-powercycle@.target   Started on Transition.PowerCycle
```

A drive named `nvme0` in its entity-manager configuration is therefore
identified as `obmc-device-poweron@nvme0.target`, not by a drive-specific target
name; the instance name, not the type, is what a consumer depends on.

`Transition.PowerCycle` runs the poweroff sequence followed by the poweron
sequence unless the instance supplies its own `PowerCycleSequence` (see Sequence
Execution); this is the one composition the daemon applies by default, because
power-cycling is defined as powering off and back on, not a platform-specific
fact. The default composed `PowerCycle` reports `TransitioningToOff` during the
poweroff phase and `TransitioningToOn` during the poweron phase, matching each
phase's own sequence; a custom `PowerCycleSequence`, which has no such phase
boundary, reports `TransitioningToOn` for its whole duration and `On` on
success, the same as `Reboot` and `HardReboot`. `Transition.Reboot` and
`Transition.HardReboot` have no such default: each instance's own
`RebootSequence` and `HardRebootSequence` determine what those transitions do,
the same way `PowerOnSequence` and `PowerOffSequence` do for `On`/`Off`, since
whether a reboot is safely a plain power cycle or needs its own sequence, for
example a terminus command instead of toggling power, is a real platform fact,
not one this design can assume either way.

Unlike a platform-authored hook target, these are not where the hardware
operation happens. The daemon runs the operation sequence itself (see Sequence
Execution) and starts the matching target only once the sequence completes
successfully, so other units that depend on a target see it become active only
when the transition genuinely succeeded, not merely attempted. The target is a
daemon-defined transition-completion notification, consistent with existing
phosphor-state-manager conventions for Host and Chassis; it is not part of the
hardware-control path, and this design does not require, recommend, or define
any platform-specific unit wiring for a sub-device transition to function.

A sequence that fails starts no target and offers no comparable attachment
point: the daemon logs the failure through its own fixed error/event mapping and
resolves `CurrentDeviceState` through the same state-query reconciliation used
elsewhere (see below). This design does not add a failure-notification target or
any other platform hook for this path; real precedent for one exists elsewhere
(Chassis's `OnFailure=chassis-poweron-failure@%i.service`), but no specific need
for it has been raised for sub-devices, and this design does not add an
extension point ahead of such a need.

The daemon updates `CurrentDeviceState` directly from the operation sequence's
own result rather than from a systemd job signal: `On` or `Off` once the
sequence completes successfully. While a sequence is running the daemon reports
`TransitioningToOn` or `TransitioningToOff`, one or the other depending on which
sequence is currently executing: for `PowerCycle`, this means
`TransitioningToOff` during the poweroff phase and `TransitioningToOn` during
the poweron phase that follows it, the same two values a plain `Off` or `On`
request would report on their own. `Reboot` and `HardReboot` report
`TransitioningToOn`, since neither transition leaves the device powered off at
any point a caller can observe. A sequence that fails does not by itself make
the daemon report `Unknown`: the daemon instead runs that device instance's
state-query sequence (see Initial state on daemon startup) to determine what
state the failed transition actually left the device in, since a failed
transition can still leave the device in a known `On` or `Off` state.
`CurrentDeviceState` becomes `Unknown` only under the same condition state-query
already leaves it `Unknown` on startup: the query itself could not determine the
state.

**Initial state on daemon startup:**

On startup, for each discovered instance the daemon runs that device instance's
state-query sequence, an ordered list using the same operation vocabulary,
typically a single read such as a power-good line. Whether this can race a live
transition on the same resource is, like any other cross-sequence ordering, up
to whether the instance's configuration references a shared group from both its
state-query and its transition sequences; the daemon does not impose one on its
own. `CurrentDeviceState` is `Unknown` until the state-query read completes.

**bmcweb integration:**

The Drives collection continues to be enumerated from `Inventory.Item.Drive`
objects in the existing inventory subtree, so empty slots stay absent from it
for the reason given under Integration with entity-manager.

On a Drive GET, bmcweb follows the `/state` association from the inventory
object. If a state endpoint exists, it reads `CurrentDeviceState` and injects
the `Actions.#Drive.Reset` block into the Redfish response. If no state
association exists, the action is omitted, ensuring platforms that do not deploy
the daemon are unaffected.

`DeviceState`'s `Off`, `TransitioningToOff`, `On` and `TransitioningToOn` values
match the existing `State.Chassis` `PowerState` enum, so `Status.State` reuses
the mapping bmcweb already applies to a chassis for those four values:

```text
CurrentDeviceState   Redfish Status.State
-------------------  --------------------
On                   Enabled
Off                  StandbyOffline
TransitioningToOff   StandbyOffline
TransitioningToOn    Starting
```

`Unknown`, the one `DeviceState` value with no `State.Chassis` `PowerState`
counterpart, means the daemon could not determine the device state, so no
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
  operations against a platform-defined concurrency limit, and occasionally
  reach the device through a terminus rather than a local line. That is a
  bounded vocabulary, not arbitrary platform logic, so it can be expressed as an
  ordered list of operations rather than requiring either a new compiled class
  per sequence shape or a platform-authored service outside the daemon's
  control. The cost is that composing a sequence is schema-checked rather than
  compile-checked, so the maintainer overview a compiled implementation gives is
  not identical, though similar in practice for a flat, unbranching list.

This does not rule out a built-in mechanism later for devices whose power
control genuinely is a single line write or a single standard command. Such a
mechanism could be added as an additional path, configured by data, without
changing the interface or the target layout described here. This design only
declines to make it the required path for every platform.

**A named, reusable sequence stored in phosphor-state-manager, selected by name
from entity-manager.** Entity-manager would supply a name (for example
`PowerOnTemplate: "drive-poweron-gpio-basic"`) plus this instance's physical
parameters, and phosphor-state-manager would hold the named definition, shared
across every instance that names it. This still keeps the operation vocabulary
closed and does not by itself put platform-authored scripts or systemd wiring
anywhere, so it would not violate the same control-flow ownership the chosen
design preserves. It was not chosen for two reasons. First, the same device type
has not used the same sequence across every platform in practice, so a shared,
reusable definition would need to be revisited per platform anyway, weakening
the case for keeping it in one place. Second, it would mean
phosphor-state-manager's own repository accumulating every platform's sequence
content over time. Writing each instance's sequence directly into its own
entity-manager configuration avoids both: entity-manager's own README already
accepts duplicated per-device configuration as a trade-off for keeping every
supported device's configuration in one place, and states that entity manager
itself does not participate in managing any device, only publishes data for a
reactor to consume, so a per-instance operation sequence is still within that
stated scope. This reading of entity-manager's own design is this proposal's own
reasoning, not something entity-manager's maintainers have confirmed; the schema
shape itself (an ordered operation list inside `Configuration.PowerSequence`) is
a new kind of content for entity-manager and needs their review.

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
Future sub-device types reuse the same `State.Device` lookup and
`ResetType`-to-`Transition` handling the reset handler implements for drives,
though type-specific Redfish route integration, such as where a given schema
exposes its own reset action, may still be required. No existing Redfish routes
are affected. The Drive.Reset action is absent on systems that do not deploy the
daemon.

Platforms provide entity-manager Board configuration for each device slot,
including that slot's own power-control operation sequences; a platform whose
hardware needs a sequence shape not already seen elsewhere writes it directly in
its own configuration rather than contributing a template to
phosphor-state-manager. Platform-specific early-boot initialization for drive
power hardware (e.g., GPIO direction setup) belongs in the platform's
platform-init implementation, not in this daemon.

### Organizational

- Does this proposal require a new repository? No.
- Which repositories are expected to be modified?
  - `phosphor-dbus-interfaces` (new `State.Device` interface and its
    `BMCNotReady` error)
  - `phosphor-state-manager` (sub-device state daemon, its systemd target units,
    and the operation-sequence interpreter)
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
