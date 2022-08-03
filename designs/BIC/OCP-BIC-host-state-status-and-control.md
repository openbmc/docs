# multi-host OCP BIC host state status and control

# Problem description

The x86 power control has host power control events such as
**poweron** and **reset** events which are based on directly mapped GPIOs.
Also the power control monitors input power lines like POWER_GOOD line which is
also commonly directly mapped GPIOs.

In some multi-host platforms there are use cases where power lines such as
POWER_GOOD are not direct mapped GPIOs. These power line status which are specific
to each hosts can only accessed by sending IPMI BIC commands which is routed
via BIC HW and response is sent as a IPMI BIC response.

These IPMI commands to request power line states are not generic but OEM specific
so this cannot be added part of the power-control process also as it deviates from
the power control's scope.

The x86-power-control has option to monitor such not directly mapped GPIO power
lines over a dbus object.(i.e a dbus object associated with the power line is
monitored for the state change).

design : https://gerrit.openbmc.org/c/openbmc/docs/+/55095/4/designs/x86-power-ctrl-dbus-based-gpio-config.md


For the above design is only a part as, there should be a separate process
 which should be polling for such power line state change and update its status
 on the dbus object which power control requires.

This is the reason for the proposed process which does the IPMB based host state
or power status lines status and control.

# Requirements

A separate IPMI BIC daemon needs to be created for monitoring states of these
IPMI based input power lines/host states as well it should be able to send IPMI
commands in case the power line is output and its state needs to be set when
requested by other process via dbus call (i.e power control).

# Proposed design

## IPMI based output power line

1. For IPMI based Power out lines, The daemon should accept specific power line
state change calls via dbus from other process(x86 power control).
2. It should be able to use the IPMB daemon to send the respective IPMB command
for setting the state of specific power line.
3. In case of any error in the response it should be reported back to the
requester process via dbus.

## IPMI based input power line

1. For IPMI based input power lines, the daemon should create a dbus property
and poll for the specific power line state via IPMB command requests and update
the state in the dbus property based on the IPMI command response.
2. the x86 power control app shall monitor these dbus property to see the power
line changes and act upon.

**Note** : The monitoring of the input power lines can be based on a config
            file which keeps list of the power lines / other states which
            needs to be monitored.

# Alternatives considered

We have submitted a patch under dbus-sensors which is similar to IPMB based
sensor process which  monitors the power lines (which is part host controller
and accessed by requesting via BIC) state changes for the configured
power lines.

https://gerrit.openbmc.org/c/openbmc/dbus-sensors/+/47952

From the community, it is suggested that finding alternate implementation
we have decided to create a separate process which does oem specific IPMB
based host state monitoring.

## Impacts
This design does not have direct impacts with other bmc processes.

## Testing
The proposed design can be tested in a platform which supports multi-host
based BIC.