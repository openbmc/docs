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
* OpenBMC platform telemetry shall leverage DMTF's [Redfish Telemetry Model][1]
for exposing platform telemetry over the network.
* OpenBMC platform telemetry shall leverage the
[OpenBMC sensors architecture implementation][2].
* OpenBMC platform telemetry shall implement a service, called Monitoring
Service to deal with metrics report and trigger management. This service
is described later in this document.
* Although we use the [hwmon][3] to gather readings from physical sensors, this
architecture does not depend on it, because the Monitoring Service component
relies on the [OpenBMC D-Bus sensors][2].


## Requirements
* [OpenBMC D-Bus sensors][2] support. This is also design limitation, since
the Monitoring Service requires telemetry sources to be implemented as
D-Bus sensors.


## Proposed Design
Redfish Telemetry Model shall implement Telemetry Service with the following
collection resources:
* Metric Definitions - contains the metadata for metrics (unit, accuracy, etc.)
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
<------------------------------------------v-----^--DBus----------v----------->
                                                 |
                                                 |
+-------+---------------------------------------------------------------------+
|bmcweb |                                        |                            |
+-------/                                        |                            |
|                                                |                            |
| +--------+-------------------------------------v--------------------------+ |
| |Redfish |                                                                | |
| +--------/                                            +---------+-------+ | |
| |                                                     |Existing |       | | |
| | +------------------------------------------------+  |Redfish  |       | | |
| | |Telemetry Service|                              |  |resources|       | | |
| | +----------------+/                              |  +---------/       | | |
| | |  +----------+  +-----------+  +-------------+  |  |   +---------+   | | |
| | |  |  Metric  |  |  Metric   |  |Metric report|  |  |   | Redfish |   | | |
| | |  | triggers |  |definitions|  |definitions  <---------+ sensors |   | | |
| | |  |          |  |           |  |             |  |  |   |         |   | | |
| | |  +----+-----+  +-----+-----+  +------+------+  |  |   +---------+   | | |
| | |       |              |               |         |  |                 | | |
| | |       |              |               |         |  |                 | | |
| | |       |              |               |         |  |                 | | |
| | |       |        +-----v-----+         |         |  |                 | | |
| | |       |        |   Metric  |         |         |  |                 | | |
| | |       +-------->   report  <---------+         |  |                 | | |
| | |                |           |                   |  |                 | | |
| | |                +-----------+                   |  |                 | | |
| | |                                                |  |                 | | |
| | +------------------------------------------------+  +-----------------+ | |
| |                                                                         | |
| +-------------------------------------------------------------------------+ |
|                                                                             |
+-----------------------------------------------------------------------------+
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

Monitoring Service supports creating and managing metric report, which may
contain single or multiple metrics from sensors. This metric report is mapped
to Metric Report for the Redfish Telemetry Service.

The diagram below shows the flows for creation and update of metric report.

```ascii
+----+              +------+              +----------+                +-------+
|User|              |bmcweb|              |Monitoring|                | D-Bus |
+-+--+              +--+---+              | Service  |                |Sensors|
  |                    |                  +----------+                +---+---+
  |                    |                       |                          |
+-----------------------------------------------------------------------------+
|Metric report definition flow|                |                          |   |
+-----------------------------+                |                          |   |
| |                    |                       |                          |   |
| |                    |                       |                          |   |
| |    POST request    |                       |                          |   |
| |    with metric     |                       |                          |   |
| |    report          |                       |                          |   |
| |    definition      |                       |                          |   |
| +-------------------->  Invoke AddReport     |  Register for D-Bus      |   |
| |                    |  method on D-Bus      |  sensors                 |   |
| |                    +----------------------->  PropertiesChanged       |   |
| |                    |                       |  signals                 |   |
| |                    |                       +-------------------------->   |
| |                    |                       |-------------------------->   |
| |                    |                       +-------------------------->   |
| |                    |                       |                          |   |
| |  HTTP response     |                       +-+Create Report           |   |
| |  code 201 with     |  Return created       | |D-Bus object            |   |
| |  Metric Report     |  Report D-Bus path    <-+                        |   |
| |  Definition's URI  <-----------------------+                          |   |
| <--------------------+                       |                          |   |
| |                    |                       |                          |   |
| |                    |                       |                          |   |
+-----------------------------------------------------------------------------+
  |                    |                       |                          |
+-----------------------------------------------------------------------------+
|Periodic metric report update flow|           |                          |   |
+----------------------------------+           +-+Metric report           |   |
| |                    |                       | |timer triggers          |   |
| |                    |                       <-+report update           |   |
| |                    |                       |                          |   |
+----------------------------------Optional-----------------------------------+
| |                    |                       |                          |   |
| |  Send report as SSE or push-style event    |                          |   |
| |  using Redfish Event Service (not shown    |                          |   |
| |  here) if configured to do so.             |                          |   |
| <--------------------------------------------+                          |   |
| |                    |                       |                          |   |
+-----------------------------------------------------------------------------+
| |  GET on Metric     |                       |                          |   |
| |  Report URI        |                       |   Sensor's Properties-   |   |
| +-------------------->                       |   Changed signal         |   |
| |                    +-+Map report's URI     <--------------------------+   |
| |                    | |to D-Bus path        |                          |   |
| |                    <-+                     | +----------------------+ |   |
| |                    | Invoke GetAll method  | |Note that sensor's    | |   |
| |                    | on report D-Bus       | |PropertiesChanged     | |   |
| |                    | object                | |signal is asynchronous| |   |
| |                    +-----------------------> |to metric report timer| |   |
| |                    |                       | |This timer is the only| |   |
| |  Return metric     | Return report data    | |thing that triggers   | |   |
| |  report in JSON    <-----------------------+ |metric report update  | |   |
| |  format            |                       | +----------------------+ |   |
| <--------------------+                       |                          |   |
| |                    |                       |                          |   |
+-----------------------------------------------------------------------------+
  |                    |                       |                          |
+-----------------------------------------------------------------------------+
|On change metric report update flow|          |   Sensor's Properties-   |   |
+-----------------------------------+          |   Changed signal         |   |
| |                    |                       <--------------------------+   |
| |                    |                       |                          |   |
| |                    |                       +-+Sensor's signal         |   |
| |                    |                       | |triggers report         |   |
| |                    |                       <-+update                  |   |
| |                    |                       |                          |   |
+----------------------------------Optional-----------------------------------+
| |                    |                       |                          |   |
| |  Send report as SSE or push-style event    |                          |   |
| |  using Redfish Event Service (not shown    |                          |   |
| |  here) if configured to do so.             |                          |   |
| <--------------------------------------------+                          |   |
| |                    |                       |                          |   |
+-----------------------------------------------------------------------------+
| |  GET on Metric     |                       |                          |   |
| |  Report URI        |                       |                          |   |
| +-------------------->                       |                          |   |
| |                    +-+Map report's URI     |                          |   |
| |                    | |to D-Bus path        | +----------------------+ |   |
| |                    <-+                     | |Note that sensor's    | |   |
| |                    | Invoke GetAll method  | |PropertiesChanged     | |   |
| |                    | on report D-Bus       | |signal triggers the   | |   |
| |                    | object                | |report update. It is  | |   |
| |                    +-----------------------> |sufficient that the   | |   |
| |                    |                       | |signal from only one  | |   |
| |  Return metric     | Return report data    | |sensor triggers report| |   |
| |  report in JSON    <-----------------------+ |update.               | |   |
| |  format            |                       | +----------------------+ |   |
| <--------------------+                       |                          |   |
| |                    |                       |                          |   |
+-----------------------------------------------------------------------------+
  |                    |                       |                          |
+-+--------------------+------------------------------------------------------+
|On demand metric report update flow|          |                          |   |
+-+--------------------+------------+          |                          |   |
| |                    |                       |                          |   |
| |  GET on Metric     |                       |                          |   |
| |  Report URI        |                       |                          |   |
| +-------------------->                       |                          |   |
| |                    +-+Map report's URI     |                          |   |
| |                    | |to D-Bus path        |                          |   |
| |                    <-+                     |                          |   |
| |                    |                       |                          |   |
| |                    |  Invoke the Update    |                          |   |
| |                    |  method for report    |                          |   |
| |                    |  D+Bus object         |                          |   |
| |                    +----------------------->                          |   |
| |                    |                       +-+Update method triggers  |   |
| |                    |                       | |report to be updated    |   |
| |                    |                       | |with the latest known   |   |
| |                    |                       | |sensor's readings.      |   |
| |                    |                       | |No additional sensor    |   |
| |                    |                       <-+readings are performed. |   |
+----------------------------------Optional-----------------------------------+
| |                    |                       |                          |   |
| |  Send report as SSE or push-style event    |                          |   |
| |  using Redfish Event Service (not shown    |                          |   |
| |  here) if configured to do so.             |                          |   |
| <--------------------------------------------+                          |   |
| |                    |                       |                          |   |
+-----------------------------------------------------------------------------+
| |                    | Update method call    |                          |   |
| |                    | result                |                          |   |
| |                    <-----------------------+                          |   |
| |                    |                       |                          |   |
| |                    | Invoke GetAll method  |                          |   |
| |                    | on report D-Bus       |                          |   |
| |                    | object                |                          |   |
| |                    +----------------------->                          |   |
| |                    |                       |                          |   |
| |  Return metric     | Return report data    |                          |   |
| |  report in JSON    <-----------------------+                          |   |
| |  format            |                       |                          |   |
| <--------------------+                       |                          |   |
| |                    |                       |                          |   |
+-----------------------------------------------------------------------------+
  |                    |                       |                          |
```

The Redfish implementation in bmcweb is stateless, thus it is not able to
store metric reports. All operations on metric reports shall be done in
the Monitoring Service. Sending metric report as SSE or push-style events
shall be done via the [Redfish Event Service][6]. It is marked as optional
because metric report does not have to be configured for pushing its data
through the event.

In case of on demand metric report update, Monitoring Service performs no
additional sensor readings because it already has the latest values, since
they are updated on PropertiesChanged signal from the D-Bus sensors.

**Monitoring service on [D-Bus][4]**

Monitoring service exposes specific interfaces on D-Bus. One of them will be
used for reading report management. The second one will be used for triggers
management.

**Reading report management**

The reading report management D-Bus object:

```ascii
xyz.openbmc_project.MonitoringService.ReportsManagement
/xyz/openbmc_project/MonitoringService/Reports
```
The ```ReportsManagement``` supports the following interface apart from
standard D-Bus interface.

| Name | Type | Signature | Result/Value | Flags |
|------|------|-----------|--------------|-------|
|```xyz.openbmc_project.MonitoringService.ReportsManagement``` | interface | - | - | - |
|```.AddReport```                          | method    | ssuas | s | - |
|```.MaxReports```                         | property  | u | 50 | emits-change |
|```.PollRateResolution```                 | property  | u | 100 | emits-change |

The ```AddReport``` method is used to create metric report. The report
may contain single or multiple sensor readings. It is stored in the BMC's
volatile memory. The method has the following arguments:

| Argument | Type | Description |
|----------|------|-------------|
| Prefix | string | Defines prefix for report so it will be "<prefix\><randomString\>" i.e.: for prefix "test" -> testqrapndyY |
| ReportingType | string | Reporting type: <br> "xyz.openbmc_project.MonitoringService.Metric.Periodic" - For periodic update "xyz.openbmc_project.MonitoringService.Metric.OnChange" - For update when value changes "xyz.openbmc_project.MonitoringService.Metric.OnRequest" - For update when user requests data |
| ScanPeriod | uint32_t | Scan period used when Periodic type is set (in milliseconds) |
| MetricsParams | array of structures | Collection of metric parameters.  |

The ```MetricParams``` array entry is a structure containing:
| Field | Type | Description |
|----------|------|-------------|
| Sensor's path | object | D-Bus path, path to the sensor providing readings. |
| Operation's type | enum | {SINGLE, MAX, MIN, AVG, SUM} - information about aggregated operation. |
| Metric id | string | Contains unique metric id, that can be mapped to Redfish MetricId. |

The ```ScanPeriod``` is defined per report, thus all sensors listed in the MetricsParams
collection will be scanned wit the same frequency. Also the ReportingType is
defined per report. In case when *xyz.openbmc_project.MonitoringService.Metric.OnChange*
ReportingType was defined, metric report will emit signal when at least one
reading has changed.

The ```AddReport``` method returns:
```ascii
String for created report - ie. '/xyz/openbmc_project/MonitoringService/Reports/testqrapndyY'
```

Such created metric report implements the following interfaces, methods and
properties (apart from standard D-Bus interface):

| Name | Type | Signature | Result/Value | Flags |
|------|------|-----------|--------------|-------|
|```xyz.openbmc_project.Object.Delete```   | interface | - | - | - |
|```.Delete```                             | method    | - | - | - |
|```xyz.openbmc_project.MonitoringService.Report``` | interface | - | - | - |
|```.Update```                             | method    | - | - | - |
|```.ReadingParameters```                  | property  | a(sos) | 1 "/" | emits-change writable |
|```.Readings```                           | property  | a(svs) | 0 | emits-change read-only |
|```.ReportingType```                      | property  | s | One of reporting type strings| emits-change writable |
|```.ScanPeriod```                         | property  | u | 100 | emits-change writable |

The ```Update``` method is defined for the on demand metric report update. It
shall trigger the ```Readings``` property to be updated and send
PropertiesChanged signal.

The ```ReadingParameters``` property contains an array of structures containing
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

The ```ScanPeriod``` property has single value for the whole metric report.
The Delete method results in deleting the whole metric report.

The ```MaxReports``` property of
the ```xyz.openbmc_project.MonitoringService.ReportsManagement``` interface
contains the max number of metric reports supported by the Monitoring Service.
This property is added to be compliant with the Redfish Telemetry Service
schema, that contains ```MaxReports``` property.

**Trigger management**

The trigger management D-Bus object:

```ascii
xyz.openbmc_project.MonitoringService.TriggersManagement
/xyz/openbmc_project/MonitoringService/Triggers
```
The ```TriggersManagement``` supports the following interface apart from
standard D-Bus interface.

| Name | Type | Signature | Result/Value | Flags |
|------|------|-----------|--------------|-------|
|```xyz.openbmc_project.MonitoringService.TriggersManagement``` | interface | - | - | - |
|```.AddTrigger```                         | method    | sssv(os) | s | - |

The ```AddTrigger``` method shall be used to create new trigger for the
certain metric. Triggers are stored in BMC's volatile memory. The method
has the following arguments:

| Argument | Type | Description |
|----------|------|-------------|
| Prefix | string | Defines prefix for report so it will be "<prefix\><randomString\>" i.e.: for prefix "test" -> trigger0dfvAgVt6 |
| ActionType | string | Action type: <br> "xyz.openbmc_project.MonitoringService.Trigger.Log" - For logging to log service "xyz.openbmc_project.MonitoringService.Trigger.Event" - For sending Redfish event "xyz.openbmc_project.MonitoringService.Trigger.Update" - For trigger metric report update |
| MetricType | string | Metric type: <br> "xyz.openbmc_project.MonitoringService.Trigger.Discrete" - for discrete sensors "xyz.openbmc_project.MonitoringService.Trigger.Numeric" - for numeric sensors |
| TriggerParams | variant | Variant containing structure with either discrete triggers or numeric thresholds. |
| MetricParam | structure | Structure containing D-Bus sensor's path and unique metric Id and optional D-Bus path to metric report to trigger. |

The ```TriggerParams``` is variant type, which shall contain structure
depending on the ```MetricType``` value. In case when ```MetricType``` contains
the ```xyz.openbmc_project.MonitoringService.Trigger.Discrete``` value,
 ```TriggerParams``` shall contain structure with discrete triggers.
When ```MetricType``` contains
the ```xyz.openbmc_project.MonitoringService.Trigger.Numeric``` value,
 ```TriggerParams``` shall contain structure with numeric thresholds.

Discrete triggers structure:

| Field | Type | Description |
|-------|------|-------------|
| TriggerCondition | string | Discrete trigger condition: <br> "xyz.openbmc_project.MonitoringService.Trigger.Discrete.Changed" - trigger ocurs when value of metric has changed; <br> "xyz.openbmc_project.MonitoringService.Trigger.Discrete.Specified" - trigger occurs when value of metric becomes one of the values listed in the discrete triggers. |
| DiscreteTriggers | array of structures | Array of discrete trigger structures. |

Member of DiscreteTriggers array:

| Field | Type | Description |
|-------|------|-------------|
| TriggerId| string     | Unique trigger Id |
| Severity | string     | Severity: <br> "xyz.openbmc_project.MonitoringService.Trigger.Discrete.Severity.OK" - normal, "xyz.openbmc_project.MonitoringService.Trigger.Discrete.Severity.Warning" - requires attention, "xyz.openbmc_project.MonitoringService.Trigger.Discrete.Severity.Critical" - requires immediate attention |
| Value | variant    | Value of discrete metric, that constitutes a trigger event. |
| DwellTime | uint64     | Time in milliseconds that a trigger occurrence persists before the action defined in the ```ActionType``` is performed. |

Numeric thresholds structure shall contain up to 4 thresholds: upper warning, upper critical,
lower warning and lower critical. Thus it will contain up to 4 structures shown below:

| Field | Type | Description |
|-------|------|-------------|
| ThresholdType | string | Numeric trigger type: <br> "xyz.openbmc_project.MonitoringService.Trigger.Numeric.UpperCritical","xyz.openbmc_project.MonitoringService.Trigger.Numeric.UpperWarning","xyz.openbmc_project.MonitoringService.Trigger.Numeric.LowerCritical","xyz.openbmc_project.MonitoringService.Trigger.Numeric.LowerWarning"|
| DwellTime | uint64 | Time in milliseconds that a trigger occurrence persists before the action defined in the ```ActionType``` is performed. |
| Activation | string | Indicates direction of crossing the threshold value that trigger the threshold's action: "xyz.openbmc_project.MonitoringService.Trigger.Numeric.Activation.Increasing", "xyz.openbmc_project.MonitoringService.Trigger.Numeric.Activation.Decreasing", "xyz.openbmc_project.MonitoringService.Trigger.Numeric.Activation.Either" |
| ThresholdValue | variant | Value of reading that will trigger the threshold |

The numeric threshold trigger type meaning:

- "xyz.openbmc_project.MonitoringService.Trigger.Numeric.UpperCritical" -
indicates the reading is above normal range and requires immediate attention
- "xyz.openbmc_project.MonitoringService.Trigger.Numeric.UpperWarning" -
indicates the reading is above normal range and may require attention
- "xyz.openbmc_project.MonitoringService.Trigger.Numeric.LowerCritical" -
indicates the reading is below normal range and requires immediate attention
- "xyz.openbmc_project.MonitoringService.Trigger.Numeric.LowerWarning" -
indicates the reading is below normal range and may require attention

The numeric threshold activation meaning:

- "xyz.openbmc_project.MonitoringService.Trigger.Numeric.Activation.Increasing" -
trigger action when reading is changing from below to above the threshold's value
- "xyz.openbmc_project.MonitoringService.Trigger.Numeric.Activation.Decreasing" -
trigger action when reading is changing from above to below the threshold's value
- "xyz.openbmc_project.MonitoringService.Trigger.Numeric.Activation.Either" -
trigger action when reading is crossing the threshold's value in either direction
described above

The ```MetricParam``` structure contains the following data:

| Field | Type | Description |
|-------|------|-------------|
| SensorPath | object path | D-Bus path to sensor, for which trigger is defined. |
| MetricId   | string | Contains unique metric id, that can be mapped to Redfish MetricId. |
| ReportPath | object path | D-Bus path to Monitoring Service's reading report which update shall be triggered when trigger condition occurs. This is optional and shall be filled when trigger's ActionType is set to "xyz.openbmc_project.MonitoringService.Trigger.Update". |

The ```AddTrigger``` method returns:
```ascii
String for created trigger - ie. '/xyz/openbmc_project/MonitoringService/Triggers/trigger0dfvAgVt6'
```
Such created trigger implements the following interfaces, methods and
properties (apart from standard D-Bus interface):

| Name | Type | Signature | Result/Value | Flags |
|------|------|-----------|--------------|-------|
|```xyz.openbmc_project.Object.Delete```   | interface | - | - | - |
|```.Delete```                             | method    | - | - | - |
|```xyz.openbmc_project.MonitoringService.Trigger``` | interface | - | - | - |
|```.MetricType```                         | property | s | One of the MetricType strings | emits-change read-only |
|```.Triggers```                           | property | {sa{ssvu64}} or a{su64sv} | The structure containing triggers. It depends on ```.MetricType``` property how the structure is defined. | emits-change writable |
|```.ActionType```                         | property | s | One of ActionType strings | emits-change writable |
|```.Metric```                             | property | (oso) | Structure containing details of metric, for which trigger is defined. | emits-change writable |

The ```.MetricType``` property contains information about metric type for which
trigger was created. It can be either discrete or numeric. This property is
read-only, thus created trigger cannot be changed from discrete to numeric or
from numeric to discrete. This also determines how the ```.Triggers``` property
looks like on D-Bus.

If ```.MetricType``` is equal to "xyz.openbmc_project.MonitoringService.Trigger.Discrete"
then ```.Triggers``` property contains discrete trigger that looks like this:

| Type | Description |
|------|-------------|
| string | Discrete trigger condition: <br> "xyz.openbmc_project.MonitoringService.Trigger.Discrete.Changed" - trigger ocurs when value of metric has changed; "xyz.openbmc_project.MonitoringService.Trigger.Discrete.Specified" - trigger occurs when value of metric becomes one of the values listed in the discrete triggers. |
| array of structures | Array of discrete trigger structures. |

Member of DiscreteTriggers array:

| Type | Description |
|------|-------------|
| string     | Unique trigger Id |
| string     | Severity: <br> "xyz.openbmc_project.MonitoringService.Trigger.Discrete.Severity.OK" - normal, "xyz.openbmc_project.MonitoringService.Trigger.Discrete.Severity.Warning" - requires attention, "xyz.openbmc_project.MonitoringService.Trigger.Discrete.Severity.Critical" - requires immediate attention |
| variant    | Value of discrete metric, that constitutes a trigger event. |
| uint64     | Time in milliseconds that a trigger occurrence persists before the action defined in the ```ActionType``` is performed. |

If ```.MetricType``` is equal to "xyz.openbmc_project.MonitoringService.Trigger.Numeric"
then ```.Triggers``` property contains numeric trigger that is an array of 4 structures
presented below:

| Type | Description |
|------|-------------|
| string | Numeric trigger type: <br> "xyz.openbmc_project.MonitoringService.Trigger.Numeric.UpperCritical", "xyz.openbmc_project.MonitoringService.Trigger.Numeric.UpperWarning", "xyz.openbmc_project.MonitoringService.Trigger.Numeric.LowerCritical", "xyz.openbmc_project.MonitoringService.Trigger.Numeric.LowerWarning"|
| uint64 | Time in milliseconds that a trigger occurrence persists before the action defined in the ```ActionType``` is performed. |
| string | Indicates direction of crossing the threshold value that trigger the threshold's action: "xyz.openbmc_project.MonitoringService.Trigger.Numeric.Activation.Increasing", "xyz.openbmc_project.MonitoringService.Trigger.Numeric.Activation.Decreasing", "xyz.openbmc_project.MonitoringService.Trigger.Numeric.Activation.Either" |
| variant | Value of reading that will trigger the threshold |

The ```.Metric``` property stores the details about reading, for which trigger was defined.
It is in a form of structure consisting of three fields.

| Field type | Description  |
|------------|--------------|
| object path  | D-Bus path of sensor. This is a path to the sensor, for which reading trigger is defined. |
| string     | Unique metric Id |
| object path | D-Bus path of existing reading report. This is required when trigger's action is to update metric report. This path shall point to existing reading report within the Monitoring Service. |

**Trigger operations**

Triggers support three types of operation: Log, Event and Update. For each,
there is a different way of proceeding.

1. For action Log, the event shall
be logged to the system journal. In this case the Monitoring Service writes
data to system journal using libjournal. The Redfish log service shall then
retrieve the data by reading system journal. All is shown on the diagram below.

```ascii
+---------------------------+
|bmcweb|                    |         +----------------------+
+------/    +-----------+-+ |         |Monitoring|           |
|           |Redfish    | | |         |Service   |           |
|           |log service| | |         +----------/           |
|           +-----------/ | |         |                      |
|           |             | |         |                      |
|           |             | |         |                      |
|           +------^------+ |         +-----------+----------+
+---------------------------+                     |
                   |                              |
                   +----collect----+            event
                     journal entry |      (write to journal)
                                   |              |
       +------------------------------------+     |
       |systemd|                   |        |     |
       +-------/ +----------+  +---+------+ |     |
       |         |journal|  |  |libjournal| |     |
       |         +-------/  <-->          <-------+
       |         |          |  +----------+ |
       |         |          |               |
       |         |          |               |
       |         +----------+               |
       |                                    |
       +------------------------------------+
```
2. For action Event, the Monitoring Service shall send event using the
[Redfish Event Service][6] either as push-style event or SSE.

3. For action Update, the Monitoring Service will trigger the update of reading
report pointed by it's D-Bus path contained in ReportPath property inside
the ```.Metric``` structure. The update shall cause the reading report's D-Bus
object to emit property change signal. This will cause Redfish Metric Report to
be streamed out if it was configured to do so.

**Telemetry Service Redfish API**

Telemetry service shall support 2019.1 Redfish schemas for telemetry resources.
Metric report definitions determines which metrics are to be included in metric
report. Metric definition is assigned to particular metric type and it
describes how the metric should be interpreted. The following resource schemas
shall be supported:

- TelemetryService 1.1.2
- MetricDefinition 1.0.3
- MetricReportDefinition 1.3.0
- MetricReport 1.2.0
- Triggers 1.1.1

The following diagram shows relations between these resources.

```ascii
 +----------------------------------------------------------------------------+
 |                             Service root                                   |
 +----------------------------------+-------------------------------+---------+
                                    |                               |
                                    |                               |
                                    |                               |
 +----------------------------------v-----------------+  +----------v---------+
 |                                                    |  |Chassis             |
 |                Telemetry Service                   |  |                    |
 |                                                    |  |                    |
 |                                                    |  |  +---------------+ |
 +---------+--------------+------------------+--------+  |  |               | |
           |              |                  |           |  |   Chassis 1   | |
           |              |                  |           |  |               | |
           |              |                  |           |  +---------+-----+ |
           |              |                  |           |            |       |
+----------v--+  +--------v----+  +----------v-----+     +--------------------+
|Triggers     |  |Metric       |  |Metric report   |                  |
|             |  |definition   |  |                |                  |
|             |  | +---------+ |  |                | Reads            |
| +---------+ |  | |Reading  | |  | +-----------+  | ReadingVolts  +--v------+
| |         | |  | |Volts    <------+           +------------------>         |
| |Trigger 1| |  | +---------+ |  | |  Metric   |  |               |         |
| |         | |  |             |  | | report 1  |  | Reads         |  Power  |
| |         | |  | +---------+ |  | |           |  | PowerConsumed |         |
| |         | |  | |         | |  | |           |  | Watts         |         |
| +--+---+--+ |  | |Power    <------+           +------------------>         |
|    |   |    |  | |Consumed | |  | +-----^-----+  |               +----^----+
|    |   |    |  | |Watts    | |  |       |        |                    |
|    |   |    |  | +---------+ |  |       |        |                    |
|    |   |    |  |             |  |       |        |                    |
+-------------+  +-------------+  +----------------+                    |
     |   |                                |                             |
     |   | Triggers report update         |                             |
	 |   | (when applicable)              |                             |
     |   +--------------------------------+                             |
     |                                                                  |
     |   Monitors PowerConsumedWatts to check                           |
	 |   whether trigger value is exceeded                              |
     +------------------------------------------------------------------+
```

The diagram shows the relations between Redfish resources. Metric report is
defined to be generated periodically, on demand or on change. Each metric in the
Metric Report contains the URI to its metric definition and Redfish sensor,
which reading value is presented. Nevertheless, under this presentation layer,
Monitoring Service is gathering D-Bus sensors readings and exposing them
in reading reports over D-Bus for the Telemetry Service. Each D-Bus sensor
is mapped to Redfish sensor.

Below examples of Redfish resources for the Telemetry Service are shown.

The telemetry service Redfish resource example:

```json
{
    "@odata.type": "#TelemetryService.v1_1_2.TelemetryService",
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
    "Triggers": {
        "@odata.id": "/redfish/v1/TelemetryService/Triggers"
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
    "@odata.type": "#MetricReportDefinition.v1_3_0.MetricReportDefinition",
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
    "@odata.type": "#MetricReport.v1_2_0.MetricReport",
    "Id": "SampleMetric",
    "Name": "Sample Metric Report",
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

Sample trigger, that will trigger metric report update:

```json
{
    "@odata.type": "#Triggers.v1_1_1.Triggers",
    "Id": "SampleTrigger",
    "Name": "Sample Trigger",
    "MetricType": "Numeric",
    "Links": {
        "MetricReportDefinitions": [
            "/redfish/v1/TelemetryService/MetricReportDefinitions/SampleMetric"
        ]
    },
    "Status": {
        "State": "Enabled"
    },
    "TriggerActions": [
        "RedfishMetricReport"
    ],
    "NumericThresholds": {
        "UpperCritical": {
            "Reading": 50,
            "Activation": "Increasing",
            "DwellTime": "PT0.001S"
        },
        "UpperWarning": {
            "Reading": 48.1,
            "Activation": "Increasing",
            "DwellTime": "PT0.004S"
        }
    },
    "MetricProperties": [
        "/redfish/v1/Chassis/Tray_1/Power#/0/PowerConsumedWatts"
    ],
    "@odata.context": "/redfish/v1/$metadata#Triggers.Triggers",
    "@odata.id": "/redfish/v1/TelemetryService/Triggers/PlatformPowerCapTriggers"
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
backed by the process' executable file and shared libraries, its heap and
stack, as well as anything else it has mapped.


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
the Redfish and may be used with multiple front-ends, thus there is no problem
 to add support telemetry over IPMI or any other API.

Since this design assumes flexibility and modularity, there is no obstacles
to use collectd in cooperation with Monitoring Service. The one of possible
configurations is shown on the diagram below.

```ascii
   +-----------------+      +-----------------+
   |                 |      |   Monitoring    |
   |  D-Bus sensors  |      |    Service      |
   |                 |      |                 |
   +--------^--------+      +--------^--------+
            |                        |
            |                        |
            |                        |
<--------^--v-----------D-Bus--------v-^---------->
         |                             |
         |                             |
         |                             |
 +-------v------------+     +----------v--------+
 |  collectd metrics  |     |                   |
 |  exposed as D-Bus  |     |     bmcweb        |
 |      sensors       |     |  (with Redfish    |
 +---------^----------+     |    Telemetry      |
           |                |     Service)      |
           |                |                   |
    +------+-------+        +-------------------+
    |              |
    |   collectd   |
    |              |
    +--------------+
```
Here collectd is used as the source of some set of metrics. It exposes them
as the D-Bus sensors, which can easily be consumed either by the bmcweb and
Monitoring Service without any changes in their D-Bus interfaces. In such
configuration Monitoring Service provides metric reports and triggers
management.

Other possible configuration is to use collectd without the Monitoring Service,
but in such case, collectd does not provide metric reports and triggers support
compatible with the Redfish. In such case, Redfish Telemetry Service won't be
supported or metric reports and triggers support has to be provided by the
collectd.

## Impacts
This design impacts the architecture of the bmcweb component, since it adds
the Telemetry Service implementation as a component for the existing
Redfish API implementation.

## Testing
This is the very high-level description of the proposed set of tests.
Testing shall be done on three basic levels:
* Unit tests
* Functional tests
* Performance tests

**Unit tests**

The Monitoring Service's code shall be covered by the unit tests. The preferred
framework is the [GTest/GMock][7]. The unit tests shall be ran before code
change is to be committed to make sure, that nothing is broken in existing
functionality. Also, when new code is introduced, a new set of unit tests shall
be committed with it according to test-driven development principle. Unit tests
shall be also carefully reviewed.

**Functional tests**

Functional tests will be divided into two steps.

First step is for testing the
Monitoring Service metric reports management. Test scenario shall contain
creating metric report by POSTing proper metric report definition, reading
metric report (using GET on proper URI) and deleting the metric report. The
required configuration for such test is D-Bus sensors (at least some of them)
and bmcweb with Redfish Telemetry Service implemented. The tests shall be
performed on real hardware. For ease of metric testing, dummy D-Bus sensors
may be provided to provide specifically prepared metrics. This configuration
shall also enable testing aggregated operations (MIN, MAX, SUM, AVG).

Second step is to test triggers and events generation. This will require also
Event Service to be implemented along with Log Service. Tests shall cover all
scenarios with sending metric report as an event, triggering metric report
update and logging events.

**Performance tests**

Performance tests shall be done using full OpenBMC configuration with all
the required set of features. The tests shall create a lot of metric reports
(up to maximum number) along with all possible triggers. Measurements shall
cover the periodic metric report jitter, delays in event logging or sending,
BMC's CPU utilization and the performance impact on other services.

[1]: https://www.dmtf.org/sites/default/files/standards/documents/DSP2051_0.1.0a.zip
[2]: https://github.com/openbmc/docs/blob/master/architecture/sensor-architecture.md
[3]: https://www.kernel.org/doc/Documentation/hwmon/
[4]: https://www.freedesktop.org/wiki/Software/dbus/
[5]: https://gerrit.openbmc-project.xyz/c/openbmc/docs/+/22257
[6]: https://gerrit.openbmc-project.xyz/c/openbmc/docs/+/24749
[7]: https://github.com/google/googletest
