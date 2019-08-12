# OpenBMC platform telemetry 

Author:
  Piotr Matuszczak <piotr.matuszczak@intel.com>

Primary assignee:
  Piotr Matuszczak

Other contributors:
  Pawel Rapkiewicz <pawel.rapkiewicz@intel.com> <pawelr>,
  Kamil Kowalski <kamil.kowalski@intel.com>

Created:
  2019-08-07

## Problem Description
The BMC on server platform gathers lots of telemetry data, which has to 
be exposed in clean, human readable and standardized format. The idea is 
to expose platform telemetry over the Redfish, since it is standard API
for platform manageability.

## Background and References
* OpenBMC telemetry leverages DMTF's [Redfish Telemetry Model][1] for exposing
platform telemetry over the network. 
* Metrics are gathered using [hwmon][3], standard Linux kernel mechanism for 
monitoring.
* OpenBMC telemetry leverages the [OpenBMC sensors architecture implementation][2].

## Requirements
* [OpenBMC sensors][2] support.


## Proposed Design
Redfish Telemetry Model shall implement Telemetry Service with the following
collection resources:
* Metric Definitions - contains the definition of metric properties
* Metric Report Definitions - defines how metric report shall be created 
(which metrics it shall contain, how often it shall be generated etc.)
* Metric Reports - contains actual metric reports containing telemetry data
generated according to the Metric Report Definitions
* Metric Triggers - contains thresholds and actions that apply to specific
metrics

OpenBMC telemetry architecture is shown on the diagram below. 

```ascii
   +--------------+               +----------------+     +-----------------+
   |hwmon|        |               |Dbus sensors|   |     |Monitoring|      |
   +-----/        |               +------------/   |     |service   |      |
   |              +--filesystem--->                |     +----------/      |
   |              |               |                |     |                 |
   +--------------+               +--------^-------+     +--------^--------+
                                           |                      |
                                           |                      |
<------------------------------------------v-----^--DBus+---------v----------------->
                                                 |
                                                 |
+-------+---------------------------------------------------------------------------+
|bmcweb |                                        |                                  |
+-------/                                        |                                  |
|                                                |                                  |
|  +--------+------------------------------------v------------------------------+   |
|  |Redfish |                                                                   |   |
|  +--------/                                            +---------+--------+   |   |
|  |                                                     |Existing |        |   |   |
|  | +------------------------------------------------+  |Redfish  |        |   |   |
|  | |Telemetry Service|                              |  |resources|        |   |   |
|  | +----------------+/                              |  +---------/        |   |   |
|  | |  +----------+  +-----------+  +-------------+  |  |   +---------+    |   |   |
|  | |  |  Metric  |  |  Metric   |  |Metric report|  |  |   | Redfish |    |   |   |
|  | |  | triggers |  |definitions|  |definitions  <---------+ sensors |    |   |   |
|  | |  |          |  |           |  |             |  |  |   |         |    |   |   |
|  | |  +----+-----+  +-----+-----+  +------+------+  |  |   +---------+    |   |   |
|  | |       |              |               |         |  |                  |   |   |
|  | |       |              |               |         |  |                  |   |   |
|  | |       |              |               |         |  |                  |   |   |
|  | |       |        +-----v-----+         |         |  |                  |   |   |
|  | |       |        |   Metric  |         |         |  |                  |   |   |
|  | |       +-------->   report  <---------+         |  |                  |   |   |
|  | |                |           |                   |  |                  |   |   |
|  | |                +-----------+                   |  |                  |   |   |
|  | |                                                |  |                  |   |   |
|  | +------------------------------------------------+  +------------------+   |   |
|  |                                                                            |   |
|  +----------------------------------------------------------------------------+   |
|                                                                                   |
+-----------------------------------------------------------------------------------+
```
The telemetry service component is a part of Redfish and implements the DMTF's 
[Redfish Telemetry Model][1]. Metric report defnitions uses Redfish sensors 
URIs for metric report creation. The Telemetry Service obtains readings from
Redfish sensors Value property. Redfish sensors leverages Monitoring Service's
D-Bus interfaces to obtain metrics. 

The purpose of the Monitoring Service component is to connect directly to 
D-Bus sensors, gather their metrics and perform aggregated operations on 
them like MIN, MAX, SUM and AVERAGE. It also is responsible for delivering
readings with defined period and support other type of triggers, like 
on change and on request. This component acts as telemetry aggregator.

**Monitoring service on [D-Bus][4]**

Monitoring service exposes specific interfaces on D-Bus, that will be used for 
metric management.

```ascii
xyz.openbmc_project.MonitoringService
/xyz/openbmc_project/MonitoringService/Metrics
```

| Name | Type | Signature | Result/Value | Flags |
|------|------|-----------|--------------|-------|
|```org.freedesktop.DBus.Introspectable``` | interface | - | -             | - |
|```.Introspect```                         | method    | - | s             | - |
|```org.freedesktop.DBus.ObjectManager```  | interface | - | -             | - |
|```.GetManagedObjects```                  | method    | - | a{oa{sa{sv}}} | - |
|```.InterfacesAdded```                    | signal    | oa{sa{sv}} | - | - |
|```.InterfacesRemoved```                  | signal    | oas | - | - |
|```org.freedesktop.DBus.Peer```           | interface | - | - | - |
|```.GetMachineId```                       | method    | - | s | - |
|```.Ping```                               | method    | - | - | - |
|```org.freedesktop.DBus.Properties```     | interface | - | - | - |
|```.Get```                                | method    | ss | v | - |
|```.GetAll```                             | method    | s  | a{sv} | - |
|```.Set```                                | method    | ssv | - | - |
|```.PropertiesChanged```                  | signal    | sa{sv}as | - | - |
|```xyz.openbmc_project.MonitoringService.MetricsManagement``` | interface | - | - | - |
|```.AddMetric```                          | method    | ssuas | s | - |
|```.MaxMetricRecords```                   | property  | u | 10 | emits-change |
|```.PollRateResolution```                 | property  | u | 100 | emits-change |

The ```AddMetric``` method has the following arguments:

| Argument | Type | Description |
|----------|------|-------------|
| Prefix | string | Defines prefix for metric so it will be "<prefix\><randomString\>" i.e.: for prefix "test" -> testqrapndyY |
| ReportingType | string | Reporting type: "xyz.openbmc_project.MonitoringService.Metric.Periodic" - For periodic update "xyz.openbmc_project.MonitoringService.Metric.OnChange" - For update when value changes "xyz.openbmc_project.MonitoringService.Metric.OnRequest" - For update when user requests data |
| ScanPeriod | uint32_t | Scan period used when Periodic type is set (in milliseconds) |
| ReadingPaths | array of string | Collection of paths to sensors that should be included in this metric |

The ```AddMetric``` method returns:
```ascii
String for created metric - ie. '/xyz/openbmc_project/MonitoringService/Metrics/testqrapndyY'
```

Such created metric implements the following interfaces, methods and properties:

| Name | Type | Signature | Result/Value | Flags |
|------|------|-----------|--------------|-------|
|```org.freedesktop.DBus.Introspectable``` | interface | - | - | - |
|```.Introspect```                         | method    | - | s | - |
|```org.freedesktop.DBus.Peer```           | interface | - | - | - |
|```.GetMachineId```                       | method    | - | s | - |
|```.Ping```                               | method    | - | - | - |
|```org.freedesktop.DBus.Properties```     | interface | - | - | - |
|```.Get```                                | method    | ss | v | - |
|```.GetAll```                             | method    | s | a{sv} | - |
|```.Set```                                | method    | ssv | - | - |
|```.PropertiesChanged```                  | signal    | sa{sv}as | - | - |
|```xyz.openbmc_project.Object.Delete```   | interface | - | - | - |
|```.Delete```                             | method    | - | - | - |
|```xyz.openbmc_project.MonitoringService.Metric``` | interface | - | - | - |
|```.ReadingPaths```                       | property  | as | 1 "/" | emits-change writable |
|```.Readings```                           | property  | a(svs) | 0 | emits-change writable |
|```.ReportingType```                      | property  | s | One of reporting type strings| emits-change writable |
|```.ScanPeriod```                         | property  | u | 100 | emits-change writable |



**Telemetry Service Redfish API**

Telemetry service shall support 2018.2 Redfish schemas for telemetry resources. 
Metric report definitions determines which metrics are to be included in metric 
report. Metric definition is assigned to particular metric type and it
describes how the metric should be interpreted. The following resource schemas 
shall be supported:

- TelemetryService 1.0.0
- MetricDefinition 1.0.0
- MetricReportDefinition 1.0.0
- MetricReport 1.0.0

The following diagram shows relations between these resources.

```ascii
                                   +--------------+
                                   |              |
                                   | Service root |
                                   |              |
                                   +--+--------+--+
                                      |        |
                        +-------------+        +----------------+
                        |                                       |
                +-------v-------+                    +----------v----------+
                |               |                    |Chassis              |
                |   Telemetry   |                    |                     |
                |    service    |                    |                     |
                |               |                    |  +---------------+  |
                +--+---------+--+                    |  |               |  |
                   |         |                       |  |   Chassis 1   |  |
         +---------+         +--------+              |  |               |  |
         |                            |              |  +--------------++  |
         |                            |              |                 |   |
+--------v--------------+  +----------v--------+     +---------------------+
|Metric definition      |  |Metric report      |                       |
|                       |  |                   |                       |
| +-------------------+ |  |                   | Reads                 |
| |ReadingVolts       | |  | +--------------+  | ReadingVolts       +--v------+
| |                   <------+              +----------------------->         |
| +-------------------+ |  | |    Metric    |  |                    |         |
|                       |  | |   report 1   |  | Reads              |  Power  |
| +-------------------+ |  | |              |  | PowerConsumedWatts |         |
| |PowerConsumedWatts <------+              +----------------------->         |
| |                   | |  | +--------------+  |                    +---------+
| +-------------------+ |  |                   |
|                       |  |                   |
|                       |  |                   |
+-----------------------+  +-------------------+
```

Metric readings for metric report shall be obtained by the Telemetry Service
from the Monitoring Service component. Metric report shall be generated 
pedriodically, on demand or on metric change. Below examples of Redfish
resources for the Telemetry Service are shown. 

The telemetry service Redfish resource example:

```json
{
    "@odata.type": "#TelemetryService.v1_0_0.TelemetryService",
    "Id": "TelemetryService",
    "Name": "Telemetry Service",
    "Status": {
        "State": "Enabled",
        "Health": "OK"
    },
    "MinCollectionInterval": "T00:00:10s",
    "SupportedCollectionFunctions": [],
    "MaxReports": <max_no_of_reports>,
    "MetricDefinitions": {
        "@odata.id": "/redfish/v1/TelemetryService/MetricDefinitions"
    },
    "MetricReportDefinitions": {
        "@odata.id": "/redfish/v1/TelemetryService/MetricReportDefinitions"
    },
    "MetricReports": {
        "@odata.id": "/redfish/v1/TelemetryService/MetricReports"
    },
    "LogService": {
        "@odata.id": "/redfish/v1/Managers/<manager_name>/LogServices/Journal"
    },
    "@odata.context": "/redfish/v1/$metadata#TelemetryService",
    "@odata.id": "/redfish/v1/TelemetryService"
}
```

Sample metric report definition:

```json
{
    "@odata.type": "#MetricReportDefinition.v1_0_0.MetricReportDefinition",
    "Id": "SampleMetric",
    "Name": "Sample Metric Report Definition",
    "MetricReportDefinitionType": "Periodic",
    "Schedule": {
        "RecurrenceInterval": "T00:00:10"
    },
    "ReportActions": [
        "LogToMetricReportsCollection"
    ],
    "ReportUpdates": "Overwrite",
    "MetricReport": {
        "@odata.id": "/redfish/v1/TelemetryService/MetricReports/SampleMetric"
    },
    "Status": {
        "State": "Enabled"
    },
    "Metrics": [
        {
            "MetricId": "Test",
            "MetricProperties": [
                "/redfish/v1/Chassis/NC_Baseboard/Power#/PowerControl/0/PowerConsumedWatts"
            ]
        }
    ],
    "@odata.context": "/redfish/v1/$metadata#MetricReportDefinition.MetricReportDefinition",
    "@odata.id": "/redfish/v1/TelemetryService/MetricReportDefinitions/PlatformPowerUsage"
}
```

Sample metric report:

```json
{
    "@odata.type": "#MetricReport.v1_0_0.MetricReport",
    "Id": "SampleMetric",
    "Name": "SampleMetric report",
    "ReportSequence": "0",
    "MetricReportDefinition": {
        "@odata.id": "/redfish/v1/TelemetryService/MetricReportDefinitions/SampleMetric"
    },
    "MetricValues": [
        {
            "MetricDefinition": {
                "@odata.id": "/redfish/v1/TelemetryService/MetricDefinitions/SampleMetricDefinition"
            },
            "MetricId": "Test",
            "MetricValue": "100",
            "Timestamp": "2016-11-08T12:25:00-05:00",
            "MetricProperty": "/redfish/v1/Chassis/Tray_1/Power#/0/PowerConsumedWatts"
        }
    ],
    "@odata.context": "/redfish/v1/$metadata#MetricReport.MetricReport",
    "@odata.id": "/redfish/v1/TelemetryService/MetricReports/AvgPlatformPowerUsage"
}
```

## Alternatives Considered
None

## Impacts
This design impacts the architecture of the bmcweb component, since it adds
the Telemetry Service implementation as a component for the existing 
Redfish API implementation. 

## Testing
TBD

[1]: https://www.dmtf.org/sites/default/files/standards/documents/DSP2051_0.1.0a.zip
[2]: https://github.com/openbmc/docs/blob/master/sensor-architecture.md
[3]: https://www.kernel.org/doc/Documentation/hwmon/
[4]: https://www.freedesktop.org/wiki/Software/dbus/