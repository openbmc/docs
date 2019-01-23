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
devices. This design doesn't preclude enablement of communication channels not
involving the BMC and the host.

## Background and References
PLDM is designed to be an effective interface and data model that provides
efficient access to low-level platform inventory, monitoring, control, event,
and data/parameters transfer functions. For example, temperature, voltage, or
fan sensors can have a PLDM representation that can be used to monitor and
control the platform using a set of PLDM messages. PLDM defines data
representations and commands that abstract the platform management hardware.

As stated earlier, PLDM is designed for different flavors of "inside the box"
communication. PLDM groups commands under broader functions, and defines
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
  and effecters are associated to entities (similar to IPMI, entities cab be
physical or logical), where sensors are a mechanism for monitoring and effecters
are a mechanism for control. Effecters can be numeric or state based. PLDM
defines commonly used entities and their IDs, but there 8K slots available to
define OEM entities.

- PLDM allows bidirectional communication, and sending asynchronous events.

- A very active PLDM related working group in the DMTF.

The plan is to run PLDM over MCTP. MCTP is defined in a spec of its own, and a
proposal on the MCTP design is in discussion already. There's going to be an
intermediate PLDM over MCTP binding layer, which lets us send PLDM messages over
MCTP. This is defined in a spec of its own, and the design for this binding will
be proposed separately.

## Requirements
How different BMC/Host/other applications make use of PLDM messages is outside
the scope of this requirements doc. The requirements listed here are related to
the PLDM protocol stack and the request/response model:

- Marshalling and unmarshalling of PLDM messages, defined in various PLDM Type
  specs, must be implemented. This can of course be staged based on the need of
specific Types and functions. Since this is just encoding and decoding PLDM
messages, I believe there would be motivation to build this into a library that
could be shared between BMC, host and other firmware stacks. The specifics of
each PLDM Type (such as FRU table structures, sensor PDR structures, etc) are
implemented by this lib.

- Mapping PLDM concepts to native OpenBMC concepts must be implemented. For
  e.g.: mapping PLDM sensors to phosphor-hwmon hosted D-Bus objects, mapping
PLDM FRU data to D-Bus objects hosted by phosphor-inventory-manager, etc. The
mapping shouldn't be restrictive to D-Bus alone (meaning it shouldn't be
necessary to put objects on the Bus just so serve PLDM requests, a problem that
exists with phosphor-host-ipmid today). Essentially these are platform specific
PLDM message handlers.

- The BMC should be able to act as a PLDM responder as well as a PLDM requester.
  As a PLDM responder, the BMC can monitor/control other devices. As a PLDM
responder, the BMC can react to PLDM messages directed to it via requesters in
the platform, for e.g, the Host.

- As a PLDM requester, the BMC must be able to discover other PLDM enabled
  components in the platform.

- As a PLDM requester, the BMC must be able to send simultaneous messages to
  different responders, but at the same time it can issue a single message to a
specific responder at a time.

- As a PLDM requester, the BMC must be able to handle out of order responses.

- As a PLDM responder, the BMC may simultaneously respond to messages from
  different requesters, but the spec doesn't mandate this. In other words the
responder could be single-threaded.

- It should be possible to plug-in non-existent PLDM functions (these maybe
  new/existing standard Types, or OEM Types) into the PLDM stack.

## Proposed Design
This document covers the architectural, interface, and design details. It
provides recommendations for implementations, but implementation details are
outside the scope of this document.

The design aims at having a single PLDM daemon serve both the requester and
responder functions, and having transport specific endpoints to communicate
on different channels.

The design enables concurrency aspects of the requester and responder functions,
but the goal is to employ ASIO and event loops, instead of multiple threads,
wherever possible.

The following are high level structural elements of the design:

### PLDM encode/decode libraries

This library would take a PLDM message, decode it and spit out the different
fields of the message. Conversely, given a PLDM Type, command code, and the
command's data fields, it would make a PLDM message. The thought is to design
this library such that it can be used by BMC and the host firmware stacks,
because it's the encode/decode and protocol piece (and not the handling of a
message). That would mean additional requirements such as having this as a C lib
instead of C++, because of the runtime constraints of host firmware stacks.

There would be one encode/decode lib per PLDM Type. So for e.g. something like
/usr/lib/pldm/libbase.so, /usr/lib/pldm/libfru.so, etc.

### PLDM provider libraries

These libraries would implement the platform specific handling of incoming PLDM
requests (basically helping with the PLDM responder implementation, see next
bullet point), so for instance they would query D-Bus objects (or even something
like a JSON file) to fetch platform specific information to respond to the PLDM
message. They would link with the encode/decode libs. Like the encode/decode
libs, there would be one per PLDM Type (for e.g
/usr/lib/pldm/providers/libfru.so).

It should be possible to plug-in a provider library. That lets someone add
functionality for new PLDM (standard as well as OEM) Types. The libraries would
implement a "register" API to plug-in handlers for specific PLDM messages.
Something like:

template <typename Handler, typename... args>
auto register(uint8_t type, uint8_t command, Handler handler);

This allows for providing a strongly-typed C++ handler registration scheme. It
would also be possible to validate the parameters passed to the handler at
compile time.

### Request/Response Model

The PLDM deamon links with the encode/decode and provider libs. The daemon
would have to implement the following functions:

#### Receiver/Responder
The receiver wakes up on getting notified of incoming PLDM messages (via D-Bus
signal or callback from the transport layer) from a remote PLDM device. If the
message type is "Request" it would route them to a PLDM provider library. Via
the library, asynchronous D-Bus calls (using sdbusplus-asio) would be made, so
that the receiver can register a handler for the D-Bus response, instead of
having to wait for the D-Bus response. This way it can go back to listening for
incoming PLDM messages.

In the D-Bus reponse handler, the receiver will send out the PLDM response
message via the transport's send message API. If the transport's send message
API blocks for a considerably long duration, then it would have to be run in a
thread of it's own.

If the incoming PLDM message is of type "Response", then the receiver emits a
D-Bus singal pointing to the response message. Any time the message is too
large to fit in a D-Bus payload, the message is written to a file, and a
read-only file descriptor pointing to that file is contained in the D-Bus
signal.

#### Requester
Designing the BMC as a PLDM requester is interesting. We haven't had this with
IPMI, because the BMC was typically an IPMI server. PLDM requester functions
will be spread across multiple OpenBMC applications (instead of a single big
requester app) - based on the responder they're talking and the high level
function they implement. For example, there could be an app that lets the BMC
upgrade firmware for other devices using PLDM - this would be a generic app
in the sense that the same set of commands might have to be run irrespective
of the device on the other side. There could also be an app that does fan
control on a remote device, based on sensors from that device and algorithms
specific to that device.

The PLDM daemon would have to implement D-Bus interfaces to form the requester
functions: a method to get the next PLDM instance id, a method to send a PLDM
message over the underlying transport (again, this will have two versions: one
that accepts a byte stream, and the other that accepts an fd, for large
messages) and a signal to indicate a PLDM response from the remote PLDM device.
The signal would comprise of the transport headers, PLDM headers, and the PLDM
payload.

The typical flow for a requester app would be to get the next instance id (via
the D-Bus API), send the PLDM message via the D-Bus API, and add a handler for
the D-Bus signal containing the response. As this flow is asynchronous, the
requester app can execute other scheduled work, if any, in its event loop,
while it waits for the D-Bus signal containing the response. The D-Bus API to
send a PLDM message to the remote PLDM device would call the underlying
transport's send API. If that API blocks for too long, the call may have to run
in a thread of it's own. The D-Bus signal containing a response message is
emitted by the receiver (see above).

### Multiple tranport channels
The PLDM daemon might have to talk to remote PLDM devices via different
channels. While a level of abstraction might be provided by MCTP, the PLDM
daemon would have to implement a D-Bus interface to target a specific
transport channel, so that requester apps on the BMC can send messages over
that transport. Also, it should be possible to plug-in platform specific D-Bus
objects that implement an interface to target a platform specific transport.

## Alternatives Considered
There have been two main alternatives to this approach:

Continue using IPMI, but start making more use of OEM extensions to
suit the requirements of new platforms. However, given that the IPMI
standard is no longer under active development, we would likely end up
with a large amount of platform-specific customisations. This also does
not solve the hardware channel issues in a standard manner.

Redfish between host and BMC. This would mean that host firmware needs a
HTTP client, a TCP/IP stack, a JSON (de)serialiser, and support for
Redfish schema. This is not feasible for all host firmware
implementations; certainly not for OpenPOWER. It's possible that we
could run a simplified Redfish stack - indeed, MCTP has a proposal for a
Redfish-over-MCTP protocol, which uses simplified serialisation and no
requirement on HTTP. However, this still introduces a large amount of
complexity in host firmware.

## Impacts
Development would be required to implement the PLDM protocol, the
request/response model, and platform specific handling. Low level design is
required to implement the protocol specifics of each of the PLDM Types. Such low
level design is not included in this proposal.

Design and development needs to involve potential host firmware
implementations.

## Testing
Testing can be done without having to depend on the underlying transport layer.

The responder function can be tested by mocking a requester and the transport
layer: this would essentially test the protocol handling and platform specific
handling. The requester function can be tested by mocking a responder: this
would test the instance id handling and the send/receive functions.

APIs from the shared libraries can be tested via fuzzing.
