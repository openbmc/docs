# Redfish EventService design

Author: AppaRao Puli

Primary assignee: AppaRao Puli !apuli

Other contributors: None

Created: 2019-07-24

## Problem description
Redfish in OpenBMC currently supports sending response to the clients
which requests for data. Currently there is no service to send asynchronous
message to the client about any events, error or state updates.

The goal of this design document is to give resource overview of
redfish event service and its implementation details in OpenBMC.

## Background and references

 - [Redfish specification](https://www.dmtf.org/sites/default/files/standards/documents/DSP0266_1.7.0.pdf)
 - [DMTF Redfish school events](https://www.dmtf.org/sites/default/files/Redfish_School-Events_2018.2.pdf)
 - [DMTF Mockups](https://redfish.dmtf.org/redfish/v1)

## Requirements

High level requirements:

  - BMC should support send asynchronous event notification to external
    subscribed clients over network using redfish(http/https) protocol,
    instead of regular querying mechanism.
  - BMC must provide mechanism to the users to configure event service
    with specific filters and accordingly send the event notifications
    to all the registered clients.
  - BMC must preserve the event service configuration across reboots, and
    must reset the same, when factory defaults applied.
  - BMC should have provision to disable the EventService.
  - The BMC Redfish interface shall support the EventService schema.
  - Must provide functionality to send out test events.
  - Must allow only users having 'ConfigureUser' to create, delete or updated
    event service configuration, subscriptions and restrict other users. Must
    allow users with 'Login' privileges to view the EventService configuration
    and subscription details.
  - EventService should be specific to Redfish Interface alone.
    i.e No other service can add/modify/delete configuration or subscriptions.
  - Either the client or the service can terminate the event stream at any time
    by deleting the subscription. The service may delete a subscription if the
    number of delivery errors exceeds preconfigured thresholds.
  - For Push style event subscription creation, BMC shall respond to a
    successful subscription with HTTP status 201 and set the HTTP Location
    header to the address of a new subscription resource. Subscriptions are
    persistent and shall remain across event service restarts.
  - For Push style event subscription creation, BMC shall respond to client with
    http status as
    - 400: If parameter in body is not supported.
    - 404: If request body contains both RegistryPrefixes and MessageIds.
  - SSE: BMC shall respond to GET method on URI specific in "ServerSentEventUri"
    in EventService schema with 201(created) along with subscription information.
    BMC shall respond with https status code 400, if filter parameters are not
    supported or 404 if filter parameter contains invalid data.
  - Clients shall terminate a subscription by sending an HTTP DELETE message to
    the URI of the subscription resource.
  - BMC may terminate a subscription by sending a special "subscription
    terminated" event as the last message. Future requests to the associated
    subscription resource will respond with HTTP status code 404.
  - BMC shall support maximum of 20 ( Max 10 SSE connections) subscriptions for
    security reasons( To avoid HTTP connection limit exhaust).
  - BMC should log an event message when new subscription is added.

## Proposed design

Redfish event mechanism provides way to asynchronously to send state or
error messages to the subscribed services. Redfish schema already covers
messages, events and event service mechanism, including security.

Two styles of eventing are supported OpenBMC EventService.

  1. Push style eventing (HTTP): When the service detects the need to send an
     event, it uses an HTTP POST to push the event message to the client.
     Clients can enable reception of events by creating a subscription entry
     in the EventService.

  2. Server-Sent Events (SSE):  Client opens an SSE connection to the service
     by performing a GET on the URI specified by the "ServerSentEventUri" in
     the Event Service.

This document majorly focus on OpenBMC implementation point of view. For more
detailed eventing information can be found in ["Redfish Specification"](
https://www.dmtf.org/sites/default/files/standards/documents/DSP0266_1.7.0.pdf)
sections:-

  - Eventing (Section 12.1)
  - Server Sent-Eventing ( Section 12.5)
  - Security ( Section 12 & 13.2.8)


**BLOCK DIGRAM**
```
+----------------------------------------------------------------------------+
|                                 CLIENT                                     |
|                             EVENT LISTENER                                 |
|               WEBSERVICE                         SSECLIENT                 |
+--------------------^-----------------------------+------^------------------+
     PUSH style(HTTP)|           NETWORK        GET|      | PUSH
                     |                             |  SSE |
+----------------------------------------------------------------------------+
|                    |         REDFISH/BMCWEB      |      |                  |
|     +--------------+-----------------------------v------+---------+        |
|     |                          EVENT MANAGER                      |        |
|     |          +-----------+   +------------+  +----------------+ | inotify|    +---------+
|     |          |FORMATTER  |   |EVENT SENDER|  |EVENT MONITOR   | +-------------+ REDFISH |
|     |          +-----------+   +------------+  +----------------+ |        |    | EVENT   |
|     +-------------------------------------------------------------+        |    | LOGS    |
|     +------------+        |        +----------------+                      |    +----+----+
|     |EventService|        |      + |  SUBSCRIPTIONS |                      |         |
|     |            |        |    + | +----------------+                      |         |
|     +------------+        |    | +----------------+                        |    +----+----+
|                           |    +----------------+                          |    |         |
|     +---------------------+---------------------------------------------+  |    | RSYSLOG |
|     |               REDFISH EVENT CONFIG(volatile)                      |  |    |         |
|     +-----------------------------+-------------------------------------+  |    +----+----+
|                                   |                                        |         |
+----------------------------------------------------------------------------+         |
                                    |                                             +----+----+
+-----------------------------------+----------------------------------------+    |         |
|                                                                            |    |JOURNAL  |
|                   Persistent Store(json file)                              |    |CONTROL  |
|                                                                            |    |LOGS     |
+----------------------------------------------------------------------------+    +---------+
```


As explained in block diagram, there are majorly 4 blocks.

  1. Redfish resource schemas
  2. Redfish event config
  4. Event manager

### Redfish resource schema's

Redfish will have three major resources to support EventService.

  1. Event Service
  2. Subscription Collections
  3. Subscriptions

All the configuration and subscriptions information data will be cached on
bmcweb webserver and synced with config json file for restoring and persisting
the data.

**EventService**

  Contains the attributes of the service such as Service Enabled, status,
  delivery retry attempts, Event Types, Subscriptions, Actions URI etc.

   - URI : `/redfish/v1/EventService`
   - SCHEMA : [EventService](https://redfish.dmtf.org/schemas/v1/EventService_v1.xml)
   - METHODS : `GET, PATCH`


Supported properties:

  - ServiceEnabled (Boolean): This indicates whether this service is enabled.
    default: True

  - DeliveryRetryAttempts (Integer): This is the number of attempts an event
    posting is retried before the subscription is terminated.
    Default: 3

  - DeliveryRetryIntervalSeconds (Integer in seconds): This represents the
    number of seconds between retry attempts for sending any given Event.
    Default: 30

  - EventFormatTypes (Array(string(enum)): Indicates the content types of the
    message that this service can send to the event destination.
      - Event: Resource types of 'Event' sent to subscribed destination.
        This is default value if nothing is specified.
      - MetricReport: Resource types of 'MetricReport' only sent to subscribed
        destination.

  - RegistryPrefixes (Array(string)): A list of the Prefixes of the Message
    Registries that can be used for the RegistryPrefix property on a
    subscription. If this property is absent or contains an empty array, the
    service does not support RegistryPrefix-based subscriptions.

  - ResourceTypes ( Array(string)): The value of this property shall specify
    an array of the valid @odata.type values that can be used for an Event
    Subscription.

  - ServerSentEventUri (String): The value of this property shall be a URI
    that specifies an HTML5 Server-Sent Event conformant endpoint.
    URi: /redfish/v1/EventService/SSE

  - Status (Object): This property shall contain any status or health properties
    of the resource.

  - Subscriptions (Object): The value of this property shall contain the link
    to a collection of type EventDestinationCollection.

  - Actions: This action shall add a test event to the event service with the
    event data specified in the action parameters. Mote details on TestEvents
    captured below.


Note: EventService.v1_3_0 was created to deprecate the EventTypesForSubscription
      and SSEFilterPropertiesSupported\EventType properties.


Example GET Response:
```
{
  "@odata.type": "#EventService.v1_2_0.EventService",
  "Id": "EventService",
  "Name": "Event Service",
  "Status": { "State": "Enabled", "Health": "OK" },
  "ServiceEnabled": true,
  "DeliveryRetryAttempts": 3,
  "DeliveryRetryIntervalSeconds": 60,
  "ServerSentEventUri": "/redfish/v1/EventService/SSE",
  "Subscriptions": {
    "@odata.id": "/redfish/v1/EventService/Subscriptions"
  },
  "Actions": {
    "#EventService.SubmitTestEvent": {
      "target": "/redfish/v1/EventService/Actions/EventService.SubmitTestEvent",
  },
  "@odata.context": "/redfish/v1/$metadata#EventService.EventService",
  "@odata.id": "/redfish/v1/EventService"
}
```


**Subscription Collections**
  This resource used get collection of all subscriptions. Using this users
  can enroll for automatic events by add new subscription details.

   - URI : `/redfish/v1/EventService/Subscriptions`
   - SCHEMA : [EventDestinationCollection](https://redfish.dmtf.org/schemas/v1/EventDestinationCollection_v1.xml)
   - METHODS : `GET, POST`

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
  This resource contains the subscription details. Used to get each individual
  get subscription information, update & Delete existing subscription
  information.

   - URI : `/redfish/v1/EventService/Subscriptions/<ID>`
   - SCHEMA : [EventDestination](https://redfish.dmtf.org/schemas/v1/EventDestination_v1.xml)
   - METHODS : `GET, PATCH, DELETE`

Supported properties:

  - Destination (String): The URI of the destination Event Service.

  - Context (String): A client-supplied string that is stored with the event
    destination subscription.
    Default: null

  - Protocol (Enum): The protocol type of the event connection.
    Default: redfish

  - HttpHeaders (array(string)): This is for setting HTTP headers, such as
    authorization information.
    Default: null

  - RegistryPrefixes (String): A list of the Prefixes for the Message Registries
    that contain the MessageIds that will be sent to this event destination.
    Default: null

  - ResourceTypes (Array(String)):  A list of Resource Type values (Schema
    names) that correspond to the OriginOfCondition. The version and full
    namespace should not be specified.
    Default: null

  - EventFormatType (String): Indicates the content types of the message that
    will be sent to the EventDestination. Supported types are 'Event' and
    'MetricReport'.
    Default: Event

Example GET Request:
```
{
  "@odata.type":"#EventDestination.v1_4_0.EventDestination",
  "@odata.context":"/redfish/v1/$metadata#EventDestination.EventDestination",
  "@odata.id":"/redfish/v1/EventService/Subscriptions/1"
  "Id":"1",
  "Name":"EventSubscription 1",
  "Destination":"http://www.dnsname.com/Destination1",
  "Context":"CustomText",
  "Protocol":"Redfish",
  "EventFormatType": "Event",
  "RegistryPrefix": "OpenBMC.0.1"
}
```

**TestEvents**
  This action shall add a test event to the event service with the event
  data specified in the action parameters. This message should then be sent
  to any appropriate ListenerDestination targets.

   - URI : `/redfish/v1/EventService/Actions/EventService.SubmitTestEvent`
   - SCHEMA : [EventService](https://redfish.dmtf.org/schemas/v1/EventService_v1.xml)
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

### Server-Sent Events (SSE):

Server-Sent Events (SSE) is a server push technology enabling a browser to
receive automatic updates from a server via HTTP connection. The Server-Sent
Events EventSource API is standardized as part of HTML5 by the W3C.

Server-Sent Events is a standard describing how servers can initiate data
transmission towards clients once an initial client connection has been
established. They are commonly used to send message updates or continuous
data streams to a browser client and designed to enhance native,
cross-browser streaming through a JavaScript API called EventSource, through
which a client requests a particular URL in order to receive an event stream.

OpenBMC implementation of the "EventService" resource contain a property called
"ServerSentEventUri". If a client performs a GET on the URI specified by the
"ServerSentEventUri", the bmcweb server keep the connection open and conform to
the HTML5 Specification until the client closes the socket. The bmcweb service
should sent the response to GET URI with following headers.

```
'Content-Type': 'text/event-stream',
'Cache-Control': 'no-cache',
'Connection': 'keep-alive'
```

Events generated by the OpenBMC will be sent to the clients using the open
connection. Response of event message should contain the "data: " prefix and
postfix "\n" on each line. For representing the end of data, it should end
with "\n\n". Example sending json response should look like
```
data: {\n
data: "msg": "hello world",\n
data: "id": 12345\n
data: }\n\n
```

When a client opens an SSE stream for the EventService, the bmcweb create an
EventDestination instance in the Subscriptions collection for the EventService
to represent the connection. The "Context" property in the EventDestination
resource shall be an opaque string generated by the service. The bmcweb can
delete the corresponding EventDestination instance when the connection is
closed. The bmcweb shall close the connection if the corresponding
EventDestination is deleted.

Closing connection: Either the client or the service can terminate the event
stream at any time. Client can send DELETE on subscribed id URI for closing the
connection so that bmcweb clear the subscription entry in its store.

Since SSE is uni-directional, if client closes connection/browser abruptly,
bmcweb service doesn't know the client connection state. This is limitation on
SSE connection. In such scenario bmcweb service may delete a subscription, if the
number of delivery errors exceeds preconfigured thresholds.

When server want to close the connection, server can send the event to client
with 'event: close'. Which will notify the client about connection closure on
server side.

Note: The SSE subscription data is not persistent in nature. When bmcweb
restarts or bmc reboots, client should re-establish the SSE connection to bmcweb
to listen for events.

The service should support using the $filter query parameter provided in the
URI for the SSE stream by the client to reduce the amount of data returned
to the client.

Supported filters are:

  - RegistryPrefix
  - ResourceType
  - EventFormatType
  - MessageId
  - OriginResource

Example for subscribing SSE with filters:
```
METHOD: GET
URi: https://<BMCIP>/redfish/v1/EventService/SSE?MessageId='Alert.1.0.LanDisconnect'
```


### Redfish event config cache

Redfish event config cache  is used to store the all configuration
related to EventService and subscriptions information. This does
below operations:

  - During bmcweb service start, it gets all the configuration and
    subscriptions information from config json file and populates the
    cache data.

  - Redfish EventService resources will get/update this event cache for any
    user activities via redfish(Like Get/Add/Update/Delete subscriptions etc.)

  - It will sync the "event cache data" with configuration Json file for
    persisting the information when there is cache update.
    __Note__: Configuration json file will be updated only for Push style event
    subscription information. SSE events related subscription information
    is volatile and will not get updated to json file.

  - It will be used for getting all the subscription information to the
    "Event manager" for filtering event data.


EventService configuration and subscription information need to be cached
across the reboots. The bmcweb service will update the config cache data
including EventService configuration and subscription information to the
json file(location: /var/lib/bmcweb/eventservice_data.json).
This will be persistent across the bmcweb service restarts or bmc reboots
and loads this while initializing bmcweb EventService class.

Note:

  1. SSE Stream http connection can't be reestablished when bmcweb service
     or BMC goes for reboot. Client will receive the close connection event.
     Client can re-subscribe for SSE events using
     "ServerSentEventUri" once bmcweb becomes functional after bmcweb
     restart or BMC reboot scenarios.

  2. All the subscription information related to SSE will not be persistent.


Cache implementation includes below structures and classes under
namespace `phosphor::redfish::eventService`.

  - Configuration structure

pseudo code:
```
struct Configuration
{
    bool enabled;
    uint32_t retryAttempts;
    uint32_t retryTimeoutInterval;
    std::vector<std::string> eventFormatTypes;
    std::vector<std::string> RegistryPrefixes;
    std::vector<std::string> resourceTypes;
    std::string sseUri;
};
```

  - Subscription structure

pseudo code:
```
struct Subscription
{
    std::string destinationUri;
    std::vector<std::string> resourceTypes;
    std::string customText;
    std::string protocol;
    std::map<std::string, std::string> httpHeaders; // key-value pair
};
```

ResourceTypes here is associated with MessageId's from Events registry
entries. Please refer all supported EventTypes in Message registry.
[Link](https://github.com/openbmc/bmcweb/tree/master/redfish-core/include/registries)


  - EventServiceCache Class

This class holds the map of all subscriptions with unique key and methods
to Create/Delete/update subscription information. It also holds the
high level configuration information associated with EventService Schema.


### Redfish Event Logs
This is existing implementation. In OpenBMC, All events will be logged to
Journal control logs. Logging messages is done by providing a Redfish
MessageId and any corresponding MessageArgs.

RSyslog service, monitors event logs associated with "REDFISH_MESSAGE_ID" and
write the required information to "/var/log/redfish" for persisting the
event logs.

bmcweb will search the specified Message Registry for the MessageKey,
construct the final message using the MessageArgs, and format the
event log.

Refer [link](https://github.com/openbmc/docs/blob/master/redfish-logging-in-bmcweb.md)
for more details on logging.

### Event Manager

Event Manager is main block which does monitor the events, format the request
body and send it to the subscribed destinations.

**Event Monitor**

In OpenBMC, all the redfish event logs are currently logged to persistent area
of BMC filesystem("/var/log/redfish"). The "Event monitor" service under
"Event Manager" will use **inotify** to watch on Redfish Event log file to
identify the new events. Once it identifies the new events, it will read last
logged event information from redfish event file to pass on to formatter.

Presently it monitors the lives events for sending asynchronous events.
It will not send the events logged before bmcweb service start.

**Formatter**

Event formatter is used to format the Request body depending on subscriptions
and logged event information. It will fetch all the subscription information
from "Redfish Event Cache" and filter the events by checking severity, event
type against the user subscribed information. It will format the http
"request body" along with CustomText, HttpHeaders and send the Events over
network using http protocol for the filtered subscribers.

Example request body looks like:
```
id: 1
data:{
data: "@odata.type": "#Event.v1_1_0.Event",
data: "Id": "1",
data: "Name": "Event Array",
data: "Context": "ABCDEFGH",
data: "Events": [
data: {
data: "MemberId": "1",
data: "EventType": "Alert",
data: "EventId": "ABC132489713478812346",
data: "Severity": "Warning",
data: "EventTimestamp": "2017-11-23T17:17:42-0600",
data: "Message": "The LAN has been disconnected",
data: "MessageId": "Alert.1.0.LanDisconnect",
data: "MessageArgs": [
data: "EthernetInterface 1",
data: "/redfish/v1/Systems/1"
data: ],
data: "OriginOfCondition": {
data: "@odata.id": "/redfish/v1/Systems/1/EthernetInterfaces/1"
data: },
data: "Context": "ABCDEFGH"
data: }
data: ]
data:}
```

**EventSender**

The "Event Sender" is used to send the alerts to filtered subscribers.

Beast is a C++ header-only library serving as a foundation for writing
interoperable networking libraries by providing low-level HTTP/1, WebSocket,
and networking protocol vocabulary types and algorithms using the consistent
asynchronous model of Boost.Asio.
For more information on beast, refer
[link](https://www.boost.org/doc/libs/develop/libs/beast/doc/html/beast/introduction.html)

  1. PUSH style eventing: BMC uses the boost:beast for asynchronously sending
     events to the subscribed clients. Here is the Beast client Examples for
     sending the synchronous and asynchronous http requests.
[link](https://www.boost.org/doc/libs/develop/libs/beast/example/http/client/)

  2. Server-Sent Events (SSE): The EventDestination resource of the
     subscription data shall contains the opaque of opened client
     SSE stream connection. After receiving the formatted event associated
     with specific subscription information, EventSender writes the formatted
     event data on open SSE connection with specific ID so that client
     can receive the event.

## Alternatives considered

**HTTP POLLING**

Polling is a traditional technique where client repeatedly polls a server for
data. The client makes a request and waits for the server to respond with data.
BMC provides a way to pool the event logs(using Events schema). Client can
run continuously http GET on Events LogService and get the all the events from
BMC. But in HTTP protocol fetching data revolves around request/response which
is greater HTTP overhead.

Push style(http) eventing or SSE will avoid the http overhead of continuous
polling and send the data/events when it happens on server side to the
subscribed clients.

**WEB SOCKET**

WebSockets provides a richer protocol to perform bi-directional, full-duplex
communication. Having a two-way channel is more attractive for some case but in
some scenarios data doesn't need to be sent from the client( Client just need
updates from server errors/events when it happens.)

SSE provides uni-directional connection and operates over traditional HTTP. SSE
do not require a special protocol or server implementation to get working.
WebSockets on the other hand, require full-duplex connections and new Web Socket
servers to handle the protocol.

**IPMI PEF**

Platform Event Filtering (PEF) provides a mechanism for configuring the BMC
to take selected actions on event messages that it receives. This mechanism
is IPMI centric which has its own limitation in terms of security, scalability
etc. when compared with Redfish.

## Impacts

Security issue with Push style eventing:

Push style eventing works on HTTP protocol which is not secured and is
susceptible to man-in-the-middle attacks and eavesdropping which may lead to
virus injections and leak sensitive information to attackers.
The push style event subscription or changes to the BMC will be done over the
secure HTTPS channel with full authentication and authorization. So BMC side
security is NOT compromised. Event Listener on client may get hacked event data
incase of man-in-the-middle attacks. To avoid such attacks, it is recommended to
use SSE event mechanism instead of push style eventing.

SSE works on HTTPS protocol which is encrypted and secured. SSE EventService
subscriptions can be created or updated over HTTPS protocol along with proper
authentication and authorization on BMC users. Please refer security section in
EPS for more information.

SSE connections:

When client opens the SSE HTTPS connection, it will be on open state until
client or server closes the connection. On Linux, there is a limit on each file
descriptor( Of course this is very huge number > 8K). Multiple open http
connections on bmcweb can slow down the performance.

Since open SSE client connection is proxied under multiple level of BMC side
protection(like Authentication is required, User should have proper privilege
level to establish the SSE connection etc.), these impacts are mitigated. Also
BMC will have support for maximum of 10 SSE subscriptions which will limit the
number of http connections exhaust scenario.


## Testing

User should cover below functionalists.

 - View and update EventService configuration using GET and PATCH methods and
   validate the functionality.
 - Subscribe for Events (Both "Push style event" and "SSE") as described below
   and validate http status code for positive and negative use cases (Body
   parameter invalid, Invalid request, improper privilege - forbidden etc.).
 - Validate all subscription collections and each individual subscriptions.
 - Using delete method on subscription URI specific to ID and see connection
   closing properly on client side and subscription data clear on server side.
 - Enable and disable EventService, Generate events and validate whether Events
   are sending properly to all subscriptions when its enabled and stopped
   sending when disabled.
 - Use SubmitTestEvent for generating Test Events. Also cover negative cases
   with invalid body data.
 - Validate the functionality of retry attempts and retry timeout with both
   positive and negative cases.
 - Client connection close: Close client connection which are listening for SSE
   and check server side connection closes after configured retry attempts and
   clears the subscription information.
 - Reboot BMC or reset bmcweb service and check all "push style events"
   subscription is preserved and functional by generating and checking events.
 - Reboot BMC or reset bmcweb service and check all "SSE" subscription data
   is not preserved and client side connection is closed properly.
 - Create user with multiple privileges and check create/delete/update
   subscription and configuration data is performed only for user having
   'ConfiguredUser' privilege and view EventService information works only for
   'Login'.
 - Create new subscription and check is the event message logged in redfish
   event logs or not.
 - Validate the authenticity of events sent to client on subscription with proper
   filtered data( Ex: If RegisteryPrefixes is set to "OpenBMC.0.1", it should
   not send events with prefixes "OpenBMC.0.2") should be validated.
 - For security, validate GET on URI specified in "ServerSentEventUri" with
   un-authorized users and see it responds with properly http status code and
   should not create any subscription or open any http connection.
 - Max limit check: Creates max supported event subscriptions and validate.


** Push style events: **

  1) EventListner: Client should configure the web service for listening the
     events(Example: http://192.168.1.2/Events).

  2) Subscription:
     Client can subscribe for events on BMC using below
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
     On successful event subscription client will receive 201( Created) http
     status code along with created subscription ID. At this point onwards,
     client will receive events associated with subscribed data.

     Client can use "SubmitTestEvent" (Documented above) to test the working
     state.

  3) ViewSubscription: Client can get the subscriptions details using
```
URI: /redfish/v1/EventService/Subscriptions/<ID>
METHOD: GET
```

  4) UpdateSubscription: At any point client can update the subscription
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

  5) DeleteSubscription: Client can unsubscribe and stop receiving events
     by deleting the subscription.
```
URI: /redfish/v1/EventService/Subscriptions/<ID>
METHOD: DELETE
```

** Server-Sent events subscription: **

  1) EventListner and Subscription: Client can open latest browser (chrome)
     and fire GET URI on "ServerSentEventUri" along with filters.
```
URI: /redfish/v1/EventService/SSE?RegistryPrefix='OpenBMC.0.2'&MessageId='OpenBMC.0.2.xyz"
METHOD: GET
```
     On successful event subscription client will receive 200(Created) http
     status code, subscription id along with below response headers for streaming.
```
'Content-Type': 'text/event-stream',
'Cache-Control': 'no-cache',
'Connection': 'keep-alive'
```
     At this point onwards, client browser will receive events associated with
     subscribed data.

     Client can use "SubmitTestEvent" (Documented above) to test the working
     state.

  2) ViewSubscription: Client can get the subscriptions details using
```
URI: /redfish/v1/EventService/Subscriptions/<ID>
METHOD: GET
```

  3) UpdateSubscription: At any point client can update the subscription
     information.
     Note Client can't change the destination as that is opaque associated with
     opened SSE connection.
```
URI: /redfish/v1/EventService/Subscriptions/<ID>
METHOD: PATCH
REQUEST BODY:
{
  "Context":"Changed Custom Text",
  "RegistryPrefix": "OpenBMC.0.2"
}
```

  4) DeleteSubscription: There are two ways to close the connection from client.
     - Client can close the browser directly which will close the SSE http
       connection and so bmcweb service can close and delete the subscription
       data.
     - Client can also unsubscribe and stop receiving events
       by deleting the subscription.
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

Client can opens an SSE connection to the service by performing a GET
on the URI specified by the "ServerSentEventUri" in the Event Service
along with filters. Browser automatically prompt for credentials and
upon successful completion of authentication and authorization, client
connection will be established for listening events.

You can also refer below link for details on Server-Sent events.
(https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events)

Another simple python script for testing SSE using EventSource.
Run it using "python sse-client.py".
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
