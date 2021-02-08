# OpenBMC in-kernel MCTP

Author: Jeremy Kerr <jk@codeconstruct.com.au>

Please refer to the [MCTP Overview](mctp.md) document for general MCTP design
description, background and requirements.

This document describes a kernel-based implementation of MCTP infrastructure,
providing a sockets-based API for MCTP communication within an OpenBMC-based
platform.

# Requirements for a kernel implementation

 * The MCTP messaging API should be an obvious application of the existing POSIX
   socket interface

 * Configuration should be simple for a straightforward MCTP endpoint: a single
   network with a single local EID.

 * Infrastructure should be flexible enough to allow for more complex MCTP
   networks, allowing:

    - each MCTP network (as defined by section 3.2.31 of DSP0236) may
      consist of multiple local physical interfaces, and/or multiple EIDs;

    - multiple distinct (non-bridged) networks, possibly containing duplicated
      EIDs between networks;

    - multiple local EIDs on a single interface, and

    - customisable routing configurations within a network.


# Proposed Design #

The design contains several components:

 * An interface for userspace applications to send and receive MCTP messages: A
   mapping of the sockets API to MCTP usage

 * Infrastructure for control and configuration of the MCTP network(s),
   consisting of a configuration utility, and a kernel messaging facility for
   this utility to use.

 * Kernel drivers for physical interface bindings.

In general, the kernel components cover the transport functionality of MCTP,
such as message assembly/disassembly, packet forwarding, and physical interface
implementations.

Higher-level protocols (such as PLDM) are implemented in userspace, through the
introduced socket API. This also includes the majority of the MCTP Control
Protocol implementation (DSP0236, section 11) - MCTP endpoints will typically
have a specific process to request and respond to control protocol messages.
However, the kernel will include a small subset of control protocol code to
allow very simple endpoints to run without this process.

## Structure: interfaces & networks #

The kernel models the local MCTP topology through two items: interfaces and
networks.

An interface (or "link") is an instance of an MCTP physical transport binding
(as defined by DSP0236, section 3.2.47), likely connected to a specific hardware
device. This is represented as a `struct netdevice`, and has a user-visible
name and index (`ifindex`).

A network defines a unique address space for MCTP endpoints by endpoint-ID (EID)
(described by DSP0236, section 3.2.31). A network has a user-visible ID and
UUID. Route definitions are specific to one network.

Interfaces are associated with one network. A network may be associated with one
or more interfaces.

If multiple networks are present, each may contain EIDs that are also present on
other networks.

## Sockets API ##

### Protocol definitions ###

We define a new address family (and corresponding protocol family) for MCTP:

```c
    #define AF_MCTP /* TBD */
    #define PF_MCTP AF_MCTP
```

MCTP sockets are created with the `socket()` syscall, specifying `AF_MCTP` as
the domain. Currently, only a `SOCK_DGRAM` socket type is defined.

```c
    int sd = socket(AF_MCTP, SOCK_DGRAM, protocol);
```

The `protocol` argument maps to the DSP0236/DSP0239 standard message types, or
an "any" type: `MCTP_PROTO_ANY`.

Sockets opened with a protocol value of ANY will communicate directly at the
transport layer; message buffers received by the application will consist of
message data from reassembled MCTP packets, and will include the full message
including message type byte and optional message integrity check (IC).
Individual packet headers are not included; they may be accessible through a
future `SOCK_RAW` socket type.

Sockets opened with a specific protocol defined will communicate at the
presentation layer; message buffers received and sent by the application will
not include the message type byte, nor the message-type defined IC. These are
implied, and handled, by the kernel protocol support.

As with all socket address families, source and destination addresses are
specified with a new `sockaddr` type:

```c
    struct sockaddr_mctp {
            sa_family_t         smctp_family; /* = AF_MCTP */
            int                 smctp_network;
            struct mctp_addr    smctp_addr;
            uint8_t             smcp_tag;
    };

    struct mctp_addr {
            uint8_t             s_addr;
    };

    /* Special network values */
    #define MCTP_NET_ANY        0
    #define MCTP_ADDR_ANY       0xff
    #define MCTP_ADDR_BCAST     0xff

    /* MCTP tag defintions; used for smcp_tag field of sockaddr_mctp */
    /* MCTP-spec-defined fields */
    #define MCTP_TAG_MASK    0x07
    #define MCTP_TAG_OWNER   0x08
    /* Others: reserved */

    /* Helpers */
    #define MCTP_TAG_RSP(x) (x & MCTP_TAG_MASK) /* response to a request: clear TO, keep value */
```

### Syscall behaviour ###

The following sections describe the MCTP-specific behaviours of the standard
socket system calls. These behaviours have been chosen to map closely to the
existing sockets APIs.

#### `bind()`: set local socket address ####

Sockets that receive incoming request packets will bind to a local address,
using the `bind()` syscall.

```c
    struct sockaddr_mctp addr;

    addr.smctp_family = AF_MCTP;
    addr.smctp_network = MCTP_NET_ANY;
    addr.smctp_addr.s_addr = MCTP_ADDR_ANY;

    int rc = bind(sd, (struct sockaddr *)&addr, sizeof(addr));
```

This establishes the local address of the socket. Only the `smctp_network` and
`sctp_addr` fields of the address are used; the `smctp_tag` value is ignored.
Incoming MCTP messages that match the network, address, and message type (if
specified on the original `socket()` call) will be received by this socket.

A `smctp_network` value of `MCTP_NET_ANY` will configure the socket to receive
incoming packets from any locally-connected network. A specific network value
will cause the socket to only receive incoming messages from that network.

The `smctp_addr` field specifies a local address to bind to. A value of
`MCTP_ADDR_ANY` configures the socket to receive messages addressed to any
local destination EID.

#### `connect()`: set remote socket address ####

Sockets may specify a socket's remote address with the `connect()` syscall:

```c
    struct sockaddr_mctp addr;
    int rc;

    addr.smctp_family = AF_MCTP;
    addr.smctp_network = MCTP_NET_ANY;
    addr.smctp_addr.s_addr = 8;
    addr.smctp_tag = MCTP_TAG_OWNER;

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

The `smctp_tag` value will configure the tag used for the local side of this
socket. If the `MCTP_TAG_OWNER` bit is set in this field, the `MCTP_TAG_VALUE`
bits are ignored, and will be set by the kernel. See the [Tag behaviour for
transmitted messages](#tag-behaviour-for-transmitted-messages) section for more
details. Specifying an "owned" tag here (ie, `MCTP_TAG_OWNER`) will allocate a
tag to the socket for all future outgoing messages, until either the socket is
closed, or `connect()` is called again. If a tag cannot be allocated,
`connect()` will report an error, with an errno value of `EAGAIN`.

#### `sendto()`, `sendmsg()`, `send()` & `write()`: transmit an MCTP message ####

An MCTP message is transmitted using one of the `sendto()`, `sendmsg()`, `send()`
or `write()` syscalls. Using `sendto()` as the primary example:

```c
    struct sockaddr_mctp addr;
    ssize_t len;
    char *buf;

    /* set message destination */
    addr.smctp_family = AF_MCTP;
    addr.smctp_network = 0;
    addr.smctp_addr.s_addr = 8;
    addr.smctp_tag = MCTP_TAG_OWNER;

    /* arbitrary message to send */
    buf = "hello, world!";

    len = sendto(sd, buf, strlen(buf), 0,
                    (struct sockaddr_mctp *)&addr, sizeof(addr));
```

The address argument is treated the same way as for `connect()`: The network and
address fields define the remote address to send to. If `smctp_tag` has the
`MCTP_TAG_OWNER`, the kernel will ignore any bits set in `MCTP_TAG_VALUE`, and
generate a tag value. If `MCTP_TAG_OWNER` is not set, the message will be sent
with the tag value as specified. If a tag value cannot be allocated, the
system call will report an errno of `EAGAIN`.

For sockets created with `MCTP_PROTO_ANY`, the application must specify the
message type in the first byte of the message buffer passed to `sendto()`. For
other protocols, the message type byte will be prepended by the kernel.

The `send()` and `write()` system calls behave in a similar way, but do not
specify a remote address. Therefore, `connect()` must be called beforehand; if
not, these calls will return an error of `EDESTADDRREQ` (Destination address
required).

Using `sendto()` or `sendmsg()` on a connected socket will override the remote
socket address specified in `connect()`. The `connect()` address will remain
associated with the socket, for future unaddressed sends. This includes any tag
allocation caused as a result.

The `sendmsg()` system call allows a more compact argument interface, and the
message buffer to be specified as a scatter-gather list. At present no
ancillary message types (used for the `msg_control` data passed to `sendmsg()`)
are defined.

#### `recvfrom()`, `recvmsg()`, `recv()` & `read()`: receive an MCTP message ####

An MCTP message can be received by an application using one of the `recvfrom()`,
`recvmsg()`, `recv()` or `read()` system calls. Using `recvfrom()` as the
primary example:

```c
    struct sockaddr_mctp addr;
    socklen_t addrlen;
    char buf[13];
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

For sockets created with `MCTP_PROTO_ANY`, the first byte of the message buffer
will contain the message type byte. For other protocols, the message type byte
is omitted.

The `recv()` and `read()` system calls behave in a similar way, but do not
provide a remote address to the application. Therefore, these are only useful if
the remote address is already known, or the message does not require a reply.

#### `getsockname()`: query local socket address ####

The `getsockname()` system call returns the `struct sockaddr_mctp` value for the
local side of this socket. This may be used to retrieve a kernel-allocated tag
value.

#### Socket options ####

The following socket options are defined for MCTP sockets:

##### `MCTP_ADDR_EXT`: Use extended addressing information in sendmsg/recvmsg #####

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

#### Syscalls not implemented ####

The following system calls are not implemented for MCTP, primarily as they are
not used in `SOCK_DGRAM`-type sockets:

 * `listen()`
 * `accept()`
 * `ioctl()`
 * `shutdown()`
 * `mmap()`

### Userspace examples ###

These examples cover three general use-cases:

 - **requester**: sends requests to a particular (EID, type) target, and
   receives responses to those packets

   This is similar to a typical UDP client

 - **responder**: receives all locally-addressed messages of a specific
   message-type, and responds to the requester immediately.

   This is similar to a typical UDP server

 - **controller**: a specific service for a bus owner; may send broadcast
   messages, manage EID allocations, update local MCTP stack state. Will
   need low-level packet data.

   This is similar to a DHCP server.

An example of a socket using `MCTP_PROTO_ANY` is included to illustrate the
message-type-byte behaviour.

#### Requester ####

"Client"-side implementation to send requests to a responder, and receive a response.
This uses a (fictitious) message type of `MCTP_PROTO_ECHO`. Since we've
specified a proto argument to sock, we will not need to prefix outgoing messages
with the type byte, and incoming messages will be filtered on a type match.

```c
    int main() {
            struct sockaddr_mctp addr;
            socklen_t addrlen;
            uint8_t msg[16];
            int sd, rc;

            sd = socket(AF_MCTP, SOCK_DGRAM, MCTP_PROTO_ECHO);

            addr.sa_family = AF_MCTP;
            addr.smctp_network = MCTP_NET_ANY; /* any network */
            addr.smctp_addr = 9;    /* remote eid 9 */
            addr.smctp_tag = MCTP_TAG_OWNER_ANY; /* any tag, but we're the owner */
            addrlen = sizeof(addr);

            strncpy(msg, "hello, world!", sizeof(msg));

            /* send message */
            rc = sendto(sd, msg, sizeof(msg), 0,
                            (struct sockaddr *)&addr, addrlen);

            if (rc < 0)
                    err(EXIT_FAILURE, "sendto");

            /* receive reply */
            rc = recvfrom(sd, msg, sizeof(msg) - 1, 0,
                        (struct sockaddr *)&addr, &addrlen);
            if (rc < 0)
                    err(EXIT_FAILURE, "recvfrom");

            msg[sizeof(msg)-1] = '\0';

            printf("reply: %s\n", msg);

            return EXIT_SUCCESS;
    }
```

#### Responder ####

"Server"-side implementation to receive requests and respond. Like the client,
This uses a (fictitious) message type of `MCTP_PROTO_ECHO`, as the proto
argument to socket(); only messages matching this type will be received. The
type byte will be stripped from incoming messages, and prepended to outgoing
data.

```c
    int main() {
            struct sockaddr_mctp addr;
            socklen_t addrlen;
            int sd, rc;

            sd = socket(AF_MCTP, SOCK_DGRAM, MCTP_PROTO_ECHO);

            addr.sa_family = AF_MCTP;
            addr.smctp_network = MCTP_NET_ANY; /* any network */
            addr.smctp_addr = MCTP_EID_ANY;
            addr.smctp_tag = 0; /* ignored for bind */
            addrlen = sizeof(addr);

            rc = bind(sd, (struct sockaddr *)&addr, addrlen);
            if (rc)
                    err(EXIT_FAILURE, "bind");

            for (;;) {
                    uint8_t msg[16];

                    rc = recvfrom(sd, msg, sizeof(msg), 0,
                                    (struct sockaddr *)&addr, &addrlen);
                    if (rc < 0)
                            err(EXIT_FAILURE, "recvfrom");

                    assert(addrlen == sizeof(addr));

                    printf("%zd bytes from EID %d\n", rc, addr.smctp_addr);

                    /* Reply to requester; this macro just clears the TO-bit.
                     * Other addr fields will describe the remote endpoint,
                     * so use those as-is.
                     */
                    addr.smctp_tag = MCTP_TAG_RSP(addr.smctp_tag);

                    rc = sendto(sd, msg, rc, 0,
                                (struct sockaddr *)&addr, addrlen);
                    if (rc < 0)
                            err(EXIT_FAILURE, "sendto");
            }

            return EXIT_SUCCESS;
    }
```

#### Broadcast request ####

Sends a request to a broadcast EID, and receives (unicast) replies. Typical
control protocol pattern.

```c
    int main() {
            struct sockaddr_mctp txaddr, rxaddr;
            socklen_t addrlen;
            uint8_t buf;

            sd = socket(AF_MCTP, SOCK_DGRAM, MCTP_PROTO_CONTROL);

            /* destination address setup */
            txaddr.sa_family = AF_MCTP;
            txaddr.smctp_network = 1; /* specific network required for broadcast */
            txaddr.smctp_addr = MCTP_TAG_BCAST; /* broadcast dest */
            txaddr.smctp_tag = MCTP_TAG_OWNER_ANY; /* any tag, but we're the owner */

            buf = 'a';

            rc = sendto(sd, &buf, 1, 0, (struct sockaddr *)&txaddr,
                            sizeof(txaddr));
            if (rc < 0)
                    err(EXIT_FAILURE, "sendto");

            addrlen = sizeof(rxaddr);
            for (;;) {
                    rc = recvfrom(sd, &buf, 1, 0, (struct sockaddr *)&rxaddr,
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

#### Single-application multi-type listener ####

Uses a `MCTP_PROTO_ANY` socket to receive all locally-bound messages. RX & TX
messages will all have a `uint8_t` type prefix.

```c
    int main() {
            struct sockaddr_mctp addr;
            socklen_t addrlen;
            struct {
                uint8_t type;
                uint8_t data[MAX_DATA];
            } req, *resp;
            int sd, rc;

            sd = socket(AF_MCTP, SOCK_DGRAM, MCTP_PROTO_ANY);

            addr.sa_family = AF_MCTP;
            addr.smctp_network = MCTP_NET_ANY;
            addr.smctp_addr = MCTP_EID_ANY;
            addr.smctp_tag = 0; /* ignored for bind */
            addrlen = sizeof(addr);

            rc = bind(sd, (struct sockaddr *)&addr, addrlen);
            if (rc)
                    err(EXIT_FAILURE, "bind");

                (;;) {
                    uint8_t buf[16];

                    rc = recvfrom(sd, &req, sizeof(req), 0,
                                    (struct sockaddr *)&addr, &addrlen);
                    if (rc < 0)
                            err(EXIT_FAILURE, "recvfrom");

                    assert(addrlen == sizeof(addr));

                    printf("%zd bytes from EID %d\n", rc, addr.smctp_addr);

                    /* handle req, possibly demultiplexing on req->type */
                    resp = handler(req.type, req.data, rc - sizeof(req.type));

                    /* reply to requester; this macro just clears the TO-bit */
                    addr.smctp_tag = MCTP_TAG_RSP(addr.smctp_tag);

                    rc = sendto(sd, resp, rc, 0,
                                (struct sockaddr *)&addr, addrlen);
                    if (rc < 0)
                            err(EXIT_FAILURE, "sendto");

                    free(resp);
            }

            return EXIT_SUCCESS;
    }
```


### Implementation notes ###

#### Addressing ####

Transmitted messages (through `sendto()` and related system calls) specify their
destination via the `smctp_network` and `smctp_addr` fields of a `struct
sockaddr_mctp`.

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

#### Bridging/routing ####

The network and interface structure allows multiple interfaces to share a common
network. By default, packets are not forwarded between interfaces.

A network can be configured for "forwarding" mode. In this mode, packets may be
forwarded if their destination EID is non-local, and matches a route for another
interface on the same network.

As per DSP0236, packet reassembly does not occur during the forwarding process.
If the packet is larger than the MTU for the destination interface/route, then
the packet is dropped.

#### Tag behaviour for transmitted messages ####

On every message sent with the tag-owner bit set ("TO" in DSP0236), the kernel
must allocate a tag that will uniquely identify responses over a (endpoint,
source address, tag) tuple. The tag value is 3 bits in size.

To allow this, a `sendto()` with the `MCTP_TAG_OWNER` bit set in the `smctp_tag`
field will cause the kernel to allocate a unique tag for subsequent replies from
that specific remote EID.

This allocation will expire when any of the following occur:

 * the reply is received
 * the socket is closed
 * a timeout expires

Because the "tag space" is limited, it may not be possible for the kernel to
allocate a unique tag for the outgoing message. In this case, the `sendto()`
call will fail with errno `EAGAIN`. This is analogous to the UDP behaviour when
a local port cannot be allocated for an outgoing message.

For applications that expect to perform an ongoing message exchange with a
particular destination address, they may use the `connect()` call to set a
persistent remote address. In this case, the tag will be allocated during
connect(), and remain reserved for this socket until any of the following occur:

 * the socket is closed
 * the remote address is changed through another call to `connect()`.

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
use with any other EID. Sending to the broadcast address should be avoided; we
expect few applications will need this functionality.


### Neighbour and routing implementation ###

The packet-transmission behaviour of the MCTP infrastructure relies on a single
routing table to lookup both route and neighbour information. Entries in this
table are of the format:

 | EID range | interface | physical address | metric | MTU | flags | expiry |
 |-----------|-----------|------------------|--------|-----|-------|--------|

This table can be updated from two sources:

  * From userspace, via a netlink interface (see the
    [Utility and configuration interfaces](#utility-and-configuration-interfaces)
    section).

  * Directly within the kernel, when basic neighbour information is discovered.
    Kernel-originated routes are marked as such in the flags field, and have a
    maximum validity age, indicated by the expiry field.

Kernel-discovered routing information can originate from two sources:

  * physical-to-EID mappings discovered through received packets

  * explicit endpoint physical-address resolution requests

When a packet is to be transmitted to an EID that does not have an entry in the
routing table, the kernel may attempt to resolve the physical address of that
endpoint using the Resolve Endpoint ID command of the MCTP Control Protocol
(section 12.9 of DSP0236). The response message will be used to add a
kernel-originated route into the routing table.

This is the only kernel-internal usage of MCTP Control Protocol messages.

## Utility and configuration interfaces ##

A small utility will be developed to control the state of the kernel MCTP stack.
This will be similar in design to the 'iproute2' tools, which perform a similar
function for the IPv4 and IPv6 protocols.

The utility will be invoked as `mctp`, and provide subcommands for managing
different aspects of the kernel stack.

### `mctp link`: manage interfaces ###

```sh
    mctp link set <link> <up|down>
    mctp link set <link> network <network-id>
    mctp link set <link> mtu <mtu>
    mctp link set <link> bus-owner <hwaddr>
```

### `mctp network`: manage networks ###

```sh
    mctp network create <network-id>
    mctp network set <network-id> forwarding <on|off>
    mctp network set <network-id> default [<true|false>]
    mctp network delete <network-id>
```

### `mctp address`: manage local EID assignments ###

```sh
    mctp address add <eid> dev <link>
    mctp address del <eid> dev <link>
```

### `mctp route`: manage routing tables ###

```sh
    mctp route add net <network-id> eid <eid|eid-range> via <link> [hwaddr <addr>] [mtu <mtu>] [metric <metric>]
    mctp route del net <network-id> eid <eid|eid-range> via <link> [hwaddr <addr>] [mtu <mtu>] [metric <metric>]
    mctp route show [net <network-id>]
```

### `mctp stat`: query socket status ###

```sh
    mctp stat
```

A set of netlink message formats will be defined to support these control
functions.
