# Updates for BMC Health Monitor

Author: Jagpal Singh Gill <paligill@gmail.com>

Created: July 11th, 2023

## Intent

The intent of this proposal is to discuss limitations of current
phosphor-health-monitor design, enumerate requirements, and propose
updates/enhancements. The intent of this proposal is **not to add new daemon**
but to retrofit existing daemons to meet specified requirements.

## Problem Description

This section covers the limitations discoverd with phosphor-health-monitor -

1. Exposes high level metrics with less granularity, for example, only free
   memory in case of memory.
2. Exposes BMC statistics as sensors, hence restricts metric which can't be
   exposed as sensors, for example total system memory which is constant.
3. Metrics are only exposed in percentage units which doesn't adhere to Redfish
   model specification, for example, bytes for memory. This limitation is due to
   the fact that current implementation exposes metrics as sensors and community
   rejected the proposal to add bytes for sensor unit.

## Background and References

- [phosphor-health-monitor](https://github.com/openbmc/phosphor-health-monitor)
- [BMC Health Monitor design document](https://github.com/openbmc/docs/blob/master/designs/bmc-health-monitor.md)

## Requirements

1. Able to provide wide variety of metrics, such as Memory Utilization, CPU
   Utilization, Reboot Statistics, etc.
2. Able to provide granular details for metrics, for example -
   - For Memory Utilization - Free Memory, Shared Memory, Buffered&CachedMemory,
     etc.
   - For CPU Utilization - Userspace CPU Utilization, Kernelspace CPU
     Utilization, etc.
   - For Reboot Statistics - Normal reboot count, Reboot count with failures,
     etc.
3. Able to appropriately model constant system metrics, for example, total BMC
   memory.
4. Able to produce an interface compliant with Redfish Model for
   [Manager Diagnostic Data](https://redfish.dmtf.org/schemas/v1/ManagerDiagnosticData.v1_2_0.json).
5. Able to raise alerts when metrics cross specified threshold levels, for
   example, raise a signal when BMC memory consumption cross \<x> percentage.

## Proposed Design

### Dbus Interface

This design proposes new
[Metrics Dbus interfaces](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/64914).

| Interface Name                       | Purpose                                                                        |
| :----------------------------------- | :----------------------------------------------------------------------------- |
| xyz.openbmc_project.Metrics.Value    | Interface to represent value for Metrics.                                      |
| xyz.openbmc_project.Metrics.Reset    | Interface to reset persistent Metrics counters.                                |
| xyz.openbmc_project.Common.Threshold | Interface to represent Metric thresholds and signals for threshold violations. |

Each metric will be exposed on a specific object path and above interfaces will
be implemented at these paths. Reset Interfaces are optional and may only be
implemented for persistent counters/intervals.

```
/xyz/openbmc_project
    |- /xyz/openbmc_project/metrics/bmc/memory/total
    |- /xyz/openbmc_project/metrics/bmc/memory/free
    |- /xyz/openbmc_project/metrics/bmc/memory/available
    |- /xyz/openbmc_project/metrics/bmc/memory/shared
    |- /xyz/openbmc_project/metrics/bmc/memory/buffered_and_cached
    |- /xyz/openbmc_project/metrics/bmc/cpu/user
    |- /xyz/openbmc_project/metrics/bmc/cpu/kernel
    |- /xyz/openbmc_project/metrics/bmc/reboot/count
    |- /xyz/openbmc_project/metrics/bmc/reboot/count_with_failure
```

Proposed solution exposes detailed statistics with required units which
addresses the Requirements# 1, 2, 3, 4 and Issue# 1, 2, 3. New Threshold
interface provides more generic model which can be easily extended by adding a
new threshold type (rather than creating an all new interface for new type).

### Multi Host Platforms

For multi host platforms, there may be proxy management controllers on other
slots which would need health monitoring. In those scenarios, DBus path can be
extended to add context about **"which manager"**. For example -

```
/xyz/openbmc_project/metrics/bmc-0/memory/total
/xyz/openbmc_project/metrics/bmc-1/memory/total
...
```

These paths can be hosted by different daemons, for example, pldmd can host DBus
paths for BICs if master BMC uses PLDM to communicate with BIC. The Value
interface for each metric would need to be associated with the appropriate
system inventory item.

### Servers for Metrics Data

| Interface Name     | Interface Server        | Info Source                                            |
| :----------------- | :---------------------- | :----------------------------------------------------- |
| Memory Utilization | phosphor-health-monitor | /proc/meminfo                                          |
| CPU Utilization    | phosphor-health-monitor | /proc/stat                                             |
| Reboot Statistics  | phosphor-state-manager  | Persistent counters incremented based on reboot status |

### Extensions for phosphor-health-monitor Configuration

Current phosphor-health-monitor consumes its configuration via a
[JSON file](https://github.com/openbmc/phosphor-health-monitor/blob/master/bmc_health_config.json).
phosphor-health-monitor can have a default internal configuration rather than
reading the default configuration from file. Platform may supply a configuration
file if it wants to over-ride the default behavior.

**Pros:**

- Prevents the need to read file from disk if no change needed in default
  configuration.
- Provides the flexibility to only over-ride specific atributes rather than
  re-writing the whole configuration file.

This proposal also extends configuration schema to add _Type_ field so multiple
metrics of same type can be categorized and implemented together.

```
  Type: [Memory, CPU, Reboot, Storage, ...]
```

## Alternatives Considered

### Collected

[Collectd](https://collectd.org/index.shtml) provides
[multiple plugins](https://collectd.org/documentation/manpages/collectd.conf.5.shtml)
which allows to gather wide variety of metrics from various sources and provides
mechanisms to store them in different ways. For exposing these metrics to DBus,
a Collectd C plugin can be written.

**Pros**:

- Off the shelf tool with support for lot of metrics.

**Cons**:

- Due to support for wide variety of systems (Linux, Solaris, OpenBSD, MacOSX,
  AIX, etc) and applications, the amount of code for each Collected plugin is
  pretty significant. Given the amount of functionality needed for openBMC,
  Collectd seems heavyweight. Majority of phosphor-health-monitor code will be
  around exposing the metrics on Dbus which will also be needed for Collectd
  plugin. Hence, directly reading from /proc/\<fileX> seems lightweight as code
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

Since proposed design changes the DBus interface from Sensors to Metrics,
following servers would need to refactored to account for change -

- BMCWeb
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

Majority of the changes will be in phosphor-health-monitor and will covered via
functional testing with GTest.

### Integration Testing

The end to end integration testing involving Servers (for example BMCWeb for
Redfish) will be covered using openbmc-test-automation.
