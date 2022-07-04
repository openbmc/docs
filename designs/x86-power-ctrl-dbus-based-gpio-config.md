# x86-power-control dbus based gpio state change

Author:
  Naveen Moses, naveen.mosess@hcl.com

Created:
  July 4, 2022

## Problem Description
In x86 power control host power control events such as
**poweron** and **reset** events are based on directly mapped GPIOs.

There is an additional config option where a host GPIO config is mentioned
as dbus type which means the GPIO state is not accessed directly
but via some other means such as via ipmi BIC command.

This design change is for multihost platforms which need to set
the GPIO states whose state is not direcetly accessed but
 by sending IPMI BIC commands.

At present dbus based gpio config is available for only
reading such GPIO state. The dbus based gpio config shall be also
used for GPIOs whose state requires to be modified such as POWER_OUT and RESET_OUT.

## Requirement
In General, The bmc has direct GPIO access of host power and reset lines for
 power on and power off events.
These gpio lines can be configured in x86 power control's json config file.

Some multihost platforms don't have direct GPIO controlled power and reset
lines from BMC. The power and reset events are requested via ipmi BIC
commands rather than direct GPIO lines.

The current x86-power-control has option to read a gpio state(input GPIO)
based on dbus property if the gpio config is mentioned as dbus type. If a gpio
is configured as dbus type then the gpio config has a dbus property whose value
represents the state of the corresponing host GPIO state. The x86 Power
control monitors this dbus property to look for GPIO state change.

In similar way the the dbus type gpio config support shall be be extended for
GPIOs that are configured as output(POWER_OUT and RESET_OUT).so if a GPIO
state needs to be modified then the state value is written to the dbus
property provided part of the dbus based GPIO config.

Example:
when the x86-power-control reads the gpio config for POWER_OUT and it has dbus type,
then to change the POWER_OUT the dbus property is written with the state value.

Note : x86 power control only need to store the required gpio state in
the provided dbus property . sending the request to set the actual host
GPIO via IPMI BIC command is done by separate process which is not the
scope of x86-power-control

## Backround and reference

https://github.com/openbmc/x86-power-control/blob/master/README.md

gerrit link for reading GPIO state for dbus based config :
https://gerrit.openbmc.org/c/openbmc/x86-power-control/+/36528


## Proposed Design

The dbus type gpio config is same as given below

Dbus based gpio config example

```
    {
        "Name" : "PowerOut",
        "DbusName" : "xyz.openbmc_project.IpmbSensor",
        "Path" : "/xyz/openbmc_project/gpio/ipmbGpioState/1_Power_Out",
        "Interface" : "xyz.openbmc_project.Chassis.Control.Power",
        "Property" : "PowerOut",
        "Type" : "DBUS"
    }
```

The Gpio config has the parameters associated with the GPIO specific
dbus object.

## Handling of  power event for dbus based gpio config

 In case of a power control event where a GPIO state needs to be
set such as (ex powerOn() or power off).

1. The power control checks the gpio config type.

2. Ff the config type is gpio then the gpio state is changed as per
the current implementation.

3. If the config is dbus type then the state which needs to be changed (HIGH/LOW)
 is written in the dbus property provided in the dbus gpio config.

4. If the Dbus property change is success then proceed further.
   Otherwise, record an error if setting dbus property fails.

## Impacts
This design does not have any impact on existing changes as
the code logic is based on the type specified in the gpio config.