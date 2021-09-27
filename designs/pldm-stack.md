# PLDM stack on OpenBMC

Author: Deepak Kodihalli <dkodihal@linux.vnet.ibm.com> <dkodihal>

Created: 2019-01-22

## Problem Description

On OpenBMC, in-band IPMI is currently the primary industry-standard means of
communication between the BMC and the Host firmware. We've started hitting some
inherent limitations of IPMI on OpenPOWER servers: a limited number of sensors,
and a lack of a generic control mechanism (sensors are a generic monitoring
mechanism) are the major ones. There is a need to improve upon the communication
protocol, but at the same time inventing a custom protocol is undesirable.

This design aims to employ Platform Level Data Model (PLDM), a standard
application layer communication protocol defined by the DMTF. PLDM draws inputs
from IPMI, but it overcomes most of the latter's limitations. PLDM is also
designed to run on standard transport protocols, for e.g. MCTP (also designed by
the DMTF). MCTP provides for a common transport layer over several physical
channels, by defining hardware bindings. The solution of PLDM over MCTP also
helps overcome some of the limitations of the hardware channels that IPMI uses.

PLDM's purpose is to enable all sorts of "inside the box communication": BMC -
Host, BMC - BMC, BMC - Network Controller and BMC - Other (for e.g. sensor)
devices.

## Background and References

PLDM is designed to be an effective interface and data model that provides
efficient access to low-level platform inventory, monitoring, control, event,
and data/parameters transfer functions. For example, temperature, voltage, or
fan sensors can have a PLDM representation that can be used to monitor and
control the platform using a set of PLDM messages. PLDM defines data
representations and commands that abstract the platform management hardware.

PLDM groups commands under broader functions, and defines separate
specifications for each of these functions (also called PLDM "Types"). The
currently defined Types (and corresponding specs) are : PLDM base (with
associated IDs and states specs), BIOS, FRU, Platform monitoring and control,
Firmware Update and SMBIOS. All these specifications are available at:

https://www.dmtf.org/standards/pmci

Some of the reasons PLDM sounds promising (some of these are advantages over
IPMI):

- Common in-band communication protocol.

- Already existing PLDM Type specifications that cover the most common
  communication requirements. Up to 64 PLDM Types can be defined (the last one
  is OEM). At the moment, 6 are defined. Each PLDM type can house up to 256 PLDM
  commands.

- PLDM sensors are 2 bytes in length.

- PLDM introduces the concept of effecters - a control mechanism. Both sensors
  and effecters are associated to entities (similar to IPMI, entities can be
  physical or logical), where sensors are a mechanism for monitoring and
  effecters are a mechanism for control. Effecters can be numeric or state
  based. PLDM defines commonly used entities and their IDs, but there 8K slots
  available to define OEM entities.

- A very active PLDM related working group in the DMTF.

The plan is to run PLDM over MCTP. MCTP is defined in a spec of its own, and a
proposal on the MCTP design is in discussion already. There's going to be an
intermediate PLDM over MCTP binding layer, which lets us send PLDM messages over
MCTP. This is defined in a spec of its own, and the design for this binding will
be proposed separately.

## Requirements

How different BMC applications make use of PLDM messages is outside the scope of
this requirements doc. The requirements listed here are related to the PLDM
protocol stack and the request/response model:

- Marshalling and unmarshalling of PLDM messages, defined in various PLDM Type
  specs, must be implemented. This can of course be staged based on the need of
  specific Types and functions. Since this is just encoding and decoding PLDM
  messages, this can be a library that could shared between the BMC, and other
  firmware stacks. The specifics of each PLDM Type (such as FRU table
  structures, sensor PDR structures, etc) are implemented by this lib.

- Mapping PLDM concepts to native OpenBMC concepts must be implemented. For
  e.g.: mapping PLDM sensors to phosphor-hwmon hosted D-Bus objects, mapping
  PLDM FRU data to D-Bus objects hosted by phosphor-inventory-manager, etc. The
  mapping shouldn't be restrictive to D-Bus alone (meaning it shouldn't be
  necessary to put objects on the Bus just to serve PLDM requests, a problem
  that exists with phosphor-host-ipmid today). Essentially these are platform
  specific PLDM message handlers.

- The BMC should be able to act as a PLDM responder as well as a PLDM requester.
  As a PLDM requester, the BMC can monitor/control other devices. As a PLDM
  responder, the BMC can react to PLDM messages directed to it via requesters in
  the platform.

- As a PLDM requester, the BMC must be able to discover other PLDM enabled
  components in the platform.

- As a PLDM requester, the BMC must be able to send simultaneous messages to
  different responders.

- As a PLDM requester, the BMC must be able to handle out of order responses.

- As a PLDM responder, the BMC may simultaneously respond to messages from
  different requesters, but the spec doesn't mandate this. In other words the
  responder could be single-threaded.

- It should be possible to plug-in OEM PLDM types/functions into the PLDM stack.

- As a PLDM sensor monitoring daemon, the BMC must be able to enumerate and
  monitor the static or self-described(with PDRs) PLDM sensors in satellite
  Management Controller, on board device or PCIe add-on card.

## Proposed Design

This document covers the architectural, interface, and design details. It
provides recommendations for implementations, but implementation details are
outside the scope of this document.

The design aims at having a single PLDM daemon serve both the requester and
responder functions, and having transport specific endpoints to communicate on
different channels.

The design enables concurrency aspects of the requester and responder functions,
but the goal is to employ asynchronous IO and event loops, instead of multiple
threads, wherever possible.

The following are high level structural elements of the design:

### PLDM encode/decode libraries

This library would take a PLDM message, decode it and extract the different
fields of the message. Conversely, given a PLDM Type, command code, and the
command's data fields, it would make a PLDM message. The thought is to design
this as a common library, that can be used by the BMC and other firmware stacks,
because it's the encode/decode and protocol piece (and not the handling of a
message).

### PLDM provider libraries

These libraries would implement the platform specific handling of incoming PLDM
requests (basically helping with the PLDM responder implementation, see next
bullet point), so for instance they would query D-Bus objects (or even something
like a JSON file) to fetch platform specific information to respond to the PLDM
message. They would link with the encode/decode lib.

It should be possible to plug-in a provider library, that lets someone add
functionality for new PLDM (standard as well as OEM) Types. The libraries would
implement a "register" API to plug-in handlers for specific PLDM messages.
Something like:

template <typename Handler, typename... args> auto register(uint8_t type,
uint8_t command, Handler handler);

This allows for providing a strongly-typed C++ handler registration scheme. It
would also be possible to validate the parameters passed to the handler at
compile time.

### Request/Response Model

The PLDM daemon links with the encode/decode and provider libs. The daemon would
have to implement the following functions:

#### Receiver/Responder

The receiver wakes up on getting notified of incoming PLDM messages (via D-Bus
signal or callback from the transport layer) from a remote PLDM device. If the
message type is "Request" it would route them to a PLDM provider library. Via
the library, asynchronous D-Bus calls (using sdbusplus-asio) would be made, so
that the receiver can register a handler for the D-Bus response, instead of
having to wait for the D-Bus response. This way it can go back to listening for
incoming PLDM messages.

In the D-Bus response handler, the receiver will send out the PLDM response
message via the transport's send message API. If the transport's send message
API blocks for a considerably long duration, then it would have to be run in a
thread of it's own.

If the incoming PLDM message is of type "Response", then the receiver emits a
D-Bus signal pointing to the response message. Any time the message is too large
to fit in a D-Bus payload, the message is written to a file, and a read-only
file descriptor pointing to that file is contained in the D-Bus signal.

#### Requester

Designing the BMC as a PLDM requester is interesting. We haven't had this with
IPMI, because the BMC was typically an IPMI server. PLDM requester functions
will be spread across multiple OpenBMC applications (instead of a single big
requester app) - based on the responder they're talking to and the high level
function they implement. For example, there could be an app that lets the BMC
upgrade firmware for other devices using PLDM - this would be a generic app in
the sense that the same set of commands might have to be run irrespective of the
device on the other side. There could also be an app that does fan control on a
remote device, based on sensors from that device and algorithms specific to that
device.

##### Proposed requester design

A requester app/flow comprises of the following :

- Linkage with a PLDM encode/decode library, to be able to pack PLDM requests
  and unpack PLDM responses.

- A D-Bus API to generate a unique PLDM instance id. The id needs to be unique
  across all outgoing PLDM messages (from potentially different processes). This
  needs to be on D-Bus because the id needs to be unique across PLDM requester
  app processes.

- A requester client API that provides blocking and non-blocking functions to
  transfer a PLDM request message and to receive the corresponding response
  message, over MCTP (the blocking send() will return a PLDM response). This
  will be a thin wrapper over the socket API provided by the mctp demux daemon.
  This will provide APIs for common tasks so that the same may not be
  re-implemented in each PLDM requester app. This set of API will be built into
  the encode/decode library (so libpldm would house encode/decode APIs, and
  based on a compile time flag, the requester APIs as well). A PLDM requester
  app can choose to not use the client requester APIs, and instead can directly
  talk to the MCTP demux daemon.

##### Proposed requester design - flow diagrams

a) With blocking API

```
+---------------+               +----------------+            +----------------+               +-----------------+
|BMC requester/ |               |PLDM requester  |            |PLDM responder  |               |PLDM Daemon      |
|client app     |               |lib (part of    |            |                |               |                 |
|               |               |libpldm)        |            |                |               |                 |
+-------+-------+               +-------+--------+            +--------+-------+               +---------+-------+
        |                               |                              |                                 |
        |App starts                     |                              |                                 |
        |                               |                              |                                 |
        +------------------------------->setup connection with         |                                 |
        |init(non_block=false)          |MCTP daemon                   |                                 |
        |                               |                              |                                 |
        +<-------+return_code+----------+                              |                                 |
        |                               |                              |                                 |
        |                               |                              |                                 |
        |                               |                              |                                 |
        +------------------------------>+                              |                                 |
        |encode_pldm_cmd(cmd code, args)|                              |                                 |
        |                               |                              |                                 |
        +<----+returns pldm_msg+--------+                              |                                 |
        |                               |                              |                                 |
        |                               |                              |                                 |
        |----------------------------------------------------------------------------------------------->|
        |DBus.getPLDMInstanceId()       |                              |                                 |
        |                               |                              |                                 |
        |<-------------------------returns PLDM instance id----------------------------------------------|
        |                               |                              |                                 |
        +------------------------------>+                              |                                 |
        |send_msg(mctp_eids, pldm_msg)  +----------------------------->+                                 |
        |                               |write msg to MCTP socket      |                                 |
        |                               +----------------------------->+                                 |
        |                               |call blocking recv() on socket|                                 |
        |                               |                              |                                 |
        |                               +<-+returns pldm_response+-----+                                 |
        |                               |                              |                                 |
        |                               +----+                         |                                 |
        |                               |    | verify eids, instance id|                                 |
        |                               +<---+                         |                                 |
        |                               |                              |                                 |
        +<--+returns pldm_response+-----+                              |                                 |
        |                               |                              |                                 |
        |                               |                              |                                 |
        |                               |                              |                                 |
        +------------------------------>+                              |                                 |
        |decode_pldm_cmd(pldm_resp,     |                              |                                 |
        |                output args)   |                              |                                 |
        |                               |                              |                                 |
        +------------------------------>+                              |                                 |
        |close_connection()             |                              |                                 |
        +                               +                              +                                 +
```

b) With non-blocking API

```
+---------------+               +----------------+            +----------------+             +---------------+
|BMC requester/ |               |PLDM requester  |            |PLDM responder  |             |PLDM daemon    |
|client app     |               |lib (part of    |            |                |             |               |
|               |               |libpldm)        |            |                |             |               |
+-------+-------+               +-------+--------+            +--------+-------+             +--------+------+
        |                               |                              |                              |
        |App starts                     |                              |                              |
        |                               |                              |                              |
        +------------------------------->setup connection with         |                              |
        |init(non_block=true            |MCTP daemon                   |                              |
        |     int* o_mctp_fd)           |                              |                              |
        |                               |                              |                              |
        +<-------+return_code+----------+                              |                              |
        |                               |                              |                              |
        |                               |                              |                              |
        |                               |                              |                              |
        +------------------------------>+                              |                              |
        |encode_pldm_cmd(cmd code, args)|                              |                              |
        |                               |                              |                              |
        +<----+returns pldm_msg+--------+                              |                              |
        |                               |                              |                              |
        |-------------------------------------------------------------------------------------------->|
        |DBus.getPLDMInstanceId()       |                              |                              |
        |                               |                              |                              |
        |<-------------------------returns PLDM instance id-------------------------------------------|
        |                               |                              |                              |
        |                               |                              |                              |
        +------------------------------>+                              |                              |
        |send_msg(eids, pldm_msg,       +----------------------------->+                              |
        |         non_block=true)       |write msg to MCTP socket      |                              |
        |                               +<---+return_code+-------------+                              |
        +<-+returns rc, doesn't block+--+                              |                              |
        |                               |                              |                              |
        +------+                        |                              |                              |
        |      |Add EPOLLIN on mctp_fd  |                              |                              |
        |      |to self.event_loop      |                              |                              |
        +<-----+                        |                              |                              |
        |                               +                              |                              |
        +<----------------------+PLDM response msg written to mctp_fd+-+                              |
        |                               +                              |                              |
        +------+EPOLLIN on mctp_fd      |                              |                              |
        |      |received                |                              |                              |
        |      |                        |                              |                              |
        +<-----+                        |                              |                              |
        |                               |                              |                              |
        +------------------------------>+                              |                              |
        |decode_pldm_cmd(pldm_response) |                              |                              |
        |                               |                              |                              |
        +------------------------------>+                              |                              |
        |close_connection()             |                              |                              |
        +                               +                              +                              +
```

##### Alternative to the proposed requester design

a) Define D-Bus interfaces to send and receive PLDM messages :

```
method sendPLDM(uint8 mctp_eid, uint8 msg[])

signal recvPLDM(uint8 mctp_eid, uint8 pldm_instance_id, uint8 msg[])
```

PLDM requester apps can then invoke the above applications. While this
simplifies things for the user, it has two disadvantages :

- the app implementing such an interface could be a single point of failure,
  plus sending messages concurrently would be a challenge.
- the message payload could be large (several pages), and copying the same for
  D-Bus transfers might be undesirable.

### Multiple transport channels

The PLDM daemon might have to talk to remote PLDM devices via different
channels. While a level of abstraction might be provided by MCTP, the PLDM
daemon would have to implement a D-Bus interface to target a specific transport
channel, so that requester apps on the BMC can send messages over that
transport. Also, it should be possible to plug-in platform specific D-Bus
objects that implement an interface to target a platform specific transport.

### Processing PLDM FRU information sent down by the host firmware

Note: while this is specific to the host BMC communication, most of this might
apply to processing PLDM FRU information received from a device connected to the
BMC as well.

The requirement is for the BMC to consume PLDM FRU information received from the
host firmware and then have the same exposed via Redfish. An example can be the
host firmware sending down processor and core information via PLDM FRU commands,
and the BMC making this information available via the Processor and
ProcessorCollection schemas.

This design is built around the pldmd and entity-manager applications on the
BMC:

- The pldmd asks the host firmware's PLDM stack for the host's FRU record table,
  by sending it the PLDM GetFRURecordTable command. The pldmd should send this
  command if the host indicates support for the PLDM FRU spec. The pldmd
  receives a PLDM FRU record table from the host firmware (
  www.dmtf.org/sites/default/files/standards/documents/DSP0257_1.0.0.pdf). The
  daemon parses the FRU record table and hosts raw PLDM FRU information on
  D-Bus. It will house the PLDM FRU properties for a certain FRU under an
  xyz.openbmc_project.Inventory.Source.PLDM.FRU D-Bus interface, and house the
  PLDM entity info extracted from the FRU record set PDR under an
  xyz.openbmc_project.Source.PLDM.Entity interface.

- Configurations can be written for entity-manager to probe an interface like
  xyz.openbmc_project.Inventory.Source.PLDM.FRU, and create FRU inventory D-Bus
  objects. Inventory interfaces from the xyz.openbmc_project. Inventory
  namespace can be applied on these objects, by converting PLDM FRU property
  values into xyz.openbmc_project.Invnetory.Decorator.Asset property values,
  such as Part Number and Serial Number, in the entity manager configuration
  file. Bmcweb can find these FRU inventory objects based on D-Bus interfaces,
  as it does today.

### MCTP endpoint discovery

`pldmd` (PLDM daemon) utilizes the
[MCTP D-Bus interfaces](https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/yaml/xyz/openbmc_project/MCTP)
to enumerate all MCTP endpoints in the system. The MCTP D-Bus interface
implements the `SupportedMessageTypes` to have which Message type supported by
each endpoint. `pldmd` watches the `InterfacesAdded` D-Bus signal from MCTP
D-Bus interface to get the new endpoint EIDs. It also matches the
`InterfaceRemoved` D-Bus signal to find the removed endpoint EIDs from MCTP
D-Bus interface.

### Terminus management and discovery

`pldmd` will maintain a terminus table to manage the PLDM terminus in system.
When `pldmd` received the updated EID table from MCTP D-Bus interface, `pldmd`
should check if the EID support PLDM message type(0x01) and then adds the EID
which is not in the terminus table yet. When the terminus EID is removed from
MCTP D-Bus interface, `pldmd` should also clean up the removed endpoint from the
terminus table.

For each of terminus in the table, `pldmd` will go through the below steps:

- Init terminus
- Discovery terminus
- Terminus monitoring and controlling

All of the added D-Bus object paths and D-Bus interfaces, Monitoring/Controlling
tasks of the terminus will be removed when it is removed from the terminus
table.

#### Terminus initialization

Each terminus in PLDM interface is identified by terminus ID (TID). This TID is
an unique number `TID#`. When a new terminus is added to terminus table, `pldmd`
should send `GetTID` to get the `TID#`. When the received `TID#` is already
existing in TID pool, `pldmd` will call the `SetTID` command to assign a new TID
for the terminus.

Beside the `TID#`, terminus can also have `$TerminusName` or `$DeviceName` which
can be encoded in the Terminus's `Entity Auxiliary Names PDR` (section 28.18 of
DSP0248 1.2.1) or in the MCTP Entity-manager endpoint EID configuration file
[Entity-Manager EID configuration](https://github.com/openbmc/entity-manager/blob/master/configurations/yosemite4_floatingfalls.json#L7).

For terminus which supports the sensors, effecters and state, the EM EID
configuration or Terminus's `Entity Auxiliary Names PDR` is recommended to be
included. When the EM EID configuration is not available, the
`Entity Auxiliary name PDR` should be added so all sensors don't have the
terminus number `TID#` in it anyhow.

#### Teminus Discovery

After the TID assignment steps, `pldmd` should go through `Terminus Discovery`
steps:

- Send `GetPLDMType` and `GetPLDMVersions` commands to the terminus to record
  the supported PLDM type message/version.
- If the terminus supports `GetPDRs` command type, `pldmd` will send that
  command to get the terminus PDRs. Based on the retrieved PDRs, `pldmd` will
  collect:

  - The association between the entities in the system using
    `Entity Association PDR` (section 28.17 of DSP0248 1.2.1).
  - The entity names using `Entity Auxiliary Names PDR` (Section 28.18 of
    DSP0248 1.2.1).
  - The sensor/effecter/state info in the entities of terminus
    sensors/effecter/state PDRs (section 28.4, 28.6, 28.8, 28.11, 28.14, 28.15,
    28.25, etc. of DSP0248 1.2.1).
  - The Fru info using FRU PDRs (section 28.22 of DSP0248 1.2.1).
  - The other info using the others PDRs in section 28.x of DSP0248 1.2.1.

  The above PDRs can also be configured in the Json configuration files. When
  the `PDR configuration` is available, the `pldmd` daemon will bypass `GetPDRs`
  steps and read those files to collect that info. The template of the
  configuration files can follow the current format of
  [PDRs configuration files](https://github.com/openbmc/pldm/tree/master/configurations)

- The `pldmd` then creates the Terminus inventory, sensors, effecters D-Bus
  object paths.
- At the final steps of `terminus discovery`, `pldmd` will send
  `SetEventReceiver` notifies about the readiness of the BMC for the event
  messages from the terminus.

#### Terminus monitoring and controlling

After finishing the discovery steps, the daemon will start monitoring the
sensors, response for the events from terminus and handle the terminus control
action from the user.

### Sensor creating and Monitoring

To find out all sensors from PLDM terminus, `pldmd` should retrieve all the
Sensor PDRs by PDR Repository commands (`GetPDRRepositoryInfo`, `GetPDR`) for
the necessary parameters (e.g., `sensorID#`, `$SensorAuxName`, unit, etc.).
`pldmd` can use libpldm encode/decode APIs
(`encode_get_pdr_repository_info_req()`,
`decode_get_pdr_repository_info_resp()`, `encode_get_pdr_req()`,
`decode_get_pdr_resp()`) to build the commands message and then sends it to PLDM
terminus.

Regarding to the static device described in section 8.3.1 of DSP0248 1.2.1, the
device uses PLDM for access only and doesn't support PDRs. The PDRs for the
device needs to be encoded by Platform specific PDR JSON file by the platform
developer. `pldmd` will generate these sensor PDRs encoded by JSON files and
parse them as the same as the PDRs fetched by PLDM terminus.

`pldmd` should expose the found PLDM sensor to D-Bus object path
`/xyz/openbmc*project/sensors/<sensor_type>/SensorName`. The format of
`sensorName` can be `$TerminusName_$SensorAuxName` or `$TerminusName_SensorID#`.
`$SensorAuxName` will be included in the `sensorName` whenever they exist. For
exposing sensor status to D-Bus, `pldmd` should implement following D-Bus
interfaces to the D-Bus object path of PLDM sensor.

- [xyz.openbmc_project.Sensor.Value](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Sensor/Value.interface.yaml),
  the interface exposes the sensor reading unit, value, Max/Min Value.

- [xyz.openbmc_project.State.Decorator.OperationalStatus](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/Decorator/OperationalStatus.interface.yaml),
  the interface exposes the sensor status which is functional or not.

After doing the discovery of PLDM sensors, `pldmd` should initialize all found
sensors by necessary commands (e.g., `SetNumericSensorEnable`,
`SetSensorThresholds`, `SetSensorHysteresis and `InitNumericSensor`) and then
start to update the sensor status to D-bus objects by polling or async event
method depending on the capability of PLDM terminus.

`pldmd` should update the value property of `Sensor.Value` D-Bus interface after
getting the response of `GetSensorReading` command successfully. If `pldmd`
failed to get the response from PLDM terminus or the completion code returned by
PLDM terminus is not `PLDM_SUCCESS`, the Functional property of
`State.Decorator.OperationalStatus` D-Bus interface should be updated to false.

#### Polling v.s. Async method

For each terminus, `pldmd` maintains a list to poll the Terminus' sensors and
exposes the status to D-Bus. `pldmd` has a polling timer with the configurable
interval to update the PLDM sensors of the terminus periodically. The PLDM
sensor in list has a `updateTime` which is initialized to the value of the
defined `updateInterval` in sensor PDRs. Upon the polling timer timeout, the
terminus' sensors will be read using `GetSensorReading` command. The read
condition is the `elapsed time` from the `last read timestamp` to
`current timestamp` is more than the sensor's `updateTime`. `pldmd` should have
APIs to be paused and resumed by other task (e.g. pausing sensor polling during
firmware updating to maximum bandwidth).

To enable async event method for a sensor to update its status to `pldmd`,
`pldmd` needs to implement the responder of `PlatformEventMessage` command
described in 13.1 PLDM Event Message of
[DSP0248 1.2.1](https://www.dmtf.org/sites/default/files/standards/documents/DSP0248_1.2.1.pdf).
`pldmd` checks the response of `EventMessageSupported` command from PLDM
terminus to identify if it can generate events. A PLDM sensor can work in event
aync method if the `updateInterval` of all sensors in the same PLDM terminus are
longer than final polling time. Before `pldmd` starts to receive async event
from PLDM terminus, `pldmd` should remove the sensor from poll list and then
send necessary commands (e.g., `EventMessageBufferSize` and `SetEventReceiver`)
to PLDM terminus for the initialization.

## Alternatives Considered

Continue using IPMI, but start making more use of OEM extensions to suit the
requirements of new platforms. However, given that the IPMI standard is no
longer under active development, we would likely end up with a large amount of
platform-specific customisations. This also does not solve the hardware channel
issues in a standard manner. On OpenPOWER hardware at least, we've started to
hit some of the limitations of IPMI (for example, we have need for >255
sensors).

## Impacts

Development would be required to implement the PLDM protocol, the
request/response model, and platform specific handling. Low level design is
required to implement the protocol specifics of each of the PLDM Types. Such low
level design is not included in this proposal.

Design and development needs to involve the firmware stacks of management
controllers and management devices of a platform management subsystem.

## Testing

Testing can be done without having to depend on the underlying transport layer.

The responder function can be tested by mocking a requester and the transport
layer: this would essentially test the protocol handling and platform specific
handling. The requester function can be tested by mocking a responder: this
would test the instance id handling and the send/receive functions.

APIs from the shared libraries can be tested via fuzzing.

The APIs to parse PDRs from PLDM terminus can be tested by a mocking responder.
A sample JSON file is provided to test the APIs for mocking PDRs for static PLDM
sensors.
