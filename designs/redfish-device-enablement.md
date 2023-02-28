# Redfish Device Enablement- RDE Daemon & Requester Library

Author: Harsh Tyagi !harshtya

Other contributors: Kasun Athukorala

Created: 2023-02-27

## Problem Description

Implementing Redfish Device Enablement (RDE) with openBMC as specified by the
following DMTF spec:
https://www.dmtf.org/sites/default/files/standards/documents/DSP0218_1.1.0.pdf

The intent of this design doc is to summarize the Redfish Device Enablement
(RDE) implementation in OpenBMC. RDE uses PLDM over MCTP (kernel) as protocols
for communication. This implementation has two components to it:

- Requester library (**Repo: libpldm**) that would be responsible for providing
  the next RDE command in sequence required to execute a particular RDE
  operation such as discovery, read operation, etc.

- RDE Daemon (**Repo: pldm**) would be the primary consumer of the requester
  library. RDE daemon would be responsible for setting up the RDE stack, i.e.,
  doing the PLDM base discovery, RDE discovery and providing handlers to perform
  equivalent CURD operations with RDE.

```
                                                     Steps followed by req lib
                      LIBPldm Req Library                    components
                   |----------------------|          |-------------------------|
                1  │ ┌──────────────────┐ │          │1 ┌──────────────────┐   │
              ┌────┼─► PLDM Base Disc.  │ │          │ ─► getNextCommand() │   │
              │    │ └──────────────────┘ │          │  └──────────────────┘   │
              │    │                      │          │                         │
┌──────────┐  │ 2  │ ┌──────────────────┐ │    ────► │2 ┌──────────────────┐   │
│RDE Daemon├──┼────┼─►  RDE Discovery   │ │          │ ─► updateContext()  │   |
└──────────┘  │    │ └──────────────────┘ │          │  └──────────────────┘   │
              │    │                      │          │                         │
              │ 3  │ ┌──────────────────┐ │          │3 ┌──────────────────┐   │
              └────┼─►  RDE Operation   │ │          │ ─► sendResultsBack()│   │
                   │ └──────────────────┘ │          │  └──────────────────┘   |
                   |----------------------|          |-------------------------|
```

**Why we need a separate requester library?**

This library is a general purpose solution for setting up the RDE stack. For
instance, the requester would not have to take care of what all operations are
required for performing the base or RDE discovery, rather just make a call to
base and RDE discovery handlers from the library and ask for the next command in
sequence to be executed. This makes the requester independent of the sequence of
operations required to perform a particular RDE operation.

Another benefit of using this library is that it is written in C which makes it
very light weight to be reused by any device that wants to leverage RDE.

## Background and References

- PLDM Base specification:
  https://www.dmtf.org/sites/default/files/standards/documents/DSP0240_1.1.0.pdf
- PLDM RDE specification:
  https://www.dmtf.org/sites/default/files/standards/documents/DSP0218_1.1.2.pdf

## Requirements

- BMC shall support RDE operations including RDE/PLDM discovery and RDE
  operations.
- RDE Requester Library should take in request objects and pack the next RDE
  command in sequence in it so that the requester can send the request to RDE
  device.
- RDE Daemon should leverage the general-purpose RDE requester library for
  providing the correct sequence of commands to perform a RDE operations.

## Proposed Design

Requester Library would be a set of files written in C residing in libpldm repo
that would provide the correct sequence of commands for RDE operations to work.
`[Location: libpldm/src/requester/rde]`

Requester Library can be primarily bifurcated into, but is not limited to, the
following:

- pldm_base_discovery_requester.c: This file will take care of all the
  operations with respect to the PLDM Base discovery, which is required to setup
  the RDE stack:

  - GetTid
  - GetPLDMTypes
  - GetPLDMVersion
  - GetPLDMCommands

- pldm_rde_discovery_requester.c: This file will take care of all the operations
  with respect to the PLDM RDE Discovery which are required to setup the RDE
  stack:

  - NegotiateRedfishParameters
  - NegotiateMediumParameters
  - Getting dictionaries

- pldm_rde_operation_requester.c: This file will take care of handling all the
  CURD operations related to RDE

### Responsibilities of the requester library

- Maintaining a context for current command being executed and related values
- init_context(s): Initializing a context before begining PLDM base discovery or
  RDE discovery etc.
- get_next_command: Gets the next command in sequence for RDE operation
- push_response: Update the context, or send the values back to the requester

**Note: This library can be extended or modified based on the need and the
feedback provided and is not limited to the above description.**

## Alternatives Considered

## Impacts

## Testing

Testing will be done using a QEMU connected to a RDE device (dev board)
