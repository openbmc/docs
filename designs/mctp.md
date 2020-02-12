# OpenBMC platform communication channel: MCTP & PLDM

Author: Jeremy Kerr <jk@ozlabs.org> <jk>

Other contributors: Sumanth Bhat, Richard Thomaiyar

## Problem Description

With IPMI standards body no longer operational, DMTF's PMCI working group
defines standards to address `inside the box` communication interfaces between
the components of the platform management subsystem, including host-BMC
communication.

This design aims to implement Management Component Transport Protocol (MCTP)
to provide a common transport layer protocol for application layer
protocols providing platform manageability solutions and to provide an
opportunity to move to newer host/BMC messaging protocols to overcome
some of the limitations we've encountered with IPMI (Sensor limit, h/w channel
limitations etc.).

## Background and References

MCTP defines a standard transport protocol, plus a number of separate
physical layer bindings for the actual transport of MCTP packets. These
are defined by the DMTF's Platform Management Working group; standards
are available at:

  https://www.dmtf.org/standards/pmci

The following diagram shows how these standards map to the areas of
functionality that we may want to implement for OpenBMC. The DSP numbers
provided are references to DMTF standard documents.

![](mctp-standards.svg)

One of the key concepts here is that separation of transport protocol
from the physical layer bindings; this means that an MCTP "stack" may be
using either a I2C, PCI, Serial or custom hardware channel, without the
higher layers of that stack needing to be aware of the hardware
implementation.  These higher levels only need to be aware that they are
communicating with a certain entity, defined by an Entity ID (MCTP EID).
These entities may be any element of the platform that communicates
over MCTP - for example, the host device, the BMC, or any other
system peripheral - static or hot-pluggable.

This document is focused on the "transport" part of the platform design.
While this does enable new messaging protocols (mainly PLDM), those
components are not covered in detail much; we will propose those parts
in separate design efforts. For example, the PLDM design at
[pldm-stack.md].

As part of the design, the references to MCTP "messages" and "packets"
are intentional, to match the definitions in the MCTP standard. MCTP
messages are the higher-level data transferred between MCTP endpoints,
which packets are typically smaller, and are what is sent over the
hardware. Messages that are larger than the hardware Maximum Transmit
Unit (MTU) are split into individual packets by the transmit
implementation, and reassembled at the receive implementation.

 - [DSP0236 - MCTP Base specification 1.3.1](https://www.dmtf.org/sites/default/files/standards/documents/DSP0239_1.6.0.pdf)
 - [DSP0239 - MCTP IDs & Codes 1.6.0](https://www.dmtf.org/sites/default/files/standards/documents/DSP0239_1.6.0.pdf)
 - [DSP2016 - MCTP Overview white paper 1.0.0a](https://www.dmtf.org/sites/default/files/standards/documents/DSP2016.pdf)
 - [OpenBMC D-Bus interfaces](https://gerrit.openbmc-project.xyz/c/openbmc/phosphor-dbus-interfaces/+/30139)

## Requirements

Any channel supported on BMC should:

 - Have a simple serialisation and deserialisation format, to enable
   implementations in host firmware, which have widely varying runtime
   capabilities

 - Allow different hardware channels, as we have a wide variety of
   target platforms for OpenBMC

 - Be usable over simple hardware implementations, but have a facility
   for higher bandwidth messaging on platforms that require it.

 - Ideally, integrate with newer messaging protocols

## Design Principles

When BMC implements the MCTP protocol as a transport service,
it should:

* Support both MCTP Bus Owner and Endpoint roles.
* Support multiple physical bindings (PCIe, SMBus, Serial, OEM etc.).
* Provide a way for the upper layer protocols to send messages to an endpoint
(EID) and receive messages from endpoints. (Tx/Rx mechanism)
* Discover MCTP protocol supported devices when BMC is the Bus Owner of the
physical medium and assign Endpoint IDs (EID)
* Advertise the supported MCTP types, when BMC is the endpoint device of the
physical medium.

To enable implementation of MCTP on both host and BMC, platform independent
library (libmctp) will be written in C. The MCTP service can leverage
libmctp to provide transport services to upper layer protocols in OpenBMC.

## Proposed Design

The MCTP core specification just provides the packetization, routing and
addressing mechanisms. The actual transmit/receive of those packets is
up to the hardware binding of the MCTP transport.

For OpenBMC, we would introduce a MCTP daemon, which implements the transport
over a configurable hardware channel (eg., Serial UART, I2C or PCIe), and
provides a socket / D-Bus based interface for other processes to send and
receive complete MCTP messages. This daemon is responsible for the
packetization and routing of MCTP messages from external endpoints, and
handling the forwarding these messages to and from individual handler
applications. This includes handling local MCTP-stack configuration,
like local EID assignments.

Following components are proposed.

### Core MCTP stack (libmctp)
which includes the following
1. MCTP Control commands - contains code to handle all the core MCTP
control Commands.
2. MCTP Packet formatter - contains code to do MCTP packet assembly or
disassembly and verification of MCTP messages.
3. MCTP binding implementations - contains code for binding implementations,
which will interact with the hardware channel to transmit / receive MCTP
packets. This will be implemented as per the specified binding specification
of that channel.
4. MCTP device discovery - contains code to discover MCTP capable devices
on the physical medium bus, when MCTP is the bus owner.

### Daemons
Two types of daemons are proposed as follows, and both will rely on Core MCTP
stack (libmctp).

1. De-mux daemon - UNIX domain socket as backbone
or
2. D-Bus based daemon - relying on D-Bus itself.

Applications can either use De-mux daemon or D-Bus based daemon to send /
receive MCTP messages, with each having it's own pro's & con's.
This vision is to make the MCTP communication through socket based approach,
but due to the current limitations on advertising MCTP discovered
devices & it's properties in standard way, D-Bus approach is provided.
Based on the usage compile time flag (TBD) option in application layer
(Wrapper library) can be used to have a easy switch.

```
+--------------------------------------------------------------------------+
|                                                                          |
|                                                                          |
|                          MCTP Daemon - De-mux with socket                |
|                                       OR                                 |
|                          MCTP Daemon - D-bus based                       |
|                                                                          |
|        +---------------------------------------------------------+       |
|        |  +------------------------------------------------+     |       |
|        |  |                Control Commands                |     |       |
|        |  +------------------------------------------------+     |       |
|        |  +------------------------------------------------+     |       |
|        |  |                Device Discovery                |     |       |
|        |  +------------------------------------------------+     |       |
|        |  +------------------------------------------------+     +--------------> libmctp
|        |  |            Message TX/RX Handlers              |     |       |
|        |  +------------------------------------------------+     |       |
|        |                                                         |       |
|        | +-------+  +--------+   +----------+   +----------+     |       |
|        | |  LPC  |  | Serial |   |  SMBus   |   |   PCIe   |     |       |
|        | +-------+  +--------+   +----------+   +----------+     |       |
|        +---------------------------------------------------------+       |
|                                                                          |
+--------------------------------------------------------------------------+

```

The proposed implementation produces an MCTP "library" component which
provides the packetization and routing functions, between:

 - an "upper" messaging transmit/receive interface, for tx/rx of a full
   message to a specific endpoint (ie, (1) above)

 - a "lower" hardware binding for transmit/receive of individual
   packets, providing a method for the core to tx/rx each packet to
   hardware, and defines the parameters of the common packetisation
   code (ie. (2) above).

The lower interface would be plugged in to one of a number of
hardware-specific binding implementations. Most of these would be
included in the library source tree, but others can be plugged-in too,
perhaps where the physical layer implementation does not make sense to
include in the platform-agnostic library.

The reason for a library is to allow the same MCTP implementation to be
used in both OpenBMC and host firmware; the library should be
bidirectional. To allow this, the library would be written in portable C
(structured in a way that can be compiled as "extern C" in C++
codebases), and be able to be configured to suit those runtime
environments (for example, POSIX IO may not be available on all
platforms; we should be able to compile the library to suit). The
licence for the library should also allow this re-use; a dual Apache &
GPLv2+ licence may be best.

These "lower" binding implementations may have very different methods of
transferring packets to the physical layer. For example, a serial
binding implementation for running on a Linux environment may be
implemented through read()/write() syscalls to a PTY device. An I2C
binding for use in low-level host firmware environments may interact
directly with hardware registers to perform packet transfers.

The application specific handlers can connect to MCTP Daemon over two
options : either over a UNIX domain socket OR over D-Bus.

### De-mux daemon - MCTP daemon over a UNIX domain socket
Each of these would register with the MCTP daemon to
receive MCTP messages of a certain type, and would transmit MCTP
messages of that same type.

The daemon's sockets to these handlers is configured for non-blocking
IO, to allow the daemon to be decoupled from any blocking behaviour of
handlers. The daemon would use a message queue to enable message
reception/transmission to a blocked daemon, but this would be of a
limited size. Handlers whose sockets exceed this queue would be
disconnected from the daemon.

One design intention of the multiplexer daemon is to allow a future
kernel-based MCTP implementation without requiring major structural
changes to handler applications. The socket-based interface facilitates
this, as the unix-domain socket interface could be fairly easily swapped
out with a new kernel-based socket type.

MCTP handlers (i.e., clients of the de-multiplexer) connect using a
unix-domain socket, at the abstract socket address:

  \0mctp-demux

The socket type used should be `SOCK_SEQPACKET`.

Once connected, the client sends a single byte message, indicating what
type of MCTP messages should be forwarded to the client. Types must be
greater than zero.

Subsequent messages sent over the socket are MCTP messages sent/received
by the de-multiplexer, that match the specified MCTP message type.
Clients should use the send/recv syscalls to interact with the socket.

Each message has a fixed small header:

   `uint8_t eid`

For messages coming from the De-mux daemon, this indicates the source EID
of the outgoing MCTP message. For messages going to the De-mux daemon,
this indicates the destination EID.

The rest of the message data is the complete MCTP message, including
MCTP message type field.

The daemon does not provide a facility for clients to specify or
retrieve values for the tag field in individual MCTP packets.


### D-Bus daemon - MCTP Daemon over D-Bus interface

The applications can also connect to the MCTP Daemon over D-Bus.

The design principles behind MCTP Daemon:

1. Support all binding compiled in to a single (MCTP Daemon) application.

2. Execute separate instance (of MCTP Daemon) for each physical interface.
This is to limit any problems of the offending bus isolated to that application.
This also enables parallel execution as many limitations apply to the physical
medium.

3. Start MCTP Daemon as user space application, which will be exposing
D-Bus objects for the MCTP devices discovered.

4. Binding support (SMBus, PCIe) and control commands will be added to libmctp.

5. Entity-manager will be used to advertise the configuration required, which
the `mctp-start.service` (more details on this below) will use to instantiate
the needed MCTP Daemons by querying the objects exposed and calling the
instance. MCTP Daemons, will query the object to know further about whether it
must work as bus owner/endpoint, and under what physical interface (SMBUS/ PCIE)
etc. Option to do the same using configuration file instead of Entity-manager.

6. PLDM, SPDM, PCI VDM, NVME-MI application will run as separate daemon
interacting with the MCTP Daemons (through D-BUS / Socket – abstracted
library).

The MCTP Daemon instance can either start in `Endpoint mode` or in `Bus Owner
mode`. When the MCTP daemon instance comes up in `Endpoint mode`, BMC can be
configured to work in static EID itself or configured to accept EID from the
bus owner in that physical medium (in which case BMC will start in special
EID 0 or static EID). When the MCTP daemon instance comes up in
`Bus Owner mode`, BMC will discover MCTP capable devices on the physical
medium and assigns EIDs from a pre-configured EID pool.

#### D-Bus Interfaces

Please [refer for D-Bus Interfaces](https://gerrit.openbmc-project.xyz/c/openbmc/phosphor-dbus-interfaces/+/30139)

An illustration of how D-Bus interfaces would look like on the MCTP is shown
below (BMC as PCIe endpoint and BMC as SMBus bus owner).

![MCTP D-Bus interfaces](media/MCTP.jpg)

![Flow Diagram](media/StateFlow.jpg)

## Alternatives Considered

There have been two main alternatives to this approach:

Continue using IPMI, but start making more use of OEM extensions to
suit the requirements of new platforms. However, given that the IPMI
standard is no longer under active development, we would likely end up
with a large amount of platform-specific customisation. This also does
not solve the hardware channel issues in a standard manner.

Redfish between host and BMC. This would mean that host firmware needs a
HTTP client, a TCP/IP stack, a JSON (de)serialiser, and support for
Redfish schema. While this may be present in some environments (for
example, UEFI-based firmware), this is may not be feasible for all host
firmware implementations (for example, OpenPOWER). It's possible that we
could run a simplified Redfish stack - indeed, MCTP has a proposal for a
Redfish-over-MCTP channel (DSP0218), which uses simplified serialisation
format and no requirement on HTTP. However, this may involve a large
amount of complexity in host firmware.

In terms of an MCTP daemon structure, an alternative is to have the
MCTP implementation contained within a single process, using the libmctp
API directly for passing messages from the core code to
application-level handlers. The drawback of this approach is that this
single process needs to implement all possible functionality that is
available over MCTP, which may be quite a disjoint set. This would
likely lead to unnecessary restrictions on the implementation of those
application-level handlers (programming language, frameworks used, etc).
Also, this single-process approach would likely need more significant
modifications if/when MCTP protocol support is moved to the kernel.

The interface between the de-multiplexer daemon and clients is currently
defined as a socket-based interface. The reason for the choice of
sockets rather than D-Bus is that the former allows a direct transition
to a kernel-based socket API when suitable. However, D-Bus option is also
provided to enable certain use-cases like Add-In-Cards. Socket interface
currently does not provide a way to discover addition of a card, removal of a
card to upper layers, and thus D-Bus based solution is provided until
kernel-based socket API has these capabilities.

## Impacts

Development would be required to implement the MCTP transport, plus any
new users of the MCTP messaging (eg, a PLDM implementation). These would
somewhat duplicate the work we have in IPMI handlers.

We'd want to keep IPMI running in parallel, so the "upgrade" path should
be fairly straightforward.

Design and development needs to involve potential host, management
controllers and managed device implementations.

## Testing

For the core MCTP library, we are able to run tests there in complete
isolation (I have already been able to run a prototype MCTP stack
through the afl fuzzer) to ensure that the core transport protocol
works.

For MCTP hardware bindings, we would develop channel-specific tests that
would be run in CI on both host and BMC.

For the OpenBMC MCTP daemon implementation, testing models would depend
on the structure we adopt in the design section.
