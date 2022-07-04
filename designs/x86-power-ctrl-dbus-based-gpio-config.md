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
GPIOs that are configured as output(POWER_OUT and RESET_OUT). So if a GPIO
state needs to be modified then the state value is written to the DBUS
property provided part of the DBUS based GPIO config.

Example:
when the x86-power-control reads the GPIO config for POWER_OUT and it has DBUS type,
then to change the POWER_OUT the DBUS property is written with the state value.

Note : x86 power control only need to store the required GPIO state in
the provided DBUS property . sending the request to set the actual host
GPIO via IPMI BIC command is done by separate process which is not the
scope of x86-power-control

## Background and reference

https://github.com/openbmc/x86-power-control/blob/master/README.md

Gerrit link for reading GPIO state for DBUS based config :
https://gerrit.openbmc.org/c/openbmc/x86-power-control/+/36528

## multi-host with x86-power-control

  The current x86-power-control has already have the multi-host implementation.
  this is achieved by spawning multiple instances of x86-power-control process
  w.r.t the host instances.

  Example :
  For single host only one power control process is spawned.
  For 4 host bmc 4 instances of x86-power-control process are spawned.

  Each power control instance also has a json config file associated, which has
   all the required power control related GPIO configs

  Reference config : https://github.com/openbmc/x86-power-control/blob/master/config/power-config-host0.json

## Proposed Design

The following is the example for a GPIO config of PowerOut:

**Direct access GPIO config example**
    {
        "Name" : "PowerOut",
        "LineName" : "POWER_OUT",
        "Type" : "GPIO",
        "Polarity" : "ActiveLow"
    }

**DBUS based GPIO config example**

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


## Existing implementation for handling direct access GPIO config

  In case of an power event the for example powerOn(), The powerOut GPIO is
  asserted and de-asserted by the following call.

```
  assertGPIOForMs(powerOutConfig, TimerMap["PowerPulseMs"]);

```
  The powerOutConfig has the direct access GPIO line details
  and that line is asserted for a specific duration( TimerMap["PowerPulseMs"])
  and de-asserted back.

## New implementation for handling DBUS based GPIO config

 In case of a power control event,

1. The power control process checks the GPIO config type.

2. If the config type is GPIO then the GPIO state is changed as per
the current implementation.

3. If the config is DBUS type then the state which needs to be changed (HIGH/LOW)
 is written in the DBUS property provided in the DBUS GPIO config.

4. If the DBUS property change is success then proceed further.
   Otherwise, record an error if setting DBUS property fails.

## DBUS based GPIO power event example

 lets consider powerOn() as example,

If the GPIO config type is GPIO( "Type" : "GPIO" ), then direct access assert api
is called.
i.e
```
   assertGPIOForMs(powerOutConfig, TimerMap["PowerPulseMs"]);
```

If the GPIO config type is DBUS( "Type" : "DBUS" ), then a new similar api
  can be introduced to set the GPIO state DBUS property.

i.e
```
    assertDbusGPIOForMs(powerOutConfig, TimerMap["PowerPulseMs"])
```

This new assert api internally changes the state of the DBUS based GPIO state
by setting the state as property which is part of the GPIO config
("Property" : "PowerOut").

**Note :**
  The process of requesting the actual GPIO state change event(request via
  IPMI BIC) w.r.t the DBUS GPIO config is handled by a separate process which
  is not part of x86-power-control's scope.

## Impacts
This design does not have any impact on existing changes as
the code logic is based on the type specified in the GPIO config.