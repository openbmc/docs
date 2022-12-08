### BMC Health Monitor

Author: Vijay Khemka <vijaykhemka@fb.com>; <vijay!> Sui Chen
<suichen@google.com>

Created: 2020-05-04

## Problem Description

The problem is to monitor the health of a system with a BMC so we have some
means to make sure the BMC is working correctly. User can get required metrics
data as per configurations instantly. Set of monitored metrics may include CPU
and memory utilization, uptime, free disk space, I2C bus stats, and so on.
Actions can be taken based on monitoring data to correct the BMC’s state.

For this purpose, there may exist a metric producer (the subject of discussion
of this document), and a metric consumer (a program that makes use of health
monitoring data, which may run on the BMC or on the host.) They perform the
following tasks:

1. Configuration, where the user specifies what and how to collect, thresholds,
   etc.
2. Metric collection, similar to what the read routine in phosphor-hwmon-readd
   does.
3. Metric staging. When metrics are collected, they will be ready to be read
   anytime in accessible forms like DBus objects or raw files for use with
   consumer programs. Because of this staging step, the consumer does not need
   to poll and wait.
4. Data transfer, where the consumer program obtains the metrics from the BMC by
   in-band or out-of-band methods.
5. The consumer program may take certain actions based on the metrics collected.

Among those tasks, 1), 2), and 3) are the producer’s responsibility. 4) is
accomplished by both the producer and consumer. 5) is up to the consumer.

We realize there is some overlap between sensors and health monitoring in terms
of design rationale and existing infrastructure, so we largely follow the sensor
design rationale. There are also a few differences between sensors and metrics:

1. Sensor data originate from hardware, while most metrics may be obtained
   through software. For this reason, there may be more commonalities between
   metrics on all kinds of BMCs than sensors on BMCs, and we might not need the
   hardware discovery process or build-time, hardware-specific configuration for
   most health metrics.
2. Most sensors are instantaneous readings, while metrics might accumulate over
   time, such as “uptime”. For those metrics, we might want to do calculations
   that do not apply to sensor readings.

As such, BMC Health Monitoring infrastructure will be an independent package
that presents health monitoring data in the sensor structure as defined in
phosphor-dbus-interface, supporting all sensor packages and allowing metrics to
be accessed and processed like sensors.

## Background and References

References: dbus-monitor

## Requirements

The metric producer should provide

- A daemon to periodically collect various health metrics and expose them on
  DBus
- A dbus interface to allow other services, like redfish and IPMI, to access its
  data
- Capability to configure health monitoring
- Capability to take action as configured when values crosses threshold
- Optionally, maintain a certain amount of historical data
- Optionally, log critical / warning messages

The metric consumer may be written in various different ways. No matter how the
consumer is obtained, it should be able to obtain the health metrics from the
producer through a set of interfaces.

The metric consumer is not in the scope of this document.

## Proposed Design

The metric producer is a daemon running on the BMC that performs the required
tasks and meets the requirements above. As described above, it is responsible
for

1. Configuration
2. Metric collection and
3. Metric staging tasks

For 1) Configuration, There is a JSON configuration file for threshold,
frequency of monitoring in seconds, window size and actions. For example,

```json
  "CPU" : {
    "Frequency" : 1,
    "Window_size": 120,
    "Threshold":
    {
        "Critical":
        {
            "Value": 90.0,
            "Log": true,
            "Target": "reboot.target"
        },
        "Warning":
        {
          "Value": 80.0,
          "Log": false,
          "Target": "systemd unit file"
        }
    }
  },
  "Memory" : {
    "Frequency" : 1,
    "Window_size": 120,
    "Threshold":
    {
        "Critical":
        {
            "Value": 90.0,
            "Log": true,
            "Target": "reboot.target"
        }
    }
  }
```

Frequency : It is time in second when these data are collected in regular
interval. Window_size: This is a value for number of samples taken to average
out usage of system rather than taking a spike in usage data. Log : A boolean
value which allows to log an alert. This field is an optional with default value
for this in critical is 'true' and in warning it is 'false'. Target : This is a
systemd target unit file which will called once value crosses its threshold and
it is optional.

For 2) Metric collection, this will be done by running certain functions within
the daemon, as opposed to launching external programs and shell scripts. This is
due to performance and security considerations.

For 3) Metric staging, the daemon creates a D-bus service named
"xyz.openbmc_project.HealthMon" with object paths for each component:
"/xyz/openbmc_project/sensors/utilization/cpu",
"/xyz/openbmc_project/sensors/utilization/memory", etc. which will result in the
following D-bus tree structure

"xyz.openbmc_project.HealthMon":

```
    /xyz/openbmc_project
    └─/xyz/openbmc_project/sensors
      └─/xyz/openbmc_project/sensors/utilization/CPU
      └─/xyz/openbmc_project/sensors/utilization/Memory
```

## Alternatives Considered

We have tried doing health monitoring completely within the IPMI Blob framework.
In comparison, having the metric collection part a separate daemon is better for
supporting more interfaces.

We have also tried doing the metric collection task by running an external
binary as well as a shell script. It turns out running shell script is too slow,
while running an external program might have security concerns (in that the 3rd
party program will need to be verified to be safe).

## Impacts

Most of what the Health Monitoring Daemon does is to do metric collection and
update DBus objects. The impacts of the daemon itself should be small.

## Testing

To verify the daemon is functionally working correctly, we can monitor the DBus
traffic generated by the Daemon, and the readings on the Daemon’s DBus objects.

This can also be tested over IPMI/Redfish using sensor command as some of
metrics data are presented as sensors like CPU and Memory are presented as
utilization sensors.

To verify the performance aspect, we can stress-test the Daemon’s DBus
interfaces to make sure the interfaces do not cause a high overhead.
