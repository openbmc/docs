# Inventory associations for bridged MCTP endpoints

Author: Andrew Jeffery <andrew@codeconstruct.com.au> | @arj

Created: 28th August 2026

## Conventions

The words "port" and "connector" are interchanged freely throughout: they refer
to the same physical concept, though "port" may be preferred when referring to
Entity Manager configurations.

## Background and References

- [DMTF PMCI DSP0236 - Management Component Transport Protocol (MCTP) Base Specification](https://www.dmtf.org/dsp/dsp0236)
- [OpenBMC in-kernel MCTP](designs/mctp/mctp-kernel.md)
- [mctpd](https://github.com/CodeConstruct/mctp/blob/31ac0e86b0e23d32abf34d191653a87a47f3c9a7/docs/mctpd.md)
- [Entity Manager](https://github.com/openbmc/entity-manager/blob/f9ab4384184c74ab05de5a2a05ac5ca5a55b64dd/README.md)
- [mctpreactor](https://gerrit.openbmc.org/c/openbmc/dbus-sensors/+/69111)
- [Physical Topology for Inventory Items](/designs/physical-topology.md)

### MCTP

The topology of an MCTP network is described in terms of devices, bridge
devices, bridge ports, buses, and routes. Information on these elements and
relationships between them can be built using MCTP's `Get Routing Table Entries`
and `Query Hop` commands.

### Related work

- [schemas: introduce platform schema](https://gerrit.openbmc.org/c/openbmc/entity-manager/+/87415)
- [schemas: Describe MCTP USB targets](https://gerrit.openbmc.org/c/openbmc/entity-manager/+/77381)
- [schemas: platform: describe MCTP bridged endpoints](https://gerrit.openbmc.org/c/openbmc/entity-manager/+/93523)

## Problem Description

Existing platform designs have non-trivial MCTP topologies. Some MCTP endpoints
are bridged, and thus are not immediate neighbours in the network.

Currently, OpenBMC's support for associating MCTP endpoints exposed on D-Bus by
`mctpd` with configuration and inventory information exposed on D-Bus by Entity
Manager only extends to the trivial case of immediate neighbours in an MCTP
network. A context gap exists for all bridged endpoints in any platform design
with a non-trivial MCTP topology.

## Requirements

An association must exist for each MCTP endpoint whose device is described by
configuration in Entity Manager, regardless of its location in the MCTP network.

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
2. The upstream interface for a given bus segment
3. Each bus interface exposed by each port on a board

FRU EEPROM detection remains the means to discover boards. The bus on which a
FRU EEPROM is detected identifies the global bus index, and therefore the bus
segment and specific connector by which the board is connected. Identification
of the connecting connector implies the connection of the remaining buses on
that connector. Contact groups described that port in the configuration enable
path traversal to resolve path equivalence.

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

- I3C (DSP0233)
- I2C (DSP0237)
- PCIe VDM (DSP0238)
- Serial (DSP0253)
- USB (DSP0283)

This delegation of responsibility and the implementation-specific nature of EID
allocation by bridges means there is no implicit global view of the composition
of an MCTP network. If any such view is to be built, it requires explicit
intervention.

Graph-traversal of bridge responses to DSP0236's mandatory
`Get Routing Table Entries` commands can be used to determine the entire MCTP
network topology in terms of bridge ports, their physical transports, and
endpoints routable via a given port.

Restated for the problem at hand, information gathered by graph-traversal of
`Get Routing Table Entries` responses allows us to construct a location identity
for each reachable endpoint in the MCTP network in terms of its path in the
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

MCTP is not constrained to I2C as a physical transport.

Therefore, the support currently provided by Entity Manager for describing the
connectivity topology is insufficient to solve the problem at hand.

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

The core proposal is to expand the `PortType` property of Entity Manager's
`Port` schema to include `connecting` and `connected_by` relationships along
with remaining definitions, such that the following example configurations are
accepted.

A new `Contacts` list of `Bus` entries is introduced on the `Port` schema to
describe connectivity provided by a connector.

An `Interfaces` list of `Bus` entries is also introduced for exposed devices on
`Board` configuration types to describe external connectivity provided by the
device, named by `Segment`. `Segment` names must be unique over the bus segments
described in the board's configuration file. `Bus` objects in an MCTP bridge
device's `Interfaces` list must have an `MCTPPort` property describing the
static MCTP port number for the bus interface.

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
Scenarios 3 and 2 as they deal with some ambiguities, and then the rest, as they
involve relatively uninteresting complexities.

<!-- prettier-ignore-start -->
> [!NOTE]
> TODO: Discuss that "upstream" vs "downstream" port properties can be derived
> from path reachability from the graph root
<!-- prettier-ignore-end -->

<!-- prettier-ignore-start -->
> [!NOTE]
> TODO: Trade-off analysis for `"ContactGroup": "..."` property vs `"Type":
> "ContactGroup", "Name": "..."`
<!-- prettier-ignore-end -->

### Scenario 1: The base case

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
      "Contacts": [
        {
          "Type": "Bus",
          "Protocol": "I2C",
          "Segment": "I2C12",
          "Group": "I2C1"
        },
        {
          "Type": "Bus",
          "Protocol": "USB 2.0",
          "Segment": "USB Port B",
          "Group": "USB1"
        }
      ],
      "Name": "BMC Port",
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
      "Contacts": [
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
      "PortType": "connected_by",
      "Type": "Port"
    },
    {
      "Type": "EEPROM",
      "Name": "Board FRU",
      "Address": "0x50",
      "Segment": "I2C1"
    },
    {
      "Type": "MCTPUSBDevice",
      "Name": "MCTP Device",
      "Segment": "USB1",
      "Configuration": 0,
      "Interface": 1
    }
  ]
  "Name": "Peripheral Board",
  "Probe": "...",
  "Type": "Board"
}
```

#### Discussion of Scenario 1

In this example we have two boards: `BMC Board` exposing an `AST2600` BMC, and
`Peripheral Board` exposing a USB-based MCTP device `MCTP Device`.

On `BMC Board` an AST2600 is described as exposing one I2C bus interface, bus
segment `I2C12`, and one USB interface, bus segment `USB Port B`. The bus
segments local to `AST2600` are associated with the controller driving them by
their `Bus` properties.

The bus segment for `I2C12` routes directly to a port on `BMC Board`:
`BMC Port`. USB connectivity is also routed to `BMC Port`.

When `Peripheral Board` is physically connected to `BMC Board`, the physical
connector connects the traces from bus segment `I2C12` on `BMC Board` bus
segment `I2C3` traces on `Peripheral Board`. Between the two configurations, the
port contacts node of type `Bus` with name `I2C1` associates the bus segments on
either board across the connector described in the configuration.

After the boards are physically plugged, the FRU EEPROM at `0x50` on
`Peripheral Board` is discovered over I2C. Its FRU data describes
`Peripheral Board`, which causes Entity Manager to bind the `Peripheral Board`
configuration to the device.

From the perspective of the `AST2600`, the FRU EEPROM for `Peripheral Board` is
discovered by driving transfers on I2C bus 12. The bus segment for I2C bus 12 is
`I2C12`, whose interface is exposed on `AST2600`. Bus segment `I2C12` is routed
to `BMC Port`. At this point, Entity Manager understands that `Peripheral Board`
must be plugged into `BMC Port`. This then implies the connection of the bus
segments of the `USB1` contacts, associating `MCTP Device` on `Peripheral Board`
with the `USB Port B` root hub on `AST2600`.

### Scenario 8: The interesting case

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
      "Contacts": [
        {
          "Type": "Bus",
          "Protocol": "I2C",
          "Segment": "I2C12",
          "Group": "I2C1"
        },
        {
          "Type": "Bus",
          "Protocol": "USB 2.0",
          "Segment": "USB Port B",
          "Group": "USB1"
        }
      ],
      "Name": "BMC Port",
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
      "Contacts": [
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
      "PortType": "connected_by",
      "Type": "Port"
    },
    {
      "Interfaces": [
        {
          "Type": "Bus",
          "Protocol": "USB 2.0",
          "Segment": "USB1",
          "Configuration": 0,
          "Interface": 1,
          "MCTPPort": 0
        },
        {
          "Type": "Bus",
          "Protocol": "I2C",
          "Segment": "I2C2",
          "Address": "0x10",
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
      "Segment": "I2C1",
      "Name": "Board FRU",
      "Type": "EEPROM"
    },
    {
      "Address": "0x12",
      "Segment": "I2C2",
      "Name": "MCTP I2C Target",
      "Type": "MCTPI2CTarget"
    }
  ]
  "Name": "Peripheral Board",
  "Probe": "...",
  "Type": "Board"
}
```

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
   address component of the tuple obtained in 2, subject to:

   1. The device is exposed on the board containing the current device, and is
      not of `Port` type, and the `Segment` value of the device matches the
      `Segment` value of the `Bus` configuration obtained in 3, or

   2. The device is exposed on the board containing the current device, and is
      not of `Port` type, and the `Segment` value of the device is reachable
      from the `Segment` value contained in 3, where relevant segments are
      related by intervening mux or switch components, or

   3. The device is exposed on a board transitively connected through `Port`s
      exposed on boards related by `connecting` and `connected_by` associations
      from the board containing the current device, and has a `Segment` value
      that is reachable from the `Segment` value of the bus configuration
      obtained in 3 where relevant segments are related by any combination of
      selected `Contacts` entries on the relevant `Port`s or by intervening mux
      or switch components

7. Aborts the resolution if the selection in 6 yields no result, otherwise

8. Terminates the resolution if no components remain in the path tuple array
   from `mctpd`, otherwise

9. Sets the device selected in 6 as the current device, and returns to 2

If resolution terminates, then `mctpreactor` creates an association between the
Entity Manager D-Bus object representing the current device at the time of
termination at 8, and the `mctpd` endpoint object from which the path tuple
array was obtained. Otherwise, if the resolution aborted, an error is logged.

### Properties of port contacts

Under this proposal, board descriptions may become entirely self-contained
except at the connectors. The act of binding a device to a configuration
propagates information from the `connecting` board to the `connected_by` board
by the `Contacts` relationships described for a `Port`.

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

Connector definitions on each connecting board must be equivalent for the
purpose of relation by entries in `Contacts`. It may be helpful to extract their
definition out of the board configuration and refer to the node by JSON
reference.

As this proposal expands the existing `Port` schema it is intended to be
backwards-compatible, and possible to convert configurations over time as
required.

## Alternatives Considered

> (2 paragraphs) Include alternate design ideas here which you are leaning away
> from. Elaborate on why a design was considered and why the idea was rejected.
> Show that you did an extensive survey about the state of the art. Compares
> your proposal's features & limitations to existing or similar solutions.

## Impacts

### API impact

The proposed design requires:

- `mctpd` publish all network paths on each endpoint's D-Bus object.
- New schema elements be defined for Entity Manager's configurations.

### Security impact

The design requires expansion of the set of used MCTP control messages. `mctpd`
is implemented in C. Increasing size of the set of supported message types poses
some risk of vulnerability due to parsing errors in a language that is not
memory safe. However, this is a class of hazard that is already present.

### Documentation impact

The additions to the Entity Manager schemas must be documented, as must their
use and utility.

### Performance impact

The proposal increases the detail of the hardware descriptions in the Entity
Manager configurations. This may have some impact on memory consumption.

`mctpreactor` must perform more work to implement the required associations.
This may have some processing overhead proportional to the increase in system
design detail.

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

No

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

## Appendix A: Entity Manager configurations for various scenarios

### Scenario 6

```json
{
  "Exposes": [
    {
      "Interfaces": [
        {
          "Protocol": "I2C",
          "Segment": "I2C12",
          "Bus": "12",
        }
        {
          "Protocol": "USB 2.0",
          "Segment": "USB Port B",
          "Bus": "1" // Root hub ID
          "MCTPPort": 0,
        }
      ],
      "Name": "AST2600",
      "Type": "SoC"
    },
    {
      "Interfaces": [
        {
          "Protocol": "USB 2.0",
          "Segment": "USB Port B"
        }
        {
          "Protocol": "USB 2.0",
          "Segment": "USB Hub Port 1"
        },
        {
          "Protocol": "USB 2.0",
          "Segment": "USB Hub Port 2"
        }
      ]
      "Name": "USB Hub",
      "Type": "USX2422"
    }
    {
      "Interfaces": [
        {
          "ContactGroup": "I2C1",
          "Protocol": "I2C",
          "Segment": "I2C12",
        },
        {
          "ContactGroup": "USB1",
          "Protocol": "USB 2.0",
          "Segment": "USB Hub Port 2"
        }
      ]
      "Name": "BMC Port"
      "PortType": "connecting"
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
      "Interfaces": [
        {
          "ContactGroup": "I2C1",
          "Protocol": "I2C",
          "Segment": "I2C3"
        },
        {
          "ContactGroup": "USB1",
          "Protocol": "USB 2.0",
          "Segment": "USB1"
        }
      ]
      "Name": "BMC Port"
      "PortType": "connected_by"
      "Type": "Port"
    },
    {
      "Address": "0x50",
      "Segment": "I2C1",
      "Name": "Board FRU",
      "Type": "EEPROM"
    },
    {
      "Segment": "USB1"
      "Configuration": 0,
      "Interface": 1,
      "Name": "MCTP Device"
      "Type": "MCTPUSBDevice"
    }
  ],
  "Name": "Peripheral Board",
  "Probe": "...",
  "Type": "Board"
}
```
