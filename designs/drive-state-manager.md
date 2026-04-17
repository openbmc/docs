# Drive State Manager

Author: Eric Yang (fornchu110)

Other contributors: None

Created: April 17, 2026

## Problem Description

On storage servers and compute servers with NVMe/SSD drive bays, there is a need
to monitor drive power state and perform controlled power cycling or reset of
individual drives. OpenBMC currently provides platform-level state control for
BMC, Host, and Chassis, but has no first-class mechanism for per-drive power
control.

Platforms that need this today must either map drives as chassis or host
instances — which carries incorrect semantics and breaks the Redfish model — or
write platform-specific daemons outside the state-manager framework.

## Background and References

The [phosphor-state-manager][1] repository provides state management for BMC,
Host, and Chassis objects using a daemon-per-instance pattern backed by systemd
target orchestration. Each daemon exposes a D-Bus interface from
[phosphor-dbus-interfaces][2] and maps requested transitions to systemd targets.
The daemon then monitors systemd `JobRemoved` signals to update the current
state property.

The `xyz.openbmc_project.State.Drive` interface was added in
phosphor-dbus-interfaces [change 43155][3]. It defines
`RequestedDriveTransition` and `CurrentDriveState` properties with the same
getter/setter pattern used by the Host and Chassis interfaces.

DMTF defines a `Drive.Reset` action in the Redfish Drive schema (Drive v1.7.0,
DSP8010 2019.2). The `ResetType` values `GracefulRestart`, `ForceRestart`, and
`PowerCycle` map naturally to the D-Bus transitions defined in the interface.

[1]: https://github.com/openbmc/phosphor-state-manager
[2]: https://github.com/openbmc/phosphor-dbus-interfaces
[3]: https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/43155

## Requirements

1. Provide per-drive power control (on, off, power cycle, reboot, hard reboot)
   via a D-Bus interface that bmcweb and other consumers can use.
2. Keep hardware-specific operations (GPIO toggling, I2C IO expander control,
   NVMe-MI commands, etc.) out of the state manager. Platform code attaches
   services to systemd targets via `.wants/` directories.
3. Use string instance identifiers (e.g. `nvme0`) rather than numeric IDs so
   that the D-Bus object path filename matches the Redfish `DriveId` directly.
4. The drive state manager must not contain any logic to monitor or react to
   host or chassis state transitions. Platforms that want drive power to follow
   chassis power must express this through systemd target dependencies.
5. Expose the `Drive.Reset` Redfish action in bmcweb. The action must only
   appear in a drive's Redfish resource when a matching
   `xyz.openbmc_project.State.Drive` D-Bus object exists.

## Proposed Design

Add `phosphor-drive-state-manager`, a new daemon in `phosphor-state-manager`
that follows the existing host and chassis daemon pattern.

### D-Bus model

```text
Bus name:    xyz.openbmc_project.State.Drive.<instance>
Object path: /xyz/openbmc_project/state/drive/<instance>
Interface:   xyz.openbmc_project.State.Drive
```

The daemon is instantiated as a systemd template service
(`xyz.openbmc_project.State.Drive@.service`), one instance per drive. The
instance name (e.g. `nvme0`) is passed via the `--drive` argument.

### Transition to systemd target mapping

```text
RequestedDriveTransition  systemd target
------------------------  --------------------------------------
On                        obmc-drive-poweron@<instance>.target
Off                       obmc-drive-poweroff@<instance>.target
Reboot                    obmc-drive-reboot@<instance>.target
HardReboot                obmc-drive-hard-reboot@<instance>.target
Powercycle                obmc-drive-powercycle@<instance>.target
```

State targets monitored by `JobRemoved` for runtime state updates:

```text
obmc-drive-poweron@<instance>.target
obmc-drive-poweroff@<instance>.target
```

Marker targets used for initial state determination on daemon restart:

```text
obmc-drive-powered-on@<instance>.target
obmc-drive-powered-off@<instance>.target
```

The transition targets (`reboot`, `hard-reboot`, `powercycle`) are empty hook
targets, following the same pattern as `obmc-chassis-powercycle@.target`.
Platforms attach services via `.wants/` to implement the actual hardware
sequence. `obmc-drive-reboot@.target` and `obmc-drive-hard-reboot@.target` each
declare `Wants=obmc-drive-powercycle@%i.target` as a default, but platforms can
override this by attaching additional services to the reboot targets (e.g. an
NVMe shutdown notification service for graceful reboot).

On startup, the daemon calls `GetUnit` on the marker targets to determine the
initial `CurrentDriveState`. At runtime, the daemon monitors `JobRemoved`
signals for the `poweron` and `poweroff` targets and updates state accordingly:
`poweron` completion sets Ready, `poweroff` completion sets Offline. Composite
transitions (`reboot`, `hard-reboot`, `powercycle`) ultimately trigger the
`poweron` target through systemd dependency ordering, so a separate handler is
not required. The `CurrentDriveState` property is read-only on D-Bus; only the
daemon updates it internally.

### bmcweb Redfish integration

Add `Drive.Reset` POST route and a `ResetActionInfo` GET route in bmcweb:

```text
POST /redfish/v1/Systems/{}/Storage/1/Drives/{}/Actions/Drive.Reset
GET  /redfish/v1/Systems/{}/Storage/1/Drives/{}/ResetActionInfo
```

The `ResetType` to D-Bus transition mapping is:

```text
On               ->  Transition.On
ForceOff         ->  Transition.Off
GracefulRestart  ->  Transition.Reboot
ForceRestart     ->  Transition.HardReboot
PowerCycle       ->  Transition.Powercycle
```

On a Drive GET, bmcweb performs an ObjectMapper `GetSubTree` under
`/xyz/openbmc_project/state` for the `xyz.openbmc_project.State.Drive` interface
and injects the `Actions.#Drive.Reset` block only if a matching object is found.
This keeps the action absent for drives managed by platforms that do not deploy
the drive state manager.

## Alternatives Considered

**Map drives as additional chassis or host instances.** Rejected because it
produces incorrect Redfish semantics (a drive is not a chassis) and would
require bmcweb workarounds to filter the spurious resources.

**Write a generic component state manager** that could handle drives, FPGAs, and
other devices through a shared framework. Rejected for this phase because the
additional abstraction would increase review risk without a concrete second use
case. Drive-specific naming keeps the scope clear and the change minimal.

**Embed host/chassis coupling logic in the daemon** (monitor host state and
power off drives automatically). Rejected because this creates hidden coupling
between state objects and makes the daemon harder to reason about. Systemd
target dependencies express the coupling at the platform level where it belongs.

## Impacts

- New daemon binary `phosphor-drive-state-manager` added to
  `phosphor-state-manager`.
- New systemd service and target templates installed by the recipe.
- bmcweb gains two new Redfish routes under the existing Storage hierarchy.
- No changes to existing D-Bus interfaces or existing Redfish routes.
- No API break; the action is absent on systems that do not deploy the daemon.

### Organizational

- Does this proposal require a new repository? No.
- Which repositories are expected to be modified?
  - `phosphor-dbus-interfaces` (paths metadata for Drive interface)
  - `phosphor-state-manager` (new daemon)
  - `bmcweb` (Drive.Reset action routes)
- Add maintainers of the above repositories as reviewers.

## Testing

The drive state manager can be tested without real NVMe hardware by attaching
test platform services to the systemd targets. These services log the transition
to the journal without performing actual hardware control. This verifies the
daemon state machine, systemd target orchestration, and the bmcweb Redfish
integration end-to-end.
