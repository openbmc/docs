### BMC Health Monitor

Author: Vijay Khemka <vijaykhemka@fb.com>, Sui Chen <suichen@google.com>, Jagpal
Singh Gill <paligill@gmail.com>

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
of design rationale and existing infrastructure. But there are also quite a few
differences between sensors and metrics:

1. Sensor data originate from hardware, while most metrics may be obtained
   through software. For this reason, there may be more commonalities between
   metrics on all kinds of BMCs than sensors on BMCs, and we might not need the
   hardware discovery process or build-time, hardware-specific configuration for
   most health metrics.
2. Most sensors are instantaneous readings, while metrics might accumulate over
   time, such as “uptime”. For those metrics, we might want to do calculations
   that do not apply to sensor readings.
3. Metrics can represent device attributes which don't change, for example,
   total system memory which is constant. Contrary, the primary intention of
   sensors is to sense the change in attributes and represent that variability.
4. Metrics are expressed in native units such as bytes for memory. Sensors
   infrastructure doesn't adhere to this and community has rejected the proposal
   to add bytes for sensor unit.

Based on above, it doesn't sound reasonable to use sensors for representing the
metrics data.

## Background and References

References: dbus-monitor

## Requirements

The metric producer should provide

- A daemon to periodically collect various health metrics and expose them on
  DBus.
- A dbus interface to allow other services, like redfish and IPMI, to access its
  data.
- Capability to configure health monitoring for wide variety of metrics, such as
  Memory Utilization, CPU Utilization, Reboot Statistics, etc.
- Capability to provide granular details for various metric types, for example -
  - Memory Utilization - Free Memory, Shared Memory, Buffered&CachedMemory, etc.
  - CPU Utilization - Userspace CPU Utilization, Kernelspace CPU Utilization,
    etc.
  - Reboot Statistics - Normal reboot count, Reboot count with failures, etc.
- Capability to take action as configured when values crosses threshold.
- Optionally, maintain a certain amount of historical data.
- Optionally, log critical / warning messages.

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
3. Metric staging & disperse tasks

For 1) Configuration, the daemon will have a default in code configuration.
Platform may supply a configuration file if it wants to over-ride the specific
default attributes. The format for the JSON configuration file is as under -

```json
  "kernel" : {
    "Frequency" : 1,
    "Window_size": 120,
    "Type": "CPU",
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
  "available" : {
    "Frequency" : 1,
    "Window_size": 120,
    "Type": "Memory",
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
it is optional. Type: This indicates the type of configuration entry. Possible
values are Memory, CPU, Reboot, Storage.

For 2) Metric collection, this will be done by running certain functions within
the daemon, as opposed to launching external programs and shell scripts. This is
due to performance and security considerations.

For 3) Metric staging & disperse, the daemon creates a D-bus service named
"xyz.openbmc_project.HealthManager". The design proposes new
[Metrics Dbus interfaces](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/64914).

| Interface Name                       | Purpose                                                                        | Required/Optional |
| :----------------------------------- | :----------------------------------------------------------------------------- | :---------------- |
| xyz.openbmc_project.Metric.Value     | Interface to represent value for Metrics.                                      | Required          |
| xyz.openbmc_project.Metric.Reset     | Interface to reset persistent Metrics counters.                                | Optional          |
| xyz.openbmc_project.Common.Threshold | Interface to represent Metric thresholds and signals for threshold violations. | Optional          |
| xyz.openbmc_project.Time.EpochTime   | Interface to indicate when the metric was collected.                           | Optional          |

Each metric will be exposed on a specific object path and above interfaces will
be implemented at these paths.

```
/xyz/openbmc_project
    |- /xyz/openbmc_project/metric/bmc/memory/total
    |- /xyz/openbmc_project/metric/bmc/memory/free
    |- /xyz/openbmc_project/metric/bmc/memory/available
    |- /xyz/openbmc_project/metric/bmc/memory/shared
    |- /xyz/openbmc_project/metric/bmc/memory/buffered_and_cached
    |- /xyz/openbmc_project/metric/bmc/cpu/user
    |- /xyz/openbmc_project/metric/bmc/cpu/kernel
    |- /xyz/openbmc_project/metric/bmc/reboot/count
    |- /xyz/openbmc_project/metric/bmc/reboot/count_with_failure
```

Servers for Metrics Data

| Interface Name     | Interface Server        | Info Source                                            |
| :----------------- | :---------------------- | :----------------------------------------------------- |
| Memory Utilization | phosphor-health-manager | /proc/meminfo                                          |
| CPU Utilization    | phosphor-health-manager | /proc/stat                                             |
| Reboot Statistics  | phosphor-state-manager  | Persistent counters incremented based on reboot status |

Multiple devices of same type -

In case there are multiple devices of same type, the D-Bus path can be extended
to add context about **"which device"**. For example -

```
/xyz/openbmc_project/metric/device-0/memory/total
/xyz/openbmc_project/metric/device-1/memory/total
...
```

These paths can be hosted by different daemons, for example, pldmd can host DBus
paths for BICs if master BMC uses PLDM to communicate with BIC. The Value
interface for each metric would need to be associated with the appropriate
system inventory item.

## Alternatives Considered

We have tried doing health monitoring completely within the IPMI Blob framework.
In comparison, having the metric collection part a separate daemon is better for
supporting more interfaces.

We have also tried doing the metric collection task by running an external
binary as well as a shell script. It turns out running shell script is too slow,
while running an external program might have security concerns (in that the 3rd
party program will need to be verified to be safe).

Collected: Collectd provides multiple plugins which allows to gather wide
variety of metrics from various sources and provides mechanisms to store them in
different ways. For exposing these metrics to DBus, a Collectd C plugin can be
written.

Pros:

- Off the shelf tool with support for lot of metrics.

Cons:

- Due to support for wide variety of systems (Linux, Solaris, OpenBSD, MacOSX,
  AIX, etc) and applications, the amount of code for each Collected plugin is
  pretty significant. Given the amount of functionality needed for openBMC,
  Collectd seems heavyweight. Majority of phosphor-health-monitor code will be
  around exposing the metrics on Dbus which will also be needed for Collectd
  plugin. Hence, directly reading from /proc/<fileX> seems lightweight as code
  already exist for it.
- Collected has minimal support for threshold monitoring and doesn't allow
  starting systemd services on threshold violations.

## Future Enhancements

Extend Metrics Dbus interface for -

- Storage
- Inodes
- Port/Network Statistics
- BMC Daemon Statistics

## Impacts

Most of what the Health Monitoring Daemon does is to do metric collection and
update DBus objects. The impacts of the daemon itself should be small.

The proposed design changes the DBus interface from Sensors to Metrics, so
following daemons would need to refactored/updated to account for interface
change -

- [BMCWeb](https://github.com/openbmc/bmcweb/blob/master/redfish-core/lib/manager_diagnostic_data.hpp)
- [phosphor-host-ipmid](https://grok.openbmc.org/xref/openbmc/openbmc/meta-quanta/meta-s6q/recipes-phosphor/configuration/s6q-yaml-config/ipmi-sensors.yaml?r=e4f3792f#82)

## Organizational

### Does this design require a new repository?

No, changes will go into phosphor-health-monitor.

### Which repositories are expected to be modified to execute this design?

- phosphor-health-monitor
- phosphor-state-manager
- BMCWeb
- phosphor-host-ipmid

## Testing

### Unit Testing

To verify the daemon is functioning correctly, monitor the DBus traffic
generated by the Daemon and the metric values from Daemon’s DBus objects.
Automated unit testing will be covered via GTest.

### Integration Testing

Manual end to end testing can be performed via Redfish GET for
ManagerDiagnosticData. The end to end automated testing will be covered using
openbmc-test-automation. To verify the performance aspect, we can stress-test
the Daemon’s DBus interfaces to make sure the interfaces do not cause a high
overhead.
