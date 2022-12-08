# OpenBMC in-kernel MCTP

Author: Jeremy Kerr `<jk@codeconstruct.com.au>`

Please refer to the [MCTP Overview](mctp.md) document for general MCTP design
description, background and requirements.

This document describes a kernel-based implementation of MCTP infrastructure,
providing a sockets-based API for MCTP communication within an OpenBMC-based
platform.

# Requirements for a kernel implementation

- The MCTP messaging API should be an obvious application of the existing POSIX
  socket interface

- Configuration should be simple for a straightforward MCTP endpoint: a single
  network with a single local endpoint id (EID).

- Infrastructure should be flexible enough to allow for more complex MCTP
  networks, allowing:

  - each MCTP network (as defined by section 3.2.31 of DSP0236) may consist of
    multiple local physical interfaces, and/or multiple EIDs;

  - multiple distinct (ie., non-bridged) networks, possibly containing
    duplicated EIDs between networks;

  - multiple local EIDs on a single interface, and

  - customisable routing/bridging configurations within a network.

# Proposed Design

The design contains several components:

- An interface for userspace applications to send and receive MCTP messages: A
  mapping of the sockets API to MCTP usage

- Infrastructure for control and configuration of the MCTP network(s),
  consisting of a configuration utility, and a kernel messaging facility for
  this utility to use.

- Kernel drivers for physical interface bindings.

In general, the kernel components cover the transport functionality of MCTP,
such as message assembly/disassembly, packet forwarding, and physical interface
implementations.

Higher-level protocols (such as PLDM) are implemented in userspace, through the
introduced socket API. This also includes the majority of the MCTP Control
Protocol implementation (DSP0236, section 11) - MCTP endpoints will typically
have a specific process to request and respond to control protocol messages.
However, the kernel will include a small subset of control protocol code to
allow very simple endpoints, with static EID allocations, to run without this
process. MCTP endpoints that require more than just single-endpoint
functionality (bus owners, bridges, etc), and/or dynamic EID allocation, would
include the control message protocol process.

A new driver is introduced to handle each physical interface binding. These
drivers expose the appropriate `struct net_device` to handle transmission and
reception of MCTP packets on their associated hardware channels. Under Linux,
the namespace for these interfaces is separate from other network interfaces -
such as those for ethernet.

## Structure: interfaces & networks

The kernel models the local MCTP topology through two items: interfaces and
networks.

An interface (or "link") is an instance of an MCTP physical transport binding
(as defined by DSP0236, section 3.2.47), likely connected to a specific hardware
device. This is represented as a `struct netdevice`, and has a user-visible name
and index (`ifindex`). Non-hardware-attached interfaces are permitted, to allow
local loopback and/or virtual interfaces.

A network defines a unique address space for MCTP endpoints by endpoint-ID
(described by DSP0236, section 3.2.31). A network has a user-visible identifier
to allow references from userspace. Route definitions are specific to one
network.

Interfaces are associated with one network. A network may be associated with one
or more interfaces.

If multiple networks are present, each may contain EIDs that are also present on
other networks.

## Sockets API

### Protocol definitions

We define a new address family (and corresponding protocol family) for MCTP:

```c
    #define AF_MCTP /* TBD */
    #define PF_MCTP AF_MCTP
```

MCTP sockets are created with the `socket()` syscall, specifying `AF_MCTP` as
the domain. Currently, only a `SOCK_DGRAM` socket type is defined.

```c
    int sd = socket(AF_MCTP, SOCK_DGRAM, 0);
```

The only (current) value for the `protocol` argument is 0. Future protocol
implementations may be added later.

MCTP Sockets opened with a protocol value of 0 will communicate directly at the
transport layer; message buffers received by the application will consist of
message data from reassembled MCTP packets, and will include the full message
including message type byte and optional message integrity check (IC).
Individual packet headers are not included; they may be accessible through a
future `SOCK_RAW` socket type.

As with all socket address families, source and destination addresses are
specified with a new `sockaddr` type:

```c
    struct sockaddr_mctp {
            sa_family_t         smctp_family; /* = AF_MCTP */
            int                 smctp_network;
            struct mctp_addr    smctp_addr;
            uint8_t             smctp_type;
            uint8_t             smctp_tag;
    };

    struct mctp_addr {
            uint8_t             s_addr;
    };

    /* MCTP network values */
    #define MCTP_NET_ANY        0

    /* MCTP EID values */
    #define MCTP_ADDR_ANY       0xff
    #define MCTP_ADDR_BCAST     0xff

    /* MCTP type values. Only the least-significant 7 bits of
     * smctp_type are used for tag matches; the specification defines
     * the type to be 7 bits.
     */
    #define MCTP_TYPE_MASK      0x7f

    /* MCTP tag defintions; used for smcp_tag field of sockaddr_mctp */
    /* MCTP-spec-defined fields */
    #define MCTP_TAG_MASK    0x07
    #define MCTP_TAG_OWNER   0x08
    /* Others: reserved */

    /* Helpers */
    #define MCTP_TAG_RSP(x) (x & MCTP_TAG_MASK) /* response to a request: clear TO, keep value */
```

### Syscall behaviour

The following sections describe the MCTP-specific behaviours of the standard
socket system calls. These behaviours have been chosen to map closely to the
existing sockets APIs.

#### `bind()`: set local socket address

Sockets that receive incoming request packets will bind to a local address,
using the `bind()` syscall.

```c
    struct sockaddr_mctp addr;

    addr.smctp_family = AF_MCTP;
    addr.smctp_network = MCTP_NET_ANY;
    addr.smctp_addr.s_addr = MCTP_ADDR_ANY;
    addr.smctp_type = MCTP_TYPE_PLDM;
    addr.smctp_tag = MCTP_TAG_OWNER;

    int rc = bind(sd, (struct sockaddr *)&addr, sizeof(addr));
```

This establishes the local address of the socket. Incoming MCTP messages that
match the network, address, and message type will be received by this socket.
The reference to 'incoming' is important here; a bound socket will only receive
messages with the TO bit set, to indicate an incoming request message, rather
than a response.

The `smctp_tag` value will configure the tags accepted from the remote side of
this socket. Given the above, the only valid value is `MCTP_TAG_OWNER`, which
will result in remotely "owned" tags being routed to this socket. Since
`MCTP_TAG_OWNER` is set, the 3 least-significant bits of `smctp_tag` are not
used; callers must set them to zero. See the
[Tag behaviour for transmitted messages](#tag-behaviour-for-transmitted-messages)
section for more details. If the `MCTP_TAG_OWNER` bit is not set, `bind()` will
fail with an errno of `EINVAL`.

A `smctp_network` value of `MCTP_NET_ANY` will configure the socket to receive
incoming packets from any locally-connected network. A specific network value
will cause the socket to only receive incoming messages from that network.

The `smctp_addr` field specifies a local address to bind to. A value of
`MCTP_ADDR_ANY` configures the socket to receive messages addressed to any local
destination EID.

The `smctp_type` field specifies which message types to receive. Only the lower
7 bits of the type is matched on incoming messages (ie., the most-significant IC
bit is not part of the match). This results in the socket receiving packets with
and without a message integrity check footer.

#### `connect()`: set remote socket address

Sockets may specify a socket's remote address with the `connect()` syscall:

```c
    struct sockaddr_mctp addr;
    int rc;

    addr.smctp_family = AF_MCTP;
    addr.smctp_network = MCTP_NET_ANY;
    addr.smctp_addr.s_addr = 8;
    addr.smctp_tag = MCTP_TAG_OWNER;
    addr.smctp_type = MCTP_TYPE_PLDM;

    rc = connect(sd, (struct sockaddr *)&addr, sizeof(addr));
```

This establishes the remote address of a socket, used for future message
transmission. Like other `SOCK_DGRAM` behaviour, this does not generate any MCTP
traffic directly, but just sets the default destination for messages sent from
this socket.

The `smctp_network` field may specify a locally-attached network, or the value
`MCTP_NET_ANY`, in which case the kernel will select a suitable MCTP network.
This is guaranteed to work for single-network configurations, but may require
additional routing definitions for endpoints attached to multiple distinct
networks. See the [Addressing](#addressing) section for details.

The `smctp_addr` field specifies a remote EID. This may be the `MCTP_ADDR_BCAST`
the MCTP broadcast EID (0xff).

The `smctp_type` field specifies the type field of messages transferred over
this socket.

The `smctp_tag` value will configure the tag used for the local side of this
socket. The only valid value is `MCTP_TAG_OWNER`, which will result in an
"owned" tag to be allocated for this socket, and will remain allocated for all
future outgoing messages, until either the socket is closed, or `connect()` is
called again. If a tag cannot be allocated, `connect()` will report an error,
with an errno value of `EAGAIN`. See the
[Tag behaviour for transmitted messages](#tag-behaviour-for-transmitted-messages)
section for more details. If the `MCTP_TAG_OWNER` bit is not set, `connect()`
will fail with an errno of `EINVAL`.

Requesters which connect to a single responder will typically use `connect()` to
specify the peer address and tag for future outgoing messages.

#### `sendto()`, `sendmsg()`, `send()` & `write()`: transmit an MCTP message

An MCTP message is transmitted using one of the `sendto()`, `sendmsg()`,
`send()` or `write()` syscalls. Using `sendto()` as the primary example:

```c
    struct sockaddr_mctp addr;
    char buf[14];
    ssize_t len;

    /* set message destination */
    addr.smctp_family = AF_MCTP;
    addr.smctp_network = 0;
    addr.smctp_addr.s_addr = 8;
    addr.smctp_tag = MCTP_TAG_OWNER;
    addr.smctp_type = MCTP_TYPE_ECHO;

    /* arbitrary message to send, with message-type header */
    buf[0] = MCTP_TYPE_ECHO;
    memcpy(buf + 1, "hello, world!", sizeof(buf) - 1);

    len = sendto(sd, buf, sizeof(buf), 0,
                    (struct sockaddr_mctp *)&addr, sizeof(addr));
```

The address argument is treated the same way as for `connect()`: The network and
address fields define the remote address to send to. If `smctp_tag` has the
`MCTP_TAG_OWNER`, the kernel will ignore any bits set in `MCTP_TAG_VALUE`, and
generate a tag value suitable for the destination EID. If `MCTP_TAG_OWNER` is
not set, the message will be sent with the tag value as specified. If a tag
value cannot be allocated, the system call will report an errno of `EAGAIN`.

The application must provide the message type byte as the first byte of the
message buffer passed to `sendto()`. If a message integrity check is to be
included in the transmitted message, it must also be provided in the message
buffer, and the most-significant bit of the message type byte must be 1.

If the first byte of the message does not match the message type value, then the
system call will return an error of `EPROTO`.

The `send()` and `write()` system calls behave in a similar way, but do not
specify a remote address. Therefore, `connect()` must be called beforehand; if
not, these calls will return an error of `EDESTADDRREQ` (Destination address
required).

Using `sendto()` or `sendmsg()` on a connected socket may override the remote
socket address specified in `connect()`. The `connect()` address and tag will
remain associated with the socket, for future unaddressed sends. The tag
allocated through a call to `sendto()` or `sendmsg()` on a connected socket is
subject to the same invalidation logic as on an unconnected socket: It is
expired either by timeout or by a subsequent `sendto()`.

The `sendmsg()` system call allows a more compact argument interface, and the
message buffer to be specified as a scatter-gather list. At present no ancillary
message types (used for the `msg_control` data passed to `sendmsg()`) are
defined.

Transmitting a message on an unconnected socket with `MCTP_TAG_OWNER` specified
will cause an allocation of a tag, if no valid tag is already allocated for that
destination. The (destination-eid,tag) tuple acts as an implicit local socket
address, to allow the socket to receive responses to this outgoing message. If
any previous allocation has been performed (to for a different remote EID), that
allocation is lost. This tag behaviour can be controlled through the
`MCTP_TAG_CONTROL` socket option.

Sockets will only receive responses to requests they have sent (with TO=1) and
may only respond (with TO=0) to requests they have received.

#### `recvfrom()`, `recvmsg()`, `recv()` & `read()`: receive an MCTP message

An MCTP message can be received by an application using one of the `recvfrom()`,
`recvmsg()`, `recv()` or `read()` system calls. Using `recvfrom()` as the
primary example:

```c
    struct sockaddr_mctp addr;
    socklen_t addrlen;
    char buf[14];
    ssize_t len;

    addrlen = sizeof(addr);

    len = recvfrom(sd, buf, sizeof(buf), 0,
                    (struct sockaddr_mctp *)&addr, &addrlen);

    /* We can expect addr to describe an MCTP address */
    assert(addrlen >= sizeof(buf));
    assert(addr.smctp_family == AF_MCTP);

    printf("received %zd bytes from remote EID %d\n", rc, addr.smctp_addr);
```

The address argument to `recvfrom` and `recvmsg` is populated with the remote
address of the incoming message, including tag value (this will be needed in
order to reply to the message).

The first byte of the message buffer will contain the message type byte. If an
integrity check follows the message, it will be included in the received buffer.

The `recv()` and `read()` system calls behave in a similar way, but do not
provide a remote address to the application. Therefore, these are only useful if
the remote address is already known, or the message does not require a reply.

Like the send calls, sockets will only receive responses to requests they have
sent (TO=1) and may only respond (TO=0) to requests they have received.

#### `getsockname()` & `getpeername()`: query local/remote socket address

The `getsockname()` system call returns the `struct sockaddr_mctp` value for the
local side of this socket, `getpeername()` for the remote (ie, that used in a
connect()). Since the tag value is a property of the remote address,
`getpeername()` may be used to retrieve a kernel-allocated tag value.

Calling `getpeername()` on an unconnected socket will result in an error of
`ENOTCONN`.

#### Socket options

The following socket options are defined for MCTP sockets:

##### `MCTP_ADDR_EXT`: Use extended addressing information in sendmsg/recvmsg

Enabling this socket option allows an application to specify extended addressing
information on transmitted packets, and access the same on received packets.

When the `MCTP_ADDR_EXT` socket option is enabled, the application may specify
an expanded `struct sockaddr` to the `recvfrom()` and `sendto()` system calls.
This as defined as:

```c
    struct sockaddr_mctp_ext {
            /* fields exactly match struct sockaddr_mctp */
            sa_family_t         smctp_family; /* = AF_MCTP */
            int                 smctp_network;
            struct mctp_addr    smctp_addr;
            uint8_t             smcp_tag;
            /* extended addressing */
            int                 smctp_ifindex;
            uint8_t             smctp_halen;
            unsigned char       smctp_haddr[/* TBD */];
    }
```

If the `addrlen` specified to `sendto()` or `recvfrom()` is sufficient to
contain this larger structure, then the extended addressing fields are consumed
/ populated respectively.

##### `MCTP_TAG_CONTROL`: manage outgoing tag allocation behaviour

The set/getsockopt argument is a `mctp_tagctl` structure:

    struct mctp_tagctl {
        bool            retain;
        struct timespec timeout;
    };

This allows an application to control the behaviour of allocated tags for
non-connected sockets when transferring messages to multiple different
destinations (ie., where a `struct sockaddr_mctp` is provided for individual
messages, and the `smctp_addr` destination for those sockets may vary across
calls).

The `retain` flag indicates to the kernel that the socket should not release tag
allocations when a message is sent to a new destination EID. This causes the
socket to continue to receive incoming messages to the old (dest,tag) tuple, in
addition to the new tuple.

The `timeout` value specifies a maximum amount of time to retain tag values.
This should be based on the reply timeout for any upper-level protocol.

The kernel may reject a request to set values that would cause excessive tag
allocation by this socket. The kernel may also reject subsequent tag-allocation
requests (through send or connect syscalls) which would cause excessive tags to
be consumed by the socket, even though the tag control settings were accepted in
the setsockopt operation.

Changing the default tag control behaviour should only be required when:

- the socket is sending messages with TO=1 (ie, is a requester); and
- messages are sent to multiple different destination EIDs from the one socket.

#### Syscalls not implemented

The following system calls are not implemented for MCTP, primarily as they are
not used in `SOCK_DGRAM`-type sockets:

- `listen()`
- `accept()`
- `ioctl()`
- `shutdown()`
- `mmap()`

### Userspace examples

These examples cover three general use-cases:

- **requester**: sends requests to a particular (EID, type) target, and receives
  responses to those packets

  This is similar to a typical UDP client

- **responder**: receives all locally-addressed messages of a specific
  message-type, and responds to the requester immediately.

  This is similar to a typical UDP server

- **controller**: a specific service for a bus owner; may send broadcast
  messages, manage EID allocations, update local MCTP stack state. Will need
  low-level packet data.

  This is similar to a DHCP server.

#### Requester

"Client"-side implementation to send requests to a responder, and receive a
response. This uses a (fictitious) message type of `MCTP_TYPE_ECHO`.

```c
    int main() {
            struct sockaddr_mctp addr;
            socklen_t addrlen;
            struct {
                uint8_t type;
                uint8_t data[14];
            } msg;
            int sd, rc;

            sd = socket(AF_MCTP, SOCK_DGRAM, 0);

            addr.sa_family = AF_MCTP;
            addr.smctp_network = MCTP_NET_ANY; /* any network */
            addr.smctp_addr.s_addr = 9;    /* remote eid 9 */
            addr.smctp_tag = MCTP_TAG_OWNER; /* kernel will allocate an owned tag */
            addr.smctp_type = MCTP_TYPE_ECHO; /* ficticious message type */
            addrlen = sizeof(addr);

            /* set message type and payload */
            msg.type = MCTP_TYPE_ECHO;
            strncpy(msg.data, "hello, world!", sizeof(msg.data));

            /* send message */
            rc = sendto(sd, &msg, sizeof(msg), 0,
                            (struct sockaddr *)&addr, addrlen);

            if (rc < 0)
                    err(EXIT_FAILURE, "sendto");

            /* Receive reply. This will block until a reply arrives,
             * which may never happen. Actual code would need a timeout
             * here. */
            rc = recvfrom(sd, &msg, sizeof(msg), 0,
                        (struct sockaddr *)&addr, &addrlen);
            if (rc < 0)
                    err(EXIT_FAILURE, "recvfrom");

            assert(msg.type == MCTP_TYPE_ECHO);
            /* ensure we're nul-terminated */
            msg.data[sizeof(msg.data)-1] = '\0';

            printf("reply: %s\n", msg.data);

            return EXIT_SUCCESS;
    }
```

#### Responder

"Server"-side implementation to receive requests and respond. Like the client,
This uses a (fictitious) message type of `MCTP_TYPE_ECHO` in the
`struct sockaddr_mctp`; only messages matching this type will be received.

```c
    int main() {
            struct sockaddr_mctp addr;
            socklen_t addrlen;
            int sd, rc;

            sd = socket(AF_MCTP, SOCK_DGRAM, 0);

            addr.sa_family = AF_MCTP;
            addr.smctp_network = MCTP_NET_ANY; /* any network */
            addr.smctp_addr.s_addr = MCTP_EID_ANY;
            addr.smctp_type = MCTP_TYPE_ECHO;
            addr.smctp_tag = MCTP_TAG_OWNER;
            addrlen = sizeof(addr);

            rc = bind(sd, (struct sockaddr *)&addr, addrlen);
            if (rc)
                    err(EXIT_FAILURE, "bind");

            for (;;) {
                    struct {
                        uint8_t type;
                        uint8_t data[14];
                    } msg;

                    rc = recvfrom(sd, &msg, sizeof(msg), 0,
                                    (struct sockaddr *)&addr, &addrlen);
                    if (rc < 0)
                            err(EXIT_FAILURE, "recvfrom");
                    if (rc < 1)
                            warnx("not enough data for a message type");

                    assert(addrlen == sizeof(addr));
                    assert(msg.type == MCTP_TYPE_ECHO);

                    printf("%zd bytes from EID %d\n", rc, addr.smctp_addr);

                    /* Reply to requester; this macro just clears the TO-bit.
                     * Other addr fields will describe the remote endpoint,
                     * so use those as-is.
                     */
                    addr.smctp_tag = MCTP_TAG_RSP(addr.smctp_tag);

                    rc = sendto(sd, &msg, rc, 0,
                                (struct sockaddr *)&addr, addrlen);
                    if (rc < 0)
                            err(EXIT_FAILURE, "sendto");
            }

            return EXIT_SUCCESS;
    }
```

#### Broadcast request

Sends a request to a broadcast EID, and receives (unicast) replies. Typical
control protocol pattern.

```c
    int main() {
            struct sockaddr_mctp txaddr, rxaddr;
            struct timespec start, cur;
            struct pollfd pollfds[1];
            socklen_t addrlen;
            uint8_t buf[2];
            int timeout;

            sd = socket(AF_MCTP, SOCK_DGRAM, 0);

            /* destination address setup */
            txaddr.sa_family = AF_MCTP;
            txaddr.smctp_network = 1; /* specific network required for broadcast */
            txaddr.smctp_addr.s_addr = MCTP_TAG_BCAST; /* broadcast dest */
            txaddr.smctp_type = MCTP_TYPE_CONTROL;
            txaddr.smctp_tag = MCTP_TAG_OWNER;

            buf[0] = MCTP_TYPE_CONTROL;
            buf[1] = 'a';

            /* We're doing a sendto() to a broadcast address here. If we were
             * sending more than one broadcast message, we'd be better off
             * doing connect(); sendto();, in order to retain the tag
             * reservation across all transmitted messages. However, since this
             * is a single transmit, that makes no difference in this
             * particular case.
             */
            rc = sendto(sd, buf, 2, 0, (struct sockaddr *)&txaddr,
                            sizeof(txaddr));
            if (rc < 0)
                    err(EXIT_FAILURE, "sendto");

            /* Set up poll behaviour, and record our starting time for
             * reply timeouts */
            pollfds[0].fd = sd;
            pollfds[0].events = POLLIN;
            clock_gettime(CLOCK_MONOTONIC, &start);

            for (;;) {
                    /* Calculate the amount of time left for replies */
                    clock_gettime(CLOCK_MONOTONIC, &cur);
                    timeout = calculate_timeout(&start, &cur, 1000);

                    rc = poll(pollfds, 1, timeout)
                    if (rc < 0)
                        err(EXIT_FAILURE, "poll");

                    /* timeout receiving a reply? */
                    if (rc == 0)
                        break;

                    /* sanity check that we have a message to receive */
                    if (!(pollfds[0].revents & POLLIN))
                        break;

                    addrlen = sizeof(rxaddr);

                    rc = recvfrom(sd, &buf, 2, 0, (struct sockaddr *)&rxaddr,
                            &addrlen);
                    if (rc < 0)
                            err(EXIT_FAILURE, "recvfrom");

                    assert(addrlen >= sizeof(rxaddr));
                    assert(rxaddr.smctp_family == AF_MCTP);

                    printf("response from EID %d\n", rxaddr.smctp_addr);
            }

            return EXIT_SUCCESS;
    }
```

### Implementation notes

#### Addressing

Transmitted messages (through `sendto()` and related system calls) specify their
destination via the `smctp_network` and `smctp_addr` fields of a
`struct sockaddr_mctp`.

The `smctp_addr` field maps directly to the destination endpoint's EID.

The `smctp_network` field specifies a locally defined network identifier. To
simplify situations where there is only one network defined, the special value
`MCTP_NET_ANY` is allowed. This will allow the kernel to select a specific
network for transmission.

This selection is entirely user-configured; one specific network may be defined
as the system default, in which case it will be used for all message
transmission where `MCTP_NET_ANY` is used as the destination network.

In particular, the destination EID is never used to select a destination
network.

MCTP responders should use the EID and network values of an incoming request to
specify the destination for any responses.

#### Bridging/routing

The network and interface structure allows multiple interfaces to share a common
network. By default, packets are not forwarded between interfaces.

A network can be configured for "forwarding" mode. In this mode, packets may be
forwarded if their destination EID is non-local, and matches a route for another
interface on the same network.

As per DSP0236, packet reassembly does not occur during the forwarding process.
If the packet is larger than the MTU for the destination interface/route, then
the packet is dropped.

#### Tag behaviour for transmitted messages

On every message sent with the tag-owner bit set ("TO" in DSP0236), the kernel
must allocate a tag that will uniquely identify responses over a (destination
EID, source EID, tag-owner, tag) tuple. The tag value is 3 bits in size.

To allow this, a `sendto()` with the `MCTP_TAG_OWNER` bit set in the `smctp_tag`
field will cause the kernel to allocate a unique tag for subsequent replies from
that specific remote EID.

This allocation will expire when any of the following occur:

- the socket is closed
- a new message is sent to a new destination EID
- an implementation-defined timeout expires

Because the "tag space" is limited, it may not be possible for the kernel to
allocate a unique tag for the outgoing message. In this case, the `sendto()`
call will fail with errno `EAGAIN`. This is analogous to the UDP behaviour when
a local port cannot be allocated for an outgoing message.

The implementation-defined timeout value shall be chosen to reasonably cover
standard reply timeouts. If necessary, this timeout may be modified through the
`MCTP_TAG_CONTROL` socket option.

For applications that expect to perform an ongoing message exchange with a
particular destination address, they may use the `connect()` call to set a
persistent remote address. In this case, the tag will be allocated during
connect(), and remain reserved for this socket until any of the following occur:

- the socket is closed
- the remote address is changed through another call to `connect()`.

In particular, calling `sendto()` with a different address does not release the
tag reservation.

Broadcast messages are particularly onerous for tag reservations. When a message
is transmitted with TO=1 and dest=0xff (the broadcast EID), the kernel must
reserve the tag across the entire range of possible EIDs. Therefore, a
particular tag value must be currently-unused across all EIDs to allow a
`sendto()` to a broadcast address. Additionally, this reservation is not cleared
when a reply is received, as there may be multiple replies to a broadcast.

For this reason, applications wanting to send to the broadcast address should
use the `connect()` system call to reserve a tag, and guarantee its availability
for future message transmission. Note that this will remove the tag value for
use with _any other EID_. Sending to the broadcast address should be avoided; we
expect few applications will need this functionality.

#### MCTP Control Protocol implementation

Aside from the "Resolve endpoint EID" message, the MCTP control protocol
implementation would exist as a userspace process, `mctpd`. This process is
responsible for responding to incoming control protocol messages, any dynamic
EID allocations (for bus owner devices) and maintaining the MCTP route table
(for bridging devices).

This process would create a socket bound to the type `MCTP_TYPE_CONTROL`, with
the `MCTP_ADDR_EXT` socket option enabled in order to access physical addressing
data on incoming control protocol requests. It would interact with the kernel's
route table via a netlink interface - the same as that implemented for the
[Utility and configuration interfaces](#utility-and-configuration-interfaces).

### Neighbour and routing implementation

The packet-transmission behaviour of the MCTP infrastructure relies on a single
routing table to lookup both route and neighbour information. Entries in this
table are of the format:

| EID range | interface | physical address | metric | MTU | flags | expiry |
| --------- | --------- | ---------------- | ------ | --- | ----- | ------ |

This table can be updated from two sources:

- From userspace, via a netlink interface (see the
  [Utility and configuration interfaces](#utility-and-configuration-interfaces)
  section).

- Directly within the kernel, when basic neighbour information is discovered.
  Kernel-originated routes are marked as such in the flags field, and have a
  maximum validity age, indicated by the expiry field.

Kernel-discovered routing information can originate from two sources:

- physical-to-EID mappings discovered through received packets

- explicit endpoint physical-address resolution requests

When a packet is to be transmitted to an EID that does not have an entry in the
routing table, the kernel may attempt to resolve the physical address of that
endpoint using the Resolve Endpoint ID command of the MCTP Control Protocol
(section 12.9 of DSP0236). The response message will be used to add a
kernel-originated route into the routing table.

This is the only kernel-internal usage of MCTP Control Protocol messages.

## Utility and configuration interfaces

A small utility will be developed to control the state of the kernel MCTP stack.
This will be similar in design to the 'iproute2' tools, which perform a similar
function for the IPv4 and IPv6 protocols.

The utility will be invoked as `mctp`, and provide subcommands for managing
different aspects of the kernel stack.

### `mctp link`: manage interfaces

```sh
    mctp link set <link> <up|down>
    mctp link set <link> network <network-id>
    mctp link set <link> mtu <mtu>
    mctp link set <link> bus-owner <hwaddr>
```

### `mctp network`: manage networks

```sh
    mctp network create <network-id>
    mctp network set <network-id> forwarding <on|off>
    mctp network set <network-id> default [<true|false>]
    mctp network delete <network-id>
```

### `mctp address`: manage local EID assignments

```sh
    mctp address add <eid> dev <link>
    mctp address del <eid> dev <link>
```

### `mctp route`: manage routing tables

```sh
    mctp route add net <network-id> eid <eid|eid-range> via <link> [hwaddr <addr>] [mtu <mtu>] [metric <metric>]
    mctp route del net <network-id> eid <eid|eid-range> via <link> [hwaddr <addr>] [mtu <mtu>] [metric <metric>]
    mctp route show [net <network-id>]
```

### `mctp stat`: query socket status

```sh
    mctp stat
```

A set of netlink message formats will be defined to support these control
functions.

# Design points & alternatives considered

## Including message-type byte in send/receive buffers

This design specifies that message buffers passed to the kernel in send syscalls
and from the kernel in receive syscalls will have the message type byte as the
first byte of the buffer. This corresponds to the definition of a MCTP message
payload in DSP0236.

This somewhat duplicates the type data provided in `struct sockaddr_mctp`; it's
superficially possible for the kernel to prepend this byte on send, and remove
it on receive.

However, the exact format of the MCTP message payload is not precisely defined
by the specification. Particularly, any message integrity check data (which
would also need to be appended / stripped in conjunction with the type byte) is
defined by the type specification, not DSP0236. The kernel would need knowledge
of all protocols in order to correctly deconstruct the payload data.

Therefore, we transfer the message payload as-is to userspace, without any
modification by the kernel.

## MCTP message-type specification: using `sockaddr_mctp.smctp_type` rather than protocol

This design specifies message-types to be passed in the `smctp_type` field of
`struct sockaddr_mctp`. An alternative would be to pass it in the `protocol`
argument of the `socket()` system call:

```c
    int socket(int domain /* = AF_MCTP */, int type /* = SOCK_DGRAM */, int protocol);
```

The `smctp_type` implementation was chosen as it better matches the "addressing"
model of the message type; sockets are bound to an incoming message type,
similar to the IP protocol's model of binding UDP sockets to a local port
number.

There is no kernel behaviour that depends on the specific type (particularly
given the design choice above), so it is not suited to use the protocol argument
here.

Future additions that perform protocol-specific message handling, and so alter
the send/receive buffer format, may use a new protocol argument.

## Networks referenced by index rather than UUID

This design proposes referencing networks by an integer index. The MCTP standard
does optionally associate a RFC4122 UUID with a networks; it would be possible
to use this UUID where we pass a network identifier.

This approach does not incorporate knowledge of network UUIDs in the kernel.
Given that the Get Network ID message in the MCTP Control Protocol is
implemented entirely via userspace, it does not need to be aware of network
UUIDs, and requiring network references (for example, the `smctp_network` field
of `struct sockaddr_mctp`, as type `uuid_t`) complicates assignment.

Instead, the index integer is used instead, in a similar fashion to the integer
index used to reference `struct netdevice`s elsewhere in the network stack.

## Tag behaviour alternatives

We considered _several_ different designs for the tag handling behaviour. A
brief overview of the more-feasible of those, and why they were rejected:

### Each socket is allocated a unique tag value on creation

We could allocate a tag for each socket on creation, and use that value when a
tag is required. This, however:

- needlessly consumes a tag on non-tag-owning sockets (ie, those which send with
  TO=0 - responders); and

- limits us to 8 sockets per network.

### Tags only used for message packetisation / reassembly

An alternative would be to completely dissociate tag allocation from sockets;
and only allocate a tag for the (short-lived) task of packetising a message, and
sending those packets. Tags would be released when the last packet has been
sent.

However, this removes any facility to correlate responses with the correct
socket, which is the purpose of the TO bit in DSP0236. In order for the sending
application to receive the response, we would either need to:

- limit the system to one socket of each message type (which, for example,
  precludes running a requester and a responder of the same type); or

- forward all incoming messages of a specific message-type to all sockets
  listening on that type, making it trivial to eavesdrop on MCTP data of other
  applications

### Allocate a tag for one request/response pair

Another alternative would be to allocate a tag on each outgoing TO=1 message,
and then release that allocation after the incoming response to that tag (TO=0)
is observed.

However, MCTP protocols exist that do not have a 1:1 mapping of responses to
requests - more than one response may be valid for a given request message. For
example, in response to a request, a NVMe-MI implementation may send an
in-progress reply before the final reply. In this case, we would release the tag
after the first response is received, and then have no way to correlate the
second message with the socket.

Broadcast MCTP request messages may have multiple replies from multiple
endpoints, meaning we cannot release the tag allocation on the first reply.
