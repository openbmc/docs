# OpenBMC platform telemetry

Author: Piotr Matuszczak <piotr.matuszczak@intel.com>

Other contributors: Pawel Rapkiewicz <pawel.rapkiewicz@intel.com> <pawelr>,
Kamil Kowalski <kamil.kowalski@intel.com>

Created: 2019-08-07

## Problem Description

The BMC on server platform gathers lots of telemetry data, which has to be
exposed in clean, human readable and standardized format. This document focuses
on telemetry over the Redfish, since it is standard API for platform
manageability.

## Background and References

- OpenBMC platform telemetry shall leverage DMTF's [Redfish Telemetry Model][1]
  for exposing platform telemetry over the network.
- OpenBMC platform telemetry shall leverage the [OpenBMC sensors architecture
  implementation][2].
- OpenBMC platform telemetry shall implement a service, called Telemetry to deal
  with metrics report and trigger management. This service is described later in
  this document.
- Although we use the [hwmon][3] to gather readings from physical sensors, this
  architecture does not depend on it, because the Telemetry service component
  relies on the [OpenBMC D-Bus sensors][2].

## Requirements

- [OpenBMC D-Bus sensors][2] support. This is also design limitation, since the
  Telemetry service requires telemetry sources to be implemented as D-Bus
  sensors.

## Proposed Design

Redfish Telemetry Model shall implement Telemetry Service with the following
collection resources:

- Metric Definitions - contains the metadata for metrics (unit, accuracy, etc.)
- Metric Report Definitions - defines how metric report shall be created (which
  metrics it shall contain, how often it shall be generated etc.)
- Metric Reports - contains actual metric reports containing telemetry data
  generated according to the Metric Report Definitions
- Metric Triggers - contains thresholds and actions that apply to specific
  metrics

OpenBMC telemetry architecture is shown on the diagram below.

```ascii
   +--------------+               +----------------+     +-----------------+
   |hwmon|        |               |Dbus sensors|   |     |Telemetry|       |
   +-----/        |               +------------/   |     +---------/       |
   |              +--filesystem--->                |     |                 |
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
URIs for metric report creation. Those sensors are also used to get URI->D-Bus
sensor mapping. Redfish Telemetry Service acts as presentation layer for the
telemetry, while Telemetry service is responsible for gathering metrics from
D-Bus sensors and exposing them as D-Bus objects. Telemetry service supports
different monitoring modes (periodic, on change and on demand) along with
aggregated operations:

- SINGLE - current reading value
- AVERAGE - average value over defined time period
- MAX - max reading value during defined time period
- MIN - min reading value during defined time period
- SUM - sum of reading values over defined time period

The time period for calculating aggregated metric is taken from the Redfish
Metric Report Definition resource for each sensor's metric.

Telemetry service supports creating and managing metric report, which may
contain single or multiple metrics from sensors. This metric report is mapped to
Metric Report for the Redfish Telemetry Service.

The diagram below shows the flows for creation and update of metric report.

```ascii
+----+              +------+              +---------+                 +-------+
|User|              |bmcweb|              |Telemetry|                 | D-Bus |
+-+--+              +--+---+              +----+----+                 |Sensors|
  |                    |                       |                      +---+---+
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
| |                    |  D-Bus object         |                          |   |
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

The Redfish implementation in bmcweb is stateless, thus it is not able to store
metric reports. All operations on metric reports shall be done in the Telemetry
service. Sending metric report as SSE or push-style events shall be done via the
[Redfish Event Service][6]. It is marked as optional because metric report does
not have to be configured for pushing its data through the event.

In case of on demand metric report update, Telemetry service performs no
additional sensor readings because it already has the latest values, since they
are updated on PropertiesChanged signal from the D-Bus sensors.

**Telemetry service on [D-Bus][4]**

Telemetry service exposes specific interfaces on D-Bus. One of them will be used
for reading report management. The second one will be used for triggers
management.

**Reading report management**

The reading report management D-Bus object:

```ascii
xyz.openbmc_project.Telemetry.ReportManager
/xyz/openbmc_project/Telemetry/Reports
```

The `ReportManager` implements D-Bus interface
[`xyz.openbmc_project.Telemetry.ReportManager`][8] for report management. The
interface is described in the phosphor-dbus-interfaces. This interface
implements `AddReport` method, which is used to create a metric report. The
report may contain a single or multiple sensor readings. The way how the report
will be stored by the BMC is defined by one of this method's parameters. The
`ReportManager` object implements property that stores the maximum number of
reports supported simultaneously.

The `AddReport` method returns the path to the newly created report object. The
report object implements the [`xyz.openbmc_project.Object.Delete`][10] and
[`xyz.openbmc_project.Telemetry.Report`][9] interfaces. The [`Delete`][10]
interface is defined to add support for removing Report object, while the
[`Report`][9] interface implements methods and properties for Report management
along with properties containing telemetry readings. Each report object contains
the timestamp of its last update. The report object contains an array of
structures containing reading with its metadata and timestamp of last update of
this metric. Each report has also the property that stores update interval (for
periodically updated reports).

**Trigger management**

The trigger management D-Bus object:

```ascii
xyz.openbmc_project.Telemetry.TriggerManager
/xyz/openbmc_project/Telemetry/Triggers
```

The `TriggerManager` supports the `xyz.openbmc_project.Telemetry.TriggerManager`
interface, which implements the `AddTrigger` method. This method shall be used
to create new trigger for the certain metric. The method's parameters allow to
define the type of metric for which trigger is set (discrete or numeric). Depend
on this setting, this method accepts different set of trigger parameters.

For discrete metric type, trigger parameters contain:

| Field            | Type                | Description                                                                                                                                                                                                     |
| ---------------- | ------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| TriggerCondition | enum                | Discrete trigger condition: <br> "Changed" - trigger occurs when value of metric has changed; <br> "Specified" - trigger occurs when value of metric becomes one of the values listed in the discrete triggers. |
| DiscreteTriggers | array of structures | Array of discrete trigger structures.                                                                                                                                                                           |

Member of DiscreteTriggers array:

| Field     | Type    | Description                                                                                                      |
| --------- | ------- | ---------------------------------------------------------------------------------------------------------------- |
| TriggerId | string  | Unique trigger Id                                                                                                |
| Severity  | enum    | Severity: <br> "OK" - normal<br> "Warning" - requires attention<br> "Critical" - requires immediate attention    |
| Value     | variant | Value of discrete metric, that constitutes a trigger event.                                                      |
| DwellTime | uint64  | Time in milliseconds that a trigger occurrence persists before the action defined for this trigger is performed. |

For numeric metric type, trigger parameters contain numeric thresholds. Numeric
thresholds structure shall contain up to 4 thresholds: upper warning, upper
critical, lower warning and lower critical. Thus it will contain up to 4
structures shown below:

| Field          | Type    | Description                                                                                                                                                                                                                                                                                                                                                                                                         |
| -------------- | ------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| ThresholdType  | enum    | Numeric trigger type: <br> "UpperCritical" - reading is above normal range and requires immediate attention<br>"UpperWarning" - reading is above normal range and may require attention<br>"LowerCritical" - reading is below normal range and requires immediate attention<br>"LowerWarning" - reading is below normal range and may require attention                                                             |
| DwellTime      | uint64  | Time in milliseconds that a trigger occurrence persists before the action defined for this trigger is performed.                                                                                                                                                                                                                                                                                                    |
| Activation     | enum    | Indicates direction of crossing the threshold value that trigger the threshold's action:<br> "Increasing" - trigger action when reading is changing from below to above the threshold's value<br> "Decreasing" - trigger action when reading is changing from above to below the threshold's value<br> "Either" - trigger action when reading is crossing the threshold's value in either direction described above |
| ThresholdValue | variant | Value of reading that will trigger the threshold                                                                                                                                                                                                                                                                                                                                                                    |

The `AddTrigger` method also allows to define the specific action when trigger
is activated. Upon the trigger activation, three possible actions are allowed,
logging event to log service, sending event via event service and triggering the
metric report update.

In order to assign trigger to specific metric, the metric parameter is defined.
Its structure contains the following data:

| Field      | Type        | Description                                                                                                                                                                                        |
| ---------- | ----------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| SensorPath | object path | D-Bus path to sensor, for which trigger is defined.                                                                                                                                                |
| MetricId   | string      | Contains unique metric id, that can be mapped to Redfish MetricId.                                                                                                                                 |
| ReportPath | object path | D-Bus path to Telemetry's reading report which update shall be triggered when trigger condition occurs. This is optional and shall be filled when trigger's action is set to update metric report. |

The `AddTrigger` method also allows to set trigger's persistency (whether
trigger shall be stored in the BMC's non-volatile memory).

The `AddTrigger` method returns:

```ascii
String for created trigger - ie. '/xyz/openbmc_project/Telemetry/Triggers/{Domain}/{Name}'
```

Such created trigger implements the `xyz.openbmc_project.Object.Delete` and the
`xyz.openbmc_project.Telemetry.Trigger` interfaces. Each trigger object contains
read-only information about metric type, for which it was created (discrete or
numeric). This information determines which triggers are stored within trigger
object.

If trigger is defined for discrete metric type, than it contains trigger
information that looks like this:

| Type                | Description                                                                                                                                                                                                   |
| ------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| enum                | Discrete trigger condition: <br> "Changed" - trigger occurs when value of metric has changed;<br>"Specified" - trigger occurs when value of metric becomes one of the values listed in the discrete triggers. |
| array of structures | Array of discrete trigger structures.                                                                                                                                                                         |

Discrete trigger structure:

| Type    | Description                                                                                                         |
| ------- | ------------------------------------------------------------------------------------------------------------------- |
| string  | Unique trigger Id                                                                                                   |
| enum    | Severity: <br> "OK" - normal<br>"Warning" - requires attention<br> "Critical" - requires immediate attention        |
| variant | Value of discrete metric, that constitutes a trigger event.                                                         |
| uint64  | Time in milliseconds that a trigger occurrence persists before the action defined in the `ActionType` is performed. |

If trigger is defined for numeric metric type, than it contains information
about numeric triggers that is an array of 4 structures presented below:

| Type    | Description                                                                                                                           |
| ------- | ------------------------------------------------------------------------------------------------------------------------------------- |
| enum    | Numeric trigger type: <br> "UpperWarning"<br>"UpperWarning"<br>"LowerCritical"<br>"LowerWarning"                                      |
| uint64  | Time in milliseconds that a trigger occurrence persists before the action defined in the `ActionType` is performed.                   |
| enum    | Indicates direction of crossing the threshold value that trigger the threshold's action:<br> "Increasing"<br>"Decreasing"<br>"Either" |
| variant | Value of reading that will trigger the threshold                                                                                      |

The trigger object also contains information about reading, for which trigger
was defined. It is in a form of structure consisting of three fields.

| Field type  | Description                                                                                                                                                                              |
| ----------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| object path | D-Bus path of sensor. This is a path to the sensor, for which reading trigger is defined.                                                                                                |
| string      | Unique metric Id                                                                                                                                                                         |
| object path | D-Bus path of existing reading report. This is required when trigger's action is to update metric report. This path shall point to existing reading report within the Telemetry service. |

**Trigger operations**

Triggers support three types of operation: Log, Event and Update. For each,
there is a different way of proceeding.

1. For action Log, the event shall be logged to the system journal. In this case
   the Telemetry service writes data to system journal using libjournal. The
   Redfish log service shall then retrieve the data by reading system journal.
   All is shown on the diagram below.

```ascii
+---------------------------+
|bmcweb|                    |         +----------------------+
+------/    +-----------+-+ |         |Telemetry|            |
|           |Redfish    | | |         +---------/            |
|           |log service| | |         |                      |
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

2. For action Event, the Telemetry service shall send event using the [Redfish
   Event Service][6] either as push-style event or SSE.

3. For action Update, the Telemetry service will trigger the update of reading
   report pointed by it's D-Bus path contained in trigger object properties. The
   update shall cause the reading report's D-Bus object to emit property change
   signal. This will cause Redfish Metric Report to be streamed out if it was
   configured to do so.

**Redfish Telemetry Service API**

Redfish Telemetry Service shall support 2019.1 Redfish schemas for telemetry
resources. Metric report definitions determines which metrics are to be include
in metric report. Metric definition is assigned to particular metric type and it
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
Telemetry is gathering D-Bus sensors readings and exposing them in reading
reports over D-Bus for the Telemetry Service. Each D-Bus sensor is mapped to
Redfish sensor.

Below examples of Redfish resources for the Telemetry Service are shown.

The Telemetry Service Redfish resource example:

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
  "ReportActions": ["LogToMetricReportsCollection"],
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
  "TriggerActions": ["RedfishMetricReport"],
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

Performance test were conducted on the AST2500 system with 64 MB flash and 512
MB RAM. Flash consumption by the Telemetry service is 197.5 kB. The runtime
statistics are shown in the table below. The reading report is mapped into
single Metric Report. The runtime data is collected for the Telemetry component
only. All reports was created with
`xyz.openbmc_project.Telemetry.Metric.OnChange` property to maximize the
workload. In the configuration with 50 reports and 50 sensors it is about 200
new readings per second, generating 200 reading reports per second. The table
shows CPU usage and memory usage. The VSZ is the amount of memory mapped into
the address space of the process. It includes pages backed by the process'
executable file and shared libraries, its heap and stack, as well as anything
else it has mapped.

| Telemetry service state                       | VSZ    | %VSZ | %CPU   |
| --------------------------------------------- | ------ | ---- | ------ |
| Idle (0 reports, 0 sensors)                   | 5188 B | 1%   | 0%     |
| 1 report, 1 sensor                            | 5188 B | 1%   | 1%     |
| 2 reports, 1 sensor                           | 5188 B | 1%   | 1%     |
| 2 reports, 2 sensors (1 sensor per report)    | 5188 B | 1%   | 1%     |
| 1 report, 10 sensors                          | 5188 B | 1%   | 1%     |
| 10 reports, 10 sensors (same for each report) | 5320 B | 1%   | 1-2%   |
| 2 reports, 20 sensors (10 per report)         | 5188 B | 1%   | 1%     |
| 30 reports, 30 sensors (10 per report)        | 5444 B | 1%   | 5-9%   |
| 50 reports, 50 sensors (10 per report)        | 5572 B | 1%   | 11-14% |

The last two configurations use 10 sensors per reading report, which gives 3 or
5 distinctive configurations. Each such configuration is used to create 10
reading reports to obtain the desired amount of 30 or 50 reading reports.

In this architecture reading report is created every time when Redfish Metric
Report Definition is posted (creating new Metric Report).

## Alternatives Considered

The [framework based on collectd/librrd][5] was considered as alternate design.
Although it seems to be versatile and scalable solution, it has some drawbacks
from our point of view:

- Collectd's footprint in the minimal working configuration is around 2.6 MB,
  while available space for the OpenBMC is limited to 64 MB.
- In this design, librrd is used to store metrics on the BMC's non-volatile
  storage, which may be an issue, when lots of metrics are captured and stored
  to OpenBMC's limited storage space. Also flash wear-out issue may occur, when
  metrics are captured frequently (like once per second).
- Telemetry service is directly compatible with Redfish Telemetry Service API,
  which means, that Telemetry's reading reports can be directly mapped to
  Redfish Metric Reports.
- Telemetry service unifies the way how the BMC's telemetry is exposed over the
  Redfish and may be used with multiple front-ends, thus there is no problem to
  add support telemetry over IPMI or any other API.

Since this design assumes flexibility and modularity, there is no obstacles to
use collectd in cooperation with Telemetry. The one of possible configurations
is shown on the diagram below.

```ascii
   +-----------------+      +-----------------+
   |  D-Bus sensors  |      |   Telemetry     |
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

Here collectd is used as the source of some set of metrics. It exposes them as
the D-Bus sensors, which can easily be consumed either by the bmcweb and
Telemetry service without any changes in their D-Bus interfaces. In such
configuration Telemetry service provides metric reports and triggers management.

Other possible configuration is to use collectd without the Telemetry service,
but in such case, collectd does not provide metric reports and triggers support
compatible with the Redfish. In such case, Redfish Telemetry Service won't be
supported or metric reports and triggers support has to be provided by the
collectd.

## Impacts

This design impacts the architecture of the bmcweb component, since it adds the
Redfish Telemetry Service implementation as a component for the existing Redfish
API implementation.

## Testing

This is the very high-level description of the proposed set of tests. Testing
shall be done on three basic levels:

- Unit tests
- Functional tests
- Performance tests

**Unit tests**

The Telemetry's code shall be covered by the unit tests. The preferred framework
is the [GTest/GMock][7]. The unit tests shall be ran before code change is to be
committed to make sure, that nothing is broken in existing functionality. Also,
when new code is introduced, a new set of unit tests shall be committed with it
according to test-driven development principle. Unit tests shall be also
carefully reviewed.

**Functional tests**

Functional tests will be divided into two steps.

First step is for testing the Telemetry metric reports management. Test scenario
shall contain creating metric report by POSTing proper metric report definition,
reading metric report (using GET on proper URI) and deleting the metric report.
The required configuration for such test is D-Bus sensors (at least some of
them) and bmcweb with Redfish Telemetry Service implemented. The tests shall be
performed on real hardware. For ease of metric testing, dummy D-Bus sensors may
be provided to provide specifically prepared metrics. This configuration shall
also enable testing aggregated operations (MIN, MAX, SUM, AVG).

Second step is to test triggers and events generation. This will require also
Event Service to be implemented along with Log Service. Tests shall cover all
scenarios with sending metric report as an event, triggering metric report
update and logging events.

**Performance tests**

Performance tests shall be done using full OpenBMC configuration with all the
required set of features. The tests shall create a lot of metric reports (up to
maximum number) along with all possible triggers. Measurements shall cover the
periodic metric report jitter, delays in event logging or sending, BMC's CPU
utilization and the performance impact on other services.

[1]:
  https://www.dmtf.org/sites/default/files/standards/documents/DSP2051_0.1.0a.zip
[2]:
  https://github.com/openbmc/docs/blob/master/architecture/sensor-architecture.md
[3]: https://www.kernel.org/doc/Documentation/hwmon/
[4]: https://www.freedesktop.org/wiki/Software/dbus/
[5]: https://gerrit.openbmc.org/c/openbmc/docs/+/22257
[6]: https://gerrit.openbmc.org/c/openbmc/docs/+/24749
[7]: https://github.com/google/googletest
[8]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Telemetry/ReportManager.interface.yaml
[9]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Telemetry/Report.interface.yaml
[10]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Object/Delete.interface.yaml
