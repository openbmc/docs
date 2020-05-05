### BMC Health Monitor

Author:
  Vijay Khemka <vijaykhemka@fb.com>; <vijay!>

Primary assignee:
  Vijay Khemka <vijaykhemka@fb.com>; <vijay!>

Created:
  2020-05-04

## Problem Description
This is to monitor health of BMC system with various components like CPU
usage, memory utilization etc. Actions can be taken based on monitored data
and provided configuration.

Why do we need to monitor CPU and memory, for any system these 2 data are
always important in terms of health of system. By monitoring these data an
alert can be generated and system administrator can take an action based on
alert.

## Background and References
None.

## Requirements
This should implement frequent capturing of CPU and memory usage at least to
start with and it can be extended for any other hardware like i2c bus, ECC.
This implementation should provide
- a daemon to collect varios component data frequently defined by user
- a daemon to monitor various component (CPU, Memory usage)
- a dbus interface to allow other services to access its data like redfish
- represent data over a dbus interface like a sensor
- capability to read component details from config file
- capability to take configuration based action when values crosses threshold

## Proposed Design
Create a D-bus service "xyz.openbmc_project.HealthMon" with object paths for
each component: "/xyz/openbmc_project/sensors/utilization/bmc_cpu",
"/xyz/openbmc_project/sensors/utilization/bmc_memory", etc.

There is a JSON configuration file for threshold, frequency of monitoring in
seconds, window size and actions.
For example,

```json
  "cpu" : {
    "frequency" : 1,
    "window_size": 120,
    "threshold":
    {
        "critical":
        {
            "value": 90.0,
            "log": true,
            "target": "reboot.target"
        },
        "warning":
        {
          "value": 80.0,
          "log": false,
          "target": "systemd unit file"
        }
    }
  },
  "memory" : {
    "frequency" : 1,
    "window_size": 120,
    "threshold":
    {
        "critical":
        {
            "value": 90.0,
            "log": true,
            "target": "reboot.target"
        }
    }
  }
```
frequency:   It is time in second when these data are collected in
             regular interval.
window_size: This is a value for number of samples taken to average
             out usage of system rather than taking an spike in usage
             data.
log:         A boolean value which allows to log an alert. Default value
             for this n critical is 'true' and in warning it is 'false'.
target:      This is a systemd target unit file which will called once
             value crosses its threshold and it is optional.

Structure like:

Under the D-bus named "xyz.openbmc_project.HealthMon":

```
    /xyz/openbmc_project
    └─/xyz/openbmc_project/sensors
      └─/xyz/openbmc_project/sensors/utilization/bmc_cpu
      └─/xyz/openbmc_project/sensors/utilization/bmc_memory
```

## Alternatives Considered
Implement as part of Entity manager and dbus sensors which may restrict to
the number of users. This will be an independent package which will provide
 monitoring of defined component and present data over dbus like utilization
sensors defined in phosphor-dbus-interface

## Impacts
This application is monitoring BMC health components and set values to D-bus.
The impacts should be small in the system.

## Testing
Testing can be done by monitoring data read from dbus interface over a period
of time and also can see these data if it changes by running loading cpu.
