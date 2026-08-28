# Inventory associations for bridged MCTP endpoints

Author: Andrew Jeffery <andrew@codeconstruct.com.au> | @arj

Created: 28th August 2026

## TODO

- Consider implications of
  [`xyz.openbmc_project.Inventory.Connector.Port`][pdi-port]

[pdi-port]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Inventory/Connector/Port.interface.yaml

## Problem Description

Existing platform designs have non-trivial MCTP topologies. Some MCTP endpoints
are bridged, and thus are not immediate neighbours in the network.

Currently, OpenBMC's support for associating MCTP endpoints exposed on D-Bus by
`mctpd` with configuration and inventory information exposed on D-Bus by Entity
Manager only extends to the trivial case of immediate neighbours in an MCTP
network. A context gap exists for all bridged endpoints in any platform design
with a non-trivial MCTP topology.

## Conventions

The words "port" and "connector" are interchanged freely throughout: they refer
to the same physical concept, though "port" may be preferred when referring to
Entity Manager configurations.

## Assumptions

1. All boards that may be either hot- or cold-plugged are self-describing by FRU
   data.

   If a board that may be plugged has no FRU data source, it is not a pluggable
   board for the purpose of Entity Manager. Such a board must be described in
   the same configuration as another board that both:

   1. connects it, and
   2. contains a FRU data source.

## Assertions

1. Hardware designs that do not satisfy assumption 1 are out of scope

## Background and References

### Related specifications

- [DMTF PMCI DSP0236 - Management Component Transport Protocol (MCTP) Base Specification](https://www.dmtf.org/dsp/dsp0236)
- [DMTF PMCI DSP0240 - Platform Level Data Model (PLDM) Base
  Specification][dmtf-dsp0240]
- [DMTF PMCI DSP0248 - Platform Level Data Model (PLDM) for Platform Monitoring
  and Control Specification][dmtf-dsp0248]
- [DMTF PMCI DSP0257 - Platform Level Data Model (PLDM) for FRU Data
  Specification][dmtf-dsp0257]
- [DMTF PMCI DSP0283 - Management Component Transport Protocol (MCTP) Universal
  Serial Bus (USB) Transport Binding Specification][dmtf-dsp0283]

[dmtf-dsp0240]: https://www.dmtf.org/dsp/dsp0240
[dmtf-dsp0248]: https://www.dmtf.org/dsp/dsp0248
[dmtf-dsp0257]: https://www.dmtf.org/dsp/dsp0257
[dmtf-dsp0283]: https://www.dmtf.org/dsp/dsp0283

### Related design proposals

- [OpenBMC in-kernel MCTP](/designs/mctp/mctp-kernel.md)
- [Physical Topology for Inventory Items](/designs/physical-topology.md)
- [designs: add MCTP device discovery for upper-layer services](https://gerrit.openbmc.org/c/openbmc/docs/+/96378)
- [designs: add inventory item ownership][docs-designs-coalesced-features]
- [design: PDI-compatible configuration
  interfaces][docs-designs-em-config-interfaces]

[docs-designs-coalesced-features]:
  https://gerrit.openbmc.org/c/openbmc/docs/+/96526
[docs-designs-em-config-interfaces]:
  https://gerrit.openbmc.org/c/openbmc/docs/+/83264

### Related documentation

- [Linux I2C Sysfs][linux-i2c-sysfs]

[linux-i2c-sysfs]: https://docs.kernel.org/i2c/i2c-sysfs.html

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

### `mctpd`

It is the case today that bridged endpoints may be discovered by `mctpd`. If
configured to do so, `mctpd` discovers active bridged endpoints by polling
addresses in the pools it allocates to bridges.

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

I2C FRU EEPROM detection remains the primary means to discover boards. The bus
on which a FRU EEPROM is detected identifies the global bus index, and therefore
the bus segment and specific connector by which the board is connected.
Identification of the connecting connector implies the connection of the
remaining buses on that connector. Groups of contacts forming buses described on
that port by the configuration enable traversal to resolve path equivalence.

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

1. Define a `FormFactor` property on the `Port` schema to capture some of the
   port's mechanical properties

2. Define a `Interfaces` property on the `Port` schema to capture some of the
   port's electrical properties

3. Prioritise coupling `Port`s by `FormFactor` and `Interfaces` over `Name`.

The prioritisation in 3 allows us to define multiple instances of the same port
type on a board while providing them different names, which avoids both:

- conflicts with Entity Manager's arrangement of D-Bus object paths, and

- [the proposed behaviour][docs-designs-coalesced-features] of coalescing
  multiple `Exposes` nodes using the same name.

If neither `FormFactor` nor `Interfaces` are present then the implementation may
fall-back to the current strategy of coupling by name.

Regarding 2, the `Interfaces` property is a list of `Bus` entries that describe
connectivity provided over the port. Any two ports with matching `FormFactor`
may be considered for connection if their set of `Bus` entries does not
conflict.

Regarding 1, `FormFactor` is proposed as an open enumeration of identifiers for
connector types.

Additionally, an `Interfaces` list of `Bus` entries is introduced for exposed
devices on `Board` configuration types to describe external connectivity
provided by the device, named by `Segment`. Where the device type is a bus hub
or mux, the entry's index in the `Interfaces` list defines the functional index
of the bus, for example the channel number for an I2C mux, or port index for a
USB hub. An entry's `Segment` value must be unique over all `Bus` segments
defined in the board's configuration file. `Interfaces` entries on an MCTP
bridge device must assign the static MCTP port number for the bus to the
`MCTPPort` property on the entry.

`Bus` objects may have an optional `Connects` property, which is a list of `Bus`
object names. `Connects` may be used when the association between `Bus`
interfaces is not implied by the value of `Type`.

### Understanding platform topology from FRU EEPROM addresses

Inventory discovery via I2C FRU EEPROM yields the bus index and device address
at which the FRU EEPROM resides. It is necessarily the case that connections
exist between the EEPROM device and the BMC driving transfers to it.

The Linux kernel documentation describes [how I2C muxes are represented in
sysfs][linux-i2c-sysfs]. The following assumes an understanding of traversing
these links.

On discovery of a board's FRU EEPROM, two traversals take place:

1. In reverse, over sysfs, to resolve the path from the FRU EEPROM's device
   address and bus index back to the related root I2C controller, and

2. forward, over Entity Manager configuration, to resolve the connecting
   connector to the connected board using the path from the relevant root I2C
   controller to the connected board's EEPROM.

Various hardware design scenarios are outlined below for illustration of the
concepts in this design proposal. The concrete sequence for the reverse and
forward traversals are discussed in each where helpful.

All cases share the first initial steps in the resolution sequence:

1. `fru-device` performs a regular bus scan, and discovers a board's FRU EEPROM
   at a given address on a given bus ID

2. `fru-device` describes the board's FRU data on D-Bus

3. Entity Manager is signalled with the announcement of `fru-device`'s D-Bus
   object for the discovered board.

## Building intuition for the proposal

### Enumeration of scenarios

We will illustrate the following design concept as the base case:

<!-- markdownlint-disable MD029 -->

1. Two boards related by one connector with one I2C bus and one USB bus, and on
   the connected board, one I2C FRU EEPROM on its I2C bus segment, and one USB
   MCTP device on its USB segment.

After that, we will consider the following design concepts assuming the simplest
relation of USB segments supporting an MCTP device on each connected board:

2. Three boards related by two connectors on the connecting board, each
   connector with one I2C bus, two I2C segments independently servicing each
   connector on the connecting board, and one I2C FRU EEPROM on each connected
   board's I2C segment

3. Three boards related by two connectors of different shape on the connecting
   board, each connector with one I2C bus, one I2C segment servicing both
   connectors on the connecting board, and one I2C FRU EEPROM on each connected
   board's I2C segment.

4. Three boards related by two connectors of the same shape on the connecting
   board, each connector with one I2C bus, one I2C segment servicing both
   connectors on the connecting board, and one I2C FRU EEPROM on each connected
   board's I2C segment.

To develop the case for USB, we will consider the following design concepts
assuming the simplest unambiguous relation of I2C segments supporting a FRU
EEPROM on each connected board:

5. Two boards related by one connector with one USB bus, and one MCTP device on
   the connected board's USB segment.

6. Two boards related by one connector with one USB bus, where the bus' segment
   on the connecting board is separated from the host controller by a USB hub,
   and one MCTP device on the connected board's USB segment

7. Two boards related by one connector with one USB bus, and two MCTP devices
   behind a USB hub on the connected board.

8. Two boards related by one connector with one USB bus, one MCTP bridge device
   on the connected board's USB segment, and one I2C MCTP device behind the
   bridge.

<!-- markdownlint-enable MD029 -->

<!-- TODO -->

Finally, for completeness with respect to the MCTP specification, we consider
endpoints that are routable through multiple ports.

Despite laying the scenarios out in the order above we will discuss them in a
different order: Scenario 1 as a base case, Scenario 8 as the interesting case,
Scenarios 4, 3 and 2 as they deal with some ambiguities, and then the rest, as
they involve relatively uninteresting complexities. The rest will be described
only as-needed in the course of discussions.

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
  "Type": "Board"
  "Name": "BMC Board",
  "Probe": "...",
  "Exposes": [
    {
      "Type": "SoC"
      "Name": "AST2600",
      "Interfaces": [
        {
          "Type": "Bus",
          "Protocol": "I2C",
          "Segment": "I2C12",
          "Bus": "12",
          "MCTPPort": 0
        },
        {
          "Type": "Bus",
          "Protocol": "USB 2.0",
          "Segment": "USB Port B",
          "Bus": "1", // Root hub ID
          "MCTPPort": 1
        }
      ],
    },
    {
      "Type": "Port"
      "Name": "BMC Port",
      "PortType": "connecting",
      "FormFactor": "SFF-TA-1002 4C+"
      "Interfaces": [
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
    }
  ],
}
```

```json
{
  "Type": "Board"
  "Name": "Managed Board",
  "Probe": "...",
  "Exposes": [
    {
      "Type": "Port"
      "Name": "BMC Port",
      "PortType": "connected_by",
      "FormFactor": "SFF-TA-1002 4C+",
      "Interfaces": [
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

When `Managed Board` is physically connected to `BMC Board`, the connector
connects the traces from bus segment `I2C12` on `BMC Board` to bus segment
`I2C3` traces on `Managed Board`. `fru-device` then discovers the FRU EEPROM at
`0x50` on `Managed Board`, whose FRU data describes `Managed Board`, and causes
Entity Manager to bind the `Managed Board` configuration to the device.

Between the configurations for `BMC Board` and `Managed Board`, the `Interfaces`
node with name `I2C1` associates the I2C bus segments on both across `BMC Port`.

From the perspective of the `AST2600`, the FRU EEPROM for `Managed Board` is
discovered by driving transfers on I2C bus 12. The bus segment for I2C bus 12 is
`I2C12`, whose interface is exposed on `AST2600`. Bus segment `I2C12` is routed
to `BMC Port`. At this point, Entity Manager understands that `Managed Board`
must be plugged into `BMC Port`. This then implies the connection of the bus
segments of the `USB1` contacts, associating `MCTP Device` on `Managed Board`
with the `USB Port B` host controller on `AST2600`.

Increasing the rigour:

#### Connectivity resolution via I2C FRU EEPROM discovery for Scenario 1

Assuming that Entity Manager has been signalled by `fru-device` for
`Managed   Board (1)` as previously described:

1. Entity Manager begins reverse traversal over sysfs to resolve I2C bus ID 12
   to its root I2C controller

   1. Tests if `/sys/bus/i2c/devices/i2c-12/mux_device` exists on the filesystem

   2. Finds the path in 1.1 does not exist

   3. Asserts I2C 12 is a root I2C controller

2. Starting from the board configuration containing the execution context for
   Entity Manager, Entity Manager begins the forward traversal for the bus. It:

   1. Locates the `Interfaces` entry on the `AST2600` component whose `Protocol`
      is `I2C` and `Bus` is `12`.

   2. Uses the `Segment` value defined on the entry from 1 to identify that no
      exposed devices are defined on the segment

   3. Uses the `Segment` value defined on the entry from 1 to identify that
      `BMC Port` connects the segment on `Interfaces` entry named `I2C1`.

   4. Having asserted the `Probe` for `Managed Board`, identifies that
      `BMC Port` on `Managed Board` is a compatible `Port` object by
      `FormFactor` and `Interfaces` for `BMC Port` on `BMC Board`, and that the
      `I2C3` segment on `Managed Board` connects to `I2C12` on `BMC Board` by
      the `I2C1` `Interfaces` entries on the respective `Port` definitions.

   5. Using the `Segment` value defined on the `I2C1` `Interfaces` entry on
      `Managed Board`, identifies the `Board FRU` device at address `0x50`
      exposed on `Managed Board`.

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
  "Type": "Board"
  "Name": "BMC Board",
  "Probe": "...",
  "Exposes": [
    {
      "Type": "SoC"
      "Name": "AST2600",
      "Interfaces": [
        {
          "Type": "Bus",
          "Protocol": "I2C",
          "Segment": "I2C12",
          "Bus": "12",
          "MCTPPort": 0
        },
        {
          "Type": "Bus",
          "Protocol": "USB 2.0",
          "Segment": "USB Port B",
          "Bus": "1", // Root hub ID
          "MCTPPort": 1
        }
      ],
    },
    {
      "Type": "Port"
      "Name": "BMC Port",
      "PortType": "connecting",
      "FormFactor": "SFF-TA-1002 4C+",
      "Interfaces": [
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
    }
  ],
}
```

```json
{
  "Type": "Board"
  "Name": "Managed Board",
  "Probe": "...",
  "Exposes": [
    {
      "Type": "Port"
      "Name": "BMC Port",
      "PortType": "connected_by",
      "FormFactor": "SFF-TA-1002 4C+",
      "Interfaces": [
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
    },
    {
      "Type": "MCTPUSBDevice",
      "Name": "MCTP Bridge"
      "Segment": "USB1",
      "Configuration": 0,
      "Interface": 1
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
    },
    {
      "Type": "EEPROM"
      "Name": "Board FRU",
      "Address": "0x50",
      "Segment": "I2C3",
    },
    {
      "Type": "MCTPI2CTarget"
      "Name": "MCTP Device",
      "Address": "0x12",
      "Segment": "I2C2",
    }
  ]
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
|                                   |                   | Port 1 || Port   |                           Board     |
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
| |         |  USB 1 |  USB Port B  |         +--+      | Port 2 || Port   |                           Board     |
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
|                                   |                   | Port 1 || Port   |                           Board     |
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
| |         |  USB 1 |  USB Port B  |         +--+      | Port 2 || Port   |                           Board     |
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
  "Type": "Board"
  "Name": "BMC Board",
  "Probe": "...",
  "Exposes": [
    {
      "Type": "SoC"
      "Name": "AST2600",
      "Interfaces": [
        {
          "Type": "Bus",
          "Protocol": "I2C",
          "Segment": "I2C12",
          "Bus": "12",
          "MCTPPort": 0
        },
        {
          "Type": "Bus",
          "Protocol": "USB 2.0",
          "Segment": "USB Port B",
          "Bus": "1", // Root hub ID
          "MCTPPort": 1
        }
      ],
    },
    {
      "Type": "PCA9540B"
      "Name": "I2C Mux",
      "Address": 0x71,
      "Segment": "I2C12",
      "Interfaces": [
        {
          "Type": "Bus",
          "Name": "CH1",
          "Protocol": "I2C",
          "Segment": "I2C12.1"
        }
        {
          "Type": "Bus",
          "Name": "CH2",
          "Protocol": "I2C",
          "Segment": "I2C12.2",
        }
      ],
    },
    {
      "Type": "USX2422"
      "Name": "USB Hub",
      "Segment": "USB Port B",
      "Interfaces": [
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
    },
    {
      "Type": "Port"
      "Name": "Ext Port 1",
      "PortType": "connecting",
      "FormFactor": "SFF-TA-1002 4C+",
      "Interfaces": [
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
    }
    {
      "Type": "Port"
      "Name": "Ext Port 2",
      "PortType": "connecting",
      "FormFactor": "SFF-TA-1002 4C+",
      "Interfaces": [
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
    }
  ]
}
```

```json
{
  "Type": "Board"
  "Name": "Managed Board",
  "Probe": "...",
  "Exposes": [
    {
      "Interfaces": [
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
      "Name": "Ext Port",
      "FormFactor": "SFF-TA-1002 4C+",
      "PortType": "connected_by",
      "Type": "Port"
    },
    {
      "Type": "EEPROM"
      "Name": "Board FRU",
      "Address": "0x50",
      "Segment": "I2C",
    },
    {
      "Type": "MCTPUSBDevice"
      "Name": "MCTP Device",
      "Segment": "USB",
    }
  ]
}
```

#### Discussion

In the arrangement above we have the strong relationship between a port and an
I2C mux channel. Linux exposes mux channels as their own I2C bus. Accesses
through the mux-channel-specific I2C bus device lock the path from the root
controller to the leaf mux channel before driving bus transfers. As the FRU
EEPROM is discovered by the port-specific mux-channel we have the information
necessary to accurately represent the device topology.

#### Connectivity resolution via I2C FRU EEPROM discovery for Scenario 2

Assuming that:

- Entity Manager has been signalled by `fru-device` for `Managed Board (1)` as
  previously described, and

- Linux has exposed new I2C bus devices mapped to the mux channels of the
  `PCA9540B` on `BMC Board` by that device's driver, where channels 0 and 1
  (mapping to bus segments `I2C12.1` and `I2C12.2`) are given Linux bus IDs 13
  and 14 respectively

1. Entity Manager begins reverse traversal over sysfs to resolve I2C bus 13 to
   its root controller

   1. Tests if `/sys/bus/i2c/devices/i2c-13/mux_device` exists on the filesystem

   2. Determines that the path from 1.1 exists

   3. Asserts that I2C bus 13 is not a root I2C controller

   4. Resolves the symbolic link at `/sys/bus/i2c/devices/i2c-13/mux_device` to
      the path to the parent device. In context, this path is `.../12-0071`

   5. From the basename element of the path resolved in 1.4, extracts bus index
      `12`

   6. From the bus index extracted at 1.5, tests if
      `/sys/bus/i2c/devices/i2c-12/mux_device` exists on the filesystem

   7. Determines that the path from 1.6 does not exist

   8. Asserts that I2C bus 12 is a root I2C controller

2. Starting from the board configuration containing the execution context for
   Entity Manager, Entity Manager begins the forward traversal for the bus. It:

   1. Locates the `Interfaces` entry on the `AST2600` component whose `Protocol`
      is `I2C` and `Bus` is `12`.

   2. Uses the `Segment` value `I2C12` defined on the entry from 2.1 to
      determine the set of exposed devices connected to that segment

   3. Identifies that `I2C Mux` is a `PCA9540B` in the set from 2 whose channel
      `0` maps to I2C bus ID 13

   4. Using channel index `0` extracted at 2.3, uses the `Segment` value
      `I2C12.1` defined on the corresponding `Interfaces` entry to determine the
      set of exposed devices connected to that segment. In context, this is the
      empty set.

   5. Expands the set of connected devices to the union of the sets discovered
      in 2.2 and 2.4

   6. Again using the channel index `0` extracted at 2.3, uses the `Segment`
      value `I2C12.1` defined on the corresponding `Interfaces` entry to
      determine that `Ext Port 1` connects the segment on `Interfaces` entry
      named `I2C`

   7. Having asserted the `Probe` for `Managed Board`, identifies that
      `Ext Port` on `Managed Board` is a compatible `Port` object by
      `FormFactor` and `Interfaces` for `Ext Port 1` on `BMC Board`, and that
      the `I2C` segment on `Managed Board` connects to `I2C12.1` on `BMC Board`
      by the `I2C` `Interfaces` entries on the respective `Port` definitions.

   8. Uses the `Segment` value defined on the `I2C` `Interfaces` entry on
      `Managed Board` to determine the set of exposed devices on `Managed Board`
      connected to that segment

   9. Expands the set of connected devices to the union of the sets discovered
      in 2.2, 2.4, and 2.8

   10. Identifies the `Board FRU` device at address `0x50` exposed on
       `Managed Board` in the connected set

## Enumerating MCTP bridge ports under Linux

Bridge configurations on BMCs running Linux may be dynamic by nature of devices
coming and going. DSP0236 requires that bridge port numbers are static (v1.4.0,
Clause 13.13, Table 27). The two must be reconciled.

An intuitive allocation of port IDs under Linux is by way of `ifindex`. However,
this fails quickly for multiple transport types under hot-plug events:

- For I2C, an interface is created for the root of each bus segment
- For USB, an interface is created for each device, as USB is point-to-point

`ifindex` values [are also recycled][linux-dev_index_reserve].

[linux-dev_index_reserve]:
  https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/net/core/dev.c?h=v7.3#n10895

For the purpose of MCTP, a connected device is a neighbouring device if there is
no intervening MCTP bridge. Under this view, any components connecting bus
segments along the path to the neighbouring device are implementation details
that the transport binding abstracts over.

For the purposes of MCTP under Linux, an MCTP interface represents the
out-of-scope addressing required to configure a path over segments of its
transport to expose the bus address of the neighbouring device, or its pipe
endpoint in the case of point-to-point transports. The port associated with the
interface used to access the neighouring device can be resolved along the path
to the root controller for the interface's bus, as the remaining details are
implementation concerns of the transport binding that exist beyond the boundary
of the local physical port.

Concretely, under [Scenario 2](#scenario-2-a-common-multi-port-design-point)
above, Linux on the AST2600 may expose the following MCTP interfaces:

- `mctpi2c0`: For devices on segment `I2C12`
- `mctpi2c1`: For devices along path (`I2C12`, `I2C12.1`, `I2C`)
- `mctpi2c2`: For devices along path (`I2C12`, `I2C12.2`, `I2C`)
- `mctpusb0`: For the device attached via path (`USB Port B`, `B.1`, `USB`)
- `mctpusb1`: For the device attached via path (`USB Port B`, `B.2`, `USB`)

This design proposes that the port ID for interfaces `mctpi2c0`, `mctpi2c1`, and
`mctpi2c2` resolve to 0, associated with `I2C 12` on the `AST2600` component,
and for interfaces `mctpusb0` and `mctpusb1` resolve to 1, associated with
`USB 1` on the `AST2600` component. These resolved port IDs shall be reflected
in `mctpd`'s response for a `Get Route Table Entries` query issued to the
`AST2600`.

One alternative choice for Scenario 2 is the to configure the downstream hub
ports on `USB Hub` as the ports described for the purpose of
`Get Routing Table Entries`. This is achieved by assigning the `MCTPPort`
property on each. The traversal performed to resolve a port ID terminates at the
first encounter of the `MCTPPort` property.

## General algorithms of the proposal

### Resolving connectivity in Entity Manager via I2C FRU EEPROM discovery

#### Reverse traversal over sysfs

Given the following definitions:

- Let `device` be a tuple containing the discovered FRU EEPROM's bus and address
- Let `parent` be a sysfs path to a mux device
- Let `bus` be a Linux bus device index
- Let `path` be an ordered list of traversed Linux bus device indices

Entity Manager takes the following steps to populate `path`:

1. Set `path` to the empty list
2. Set `bus` to the bus component of `device`
3. Prepend `bus` as a component of `path`
4. Use `bus` to compose the sysfs I2C device path for the bus
5. Test for the existence of `mux_device` under the path from 4
6. Terminate if the test in 5 failed, otherwise
7. Set `parent` to the value of the resolved `mux_device` symbolic link
8. Set `bus` to the bus component of the basename of `parent`
9. Return to 3

#### Forward traversal over Entity Manager configurations

With the following definitions:

- Let `path` be the relevant, non-empty, ordered list of Linux bus device
  indices determined by the reverse traversal process

- Let `root` be the exposed component in the Entity Manager board configuration
  representing the execution context for Entity Manager

- Let `address` be the address of the discovered I2C FRU EEPROM

- Let `board` the an Entity Manager board configuration

- Let `bus` be a Linux bus device index

- Let `component` be an element in an `Exposes` list of an Entity Manager board
  configuration

- Let `interface` be an entry of an `Interfaces` list in an Entity Manager board
  configuration

- Let `connected` be a set of I2C device configurations

- Let `ports` be an ordered list of `Port` instances

- Let `transaction` be a speculative set of changes to system configuration that
  are either committed or rejected in its entirety

Entity Manager takes the following steps to resolve I2C connectivity:

1. Set `connected` to the empty set
2. Set `component` to `root`
3. Set `board` to the configuration containing `component`
4. Set `bus` to the head of `path`, and `path` to its tail

5. Select the entry in the `Interfaces` list of `component` whose `Protocol`
   value is `I2C` and whose `Bus` value is `bus`

6. Abort if no such entry exists, otherwise
7. Set `interface` to the value from 5

8. Derive the set of devices exposed on `board` whose `Segment` value matches
   the `Segment` value of `interface`.

9. Abort if the intersection of the set of addresses of the devices in
   `connected` with the set of addresses of the set of devices derived at 8 is
   non-empty, otherwise

10. Set `connected` to the union of itself with the set from 8

11. Terminate if there exists an element of `connected` whose `Address` property
    matches `address`, otherwise

12. If `path` is not empty:

    1. Select from `connected` set of mux devices of the I2C bus whose parent
       channel maps to the head element of `path`.

    2. Abort if the selection in 12.1 yields a set size greater than 1

    3. If the selection 12.1 yields a set of size 1:

       1. Set `component` to element of the set from 12.1

       2. Return to 3

13. Select the set of `Port`s exposed on `board` that have an `Interfaces` entry
    whose `Segment` value matches the `Segment` value of `interface`.

14. Abort if the size of the set from 13 is not 1, otherwise

15. Set `port` to the element of the set from 13

16. If `port` is not yet populated, prepare a transaction in `transaction`
    binding the probed configuration to the discovered board and the discovered
    board to `port`, then

    1. For each I2C mux on each I2C bus traversing `port` described in the
       probed configuration, decorate each entry in `Interfaces` with the `Bus`
       property whose value takes the corresponding Linux I2C bus ID

17. Set `component` to the `Port` instance in the configuration of the board
    connected on `port`

18. Return to 3

Where a transaction is active, aborting the resolution aborts the transaction,
while normal termination commits the transaction.

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

#### `mctpreactor`

Entity Manager, having probed the configuration for both boards, publishes the
device configurations on D-Bus. With both sets of information published by
`mctpd` and Entity Manager, `mctpreactor` takes the steps outlined below.

The traversal is similar to the case of Entity Manager resolving the path for an
I2C device, though is more abstract, as it uses the route path over the MCTP
topology as input rather than the bus path over the I2C topology. The bus path
over the I2C topology is applicable for the case of an MCTP route hop traversing
the I2C transport binding. Equivalent traversals must be implemented for the
remaining MCTP transport bindings.

1. Identifies the device that is the execution environment for `mctpd` in the
   Entity Manager configurations, and sets this as the current device.

2. Consumes the head tuple from the path tuple array published by `mctpd` for
   the endpoint, leaving its tail

3. Selects the `Bus` object from the `Interfaces` list of the current device
   whose `MCTPPort` value corresponds to the the port component of the tuple
   obtained in 2[^1].

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
      selected `Interfaces` entries on the relevant `Port`s or by intervening
      mux or switch components

7. Aborts the resolution if the selection in 6 yields no result, otherwise

8. Terminates the resolution if no components remain in the path tuple array
   from `mctpd`, otherwise

9. Sets the device selected in 6 as the current device, and returns to 2

[^1]:
    Note that this may require resolution via traversal back from the port on
    which `MCTPPort` is defined.

If resolution terminates, then `mctpreactor` creates a
`configures`/`configured_by` association between the Entity Manager D-Bus object
representing the current device at the time of termination at 8, and the `mctpd`
endpoint object from which the path tuple array was obtained. Otherwise, if
resolution aborted for reasons other than insufficient information, an error is
logged.

## Consequences of the proposal

### Properties of port traversal

Under this proposal, board descriptions may become entirely self-contained
except at the connectors. The act of binding a device to a configuration
propagates information from the `connecting` board to the `connected_by` board
by the `Interfaces` relationships described for a `Port`.

A discovered board has a strong relationship to an existing board as a
consequence of the bus on which the FRU data was discovered being a function of
the connectors it traverses. Entity Manager knows the precise existing board
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
not conflict for the purpose of relation by entries in `Interfaces`. It may be
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
- openbmc/pldm

## Testing

### How will this be tested?

- Configuration validation against schemas in Entity Manager
- Unit testing in Entity Manager
- Unit testing in `mctpd`
- Unit testing in `mctpreactor`
- Integration testing

#### Integration testing

Integration testing must cover the fundamental actions of the proposal:

1. FRU data discovery
2. FRU data path resolution by Entity Manager
3. Board connectivity resolution by Entity Manager
4. Endpoint connectivity path resolution by mctpreactor
5. Validation of created associations

It's desirable that this can be achieved in software using emulation, removing
hardware requirements.

Some observations:

1. QEMU already supports several BMC SoC evaluations boards
2. The [usbredir protocol][usbredir] is [supported by QEMU][qemu-usbredir]
3. [mctp-dev][github-codeconstruct-mctp-dev] supports `usbredir`
4. `mctp-dev` has some existing support for PLDM
5. FRU data may be exposed over PLDM using [DSP0257][dmtf-dsp0257]
6. A process for device discovery using FRU data sourced from PLDM is described
   in Appendix A

[usbredir]: https://gitlab.freedesktop.org/spice/usbredir/
[qemu-usbredir]:
  https://gitlab.com/qemu-project/qemu/-/commit/69354a83346da2d4d8eb9cda18b639794566c902
[github-codeconstruct-mctp-dev]: https://github.com/codeconstruct/mctp-dev

Given these observations, integration testing may occur by:

1. Providing Entity Manager configuration for at least one BMC SoC evaluation
   board

2. Describing the USB topology in the configuration at 1

3. Adding support for FRU data to `mctp-dev`

4. Connecting a `mctp-dev` instance to a QEMU instance configured for the BMC
   SoC evaluation board in 1.

The sequences described in Appendix A should then follow for evaluation of the
resulting associations.

### How will this feature impact CI testing?

No new impact.

## Appendix A: FRU data discovery over PLDM

While this proposal describes its behaviour in terms of FRU data discovery over
I2C, it is not constrained to I2C. It is expected that FRU data discovered over
PLDM will become increasingly popular.

While DMTF defines PLDM transport bindings for both MCTP and RBT, we will
disregard RBT for the moment.

### Observations

<!-- markdownlint-disable MD029 -->

1. This design proposal concerns forming associations between MCTP endpoints and
   their configuration hosted by Entity Manager.

2. The associations in 1 are not required for functional MCTP communication to a
   device.

3. [DSP0257][dmtf-dsp0257] defines the behaviour of PLDM for FRU Data. It builds
   on PDRs as defined in [DSP0248][dmtf-dsp0248].

4. Support at a PLDM terminus for DSP0257 can be determined by its response to
   `GetPLDMCommands` indicating support for `GetFRURecordTableMetadata` and
   `GetFRURecordTable`

5. To issue `GetPLDMCommands` to a terminus, the device must be configured for
   PLDM communication by the PLDM initialisation agent.

   1. The agent should follow the sequence outlined in DSP0240 v1.2.0 Clause 11
      Annex A.

   2. In OpenBMC, `pldmd` acts as the PLDM initialisation agent

6. To configure the device as a PLDM terminus, the PLDM initialisation agent
   must learn that the underlying MCTP endpoint advertises PLDM support

   1. In OpenBMC, `mctpd` maintains the configuration of MCTP networks.

   2. Often, `mctpd` takes the role of Top-most Bus Owner, responsible for
      maintenance across the entire network.

   3. `mctpd` advertises the supported message types for each known endpoint.

7. To configure the device as an MCTP endpoint, the device's Bus Owner must
   assign an EID to the device.

8. To assign an EID to the device, the device's bus owner must be aware of its
   existence.

9. DSP0236 provisions for hotplug discovery of devices. `Discovery Notify` is
   provided for buses of transport bindings that are not discoverable in their
   own right. Any transport binding not defining `Discovery Notify` as the
   discovery mechanism must specify its own.

10. MCTP device hotplug discovery notification only extends to the device's bus
    owner

11. The device's bus owner and the PLDM initialisation agent may not be
    executing on the same device

12. `mctpd` supports polling EIDs contained in pools allocated to neighbouring
    bridges.

### Assuming

1. That all hotplug MCTP devices are discoverable

2. That the device of the PLDM terminus, its MCTP bus owner, and the device of
   the PLDM initialisation agent are all routable on the one MCTP network.

3. FRU data discovery occurs in a regular OpenBMC environment

### Discovery sequence

One of the following sequences occurs:

1. Where the MCTP bus owner and the PLDM initialisation agent for the PLDM
   terminus are the same device and the device is an OpenBMC system:

   1. The defined discovery mechanism for the MCTP transport of the device
      signals `mctpd` to dynamically allocate an endpoint ID to the device.

   2. `mctpd` completes the assignment and tracks the device

   Otherwise,

2. `mctpd` polls the pools of endpoint IDs it has allocated to neighbouring
   bridges to discover the appearance of downstream devices.

   1. `mctpd` tracks the endpoint ID once it receives a response to a poll

3. Having tracked an endpoint, `mctpd` queries then advertises the endpoint's
   supported message types, including the PLDM message type.

4. `pldmd` is signalled with the information of a newly discovered, PLDM-capable
   MCTP endpoint

5. `pldmd` performs the terminus initialisation sequence in DSP0240 v1.2.0
   Clause 11 Annex A for the endpoint.

6. `pldmd` determines the terminus supports DSP0257 and advertises the FRU data

7. Entity Manager is signalled with the information of the newly discovered FRU

8. Understanding the FRU data is sourced from PLDM rather than I2C, Entity
   Manager resolves the path to the PLDM terminus via its MCTP endpoint using
   the algorithm previously outlined for `mctpreactor`.

<!-- markdownlint-enable MD029 -->

## Appendix B: Path identity for USB MCTP devices

### Neighbour devices

The USB specification has integrated support for dynamic attachment and removal
of devices, and requires they be self-describing.

The Linux USB stack responds to dynamic attachment, binding drivers to device
functions as appropriate.

[DSP0283][dmtf-dsp0283] defines a USB class (0x14) for USB MCTP devices.

Linux implements a [class driver for the MCTP USB
class][linux-drivers-net-mctp-usb].

[linux-drivers-net-mctp-usb]:
  https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/net/mctp/mctp-usb.c?h=v6.15#n368

The USB MCTP class driver instantiates an MCTP interface for each attached USB
MCTP device.

Instantiation of the interface occurs concurrent to any FRU discovery process.

USB MCTP interfaces can be resolved to their underlying USB device through
`sysfs` by resolving their `net` link. For example, `mctpusb0`'s `net` link can
befound at `/sys/class/net/mctpusb0/device/net`, which may for example resolve
to `/sys/devices/platform/ahb/1e6a3000.usb/usb1/1-1/1-1:1.0/net`, or
`/sys/devices/pci0000:00/0000:00:14.0/usb3/3-6/3-6.4/3-6.4.2/3-6.4.2.1/3-6.4.2.1 :1.0/net`.

Both the path to the net device and the name of the parent directory [encode the
path taken to reach the device's function][linux-docs-abi-stable-usb]. An
implementation may use either as an information source in a strategy to resolve
the path under the USB topology.

[linux-docs-abi-stable-usb]:
  https://docs.kernel.org/admin-guide/abi-stable.html#abi-sys-bus-usb-devices-busnum-port-port-config-num-interface-num-supports-autosuspend

As USB is point-to-point, segment names are only necessary to relate host
controller and hub USB port segments to `Bus` entries in a `Port`'s `Interfaces`
list in Entity Manager configurations. Nonetheless, we will require they be
defined for all segments.

Having acquired the USB path, Entity Manager or other components such as
`mctpreactor` can perform the forward traversal to locate the device under the
current platform configuration.

As it may be the case that the device is a PLDM terminus providing FRU data, an
EID must be assigned to the device. Any appropriate association will be applied
later when necessary information becomes available from Entity Manager.

Where the MCTP endpoint is a PLDM terminus providing FRU data, the process
outlined in Appendix A then applies.

### Bridged devices

The USB topology on downstream ports of MCTP bridges is not exposed by
`Get Routing Table Entries`: [DSP0283][dmtf-dsp0283] v1.0.0 Clause 6.7 specifies
the physical address format as the tuple of USB device address and endpoint
number.

A USB device address is dynamically assigned by the USB system software to
abstract over port segments to the device. The method to choose the device
address is not defined and is a design-point for the system software
implementation. It follows that the device address cannot be defined ahead of
time on USB device configurations in Entity Manager.

Above we discussed the placement of the `MCTPPort` property. For all buses, it
must be placed to eliminate ambiguity beyond the port. For MCTP transports that
are dynamically addressed or whose address scheme does not reveal the bus
topology, any mux or hub downstream from an MCTP bridge must not have an
intervening connector, and the MCTP bridge implementation must support
`Get Routing Table Entries` in an unambiguous form.

#### Bridge device scenarios

Scenario EBNF grammar:

```ebnf
Configuration = Host Controller, { USB Segment, Node }, [ USB Segment, USB Device ]
Host Controller = "HC"
USB Segment = " "
Node = MCTP Bridge | USB Hub | Connector
MCTP Bridge = "B"
USB Hub = "H"
Connector = "|"
USB Device = "D"
Element = Host Controller | Node | USB Device
```

All `Element`s connected by a USB Segment adjacent to a `Connector` constitute a
board.

##### Design group with observable topology

- `HC D`
- `HC H D`
- `HC | D`
- `HC H | D`
- `HC | H D`
- `HC H | H D`

These configurations are all supported as the path to the device in the USB
topology is observable under Linux at the host controller.

##### Design group with unambiguous abstract topology

- `HC B D`
- `HC H B D`
- `HC | B D`
- `HC H | B D`

These configurations are all supported as the location of the device is
unambiguous without relying on implementation details of the bridge.

##### Design group with implementation-dependent abstract topology

- `HC | B H D`

This configuration may be supported. `MCTPPort` definitions are self-contained
on hub of the connected board. The bridge implementation for
`Get Routing Table Entries` must reflect the placement of `MCTPPort` properties
on the hub's downstream ports in the configuration.

##### Design group with ambiguous abstract topology

- `HC B | H D`
- `HC B H | H D`

These configurations cannot be supported. `MCTPPort` definitions are not
self-contained as they cannot be both placed on the board containing the bridge,
and placed at locations adjacent to a device. The USB topology beyond the bridge
is not observable to Linux at the host controller.
