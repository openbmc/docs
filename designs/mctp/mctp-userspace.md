# OpenBMC platform communication channel: MCTP & PLDM in userspace

Author: Jeremy Kerr <jk@ozlabs.org> <jk>

Please refer to the [MCTP Overview](mctp.md) document for general MCTP design
description, background and requirements.

This document describes a userspace implementation of MCTP infrastructure,
allowing a straightforward mechanism of supporting MCTP messaging within an
OpenBMC system.

## Proposed Design

The MCTP core specification just provides the packetisation, routing and
addressing mechanisms. The actual transmit/receive of those packets is up to
the hardware binding of the MCTP transport.

For OpenBMC, we would introduce a MCTP daemon, which implements the transport
over a configurable hardware channel (eg., Serial UART, I2C or PCIe), and
provides a socket-based interface for other processes to send and receive
complete MCTP messages. This daemon is responsible for the packetisation and
routing of MCTP messages from external endpoints, and handling the forwarding
these messages to and from individual handler applications. This includes
handling local MCTP-stack configuration, like local EID assignments.

This daemon has a few components:

1.  the core MCTP stack

2.  one or more binding implementations (eg, MCTP-over-serial), which interact
    with the hardware channel(s).

3.  an interface to handler applications over a unix-domain socket.

The proposed implementation here is to produce an MCTP "library" which provides
the packetisation and routing functions, between:

- an "upper" messaging transmit/receive interface, for tx/rx of a full message
  to a specific endpoint (ie, (1) above)

- a "lower" hardware binding for transmit/receive of individual packets,
  providing a method for the core to tx/rx each packet to hardware, and defines
  the parameters of the common packetisation code (ie. (2) above).

The lower interface would be plugged in to one of a number of hardware-specific
binding implementations. Most of these would be included in the library source
tree, but others can be plugged-in too, perhaps where the physical layer
implementation does not make sense to include in the platform-agnostic library.

The reason for a library is to allow the same MCTP implementation to be used in
both OpenBMC and host firmware; the library should be bidirectional. To allow
this, the library would be written in portable C (structured in a way that can
be compiled as "extern C" in C++ codebases), and be able to be configured to
suit those runtime environments (for example, POSIX IO may not be available on
all platforms; we should be able to compile the library to suit). The licence
for the library should also allow this re-use; a dual Apache & GPLv2+ licence
may be best.

These "lower" binding implementations may have very different methods of
transferring packets to the physical layer. For example, a serial binding
implementation for running on a Linux environment may be implemented through
read()/write() syscalls to a PTY device. An I2C binding for use in low-level
host firmware environments may interact directly with hardware registers to
perform packet transfers.

The application-specific handlers implement the actual functionality provided
over the MCTP channel, and connect to the central daemon over a UNIX domain
socket. Each of these would register with the MCTP daemon to receive MCTP
messages of a certain type, and would transmit MCTP messages of that same type.

The daemon's sockets to these handlers is configured for non-blocking IO, to
allow the daemon to be decoupled from any blocking behaviour of handlers. The
daemon would use a message queue to enable message reception/transmission to a
blocked daemon, but this would be of a limited size. Handlers whose sockets
exceed this queue would be disconnected from the daemon.

One design intention of the multiplexer daemon is to allow a future
kernel-based MCTP implementation without requiring major structural changes to
handler applications. The socket-based interface facilitates this, as the
unix-domain socket interface could be fairly easily swapped out with a new
kernel-based socket type.

MCTP is intended to be an optional component of OpenBMC. Platforms using
OpenBMC are free to adopt it as they see fit.

### Demultiplexer daemon interface

MCTP handlers (ie, clients of the demultiplexer) connect using a unix-domain
socket, at the abstract socket address:

```
\0mctp-demux
```

The socket type used should be `SOCK_SEQPACKET`.

Once connected, the client sends a single byte message, indicating what type of
MCTP messages should be forwarded to the client. Types must be greater than
zero.

Subsequent messages sent over the socket are MCTP messages sent/received by the
demultiplexer, that match the specified MCTP message type. Clients should use
the send/recv syscalls to interact with the socket.

Each message has a fixed small header:

```
uint8_t eid
```

For messages coming from the demux daemon, this indicates the source EID of the
outgoing MCTP message. For messages going to the demux daemon, this indicates
the destination EID.

The rest of the message data is the complete MCTP message, including MCTP
message type field.

The daemon does not provide a facility for clients to specify or retrieve
values for the tag field in individual MCTP packets.


## Alternatives Considered

In terms of an MCTP daemon structure, an alternative is to have the MCTP
implementation contained within a single process, using the libmctp API
directly for passing messages from the core code to application-level handlers.
The drawback of this approach is that this single process needs to implement
all possible functionality that is available over MCTP, which may be quite a
disjoint set. This would likely lead to unnecessary restrictions on the
implementation of those application-level handlers (programming language,
frameworks used, etc).  Also, this single-process approach would likely need
more significant modifications if/when MCTP protocol support is moved to the
kernel.

The interface between the demultiplexer daemon and clients is currently defined
as a socket-based interface. However, an alternative here would be to pass MCTP
messages over dbus instead. The reason for the choice of sockets rather than
dbus is that the former allows a direct transition to a kernel-based socket API
when suitable.

## Testing

For the core MCTP library, we are able to run tests there in complete isolation
(I have already been able to run a prototype MCTP stack through the afl fuzzer)
to ensure that the core transport protocol works.

For MCTP hardware bindings, we would develop channel-specific tests that would
be run in CI on both host and BMC.

For the OpenBMC MCTP daemon implementation, testing models would depend on the
structure we adopt in the design section.
