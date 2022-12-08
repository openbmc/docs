# OpenBMC Server Power Recovery

Author: Andrew Geissler (geissonator)

Other contributors:

Created: October 11th, 2021

## Problem Description

Modern computer systems have a feature, automated power-on recovery, which in
essence is the ability to tell your system what to do when it hits issues with
power to the system. If the system had a black out (i.e. power was completely
cut to the system), should it automatically power the system on? Should it leave
it off? Or maybe the user would like the system to go to whichever state it was
at before the power loss.

There are also instances where the user may not want automatic power recovery to
occur. For example, some systems have op-panels, and on these op-panels there
can be a pin hole reset. This is a manual mechanism for the user to force a hard
reset to the BMC in situations where it is hung or not responding. In these
situations, the user may wish for the system to not automatically power on the
system, because they want to debug the reason for the BMC error.

During blackout scenarios, system owners may have a set of services they need
run once the power is restored. For example, IBM requires all LED's be toggled
to off in a blackout. OpenBMC needs to provide a mechanism for system owners to
run services in this scenario.

A brownout is another scenario that commonly utilizes automated power-on
recovery features. A brownout is a scenario where BMC firmware detects (or is
told) that chassis power can no longer be supported, but power to the BMC will
be retained. On some systems, it's desired to utilize the automated power-on
feature to turn chassis power back on as soon as the brownout condition ends.

Some system owners may chose to attach an Uninterrupted Power Supply (UPS) to
their system. A UPS continues to provide power to a system through a blackout or
brownout scenario. A UPS has a limited amount of power so it's main purpose is
to handle brief power interruptions or to allow for an orderly shutdown of the
host firmware.

The goal of this design document is to describe how OpenBMC firmware will deal
with these questions.

## Background and References

The BMC already implements a limited subset of function in this area. The
[PowerRestorePolicy][pdi-restore] property out in phosphor-dbus-interface
defines the function capability.

In smaller servers, this feature is commonly found within the Advanced
Configuration and Power Interface (ACPI).

[openbmc/phosphor-state-manager][state-mgr] supports this property as defined in
the phosphor-dbus-interface.

## Requirements

### Automated Power-On Recovery

OpenBMC software must ensure it persists the state of power to the chassis so it
can know what to restore it to if necessary

OpenBMC software must provide support for the following options:

- Do nothing when power is lost to the system (this will be the default)
- Always power the system on and boot the host
- Always power the system off (previous power was on, power is now off, run all
  chassis power off services to ensure a clean state of software and hardware)
- Restore the previous state of the chassis power and host

These options are only checked and enforced in situations where the BMC does not
detect that chassis power is already on to the system when it comes out of
reboot.

OpenBMC software must also support the concept of a one_time power restore
policy. This is a separate instance of the `PowerRestorePolicy` which will be
hosted under a D-Bus object path which ends with "one_time". If this one_time
setting is not the default, `None`, then software will execute the policy
defined under it, and then reset the one_time property to `None`. This one_time
feature is a way for software to utilize automated power-on recovery function
for other areas like firmware update scenarios where a certain power on behavior
is desired once an update has completed.

### BMC and System Recovery Paths

In situations where the BMC or the system have gotten into a bad state, and the
user has initiated some form of manual reset which is detectable by the BMC as
being user initiated, the BMC software must:

- Fill in appropriate `RebootCause` within the [BMC state interface][bmc-state]
  - At a minimum, `PinholeReset` will be added. Others can be added as needed
- Log an error indicating a user initiated forced reset has occurred
- Not log an error indicating a blackout has occurred if chassis power was on
  prior to the pin hole reset
- Not implement any power recovery policy on the system
- Turn power recovery back on once BMC has a normal reboot

### Blackout

A blackout occurs when AC power is cut from the system, resulting in a total
loss of power if there is no UPS installed to keep the system on. To identify
this scenario after a BMC reboot, chassis-state-manager will check to see what
the last power state was before the loss of power and compares it against the
pgood pin. Blackouts can be intentionally triggered by a user (i.e a pinhole
reset) or in severe cases occur when there is some sort of an external outage.
In either case the BMC must take into account this detrimental state. When this
condition occurs, the BMC may(depending on configuration):

- Provide a generic target, `obmc-chassis-blackout@.target` to be called when a
  blackout is detected
- Adhere to the current power restore policy

BMC firmware must also be able to:

- Discover why the system is in a blackout situation. From either loss of power
  or user actions.

### Brownout

As noted above, a brownout condition is when AC power can not continue to be
supplied to the chassis, but the BMC can continue to have power and run.

When this condition occurs, the BMC must:

- Power system off as quickly as situations requires (or gracefully handle the
  loss of power if it occurred without warning)
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
- Not run any power-on recovery logic when a brownout is occurring
- Tell the host firmware that it is a automated power-on recovery initiated boot
  when that firmware is what boots the system

### Uninterruptible Power Supply (UPS)

When a UPS is present and a blackout or brownout condition occurs, the BMC must:

- Log an error to indicate the condition has occurred
- If host firmware is running, notify the host firmware of this utility failure
  condition (this behavior is build-time configurable)
- If the UPS battery power becomes low and if host firmware is running, notify
  the host firmware of the condition, indicating a quick power off is required
  (this behavior is build-time configurable)
- Log an error if the UPS battery power becomes low and a power loss to the
  entire system is imminent(i.e. a blackout scenario where BMC will also lose
  power and UPS is about to run out of power)
- Not execute any automated power-on recovery logic to prevent power on/off
  thrasing (this behavior is build-time configurable)

## Proposed Design

### Automated Power-On Recovery

An application will be run after the chassis and host states have been
determined which will only run if the chassis power is not on.

This application will look for the one_time setting and use it if its value is
not `None`. If it does use the one_time setting then it will reset it to `None`
once it has read it. Otherwise the application will read the persistent value of
the `PowerRestorePolicy`. The application will then run the logic as defined in
the Requirements above.

This function will be hosted in phosphor-state-manger and potentially
x86-power-control.

### BMC and System Recovery Paths

The BMC state manager application currently looks at a file in the sysfs to try
and determine the cause of a BMC reboot. It then puts this reason in the
`RebootCause` property.

One possible cause of a BMC reset is an external reset (EXTRST). There are a
variety of reasons an external reset can occur. Some systems are adding GPIOs to
provide additional detail on these types of resets.

A new GPIO name will be added to the [device-tree-gpio-naming.md][dev-tree]
which reports whether a pin hole reset has occurred on the previous reboot of
the BMC. The BMC state manager application will enhance its support of the
`RebootCause` to look for this GPIO and if present, read it and set
`RebootCause` accordingly when it can either not determine the reason for the
reboot via the sysfs or sysfs reports a EXTRST reason (in which case the GPIO
will be utilized to enhance the reboot reason).

If the power recovery software sees the `PinholeReset` reason within the
`RebootCause` then it will not implement any of its policy. Future BMC reboots
which are not pin hole reset caused, will cause `RebootCause` to go back to a
default and therefore power recovery policy will be re-enabled on that BMC boot.

The phosphor-state-manager chassis software will not log a blackout error if it
sees the `PinholeReset` reason (or any other reason that indicates a user
initiated a reset of the system).

### Blackout

A new systemd target `obmc-chassis-blackout.target` should be added to allow
system maintainers to call services in this condition. This new target will be
called when the BMC detects a blackout. The target will allow for system owners
to add their own specific services to this new target.
Phosphor-chassis-state-manager will ensure `obmc-chassis-blackout.target` will
be called after a blackout.

### Brownout

The existing `xyz.openbmc_project.State.Chassis` interface will be enhanced to
support a `CurrentPowerStatus` property. The existing
phosphor-chassis-state-manager, which is instantiated per instance of chassis in
the system, will support a read of this property. The following will be the
possible returned values for the power status of the target chassis:

- `Undefined`
- `BrownOut`
- `UninterruptiblePowerSupply`
- `Good`

The phosphor-psu-monitor application within the phosphor-power repository will
be responsible for monitoring for brownout conditions. It will support a
per-chassis interface which represents the status of the power going into the
target chassis. This interface will be generic in that other applications could
host it to report the status of the power. The state-manager software will
utilize mapper to look for all implementations of the interface for its chassis
and aggregate the status (i.e. if any reports a brownout, then `BrownOut` will
be returned). This interface will be defined in a later update to this document.

The application(s) responsible for detecting and reporting chassis power will
run on startup and discover the correct state for their property. These
applications will log an error when a brownout occurs and initiate the fast
power off.

If the system design needs it, the existing one-time function provided by
phosphor-state-manager for auto power on policy will be utilized for when the
brownout completes.

When the phosphor-power application detects that a brownout condition has
completed it will reset its interface representing power status to good and
start the state-manager service which executes the automated power-on logic.

phosphor-state-manager will ensure automated power-on recovery logic is only run
when the power supply interface reports the power status is good. If there are
multiple chassis and/or host instances in the system then the host instances
associated with the chassis(s) with a bad power status will be the only ones
prevented from booting.

### Uninterruptible Power Supply (UPS)

A new phosphor-dbus-interface will be defined to represent a UPS. A BMC
application will implement one of these per UPS attached to the system. This
application will monitor UPS status and monitor for the following:

- UPS utility fail (system power has failed and UPS is providing system power)
- UPS battery low (UPS is about to run out of power)

If the application sees power has been lost and the system is running on UPS
battery power then it will monitor for the power remaining in the UPS and notify
the host that a shutdown is required if needed. This application will also be
responsible for logging an error indicating the UPS backup power has been
switched to and set the appropriate property in their interface to indicate the
scenario is present when the system can no longer remain on.
phosphor-state-manager will query mapper for implementation of this new UPS
interface and utilize them in combination with power supply brownout status when
determining the value to return for its `CurrentPowerStatus`.

Similar to the above brownout scenario, phosphor-state-manager will ensure
automated power-on recovery logic is not run if `PowerStatus` is not set to
`Good`. This behavior will be build-time configurable within
phosphor-state-manager.

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

Validate that when multiple black outs occur, the firmware continues to try and
power on the system when policy is `AlwaysOn` or `Restore`.

On supported systems, a pin hole reset should be done with a system that has a
policy set to always power on. Tester should verify system does not
automatically power on after a pin hole reset. Verify it does automatically
power on when a normal reboot of the BMC is done.

A brownout condition should be injected into a system and appropriate paths
should be verified:

- Error log generated
- Host notified (if running and notification possible)
- System quickly powered off
- Power recovery function is not run while a brownout is present
- System automatically powers back on when brownout condition ends (assuming a
  one-time or system auto power-on recovery policy of `AlwaysOn` or `Restore`)

Plug a UPS into a system and ensure when power is cut to the system that an
error is logged and the host is notified and allowed to power off.

[pdi-restore]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Control/Power/RestorePolicy.interface.yaml
[state-mgr]: https://github.com/openbmc/phosphor-state-manager
[bmc-state]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/BMC.interface.yaml
[dev-tree]:
  https://github.com/openbmc/docs/blob/master/designs/device-tree-gpio-naming.md
