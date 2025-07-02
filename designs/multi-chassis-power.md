# Multi-Chassis Power Monitoring Application

Author: Derek Howard (derekhoward55)

Other contributors:

Created: 2025-06-26

## Problem Description

In systems with multiple chassis, a subset of the chassis may contain a BMC to 
handle system function; other chassis (while still part of the system) may not 
contain a BMC.  Individual chassis may experience problems (such as loss of input 
power) while other chassis in the system may not.  The system requirements to 
handle chassis that are experiencing power problems may be different based on 
which chassis experienced the problem, the type of problem, current state of the 
system, etc.  How the chassis is re-integrated into the system after the problem 
is resolved may also be different based on the above.

A new application is needed to monitor each individual chassis' power or other 
status, to handle failures (such as isolate and log errors and enable any 
chassis cleanup or re-integration function), to help portray the state of the 
chassis, and to provide information to other dependent applications. It should 
be data-driven to support a variety of requirements and hardware implementations 
and to avoid hard-coded logic.

## Background and References

In single-chassis systems, the BMC, all processors, memory, etc reside in the 
same chassis.  When the chassis experiences problems such as loss of input power 
due to power line disturbances, all of the logic synchronously loses power. 
However, in multi-chassis systems, BMCs, processors, memory, etc utilized by the 
system will be spread across all of the individual chassis.  A subset of the 
chassis may experience the problem, while other chassis may remain on.

For example, if all of the power supplies in a chassis lose input power, not 
only will the processors&memory in that chassis become unusable, all of the 
devices running in that chassis to detect the problem (such as power supplies) 
will become unusable as well.  To differentiate between a loss of input power 
versus hardware failures versus a chassis being unplugged, additional hardware 
may be utilized to isolate the problem correctly.

The OpenBMC project currently contains apps for:

- power supply monitoring (eg [phosphor-psu-monitor][1] and [psusensor][2])
- chassis power sequencing (eg [phospor-power-sequencer][3] and 
  [x86-power-control][4])
- chassis state management (eg [phosphor-chassis-state-manager][5])

Enterprise class servers may contain other types of hardware to monitor 
chassis-level (not individual device-level) power or other chassis-level 
problems.  This hardware is independent of individual power supply or voltage 
regulator monitoring.

The intent of this new application design is to handle chassis-level fault 
monitoring and recovery.  This app will be independent of which apps are used for 
power sequencing and power supply monitoring.

## Requirements

Some of these requirements may be deemed as business specific logic, and thus 
could be configurable options as appropriate.

- When input power is lost to a chassis while the system is powered on, the 
  system should reboot with whatever chassis have sufficient input power.

- An error should be logged against the chassis that lost input power.

- An error should be logged against any chassis that does not boot (for whatever 
  reason, including that it does not have sufficient input power).

- Differentiation between a missing chassis versus a present chassis with no 
  input power versus a present chassis with communication problems is needed for 
  error isolation.

- Interfaces should be provided to allow other apps to collect chassis status 
  based on input power.

## Proposed Design

### Application

A new application named `phosphor-chassis-power` will be created to monitor 
chassis power. The application will be located in the `phosphor-power` 
repository.

### Application Data File

The application will read a JSON file at startup that defines the power fault 
detection details for all potential chassis. The JSON file will be specific to a 
system type, so it will be stored in the GitHub repository for the appropriate 
build layer (such as meta-ibm).

JSON file location on the BMC:

- Standard directory: `/usr/share/phosphor-chassis-power` (read-only)

A document will be provided that describes the objects and properties that can 
appear in the JSON file. A validation tool will also be provided to verify the 
syntax and contents of the JSON file.

### D-Bus Interfaces

The existing [PowerSystemInputs][6] D-Bus interface will be used:

- Name: `xyz.openbmc_project.State.Decorator.PowerSystemInputs`
- Description: Describes the status of power supplied to the chassis.
- Properties: Status (Good, Fault)
- Methods: None

The PowerSystemInputs interface was previously used on a "power_supplies" 
object, since it described the state of the input power as read from the power 
supplies.

### D-Bus Paths

The existing `xyz.openbmc_project.State.Decorator.PowerSystemInputs` interface 
will be implemented on the new object path 
`/xyz/openbmc_project/power/chassis/chassisX`.

A new "chassis" object will re-use the existing PowerSystemInputs interface, 
since the new usage will be for the power read for the chassis directly (ie, NOT 
from the power supplies, as the power supplies will not be able to communicate 
if they are not powered). In chassisX, X will be 0, 1, 2, etc for each chassis.

### Systemd

The application will be started using systemd when the BMC is being booted 
(typically at standby). The service file will be named 
`xyz.openbmc_project.ChassisPower.service`.

During the BMC boot, this should be started prior to any general apps that 
access devices on the individual chassis so the DBus object can be checked for 
existence to know if it can be used to read the chassis power.

## Alternatives Considered

Implementing this in the chassis state management repo was considered. This 
design was avoided due to:

- While chassis state management needs some of the information provided by this 
  app, power and other potential faults are independent of the chassis/system 
  state.
- This function is only needed in multi-chassis configurations, so it was 
  determined to keep this isolated from the much-used state management code.
- The phosphor-power repo contains other similar power fault handling function.

## Impacts

No major impacts are expected to existing APIs, security, documentation, 
performance, or upgradability.  The application will not require changes in other 
apps in systems that don't need this functionality.

The main documentation impact should be this design document. Future 
enhancements or clarifications may be required for this document.

## Testing

Testing can be accomplished via automated or manual testing to verify.

- Input power faults to a subset of the present chassis will be injected to 
  ensure the system boots with the remaining chassis, logs the appropriate 
  errors, and functions properly.
- Input power faults will be injected temporarily to ensure the chassis are 
  configured and booted correctly.
- End-to-end boot testing will be performed to ensure that the correct hardware 
  communication occurred, that the system boots, and that no errors occur.
- CI tests that boot a system will indirectly test this application.

[1]: https://github.com/openbmc/phosphor-power/tree/master/phosphor-power-supply
[2]: https://github.com/openbmc/dbus-sensors/tree/master/src/psu
[3]: 
  https://github.com/openbmc/phosphor-power/tree/master/phosphor-power-sequencer
[4]: https://github.com/openbmc/x86-power-control
[5]: https://github.com/openbmc/phosphor-state-manager
[6]: 
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/Decorator/PowerSystemInputs.interface.yaml
