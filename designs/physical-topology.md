# Physical Topology for Inventory Items

Author: Benjamin Fair <benjaminfair>

Other contributors:
  < Name and/or Discord nickname or `None` >

Created: June 1, 2022

## Problem Description
Complex systems may contain many inventory objects (such as chassis, power
supplies, cables, fans, etc.) with different types of relationships among these
objects. For instance, one chassis can contain another, be powered by a set of
power supplies, connect to cables, and be cooled by fans. OpenBMC does not
currently have a standard way to represent these types of relationships.

## Background and References
This builds on a [prior
proposal](https://gerrit.openbmc.org/c/openbmc/docs/+/41468), but specifies
using Associations for all relationships (Proposal II) rather than path
hierarchies (Proposal I).

The main driver of this design is Redfish, particularly the Links section of the
[Chassis schema](https://redfish.dmtf.org/schemas/Chassis.v1_20_0.json).

CLs to phosphor-dbus-interfaces documenting new Associations have been
[proposed](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/46806)
but not yet merged until consensus can be reached on the desing.

This design was initially discussed in
[Discord](https://discord.com/channels/775381525260664832/819741065531359263/964321666790477924),
where some initial consensus was reached.

## Requirements
* Must represent one-to-many relationships from chassis inventory objects which:
    * Connect to cables
    * Contain other chassis and/or are contained by a chassis
    * Contain storage drives
    * Are cooled by fans
    * Are powered by power supplies
    * Contain processors such as CPUs
    * Contain memory such as DIMMs
* Must support relationships which are predefined, detected at runtime, and a
  combination of both
    * Runtime detection could include I2C bus scanning, USB enumeration, and/or
      MCTP discovery

## Proposed Design
The design affects three layers of the OpenBMC architecture:
phosphor-dbus-interfaces, inventory managers, and inventory consumers such as
bmcweb.

### phosphor-dbus-interfaces
In the interface definition for Chassis inventory items, we add an association
definition for each of the relationship types listed above and corresponding
assocation definitions for the other item types linking back to a Chassis item.

### Inventory Managers
phosphor-inventory-manager already has support for exporting custom
Associations, so no changes are needed here.

For entity-manager, we add a new top level object to the JSON schemas for
inventory items. This contains two entries: `Ports` and `Connections`. `Ports`
is an array of strings which give names for all the connection points on the
current item, such as PCIe slots, power inputs, or custom connectors.
`Connections` uses `Probe`-style D-Bus matching statements to refer to other
inventory items based on `Ports` they have available and specifies what forward
and reverse association names to export regarding the located item and the
current one.

### Inventory consumers
When a daemon such as bmcweb wants to determine what other inventory items have
a relationship to a specific item, it makes a query to the object mapper which
returns a list of all associated items and the relationship types between them.

## Alternatives Considered
### Path hierarchies

(2 paragraphs) Include alternate design ideas here which you are leaning away
from. Elaborate on why a design was considered and why the idea was rejected.
Show that you did an extensive survey about the state of the art. Compares
your proposal's features & limitations to existing or similar solutions.

## Impacts
API impact? Security impact? Documentation impact? Performance impact?
Developer impact? Upgradability impact?

### Organizational
Does this repository require a new repository?  (Yes, No)
Who will be the initial maintainer(s) of this repository?

## Testing
How will this be tested? How will this feature impact CI testing?
