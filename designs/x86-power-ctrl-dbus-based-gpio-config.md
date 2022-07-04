# x86-power-control DBUS based GPIO state change

Author:
  Naveen Moses, naveen.mosess@hcl.com

Created:
  July 4, 2022

## Problem Description
In x86 power control host power control events such as
**poweron** and **reset** events are based on directly mapped GPIOs.

There is an additional config option where a host GPIO config is mentioned
as DBUS type which means the GPIO state is not accessed directly
but via some other means such as via ipmi BIC command.

This design change is for multi-host platforms which need to set
the GPIO states whose state is not directly accessed but
 by sending IPMI BIC commands.

At present DBUS based GPIO config is available for only
reading such GPIO state. The DBUS based GPIO config shall be also
used for GPIOs whose state requires to be modified such as POWER_OUT and RESET_OUT.

## Requirement
In General, The bmc has direct GPIO access of host power and reset lines for
 power on and power off events.
These GPIO lines can be configured in x86 power control's json config file.

Some multi-host platforms don't have direct GPIO controlled power and reset
lines from BMC. The power and reset events are requested via ipmi BIC
commands rather than direct GPIO lines.

The current x86-power-control has option to read a GPIO state(input GPIO)
based on DBUS property if the GPIO config is mentioned as DBUS type. If a GPIO
is configured as DBUS type then the GPIO config has a DBUS property whose value
represents the state of the corresponding host GPIO state. The x86 Power
control monitors this DBUS property to look for GPIO state change.

In similar way the the DBUS type GPIO config support shall be be extended for
GPIOs that are configured as output(POWER_OUT and RESET_OUT).so if a GPIO
state needs to be modified then the state value is written to the DBUS
property provided part of the DBUS based GPIO config.

Example:
when the x86-power-control reads the GPIO config for POWER_OUT and it has DBUS type,
then to change the POWER_OUT the DBUS property is written with the state value.

Note : x86 power control only need to store the required GPIO state in
the provided DBUS property . sending the request to set the actual host
GPIO via IPMI BIC command is done by separate process which is not the
scope of x86-power-control

## Backround and reference

https://github.com/openbmc/x86-power-control/blob/master/README.md

gerrit link for reading GPIO state for dbus based config :
https://gerrit.openbmc.org/c/openbmc/x86-power-control/+/36528


## Proposed Design

The DBUS type GPIO config is same as given below

DBUS based GPIO config example

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

The GPIO config has the parameters associated with the GPIO specific
dbus object.

## Handling of  power event for dbus based gpio config

 In case of a power control event where a GPIO state needs to be
set such as (ex powerOn() or power off).

1. The power control checks the GPIO config type.

2. If the config type is GPIO then the GPIO state is changed as per
the current implementation.

3. If the config is DBUS type then the state which needs to be changed (HIGH/LOW)
 is written in the DBUS property provided in the DBUS GPIO config.

4. If the DBUS property change is success then proceed further.
   Otherwise, record an error if setting DBUS property fails.

## Impacts
This design does not have any impact on existing changes as
the code logic is based on the type specified in the GPIO config.