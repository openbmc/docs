# Redfish Telemetry Interface 

Author: Paul Vancil (pwvancil)  

Primary assignee:     

Other contributors:  Neeraj Ladkani 

Created: 2019-07-22

## Problem Description
OpenBMC is developing a Platform Telemegtry feature which will include two parts:
- A mechanism to collect the desired metrics data, and
- A interface to remotely access the metrics data -- using both a pull and pull model.

This design document addresses the second part:  the interface to access to metric
data after the system has collected it.

Since OpenBMC is moving to [Redfish][RForg] as its standard for out-of-band management,
the proposal is to use the Redfish Telemetry interfaces for remotely accessing Telemetry data.


## Background and References
The DMTF Redfish forum added support for Telemetry as part of the 
***2018.2 Redfish Release Bundle*** that was released in Sept 
2018 -- *see the news release:* [New Redfish Release Adds ... Telemetry][PR].

Note that the Redfish forum had previously released a *work-in-progress*
[Redfish Telemetry Whitepaper][WP] in Jan 2018.  This White Paper has not yet been updated 
to align with all changes in the official release and does have some discrepencies from the release; 
however, it is generally correct in describing the overall Redfish Telemetry model. 

The Redfish Telemetry model supports the following features:
- Metric Reports can be delivered with either a *push* and *pull* model
- Metric Reports can be generated on pre-defined *intervals* or as the result of a 
*trigger* event (eg a sensor crossing a threshold or something else happening)
- The model is *very extensible* and allows OEM-defined Metric Reports that may
contain:
  - Any number of OEM-defined metric properties,
  - OEM-defined triggers that cause the report to be generated,
  - OEM-defined filter algorithms that operate on other properties (a filter being
and algorithm such as *average*, *max*, *min*)
- In fact, Redfish does not actually pre-define any specific Metric Reports or Metric 
properties in the specification or schemas.  It only defines: 
  - The mechanisms used to deliver the Metric Reports
  - The format of metric reports -- a Redfish-like JSON Schema payload
  - Schema that defines how the implementation describes the Metric Report, Metric
Properties, and Triggers that it supports
  - A few common *filtering methods* are pre-defined (average, minimum, and maximum) 
which may be used to define a Metric property from other properties already in the
Redfish resource model (standard properties or OEM)
  - Standard *formats* for describing time intervals and common triggers such as
when a sensor crosses thresholds 
- The model *supports* the case where a client could *create* a Metric Report Definition 
(by POSTing the new *MetricReportDefinition* to the *MetricDefinitions Collection*).
  - In this case any Metric properties and Triggers included in the new Metric Report
would need to already be defined of course in the *Triggers Collection* and *Metric properties
Collection*
  - Because of implementation complexity in implementing client-defined Metric Report definitions, 
initial implementations are expected to only support some set of pre-defined Metric Reports 
created by the platform vendor (based on customer feedback) that clients choose from and subscribe to. 
Each vendor is defining their own.

The Redfish Telemetry model supports three ways to get metric reports:
- *HTTP Push*, 
- *SSE Push*, and 
- *HTTP Pull*

The MetricReportDefinition resource for each metric report supported by the system specifies:
- whether the metric report is enabled
- whether to log the metric for a client to pull, or whether to push the metric to clients 
that have subscribed for this metric report (or have been pre-subscribed by configuration)
- if logging is enabled for the metric report, whether to append the new metric data to the
MetricReport log resource or overwrite the current entry with the new entry
- the max number of entries in the MetricReport log resource and whether to wrap or stop 
logging when full

For the ***HTTP Push*** model:
- The BMC must be configured with the IP Address and credentials of a
remote *metric report catcher service* which is effectively a subscription to
send metrics to that catcher client
- When the Metric Report is generated (based on interval timer or other trigger),
the BMC formats the report and pushes it to a remote catcher service
using HTTPS POST to send the JSON formated Metric Report to the report catcher.
- With this model, the catcher destination configuration is persistent across
BMC resets or the remote client catcher going offline temporarily

For the ***SSE Push*** (Server-Sent Events) model:
- The SSE model is similar to how Redfish supports pushing other events (eg SEL events)
with SSE
- A client must create the SSE stream by sending a GET to a ServerSentEventUri with
various URI query parameters to indicate it is a Metric Report SSE stream vs a normal
Event Message Stream, as well as which events or metric reports the client wants.
- The service then keeps the connection open until the client closes it, and
Pushes the designated Metric Reports out the SSC connection to the client as they
are generated.
- With this model, the client must initiate the Push stream, and the stream ends
when the connection is dropped for any reason including BMC reset or client closing 
the SSE connection

For the ***HTTP Pull*** model:
- The Metric Report Definition that defines the metric report must specify that the
metric reports should be logged in the MetricReports collection.
  - When a new Metric Report is generated the new metric data is added under the
MetricReports collection. 
Either a new resource is added to the collection (named by timestamp and report),
or the data is appended to an array inside a single resource for the report type
and configuration tells the model whether to wrap after a max number of entries.
  - For the 2nd case (a single resource for each report type), 
a client can read the entire array of saved samples with a single HTTP GET
of the MetricReport for a particular metric type.

The current versions of the Schema files used to model the Telemetry serice are listed
below.  The latest Schema versions in CSDL-XML, JSON-Schema, as well as YAML can be accessed 
from the [Redfish Schema Folder URI][RfSchemas] at `https://redfish.dmtf.org/schemas/v1/`:

- ***[TelemetryService][TS]*** -- the base/root Telemetry resource that contains links to the 
MetricReportDefinitions collection,  MetricDefinitions collection, Triggers collection,
and MetricReports collection.
The Redfish ServiceRoot at `/redfish/v1/` has a link to this resource.
- ***[MetricReportDefinitionCollection][MRDC]*** -- collection of all MetricReportDefinitions
that are supported by the service and that a client can subscribe to
- ***[MetricReportDefinition][MRD]*** -- describes a specific MetricReport by identifying the 
Metric Properties included, Triggers that create the report, and whether the report is
logged in the MetricReportCollection for a client to retrieve later
  - An Example MetricReportDefinition ***Response*** is shown below
- ***[MetricDefinitionCollection][MDC]*** -- a collection of all Metric [Property] Definitions.  
- ***[MetricDefinition][MD]*** -- describes a single Metric Property that can be included in
a Metric Report.  Metrics can be existing Redfish properties, filtered properties
(ex average or max power), or can be OEM defined
  - An Example MetricReportDefinition ***Response*** is shown below
- ***[TriggersCollection][TC]*** -- a collection of all Metric Trigger Definitions.  
- ***[Triggers][TRIG]*** -- describes a single Trigger mechanism that causes a Metric Report to be
generat
- ***[MetricReportCollection][MRC]*** -- a collection of MetricReport resources that contain
the actual metric reports available for a client to *Pull* by executing an HTTP GET
on the resource.
- ***[MetricReport][MR]*** -- contians a MetricReport containing timestamp and all metric properties
that are part of the metric report.

- Example `GET MetricReportDefinition Response`: 
  - `GET /redfish/v1/TelemetryService/MetricReportDefinitions/OBmcPowerMetrics`
  - Defines a Metric Report with the following *Metrics*:
    - MinConsumedWatts - the *Minimum* power draw over the interval
    - MaxConsumedWatts - the *Maximum* power draw over the interval
    - AveConsumedPowwe - the *Average* power draw over the interval
  - the report is generated on a 1 hour schedule
  - the report is pushed as well as logged
    - each report is appended to the OBmcPowerMetrics log until 256 entries, when it wraps
  - The report can be disabled by Patching property `"MetricReportDefinitionEnabled": false`

```
{
    "@odata.type": "#MetricReportDefinition.v1_2_1.MetricReportDefinition",
    "@odata.id": "/redfish/v1/TelemetryService/MetricReportDefinitions/OBmcPowerMetrics",
    "Id": "OBmcPowerMetrics",
    "Name": "Push and Log Platform Power Metrics: Min, Max, Average",
    "MetricReportDefinitionType": "Periodic",
    "MetricReportDefinitionEnabled": true,
    "Schedule": {
        "RecurrenceInterval": "PT01:00:00"
    },
    "ReportActions": [
        "RedfishEvent",
        "LogToMetricReportsCollection"
    ],
    "ReportUpdates": "AppendWrapsWhenFull",
    "AppendLimit": 256,
    "MetricReport": {
        "@odata.id": "/redfish/v1/TelemetryService/MetricReports/OBmcPowerMetricsReport"
    },
    "Metrics": [
        {
            "MetricId": "MinConsumedWatts",
        },
        {
            "MetricId": "MaxConsumedWatts",
        },
        {
            "MetricId": "AveConsumedWatts",
        }
    ],
    "Links": {
        "Triggers": []
    },
    "Status": {
        "State": "Enabled"
    }
}


```
- Example `GET MetricDefinition Response` for Metric Property: `MinConsumedWatts`
  - `GET /redfish/v1/TelemetryService/MetricDefinitions/MinConsumedWatts`
  - the metric property: `"MinConsumedWatts"` is created using the `Minimum` filter 
applied to the Redfish Propoerty `PowerConsumedWatts` in Redfish resource 
`/redfish/v1/Chassis/1/Power#PowerControl/0`
    - the filter applies over a 1 hour interval

```
{
    "@odata.type": "#MetricDefinition.v1_0_2.MetricDefinition",
    "@odata.id": "/redfish/v1/TelemetryService/MetricDefinitions/MinConsumedWatts",
    "Id": "MinConsumedWatts",
    "Name": "Minimum Consumed Watts Metric Definition",
    "MetricType": "Numeric",
    "Implementation": "Calculated",
    "PhysicalContext": "PowerSupply",
    "MetricDataType": "Decimal",
    "Units": "W",
    "CalculationAlgorithm": "Minimum",
    "CalculationTimeInterval": "PT01:00:00"
    "Calculable": "NonSummable",
    "IsLinear": true,
    "CalculationParameters": [
        {
            "SourceMetric": "/redfish/v1/Chassis/1/Power#/PowerControl/0/PowerConsumedWatts",
        }
    ]
}


```




## Requirements

The initial implementation of Redfish Telemetry interface support should include:
- Implement the general Redfish Telemetry model APIs
  - include a TelemetryService link in Redfish Service Root GET API
  - GET TelemetryService resource --static response with links to other metric collections
  - GET MetricDefinitions Collection -- return list of links to pre-defined metric properties
  - GET MetricDefinition -- return a specific pre-defined metric property
  - GET MetricReportDefinitions Collection -- return list of links to pre-defined metric report definitions
  - GET MetricReportDefinition -- return a specific pre-defined metric report
  - PATCH MetricReportDefinition -- update some of the settable properties in a specific metric report): 
    - MetricReportDefinitionEnabled (true, false)
    - Schedule (sets the period if the report is a periodic metric report) 
    - other settable properties could be deferred to the second step -- see below
  - GET Triggers Collection -- return list of links to pre-defined metric triggers
  - GET Trigger -- return a specific pre-defined metric trigger
  - GET MetricReports Collection -- return list of metric reports that have been created
  - GET MetricReport -- return a specific report

- Support for HTTP Push and HTTP Pull metric report delivery to clients

- Implementation of two to three well-bounded metric reports that
rely on existing underlying sensors to calculate the metric results
(eg average, max, min power over an interval)
  - PeriodicPowerMetrics -- Average, Min, and Max Power Consumption over interval
    - Average, Min Power Consumption of interval
  - PeriodicBmcHealthMetrics -- Ave, Min, Max for memory usage, cpu usage, etc over interval
  - Other TBD

- integration with telemetry collection backend eg collectd

The second step would include:
- Support for tuning Metric Report Definitions more
  - PATCH MetricReportDefinition -- update the settable properties in a specific metric report): 
    - MetricReportDefinitionEnabled (true, false)
    - MetricReportDefinitionType (`Periodic, OnChange, OnRequest`)
    - Schedule (if Type above is set to `Periodic`
    - MetricProperties (list of any properties that are in the Redfish model to include in the report)
    - MetricReportHeartbeatInterval
    - SuppressRepeatedMetricValue
- Support the *SSE Push* delivery model
- More complex metric reports


## Proposed Design

- **bmcweb** will need to implement the general Telemetry infrastructure listed above 
under *Requirements*
  - Some of the infrastructure is simply adding some static responses in *bmcweb*:
```
  1) in ServiceRoot response (GET /redfish/v1/), add TelemetryService property under *Links*:
    ...
    "TelemetryService": { 
        "@odata.id": "/redfish/v1/TelemetryService"
    },
    ...

  2) GET /redfish/v1/TelemetryService -- returns a static response with links:
    ...
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
    ...
```
  - Some of the infrastructure is GET interfaces to mostly pre-defined MetricDefinitions,
MetricReportDefinitions, and Triggers.  
    - JSON config files can be used to describe these pre-defined resources
      - If there is a better way than using Json config files for this, I am open to it of course.
    - *bmcweb* can then generate the GET responses based on those config files
    - The config files would be very similar to the two example responses (above) for GET on a 
MetricReportDefinition and MetricDefinition (since most of that data is required)
    - Any settable properties in these configurations (eg MetricReportDefinitionEnable) 
may best be modeled as Dbus properties. 
    - These config files (that describe a metric report) would need to be aligned with the 
backend collectors that collect the data for the repoprt
    - Long-term, if we support POSTing ***new*** metric definitions or metric report definitions, 
the result of the post can be to create the JSON config file.  Backend changes (code or
collectd plugin config changes) would need to be uploaded separately but aligned with
the config files
    - The JSON Config file-driven APIs are shown below
```
      1) GET /redfish/v1/TelemetryService/MetricDefinitions -- reads a MetricDefinitions.json
           config file with an Id for each Metric, and generates the properly formated
           Redfish Get Collection response

      2) GET /redfish/v1/TelemetryService/MetricReportDefinitions -- reads a MetricReportDefinitions.json
           config file with an Id for each MetricReport, and generates the properly formated response

      3) GET /redfish/v1/TelemetryService/Triggers -- reads a MetricTriggers.json
           config file with an Id for each Trigger, and generates the properly formated response

      4) GET /redfish/v1/TelemetryService/MetricDefinitions/<id> -- reads MetricDefinitiosn.json
           config file, extracts the information for Id: <id>, and returns a properly formated
           Redfish Get Metric response

      5) GET /redfish/v1/TelemetryService/MetricReportDefinitions/<id> -- reads MetricReportDefinitiosn.json
           config file, extracts the information for Id: <id>, and returns a properly formated
           Redfish Get MetricReport response

      6) GET /redfish/v1/TelemetryService/Triggers/<id> -- reads MetricTriggers.json
           config file, extracts the information for Id: <id>, and returns a properly formated
           Redfish Get Metric Trigger response
```
    - `PATCH to MetricReportDefinitions/<id>` will need to be supported to update a few properties:
      - EnableMetricReport
      - maybe interval if it is configured as an interval report

  - *bmcweb* must also implement Redfish APIs to GET any metric reports that were *logged* 
into the Redfish MetricReports collection.
    - Note that for Redfish, the MetricReportDefinition has properties that says whether 
to ***log*** a new report, or whether to ***push*** the report, or **both**.
    - In addition, if the report is to be logged, there are also several Redfish-specific 
requirements for handling:
      - wrapping and overwriting of reports in the log, and 
      - whether multiple reports of the same MetricReportDefinition (ie same report type) 
are read as a single GET request with an array of timestamped entries, 
or as multiple individual report resources in the MetricReports
collection requiring multiple GETs
    - Since reports could be pushed or logged or both, and since the way the data is 
saved in the log (replace vs wrap, etc) must be Redfish-aligned, 
I would propose that any time the backend generates a new metric report, that it 
should use an internal Redfish *handleRedfishMetricReport* API 
(described in *below major bullet section* ) that would handle the pushing as well as
the logging into a Redfish metricReports log. 
We need to discuss the best API approach -- whether it is built on a Dbus interface
or uses an existing connectd interface.
    - If some handleRedfishMetricReport indeed logs the reports that need to be logged, 
then *bmcweb* will need to access the logged metricReports for
the APIs below that a client can use to *Pull* metric reports:
```
      1) GET /redfish/v1/TelemetryService/MetricReports -- reads the MetricReports 
           database (TBD), identifies all of the existing MetricReports, and 
           generates a properly formatted Redfish Get MetricReports Collection response

      2) GET /redfish/v1/TelemetryService/MetricReports/<id> -- reads the MetricReports 
           database (TBD), extracts the data for the report associated with <id>, 
           and generates a properly formatted Redfish Get MetricReport response
```
- **handleRedfishMetricReport** design
  - As described above, When the telemetry backend generates a new report, 
it would call the *handleRedfishMetricReport* API to handle pushing the report and/or logging 
the report in the proper Redfish way
  - If configured to log, the library will need to log the new report data based on 
configuration details that were specified in the MetricReportDefinition which includes:
      - append the report to the single log for this MetricReportDefinition.
        - with wrap after N entries
        - with stop after N entries
      - over-write the report data in the single log for this MetricReportDefinition with new report
      - create a separate MetricReport resource for each new report generated with the Id being
the MetricReportDefinition + timestamp
  - Details of how the metric log stores the data need to be discussed including:
    - whether they should or could be non-volatile or volatile (stored in RAM)
    - how much space can be allocated, and thus what the log format needs to be
    - whether a separate process will need to catch the calls and handle the processing since
pushing a report could involve retries.  If OpenBMC has implemented *http Push* events
for normal events (not metrics), the much of this has already been solved. 
    - what the interface that the backend uses to send the data to be logged or pushed

- Finally, the Redfish EventService will need minor modifications to support
*Push MetricReport* subscriptions in addition to standard Redfish event subscriptions.


- Other Questions for Discussion:
  - How many independent metric collection ***schedules*** do we want to support
  - Do we want to design for supporting:  
    - client-defined Metric *Property* Definitions that map directly to Redfish properties? 
    - client-defined Metric *Report* Definitions that are composed a a set of any of these 
Metric Properties and Redfish properties?
    - client-defined sensor threshold triggers ?
  - Heavy support for client-defined reports is all much more complex to implement and test, 
and likely to fall short of need without OEM-specific backend code.
  - It may be more pratical to just make it easy to define the JSON config files that 
pre-define metric propertis and reports and upload those along with the corresponding 
backend plugin code.


## Alternatives Considered
The main alternative to using the Redfish Telemetry model to externally access and manage
metric reports is to use one of the existing collectd interfaces such as `Write HTTP` to
push the metrics directly out to the desired client
- in this case, all of the configurationa and enablement would be via collectd native
conf files and plugins
- It appears that both PUTVAL and JSON format can be output.
- see [Write_HTTP Plugin example][WritePlugin] for example output

Data saved on the bmc could be saved in a file and accessed by file transfer protocols.


## Impacts
- Additional Redfish APIs must be added to the bmcweb Redfish service
- A new backend telemetry service must be implemented to generate telemetry
data which is described in another design proposal
- As new MetricDefinitions and MetricReports are added, this will require: 
  - additional backend plugins or code
  - new corresponding Metric property and report definition config files


## Testing
Additional test suites will need to be added to verify the overall Redfish
Telemetry model consistency and that the base Metric Reports are property formated.
We should create a test metric report that can be used to test much of the
underlying infrastructure.

[RForg]: https://redfish.dmtf.org/
[PR]: https://www.dmtf.org/content/new-redfish-release-adds-openapi-30-support-telemetry
[WP]: https://www.dmtf.org/documents/redfish-spmf/redfish-telemetry-white-paper-010a
[RFSpec]: https://www.dmtf.org/sites/default/files/standards/documents/DSP0266_1.7.0.pdf
[RfSchemas]: https://redfish.dmtf.org/schemas/v1/
[TS]: https://redfish.dmtf.org/schemas/v1/TelemetryService_v1.xml
[MRDC]: https://redfish.dmtf.org/schemas/v1/MetricReportDefinitionCollection_v1.xml
[MRD]: https://redfish.dmtf.org/schemas/v1/MetricReportDefinition_v1.xml
[MDC]: https://redfish.dmtf.org/schemas/v1/MetricDefinitionCollection_v1.xml
[MD]: https://redfish.dmtf.org/schemas/v1/MetricDefinition_v1.xml
[TC]: https://redfish.dmtf.org/schemas/v1/TriggersCollection_v1.xml
[TRIG]: https://redfish.dmtf.org/schemas/v1/Triggers_v1.xml
[MRC]: https://redfish.dmtf.org/schemas/v1/MetricReportCollection_v1.xml
[MR]: https://redfish.dmtf.org/schemas/v1/MetricReport_v1.xml
[WritePlugin]: https://collectd.org/wiki/index.php/Plugin:Write_HTTP


