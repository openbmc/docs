# Inventory associations for bridged MCTP endpoints

Author: Andrew Jeffery <andrew@codeconstruct.com.au> | @arj

Created: 28th August 2026

## TODO

- Consider implications of
  [`xyz.openbmc_project.Inventory.Connector.Port`][pdi-port]

[pdi-port]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Inventory/Connector/Port.interface.yaml

## Conventions

The words "port" and "connector" are interchanged freely throughout: they refer
to the same physical concept, though "port" may be preferred when referring to
Entity Manager configurations.

## Background and References

### Related specifications

- [DMTF PMCI DSP0236 - Management Component Transport Protocol (MCTP) Base Specification](https://www.dmtf.org/dsp/dsp0236)
- [openbmc/docs: OpenBMC in-kernel MCTP](/designs/mctp/mctp-kernel.md)
- [openbmc/docs: Physical Topology for Inventory Items](/designs/physical-topology.md)

### Related design proposals

- [designs: add MCTP device discovery for upper-layer services](https://gerrit.openbmc.org/c/openbmc/docs/+/96378)
- [designs: add inventory item ownership][docs-designs-coalesced-features]
- [design: PDI-compatible configuration
  interfaces][docs-designs-em-config-interfaces]

[docs-designs-coalesced-features]:
  https://gerrit.openbmc.org/c/openbmc/docs/+/96526
[docs-designs-em-config-interfaces]:
  https://gerrit.openbmc.org/c/openbmc/docs/+/83264

### Related implementations

- [CodeConstruct/mctpd](https://github.com/CodeConstruct/mctp/blob/31ac0e86b0e23d32abf34d191653a87a47f3c9a7/docs/mctpd.md)
- [openbmc/entity-manager](https://github.com/openbmc/entity-manager/blob/f9ab4384184c74ab05de5a2a05ac5ca5a55b64dd/README.md)
- [openbmc/dbus-sensors: mctpreactor](https://gerrit.openbmc.org/c/openbmc/dbus-sensors/+/69111)

### Related Entity Manager configuration schemas

- [schemas: introduce platform schema](https://gerrit.openbmc.org/c/openbmc/entity-manager/+/87415)
- [schemas: Describe MCTP USB targets](https://gerrit.openbmc.org/c/openbmc/entity-manager/+/77381)
- [schemas: platform: describe MCTP bridged endpoints](https://gerrit.openbmc.org/c/openbmc/entity-manager/+/93523)

### MCTP

The topology of an MCTP network is described in terms of devices, bridge
devices, bridge ports, buses, and routes. Information on these elements and
relationships between them can be built using MCTP's `Get Routing Table Entries`
and `Query Hop` commands.

## Problem Description

Existing platform designs have non-trivial MCTP topologies. Some MCTP endpoints
are bridged, and thus are not immediate neighbours in the network.

Currently, OpenBMC's support for associating MCTP endpoints exposed on D-Bus by
`mctpd` with configuration and inventory information exposed on D-Bus by Entity
Manager only extends to the trivial case of immediate neighbours in an MCTP
network. A context gap exists for all bridged endpoints in any platform design
with a non-trivial MCTP topology.

## Requirements

A `configured_by`/`configures` association must exist for each MCTP endpoint
whose device is described by configuration in Entity Manager, regardless of its
location in the MCTP network.

### Derived Requirements

We must be able to equate paths over topology information reported by both
Entity Manager and the MCTP network to identify components and form the required
associations.

It is necessary for Entity Manager to provide connection topology information in
order to test path equivalence.

## Current implementation shortfalls

### MCTP

- `mctpd` does not expose details of connection paths to a given endpoint.

### Entity Manager

- Entity Manager does not currently provide the necessary topology information
  to test connection path equivalence.

## Design abstract

To form the associations the proposed design is to allow Entity Manager's
schemas to describe:

1. Bus segments
2. Bus interfaces exposed by each active component on a board
3. Bus interfaces exposed by each port on a board

FRU EEPROM detection remains the means to discover boards. The bus on which a
FRU EEPROM is detected identifies the global bus index, and therefore the bus
segment and specific connector by which the board is connected. Identification
of the connecting connector implies the connection of the remaining buses on
that connector. Groups of contacts forming buses described on that port by the
configuration enable traversal to resolve path equivalence.

## Exploration of concerns

### Device location identity

Complex platform configurations may contain multiple instances of a device or
board.

In general, the location identity of a device or board in the system cannot be
uniquely described based on the device's type alone.

A stable identity for the location of a device in the platform configuration is
its path under some view of the platform configuration's topology. There are a
number of views in a platform's topology: Containment, power, cooling, and
various forms of connectivity for communication.

#### Device connectivity for communication

Device connectivity may be complex. Communication to a device in the platform
may span multiple physical transports.

Establishing a path over these physical transports provides a location identity
for the device in terms of some topology of platform connectivity.

The problem at hand regards bridged MCTP endpoints. This constrains our
considerations to the configuration topology that MCTP is capable of describing.

### MCTP

DSP0236 delegates to bridges the responsibility of assigning endpoint IDs to
their neighbours for which they are bus owners, and where those neighbours are
themselves bridges, allocation of an EID pool to each bridge to be used for its
own downstream assignments and related pool allocations.

The method for selecting any specific endpoint ID from a bridge's pool to be
assigned to a given downstream neighbour of the bridge is outside the scope of
DSP0236. It may vary between devices, as may the number of ports per bridge, and
the physical transport for any given port exposed by a bridge.

Physical transports defined for MCTP include:

- I2C (DSP0237)
- I3C (DSP0233)
- PCIe VDM (DSP0238)
- Serial (DSP0253)
- USB (DSP0283)

This delegation of responsibility and the implementation-specific nature of EID
allocation by bridges means there is no implicit global view of the composition
of an MCTP network. If any such view is to be built, it requires explicit
intervention.

Graph-traversal of bridge responses to DSP0236's mandatory
`Get Routing Table Entries` command can be used to determine the entire MCTP
network topology in terms of bridge ports, their physical transports, and
endpoints routable via a given port.

Restated for the problem at hand, information gathered by graph-traversal of
`Get Routing Table Entries` responses allows us to construct a location identity
for each reachable endpoint in the MCTP network in terms of a path to it in the
topology of physical transports and bridges.

### Entity Manager

Entity Manager binds board configurations to discovered devices by matching a
configuration's `Probe` statement against a discovered device's FRU metadata.
FRU metadata of discovered devices is often hosted on D-Bus by `fru-device`.
`fru-device` periodically scans I2C buses exposed either by the BMC SoC, or
those that represent the downstream segments of an I2C mux device, for FRU
EEPROMs.

Entity Manager publishes D-Bus objects hosting inventory information for
discovered boards, as well as a collection of device configuration objects
described in the matching board configuration's "Exposes" property

Entity Manager may interpolate values derived from the FRU metadata, or metadata
determined in the process of discovering the FRU metadata, into the
configuration for a board upon binding a configuration to the discovered FRU
device. The data available for interpolation includes the FRU EERPOM bus address
and the bus index that hosts the FRU EEPROM device. Together these are often
used to derive the addresses of other communication devices described in the
configuration, usually by way of small arithmetic expressions embedded in
configuration value strings. Interpolated values such as bus indexes are global
values derived from the system hardware composition and the order of its
enumeration.

Restated, the current location identity of communicating components in an Entity
Manager configuration only extends to the bus on which the FRU EEPROM is
discovered, usually an I2C bus. Further, the connectivity path is implicitly
described by a possibly-unstable global bus number, but which captures mux
relationships necessary for communicating with the FRU EEPROM.

### Synthesis of concerns

Creating an assocation between `mctpd`'s endpoint D-Bus objects and the
corresponding configuration D-Bus objects hosted by Entity Manager requires
establishing the equivalence of device location identities. Using paths in the
platform configuration topology for the purpose of location identity, this
transforms to equating paths in the device topology. The class of paths
understood by both `mctpd` and Entity Manager must necessarily be equivalent.

MCTP is not constrained to I2C as a physical transport, therefore, the support
currently provided by Entity Manager for describing the connectivity topology is
insufficient to solve the problem at hand.

## Observations regarding Entity Manager

Despite statments above, Entity Manager does have some support for describing
topology beyond I2C. It has a schema for `Port`s, and the ability to bind two
boards by a D-Bus association, based on correlated port names and `PortType`s.

The current schema for ports describes the following relationships:

- `containing`
- `powering`
- `probing`

While these relationships describe some platform configuration topologies, they
are not the topologies necessary to solve the problem at hand.

Returning to the fundamentals of Entity Manager, it remains the case that FRU
discovery is necessary, along with binding a configuration to a discovered
device. The act of binding a configuration to a device establishes the existence
of the features described in the configuration's `Exposes` statement, and their
relationship to the device's FRU EEPROM.

For the problem at hand it's necessary that the devices and physical transports
supported by MCTP are among the features described in the probed device's
configuration, along with their relationship to the bus on which the FRU EEPROM
was discovered.

Describing devices on boards that exist on buses that aren't the bus of the
board's FRU EEPROM device requires describing the buses of those devices.

Relating the buses of those devices to the bus of the FRU EEPROM requires
describing connectors.

## A proposal to describe bus topologies in Entity Manager

The concept is to expand the `PortType` property of Entity Manager's `Port`
schema to include `connecting` and `connected_by` relationships, along with
remaining definitions.

Several further changes are proposed:

1. Define a `FormFactor` property on the `Port` schema to capture the port's
   mechanical properties

2. Define a `Pinout` property on the `Port` schema to capture some of the port's
   electrical properties

3. Prioritise coupling `Port`s by `FormFactor` and `Pinout` over `Name`.

The prioritisation in 3 allows us to define multiple instances of the same port
type on a board while providing them different names, which avoids both:

- conflicts with Entity Manager's arrangement of D-Bus object paths, and

- [the proposed behaviour][docs-designs-coalesced-features] of coalescing
  multiple `Exposes` nodes using the same name.

If neither `FormFactor` nor `Pinout` are present then the implementation may
fall-back to the current strategy of coupling by name.

Regarding 2, the `Pinout` property is a list of `Bus` entries that describe
connectivity provided over the port. Any two ports with matching `FormFactor`
may be considered for connection if their set of `Bus` entries does not
conflict.

Regarding 1, `FormFactor` is proposed as an open enumeration of identifiers for
connector types.

Additionally, an `Interfaces` list of `Bus` entries is introduced for exposed
devices on `Board` configuration types to describe external connectivity
provided by the device, named by `Segment`. `Segment` names must be unique over
the bus segments described in the board's configuration file. `Bus` objects in
an MCTP bridge device's `Interfaces` list must have an `MCTPPort` property
describing the static MCTP port number for the bus interface.

### Enumeration of scenarios

We will illustrate the following design concept as the base case:

<!-- markdownlint-disable MD029 -->

1. Two boards related by one connector with one I2C contact group and one USB
   contact group, and on the connected board, one I2C FRU EEPROM on its I2C bus
   segment, and one USB MCTP device on its USB segment.

After that, we will consider the following design concepts assuming the simplest
relation of USB segments supporting an MCTP device on each connected board:

2. Three boards related by two connectors on the connecting board, each
   connector with one I2C contact group, two I2C segments independently
   servicing each connector on the connecting board, and one I2C FRU EEPROM on
   each connected board's I2C segment

3. Three boards related by two connectors of different shape on the connecting
   board, each connector with one I2C contact group, one I2C segment servicing
   both connectors on the connecting board, and one I2C FRU EEPROM on each
   connected board's I2C segment.

4. Three boards related by two connectors of the same shape on the connecting
   board, each connector with one I2C contact group, one I2C segment servicing
   both connectors on the connecting board, and one I2C FRU EEPROM on each
   connected board's I2C segment.

To develop the case for USB, we will consider the following design concepts
assuming the simplest unambiguous relation of I2C segments supporting a FRU
EEPROM on each connected board:

5. Two boards related by one connector with one USB contact group, and one MCTP
   device on the connected board's USB segment.

6. Two boards related by one connector with one USB contact group, where the
   contact group's bus segment on the connecting board is separated from the
   host controller by a USB hub, and one MCTP device on the connected board's
   USB segment

7. Two boards related by one connector with one USB contact group, and two MCTP
   devices behind a USB hub on the connected board.

8. Two boards related by one connector with one USB contact group, one MCTP
   bridge device on the connected board's USB segment, and one I2C MCTP device
   behind the bridge.

<!-- markdownlint-enable MD029 -->

Finally, for completeness with respect to the MCTP specification, we consider
endpoints that are routable through multiple ports.

Despite laying the scenarios out in the order above we will discuss them in a
different order: Scenario 1 as a base case, Scenario 8 as the interesting case,
Scenarios 4, 3 and 2 as they deal with some ambiguities, and then the rest, as
they involve relatively uninteresting complexities.

It's worth noting that Scenario 4 will trigger the Entity Manager behaviour
outlined in [design: PDI-compatible configuration
interfaces][docs-designs-em-config-interfaces]. I (Andrew) am of the opinion
that the current behaviour of Entity Manager is undesirable, though resolving it
is not yet viewed as a pre-condition for adoption of this proposal.

### Scenario 1: The base case

As an ASCII diagram:

```text
+----------------------------------------+          +------------------------------+
|                                        |          |                              |
|  +-----------------+                 +------++------+                            |
|  |                 |                 |      ||      |                            |
|  |           +--------+              |+-----++-----+|           +-------------+  |
|  |           | I2C 12 |    I2C12     || I2C1||I2C1 ||   I2C3    |  Board FRU  |  |
|  |           |        +---------------+     ||     +------------+             |  |
|  |           +--------+              |+-----++-----+|           +-------------+  |
|  |                 |                 |      ||      |                            |
|  |     AST2600     |        BMC      |  BMC || BMC  |  Managed                   |
|  |                 |       Board     | Port || Port |   Board                    |
|  |           +--------+              |+-----++-----+|           +-------------+  |
|  |           |  USB 1 |  USB Port B  || USB1||USB1 ||   USB1    | MCTP Device |  |
|  |           |        +---------------+     ||     +------------+             |  |
|  |           +--------+              |+-----++-----+|           +-------------+  |
|  |                 |                 |      ||      |                            |
|  +-----------------+                 +------++------+                            |
|                                        |          |                              |
+----------------------------------------+          +------------------------------+
```

As Entity Manager configuration:

```json
{
  "Exposes": [
    {
      "Interfaces": [
        {
          "Type": "Bus",
          "Protocol": "I2C",
          "Segment": "I2C12",
          "Bus": "12"
        },
        {
          "Type": "Bus",
          "Protocol": "USB 2.0",
          "Segment": "USB Port B",
          "Bus": "1", // Root hub ID
          "MCTPPort": 0
        }
      ],
      "Name": "AST2600",
      "Type": "SoC"
    },
    {
      "Pinout": [
        {
          "Type": "Bus",
          "Name": "I2C1"
          "Protocol": "I2C",
          "Segment": "I2C12",
        },
        {
          "Type": "Bus",
          "Name": "USB1"
          "Protocol": "USB 2.0",
          "Segment": "USB Port B",
        }
      ],
      "Name": "BMC Port",
      "FormFactor": "SFF-TA-1002 4C+"
      "PortType": "connecting",
      "Type": "Port"
    }
  ],
  "Name": "BMC Board",
  "Probe": "...",
  "Type": "Board"
}
```

```json
{
  "Exposes": [
    {
      "Pinout": [
        {
          "Type": "Bus",
          "Name": "I2C1",
          "Protocol": "I2C",
          "Segment": "I2C3"
        },
        {
          "Type":  "Bus",
          "Name": "USB1",
          "Protocol": "USB 2.0",
          "Segment": "USB1"
        }
      ],
      "Name": "BMC Port",
      "FormFactor": "SFF-TA-1002 4C+",
      "PortType": "connected_by",
      "Type": "Port"
    },
    {
      "Type": "EEPROM",
      "Name": "Board FRU",
      "Address": "0x50",
      "Segment": "I2C3"
    },
    {
      "Type": "MCTPUSBDevice",
      "Name": "MCTP Device",
      "Segment": "USB1",
      "Configuration": 0,
      "Interface": 1
    }
  ]
  "Name": "Managed Board",
  "Probe": "...",
  "Type": "Board"
}
```

#### Discussion of Scenario 1

In this example we have two boards: `BMC Board` exposing an `AST2600` BMC, and
`Managed Board` exposing a USB-based MCTP device `MCTP Device`.

On `BMC Board` an AST2600 is described as exposing one I2C bus interface, bus
segment `I2C12`, and one USB interface, bus segment `USB Port B`. The bus
segments local to `AST2600` are associated with the controller driving them by
their `Bus` properties.

The bus segment for `I2C12` routes directly to a port on `BMC Board`:
`BMC Port`. USB connectivity is also routed to `BMC Port`.

When `Managed Board` is physically connected to `BMC Board`, the physical
connector connects the traces from bus segment `I2C12` on `BMC Board` bus
segment `I2C3` traces on `Managed Board`. Between the two configurations, the
port contacts node of type `Bus` with name `I2C1` associates the bus segments on
either board across the connector described in the configuration.

After the boards are physically plugged, the FRU EEPROM at `0x50` on
`Managed Board` is discovered over I2C. Its FRU data describes `Managed Board`,
which causes Entity Manager to bind the `Managed Board` configuration to the
device.

From the perspective of the `AST2600`, the FRU EEPROM for `Managed Board` is
discovered by driving transfers on I2C bus 12. The bus segment for I2C bus 12 is
`I2C12`, whose interface is exposed on `AST2600`. Bus segment `I2C12` is routed
to `BMC Port`. At this point, Entity Manager understands that `Managed Board`
must be plugged into `BMC Port`. This then implies the connection of the bus
segments of the `USB1` contacts, associating `MCTP Device` on `Managed Board`
with the `USB Port B` root hub on `AST2600`.

### Scenario 8: The interesting case

As an ASCII diagram:

```text
+----------------------------------------+          +---------------------------------------------------------+
|                                        |          |                                                         |
|  +-----------------+                 +-+----++----+-+                                                       |
|  |                 |                 |      ||      |                                                       |
|  |           +--------+              |+-----++-----+|           +-------------+                             |
|  |           | I2C 12 |    I2C12     || I2C1||I2C1 ||   I2C3    |  Board FRU  |                Managed      |
|  |           |        +---------------+     ||     +------------+   EEPROM    |                 Board       |
|  |           +--------+              |+-----++-----+|           +-------------+                             |
|  |                 |                 |      ||      |                                                       |
|  |     AST2600     |        BMC      |  BMC || BMC  |                                                       |
|  |                 |       Board     | Port || Port |                                                       |
|  |           +--------+              |+-----++-----+|           +-------------+            +-------------+  |
|  |           |  USB 1 |  USB Port B  || USB1||USB1 ||   USB1    | MCTP Bridge |            | MCTP Device |  |
|  |           |        +---------------+     ||     +-----+      |             |            |             |  |
|  |           +--------+              |+-----++-----+|    |   +--+--+       +--+--+  I2C2   |             |  |
|  |                 |                 |      ||      |    +---+ USB |       | I2C +---------+             |  |
|  +-----------------+                 +-+----++----+-+        +--+--+       +--+--+         |             |  |
|                                        |          |             +-------------+            +-------------+  |
+----------------------------------------+          +---------------------------------------------------------+
```

As Entity Manager configuration:

```json
{
  "Exposes": [
    {
      "Interfaces": [
        {
          "Type": "Bus",
          "Protocol": "I2C",
          "Segment": "I2C12",
          "Bus": "12"
        },
        {
          "Type": "Bus",
          "Protocol": "USB 2.0",
          "Segment": "USB Port B",
          "Bus": "1", // Root hub ID
          "MCTPPort": 0
        }
      ],
      "Name": "AST2600",
      "Type": "SoC"
    },
    {
      "Pinout": [
        {
          "Type": "Bus",
          "Name": "I2C1"
          "Protocol": "I2C",
          "Segment": "I2C12",
        },
        {
          "Type": "Bus",
          "Name": "USB1"
          "Protocol": "USB 2.0",
          "Segment": "USB Port B",
        }
      ],
      "Name": "BMC Port",
      "FormFactor": "SFF-TA-1002 4C+",
      "PortType": "connecting",
      "Type": "Port"
    }
  ],
  "Name": "BMC Board",
  "Probe": "...",
  "Type": "Board"
}
```

```json
{
  "Exposes": [
    {
      "Pinout": [
        {
          "Type": "Bus",
          "Name": "I2C1",
          "Protocol": "I2C",
          "Segment": "I2C3"
        },
        {
          "Type":  "Bus",
          "Name": "USB1",
          "Protocol": "USB 2.0",
          "Segment": "USB1"
        }
      ],
      "Type": "Port"
      "FormFactor": "SFF-TA-1002 4C+",
      "PortType": "connected_by",
      "Name": "BMC Port",
    },
    {
      "Interfaces": [
        {
          "Type": "Bus",
          "Name": "UP",
          "Protocol": "USB 2.0",
          "Segment": "USB1",
          "Connects": ["DOWN"],
          "MCTPPort": 0
        },
        {
          "Type": "Bus",
          "Name": "DOWN",
          "Protocol": "I2C",
          "Segment": "I2C2",
          "Address": "0x10",
          "Connects": ["UP"],
          "MCTPPort": 1
        }
      ],
      "Type": "MCTPUSBDevice",
      "Name": "MCTP Bridge"
      "Segment": "USB1",
      "Configuration": 0,
      "Interface": 1
    },
    {
      "Address": "0x50",
      "Segment": "I2C3",
      "Name": "Board FRU",
      "Type": "EEPROM"
    },
    {
      "Address": "0x12",
      "Segment": "I2C2",
      "Name": "MCTP Device",
      "Type": "MCTPI2CTarget"
    }
  ]
  "Name": "Managed Board",
  "Probe": "...",
  "Type": "Board"
}
```

### Scenario 4: The ambiguous multi-port case

As an ASCII diagram

```text
+---------------------------------------------------------+              +---------------------------------------+
|                                                       +-+------++------+-+                                     |
|                                                       |+-------++-------+|          +-----------+              |
|                                                       ||  I2C  ||  I2C  ||   I2C    | Board FRU |              |
|                                   +--------------------+       ||       +-----------+   EEPROM  |              |
|                                   |                   |+-------++-------+|          +-----------+              |
|               BMC Board           |                   |    Ext || Ext    |                          Managed    |
|                                   |                   |   Port || Port   |                           Board     |
|                                   |                   |+-------++-------+|          +-----------+     (1)      |
|                                   |               B.1 ||  USB  ||  USB  ||   USB    |MCTP Device|              |
| +--------------+                  |            +-------+       ||       +-----------+           |              |
| |         +----+---+              |            |      |+-------++-------+|          +-----------+              |
| |         | I2C 12 |    I2C12     |            |      +-+------++------+-+                                     |
| |         |        +--------------+            |        |              +---------------------------------------+
| |         +----+---+              |            |        |
| |              |                  |            |        |              +---------------------------------------+
| |              |                  |            |      +-+------++------+-+                                     |
| |              |                  |            |      |+-------++-------+|          +-----------+              |
| |   AST2600    |                  |          +-+      ||  I2C  ||  I2C  ||   I2C    | Board FRU |              |
| |              |                  +----------|---------+       ||       +-----------+   EEPROM  |              |
| |              |                             +-+      |+-------++-------+|          +-----------+              |
| |         +----+---+              +---------+  |      |    Ext || Ext    |                          Managed    |
| |         |  USB 1 |  USB Port B  |         +--+      |   Port || Port   |                           Board     |
| |         |        +--------------+ USX2422 |         |+-------++-------+|          +-----------+     (2)      |
| |         +----+---+              |         |     B.2 ||  USB  ||  USB  ||   USB    |MCTP Device|              |
| +--------------+                  |         +----------+       ||       +-----------+           |              |
|                                   +---------+         |+-------++-------+|          +-----------+              |
|                                                       +-+------++------+-+                                     |
+---------------------------------------------------------+              +---------------------------------------+
```

#### Discussion

Here, for the sake of the argument, we have `BMC Board` with two instances of
`Ext Port`. The `Managed Board` design also has an `Ext Port`, and so can in
concept attach to either of `BMC Board`'s `Ext Port`s.

Assuming a fixed address for `Board FRU EEPROM` in the design of
`Managed Board`, plugging in two instances of the board (one in each `Ext Port`
on `BMC Board`) introduces an address conflict. This which renders the design
concept invalid, or constrains the application to only plug at most one card in
at any point.

A similarly constrained alternative is to use card designs variants each with
EEPROMs strapped for distinct I2C addresses. In this case both cards may be
plugged simultaneously, but due to the I2C topology, the slot location of the
card is ambiguous. While this could be overcome with an administrative
constraint that requires cards be plugged in physically ordered fashion, it
allows for human errors, and remains difficult to describe in a reasonable way.

An alternative might be that the port is electrically keyed to strap the EEPROM
address, thus allowing multiple instances, though this will likely mean the port
and connecting cards are application-specific. It's worth nothing that this
approach doesn't preclude support in Entity Manager, but it also is yet to be
implemented.

Designs implementing Scenario 4 concepts are out of scope for this proposal.

### Scenario 3: A slightly less ambiguous multi-port case

Scenario 3 is similar to scenario 4, but the administrative constraint becomes
the combination of a design constraint and a physical constraint: Each connector
is physically distinct, preventing insertion of cards into the wrong port, and
the design constraint is to require distinct addresses for the FRU EEPROM.

Support for scenario 3 is in scope for this proposal, as the description of each
board's port must align and thus the relationships are unambiguous.

### Scenario 2: A common multi-port design-point

As an ASCII diagram:

```text
+---------------------------------------------------------+              +---------------------------------------+
|                                                       +-+------++------+-+                                     |
|                                                       |+-------++-------+|          +-----------+              |
|                                                       ||  I2C  ||  I2C  ||   I2C    | Board FRU |              |
|                                   +--------------------+       ||       +-----------+   EEPROM  |              |
|                                   |                   |+-------++-------+|          +-----------+              |
|               BMC Board           |                   |    Ext || Ext    |                          Managed    |
|                                   |                   |   Port || Port   |                           Board     |
|                           I2C12.1 |                   |+-------++-------+|          +-----------+     (1)      |
|                                   |               B.1 ||  USB  ||  USB  ||   USB    |MCTP Device|              |
| +--------------+             +----+----+       +-------+       ||       +-----------+           |              |
| |         +----+---+         |         |       |      |+-------++-------+|          +-----------+              |
| |         | I2C 12 |  I2C12  |   I2C   |       |      +-+------++------+-+                                     |
| |         |        +---------+   Mux   |       |        |              +---------------------------------------+
| |         +----+---+         |         |       |        |
| |              |             +----+----+       |        |              +---------------------------------------+
| |              |                  |            |      +-+------++------+-+                                     |
| |              |          I2C12.2 |            |      |+-------++-------+|          +-----------+              |
| |   AST2600    |                  |          +-+      ||  I2C  ||  I2C  ||   I2C    | Board FRU |              |
| |              |                  +----------|---------+       ||       +-----------+   EEPROM  |              |
| |              |                             +-+      |+-------++-------+|          +-----------+              |
| |         +----+---+              +---------+  |      |    Ext || Ext    |                          Managed    |
| |         |  USB 1 |  USB Port B  |         +--+      |   Port || Port   |                           Board     |
| |         |        +--------------+ USX2422 |         |+-------++-------+|          +-----------+     (2)      |
| |         +----+---+              |         |     B.2 ||  USB  ||  USB  ||   USB    |MCTP Device|              |
| +--------------+                  |         +----------+       ||       +-----------+           |              |
|                                   +---------+         |+-------++-------+|          +-----------+              |
|                                                       +-+------++------+-+                                     |
+---------------------------------------------------------+              +---------------------------------------+
```

As Entity Manager configuration:

```json
{
  "Exposes": [
    {
      "Interfaces": [
        {
          "Type": "Bus",
          "Protocol": "I2C",
          "Segment": "I2C12",
          "Bus": "12"
        },
        {
          "Type": "Bus",
          "Protocol": "USB 2.0",
          "Segment": "USB Port B",
          "Bus": "1", // Root hub ID
          "MCTPPort": 0
        }
      ],
      "Name": "AST2600",
      "Type": "SoC"
    },
    {
      "Interfaces": [
        {
          "Type": "Bus",
          "Name": "UP",
          "Protocol": "I2C",
          "Segment": "I2C12",
          "Connects": ["CH1", "CH2"]
        },
        {
          "Type": "Bus",
          "Name": "CH1",
          "Protocol": "I2C",
          "Segment": "I2C12.1"
          "Connects": ["UP"]
        }
        {
          "Type": "Bus",
          "Name": "CH2",
          "Protocol": "I2C",
          "Segment": "I2C12.2",
          "Connects": ["UP"]
        }
      ],
      "Name": "I2C Mux",
      "Type": "PCA9540B"
    },
    {
      "Interfaces": [
        {
          "Type": "Bus",
          "Protocol": "USB 2.0",
          "Segment": "USB Port B"
        }
        {
          "Type": "Bus",
          "Protocol": "USB 2.0",
          "Segment": "B.1"
        },
        {
          "Type": "Bus",
          "Protocol": "USB 2.0",
          "Segment": "B.2"
        }
      ]
      "Name": "USB Hub",
      "Type": "USX2422"
    }
    {
      "Pinout": [
        {
          "Type": "Bus",
          "Name": "I2C",
          "Protocol": "I2C",
          "Segment": "I2C12.1"
        },
        {
          "Type": "Bus",
          "Name": "USB",
          "Protocol": "USB 2.0",
          "Segment": "B.1"
        },
      ],
      "Name": "Ext Port",
      "FormFactor": "SFF-TA-1002 4C+",
      "PortType": "connecting",
      "Type": "Port"
    }
    {
      "Pinout": [
        {
          "Type": "Bus",
          "Name": "I2C",
          "Protocol": "I2C",
          "Segment": "I2C12.2"
        },
        {
          "Type": "Bus",
          "Name": "USB",
          "Protocol": "USB 2.0",
          "Segment": "B.2"
        },
      ],
      "Name": "Ext Port",
      "FormFactor": "SFF-TA-1002 4C+",
      "PortType": "connecting",
      "Type": "Port"
    }
  ]
  "Name": "BMC Board",
  "Probe": "...",
  "Type": "Board"
}
```

```json
{
  "Exposes": [
    {
      "Pinout": [
        {
          "Type": "Bus",
          "Name": "I2C",
          "Protocol": "I2C",
          "Segment": "I2C"
        },
        {
          "Type":  "Bus",
          "Name": "USB",
          "Protocol": "USB 2.0",
          "Segment": "USB"
        }
      ],
      "Name": "BMC Port",
      "FormFactor": "SFF-TA-1002 4C+",
      "PortType": "connected_by",
      "Type": "Port"
    },
    {
      "Address": "0x50",
      "Segment": "I2C",
      "Name": "Board FRU",
      "Type": "EEPROM"
    },
    {
      "Segment": "USB",
      "Name": "MCTP Device",
      "Type": "MCTPUSBDevice"
    }
  ]
  "Name": "Managed Board",
  "Probe": "...",
  "Type": "Board"
}
```

#### Discussion

In the arrangement above we have the strong relationship between a port and an
I2C mux leg. Linux exposes mux legs as their own I2C bus. Accessing through the
mux-leg-specific I2C bus device locks the path from the root controller to the
leaf mux leg before driving bus transfers. As the FRU EEPROM is discovered by
the port-specific mux-leg we have the information necessary to accurately
represent the device topology.

### Resolving associations for bridged endpoints

Associations are resolved in `mctpreactor` using information from `mctpd` and
Entity Manager.

#### `mctpd`

Given the following definitions:

1. Let `target` be the EID of the endpoint of interest

2. Let `current` and `next` be in the set of valid EIDs for the network
   containing `target`

3. Let `local` be in the set of EIDs that are bound to one or more MCTP
   interfaces on the network containing `target` in the execution environment of
   `mctpd`.

`mctpd` takes the following steps to establish an endpoint's path:

1. Sets `current` to `local`

2. Issues `Query Hop` to `current` for `target`

3. Sets `next` to the EID in the result from 3

4. If `next` is 0, sets `next` to `target`

5. Issues `Get Routing Table Entries` to `current`

6. Selects the entry from the table obtained in 5 corresponding to `next`

7. Extracts the port number of the interface used to route to `next` from the
   entry obtained in 6, along with the physical transport binding identifier and
   the physical address of `next`

8. Terminates if `next` equals `target`, otherwise

9. Sets `current` to `next`, and returns to 2

Once terminated, `mctpd` composes a representation of the path-order array of
(port, transport, address) tuples using the sequence of values extracted at step
7 and publishes the result on the endpoint's D-Bus object.

#### Enumerating MCTP bridge ports under Linux

Bridge configurations on BMCs running Linux may be dynamic by nature of devices
coming and going. DSP2036 requires that bridge port numbers are static (v1.4.0,
Clause 13.13, Table 27). The two must be reconciled.

An intuitive binding of port Ids to ports under Linux is by way of `ifindex`.
However, this fails quickly for multiple transport types under hot-plug events:

- For I2C, an interface is created for the root of each bus segment
- For USB, an interface is created for each device, as USB is point-to-point

#### Entity Manager

Entity Manager, having probed the configuration for both boards, publishes the
device configurations on D-Bus.

#### `mctpreactor`

With both sets of information published by `mctpd` and Entity Manager,
`mctpreactor` takes the following steps:

1. Identifies the device that is the execution environment for `mctpd` in the
   Entity Manager configurations, and sets this as the current device.

2. Consumes the head tuple from the path tuple array published by `mctpd` for
   the endpoint, leaving its tail

3. Selects the `Bus` object from the `Interfaces` list of the current device
   whose `MCTPPort` value corresponds to the the port component of the tuple
   obtained in 2

4. Aborts the resolution if the selection in 3 yields no result, otherwise

5. Aborts the resolution if the transport binding identifier component from the
   tuple obtained in 2 does not match the protocol property of the configuration
   obtained in 3, otherwise

6. Selects the reachable device with the `Address` value that matches the
   address component of the tuple obtained in 2, subject to the bus being
   addressable, and:

   1. The device is exposed on the board containing the current device, and is
      not of `Port` type, and the `Segment` value of the device matches the
      `Segment` value of the `Bus` configuration obtained in 3, or

   2. The device is exposed on the board containing the current device, and is
      not of `Port` type, and the `Segment` value of the device is reachable
      from the `Segment` value obtained in 3, where relevant segments are
      related by intervening mux or switch components, or

   3. The device is exposed on a board transitively connected through `Port`s
      exposed on boards related by `connecting` and `connected_by` associations
      from the board containing the current device, and has a `Segment` value
      that is reachable from the `Segment` value of the bus configuration
      obtained in 3 where relevant segments are related by any combination of
      selected `Pinout` entries on the relevant `Port`s or by intervening mux or
      switch components

7. Aborts the resolution if the selection in 6 yields no result, otherwise

8. Terminates the resolution if no components remain in the path tuple array
   from `mctpd`, otherwise

9. Sets the device selected in 6 as the current device, and returns to 2

If resolution terminates, then `mctpreactor` creates a
`configures`/`configured_by` association between the Entity Manager D-Bus object
representing the current device at the time of termination at 8, and the `mctpd`
endpoint object from which the path tuple array was obtained. Otherwise, if
resolution aborted for reasons other than insufficient information, an error is
logged.

### Properties of port contacts

Under this proposal, board descriptions may become entirely self-contained
except at the connectors. The act of binding a device to a configuration
propagates information from the `connecting` board to the `connected_by` board
by the `Pinout` relationships described for a `Port`.

A discovered board has a strong relationship to an existing board as a
consequence of the bus on which the FRU EEPROM was discovered being a function
of the connectors it traverses. Entity Manager knows the precise existing board
into which the newly discovered board plugs, and the precise relationships of
other buses traversing the boards' ports.

The usual interpolation of `$bus` and `$address` into other expressions may be
unnecessary at the scope of configuration files, as a bus' `Segment` name is
defined local to the board configuration, with the global bus number derived
over the connecting ports at the time of binding.

A `Segment` consumed by a `Port` of `connecting` `PortType` is instantiated
either by a component on the board, or by an upstream `Port` of `connected_by`
`PortType` on the board. All `Segment` names can be resolved locally, internal
to elements of the board's `Exposes` statement.

Connector definitions on each connecting board must be share a `FormFactor` and
not conflict for the purpose of relation by entries in `Pinout`. It may be
helpful to extract their definition out of the board configuration and refer to
the node by JSON reference.

As this proposal expands the existing `Port` schema it is intended to be
backwards-compatible, and possible to convert configurations over time as
required.

## Alternatives Considered

### Design alternatives

#### Platform Schema

An alternative approach is to describe relationships of MCTP bridges to devices
under an aggregate platform configuration using Entity Manager's [platform
schema][entity-manager-platform-schema].

[entity-manager-platform-schema]:
  https://github.com/openbmc/entity-manager/blob/94eff0ac21b91489baf3c6f38e0a2f40727f4026/docs/platform_configuration.md

In my view this is less desirable as the applicability of a platform
configuration hinges on detection of all its required sub-components. The
presence of these dependencies may change over time due to hardware failure,
data corruption, or other maintenance actions. The absence of any sub-component
invalidates application of the platform configuration.

By contrast, the impact of changing presence over time of one or more components
under the proposed design is isolated to the features of that component.

### Implementation alternatives

#### `ContactGroup`

Early revisions of the proposal used a `ContactGroup` property of nodes in an
`Interfaces` list in place of the usual `Type` property. The proposal was
adjusted to better align with Entity Manager idioms by defining a `Bus` type
with properties that are applicable across `Port`s and devices described in
`Exposes` lists.

## Impacts

### API impact

The proposed design requires:

- `mctpd` publish all network paths on each endpoint's D-Bus object.
- New schema elements be defined for Entity Manager's configurations.

### Security impact

No new classes of security concern. The proposal is an expansion on existing
projects and embodies their already-accepted risks.

### Documentation impact

The utility of the new entity-manager schemas must be documented.

### Performance impact

No impact beyond what's implied by the increase in detail in the Entity Manager
configurations and the relationships between them.

### Developer impact

There is a small increase in implementation complexity to Entity Manager with
regard to tracking the `connecting` and `connected_by` associations between
ports.

There is a moderate increase in implementation complexity to `mctpd` to
implement support for the `Query Hop` and `Get Routing Table Entries` commands,
resolving endpoint paths, and hosting the properties exposing endpoint paths.

There is a relatively large increase in complexity to `mctpreactor` to implement
graph traversals of Entity Manager configuration D-Bus objects for the purpose
of associating `mctpd` endpoint D-Bus objects.

There is no overarching increase in cognitive complexity regarding the
implementations, in the sense handling concurrency, time, undefined and
unspecified behaviour, or other problems that require constant vigilence.

### Upgradability impact

None.

### Organizational

#### Does this proposal require a new repository?

No.

#### Who will be the initial maintainer(s) of this repository?

Not applicable.

#### Which repositories are expected to be modified to execute this design?

- codeconstruct/mctp
- openbmc/entity-manager
- openbmc/dbus-sensors

## Testing

### How will this be tested?

- Configuration validation against schemas in Entity Manager
- Unit testing in Entity Manager
- Unit testing in `mctpd`
- Unit testing in `mctpreactor`

### How will this feature impact CI testing?

No new impact.
