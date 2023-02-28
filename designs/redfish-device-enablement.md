# Redfish Device Enablement- RDE Daemon & Requester Library

Author: Harsh Tyagi !harshtya

Other contributors: Kasun Athukorala, Jayanth Othayoth

Created: 2023-02-27

## Problem Description

Implementing Redfish Device Enablement (RDE) with openBMC as specified by the
following DMTF spec:
https://www.dmtf.org/sites/default/files/standards/documents/DSP0218_1.1.0.pdf

The intent of this design doc is to summarize the Redfish Device Enablement
(RDE) implementation in OpenBMC. RDE uses PLDM over MCTP (kernel) as protocols
for communication.

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

This implementation has two components to it:

### Extended PLDM Daemon for RDE

This enhancement introduces RDE capabilities directly into the existing PLDM
daemon, eliminating the need for a separate RDE daemon. By embedding RDE
functionality into the PLDM infrastructure, the system gains architectural
simplicity, better maintainability, and more efficient resource utilization.

#### Architecture Overview

The updated design integrates the RDE stack within the PLDM daemon, enabling:

- PLDM Base and RDE Discovery through unified service orchestration.
- In-process RDE handlers to perform Redfish-equivalent CRUD operations.
- One daemon, one purpose: managing both PLDM and RDE flows without inter-daemon
  communication overhead.

```
                            |-------------------|
                            │ BMCWeb Aggregator │
                            |-------------------|
                                      │   1.Sends req to RDE
                                      ▼
                         |-------------------------|
                         │     DBus interface      │
                         |-------------------------|
                                      │
                                      │  2. Reaches the RDE op handler
                                      ▼
                         +----------------------------+
                         |        PLDM Daemon         |
                         |  +----------------------+  |
                         |  |  Base PLDM Handlers  |  |
                         |  +----------------------+  |
                         |  |  RDE Handler Module  |  |
                         |  |  (PLDM Extension)    |  |
                         |  +----------------------+  |
                         |        |         |         |
                         |        ▼         ▼         |
                         |  Command Parser  Context   |
                         |     Engine       Manager   |
                         +----------------------------+
                                     │
                                     │
                                     ▼
                         +-----------------------------+
                         |     MCTP Transport Layer    |
                         +-----------------------------+
```

**Fetching the RDE Devices:**

Note: Currently we propose to support RDE devices over serial connection (USB
Devices), but we can expand it to different types of RDE devices later.

- Detector: We would create a separate daemon (in Entity Manager repo probably)
  that would fetch the RDE devices and make them available under a DBus Object
  path
- The Entity Manager would have a config that would filter out the USB devices
  that match a RDE device config (Let's say based on the Vendor ID)
- Reactor: There would be a Reactor that would do the MCTP setup on the USB
  paths of the discovered RDE device and perform the PLDM & RDE discovery
  operation.

For different sorts of RDE devices, we can follow the same detector-reactor
model with Entity Manager.

**BMCWeb integration with PLDM Daemon:**

- The BMCWeb aggregator would take care of sending the request to the correct
  object path based on a prefix system (Not in the scope of this design doc).
- The BMCWeb aggregator would extract the required fields for making the DBus
  method call- such as URI, request payload, etc. and invoke the DBus method for
  the RDE operation corresponding to the HTTP request type.

**RDE D-Bus Interface – Manager for Redfish Operation**

- The xyz.openbmc_project.RDE.Manager interface enables RDE operations over
  D-Bus. It supports sending Redfish HTTP methods (GET, POST, PATCH, etc.),
  retrieving schema information, and querying supported operations per device.
  Payloads can be passed inline or via file references, with support for both
  BEJ and JSON encodings.

**RDE Translator- HTTP to RDE conversion**

- Once the request reaches the PLDMD daemon, it will encode the payload into
  Binary Encoded Json using `libbej` library and forward the request to the
  miniBMC using the requester library.
- After receiving the response from the RDE device, the RDE daemon would use the
  Translator library to decode the response using the dictionaries saved during
  the RDE discovery phase for resources, convert the RDE responses into HTTP
  responses and send it back to the requester.
- Translator library would also be responsible in converting the URI to the
  corresponding resource id as RDE devices only understand resource ids.

**PLDM Types supported by RDE:**

- Currently the RDE daemon proposes of supporting the following PLDM_BASE
  PLDM_PLATFORM_AND_MONITORING and PLDM_RDE types.
- RDE Daemon would be responsible, during the initialization phase, for
  performing all the above operations and saving the dictionaries and PDRs.

_Responsibilities of the PLDM Daemon (Repo: pldm)_

- Enhancing PLDM Base Discovery to recognize and support RDE-specific command
  sets and PDRs.
- Initiating RDE Discovery Workflows using the requester library, enabling the
  daemon to identify RDE-capable devices and their capabilities.
- Handling RDE Operation Requests, primarily Read/Write/Patch-style
  Redfish-equivalent operations via in-process CRUD handlers.
- Translating and Dispatching RDE Requests using the existing pldm_send()
  mechanism, ensuring seamless integration with the transport layer.
- Processing Responses from Devices through pldm_recv(), with added handling for
  RDE-specific response decoding and validation.

## Alternatives Considered

**Proposed Standalone RDE Daemon** An initial design proposal suggested creating
a separate RDE daemon to handle RDE tasks independently of the PLDM daemon. The
goal was to isolate RDE-related responsibilities while keeping PLDM operations
unaffected.

However, based on feedback from reviewers, a few concerns were raised:

- Duplicate PDR Management: Both daemons would require their own mechanisms for
  handling and storing PDRs. This would not only result in code duplication but
  also increase the chances of inconsistency or sync issues between two sources
  of truth.
- Duplicate Responsibilities: While RDE primarily handles CRUD operations, it
  still depends on shared workflows like request routing, payload validation,
  and response formatting. Additionally, any discovery-related logic or
  transport coordination (e.g., PLDM message routing, instance ID tracking)
  would need to be exposed again in the RDE daemon or kept tightly in sync with
  the PLDM daemon.
- Complex Debugging and Maintenance: Splitting related logic across processes
  adds overhead when diagnosing issues or making architectural changes.

**Communication with BMCWeb and PLDM Daemoqn**

- Considered using localhost port forwarding but that is unsecure and can lead
  to port conflicts
- If unix sockets are used for forwarding requests from BMCWeb to RDE daemon it
  would require us to handle all the functionalities of timeouts, retries, etc
  that BMCWeb handles and might also require us to work with an exclusive
  protocol for transferring data.

## Impacts

- Will provide RDE support in openBMC which is based over PLDM that provides
  efficient access to low level platform inventory, monitoring, control, event,
  and data/parameters transfer functions

## Testing

Testing will be done using a QEMU connected to a RDE device (dev board)
