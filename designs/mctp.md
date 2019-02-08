# Host/BMC communication channel: MCTP & PLDM

Author: Jeremy Kerr <jk@ozlabs.org> <jk>

## Problem Description

Currently, we have a few different methods of communication between host
and BMC. This is primarily IPMI-based, but also includes a few
hardware-specific side-channels, like hiomap. On OpenPOWER hardware at
least, we've definitely started to hit some of the limitations of IPMI
(for example, we have need for >255 sensors), as well as the hardware
channels that IPMI typically uses.

This design aims to use the Management Component Transport Protocol
(MCTP) to provide a common transport layer over the multiple channels
that OpenBMC platforms provide. Then, on top of MCTP, we have the
opportunity to move to newer host/BMC messaging protocols to overcome
some of the limitations we've encountered with IPMI.

## Background and References

Separating the "transport" and "messaging protocol" parts of the current
stack allows us to design these parts separately. Currently, IPMI
defines both of these; we currently have BT and KCS (both defined as
part of the IPMI 2.0 standard) as the transports, and IPMI itself as the
messaging protocol.

Some efforts of improving the hardware transport mechanism of IPMI have
been attempted, but not in a cross-implementation manner so far. This
does not address some of the limitations of the IPMI data model.

MCTP defines a standard transport protocol, plus a number of separate
hardware bindings for the actual transport of MCTP packets. These are
defined by the DMTF's Platform Management Working group; standards are
available at:

  https://www.dmtf.org/standards/pmci

The following diagram shows how these standards map to the areas of
functionality that we may want to implement for OpenBMC. The DSP numbers
provided are references to DMTF standard documents.

![](mctp-standards.svg)

One of the key concepts here is that separation of transport protocol
from the hardware bindings; this means that an MCTP "stack" may be using
either a I2C, PCI, Serial or custom hardware channel, without the higher
layers of that stack needing to be aware of the hardware implementation.
These higher levels only need to be aware that they are communicating
with a certain entity, defined by an Entity ID (MCTP EID).

I've mainly focussed on the "transport" part of the design here. While
this does enable new messaging protocols (mainly PLDM), I haven't
covered that much; we will propose those details for a separate design
effort.

As part of the design, I have referred to MCTP "messages" and "packets";
this is intentional, to match the definitions in the MCTP standard. MCTP
messages are the higher-level data transferred between MCTP endpoints,
which packets are typically smaller, and are what is sent over the
hardware. Messages that are larger than the hardware MTU are split into
individual packets by the transmit implementation, and reassembled at
the receive implementation.

A final important point is that this design is intended for internal BMC
channels (eg host <--> BMC or device <--> BMC) *only*. Even if we do
replace IPMI for internal interfaces, we intend to allow platforms to
continue to provide an IPMI/Redfish/etc interface for external system
management.

## Requirements

Any channel between host and BMC should:

 - Have a simple serialisation and deserialisation format, to enable
   implementations in host firmware, which have widely varying runtime
   capabilities

 - Allow different hardware channels, as we have a wide variety of
   target platforms for OpenBMC

 - Be usable over simple hardware implementations, but have a facility
   for higher bandwidth messaging on platforms that require it.

 - Ideally, integrate with newer messaging protocols

## Proposed Design

The MCTP core specification just provides the packetisation, routing and
addressing mechanisms. The actual transmit/receive of those packets is
up to the hardware binding of the MCTP transport.

For OpenBMC, we would introduce an MCTP daemon, which implements the
transport over a configurable hardware channel (eg., Serial UART, I2C or
PCI). This daemon is responsible for the packetisation and routing of
MCTP messages to and from host firmware.

I see two options for the "inbound" or "application" interface of the
MCTP daemon:

 - it could handle upper parts of the stack (eg PLDM) directly, through
   in-process handlers that register for certain MCTP message types; or

 - it could channel raw MCTP messages (reassembled from MCTP packets) to
   DBUS messages (similar to the current IPMI host daemons), where the
   upper layers receive and act on those DBUS events.

I have a preference for the former, but I would be interested to hear
from the IPMI folks about how the latter structure has worked in the
past.

The proposed implementation here is to produce an MCTP "library" which
provides the packetisation and routing functions, between:

 - an "upper" messaging transmit/receive interface, for tx/rx of a full
   message to a specific endpoint

 - a "lower" hardware binding for transmit/receive of individual
   packets, providing a method for the core to tx/rx each packet to
   hardware

The lower interface would be plugged in to one of a number of
hardware-specific binding implementations (most of which would be
included in the library source tree, but others can be plugged-in too)

The reason for a library is to allow the same MCTP implementation to be
used in both OpenBMC and host firmware; the library should be
bidirectional. To allow this, the library would be written in portable C
(structured in a way that can be compiled as "extern C" in C++
codebases), and be able to be configured to suit those runtime
environments (for example, POSIX IO may not be available on all
platforms; we should be able to compile the library to suit). The
licence for the library should also allow this re-use; I'd suggest a
dual Apache & GPL licence.

As for the hardware bindings, we would want to implement a serial
transport binding first, to allow easy prototyping in simulation. For
OpenPOWER, we'd want to implement a "raw LPC" binding for better
performance, and later PCIe for large transfers. I imagine that there is
a need for an I2C binding implementation for other hardware platforms
too.

Lastly, I don't want to exclude any currently-used interfaces by
implementing MCTP - this should be an optional component of OpenBMC, and
not require platforms to implement it.

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

Development would be required to implement the MCTP transport, plus any
new users of the MCTP messaging (eg, a PLDM implementation). These would
somewhat duplicate the work we have in IPMI handlers.

We'd want to keep IPMI running in parallel, so the "upgrade" path should
be fairly straightforward.

Design and development needs to involve potential host firmware
implementations.

## Testing

For the core MCTP library, we are able to run tests there in complete
isolation (I have already been able to run a prototype MCTP stack
through the afl fuzzer) to ensure that the core transport protocol
works.

For MCTP hardware bindings, we would develop channel-specific tests that
would be run in CI on both host and BMC.

For the OpenBMC MCTP daemon implementation, testing models would depend
on the structure we adopt in the design section.
