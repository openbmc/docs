# OpenBMC State Management and Redfish

Author: Andrew Geissler (geissonator)

Primary assignee: Andrew Geissler (geissonator)

Created: Jan 22, 2020

## Problem Description

As OpenBMC moves to fully supporting the Redfish protocol, it's important to
have the appropriate support within OpenBMC for the [ResetType][1] within the
Resource schema. OpenBMC currently has limited support and the goal with this
design is to get that support more complete.

## Background and References

[phoshor-state-manager][2] implements the xyz.openbmc_project.State.\*
interfaces. These interfaces control and track the state of the BMC, Chassis,
and Host within a OpenBMC managed system. The README within the repository
can provide some further background information. [bmcweb][3], OpenBMC's web
server and front end Redfish interface, then maps commands to the ResetType
object to the appropriate xyz.openbmc_project.State.* D-Bus interface.

The goal with this design is to enhance the xyz.openbmc_project.State.*
interfaces to support more of the Redfish ResetType. Specifically this design
is looking to support the capability to reboot a operating system on a system
without cycling power to the chassis.

Currently phosphor-state-manager supports the following:
  - Chassis: On/Off
  - Host: On/Off/Reboot

The `Reboot` to the host currently causes a power cycle to the chassis.

The Redfish [ResetType][1] has the following operations associated with it:
```
"ResetType": {
    "enum": [
        "On",
        "ForceOff",
        "GracefulShutdown",
        "GracefulRestart",
        "ForceRestart",
        "Nmi",
        "ForceOn",
        "PushPowerButton",
        "PowerCycle"
    ],
    "enumDescriptions": {
        "ForceOff": "Turn off the unit immediately (non-graceful shutdown).",
        "ForceOn": "Turn on the unit immediately.",
        "ForceRestart": "Shut down immediately and non-gracefully and restart
          the system.",
        "GracefulRestart": "Shut down gracefully and restart the system.",
        "GracefulShutdown": "Shut down gracefully and power off.",
        "Nmi": "Generate a diagnostic interrupt, which is usually an NMI on x86
          systems, to stop normal operations, complete diagnostic actions, and,
          typically, halt the system.",
        "On": "Turn on the unit.",
        "PowerCycle": "Power cycle the unit.",
        "PushPowerButton": "Simulate the pressing of the physical power button
          on this unit."
    },
    "type": "string"
},
```

Certain concepts in this proposal were also within this [design][4]. That design
is mostly focused on the Chassis side of things but the two designs could
potentially be merged into one.

## Requirements

- Keep legacy support where `xyz.openbmc_project.State.Host.Transition.Reboot`
  causes a graceful shutdown of the host, a power cycle of the chassis, and
  a starting of the host.
- Support a reboot of the host with chassis power on
  - Support `GracefulRestart` (where the host is notified of the reboot)
  - Support `ForceRestart` (where the host is not notified of the reboot)
- Map `PowerCycle` to a host or chassis operation depending on the current state
  of the system.
  - If host is running, then a `PowerCycle` should cycle power to the chassis
    and boot the host.
  - If host is not running, then a `PowerCycle` should only cycle power to the
    chassis.

## Proposed Design

Create two new `xyz.openbmc_project.State.Host.Transition` options:
- `ForceWarmReboot`, `GracefulWarmReboot`

Create a new `xyz.openbmc_project.State.Chassis.Transition` option:
- `PowerCycle`

The existing bmcweb code uses some additional xyz.openbmc_project.State.*
interfaces that are not defined within phosphor-dbus-interfaces. These are
implemented within the x86-power-control repository which is an alternate
implementation to phosphor-state-manager. It has the following mapping for
these non-phosphor-dbus-interfaces
- `ForceRestart` -> `xyz.openbmc_project.State.Chassis.Transition.Reset`
- `PowerCycle` -> `xyz.openbmc_project.State.Chassis.Transition.PowerCycle`

A `ForceRestart` should restart the host, not the chassis. The proposal is to
change the current bmcweb mapping for `ForceRestart` to a new host transition:
`xyz.openbmc_project.State.Host.Transition.ForceWarmReboot`

A `GracefulRestart` will map to our new host transition:
`xyz.openbmc_project.State.Host.Transition.GracefulWarmReboot`

The `PowerCycle` operation is dependent on the current state of the host.
If host is on, it will map to `xyz.openbmc_project.State.Host.Transition.Reboot`
otherwise it will map to
`xyz.openbmc_project.State.Chassis.Transition.PowerCycle`

To summarize the new Redfish to xyz.openbmc_project.State.* mappings:
- `ForceRestart` -> `xyz.openbmc_project.State.Host.Transition.ForceWarmReboot`
- `GracefulRestart`-> `xyz.openbmc_project.State.Host.Transition.GracefulWarmReboot`
- `PowerCycle`:
  - If host on: `xyz.openbmc_project.State.Host.Transition.Reboot`
  - If host off: `xyz.openbmc_project.State.Chassis.Transition.PowerCycle`

## Alternatives Considered

No other alternatives considered.

## Impacts

Existing interfaces will be kept the same. Some changes in x86-power-control
would be needed to ensure the bmcweb mappings work as expected with the new
interfaces.

## Testing

These new state transitions will be backed by systemd targets. These targets
will be thoroughly tested. OpenBMC test team will be approached to ensure these
are tested in automation.

[1]: http://redfish.dmtf.org/schemas/v1/Resource.json#/definitions/ResetType
[2]: https://github.com/openbmc/phosphor-state-manager
[3]: https://github.com/openbmc/bmcweb
[4]: https://gerrit.openbmc-project.xyz/c/openbmc/docs/+/22358
