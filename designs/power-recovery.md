____
# OpenBMC Server Power Recovery

Author: Andrew Geissler (geissonator)

Primary assignee: Andrew Geissler (geissonator)

Other contributors:

Created: October 11th, 2021

## Problem Description
Modern computer systems have a feature, automated power-on recovery, which
in essence is the ability to tell your system what to do when it hits
issues with power to the system. If the system had a black out (i.e. power
was completely cut to the system), should it automatically power the system
on? Should it leave it off? Or maybe the user would like the system to
go to whichever state it was at before the power loss.

The goal of this design document is to describe how OpenBMC firmware will
deal with these questions.

## Background and References
The BMC already implements a limited subset of function in this area.
The [PowerRestorePolicy][pdi-restore] property out in phosphor-dbus-interface
defines the function capability.

In smaller servers, this feature is commonly found within the Advanced
Configuration and Power Interface (ACPI).

[openbmc/phosphor-state-manager][state-mgr] supports this property as defined
in the phosphor-dbus-interface.

Future updates to this document will touch on more complex scenarios like
brown outs (chassis power loss but BMC remains on), handling of external
uninterrupted power devices (UPS), and enhanced tracking of the different types
of errors that can occur in this area on systems.

## Requirements
OpenBMC software must ensure it persists the state of power to the chassis so
it can know what to restore it to if necessary

OpenBMC software must provide support for the following options:
- Do nothing when power is lost to the system (this will be the
  default)
- Always power the system on and boot the host
- Always power the system off (previous power was on, power is now off, run
  all chassis power off services to ensure a clean state of software and
  hardware)
- Restore the previous state of the chassis power and host

These options are only checked and enforced in situations where the BMC does
not detect that chassis power is already on to the system when it comes out
of reboot.

OpenBMC software must also support the concept of a one_time power restore
policy. This is a separate instance of the `PowerRestorePolicy` which will
be hosted under a D-Bus object path which ends with "one_time". If this
one_time setting is not the default, `None`, then software will execute
the policy defined under it, and then reset the one_time property to `None`.
This one_time feature is a way for software to utilize automated power-on
recovery function for other areas like firmware update scenarios where a
certain power on behavior is desired once an update has completed.

## Proposed Design
An application will be run after the chassis and host states have been
determined which will only run if the chassis power is not on.

This application will look for the one_time setting and use it if its value
is not `None`. If it does use the one_time setting then it will reset it
to `None` once it has read it. Otherwise the application will read the
persistent value of the `PowerRestorePolicy`. The application will then
run the logic as defined in the Requirements above.

This function will be hosted in phosphor-state-manger and potentially
x86-power-control.

## Alternatives Considered
None, this is a pretty basic feature that does not have a lot of alternatives
(other then just not doing it).

## Impacts
None

## Testing
The control of this policy can already bet set via the Redfish API.
```
#  Power Restore Policy
curl -k -X PATCH -d '{"PowerRestorePolicy":"AlwaysOn"}' https://${bmc}/redfish/v1/Systems/system
curl -k -X PATCH -d '{"PowerRestorePolicy":"AlwaysOff"}' https://${bmc}/redfish/v1/Systems/system
curl -k -X PATCH -d '{"PowerRestorePolicy":"LastState"}' https://${bmc}/redfish/v1/Systems/system
```
For testing, each policy should be set and verified. The one_time aspect should
also be checked for each possible value and verified to only be used once.

[pdi-restore]:https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Control/Power/RestorePolicy.interface.yaml
[state-mgr]: https://github.com/openbmc/phosphor-state-manager
