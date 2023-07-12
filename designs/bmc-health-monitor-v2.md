# BMC Health Monitor V2

**Author**: Jagpal Singh Gill <jagpalgill@meta.com> **Created**: July 11th, 2023

## Intent

---

The purpose of this proposal is to enumerate generic BMC health monitoring
requirements, discuss limitations with current design and propose
updates/enhancements. The intent of this proposal is **not to add new daemon**
but to retrofit existing daemons to meet specified requirements.

## Requirements

---

1. The BMC health monitoring provides wide varierty of metrics, such as Memory
   Utilization, CPU Utilization, Reboot Statistics etc.
2. Provides granular attributes for metrics, for example -
   - For Memory Utilization - Free Memory, Shared Memory, Buffered&CachedMemory
     etc.
   - For CPU Utilization - Userspace CPU Utilization, Kernelspace CPU
     Utilization etc.
   - For Reboot Statistics - Normal Reboot Count, Reboot count with failures
     etc.
3. Provides system's static metrics such as total BMC memory.
4. Compliant with Redfish Model for
   [Manager Diagnostic Data](https://redfish.dmtf.org/schemas/v1/ManagerDiagnosticData.v1_2_0.json)
5. Asynchronously query and cache BMC health data for immediate consumption by
   clients.
6. Provides configuration knobs to tweak monitoring capabilities.
7. Raise alerts/alarms when metrics crosses specified threshold levels.
8. Start remediation actions/services in case of alarms/alerts.

## Background and References

---

There is an existing implemenation for BMC Health Monitor aka
[phosphor-health-monitor](https://github.com/openbmc/phosphor-health-monitor).
Refer to
[design document](https://github.com/openbmc/docs/blob/master/designs/bmc-health-monitor.md)
for more details.

## Problem Description

---

This section covers the limitations discoverd with
[phosphor-health-monitor](https://github.com/openbmc/phosphor-health-monitor) -

1. Exposes broader metrics with no granularities, such as only free memory (in
   case of memory).
2. Metrics are only exposed in percentage units which doesn't adhere to Redfish
   model specification (such as bytes for memory).
3. Exposes BMC statistics as sensors, hence limits metric types which can't be
   exposed as sensors, for example static system metrics
4. Implementation needs refactoring as all code is concentrated into
   healthMonitor.hpp and healthMonitor.cpp.

## Proposed Design

---

### Dbus Interface

This design proposes new
[Metrics Dbus interfaces](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/64914) -

- Each metric is a different Dbus interface, so they can be hosted by multiple
  daemons (if needed).
- Proposed interfaces exposes granular statistics with basic units which
  addresses the Requirements#1, 2, 3, 4 and Limitations# 1, 2.
- Proposed Dbus interface models statistics as metrics, hence addresses the
  limitation# 3.
- The metrics interfaces will be hosted on following object paths -

```
/xyz/openbmc_project
    |- /xyz/openbmc_project/Metrics
```

#### Sample Dbus Command

```
busctl get-property xyz.openbmc_project.HealthMon /xyz/openbmc_project/Metrics xyz.openbmc_project.Metrics.BMC.MemoryUtilization FreeMemory
```

#### Throshold Monitoring

Metrics thresholds would be monitored through existing
[threshold interface](https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/yaml/xyz/openbmc_project/Sensor/Threshold).

### Servers for Metrics DBus Interface

| Interface Name    |     Interface Server     |                      Info Source                       |
| ----------------- | :----------------------: | :----------------------------------------------------: |
| MemoryUtilization | phosphor-health-monitor  |                     /proc/meminfo                      |
| CpuUtilization    | phosphor-health-monitor  |                       /proc/stat                       |
| RebootStatistics  | phosphor-debug-collector | Persistent counters incremented based on reboot status |

### Extensions for phosphor-health-monitor Configuration

Current phoshor-health-monitor
[design](https://github.com/openbmc/docs/blob/master/designs/bmc-health-monitor.md)
supports requirement#6 through a
[configuration json](https://github.com/openbmc/phosphor-health-monitor/blob/master/bmc_health_config.json).
This proposal extends configuration schema to add _Type_ field. This will help
with multiple metrics of same type to be categorized and implemented together.

```
    Type: [Memory, CPU, Reboot, Storage, ...]
```

### Implementation Refactoring

#### phosphor-health-monitor

- Breakup existing
  [healthMonitor.hpp](https://github.com/openbmc/phosphor-health-monitor/blob/master/healthMonitor.hpp)
  and
  [healthMonitor.cpp](https://github.com/openbmc/phosphor-health-monitor/blob/master/healthMonitor.cpp)
  into submodules based on metrics type as memory, cpu, reboot statistics etc.
  This will also help health monitor design to fall in sync with proposed dbus
  interface.
- Rename HealthSensors as HealthMetrics and create a generic programming
  interface for other metrics type to extend.
- Make use of better coding techniques such as coroutines instead of
  boost::asio.

#### BMCWeb

- Refactor
  [BMCWeb implementation](https://grok.openbmc.org/xref/openbmc/bmcweb/redfish-core/lib/manager_diagnostic_data.hpp?r=ac106bf6#62)
  to use coroutines.

## Scope

---

Requirements#5, 6, 7 & 8 are already covered by current
[phosphor-health-monitor](https://github.com/openbmc/phosphor-health-monitor)
design, hence are out of scope of this proposal.

## Alternatives Considered

---

### Alternate SDBus Interface

Expose each metric as a different object path and implement
[Value](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Sensor/Value.interface.yaml)
and
[Threshold](https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/yaml/xyz/openbmc_project/Sensor/Threshold)
interface at these paths.

```
/xyz/openbmc_project
    |- /xyz/openbmc_project/Metrics/BMC/MemoryUtilization/TotalMemory
    |- /xyz/openbmc_project/Metrics/BMC/MemoryUtilization/FreeMemory
    |- /xyz/openbmc_project/Metrics/BMC/MemoryUtilization/AvailableMemory
    |- /xyz/openbmc_project/Metrics/BMC/MemoryUtilization/SharedMemory
    |- ...
    |- /xyz/openbmc_project/Metrics/BMC/CpuUtilization/UserCpuUtilization
    |- ...
    |- ...
```

#### Pros

- Client/BMCWeb have to make one dbus call to get all the _Value_ properties for
  _Value_ interface from the object path _/xyz/openbmc_project/Metrics_, so
  fewer number of sdbus calls. In contrary, with multiple interfaces in proposed
  design each interface request is a separate dbus call.

#### Cons

- Will end up using regex on object paths in situations, such as, when getting
  all memory metrics from
  \*/xyz/openbmc_project/Metrics/BMC/MemoryUtilization/\*\*. Using regexs on
  paths should be avoided.

The affect of the Pro# 1 can be minimized for proposed Metrics DBus interface
using parallel coroutines to fetch info from multiple interfaces in parallel.

## Future Enhancements

---

Extend Metrics Dbus interface for -

- Storage
- Inodes
- Port/Network Statistics
- BMC Daemon Statistics.

## Impacts

---

Since proposed design changes the DBus interface from Sensors to Metrics, any
**dependent client** code would need to refactored to account for change.

## Organizational

---

### Does this design require a new repository?

No, changes will go into phosphor-health-monitor.

### Which repositories are expected to be modified to execute this design?

- phosphor-health-monitor
- phosphor-debug-collector
- bmcweb

## Testing

---

gtest based unit test will be developed for API level testing. Jenkins will be
used for CI.
