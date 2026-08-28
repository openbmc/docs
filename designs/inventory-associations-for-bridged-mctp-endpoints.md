# Inventory associations for bridged MCTP endpoints

Author: Andrew Jeffery <andrew@codeconstruct.com.au> | @arj

Created: 28th August 2026

## Problem Description

Existing platform designs have non-trivial MCTP topologies. Some MCTP endpoints
are bridged, and thus are not immediate neighbours in the network.

Currently, OpenBMC's support for associating MCTP endpoints exposed on D-Bus
by `mctpd` with configuration and inventory information exposed on D-Bus by
Entity Manager only extends to the trivial case of immediate neighbours in an
MCTP network. A context gap exists for all bridged endpoints in any platform
design with a non-trivial MCTP topology.

## Background and References

- [DMTF PMCI DSP0236 - Management Component Transport Protocol (MCTP) Base Specification](https://www.dmtf.org/dsp/dsp0236)
- [OpenBMC in-kernel MCTP](designs/mctp/mctp-kernel.md)
- [mctpd](https://github.com/CodeConstruct/mctp/blob/31ac0e86b0e23d32abf34d191653a87a47f3c9a7/docs/mctpd.md)
- [Entity Manager](https://github.com/openbmc/entity-manager/blob/f9ab4384184c74ab05de5a2a05ac5ca5a55b64dd/README.md)
- [mctpreactor](https://gerrit.openbmc.org/c/openbmc/dbus-sensors/+/69111)
- [Physical Topology for Inventory Items](/designs/physical-topology.md)

## Requirements

An association must exist for each MCTP endpoint whose device is described by
configuration in Entity Manager, regardless of its location in the MCTP network.

## Considerations

### Device Location Identity

Complex platform configurations may contain multiple instances of a device or
board.

In general, the location identity of a device or board in the system cannot be
uniquely described based on the device's type alone.

A stable identity for the location of a device in the platform configuration
is its path under some view of the platform configuration's topology. There
are a number of views in a platforms topology: Containment, power, cooling, and
various forms of connectivity for communication.

Device connectivity may be complex. Communication to a device in the platform
may span multiple physical transports.

Establishing a path over these physical transports provides a location identity
for the device in terms of some topology of platform connectivity.

The problem at hand regards bridged MCTP endpoints. This constrains our
considerations to the configuration topology that MCTP is capable of describing.

### MCTP

DSP0236 delegates to bridges the responsibility of assigning endpoint IDs to
their neighbours for which they are bus owners, and where those neighbours are
themselves bridges, allocation of an EID pool to each bridge to be used for
its own downstream assignments and related pool allocations.

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

Graph-traversal of bridge responses to DSP0236's mandatory `Get Routing Table
Entries` commands can be used to determine the entire MCTP network topology in
terms of bridge ports, their physical transports, and endpoints routable via a
given port.

Restated for the problem at hand, information gathered by graph-traversal of
`Get Routing Table Entries` responses allows us to construct a location identity
for each reachable endpoint in the MCTP network in terms of its path in the
topology of physical transports and bridges.

### Entity Manager

Entity Manager binds board configurations to discovered devices by matching a
configuration's "Probe" statement against a discovered device's FRU metadata.
FRU metadata of discovered devices is often hosted on D-Bus by `fru-device`.
`fru-device` periodically scans I2C buses exposed either by the BMC SoC, or
those that represent the downstream segments of an I2C mux device, for FRU
EEPROMs.

Entity Manager publishes D-Bus objects hosting inventory information for
discovered boards, as well as a collection of device configuration objects
described in the matching board configuration's "Exposes" property

Entity Manager may interpolate values derived from the FRU metadata, or
metadata determined in the process of discovering the FRU metadata, into the
configuration for a board upon binding a configuration to the discovered FRU
device. The data available for interpolation includes the FRU EERPOM bus address
and the bus index that hosts the FRU EEPROM device. Together these are often
used to derive the addresses of other communication devices described in
the configuration, usually by way of small arithmetic expressions embedded
in configuration value strings. Interpolated values such as bus indexes are
global values derived from the system hardware composition and the order of
its enumeration.

Restated, the location identity of communicating components in an Entity Manager
configuration currently only extends to the bus on which the FRU EEPROM is
discovered, usually an I2C bus. Further, the connectivity path is implicitly
described by a possibly-unstable global bus number, but which captures mux
relationships necessary for communicating with the FRU EEPROM.

### Synthesis of considerations

Creating an assocation between `mctpd`'s endpoint D-Bus objects and the
corresponding configuration D-Bus objects hosted by Entity Manager requires
establishing the equivalence of device location identities. Using paths in
the platform configuration topology for the purpose of location identity,
this transforms to equating paths in the device topology. The class of paths
understood by both `mctpd` and Entity Manager must necessarily be equivalent.

MCTP is not constrained to I2C as a physical transport.

Therefore, the support currently provided by Entity Manager for describing the
connectivity topology is insufficient to solve the problem at hand.

## Further observations

### Observations regarding `mctpd`

`mctpd` does not currently host physical topology information on D-Bus.

However, the means to do so is under consideration.

### Observations regarding Entity Manager

Despite statments above, Entity Manager does have some support for describing
topology beyond I2C. It has a schema for "Port"s, and the ability to bind two
boards by a D-Bus association, based on correlated port names and "PortType"s.

The current schema for ports describes the following relationships:

- `containing`
- `powering`
- `probing`

While these relationships describe some platform configuration topologies, they
are not the topologies necessary to solve the problem at hand.

Returning to the fundamentals of Entity Manager, it remains the case that FRU
discovery is necessary, along with binding a configuration to a discovered
device. The act of binding a configuration to a device establishes the existence
of the features described in the configuration's "Exposes" statement, and their
relationship to the device's FRU EEPROM.

For the problem at hand it's necessary that the devices and physical transports
supported by MCTP are among the features described in the probed device's
configuration, along with their relationship to the bus on which the FRU EEPROM
was discovered.

Describing devices on boards that exist on buses that aren't the bus of the
board's FRU EEPROM device requires describing the buses of those devices.

Relating the buses of those devices to the bus of the FRU EEPROM requires
describing connectors.

## Proposed Design

Expand Entity Manager's `Port` schema's `PortType` to include `connecting` and
`connected_by` relationships, such that the following example configuration
is accepted:


```json
{
  "Exposes": [
    {
      "Interfaces": [
        {
          "BusType": "I2C",
          "BusSegment": "I2C12",
          "Bus": "$bus"
        }
        {
          "BusType": "USB",
          "BusSegment": "USB1",
        }
      ],
      "Name": "AST2600",
      "Type": "SoC"
    },
    {
      "Interfaces": [
        {
          "BusType": "USB",
          "BusSegment": "USB Hub Port 1"
        },
        {
          "BusType": "USB",
          "BusSegment": "USB Hub Port 2"
        }
      ]
      "Name": "USB Hub",
      "Type": "USX2422"
    }
    {
      "Interfaces": [
        {
          "ContactGroup": "I2C1",
          "BusType": "I2C",
          "BusSegment": "I2C12",
        },
        {
          "ContactGroup": "USB1",
          "BusType": "USB",
          "BusSegment": "Usb Hub Port 2"
        }
      ]
      "Name": "BMCPort"
      "PortType": "connecting"
      "Type": "Port"
    }
  ],
  "Name": "...",
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
          "BusType": "I2C",
          "BusSegment": "I2C3"
        },
        {
          "ContactGroup": "USB1",
          "BusType": "USB",
          "BusSegment": "USB1"
        }
      ]
      "Name": "BMCPort"
      "PortType": "connected_by"
      "Type": "Port"
    },
    {
      "Address": "0x50",
      "BusSegment": "I2C1",
      "Name": "Board FRU",
      "Type": "EEPROM"
    },
    {
      "BusSegment": "USB1"
      "Configuration": 0,
      "Interface": 1,
      "Name": "Some MCTP Device"
      "Type": "MCTPUSBDevice"
    }
  ],
  "Name": "...",
  "Probe": "...",
  "Type": "Board"
}
```

### Discussion

Under this proposal, board descriptions may become entirely self-contained
except at the connectors. The act of binding a device to a configuration
propagates information from the `connecting` board to the `connected_by` board
by the relationships described under the `Interfaces` property for a `Port`.

A discovered board has a strong relationship to an existing board as a
consequence of the bus on which the FRU EEPROM was discovered being a function
of the connectors it traverses. Entity Manager knows the precise existing board
into which the newly discovered board plugs, and the precise relationships of
other buses traversing the boards' ports.

The usual interpolation of `$bus` and `$address` into other expressions may be
unnecessary at the scope of configuration files, as a bus' `BusSegment` name
is defined local to the board configuration, with the global bus number derived
over the connecting ports at the time of binding.

A `BusSegment` consumed by a `Port` of `connecting` `PortType` is instantiated
either by a component on the board, or by an upstream `Port` of `connected_by`
`PortType` on the board. All `BusSegment` names can be resolved locally,
internal to elements of the board's `Exposes` statement.

Connector definitions on each connecting board must be equivalent for the
purpose of relation by `ContactGroup`s. It may be helpful to extract their
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

> API impact? Security impact? Documentation impact? Performance impact?
> Developer impact? Upgradability impact?

### Organizational

> - Does this proposal require a new repository? (Yes, No)
> - Who will be the initial maintainer(s) of this repository?
> - Which repositories are expected to be modified to execute this design?
> - Make a list, and add listed repository maintainers to the gerrit review.

## Testing

> How will this be tested? How will this feature impact CI testing?
