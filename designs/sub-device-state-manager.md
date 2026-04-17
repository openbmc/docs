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
phosphor-state-manager: a shared common layer plus a per-type daemon that
discovers devices through entity-manager and maps Redfish reset actions to
systemd targets. The first supported type is NVMe drives.

## Background and References

[phosphor-state-manager][1] manages BMC, Host, and Chassis state. Each daemon
exposes a D-Bus interface from [phosphor-dbus-interfaces][2] and maps requested
transitions to systemd targets, monitoring `JobRemoved` signals to update
current state.

The `xyz.openbmc_project.State.Drive` interface was added in
phosphor-dbus-interfaces [change 43155][3], defining `RequestedDriveTransition`
and `CurrentDriveState` with the same pattern used by Host and Chassis. The
existing Transition enum covers `Reboot`, `HardReboot`, and `Powercycle`; this
design extends it with `On` and `Off` and adds a `paths:` metadata block so the
interface matches the Host and Chassis namespace convention.

[entity-manager][4] is the standard mechanism for runtime hardware discovery. It
consumes probe signals from presence-detection daemons and publishes inventory
and configuration interfaces on D-Bus.

[dbus-sensors][5] organises multiple related daemons (adc, fan, nvme, psu, and
others) around a shared common source tree and one binary plus one systemd
service per sensor type. Each binary is long-lived, single-instance, and
entity-manager-driven. This design adopts the same structural pattern for
sub-device state daemons.

DMTF defines a `Drive.Reset` action in the Redfish Drive schema (Drive v1.7.0,
DSP8010 2019.2). The `ResetType` values map to the D-Bus transitions.

[1]: https://github.com/openbmc/phosphor-state-manager
[2]: https://github.com/openbmc/phosphor-dbus-interfaces
[3]: https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/43155
[4]: https://github.com/openbmc/entity-manager
[5]: https://github.com/openbmc/dbus-sensors

## Requirements

1. Provide per-device power control (on, off, power cycle, reboot, hard reboot)
   via D-Bus interfaces that bmcweb and other consumers can use. The current
   state must be readable and updated as transitions complete.
2. The daemon must discover devices at runtime through entity-manager rather
   than hardcoding instances at build time. Slots without a device present must
   produce no D-Bus object and no Redfish resource.
3. Keep hardware-specific operations (GPIO toggling, I2C IO expander control,
   NVMe-MI commands, etc.) out of the state manager. Platform code attaches
   services to systemd targets via `.wants/` directories.
4. Each state object must maintain a D-Bus association to its inventory object
   so that bmcweb can locate the state from inventory without relying on path
   naming conventions.
5. Core infrastructure (entity-manager discovery, association lifecycle, systemd
   target orchestration, `JobRemoved` handling) is shared across device types.
   Adding a new type (NIC, PCIe card) requires defining a new PDI state
   interface, producing a new binary on top of the shared layer, and installing
   a new systemd service; it does not require reworking existing types.
6. Per-type requirements such as host dependency are expressed through per-type
   mechanisms (systemd target dependencies for execution ordering, and the
   existing `Inventory.Decorator.ManagedHost` interface for host association),
   not imposed on all device types by the framework.
7. Expose the Redfish reset action for each device type in bmcweb. The action
   must only appear when a matching state D-Bus object exists.

## Proposed Design

### Daemon Architecture

The framework follows the dbus-sensors structural pattern: a shared common
source layer in phosphor-state-manager provides the infrastructure, and each
supported device type ships as its own binary and systemd service built on top
of the shared layer.

The shared common layer handles the concerns that are identical across types:

- entity-manager `InterfacesAdded` and `InterfacesRemoved` subscription plus an
  initial `GetManagedObjects` scan
- D-Bus state object lifecycle keyed on inventory path
- `Association.Definitions` publication with
  `(inventory, state, <inventory path>)` tuples
- systemd `StartUnit` invocation and `JobRemoved` signal handling
- marker-target initial-state resolution on daemon startup

A per-type binary supplies the parts that must be type-specific:

- the PDI state interface to expose (e.g. `State.Drive`) and its property
  accessors
- the inventory item interface to watch (e.g. `Inventory.Item.Drive`)
- the transition-to-target name map and marker-target names
- any type-specific pre-transition validation such as a host-dependency check

Each per-type binary is long-lived and single-instance. Discovery through
entity-manager creates per-device state objects at runtime within the single
process, so there is no systemd template unit per device instance. The first
supported type is the drive binary (`phosphor-drive-state-manager` running as
`xyz.openbmc_project.State.Drive.service`). Adding a new device type (NIC, PCIe
card) means defining a new PDI interface, producing a new binary that links the
shared layer, and installing a new service file.

### Integration with entity-manager

The daemon discovers devices through entity-manager. Platforms provide Board
entities with the appropriate `Inventory.Item.<Type>` marker interface.

The entity-manager supports multiple detection mechanisms that platforms choose
based on their hardware:

- GPIO presence pins via entity-manager's `gpio-presence` sub-binary, publishing
  `Inventory.Source.DevicePresence` interfaces
- FRU EEPROM data via FruDevice scanner

The daemon is agnostic to the detection mechanism. It only watches for the
resulting `Inventory.Item.<Type>` interfaces that entity-manager publishes after
a successful probe. Empty slots produce no detection signal, so entity-manager
creates no inventory object, the daemon creates no state object, and bmcweb
shows no Redfish resource.

Per-type mapping may read additional properties from the Board entity as needed.

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

The drive binary (`phosphor-drive-state-manager`, running as
`xyz.openbmc_project.State.Drive.service`) watches `Inventory.Item.Drive` and
creates `State.Drive` objects per discovered drive.

**Configuration in entity-manager:**

Platforms provide a Board entity with `Inventory.Item.Drive`:

```json
{
  "Name": "nvme0",
  "Probe": "xyz.openbmc_project.Inventory.Source.DevicePresence({'Name': 'MB E1S0'})",
  "Type": "Board",
  "xyz.openbmc_project.Inventory.Item.Drive": {},
  "xyz.openbmc_project.Inventory.Decorator.ManagedHost": {
    "HostIndex": 0
  }
}
```

**D-Bus object:**

```text
/xyz/openbmc_project/state/drive/<instance>
  xyz.openbmc_project.State.Drive
    RequestedDriveTransition  (enum: On, Off, Reboot, HardReboot, Powercycle)
    CurrentDriveState         (enum: Ready, Offline, ...)
    Rebuilding                (bool)
  xyz.openbmc_project.Association.Definitions
    [("inventory", "state", <inventory path>)]
```

**Host dependency:**

Drives depend on host power state. When the host is off, the drive state must
reflect off, and a request to power on the drive must return an error. The
relationship is expressed in two complementary layers.

Execution ordering is handled by systemd target dependencies. Platforms wire
`obmc-host-stop@.target` to trigger `obmc-drive-poweroff@.target` so the drive
is powered off when the host stops. The daemon's `JobRemoved` handler sees the
poweroff target complete and updates `CurrentDriveState` to Offline.

Semantic validation is handled at the daemon level using host association data
published by entity-manager. A Board entity may include the existing
`Inventory.Decorator.ManagedHost` interface with a `HostIndex` property
identifying which host the device belongs to. This is the same interface already
used by multi-host platforms (e.g. Yosemite4 retimer configs) and consumed by
bmcweb for system collection enumeration. When a `RequestedDriveTransition` of
`On`, `Reboot`, or `Powercycle` is received, the daemon reads `CurrentHostState`
for the host indicated by `HostIndex`; if the host is not in a state that
permits drives to be powered on, the transition is rejected with a D-Bus error,
which bmcweb surfaces as a Redfish 4xx response. Board entities without
`ManagedHost` receive no semantic check and behave as host-independent devices.

Systemd dependencies alone cannot deliver a D-Bus error response because a
failed target start results in a unit job failure, not a property-set failure.
The daemon-level check closes this gap without duplicating the execution
ordering that systemd already provides.

**Systemd targets:**

The following systemd targets are installed by the phosphor-state-manager recipe
for the drive type. All are template units parameterized by drive instance name.

Transition targets (started by the daemon on `RequestedDriveTransition`):

```text
obmc-drive-poweron@.target      Started on Transition.On
obmc-drive-poweroff@.target     Started on Transition.Off
obmc-drive-reboot@.target       Started on Transition.Reboot
obmc-drive-hard-reboot@.target  Started on Transition.HardReboot
obmc-drive-powercycle@.target   Started on Transition.Powercycle
```

Marker targets (checked on daemon startup to determine initial state):

```text
obmc-drive-powered-on@.target   Active state -> CurrentDriveState = Ready
obmc-drive-powered-off@.target  Active state -> CurrentDriveState = Offline
```

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
update `CurrentDriveState`: poweron completion sets Ready, poweroff completion
sets Offline. Composite transitions ultimately trigger poweron through systemd
dependency ordering, so no separate handler is needed.

**Platform systemd target hooking:**

Platforms provide the actual power control by attaching services to targets in
their openbmc meta-layer:

```text
obmc-drive-poweron@.target
  .wants/drive-poweron@.service     (platform-specific: GPIO, I2C, etc.)

obmc-drive-poweroff@.target
  .wants/drive-poweroff@.service    (platform-specific: GPIO, I2C, etc.)
```

This follows the same pattern used for host and chassis power control, where
platform services are attached to framework-defined targets.

**bmcweb integration:**

The Drives collection continues to be enumerated from `Inventory.Item.Drive`
objects in the existing inventory subtree. Because entity-manager creates no
inventory object for an unprobed slot, empty slots are absent from the
collection automatically.

On a Drive GET, bmcweb follows the `/state` association from the inventory
object. If a state endpoint exists, it reads `CurrentDriveState` and
`Rebuilding`, and injects the `Actions.#Drive.Reset` block into the Redfish
response. If no state association exists, the action is omitted, ensuring
platforms that do not deploy the daemon are unaffected.

For `Drive.Reset` POST, bmcweb follows the association to the state object and
sets `RequestedDriveTransition`. The Redfish `ResetType` to D-Bus transition
mapping:

```text
ResetType            D-Bus Transition
-------------------  -------------------------
On                   Transition.On
ForceOff             Transition.Off
GracefulRestart      Transition.Reboot
ForceRestart         Transition.HardReboot
PowerCycle           Transition.Powercycle
```

A `ResetActionInfo` GET route returns static `AllowableValues` for the supported
reset types.

## Alternatives Considered

**Single daemon supporting all device types** (e.g.
`phosphor-device-state-manager`) running as one long-lived process that routes
based on the inventory item interface type. Each `State.*` interface in
phosphor-dbus-interfaces already has its own property names, transition enum,
and state enum, so per-type binding code (property accessors, enum validation,
type-specific state handling) must exist in either arrangement. The trade-off is
where the infrastructure layer lives.

- Single daemon: one entity-manager subscription, one event loop, one
  association lifecycle; new types add a PDI interface and a registration entry.
  One process crash or upgrade affects all types, and the binary always links
  every supported type with no per-platform selection.
- Per-type binaries (chosen): each type has an independent lifecycle, crash
  domain, and recipe-level selection, matching the existing
  phosphor-state-manager and dbus-sensors conventions. The entity-manager
  subscription and event loop are replicated per binary, which is marginal since
  each binary filters on a different inventory item interface.

**Map drives as additional chassis or host instances.** Reuses existing daemons
and services without new code, but produces incorrect Redfish semantics (a drive
is not a chassis) and requires bmcweb workarounds to filter the spurious
resources from Redfish collections.

**Single generic `State.SubDevice` interface for all types.** A shared interface
would reduce PDI definition work, but the current PDI convention uses per-type
interfaces (State.BMC, State.Host, State.Chassis each with independent
transition and state enums; Inventory.Item has ~35 per-type sub-interfaces).
This design follows the existing convention and generalizes at the daemon level
instead.

**Build-time instance configuration via systemd template instances.** Matches
the existing PSM host/chassis pattern and needs no entity-manager coupling, but
cannot represent dynamic presence: empty slots would still produce D-Bus objects
and Redfish resources unless the platform statically tracks population per
build.

**Host dependency via systemd target dependencies alone.** Uses only the systemd
wiring layer and needs no entity-manager metadata, but cannot translate a failed
host-dependency gate into a D-Bus error: a unit job failure does not surface as
a property-set failure, so bmcweb clients receive a silently successful response
while the drive state remains unchanged. The design combines this layer with a
daemon-level check, as described in the Device Type: Drive section.

## Impacts

phosphor-dbus-interfaces extends the `State.Drive` Transition enum with `On` and
`Off` values, and adds a `paths:` metadata block matching the Host/Chassis
convention.

phosphor-state-manager gains a shared common source layer for sub-device state
infrastructure (entity-manager discovery, association lifecycle, systemd
orchestration) and its first consumer: a drive state daemon binary with its
systemd service. No changes to existing host, chassis, or BMC daemons. Future
sub-device types (NIC, PCIe) will add new binaries and services on top of the
same shared layer.

entity-manager schemas are extended to accept
`xyz.openbmc_project.Inventory.Item.Drive` as a top-level Board interface. Host
association uses the existing `Inventory.Decorator.ManagedHost` interface
(already defined in entity-manager schemas and used by multi-host platforms), so
no new schema properties are required for host association.

bmcweb gains Drive.Reset routes under the existing Storage hierarchy, resolving
the state object through the `/state` association on the drive inventory path.
No existing Redfish routes are affected. The Drive.Reset action is absent on
systems that do not deploy the daemon.

Platforms provide entity-manager Board configuration for each device slot and
systemd target hook services for hardware-specific power control.
Platform-specific early-boot initialization for drive power hardware (e.g., GPIO
direction setup) belongs in the platform's platform-init implementation, not in
this daemon.

### Organizational

- Does this proposal require a new repository? No.
- Which repositories are expected to be modified?
  - `phosphor-dbus-interfaces` (State.Drive On/Off transitions, paths metadata)
  - `phosphor-state-manager` (new shared sub-device infrastructure layer and
    drive state daemon)
  - `bmcweb` (Drive.Reset action, association-based state lookup)
  - `entity-manager` (schema: allow `Inventory.Item.Drive` as Board interface)
  - `openbmc` (platform entity-manager configs, systemd target services,
    platform-init hardware setup where applicable)

## Testing

TBD
