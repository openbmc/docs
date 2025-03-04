# Redfish Event Aggregation

Author: Rohit PAI:ropai@nvidia.com

Other contributors: None

Created: March 4, 2025

## Problem Description

Currently bmcweb lacks the support to aggregate events from satellite
management(SatMC) controllers. An external redfish client has no means to
subscribe and receive events from a SatMC which is behind the host BMC. Goal is
to extend the redfish aggregation support in bmcweb to aggregate events from
SatMC and make it available to external clients.

## Background and References

- [Satellite Management Controller (SMC) Specification](https://www.dmtf.org/standards/smc)
- [Redfish Event Service](https://redfish.dmtf.org/schemas/DSP0266_Redfish_Event_Service_Schema.html)

## Requirements

1. Bmcweb should support event aggregation from SatMCs.
2. The events recived from SatMC must be forwarded by the bmcweb on the Host BMC
   to all connected evnet subscribers.
3. When there are no event subscribers connected, bmcweb must store the events
   received from SatMCs in a circular buffer.
4. The events received from the SatMC will go through the same event filtering
   configuration prefered by the event subscriber.
5. The events forwarded from the SatMC by the Host BMC will have the satmc
   prefix in the event data.

## Proposed Design

### Overview

The proposed design implements event aggregation by having the Host BMC act as
an SSE client to receive events from Satellite BMCs, and then leverages the
existing Redfish Event Service infrastructure to forward these events to
external subscribers. The Host BMC will support forwarding events to both SSE
and HTTP push-style subscribers using the standard event delivery mechanisms
already in place.

```ascii
┌─────────────────────────────────────────────────────────┐
│                      Host BMC (bmcweb)                  │
│                                                         │
│  ┌─────────────┐    ┌─────────────┐    ┌────────────┐   │
│  │   HTTP      │    │  Satellite  │    │   Event    │   │
│  │   Client    │<───│   Event     │───>│  Service   │   │
│  │  Library    │    │  Monitor    │    │  Manager   │   │
│  └─────────────┘    └─────────────┘    └────────────┘   │
│         ▲                                     ▲         │
└─────────│─────────────────────────────────────│─────────┘
          │                                     │
          │                                     │
          ▼                                     ▼
┌─────────────────┐                    ┌─────────────────┐
│    Satellite    │                    │    External     │
│      BMCs       │                    │   Subscribers   │
└─────────────────┘                    └─────────────────┘
```

The Host BMC will:

- Receive events via SSE streaming from Satellite BMCs
- Use an HTTP client library to manage SSE connections
- Process and filter events before forwarding
- Maintain robust connections with automatic reconnection
- Buffer events when no subscribers are connected

The design consists of three main components:

1. SatelliteEventMonitor

   - Manages SSE connections to Satellite BMCs
   - Host BMC connects to satellite BMC's /redfish/v1/EventService/SSE endpoint
   - Maintains persistent HTTP connection with text/event-stream content type
   - Events pushed from satellite BMC to host BMC in real-time
   - Events formatted as SSE data with proper event formatting

2. HTTP Client Library

   - Provides SSE connection capabilities
   - Extended to support SSE connections and streaming
   - Handles SSE-specific headers and connection management
   - Provides callbacks for event data and connection status
   - Manages connection state and automatic reconnection

3. EventServiceManager

   - Handles event subscriptions and forwarding

4. Event Flow:

- Satellite BMC generates events
- Events sent over SSE to host BMC
- Host BMC converts SSE format to Redfish event format
- Events filtered and forwarded to subscribers

Detailed Sequence:

```ascii
External          Host             Satellite
Client            BMC                BMC
   │               │                  │
   │               │    SSE Connect   │
   │               │─────────────────>│
   │               │                  │
   │               │ Connection OK    │
   │               │<─────────────────│
   │               │                  │
   │   Subscribe   │                  │
   │──────────────>│                  │
   │               │                  │
   │   201 Created │                  │
   │<──────────────│                  │
   │               │                  │
   │               │    Event 1       │
   │               │<─────────────────│
   │               │                  │
   │  Event 1      │                  │
   │<──────────────│                  │
   │               │    Event 2       │
   │               │<─────────────────│
   │               │                  │
   │  Event 2      │                  │
   │<──────────────│                  │
   │               │                  │
   │  Unsubscribe  │                  │
   │──────────────>│                  │
   │               │                  │
   │     200 OK    │                  │
   │<──────────────│                  │
   │               │                  │
```

## Alternatives Considered

- Running a separate daemon (Redfish Client) to aggregate events from SatMCs:
  - In this approach, a new daemon would:
    - Handle SSE connection management to SatMCs
    - Receive and process events from SatMCs
    - Expose the events on D-Bus for other services to consume
  - Drawbacks:
    - Adds unnecessary architectural complexity
    - Requires additional IPC mechanism (D-Bus) between the daemon and bmcweb
    - Increases system resource usage with another daemon
    - No clear benefit since bmcweb would be the only consumer of these events
    - Adds latency in event delivery due to additional hop through D-Bus
  - This alternative was rejected because integrating the functionality directly
    into bmcweb provides a simpler and more efficient solution.

## Impacts

Performance impact

- If there is event flooding from the SatMC, then there might some performance
  impact on the other URI turn around tiime. This needs to be tested and
  evaluated.

### Organizational

- Does this proposal require a new repository?
  - No
- Which repositories are expected to be modified to execute this design?
  - bmcweb
- Make a list, and add listed repository maintainers to the gerrit review.
  - Ed Tanous
  - Gunnar Mills

## Testing

- Unit tests
  - This will cover all the event aggregation logic in bmcweb.
- Integration tests
  - End to end tests to verify the event aggregation from SatMC to the Host BMC
    and from the Host BMC to the event subscribers.
