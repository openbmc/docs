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

### RDE Daemon
- RDE Daemon (**Repo: pldm**) RDE daemon would be responsible for setting up the
  RDE stack, i.e., doing the PLDM base discovery, RDE discovery and providing
  handlers to perform equivalent CRUD operations with RDE.

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
                               |--------------|
                       --------|  RDE daemon  |---------
                       │       |--------------|        │
3.Translates HTTP req  │               │               │  4. Get next command in seq
     to RDE struct     │               │               │
                       │               │               │
                       ▼               │               ▼
              |----------------|       │         |----------------|
              │ RDE Translator │       │         │ Requester lib  │
              |----------------|       │         |----------------|
                                       │
                                       │
                                       │5. Sends RDE op
                                       │
                      |------------|   │     |------------|
                      │RDE Device 1│◄--|---► │RDE Device 2│
                      |------------|         |------------|
```
**Fetching the RDE Devices:**

Note: Currently we propose to support RDE devices over serial connection (USB Devices), but we
can expand it to different types of RDE devices later.

- Detector: We would create a separate daemon (in Entity Manager repo probably) that would
  fetch the RDE devices and make them available under a DBus Object path
- The Entity Manager would have a config that would filter out the USB devices
  that match a RDE device config (Let's say based on the Vendor ID)
- Reactor: There would be a Reactor that would do the MCTP setup on the USB
  paths of the discovered RDE device and perform the PLDM & RDE discovery
  operation.
  
For different sorts of RDE devices, we can follow the same detector-reactor
model with Entity Manager.

**BMCWeb integration with RDE Daemon:**

- The BMCWeb aggregator would take care of sending the request to the correct
  object path based on a prefix system (Not in the scope of this design doc).
- The BMCWeb aggregator would extract the required fields for making the DBus
  method call- such as URI, request payload, etc. and invoke the DBus method for
  the RDE operation corresponding to the HTTP request type.

**RDE Translator- HTTP to RDE conversion**

- Once the request reaches the RDE daemon, it will encode the payload into
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

_Responsibilities of the RDE Daemon (Repo: pldm)_

- Initiating the PLDM base discovery using the requester library
- Initiating the RDE discovery using the requester library
- Handling RDE operation calls such as Read operation, etc.
- Sending the requests to the RDE devices using already existing pldm_send()
- Receiving the responses from the RDE devices using already exisitng
  pldm_recv()

### Requester Library

- Requester library (**Repo: libpldm**) would be responsible for providing
  the next RDE command in sequence required to execute a particular RDE
  operation such as discovery, read operation, etc.

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

### RDE Daemon:

### Requester Library

Req Library would be a set of files written in C residing in libpldm repo
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
  CRUD operations related to RDE

_Responsibilities of the requester library_

- Maintaining a context for current command being executed and related values
- init_context(s): Initializing a context before begining PLDM base discovery or
  RDE discovery etc.
- get_next_command: Gets the next command in sequence for RDE operation
- push_response: Update the context, or send the values back to the requester

**Note: This library can be extended or modified based on the need and the
feedback provided and is not limited to the above description.**

## Alternatives Considered

**Communication with BMCWeb and RDE Daemoqn**

- Considered using localhost port forwarding but that is unsecure and can lead
  to port conflicts
- If unix sockets are used for forwarding requests from BMCWeb to RDE daemon it
  would require us to handle all the functionalities of timeouts, retries, etc
  that BMCWeb handles and might also require us to work with an exclusive
  protocol for transferring data.


## Impacts

- Will provide RDE support in openBMC which is based over PLDM that provides
  efficient access to low level platform inventory, monitoring, control, event, and data/parameters transfer functions

## Testing

Testing will be done using a QEMU connected to a RDE device (dev board)
