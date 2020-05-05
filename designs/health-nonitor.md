### BMC Health Monitor

Author:
  Vijay Khemka <vijaykhemka@fb.com>; @vijay

Primary assignee:
  Vijay Khemka <vijaykhemka@fb.com>; @vijay

Created:
  May 4, 2020

## Problem Description
This is to monitor health of BMC system with various components like CPU
usage, memory utilization etc. Actions can be taken based of monitored data
and provided configuration.

## Background and References
None.

## Requirements
This should implement frequent capturing of CPU and memory usage at least to
start with and it can be extended for any other hardware like i2c bus, ECC.
This implementation should provide
- a daemon to monitor various component (CPU, Memory usage)
- a dbus interface to allow other services to access its data
- capability to read component details from config file
- capability to take configuration based action when values crosses threshold

## Proposed Design
Create a D-bus service "xyz.openbmc_project.HealthMon" with object paths for
each component: "/xyz/openbmc_project/HealthMon/cpu",
"/xyz/openbmc_project/sensors/HealthMon/memory", etc.

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
            "target": "reboot"
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
            "target": "reboot"
        }
    }
  }
```

Structure like:

Under the D-bus named "xyz.openbmc_project.HealthMon":

```
    /xyz/openbmc_project
    └─/xyz/openbmc_project/HealthMon
      └─/xyz/openbmc_project/HealthMon/cpu
      └─/xyz/openbmc_project/HealthMon/memory
```

## Alternatives Considered
None

## Impacts
This application is monitoring BMC health components and set values to D-bus.
The impacts should be small in the system.

## Testing
Testing can be done by monitoring data read from dbus interface over a period
of time and also can see these data if it changes by running loading cpu.
