### Virtual sensors

Author:
  Vijay Khemka <vijaykhemka@fb.com>; <vijay!>

Primary assignee:
  Vijay Khemka <vijaykhemka@fb.com>; <vijay!>

Created:
  2020-05-13

## Problem Description
There are some sensors in the system whose values are not accurate and more
precise accuracy depends on some other factor like fan speed for temperature
sensors, some complex and non standard scale factors or some other algorithmic
calculations.

This design will allow to generate a new virtual sensor for every given
sensor and report more accurate data. This will also help in driving fan
speeed with more accurate data.

## Background and References
None.

## Requirements
This should implement a new virtual sensor for every given sensor and update
them as per changes in existing sensors.
This implementation should provide
- a daemon to create and monitor each sensor defined in config file and
  update them based on value change in given sensor.
- a dbus interface like other sensors to allow other services to access its
  data
- capability to read parameter details from config file
- capability to read a algorithm string from config file and use the same
  algorithm in calculating value for new virtual sensor

## Proposed Design
Create a D-bus service "xyz.openbmc_project.VirtualSensor" with object paths
for each sensor: "/xyz/openbmc_project/sensor/temperature/sensor_name"

There is a JSON configuration file for name, old path and algorithm.
For example,

```json
  {
    "Name": "Virtual_Inlet_Temp",
    "Path": "/xyz/openbmc_project/sensor/temperature/"Inlet_Temp",
    "Algo": "Val + P1 + 5 - P2 * 0.1",
    "Params":
    {
      "P1": "/xyz/openbmc_project/sensor/fan/fan_0",
      "P2": "200"
    }
  }

Name:   Name of new sensor
Path:   dbus path of old sensor
Algo:   An algorithm to be used to calculate new value
Val:    It is read from given path
Params: It can be a path to read value or it can be a value to be used in
        calculation

```
## Alternatives Considered
None

## Impacts
This application is monitoring existing sensors whenever they change values
then only it will update new sensors so impact should be very minimal.

## Testing
Testing can be done by monitoring data read from dbus interface over a period
of time and also can see these data if it changes by comparing with given
sensors.
