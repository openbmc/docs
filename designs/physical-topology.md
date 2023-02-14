# Physical Topology for Inventory Items

Author: Benjamin Fair <benjaminfair>

Other contributors: Ed Tanous <edtanous>

Created: June 1, 2022

## Problem Description

Complex systems may contain many inventory objects (such as chassis, power
supplies, cables, fans, etc.) with different types of relationships among these
objects. For instance, one chassis can contain another, be powered by a set of
power supplies, connect to cables, and be cooled by fans. OpenBMC does not
currently have a standard way to represent these types of relationships.

## Background and References

This builds on a
[prior proposal](https://gerrit.openbmc.org/c/openbmc/docs/+/41468), but
specifies using Associations for all relationships (Proposal II) rather than
path hierarchies (Proposal I).

The main driver of this design is Redfish, particularly the Links section of the
[Chassis schema](https://redfish.dmtf.org/schemas/Chassis.v1_20_0.json).

Changes to phosphor-dbus-interfaces documenting new Associations have been
[proposed](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/46806)
but not yet merged until consensus can be reached on the design.

This design was initially discussed in
[Discord](https://discord.com/channels/775381525260664832/819741065531359263/964321666790477924),
where some initial consensus was reached.

## Requirements

- Must represent one-to-many relationships from chassis inventory objects which:
  - Connect to cables
  - Contain other chassis and/or are contained by a chassis
  - Contain storage drives
  - Are cooled by fans
  - Are powered by power supplies
  - Contain processors such as CPUs
  - Contain memory such as DIMMs
- Must support relationships which are predefined, detected at runtime, or a
  combination of both
  - Runtime detection could include I2C bus scanning, USB enumeration, and/or
    MCTP discovery

### Optional goals (beyond initial implementation)

- Non-chassis inventory objects may also need one-to-many relationships
  - CPUs have CPU cores and associated PCIe slots
  - CPU cores have threads

## Proposed Design

The design affects three layers of the OpenBMC architecture:
phosphor-dbus-interfaces, inventory managers, and inventory consumers such as
bmcweb.

### phosphor-dbus-interfaces

In the interface definition for Inventory/Item or the more specific subtypes, we
add an association definition for each of the relationship types listed above
and corresponding association definitions linking back to that item.

### Inventory Managers

#### phosphor-inventory-manager

phosphor-inventory-manager already has support for exporting custom
Associations, so no changes are needed here.

#### entity-manager

For entity-manager, we add new `Exposes` stanzas for the upstream and downstream
ports in the JSON configurations. The upstream port has a type ending in `Port`
(such as a `BackplanePort`, `PowerInputPort`, etc). The downstream port has type
`DownstreamPort` and a `ConnectsToType` property that refers to the upstream
port based on its type.

New code in entity-manager matches these properties and exposes associations on
D-Bus based on the types of the inventory objects involved. Two Chassis objects
will have `containing` and `contained_by`, a Chassis and PowerSupply will have
`powering` and `powered_by` respectively, etc.

Example JSON configurations:

superchassis.json

```
{
    "Exposes": [
        {
            "Name": "MyPort",
            "Type": "BackplanePort"
        }
    ],
    "Name": "Superchassis",
    "Probe": "TRUE",
    "Type": "Chassis"
}
```

subchassis.json:

```
{
    "Exposes": [
        {
            "ConnectsToType": "BackplanePort",
            "Name": "MyDownstreamPort",
            "Type": "DownstreamPort"
        }
    ],
    "Name": "Subchassis",
    "Probe": "TRUE",
    "Type": "Chassis"
}
```

#### Other inventory managers

If there are other daemons on the system exporting inventory objects, they can
choose to include the same Associations that phosphor-inventory-manager and
entity-manager use.

### Inventory consumers

When a daemon such as bmcweb wants to determine what other inventory items have
a relationship to a specific item, it makes a query to the object mapper which
returns a list of all associated items and the relationship types between them.

Example `busctl` calls:

```
$ busctl get-property xyz.openbmc_project.ObjectMapper \
/xyz/openbmc_project/inventory/system/chassis/Superchassis/containing \
xyz.openbmc_project.Association endpoints

as 1 "/xyz/openbmc_project/inventory/system/chassis/Subchassis"

$ busctl get-property xyz.openbmc_project.ObjectMapper \
/xyz/openbmc_project/inventory/system/chassis/Subchassis/contained_by \
xyz.openbmc_project.Association endpoints

as 1 "/xyz/openbmc_project/inventory/system/chassis/Superchassis"
```

## Alternatives Considered

### Path hierarchies

An alternative proposal involves encoding the topological relationships between
inventory items using D-Bus path names. As an example, a chassis object
underneath another would be contained by that parent chassis. This works for
simple relationships which fit into a tree structure, but breaks down when more
complicated relationships are introduced such as cables or fans and power
supplies shared by multiple objects or soldered CPUs which are "part of" instead
of "contained by" a chassis. Introducing separate trays with their own topology
further complicates the path hierarchy approach.

A potential compromise would be allowing a combination of path hierarchies and
associations to communicate topology, but this significantly increases the
complexity of consumers of this information since they would have to support
both approaches and figure out a way to resolve conflicting information.

Associations are the only approach that fits all use cases, so we should start
with this method. If the additional complexity of path hierarchies is needed in
the future, it can be added as a separate design in the future.

To improve usability for humans inspecting a system, there could also be a
dedicated tool to query for Associations of a specific type and present a
hierarchical view of the current topology. Additionally,
phosphor-inventory-manager configurations can organize their D-Bus objects in
whatever way makes sense to the author of those configurations, but the
Association properties would still need to be present in order for inventory
consumers to understand the topology.

## Impacts

This new API will be documented in phosphor-dbus-interfaces as described above.
If no topology information is added to configuration files for entity-manager or
phosphor-inventory-manager, then the D-Bus interfaces exported by them will not
change. If consumers of inventory data such as bmcweb do not find the new
associations, then their output such as Redfish will not change either.

### Organizational

Does this repository require a new repository? No - all changes will go in
existing repositories.

## Testing

All new code in entity-manager and bmcweb will be unit tested using existing
frameworks and infrastructure. We will add new end-to-end tests in
openbmc-test-automation to ensure the Redfish output is correct.

## Update history

### February 14, 2023

* Update the example port type and name from `Connector` to `Port`
* Change the association names based on feedback from phosphor-dbus-interfaces
  maintainers (chassisContains -> containing, chassisContainedBy -> contained\_by, etc)
