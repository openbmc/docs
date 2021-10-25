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

There are also instances where the user may not want automatic power recovery
to occur. For example, some systems have op-panels, and on these op-panels
there can be a pin hole reset. This is a manual mechanism for the user to
force a hard reset to the BMC in situations where it is hung or not responding.
In these situations, the user may wish for the system to not automatically
power on the system, because they want to debug the reason for the BMC error.

A brownout is another scenario that commonly utilizes automated power-on
recovery features. A brownout is a scenario where BMC firmware detects (or is
told) that chassis power can no longer be supported, but power to the BMC
will be retained. On some systems, it's desired to utilize the automated
power-on feature to turn chassis power back on as soon as the brownout condition
ends.

Some system owners may chose to attach an Uninterrupted Power Supply (UPS) to
their system. A UPS continues to provide power to a system through a blackout
or brownout scenario. A UPS has a limited amount of power so it's main
purpose is to handle brief power interruptions or to allow for an orderly
shutdown of the host firmware.

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

## Requirements

### Automated Power-On Recovery
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

### BMC and System Recovery Paths
In situations where the BMC or the system have gotten into a bad state, and
the user has initiated some form of manual reset which is detectable by the
BMC as being user initiated, the BMC software must:
- Fill in appropriate `RebootCause` within the [BMC state interface][bmc-state]
  - At a minimum, `PinHole` will be added. Others can be added as needed
- Log an error indicating a user initiated forced reset has occurred
- Not log an error indicating a blackout has occurred if chassis power was on
  prior to the pin hole reset
- Not implement any power recovery policy on the system
- Turn power recovery back on once BMC has a normal reboot

### Brownout
As noted above, a brownout condition is when AC power can not continue to be
supplied to the chassis, but the BMC can continue to have power and run.

When this condition occurs, the BMC must:
- Power system off as quickly as situations requires (or gracefully handle
  the loss of power if it occurred without warning)
- Log an error indicating the brownout event has occurred
- Support the ability for host firmware to indicate a one-time power restore
  policy if they wish for when the brownout completes
- Identify when a brownout condition has completed
- Wait for the brownout to complete and implement the one-time power restore
  policy. If no one-time policy is defined then run the standard power restore
  policy defined for the system

BMC firmware must also be able to:
- Discover if system is in a brownout situation
  - Run when the BMC first comes up to know if it should implement any automated
    power-on recovery
- Do not allow a system to go through any type of power on if in a brownout
  situation
- Tell the host firmware that it is a automated power-on recovery initiated
  boot when that firmware is what boots the system

### Uninterruptible Power Supply (UPS)
When a UPS is present and a blackout or brownout condition occurs, the BMC must:
- Log an error to indicate the condition has occurred
- If UPS power becomes low and if host firmware is running, notify the host
  firmware of the condition, indicating a quick power off is required
- Log an error if power loss to the entire system is imminent (i.e. a blackout
  scenario where BMC will also lose power and UPS is about to run out of power)

## Proposed Design

### Automated Power-On Recovery
An application will be run after the chassis and host states have been
determined which will only run if the chassis power is not on.

This application will look for the one_time setting and use it if its value
is not `None`. If it does use the one_time setting then it will reset it
to `None` once it has read it. Otherwise the application will read the
persistent value of the `PowerRestorePolicy`. The application will then
run the logic as defined in the Requirements above.

This function will be hosted in phosphor-state-manger and potentially
x86-power-control.

### BMC and System Recovery Paths
A new GPIO name will be added to the [device-tree-gpio-naming.md][dev-tree]
which reports whether a pin hole reset has occurred on the previous reboot of
the BMC. The BMC state manager application will enhance it's support of the
`RebootCause` to look for this GPIO and if present, read it and set
`RebootCause` accordingly.

If the power recovery software sees the `PinHole` reason within the
`RebootCause` then it will not implement any of its policy. Future BMC
reboots which are not pin hole reset caused, will cause `RebootCause` to go
back to a default and therefore power recovery policy will be reenabled on that
BMC boot.

The phosphor-state-manager chassis software will not log a blackout error
if it sees the `PinHole` reason (or any other reason that indicates a user
initiated reset of the system).

### Brownout
The phosphor-psu-monitor application within the phosphor-power repository will
be responsible for monitoring for brownout conditions. It will support a D-Bus
property, `PowerStatus`, hosted under a `xyz.openbmc_project.State.Chassis`
interface. `PowerStatus` will have the following fields:
- `Undefined`
- `BrownOut`
- `Good`

This application will run on startup and discover the correct state for
this property. This application will log an error when a brownout occurs and
initiate the fast power off.

PLDM will support an effecter for the host to set a one-time automated power-on
recovery for when a brownout condition ends. This will utilize the existing
one-time function provided by phosphor-state-manager.

When a the phosphor-power application detects that a brownout condition has
completed, it will reset the `PowerStatus` to `Good` and start the
state-manager service which executes the automated power-on logic.

phosphor-state-manager will ensure a system boot is not possible if
`PowerStatus` is not set to `Good`.

### Uninterruptible Power Supply (UPS)
An application within the phosphor-power repository will monitor for UPS
presence. If one is connected to the system then this application will monitor
UPS status and if it sees power has been lost and the system is running on
UPS battery power then it will monitor for the power remaining in the UPS and
notify the host that a shutdown is required if needed. This application
will also be responsible for logging an error indicating the UPS backup power
has been switched too and set the `PowerStatus` field to indicate a `BrownOut`
scenario is present when the system can no longer remain on.


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

On supported systems, a pin hole reset should be done with a system that has
a policy set to always power on. Tester should verify system does not
automatically power on after a pin hole reset. Verify it does automatically
power on when a normal reboot of the BMC is done.

A brownout condition should be injected into a system and appropriate paths
should be verified:
- Error log generated
- Host notified (if running and notification possible)
- System quickly powered off
- System can not be powered on while in brownout condition
- System automatically powers back on when brownout condition ends (assuming a
  one-time or system auto power-on recovery policy of `AlwaysOn` or `Restore`)

Plug a UPS into a system and ensure when power is cut to the system that an
error is logged and the host is notified and allowed to power off.

[pdi-restore]:https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Control/Power/RestorePolicy.interface.yaml
[state-mgr]: https://github.com/openbmc/phosphor-state-manager
[bmc-state]:https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/BMC.interface.yaml
[dev-tree]:https://github.com/openbmc/docs/blob/master/designs/device-tree-gpio-naming.md
