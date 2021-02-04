# PLDM stack on OpenBMC

Author: Deepak Kodihalli <dkodihal@linux.vnet.ibm.com> <dkodihal>

Primary assignee: Deepak Kodihalli

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

PLDM groups commands under broader functions, and defines
separate specifications for each of these functions (also called PLDM "Types").
The currently defined Types (and corresponding specs) are : PLDM base (with
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
How different BMC applications make use of PLDM messages is outside the scope
of this requirements doc. The requirements listed here are related to the PLDM
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

## Proposed Design
This document covers the architectural, interface, and design details. It
provides recommendations for implementations, but implementation details are
outside the scope of this document.

The design aims at having a single PLDM daemon serve both the requester and
responder functions, and having transport specific endpoints to communicate
on different channels.

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

template <typename Handler, typename... args>
auto register(uint8_t type, uint8_t command, Handler handler);

This allows for providing a strongly-typed C++ handler registration scheme. It
would also be possible to validate the parameters passed to the handler at
compile time.

### Request/Response Model

The PLDM daemon links with the encode/decode and provider libs. The daemon
would have to implement the following functions:

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
D-Bus signal pointing to the response message. Any time the message is too
large to fit in a D-Bus payload, the message is written to a file, and a
read-only file descriptor pointing to that file is contained in the D-Bus
signal.

#### Requester
Designing the BMC as a PLDM requester is interesting. We haven't had this with
IPMI, because the BMC was typically an IPMI server. PLDM requester functions
will be spread across multiple OpenBMC applications (instead of a single big
requester app) - based on the responder they're talking to and the high level
function they implement. For example, there could be an app that lets the BMC
upgrade firmware for other devices using PLDM - this would be a generic app
in the sense that the same set of commands might have to be run irrespective
of the device on the other side. There could also be an app that does fan
control on a remote device, based on sensors from that device and algorithms
specific to that device.

##### Proposed requester design

A requester app/flow comprises of the following :

- Linkage with a PLDM encode/decode library, to be able to pack PLDM requests
  and unpack PLDM responses.

- A D-Bus API to generate a unique PLDM instance id. The id needs to be unique
  across all outgoing PLDM messages (from potentially different processes).
  This needs to be on D-Bus because the id needs to be unique across PLDM
  requester app processes.

- A requester client API that provides blocking and non-blocking functions to
  transfer a PLDM request message and to receive the corresponding response
  message, over MCTP (the blocking send() will return a PLDM response).
  This will be a thin wrapper over the socket API provided by the mctp demux
  daemon. This will provide APIs for common tasks so that the same may not
  be re-implemented in each PLDM requester app. This set of API will be built
  into the encode/decode library (so libpldm would house encode/decode APIs, and
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
daemon would have to implement a D-Bus interface to target a specific
transport channel, so that requester apps on the BMC can send messages over
that transport. Also, it should be possible to plug-in platform specific D-Bus
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

### PLDM firmware update
The primary goals of the proposed design are:
- Enable firmware update of PLDM devices via the PLDM firmware update
specification, DSP0267. Specifically, the BMC's PLDM service acts as a
User Agent (UA) and can update firmware of a connected firmware device (FD)
that supports DSP0267.
- Enable the use of Redfish to target an FD for PLDM-based firmware update.
- Avoid Redfish specifics in the PLDM design and vice-versa. This is made
possible by basing the design on the existing D-Bus APIs defined in the
xyz.openbmc_project.Software namespace.
- While details below reference MCTP, the design is contained within the
boundary of PLDM and hence applies to PLDM running on MCTP, NC-SI and other
transports.

Following are non-goals and TODO items:
- This section doesn't cover PLDM/MCTP discovery. The same will/is being worked
on via other design update commits.
- This section will not get into the minute details of the PLDM firmware update
protocol. For that, refer DSP0267.
- Firmware Device Proxies (FDP) from DSP2067 are yet to be incorporated in the
design.
- The design for the BMC acting as an FD (for eg when it's connected to another
BMC acting as a UA) is not yet covered in this section.

The following diagram and associated text describes the main elements of the
proposed design, by walking through the PLDM firmware update flow. In terms of
the terminology defined in the xyz/openbmc_project/Software namespace,
this design expects the PLDM service to act both as the *ImageManager* and the
*ItemUpdater*.

                     **BMC**
                     +--------------------------------------------------------------------------------------------+
                     |                                                                                            |
                     |                                                                                            |
**Redfish Client**   |        bmcweb                             pldmd(UA)                      mctpd             |         **PLDM device(FD)**
      +              |           +                                 +                              +               |                 +
      |              |           |                                 |                              |     1) Find MCTP endpoints      |
      |              |           |                                 |                              +--------------------------------->
      |              |           |                                 |                              <---------------------------------+
      |              |           |                                 |                              |               |                 |
      |              |           |                                 |  2) Find PLDM capabilities and PLDM FW params|                 |
      |              |           |                                 +---------------------------------------------------------------->
      |              |           |                                 <----------------------------------------------------------------+
      |              |           |                                 |                              |               |                 |
      |              |           |                                 +----+                         |               |                 |
      |              |           |                                 |    | 3) Map PLDM FW info to  |               |                 |
      |              |           |                                 |    | D-Bus objects           |               |                 |
      |              |           |                                 +----+                         |               |                 |
      | 4) Fetch FW inventory    |  4) Find D-Bus objects from 3)  |                              |               |                 |
      +------------------------> +--------------------------------->                              |               |                 |
      <------------------------+ <---------------------------------+                              |               |                 |
      |              |           |                                 |                              |               |                 |
      |              |           |                                 |                              |               |                 |
      |              |           |                                 |                              |               |                 |
      |              |           |                                 |                              |               |                 |
      |              |           |                                 |                              |               |                 |
      | 5) POST image to endpoint|   5)Activate FW endpoint        |                              |               |                 |
      +------------------------> +--------------------------------->                              |               |                 |
      <------------------------+ <---------------------------------+                              |               |                 |
      |              |           |                                 +---+                          |               |                 |
      |              |           |                                 |   | 6) Create Activation D-Bus object        |                 |
      |              |           |                                 +---+                          |               |                 |
      |              |           |                                 |     7) Request update        |               |                 |
      |              |           |                                 +---------------------------------------------------------------->
      |              |           |                                 <----------------------------------------------------------------+
      |              |           |                                 |                              |               |                 |
      |              |           |                                 |     8) Fetch image           |               |                 |
      |              |           |                                 <----------------------------------------------------------------+
      |              |           |                                 +---------------------------------------------------------------->
      |              |           |                                 |                              |               |                 |
      |              |           |         9) Monitor progress ----+                              |               |                 |
      |              |           |         and report on D-Bus|    |                              |               |                 |
      |  Progress on FW update task                           +----+                              |               |                 |
      <--------------+---------+ <---------------------------------+     10) Apply and activate image             |                 |
      |              |           |                                 +---------------------------------------------------------------->
      |              |           |                                 <----------------------------------------------------------------+
      |              |           |                                 |                              |               |                 |
      |              |           |                                 |                              |               |                 |
      |              |           |                                 +----+                         |               |                 |
      |              |           |                                 |    | 11) Modify PLDM FW info |               |                 |
      |              |           |                                 +----+ D-Bus objects           |               |                 |
      |              |           |                                 |                              |               |                 |
      +              +-----------+---------------------------------+------------------------------+---------------+                 +

1) The first step involves discovery of devices based on the transport protocol
being used.

2) Based on the transport channels established in 1), the BMC PLDM service will
determine if the connected devices (FDs) support PLDM, and in addition will
determine:
- whether they support PLDM based firmware update
- their current firmware version
- firmware update relate metadata (image activation mechanism, recovery
capabilities, etc.)
- from a DSP0267 perspective, relevant commands are (not necessarily limited to
these): *QueryDeviceIdentifiers*, *GetFirmwareParameters*

3) Based on the information retrieved in 2), the PLDM service will create
certain D-Bus objects implementing the following interfaces:
a) xyz.openbmc_project.Inventory.Item.<Type> to depict an FD.
b) xyz.openbmc_project.Software.Version, to denote the current version of the
firmware on the FD.
c) xyz.openbmc_project.Software.Activation, to denote the current state of the
firmware on the FD.
d) xyz.openbmc_project.Association.Definitions to form an association between
the object implementing a) and the object implementing b) and c)
e) xyz.openbmc_project.Common.FilePath or similar to denote the staging area for
fiwmware images directed at the object that implements a). The object that
implements a) will also implement this interface. Alternatively, this can be a
new interface in the the xyz.openbmc_project.Software namespace, and in that
case the object that implements d) would implement this interface as well.
The PLDM service will maintain a mapping of FD identifiers to the D-Bus objects
created.

4) At this stage a Redfish client should be able to view the FDs as part of the
SoftwareInventory collection. The webserver can find these FDs and the related
inventory items based on the D-Bus interfaces from 3). The webserver based on
this information can also prepare endpoints to be pushed into the
*HttpPushUriTargets* (part of the UpdateService schema) array for these FDs
(for the purpose of using the UpdateService to update firmware of these FDs).
For example the webserver might find an /xyz/openbmc_project/inventory/nic0
object (for a NIC that implements PLDM) that implements
xyz.openbmc_project.Inventory.Item.<Type> and is also associated
to the xyz.openbmc_project.Software interfaces from 3). The webserver can make a
*SoftwareInventory* endpoint and determine its *RelatedItem* based on this info.
The webserver can also make a *HttpPushUri* such as
/redfish/v1/UpdateService/inventory/nic0 - the scheme can be based on pathnames
or derived out of an appropriate D-Bus property on the nic0 object.


5) At this stage a Redfish client should be able to POST an image to a
*HTTPPushURI* endpoint from 4) to target firmware update of a specific FD. The
webserver can copy this image over to a staging area, as determined by the
property from 3e). The image here could be a tarball consisting of the image
payload and minimal metadata, in the form of a Manifest file (for eg the
version), as expected by the PLDM service.

6) The PLDM service will watch for images to appear in the stating area. Based
on the staging area, the PLDM service can determine the relevant inventory D-Bus
object and the related FD info. The PLDM service will create a new D-Bus object
implementing the Software D-Bus interfaces from 3). These will be associated to
the inventory D-Bus object by means of D-Bus associations (as described in 3).
The *Activation* property would be set to *Ready*. In addition the PLDM service
will also add the xyz.openbmc_project.Software.ActivationProgress interface on
this object to denote progress of the firmware update.
The webserver will monitor for the creation of the D-Bus object described above.
Once notified, the webserver can create a Task to monitor progress, and will
also request an Activation on the created object.

7) - 10) The PLDM daemon will then initiate a firmware update to the FD as a
reaction to the webserver setting the *RequestedActivation* to *Active*. The
set of commands to be exchanged between the UA and the FD are detailed in
DSP0267. The typical exchanges are:
- The UA initiates the update via the *RequestUpdate* command.
- The UA sends firmware image metadata via the *PassCompoenentTable* command.
- Based on the metadata, the FD retrieves the image via the
*RequestFirmwareData* command.
- Once the entire firmware image has been received, the FD sends the
*TransferComplete* command.
- The FD proceeds to verify the image. The UA can query status via the
*GetStatus* command.
- Once verified, the FD sends a *ApplyComplete* request. The UA proceeds to send
a *ActivateFirmware* request. The FD performs activation, or might indicate that
it relies on the UA to perform a reset for activation. The latter case must be
handled by the PLDM service. During the activation (if the FD is
self-activating), the UA can retrieve status via *GetStatus*.
The PLDM service will keep updating the Software.Activation and
Software.ActivationProgress interfaces. The webserver can relay the same via a
Redfish task.

11) Once the firmware activation is complete, the PLDM service will clean-up the
D-Bus model in order to reflect the latest firmware version.

## Alternatives Considered
Continue using IPMI, but start making more use of OEM extensions to
suit the requirements of new platforms. However, given that the IPMI
standard is no longer under active development, we would likely end up
with a large amount of platform-specific customisations. This also does
not solve the hardware channel issues in a standard manner.
On OpenPOWER hardware at least, we've started to hit some of the limitations of
IPMI (for example, we have need for >255 sensors).

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
