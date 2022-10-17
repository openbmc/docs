# Redfish Composability

Author:
  Gunnar Mills / GunnarM

Created:
  October 17, 2022

## Problem Description

Allow for Specific Composition defined by the Redfish standard in section
15.1.2, of version 1.15.1, of the Redfish Specification (DSP0266). This design
does not include 15.1.3 Constrained Composition or any other composition design.

Specific composition is when a client identifies an exact set of resources in
which to build a logical entity.

Additional Redfish material on composability:

https://www.dmtf.org/sites/default/files/standards/documents/DSP0266_1.15.1.pdf

https://www.dmtf.org/sites/default/files/standards/documents/DSP2050_1.1.0.pdf

https://www.youtube.com/watch?v=0g2lyZAnuFE


## Background and References

### What Is Composability?

In the context of disaggregated hardware, components are treated independently
and are not bound to a singular system. Compute, network, storage, and offload
components are used as individual resources. Software is used to bind components
together to create a logical system, which would be based on the user's needs
for their application. These logical systems function just like traditional
industry standard rackmount systems. This allows a user to compose systems using
different sets of components without having to touch any hardware.

### Composition Service

The Composition Service contains resource blocks and resource zones and is off
the Redfish Service Root.

### Resource Blocks

Each Resource Block instance represents the lowest level building blocks for
creating a system. Resource Blocks contain other statuses about the block, for
example, CompositionState, and links to a Computer System if the block is
already in use.

### Resource Zones

Shows the relationships between the different blocks to establish which blocks
can be in the same composition request. Resource Zones also report Capabilities,
which allows a client to understand the format of composition requests.

### Creating a Composed ComputerSystem

The following is from the Redish Standard.

A service that supports specific compositions shall support a POST request that
contains an array of hyperlinks to resource blocks. The schema for the resource
being composed defines where the resource blocks are specified in the request.

The following example shows a ComputerSystem being composed with a specific
composition request:

```

POST /redfish/v1/Systems HTTP/1.1
Content-Type: application/json;charset=utf-8
Content-Length: <computed length>
OData-Version: 4.0

{
   "Name": "Sample Composed System",
   "Links": {
      "ResourceBlocks": [{
         "@odata.id": "/redfish/v1/CompositionService/ResourceBlocks/ComputeBlock0"
      }, {
         "@odata.id": "/redfish/v1/CompositionService/ResourceBlocks/DriveBlock2"
      }, {
         "@odata.id": "/redfish/v1/CompositionService/ResourceBlocks/NetBlock4"
      }]
   }
}

```

The Redfish Service might respond something like
```
HTTP/1.1 201 Created
Location: /redfish/v1/Systems/ComposedSystem0
```

### Retiring a Composed System and freeing the blocks

When finished with a composed system, the client would delete the composed
system, such as
```
DELETE /redfish/v1/Systems/ComposedSystem0
```

## Requirements

Implement the following Redfish resources:

- CompositionService at `/redfish/v1/CompositionService`.

- ResourceBlocks at
`/redfish/v1/CompositionService/ResourceBlocks/<ResourceBlockId>`.

- ResourceZone at
`/redfish/v1/CompositionService/ResourceZones/<ResourceZoneId>`.

- Allow for Composed Redfish Computers Systems (with 'SystemType' 'Composed').
These Redfish Computer Systems would be at `/redfish/v1/Systems/`.

- Creation of these Composed Redfish Computer Systems would be via POST to
`/redfish/v1/System`.

- Allow for deletion of Composed Systems.

- Support ComputerSystem actions AddResourceBlock and RemoveResourceBlock.


## Proposed Design

Introduce new D-Bus interfaces and have bmcweb read and write these to allow for
Redfish Composability. The code that implements these D-Bus interfaces goes into
a separate new composed system repository.

New D-Bus Interfaces include:

- Resource Block with properties such as Composed State.

- Resource Zone with a link to the Resource Blocks this encompasses.

- D-Bus method to create composed redfish systems.

- D-Bus interfaces to AddResourceBlock and RemoveResourceBlock.

## Alternatives Considered
Following Redfish and using D-Bus interfaces follow OpenBMC's architecture. We
could look at implementing these D-Bus interfaces in an existing app like bmcweb
or entity-manager, but a new repository and binary fit better. This is similar
to bmcweb's relationship with https://github.com/openbmc/telemetry.

## Impacts

There will be an API for composability.

### Organizational

It is expected the D-Bus interfaces go in phosphor-dbus-interfaces, the Redfish
implementation goes in bmcweb, and the code which implements these D-Bus
interfaces go in a separate new composed system repository.

## Testing

This can be tested using the Redfish Service Validator and other Redfish client
tools. Scripts could be written to compose a system, use the system, and then
delete the composed system when finished, verifying the resource blocks are
freed.