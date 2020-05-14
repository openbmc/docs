### Virtual sensors

Author:
  Vijay Khemka <vijaykhemka@fb.com>; <vijay!>

Primary assignee:
  Vijay Khemka <vijaykhemka@fb.com>; <vijay!>

Created:
  2020-05-13

## Problem Description
There are some sensors in the system whose values are derived from actual
sensor data and some other factor like fan speed for temperature
sensors, some complex and non standard scale factors or some other algorithmic
calculations. Some of Virtual sensor examples are:

1. Airflow: Function of Fan RPM reported in units of cubic-feet per second.
2. Voltage delta between two ADC inputs. When the actual diff is what which
   should not cross a threshold.
3. Power consumption when we have Energy/time.

This design will allow to generate a new virtual sensors which are defined to
be specific manipulation/function of existing sensors.

## Background and References
None.

## Requirements
This should implement a new virtual sensor for every given sensor and update
them as per changes in existing sensors.
This implementation should provide
- a daemon to create and monitor each sensor defined in config file and
  update them based on value change in each given sensor.
- Every virtual sensor will be waiting for event for change in dbus value of
  each sensor parameter and will update this sensor.
- Sensor parameter can be any sensor dbus path and it also supports
  chaining of virtual sensors.
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
    "Algo": "P1 + P2 + 5 - P3 * 0.1",
    "Params":
    {
      "P1": "/xyz/openbmc_project/sensor/temperature/"Inlet_Temp",
      "P2": "/xyz/openbmc_project/sensor/fan/fan_0",
      "P3": "200"
    },
    "Thresholds":
    {
	"CriticalHigh": 90,
	"CriticalLow": 20,
	"WarningHigh": 70,
	"WarningLow": 30
    }
  }

Name:       Name of virtual sensor
Algo:       An algorithm to be used to calculate value
Params:     It can be an existing/virtual sensor dbus path to read value or it
            can be a value to be used in calculation
Thresholds: It has critical and warning high/low value to monitor sensor
            value and used as per sensor interface defined in
            phosphor-dbus-interfaces.

```
## Alternatives Considered
None

## Impacts
This application is monitoring existing sensors whenever they change values
then only it will update this sensors so impact should be very minimal.

## Testing
Testing can be done by monitoring data read from dbus interface over a period
of time and also can see these data if it changes by comparing with given
sensors.
