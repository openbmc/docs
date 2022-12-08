# Redfish EventService design

Author: AppaRao Puli

Other contributors: None

Created: 2019-07-24

## Problem description

Redfish in OpenBMC currently supports sending response to the clients which
requests for data. Currently there is no service to send asynchronous message to
the client about any events, error or state updates.

The goal of this design document is to give resource overview of redfish event
service and its implementation details in OpenBMC.

## Background and references

- [Redfish specification](https://www.dmtf.org/sites/default/files/standards/documents/DSP0266_1.7.0.pdf)
- [DMTF Redfish school events](https://www.dmtf.org/sites/default/files/Redfish_School-Events_2018.2.pdf)
- [Redfish event logging](https://github.com/openbmc/docs/blob/master/redfish-logging-in-bmcweb.md)
- [Telemetry service](https://gerrit.openbmc.org/#/c/openbmc/docs/+/24357/)
- [Beast async client](https://www.boost.org/doc/libs/develop/libs/beast/example/http/client/async/)
- [Beast async-ssl client](https://www.boost.org/doc/libs/develop/libs/beast/example/http/client/async-ssl/)
- [EventService schema](https://redfish.dmtf.org/schemas/v1/EventService_v1.xml)
- [EventDestinationCollection schema](https://redfish.dmtf.org/schemas/v1/EventDestinationCollection_v1.xml)
- [EventDestination schema](https://redfish.dmtf.org/schemas/v1/EventDestination_v1.xml)
- [Upgrade connection for SSE](https://gerrit.openbmc.org/#/c/openbmc/bmcweb/+/13948/)

## Requirements

High level requirements:

- BMC shall support send asynchronous event notification to external subscribed
  clients over network using redfish(http/https) protocol, instead of regular
  querying mechanism.
- BMC shall provide mechanism to the users to configure event service with
  specific filters and accordingly send the event notifications to all the
  registered clients.
- BMC shall preserve the event service configuration across reboots, and shall
  reset the same, when factory defaults applied.
- BMC shall have provision to disable the EventService.
- The BMC Redfish interface shall support the EventService, EventDestination and
  EventDestinationCollection schemas.
- BMC can provide functionality to send out test events(SubmitTestEvent).
- Must allow only users having 'ConfigureManager' to create, delete or updated
  event service configuration, subscriptions and restrict other users. Must
  allow users with 'Login' privileges to view the EventService configuration and
  subscription details.
- EventService shall be specific to Redfish Interface alone. i.e No other
  service can add/modify/delete configuration or subscriptions.
- Either the client or the service can terminate the event stream at any time.
  The service may terminate a subscription if the number of delivery error's
  exceeds preconfigured thresholds.
- For Push style event subscription creation, BMC shall respond to a successful
  subscription with HTTP status 201 and set the HTTP Location header to the
  address of a new subscription resource. Subscriptions are persistent and shall
  remain across event service restarts.
- For Push style event subscription creation, BMC shall respond to client with
  http status as
  - 400: If parameter in body is not supported.
  - 404: If request body contains both RegistryPrefixes and MessageIds.
- SSE: BMC shall respond to GET method on URI specific in "ServerSentEventUri"
  in EventService schema with 201(created) along with subscription information.
  BMC shall open a new https connection and should keep connection alive till
  subscription is valid. BMC shall respond with https code 400, if filter
  parameters are not supported or 404 if filter parameter contains invalid data.
  Also SSE subscription data cannot be persistent across service resets/reboots.
- Clients shall terminate a subscription by sending an DELETE message to the URI
  of the subscription resource.
- BMC may terminate a subscription by sending a special "subscription
  terminated" event as the last message. Future requests to the associated
  subscription resource will respond with HTTP status code 404.
- BMC shall accept the "Last-Event-ID" header from the client to allow a client
  to restart the event/metric report stream in case the connection is
  interrupted.
- To avoid exhausting the connection resource, BMC shall implement a restriction
  of maximum 20 connections (Maximum of 10 SSE connections). BMC shall respond
  with 503 Service Unavailable server error response code indicating that the
  server is not ready to handle the request.
- BMC shall log an redfish event message for subscription addition, deletion and
  modification.
- Services shall not send a "push" event payload of size more than 1 MB.

## Proposed design

Redfish event service provides a way to subscribe to specific kinds of Events,
MetricReport and asynchronously send subscribed type of events or reports to
client whenever it occurs.

Two styles of events are supported OpenBMC EventService.

1. Push style events (https): When the service detects the need to send an
   event, it uses an HTTP POST to push the event message to the client. Clients
   can enable reception of events by creating a subscription entry in the
   EventService.

2. Server-Sent Events (SSE): Client opens an SSE connection to the service by
   performing a GET on the URI specified by the "ServerSentEventUri" in the
   Event Service.

This document majorly focuses on OpenBMC implementation point of view. For more
details specific to EventService can be found in
["Redfish Specification"](https://www.dmtf.org/sites/default/files/standards/documents/DSP0266_1.7.0.pdf)
sections:-

- Eventing (Section 12.1)
- Server Sent-Events (Section 12.5)
- Security (Section 12 & 13.2.8)

**BLOCK DIAGRAM**

```
+------------------------------------------------------------------------------------+
|                                 CLIENT                                             |
|                             EVENT LISTENER                                         |
|                                                                                    |
+-----------------------------------------------------------------^---------^--------+
CONFIGURATION|         |SSE(GET)        NETWORK        PUSH STYLE |         |SSE
SUBSCRIPTION |         |                                 EVENTS   |  POST   |
+------------------------------------------------------------------------------------+
|            |         |                                          |         |        |
|   +--------+---------+---+  +---------------------------------------------------+  |
|   |     REDFISH          |  |           EVENT SERVICE MANAGER   |         |     |  |
|   |     SCHEMA           |  |   +-------------------------------+---------+--+  |  |
|   |    RESOURCES         |  |   |          EVENT SERVICE                     |  |  |
|   |                      |  |   |            SENDER                          |  |  |
|   |                      |  |   +------^----------------------------+--------+  |  |
|   |  +-------------+     |  |          |                            |           |  |
|   |  |EVENT SERVICE|     |  |   +------+--------+            +------+--------+  |  |
|   |  +-------------+     |  |   | METRICSREPORT |            |  EVENT LOG    |  |  |
|   |                      |  |   | FORMATTER     |            |  FORMATTER    |  |  |
|   |                      |  |   +------^--------+            +------^--------+  |  |
|   |     +-------------+  |  |          |                            |           |  |
|   |   + |SUBSCRIPTIONS+--------------------+------------------------------+     |  |
|   | + | +-------------+  |  |          |   |                        |     |     |  |
|   | | +--------------+   |  |   +------+---+----+            +------+-----+--+  |  |
|   | +--------------|     |  |   | METRCISREPORT |            |  EVENT LOG    |  |  |
|   |                      |  |   | MONITOR/AGENT |            |  MONITOR      |  |  |
|   |                      |  |   +---------------+            +---------------+  |  |
|   +----+-----------------+  +---------------------------------------------------+  |
|        |                              |                             |              |
+------------------------------------------------------------------------------------+
         |                              |                             |
         |                              |                             |INOTIFY
         |                              |                             |
+--------+----------+         +---------+----------+        +-----------------------+
|                   |         |                    |        |  +-----------------+  |
|    REDFISH        |         |     TELEMETRY      |        |  |     REDFISH     |  |
|  EVENT SERVICE    |         |      SERVICE       |        |  |    EVENT LOGS   |  |
|    CONFIG         |         |                    |        |  +-----------------+  |
| PERSISTENT STORE  |         |                    |        |         |RSYSLOG      |
|                   |         |                    |        |  +-----------------+  |
|                   |         |                    |        |  |     JOURNAL     |  |
|                   |         |                    |        |  |  CONTROL LOGS   |  |
|                   |         |                    |        |  +-----------------+  |
+-------------------+         +--------------------+        +-----------------------+

```

Push style events(HTTPS) flow diagram:

```
+-------------+   +------------+               +-------------------------------------+ +-----------------+
|             |   |  CLIENT    |               |              BMCWEB                 | |   EVENT LOG/    |
|   CLIENT    |   | LISTENERS  |               |RESOURCE SENDER  FORMATTER  MONITOR  | |TELEMETRY SERVICE|
+-----+-------+   +-----+------+               +--+--------+--------+---------+------+ +-----+-----------+
      |                 |                         |        |        |         |              |
+----------------------------------------------------------------------------------------------------
|SUBSCRIBE FOR EVENT|   |                         |        |        |         |              |
+-------------------+   |                         |        |        |         |              |
      |                 |                         |        |        |         |              |
      |                 |                         |        |        |         |              |
      |                 |    EVENT SUBSCRIPTION   |        |        |         |              |
      +------------------------------------------>+        |        |         |              |
      |                 |     PUSH STYLE EVENT    |        |        |         |              |
      |                 |                         |-------------------------> |              |
      |                 |                         |    SEND SUBSCRIPTION INFO |              |
      |                 |                         |----------------------------------------->|
      |                 |                         |    LOG REDFISH EVENT LOG  |              |
      |                 |                         |        |        |         |              |
+----------------------------------------------------------------------------------------------------
|SEND EVENTS/METRICS WHEN GENERATED |             |        |        |         |              |
+-----------------------------------+             |        |        |         |              |
      |                 |                         |        |        |         |              |
      |                 |                         |        |        |         |              +----+
      |                 |                         |        |        |         |              |LOOP|
      |                 |                         |        |        |         |              +<---+
      |                 |                         |        |        |         +<------------>+
      |                 |                         |        |        |         | NOTIFY EVENT/TELEMETRY
      |                 |                         |        |        |         |       DATA   |
      |                 |                         |        |        |         +-------------->
      |                 |                         |        |        |         | READ DATA    |
      |                 |                         |        |        |         +<-------------+
      |       +-----+--------------------------------------------------------------------------+
      |       |LOOP |   |                         |        |        |         |              | |
      |       +-----+   |                         |        |        |         |              | |
      |       |         |                         |        |        |         +-----------+  | |
      |       |         |                         |        |        |         | MATCH     |  | |
      |       |         |                         |        |        |         |SUBSCRIPTION  | |
      |       |         |                         |        |        |         |WITH DATA  |  | |
      |       |         |                         |        |        |         +-----------+  | |
      |       |         |                         |        |        |         |              | |
      |       |         |                         |        |        +<--------+              | |
      |       |         |                         |        |        | SUBSCRIPTION           | |
      |       |         |                         |        |        |  AND  DATA             | |
      |       |         |                         |        |        |         |              | |
      |       |         |                         |        +<-------+         |              | |
      |       |         |                         |        |SUBSCRIPTION AND  |              | |
      |       |         |                         |        |FORMATTED DATA    |              | |
      |       |         |                         |        |        |         |              | |
      |       |         +--------------------------------------------------------------------+-+
      |       |         |FIRST TIME, CREATE CONNECTION |   |        |         |              | |
      |       |         +------------------------------+   |        |         |              | |
      |       |         <---------------------------------->        |         |              | |
      |       |         |  ESTABLISH HTTP/HTTPS   |        +-----+  |         |              | |
      |       |         |     CONNECTION          |        |STORE|  |         |              | |
      |       |         |                         |        |CONN-SUBS         |              | |
      |       |         |                         |        |MAP  |  |         |              | |
      |       |         |                         |        |<----+  |         |              | |
      |       |         |                         |        |        |         |              | |
      |       |         |                         |        +----+|  |         |              | |
      |       |         |                         |        |PUSH QUEUE        |              | |
      |       |         |                         |        |INCOMING|         |              | |
      |       |         |                         |        |DATA |  |         |              | |
      |       |         |                         |        |<---+|  |         |              | |
      |       |         |                         |        |        |         |              | |
      |       | +------+-----------------------------------------+  |         |              | |
      |       | |RETRY ||<---------------------------------+     |  |         |              | |
      |       | |LOOP  ||   SEND FORMATTED DATA   |        |     |  |         |              | |
      |       | |-+----+|                         |        |     |  |         |              | |
      |       | | +-----------------------------------------------------------------------------
      |       | | |SEND FAILED |                  |        |     |  |         |              | |
      |       | | +------------+                  |        |     |  |         |              | |
      |       | | |     |                         |        |     |  |         |              | |
      |       | | |     <---------------------------------->     |  |         |              | |
      |       | | |     |  ESTABLISH HTTP/HTTPS   |        +----+|  |         |              | |
      |       | | |     |     CONNECTION          |        |REOPEN  |         |              | |
      |       | | |     |                         |        |CONN&|  |         |              | |
      |       | | |     |                         |        |UPDATE STORE      |              | |
      |       | | |     |                         |        |<---+|  |         |              | |
      |       | | +------------REPEAT SEND LOOP------------------|  |         |              | |
      |       | | +-----------------------------------------------------------------------------
      |       | | |SEND SUCCESS |                 |        |     |  |         |              | |
      |       | | +-------------+                 |        |     |  |         |              | |
      |       | |       |                         |        |---+ |  |         |              | |
      |       | |       |                         |        |POP| |  |         |              | |
      |       | |       |                         |        |QUEUE|  |         |              | |
      |       | |       |                         |        |<--+ |  |         |              | |
      |       | |       |                         |        |     |  |         |              | |
      |       | |       |                         |        +----+|  |         |              | |
      |       | |       |                         |        |SEND NEXT         |              | |
      |       | |       |                         |        |QUEUEED |         |              | |
      |       | |       |                         |        |DATA||  |         |              | |
      |       | |       |                         |        |<---+|  |         |              | |
      |       | |       |                         |        |     |  |         |              | |
      |       | | +-----------------------------------------------------------------------------
      |       | | |REACHED THRESHOLD LIMIT |      |        |     |  |         |              | |
      |       | | +------------------------+      |        |     |  |         |              | |
      |       | |       |                         |        |----+|  |         |              | |
      |       | |       |                         |        |DELETE  |         |              | |
      |       | |       |                         |        |STORE|  |         |              | |
      |       | |       |                         |        |&QUEUE  |         |              | |
      |       | |       |                         |        |<--+ |  |         |              | |
      |       | |       |                         |<-------|     |  |         |              | |
      |       | |       |                         |REMOVE RESOURCE  |         |              | |
      |       | |       |                         +------+ |     |  |         |              | |
      |       | |       |                         |REMOVE| |     |  |         |              | |
      |       | |       |                         |SUBSCRIPTION  |  |         |              | |
      |       | |       |                         |& CONNECTION  |  |         |              | |
      |       | |       |                         |<-----+ |     |  |         |              | |
      |       | |       |                         |-------------------------->|              | |
      |       | |       |                         |  UPDATE SUBSCRIPTION      |              | |
      |       | |       |                         |----------------------------------------->| |
      |       | |       |                         |    LOG REDFISH EVENT LOG  |              | |
      |       | |       |                         |        |     |  |         |              | |
      |       | +-------+----------------------------------------+  |         |              | |
      |       |         |                         |        |        |         |              | |
      |       +--------------------------------------------------------------------------------+
      |                 |                         |        |        |         |              |
+--------------------------------------------------------------------------------------------------
|DELETE EVENT SUBSCRIPTIONS |                     |        |        |         |              |
+---------------------------+                     |        |        |         |              |
      |                 |                         |        |        |         |              |
      +-----------------+------------------------>+        |        |         |              |
      |        DELETE SUBSCRIPTION                |        |        |         |              |
      |                 |                         +------+ |        |         |              |
      |                 |                         |REMOVE| |        |         |              |
      |                 |                         |SUBSCRIPTION     |         |              |
      |                 |                         <------+ |        |         |              |
      |                 +<------------------------|        |        |         |              |
      |                 |    CLOSE CONNECTION     |        |        |         |              |
      |                 |                         |--------------------------->              |
      |                 |                         |    UPDATE SUBSCRIPTION    |              |
      |                 |                         |----------------------------------------->|
      |                 |                         |    LOG REDFISH EVENT LOG  |              |
      |                 |                         |        |        |         |              |

```

SSE flow diagram:

```
+-------------+   +------------+               +-------------------------------------+ +-----------------+
|             |   |  CLIENT    |               |              BMCWEB                 | |   EVENT LOG/    |
|   CLIENT    |   | LISTENERS  |               |RESOURCE SENDER  FORMATTER  MONITOR  | |TELEMETRY SERVICE|
+-----+-------+   +-----+------+               +--+--------+--------+---------+------+ +-----+-----------+
      |                 |                         |        |        |         |              |
+--------------------------------------------------------------------------------------------------------
|SUBSCRIBE FOR EVENT|   |                         |        |        |         |              |
+-------------------+   |                         |        |        |         |              |
      |                 |                         |        |        |         |              |
      |                 |                         |        |        |         |              |
      |                 |    EVENT SUBSCRIPTION   |        |        |         |              |
      |                 |-------------------------|        |        |         |              |
      |                 |     SSE(ON ESTABLISHED CONNECTION)        |         |              |
      |                 |                         |        |        |         |              |
      |                 |                         +-----+  |        |         |              |
      |                 |                         |KEEP |  |        |         |              |
      |                 |                         |CONN |  |        |         |              |
      |                 |                         |HANDLE  |        |         |              |
      |                 |                         +<----+  |        |         |              |
      |                 |                         |        |        |         |              |
      |                 |                         |--------------------------->              |
      |                 |                         |    SEND SUBSCRIPTION INFO |              |
      |                 |                         |        |        |         |              |
+-----------------------------------------------------------------------------------------------------
|SEND EVENTS/METRICS WHEN GENERATED |             |        |        |         |              |
+-----------------------------------+             |        |        |         |              |
      |                 |                         |        |        |         |              |
      |                 |                         |        |        |         |              +----+
      |                 |                         |        |        |         |              |LOOP|
      |                 |                         |        |        |         |              +<---+
      |                 |                         |        |        |         +<------------>+
      |                 |                         |        |        |         | NOTIFY EVENT/TELEMETRY
      |                 |                         |        |        |         |       DATA   |
      |                 |                         |        |        |         +-------------->
      |                 |                         |        |        |         | READ DATA    |
      |                 |                         |        |        |         +<-------------+
      |                 |                         |        |        |         |              |
      |                 |                         |        |        |         +-----------+  |
      |                 |                         |        |        |         | MATCH     |  |
      |                 |                         |        |        |         |SUBSCRIPTION  |
      |                 |                         |        |        |         |WITH DATA  |  |
      |                 |                         |        |        |         +-----------+  |
      |       +-----+--------------------------------------------------------------------------+
      |       |LOOP |   |                         |        |        +<--------+              | |
      |       +-----+   |                         |        |        | SUBSCRIPTION           | |
      |       |         |                         |        |        |  AND  DATA             | |
      |       |         |                         |        |        |         |              | |
      |       |         |                         |        +<-------+         |              | |
      |       |         |                         |        |SUBSCRIPTION AND  |              | |
      |       |         |                         |        |FORMATTED DATA    |              | |
      |       |         |                         |        |        |         |              | |
      |       |         |                         |        +----+|  |         |              | |
      |       |         |                         |        |PUSH QUEUE        |              | |
      |       |         |                         |        |INCOMING|         |              | |
      |       |         |                         |        |DATA |  |         |              | |
      |       |         |                         |        |<---+|  |         |              | |
      |       |         |                         |        |        |         |              | |
      |       | +------+-----------------------------------------+  |         |              | |
      |       | |RETRY ||<---------------------------------+     |  |         |              | |
      |       | |LOOP  ||   SEND FORMATTED DATA   |        |     |  |         |              | |
      |       | |+-----+|                         |        |     |  |         |              | |
      |       | |+-------------------------------------------------------------------------------
      |       | ||SEND FAILED |                   |        |     |  |         |              | |
      |       | |+------------+                   |        |     |  |         |              | |
      |       | ||      |                         |        |     |  |         |              | |
      |       | |+-------------REPEAT SEND LOOP------------------|  |         |              | |
      |       | |       |                         |        |     |  |         |              | |
      |       | |+-------------------------------------------------------------------------------
      |       | ||SEND SUCCESS |                  |        |     |  |         |              | |
      |       | |+-------------+                  |        |     |  |         |              | |
      |       | |       |                         |        |---+ |  |         |              | |
      |       | |       |                         |        |POP| |  |         |              | |
      |       | |       |                         |        |QUEUE|  |         |              | |
      |       | |       |                         |        |<--+ |  |         |              | |
      |       | |       |                         |        +----+|  |         |              | |
      |       | |       |                         |        |SEND NEXT         |              | |
      |       | |       |                         |        |QUEUEED |         |              | |
      |       | |       |                         |        |DATA ON |         |              | |
      |       | |       |                         |        |<---+|  |         |              | |
      |       | |       |                         |        |     |  |         |              | |
      |       | |+-------------------------------------------------------------------------------
      |       | ||REACHED THRESHOLD LIMIT |       |        |     |  |         |              | |
      |       | |+------------------------+       |        |     |  |         |              | |
      |       | |       |                         |        |----+|  |         |              | |
      |       | |       |                         |        |DELETE  |         |              | |
      |       | |       |                         |        |STORE|  |         |              | |
      |       | |       |                         |        |& QUEUE |         |              | |
      |       | |       |                         |        |<---+|  |         |              | |
      |       | |       |                         |<-------|     |  |         |              | |
      |       | |       |                         |REMOVE RESOURCE  |         |              | |
      |       | |       |                         +------+ |     |  |         |              | |
      |       | |       |                         |REMOVE| |     |  |         |              | |
      |       | |       |                         |SUBSCRIPTION  |  |         |              | |
      |       | |       |                         |& CONNECTION  |  |         |              | |
      |       | |       |                         |<-----+ |     |  |         |              | |
      |       | |       |                         |-------------------------->|              | |
      |       | |       |                         |  UPDATE SUBSCRIPTION      |              | |
      |       | |       |                         |----------------------------------------->| |
      |       | |       |                         |    LOG REDFISH EVENT LOG  |              | |
      |       | |       |                         |        |     |  |         |              | |
      |       | +-------+----------------------------------------+  |         |              | |
      |       |         |                         |        |        |         |              | |
      |       +--------------------------------------------------------------------------------+
      |                 |                         |        |        |         |              |
+--------------------------------------------------------------------------------------------------
|DELETE EVENT SUBSCRIPTIONS |                     |        |        |         |              |
+---------------------------+                     |        |        |         |              |
      |                 |                         |        |        |         |              |
      +-----------------+------------------------>+        |        |         |              |
      |        DELETE SUBSCRIPTION                |        |        |         |              |
      |                 |                         +------+ |        |         |              |
      |                 |                         |REMOVE| |        |         |              |
      |                 |                         |SUBSCRIPTION     |         |              |
      |                 |                         <------+ |        |         |              |
      |                 +<------------------------|        |        |         |              |
      |                 |    CLOSE CONNECTION     |        |        |         |              |
      |                 |                         |--------------------------->              |
      |                 |                         |    UPDATE SUBSCRIPTION    |              |
      |                 |                         |----------------------------------------->|
      |                 |                         |    LOG REDFISH EVENT LOG  |              |
      |                 |                         |        |        |         |              |
+--------------------------------------------------------------------------------------------------
|CLIENT CLOSE CONNECTION |                        |        |        |         |              |
+------------------------+                        |        |        |         |              |
      |                 |                         |        |        |         |              |
      |                 |    CLIENT CLOSED        |        |        |         |              |
      |                 |------------------------>|        |        |         |              |
      |                 |                         +-------+|        |         |              |
      |                 |                         |REMOVE ||        |         |              |
      |                 |                         |SUBSCRIPTION     |         |              |
      |                 |                         +<------+|        |         |              |
      |                 |                         |--------------------------->              |
      |                 |                         |    UPDATE SUBSCRIPTION    |              |
      |                 |                         |----------------------------------------->|
      |                 |                         |    LOG REDFISH EVENT LOG  |              |
      |                 |                         |        |        |         |              |
      v                 v                         v        v        v         v              v
```

EventService design includes below major blocks.

1. Redfish resources (Schema based)
   1. EventService
   2. Subscription Collections (EventDestination collection)
   3. Subscriptions (EventDestination)
2. Event Service Manager
   1. Event log monitor
   2. Event log formatter
   3. MetricReport monitor/agent
   4. MetricReport formatter
   5. Event service Sender (Common)
3. EventService config store ( Persistent)

This document also covers the description of below modules.

1. Server-Sent Event(SSE): It covers details on what is SSE and how connection
   open, close are handled in different cases.
2. Push style events(HTTP/HTTPS): It covers how to subscribe for Push style
   events and how the http/https connections are handled in bmcweb.
3. SubmitTestEvent: Details on submit test event action.

This document doesn't cover below component design.

1.  Redfish Event Logs: This is existing implementation. In OpenBMC, All events
    will be logged to Journal control logs and RSyslog service, monitors event
    logs associated with "REDFISH_MESSAGE_ID" and write the required information
    to "/var/log/redfish" for persisting the event logs.

         So EventService design is hooked to Redfish Event Logs majorly for below
         two reasons.

           - Get the notification whenever new event is logged to Redfish event
             Logs.
           - Way to read the Redfish event log information.

         This document covers one example Redfish Event Log implementation covered
         under below design. As part of it, it uses inotify for getting
         notification about new event log and read the log information by accessing
         the redfish event log file.

         Refer below link for more information on same.

    [link](https://github.com/openbmc/docs/blob/master/redfish-logging-in-bmcweb.md)

         If other OEM's has different implementation for Redfish EventLogs, they
         can satisfy  above mentioned two requirement and can hook it to
         EventService design specified here.
         For example, If xyz OEM uses D-Bus based Redfish Event logs, They can
         use 'signal' for notifying the new redfish event log and event log
         information. They can replace 'inotify' with 'signal' and hook their
         OEM specific Redfish event log to this Redfish EventService design.

2.  Telemetry service: Redfish telemetry service is used to gather all the
    metrics reports associated with different resources such as Power metrics,
    thermal metrics, memory metrics, processor metrics etc... Monitoring service
    in telemetry supports different modes along with aggregated
    operations(Single, average, max, min, sum etc..).

         Refer below link for design document of telemetry service.

    [link](https://gerrit.openbmc.org/#/c/openbmc/docs/+/24357/)

### Redfish resource schema's

Redfish will have three major resources to support EventService.

1. Event Service
2. Subscription Collections
3. Subscriptions

All the configuration and subscriptions information data will be cached on
bmcweb webserver and synced with config json file for restoring and persisting
the data.

**EventService**

Contains the attributes of the service such as Service Enabled, status, delivery
retry attempts, Event Types, Subscriptions, Actions URI etc.

- URI : `/redfish/v1/EventService`
- SCHEMA :
  [EventService](https://redfish.dmtf.org/schemas/v1/EventService_v1.xml)
- METHODS : `GET, PATCH`

Refer Testing for supported properties and example.

**Subscription Collections** This resource is used to get collection of all
subscriptions and also used to configure the new subscriptions for automatic
events.

- URI : `/redfish/v1/EventService/Subscriptions`
- SCHEMA :
  [EventDestinationCollection](https://redfish.dmtf.org/schemas/v1/EventDestinationCollection_v1.xml)
- METHODS : `GET, POST`

Refer Testing section for example.

**Subscriptions** This resource contains the subscription details. Used to get
each individual get subscription information, update & Delete existing
subscription information.

- URI : `/redfish/v1/EventService/Subscriptions/<ID>`
- SCHEMA :
  [EventDestination](https://redfish.dmtf.org/schemas/v1/EventDestination_v1.xml)
- METHODS : `GET, PATCH, DELETE`

Refer Testing section for supported properties and example.

### Event service manager

Event Manager is main block which does monitor the events, format the request
body and send it to the subscribed destinations. It shall support for two types
of event formats which are define in redfish specification.

1. Event: Resource type 'Event' is used to receive alerts in the form of json
   body to subscribed clients when the subscription details matches for those
   event logs.

2. MetricReport: It is used to receive alerts in the forms of json body to
   subscribed clients when the subscription details matches for the collected
   telemetry data by telemetry service.

The "Event service sender" is common entity for both type of resources. The
formatter and monitor entities are different for each type of resources.

**Event log monitor**

In OpenBMC, all the redfish event logs are currently logged to persistent area
of BMC filesystem("/var/log/redfish"). The "Event log monitor" service under
"Event service manager" will use **inotify** to watch on redfish event log file
to identify the new events. Once it identifies the new events, it will read all
logged events information from redfish event file and pass it to 'Event log
formatter' for further processing. The last processed event data timestamp will
be cached in bmcweb to identify the new events in redfish event log file.

It monitors the live events for sending asynchronously and does not send the
events logged before bmcweb service start.

**Event log formatter**

It is used to format the request body by reading event logs passed from 'Event
log monitor' and filter the data by reading subscription information. It will
fetch all the subscription information from "Redfish event service store" and
filter the events by checking severity, event type against the user subscribed
information. It formats the http "request body" along with CustomText,
HttpHeaders and send the Events over network using http protocol for the
filtered subscribers.

Refer Tested section for example request body.

**MetricReport monitor**

Telemetry monitoring service ( Not covered in this document) is responsible for
gathering metrics from D-Bus sensors and exposing them as D-Bus objects.
Monitoring service supports different monitoring modes (periodic, on change and
on demand) along with aggregated operations such as single, average, max min,
sum etc... This service periodically collects the metrics report depending on
Telemetry service configuration such as 'Metric Definitions', 'Metric Report
Definitions' and 'Metric Triggers'.

The telemetry monitor shall emit a signal when there is a collection of report.
MetricReport monitor/agent shall implement the listener for telemetry signal and
using the subscription information, it should match, filter the desired metric
report to pass it on to 'MetricReport formatter'. Since 'MetricReport monitor'
is tightly coupled with 'Telemetry monitor' which already does the monitoring
loop, its responsibility of 'Telemetry monitor' to match the subscriptions rules
and provide it to MetricReport formatter. The EventService should provide a way
to communicate the subscriptions data whenever user creates or updates or delete
subscription.

**MetricReport formatter**

The 'MetricReport formatter' is responsible for formatting the data which comes
from 'MetricReport monitor/agent' by checking subscriptions information such as
HttpHeaders, Custom text strings along with Telemetry report data.

**Event service sender**

The "Event service sender" is used to send the formatted request body which is
passed from 'Event log formatter' or 'Telemetry formatter' to all filtered
subscribers. It supports sending both https and http requests by using beast
library. The http/https connections handling (open/close) differs for both
subscription types such as 'Push style events' and 'SSE' and more detailed are
covered in respective sections.

Beast is a C++ header-only library serving as a foundation for writing
interoperable networking libraries by providing low-level HTTP/1, WebSocket, and
networking protocol vocabulary types and algorithms using the consistent
asynchronous model of Boost.Asio. For more information on beast, refer
[link](https://www.boost.org/doc/libs/develop/libs/beast/doc/html/beast/introduction.html)

1. PUSH style eventing: BMC uses the boost::beast for asynchronously sending
   events to the subscribed clients. Here is the Beast client Examples for
   sending the synchronous and asynchronous requests over encrypted and
   un-encrypted channels.
   [link](https://www.boost.org/doc/libs/develop/libs/beast/example/http/client/async/)
   [link](https://www.boost.org/doc/libs/develop/libs/beast/example/http/client/async-ssl/)

2. Server-Sent Events (SSE): The EventDestination resource of the subscription
   data shall contains the opaque of opened client SSE stream connection. After
   receiving the formatted event associated with specific subscription
   information, EventSender writes the formatted event data on open SSE
   connection with specific ID so that client can receive the event.

In case of push style eventing, https connection will be established during
first event. If the connection is already established with subscription ID, then
it uses same connection handle for sending events. This connection handle and
subscription map will be stored in event service sender. Users can have multiple
subscriptions coming from single destination and BMC will have separate
connection handle associated with each subscription. Event listeners (web
servers) has capability to accept multiple connections from same client(BMC) and
depending on routing rules, the received events will be parsed on event listener
side.

In case of SSE, connection handle will be read from resource and used by sender
for sending events.

Received formatted data will be pushed to queue maintained under sender, against
the connection-subscription map. Upon successful sending of event data,
formatted data entry will be pop-ed from queue and next formatted data will be
attempted for send and so on.

If send event(logs or metric reports) fails, then it will retry for number of
attempts configured in Event Service "DeliveryRetryAttempts" with an interval of
"DeliveryRetryIntervalSeconds". In case of Push Style Events if send fails due
to some reasons, https connection will be re-established and first send will be
re-attempted. If the failure persist even after the configured retry attempts,
subscription information will be deleted and same will be updated to
consumers(Telemetry service or event log service). In SSE connection, bmcweb
would only retry sending events in the case of network
failure/timeout/congestion. If there is error like connreset/badf, bmcweb will
delete the subscription as specified above. In such scenario's, user can
re-subscribe for events. This is requirement as per current redfish
specification.

Any temporary network failure during event notification, and the failure persist
till threshold limit, subscription entry will be remove. This may be security
concern as client may not know about subscription termination, once network
comes back. Note: Redfish specification says, Event destination subscription
should be terminated after reaching threshold limit. This is known security
concern. TBD: Need to work with Redfish forum for resolution.

### Redfish event service store

Redfish event service store is used to store the all configuration related to
EventService and subscriptions information. This does below operations:

- During bmcweb service start, it gets all the configuration and subscriptions
  information from config json file and populates the internal structures data.

- Redfish EventService resources will get/update this event store for any user
  activities via redfish(Like Get/Add/Update/Delete subscriptions etc.)

- It will sync the "event config store" with configuration Json file for
  persisting the information when there is update. **Note**: Configuration json
  file will be updated only for Push style event subscription information. SSE
  events subscription information is volatile and will not get updated to json
  file.

- It will be used for getting all the subscription information to the "Event
  service manager" for filtering event data.

The bmcweb service will update the config store data including EventService
configuration and subscription information to the json file(location:
/var/lib/bmcweb/eventservice_data.json). This will be persistent across the
bmcweb service restarts or bmc reboots and loads this while initializing bmcweb
EventService class.

Note:

1. In case of SSE, Client is initiator of connection and so SSE stream
   connection can't be re-established when bmcweb service or BMC goes for
   reboot. Hence subscription information related to SSE will not be persistent
   and Client must re-subscribe for SSE events using "ServerSentEventUri",
   whenever bmc comes out of reset.

   Limitation: If bmcweb server closes abruptly due to some reasons like
   watchdog trigger, then bmcweb can't send close connection event to SSE
   client(For notifying SSE client to re-establishing connection).

Cache implementation includes below major blocks:

- EventServiceStore Class: It holds the map of all subscriptions with unique key
  and methods to Create/Delete/update subscription information. It also holds
  the high level configuration information associated with EventService schema.

- Configuration structure: It holds the configuration associated with
  EventService schema properties.

- Subscription structure: It holds the data associated with each
  EventDestination schema properties along with established connection handle.

### Server-Sent Events (SSE):

Server-Sent Events (SSE) is a server push technology enabling a browser to
receive automatic updates from a server via HTTP connection. The Server-Sent
Events EventSource API is standardized as part of HTML5 by the W3C.

Server-Sent Events is a standard describing how servers can initiate data
transmission towards clients once an initial client connection has been
established. They are commonly used to send message updates or continuous data
streams to a browser client and designed to enhance native, cross-browser
streaming through a JavaScript API called EventSource, through which a client
requests a particular URL in order to receive an event stream.

The "EventService" resource contains a property called "ServerSentEventUri". If
a client performs a GET on the URI specified by the "ServerSentEventUri", the
bmcweb server keep the connection open and conform to the HTML5 Specification
until the client closes the socket. The bmcweb service should sent the response
to GET URI with following headers.

```
'Content-Type': 'text/event-stream',
'Cache-Control': 'no-cache',
'Connection': 'keep-alive'
```

Events generated by the OpenBMC will be sent to the clients using the open
connection. Response of event message should contain the "data: " prefix and
postfix "\n" on each line. For representing the end of data, it should end with
"\n\n". Example sending json response should look like

```
data: {\n
data: "msg": "hello world",\n
data: "id": 12345\n
data: }\n\n
```

Open connection: When a client performs get on 'ServerSentEventUri' with desired
filters, bmcweb create an EventDestination instance in the subscriptions
collection. Once bmcweb receives the SSE stream request, the open connection
will be upgraded to WebSocket by checking the request Content-Type
header(matches to 'text/event-stream') and keep the connection alive till the
client/server closes the connection. Please refer
[link](https://gerrit.openbmc.org/#/c/openbmc/bmcweb/+/13948/) for details on
how to upgrade the connection for SSE. The "Context" property in the
EventDestination resource shall be an opaque string generated by the service.

Note: since its GET operation on existing open connection(which is authenticated
and authorized as normal operation), there is no special security breach with
it. All bmcweb related security threat model applies here.

Close connection: Either the client or the server can terminate the event stream
at any time and close the connection. The bmcweb server can delete the
corresponding EventDestination instance when the connection is closed.

Connection can be closed by,

- Client by sending DELETE method on subscribed id URI.
- Server(bmcweb) can close connection and send notification to client with
  'event: close' request body.

Since SSE is uni-directional, if client closes connection/browser abruptly,
bmcweb service doesn't know the client connection state. This is limitation on
SSE connection. In such scenario bmcweb service may delete a subscription, if
the number of delivery errors exceeds preconfigured thresholds.

Note:

- The SSE subscription data is not persistent in nature. When bmcweb restarts or
  BMC reboots, client should re-establish the SSE connection to bmcweb to listen
  for events.
- In case of SSE, client can send the "Last-Event-ID" as header. Monitor service
  will fetch all the event or metric report stream captured after that specified
  "Last-Event-ID" and send to client after formatting. If it fails to find the
  specified event ID, it will start streaming only live events/metric reports.

The service should support using the $filter query parameter provided in the URI
for the SSE stream by the client to reduce the amount of data returned to the
client.

Supported filters are:

- RegistryPrefix
- ResourceType
- EventFormatType
- MessageId
- OriginResource

Example for subscribing SSE with filters:

```
MessageId based filter:
METHOD: GET
URi: https://<BMCIP>/redfish/v1/EventService/SSE?MessageId='Alert.1.0.LanDisconnect'

RegistryPrefix based filter:
https://sseuri?$filter=(RegistryPrefix eq Resource) or (RegistryPrefix eq
Task)

ResourceType based filter:
https://sseuri?$filter=(ResourceType eq 'Power') or (ResourceType eq
'Thermal')
```

### Push style events (HTTP/HTTPS)

Unlike the SSE (where bmcweb act as server) push style events works in different
way. In this case, BMC acts as HTTP client and send the data to webserver
running on external system. In case of SSE, connection will be kept alive till
stream closes but in push style events, connections are opened on need and
closes immediately after pushing event/report data.

By using POST method on '/redfish/v1/EventService/Subscriptions' URI with
desired request body (mandatory fields: Destination, Protocol), user can
subscribe for the events. The monitor and formatter resources (both Event log
and MetricReport) uses the subscribed data for match rules, when there is event
log or Telemetry data available. The 'Event service sender' uses the destination
field in subscribed data for opening the connection and pushing data to
webserver running externally.

It shall support both http and https (https is not defined in Redfish
specification) connections which will be created using the boost::beast library.
For https connection it uses the self-signed certificates which are loaded in
certificate store. If establishing connection fails, it retries for configured
retry attempts (DeliveryRetryAttempts) and delete the subscription if it reaches
threshold limit. If SSL negotiation fails, it will not retry for connection
establishment. Users can upload new certificates. More details about certificate
can be found at
[link](https://github.com/openbmc/docs/blob/master/designs/redfish-tls-user-authentication.md)

Example for created Push style event is documented in testing section of same
document.

### SubmitTestEvent

This action shall add a test event to the event service with the event data
specified in the action parameters. This message should then be sent to any all
subscribed Listener destination targets depending on subscription data.

- URI: `/redfish/v1/EventService/Actions/EventService.SubmitTestEvent`
- SCHEMA:
  [EventService](https://redfish.dmtf.org/schemas/v1/EventService_v1.xml)
- METHODS : `POST`

Note: EventService.v1_3_0 was updated to deprecate the EventType parameter in
SubmitTestEvent, and add the EventGroupId parameter in SubmitTestEvent.

Example POST Request body:

```
{
  "Message": "Test Event for validation",
  "MessageArgs": [],
  "EventId": "OpenBMC.0.1.TestEvent",
  "EventGroupId" : "",
  "Severity": "OK"
}
```

## Alternatives considered

**HTTP POLLING**

Polling is a traditional technique where client repeatedly polls a server for
data. The client makes a request and waits for the server to respond with data.
BMC provides a way to pool the event logs (using Events schema). Client can run
continuously http GET on Events LogService and get the all the events from BMC.
But in HTTP protocol fetching data revolves around request/response which is
greater HTTP overhead.

Push style(https) eventing or SSE will avoid the http overhead of continuous
polling and send the data/events when it happens on server side to the
subscribed clients.

**WEBSOCKET**

WebSocket provides a richer protocol to perform bi-directional, full-duplex
communication. Having a two-way channel is more attractive for some case but in
some scenarios data doesn't need to be sent from the client(Client just need
updates from server errors/events when it happens.)

SSE provides uni-directional connection and operates over traditional HTTP. SSE
do not require a special protocol or server implementation to get working.
WebSocket on the other hand, require full-duplex connections and new Web Socket
servers to handle the protocol.

**IPMI PEF**

Platform Event Filtering (PEF) provides a mechanism for configuring the BMC to
take selected actions on event messages that it receives. This mechanism is IPMI
centric which has its own limitation in terms of security, scalability etc. when
compared with Redfish.

## Implementation design alternatives

- Direct journal logs vs Redfish event log file using inotify

Considered reading direct journald logs using sd*journal*<xyz> api's but that
seems to be in-efficient because

1. sdjournal has multiple logs currently( not just event logs) and so this
   become very inefficient to monitor and process data to identify just for
   redfish event logs.

2. Some hooks need to be added to sdjournal to emit signals whenever messages
   are logged to sdjournal.

To overcome above challenges currently redfish event logs are filtered and
logged to separate file using rsyslog. Same file is used by EventService by
monitor the file using inotify.

- D-Bus service vs Event Service config store

Considered creating separate D-Bus service for storing EventService
configuration and subscriptions data. Also considered using settings D-Bus
service for persisting the data across the reboots. But since EventService is
specific to redfish/bmcweb and not consumed/updated by any other processes in
system, there is no real gain with having D-Bus service.

## Impacts

Security issue with Push style events:

Push style events also support HTTP protocol which is not secured and is
susceptible to man-in-the-middle attacks and eavesdropping which may lead to
virus injections and leak sensitive information to attackers. The push style
event subscription or changes to the BMC will be done over the secure HTTPS
channel with full authentication and authorization. So, BMC side security is NOT
compromised. Event Listener on client may get hacked data in-case of
man-in-the-middle attacks. To avoid such attacks, it is recommended to use SSE
event mechanism or https-based push style events.

SSE works on HTTPS protocol which is encrypted and secured. SSE EventService
subscriptions can be created or updated over HTTPS protocol along with proper
authentication and authorization on BMC users. Please refer security details
(section 13) in DSP0266 for more information.

SSE connections:

When client opens the SSE HTTPS connection, it will be on open state until
client or server closes the connection. On Linux, there is a limit on each file
descriptor(Of course this is very huge number > 8K). Multiple open http
connections on bmcweb can slow down the performance.

Since open SSE client connection is proxied under multiple level of BMC side
protection (like Authentication is required, User should have proper privilege
level to establish the SSE connection etc.), these impacts are mitigated. Also
BMC will have support for maximum of 10 SSE subscriptions which will limit the
number of http connections exhaust scenario. This can be compile time option.

## Testing

User should cover below functionalists.

- View and update EventService configuration using GET and PATCH methods and
  validate the functionality.
- Subscribe for Events (Both "Push style event" and "SSE") as described below
  and validate http status code for positive and negative use cases (Body
  parameter invalid, Invalid request, improper privilege - forbidden etc.).
- Validate all subscription collections and each individual subscription.
- Using delete method on subscription URI specific to ID and see connection
  closing properly on client side and subscription data clear on server side.
- Enable and disable EventService, generate events and validate whether Events
  are sending properly to all subscriptions when its enabled and stopped sending
  when disabled.
- Use SubmitTestEvent for generating Test Events. Also cover negative cases with
  invalid body data.
- Validate the functionality of retry attempts and retry timeout with both
  positive and negative cases.
- Client connection close: Close client connection which are listening for SSE
  and check server-side connection closes after configured retry attempts and
  clears the subscription information.
- Reboot BMC or reset bmcweb service and check all "push style events"
  subscription is preserved and functional by generating and checking events.
- Reboot BMC or reset bmcweb service and check all "SSE" subscription data is
  not preserved and client-side connection is closed properly.
- Create user with multiple privileges and check create/delete/update
  subscription and configuration data are performed only for user having
  'ConfiguredManager' privilege and view EventService information works only for
  'Login'.
- Create new subscription and check is the event message logged in redfish event
  logs or not. Also modify, delete subscription and see Redfish events are
  logged or not.
- Validate the authenticity of events sent to client on subscription with proper
  filtered data(Ex: If RegisteryPrefixes is set to "OpenBMC.0.1", it should not
  send events with prefixes "OpenBMC.0.2") should be validated.
- For security, validate GET on URI specified in "ServerSentEventUri" with
  un-authorized users and see it responds with properly http status code and
  should not create any subscription or open any http connection.
- Max limit check: Creates max supported event subscriptions and validate.

** Push style events: **

1. EventListner: Client should configure the web service for listening the
   events (Example: http://192.168.1.2/Events).

2. Subscription: Client can subscribe for events on BMC using below

```
URI: /redfish/v1/EventService/Subscriptions
METHOD: POST
REQUEST BODY:
{
  "Destination":"http://192.168.1.2/Events",
  "Context":"CustomText",
  "Protocol":"Redfish",
}
```

     On successful event subscription client will receive 201(Created) http
     status code along with created subscription ID. At this point onwards,
     client will receive events associated with subscribed data.

     Client can use "SubmitTestEvent" (Documented above) to test the working
     state.

3. ViewSubscription: Client can get the subscriptions details using

```
URI: /redfish/v1/EventService/Subscriptions/<ID>
METHOD: GET
```

4. UpdateSubscription: At any point client can update the subscription
   information.

```
URI: /redfish/v1/EventService/Subscriptions/<ID>
METHOD: PATCH
REQUEST BODY:
{
  "Destination":"http://192.168.1.2/Events",
  "Context":"Changed Custom Text",
  "RegistryPrefix": "OpenBMC.0.2"
}
```

5. DeleteSubscription: Client can unsubscribe and stop receiving events by
   deleting the subscription.

```
URI: /redfish/v1/EventService/Subscriptions/<ID>
METHOD: DELETE
```

** Server-Sent events subscription: **

1. EventListner and Subscription: Client can open latest browser (chrome) and
   fire GET URI on "ServerSentEventUri" along with filters.

```
URI: /redfish/v1/EventService/SSE?RegistryPrefix='OpenBMC.0.2'&MessageId='OpenBMC.0.2.xyz"
METHOD: GET
```

     On successful event subscription client will receive 200(Created) http
     status code, subscription id along with below response headers for
     streaming.

```
'Content-Type': 'text/event-stream',
'Cache-Control': 'no-cache',
'Connection': 'keep-alive'
```

     At this point onwards, client browser will receive events associated with
     subscribed data.

     Client can use "SubmitTestEvent" (Documented above) to test the working
     state.

2. ViewSubscription: Client can get the subscriptions details using

```
URI: /redfish/v1/EventService/Subscriptions/<ID>
METHOD: GET
```

3. UpdateSubscription: At any point client can update the subscription
   information. Note Client can't change the destination as that is opaque
   associated with opened SSE connection.

```
URI: /redfish/v1/EventService/Subscriptions/<ID>
METHOD: PATCH
REQUEST BODY:
{
  "Context":"Changed Custom Text",
  "RegistryPrefix": "OpenBMC.0.2"
}
```

4. DeleteSubscription: There are two ways to close the connection from client.
   - Client can close the browser directly which will close the SSE http
     connection and so bmcweb service can close and delete the subscription
     data.
   - Client can also unsubscribe and stop receiving events by deleting the
     subscription.

```
URI: /redfish/v1/EventService/Subscriptions/<ID>
METHOD: DELETE
```

** EventListner **

Push style Events:

To listen the push style events, client can have web server running on any
external system and subscribe to BMC with the subscription details along with
destination URI.

There are multiple EventListners exist and here is simple node js server, which
can be run using "node server.js".

```
// server.js
const http = require("http");
var displayResponse;

http.createServer((request, response) => {
    console.log("Requested url: " + request.url);
    const { headers, method, url } = request;

    if ((method.toLowerCase() == "post") && (url.toLowerCase() === "/events")) {
        let body = [];
        request.on('error', (err) => {
          console.error(err);
        }).on('data', (chunk) => {
          body.push(chunk);
        }).on('end', () => {
          body = Buffer.concat(body).toString();

          console.log(body);
          displayResponse.write(body);
          displayResponse.write("\n\n");

          response.statusCode = 200;
          response.end();
        })
    } else if ((method.toLowerCase() == "get") && (url.toLowerCase() === "/display_events")) {
      response.writeHead(200, {
        Connection: "keep-alive",
        "Content-Type": "text/event-stream",
        "Cache-Control": "no-cache",
        "Access-Control-Allow-Origin": "*"
      });

      displayResponse = response;
    } else {
      response.writeHead(404);
      response.end();
    }
  }).listen(5000, () => {
    console.log("Use 'http://<IP>:5000/events' as destination to subscribe for BMC events");
    console.log("Open 'http://<IP>:5000/display_events' URL on browser\n");
  });
```

Server-Sent Events (SSE):

Client can open an SSE connection to the service by performing a GET on the URI
specified by the "ServerSentEventUri" in the Event Service along with filters.
Browser automatically prompt for credentials and upon successful completion of
authentication and authorization, client connection will be established for
listening events.

You can also refer below link for details on Server-Sent events.
(https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events)

Another simple python script for testing SSE using EventSource. Run it using
"python sse-client.py".

```
// sse-client.py
import requests, json, sys, time, re, os
import sseclient
from requests.packages.urllib3.exceptions import InsecureRequestWarning
requests.packages.urllib3.disable_warnings(InsecureRequestWarning)
from sseclient import SSEClient

try:
  bmc_ip = sys.argv[1]
  username = sys.argv[2]
  password = sys.argv[3]

except:
  print "FAIL: Usage: <ScriptName> <BMC_IP> <Username> <Password>"
  print "Example: python sseClient.py 10.223.244.145 root root\n"
  sys.exit()


url = 'https://%s/redfish/v1/EventService/SSE?<filters>' %bmc_ip

try:
  messages = SSEClient(url, verify=False, auth=(username, password))

  for msg in messages:
    print(json.dumps(msg.data))

except Exception as exc:
  print("\nERROR: Error in subscribing the SSE events\n")
```

**EventService**

Supported properties:

| PROPERTY NAME                | TYPE          | DESCRIPTION                                                                                                                                                                                                                                                                                                     |
| ---------------------------- | ------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| ServiceEnabled               | Boolean       | This indicates whether event service is enabled. Default: True                                                                                                                                                                                                                                                  |
| DeliveryRetryAttempts        | Integer       | This is the number of attempts an event posting is retried before the subscription is terminated. Default: 3                                                                                                                                                                                                    |
| DeliveryRetryIntervalSeconds | Integer       | This represents the number of seconds between retry attempts for sending any given Event. Default: 30                                                                                                                                                                                                           |
| EventFormatTypes             | Array(string) | Indicates the content types of the message that this service can send to the event destination. ** Event: ** Resource types of 'Event' sent to subscribed destination. This is default value if nothing is specified. ** MetricReport: ** Resource types of 'MetricReport' only sent to subscribed destination. |
| RegistryPrefixes             | Array(string) | A list of the Prefixes of the Message Registries that can be used for the RegistryPrefix property on a subscription. If this property is absent or contains an empty array, the service does not support RegistryPrefix-based subscriptions.                                                                    |
| ResourceTypes                | Array(string) | A list of the Prefixes of the message registries that can be used for the RegistryPrefix property on a subscription. If this property is absent or contains an empty array, the service does not support RegistryPrefix-based subscriptions                                                                     |
| ServerSentEventUri           | String        | The value of this property shall be a URI that specifies an HTML5 Server-Sent Event conformant endpoint. URI: /redfish/v1/EventService/SSE                                                                                                                                                                      |
| Subscriptions                | Object        | The value of this property shall contain the link to a collection of type EventDestinationCollection.                                                                                                                                                                                                           |
| Actions                      | Object        | This action shall add a test event to the event service with the event data specified in the action parameters. More details on TestEvents captured below.                                                                                                                                                      |

Note: EventService.v1_3_0 was created to deprecate the EventTypesForSubscription
and SSEFilterPropertiesSupported\EventType properties.

Example GET Response:

```
{
  "@odata.context": "/redfish/v1/$metadata#EventService.EventService",
  "@odata.id": "/redfish/v1/EventService",
  "@odata.type": "#EventService.v1_3_0.EventService",
  "Actions": {
    "#EventService.SubmitTestEvent": {
      "target": "/redfish/v1/EventService/Actions/EventService.SubmitTestEvent"
    }
  },
  "DeliveryRetryAttempts": 3,
  "DeliveryRetryIntervalSeconds": 30,
  "EventFormatTypes": [
    "Event"
  ],
  "Id": "EventService",
  "Name": "Event Service",
  "RegistryPrefixes": [
    "Base",
    "OpenBMC"
  ],
  "ServerSentEventUri": "/redfish/v1/EventService/SSE",
  "ServiceEnabled": true,
  "Subscriptions": {
    "@odata.id": "/redfish/v1/EventService/Subscriptions"
  }
}
```

**Subscription Collections**

Example GET output:

```
{
  "@odata.type":"#EventDestinationCollection.EventDestinationCollection",
  "@odata.context":"/redfish/v1/$metadata#EventDestinationCollection.EventDestinationCollection",
  "@odata.id":"/redfish/v1/EventService/Subscriptions"
  "Id":"Event Subscription Collection",
  "Name":"Event Subscriptions Collection",
  "Members@odata.count": 2,
  "Members": [
    {
      "@odata.id":"/redfish/v1/EventService/Subscriptions/1",
      "@odata.id":"/redfish/v1/EventService/Subscriptions/2"
    }
  ],
}
```

**Subscriptions**

Supported properties:

| PROPERTY NAME    | TYPE          | DESCRIPTION                                                                                                                                                   |
| ---------------- | ------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Destination      | String        | The URI of the destination Event Service.                                                                                                                     |
| Context          | String        | A client-supplied string that is stored with the event destination subscription. Default: null                                                                |
| Protocol         | Enum          | The protocol type of the event connection. Default: redfish                                                                                                   |
| HttpHeaders      | array(string) | This is for setting HTTP headers, such as authorization information. Default: null                                                                            |
| RegistryPrefixes | String        | A list of the Prefixes for the Message Registries that contain the MessageIds that will be sent to this event destination. Default: null                      |
| ResourceTypes    | Array(String) | A list of Resource Type values (Schema names) that correspond to the OriginOfCondition. The version and full namespace should not be specified. Default: null |
| EventFormatType  | String        | Indicates the content types of the message that will be sent to the EventDestination. Supported types are 'Event' and 'MetricReport'. Default: Event          |

Example GET Request:

```
{
  "@odata.context": "/redfish/v1/$metadata#EventDestination.EventDestination",
  "@odata.id": "/redfish/v1/EventService/Subscriptions/1",
  "@odata.type": "#EventDestination.v1_5_0.EventDestination",
  "Context": "143TestString",
  "Destination": "http://test3.domain.com/EventListener",
  "EventFormatTypes": [
    "Event"
  ],
  "HttpHeaders": [
    "X-Auth-Token:XYZABCDEDF"
  ],
  "Id": "1",
  "MessageIds": [
    "InventoryAdded",
    "InventoryRemoved"
  ],
  "Name": "EventSubscription 1",
  "Protocol": "Redfish",
  "RegistryPrefixes": [
    "Base",
    "OpenBMC"
  ],
  "SubscriptionType": "Event"
}
```

- Event log formatted response body looks like:

```
id: 1
data:{
data: "@odata.type": "#Event.v1_1_0.Event",
data: "Id": "1",
data: "Name": "Event Array",
data: "Context": "ABCDEFGH",
data: "Events": [
data: {
data:  "Created": "2019-08-13T06:25:31+00:00",
data:  "EntryType": "Event",
data:  "Id": "1565677531",
data:  "Message": "F1UL16RISER3 Board with serial number BQWK64100593 was installed.",
data:  "MessageArgs": [
data:    "F1UL16RISER3",
data:    "Board",
data:    "BQWK64100593"
data:  ],
data:  "MessageId": "OpenBMC.0.1.InventoryAdded",
data:  "Name": "System Event Log Entry",
data:  "Severity": "OK"
data:  "Context": "ABCDEFGH"
data:  }
data: ]
data:}
```

- Configuration structure pseudo code:

```
struct EventSrvConfig
{
    bool enabled;
    uint32_t retryAttempts;
    uint32_t retryTimeoutInterval;
};
```

- Subscription structure pseudo code:

```
struct EventSrvSubscription
{
    std::string destinationUri;
    std::string customText;
    std::string subscriptionType;
    std::string protocol;
    std::vector<std::string> httpHeaders; // key-value pair
    std::vector<std::string> registryMsgIds;
    std::vector<std::string> registryPrefixes;
    std::vector<std::string> eventFormatTypes;
};
```
