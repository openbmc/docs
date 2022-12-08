# OpenBMC platform communication channel: MCTP & PLDM

Author: Jeremy Kerr <jk@ozlabs.org> <jk>

## Problem Description

Currently, we have a few different methods of communication between host and
BMC. This is primarily IPMI-based, but also includes a few hardware-specific
side-channels, like hiomap. On OpenPOWER hardware at least, we've definitely
started to hit some of the limitations of IPMI (for example, we have need
for >255 sensors), as well as the hardware channels that IPMI typically uses.

This design aims to use the Management Component Transport Protocol (MCTP) to
provide a common transport layer over the multiple channels that OpenBMC
platforms provide. Then, on top of MCTP, we have the opportunity to move to
newer host/BMC messaging protocols to overcome some of the limitations we've
encountered with IPMI.

## Background and References

Separating the "transport" and "messaging protocol" parts of the current stack
allows us to design these parts separately. Currently, IPMI defines both of
these; we currently have BT and KCS (both defined as part of the IPMI 2.0
standard) as the transports, and IPMI itself as the messaging protocol.

Some efforts of improving the hardware transport mechanism of IPMI have been
attempted, but not in a cross-implementation manner so far. This does not
address some of the limitations of the IPMI data model.

MCTP defines a standard transport protocol, plus a number of separate physical
layer bindings for the actual transport of MCTP packets. These are defined by
the DMTF's Platform Management Working group; standards are available at:

https://www.dmtf.org/standards/pmci

The following diagram shows how these standards map to the areas of
functionality that we may want to implement for OpenBMC. The DSP numbers
provided are references to DMTF standard documents.

![](mctp-standards.svg)

One of the key concepts here is that separation of transport protocol from the
physical layer bindings; this means that an MCTP "stack" may be using either a
I2C, PCI, Serial or custom hardware channel, without the higher layers of that
stack needing to be aware of the hardware implementation. These higher levels
only need to be aware that they are communicating with a certain entity, defined
by an Entity ID (MCTP EID). These entities may be any element of the platform
that communicates over MCTP - for example, the host device, the BMC, or any
other system peripheral - static or hot-pluggable.

This document is focused on the "transport" part of the platform design. While
this does enable new messaging protocols (mainly PLDM), those components are not
covered in detail much; we will propose those parts in separate design efforts.
For example, the PLDM design at [pldm-stack.md].

As part of the design, the references to MCTP "messages" and "packets" are
intentional, to match the definitions in the MCTP standard. MCTP messages are
the higher-level data transferred between MCTP endpoints, which packets are
typically smaller, and are what is sent over the hardware. Messages that are
larger than the hardware Maximum Transmit Unit (MTU) are split into individual
packets by the transmit implementation, and reassembled at the receive
implementation.

## Requirements

Any channel between host and BMC should:

- Have a simple serialisation and deserialisation format, to enable
  implementations in host firmware, which have widely varying runtime
  capabilities

- Allow different hardware channels, as we have a wide variety of target
  platforms for OpenBMC

- Be usable over simple hardware implementations, but have a facility for higher
  bandwidth messaging on platforms that require it.

- Ideally, integrate with newer messaging protocols

## Proposed Designs

The MCTP infrastrcuture in OpenBMC is implemented in two approaches:

- A userspace-based approach, using a core library, plus a demultiplexing
  daemon. This is described in [MCTP Userspace](mctp-userspace.md).

  This is in use by a few platforms, but should be avoided for new designs.

- A kernel-based approach, using a sockets API for client and server
  applications. This approach is recommended for new designs, and is described
  in [MCTP Kernel](mctp-kernel.md)

Design details for both approaches are covered in their relevant documents, but
both share the same Problem Description, Background and Requirements,
Alternatives and Impacts sections as defined by this document.

## Alternatives Considered

There have been two main alternatives to an MCTP implementation in OpenBMC:

Continue using IPMI, but start making more use of OEM extensions to suit the
requirements of new platforms. However, given that the IPMI standard is no
longer under active development, we would likely end up with a large amount of
platform-specific customisations. This also does not solve the hardware channel
issues in a standard manner.

Redfish between host and BMC. This would mean that host firmware needs a HTTP
client, a TCP/IP stack, a JSON (de)serialiser, and support for Redfish schema.
While this may be present in some environments (for example, UEFI-based
firmware), this is may not be feasible for all host firmware implementations
(for example, OpenPOWER). It's possible that we could run a simplified Redfish
stack - indeed, MCTP has a proposal for a Redfish-over-MCTP channel (DSP0218),
which uses simplified serialisation format and no requirement on HTTP. However,
this may involve a large amount of complexity in host firmware.

## Impacts

Development would be required to implement the MCTP transport, plus any new
users of the MCTP messaging (eg, a PLDM implementation). These would somewhat
duplicate the work we have in IPMI handlers.

We'd want to keep IPMI running in parallel, so the "upgrade" path should be
fairly straightforward.

Design and development needs to involve potential host, management controllers
and managed device implementations.
