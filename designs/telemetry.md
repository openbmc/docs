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
be exposed in clean, human readable and standardized format. This document
focuses on telemetry over the Redfish, since it is standard API
for platform manageability.

## Background and References
* OpenBMC telemetry leverages DMTF's [Redfish Telemetry Model][1] for exposing
platform telemetry over the network. 
* OpenBMC telemetry leverages the [OpenBMC sensors architecture implementation][2].
* Although we use the [hwmon][3] to gather readings from physical sensors, this
arhictecture does not depend on it, because the Monitoring Service component 
relies on the [OpenBMC D-Bus sensors][2]. 


## Requirements
* [OpenBMC D-Bus sensors][2] support. This is also design limitation, since the 
Monitoring Service requires telemetry sources to be implemented as D-Bus sensors.


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
<------------------------------------------v-----^--DBus----------v----------------->
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
[Redfish Telemetry Model][1]. Metric report definitions uses Redfish sensors 
URIs for metric report creation. Those sensors are also used to get 
URI->D-Bus sensor mapping. Redfish Telemetry Service acts as presentation 
layer for the telemetry, while Monitoring Service is responsible for gathering
metrics from D-Bus sensors and exposing them as D-Bus objects. Monitoring
Service supports different monitoring modes (periodic, on change and on demand) 
along with aggregated operations:
* SINGLE - current reading value
* AVERAGE - average value over defined time period
* MAX - max reading value during defined time period
* MIN - min reading value during defined time period
* SUM - sum of reading values over defined time period

The time period for calculating aggregated is taken from the Redfish Metric 
Definition resource for each sensor's metric.

Monitoring Service supprots creating and managing metric report, which may 
contain single or multiple metrics from sensors. This metric report is mapped 
to Metric Report for the Redfish Telemetry Service. 

**Monitoring service on [D-Bus][4]**

Monitoring service exposes specific interfaces on D-Bus, that will be used for 
metric management.

```ascii
xyz.openbmc_project.MonitoringService
/xyz/openbmc_project/MonitoringService/Reports
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
|```xyz.openbmc_project.MonitoringService.ReportsManagement``` | interface | - | - | - |
|```.AddReport```                          | method    | ssuas | s | - |
|```.MaxMetricRecords```                   | property  | u | 10 | emits-change |
|```.PollRateResolution```                 | property  | u | 100 | emits-change |

The ```AddReport``` method is used to create metric report. The report
may contain single or multiple sensor readings. It has the following arguments:

| Argument | Type | Description |
|----------|------|-------------|
| Prefix | string | Defines prefix for report so it will be "<prefix\><randomString\>" i.e.: for prefix "test" -> testqrapndyY |
| ReportingType | string | Reporting type: "xyz.openbmc_project.MonitoringService.Metric.Periodic" - For periodic update "xyz.openbmc_project.MonitoringService.Metric.OnChange" - For update when value changes "xyz.openbmc_project.MonitoringService.Metric.OnRequest" - For update when user requests data |
| ScanPeriod | uint32_t | Scan period used when Periodic type is set (in milliseconds) |
| MetricsParams | array of structures | Collection of metric parameters.  |

```MetricParams``` array entry is a structure contatining:
* Sensor's path - D-Bus path, path to the sensor providing readings
* Operation's type - enum {SINGLE, MAX, MIN, AVG, SUM}, information about aggregated
operation
* Metric id - string, contains unique metric id, that can be mapped to Redfish MetricId


The ```ScanPeriod``` is defined per report, thus all sensors listed in the MetricsParams
collection will be scanned wit the same frequency. Also the ReportingType is
defined per report. In case when *xyz.openbmc_project.MonitoringService.Metric.OnChange*
ReportingType was defined, metric report will emit signal when at least one 
reading has changed. 

The ```AddReport``` method returns:
```ascii
String for created report - ie. '/xyz/openbmc_project/MonitoringService/Reports/testqrapndyY'
```

Such created metric report implements the following interfaces, methods and properties:

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
|```xyz.openbmc_project.MonitoringService.Report``` | interface | - | - | - |
|```.ReadingParameters```                  | property  | a(sos) | 1 "/" | emits-change writable |
|```.Readings```                           | property  | a(svs) | 0 | emits-change read-only |
|```.ReportingType```                      | property  | s | One of reporting type strings| emits-change writable |
|```.ScanPeriod```                         | property  | u | 100 | emits-change writable |

The ReadingParameters property contains an array of structures containing 
unique metric id, D-Bus sensor path and aggregated operation type. This 
property is made writable in order to support metric report modifications.

| Field Type  | Field Description          |
|-------------|----------------------------|
| string      | Unique metric id           |
| object path | D-Bus sensor's path        |
| string      | Aggregated operation type  |

The Readings property contains the array of the structures containing metric
unique id, sensor's reading value and reading timestamp.

| Field Type | Field Description          |
|------------|----------------------------|
| string     | Unique metric id           |
| variant    | Sensor's reading value     |
| string     | Sensor's reading timestamp |

The ScanPeriod property has single value for the whole metric report. 
The Delete method results in deleting the whole metric report. 

Monitoring service supports ring buffers to store metrics contained in the
metric report. The ```MaxMetricRecords``` property of the 
```xyz.openbmc_project.MonitoringService.ReportsManagement``` interface 
contains the ring buffer length.

**Telemetry Service Redfish API**

Telemetry service shall support 2019.1 Redfish schemas for telemetry resources. 
Metric report definitions determines which metrics are to be included in metric 
report. Metric definition is assigned to particular metric type and it
describes how the metric should be interpreted. The following resource schemas 
shall be supported:

- TelemetryService 1.1.0
- MetricDefinition 1.0.1
- MetricReportDefinition 1.0.1
- MetricReport 1.0.1

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

The diagram shows the relations between Redfish resources. Metric report is
defined to be generated edriodically, on demand or on change. Each metric in the 
Metric Report contains the URI to its metric definition and Redfish sensor, 
which reading value is presented. Nevertheless, under this presentation layer, 
Monitoring Service is gathering D-Bus sensors readings and exposing them
in reading reports over D-Bus for the Telemetry Service. Each D-Bus sensor
is mapped to Redfish sensor. 

Below examples of Redfish resources for the Telemetry Service are shown. 

The telemetry service Redfish resource example:

```json
{
    "@odata.type": "#TelemetryService.v1_1_0.TelemetryService",
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
    "@odata.type": "#MetricReportDefinition.v1_0_1.MetricReportDefinition",
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
    "@odata.type": "#MetricReport.v1_0_1.MetricReport",
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

**Performance tests**

Performance test were conducted on the AST2500 system with 64 MB flash and 
512 MB RAM. Flash consumption by the Monitoring Service is 197.5 kB. The
runtime statistics are shown in the table below. The reading report is 
mapped into single Metric Report. The runtime data is collected for the
Monitoring Service component only. All reports was created with 
```xyz.openbmc_project.MonitoringService.Metric.OnChange``` property to 
maximize the workload. In the configuration with 50 reports and 50 sensors
it is about 200 new readings per second, generating 200 reading reports
per second. The table shows CPU usage and memory usage. The VSZ is the amount
of memory mapped into the address space of the process. It includes pages 
backed by the process' executable file and shared libraries, its heap and stack,
as well as anything else it has mapped.


| Monitoring service state                         | VSZ  | %VSZ | %CPU |
|--------------------------------------------------|------|------|------|
| Idle (0 reports, 0 sensors)                      |5188 B| 1%   | 0%   |
| 1 report, 1 sensor                               |5188 B| 1%   | 1%   |
| 2 reports, 1 sensor                              |5188 B| 1%   | 1%   |
| 2 reports, 2 sensors (1 sensor per report)       |5188 B| 1%   | 1%   |
| 1 report, 10 sensors                             |5188 B| 1%   | 1%   |
| 10 reports, 10 sensors (same for each report)    |5320 B| 1%   | 1-2% |
| 2 reports, 20 sensors (10 per report)            |5188 B| 1%   | 1%   |
| 30 reports, 30 sensors (10 per report)           |5444 B| 1%   | 5-9% |
| 50 reports, 50 sensors (10 per report)           |5572 B| 1%   |11-14%|

The last two configurations use 10 sensors per reading report, which gives
3 or 5 distinctive configurations. Each such configuration is used to 
create 10 reading reports to obtain the desired amount of 30 or 50 reading
reports. 

In this architecture reading report is created every time when Redfish
Metric Report Definition is posted (creating new Metric Report). 

## Alternatives Considered
The [framework based on collectd/librrd][5] was considered as alternate design.
Although it seems to be versatile and scalable solution, it has some drawbacks
from our point of view:
* Collectd's footprint in the minimal working configuration is around 2.6 MB, 
while available space for the OpenBMC is limited to 64 MB.
* In this design, librrd is used to store metrics on the BMC's non-volatile
storage, which may be an issue, when lots of metrics are captured and stored
to OpenBMC's limited storage space. Also flash wear-out issue may occur, when
metrics are captured frequently (like once per second).
* Monitoring Service is directly compatible with Redfish Telemetry 
Service API, which means, that Monitoring Service's reading reports can
be directly mapped to Redfish Metric Reports. 
* Monitoring Service unifies the way how the BMC's telemetry is exposed over
the Redfisn and may be used with multiple front-ends, thus there is no problem
 to add support telemetry over IPMI or any other API. 

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
[5]: https://gerrit.openbmc-project.xyz/c/openbmc/docs/+/22257
