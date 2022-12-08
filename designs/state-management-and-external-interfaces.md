# OpenBMC State Management and External Interfaces

Author: Andrew Geissler (geissonator)

Other contributors: Jason Bills (jmbills)

Created: Jan 22, 2020

## Problem Description

As OpenBMC moves to fully supporting the Redfish protocol, it's important to
have the appropriate support within OpenBMC for the [ResetType][1] within the
Resource schema. OpenBMC currently has limited support and the goal with this
design is to get that support more complete.

Please note that the focus of this document is on the following `ResetType`
instance: `redfish/v1/Systems/system/Actions/ComputerSystem.Reset`

This support will also map to the existing IPMI Chassis Control command.

## Background and References

[phoshor-state-manager][2] implements the xyz.openbmc_project.State.\*
interfaces. These interfaces control and track the state of the BMC, Chassis,
and Host within an OpenBMC managed system. The README within the repository can
provide some further background information. [bmcweb][3], OpenBMC's web server
and front end Redfish interface, then maps commands to the ResetType object to
the appropriate xyz.openbmc_project.State.\* D-Bus interface.

The goal with this design is to enhance the xyz.openbmc_project.State.\*
interfaces to support more of the Redfish ResetType. Specifically this design is
looking to support the capability to reboot an operating system on a system
without cycling power to the chassis.

Currently phosphor-state-manager supports the following:

- Chassis: On/Off
- Host: On/Off/Reboot

The `Reboot` to the host currently causes a power cycle to the chassis.

### Redfish

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

### IPMI

The IPMI specification defines a Chassis Control Command with a chassis control
parameter as follows:

| Option                         | Description                                                                                                                                                                                                                                                                                                                                                            |
| ------------------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| power down                     | Force system into soft off (S4/S45) state. This is for ‘emergency’ management power down actions. The command does not initiate a clean shut-down of the operating system prior to powering down the system.                                                                                                                                                           |
| power up                       |                                                                                                                                                                                                                                                                                                                                                                        |
| power cycle                    | This command provides a power off interval of at least 1 second following the deassertion of the system’s POWERGOOD status from the main power subsystem. It is recommended that no action occur if system power is off (S4/S5) when this action is selected, and that a D5h “Request parameter(s) not supported in present state.” error completion code be returned. |
| hard reset                     | In some implementations, the BMC may not know whether a reset will cause any particular effect and will pulse the system reset signal regardless of power state.                                                                                                                                                                                                       |
| pulse Diagnostic Interrupt     | Pulse a version of a diagnostic interrupt that goes directly to the processor(s). This is typically used to cause the operating system to do a diagnostic dump (OS dependent).                                                                                                                                                                                         |
| Initiate a soft-shutdown of OS |                                                                                                                                                                                                                                                                                                                                                                        |

## Requirements

- Keep legacy support where `xyz.openbmc_project.State.Host.Transition.Reboot`
  causes a graceful shutdown of the host, a power cycle of the chassis, and a
  starting of the host.
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

The existing bmcweb code uses some additional xyz.openbmc_project.State.\*
interfaces that are not defined within phosphor-dbus-interfaces. These are
implemented within the x86-power-control repository which is an alternate
implementation to phosphor-state-manager. It has the following mapping for these
non-phosphor-dbus-interfaces

- `ForceRestart` -> `xyz.openbmc_project.State.Chassis.Transition.Reset`
- `PowerCycle` -> `xyz.openbmc_project.State.Chassis.Transition.PowerCycle`

A `ForceRestart` should restart the host, not the chassis. The proposal is to
change the current bmcweb mapping for `ForceRestart` to a new host transition:
`xyz.openbmc_project.State.Host.Transition.ForceWarmReboot`

A `GracefulRestart` will map to our new host transition:
`xyz.openbmc_project.State.Host.Transition.GracefulWarmReboot`

The `PowerCycle` operation is dependent on the current state of the host. If
host is on, it will map to `xyz.openbmc_project.State.Host.Transition.Reboot`
otherwise it will map to
`xyz.openbmc_project.State.Chassis.Transition.PowerCycle`

To summarize the new Redfish to xyz.openbmc_project.State.\* mappings:

- `ForceRestart` -> `xyz.openbmc_project.State.Host.Transition.ForceWarmReboot`
- `GracefulRestart`->
  `xyz.openbmc_project.State.Host.Transition.GracefulWarmReboot`
- `PowerCycle`:
  - If host on: `xyz.openbmc_project.State.Host.Transition.Reboot`
  - If host off: `xyz.openbmc_project.State.Chassis.Transition.PowerCycle`

The full mapping of Redfish and IPMI to xyz.openbmc_project.State.\* is as
follows:

| Redfish               | IPMI        | xyz.openbmc_project.State.Transition |
| --------------------- | ----------- | ------------------------------------ |
| ForceOff              | power down  | Chassis.Off                          |
| ForceOn               | power up    | Host.On                              |
| ForceRestart          | hard reset  | Host.ForceWarmReboot                 |
| GracefulRestart       |             | Host.GracefulWarmReboot              |
| GracefulShutdown      | soft off    | Host.Off                             |
| On                    | power up    | Host.On                              |
| PowerCycle (host on)  | power cycle | Host.Reboot                          |
| PowerCycle (host off) |             | Chassis.PowerCycle                   |

## Alternatives Considered

No other alternatives considered.

## Impacts

Existing interfaces will be kept the same. Some changes in x86-power-control
would be needed to ensure the bmcweb mappings work as expected with the new
interfaces.

Some changes in phosphor-host-ipmid would be needed to support the new state
transitions.

## Testing

These new state transitions will be backed by systemd targets. These targets
will be thoroughly tested. OpenBMC test team will be approached to ensure these
are tested in automation.

[1]: http://redfish.dmtf.org/schemas/v1/Resource.json#/definitions/ResetType
[2]: https://github.com/openbmc/phosphor-state-manager
[3]: https://github.com/openbmc/bmcweb
[4]: https://gerrit.openbmc.org/c/openbmc/docs/+/22358
