# Redfish Device Enablement Requester Library

Author: Harsh Tyagi !harshtya

Other contributors: Kasun Athukorala

Created: 2023-02-27

## Problem Description

Implementing Redfish Device Enablement (RDE) with openBMC as specified by the
following DMTF spec:
https://www.dmtf.org/sites/default/files/standards/documents/DSP0218_1.1.0.pdf

The intent of this design doc is to summarize the Redfish Device Enablement
(RDE) requester library that would be responsible for providing the next RDE
operation in sequence required to achieve a particular RDE task such as
discovery, read operation, etc.

RDE Daemon (**in PLDM repo**) would be the primary consumer of this library but
it can be used and extended as required by anyone trying to work with RDE. To
summarize, RDE daemon would be responsible for setting up the RDE stack, i.e.,
doing the PLDM base discovery, RDE discovery and providing handlers to perform
equivalent CURD operations with RDE.

**Why we need this library?**

This library is a general purpose solution for setting up the RDE stack. For
instance, the requester would not have to take care of what all operations are
required for performing the base or RDE discovery, rather just make a call to
base and RDE discovery handlers from the library and ask for the next command in
sequence to be executed. This makes the requester independent of the sequence of
operations required to perform a particular RDE operation.

## Background and References

- PLDM Base specification:
  https://www.dmtf.org/sites/default/files/standards/documents/DSP0240_1.1.0.pdf
- PLDM RDE specification:
  https://www.dmtf.org/sites/default/files/standards/documents/DSP0218_1.1.2.pdf

## Requirements

- Requester library should able to produce the next RDE operation in sequence

## Proposed Design

Requester Library would be a set of files written in C residing in libpldm repo
that would provide the correct sequence of commands for RDE operations to work.
`[Location: libpldm/src/requester/rde]`

Requester Library can be primarily bifurcated into, but is not limited to, the
following:
- pldm_base_requester.c: This file will take care of all the operations with
  respect to the PLDM Base discovery, which is required to setup the RDE stack:
    - GetTid
    - GetPLDMTypes
    - GetPLDMVersion
    - GetPLDMCommands

- pldm_rde_requester.c:  This file will take care of all the operations with
  respect to the PLDM RDE Discovery which are required to setup the RDE stack:
    - NegotiateRedfishParameters
    - NegotiateMediumParameters
    - Getting dictionaries
    - Handling CURD operation sequences

### Responsibilities of the requester library
- Maintaining a context for current command being executed and related values
- init_context(s): Initializing a context before begining PLDM base discovery or
  RDE discovery etc.
- get_next_command: Getting the next command in sequence to perform the PLDM base
  discovery
- push_response: Update the context, or send the values back to the requester

**Note: This library can be extended or modified based on the need and the
feedback provided and is not limited to the above description.**

## Alternatives Considered

A new daemon to track global health state. Although this would be difficult to
reuse to track individual component health.

## Impacts

## Testing

Testing will be performed using sensor values and the status LED.
