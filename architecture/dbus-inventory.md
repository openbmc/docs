# Representing Inventory in a D-Bus Model

This document attempts to outline certain rules with respect to how the OpenBMC
firmware stack should represent inventory items in its D-Bus model. These rules
are formulated so that OpenBMC's D-Bus model can easily and reliably be
translated to a set of Redfish schemas.

More information on the Redfish specification and schemas can be found here:
[Redfish Specification](http://redfish.dmtf.org/schemas/DSP0266_1.6.1.html).

## Representing Relations Between Inventory Items

An inventory item is typically a physical entity within a system that the BMC
manages. The OpenBMC firmware represents such entities as D-Bus objects. The
D-Bus objects implement one or more Inventory interfaces to identify the type of
entity(-ies) they represent. The inventory D-Bus interfaces can be found in:
[Phosphor D-Bus Interfaces](https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/xyz/openbmc_project/Inventory/Item)

On every system, inventory items are related to one another in one or more ways.
Examples of such relations include, relation that denotes physical containment;
a logical relation such as an LED that is physically located on the motherboard,
but is actually used to indicate fault in another entity.

Further examples used to illustrate this point are:
* A PCIe card that plugs into a slot located on the motherboard of a given
  chassis. In this case, the PCIe device/slot can be considered to be contained
  within the chassis. Another way to put this could be that the slot is a child
  to the parent chassis. In Redfish, this would be represented as a PCIeDevices
  link in the chassis schema.
* A power supply that is located either inside or outside of the main chassis.
  In Redfish, this would be represented as PoweredBy link in the chassis schema.

D-Bus objects can be associated with one another using the [Associations
Interface](https://github.com/openbmc/docs/blob/master/architecture/object-mapper.md#associations)

On systems running OpenBMC, inventory items are hosted as D-Bus objects by one
or more of the following programs as of this writing:

[Phosphor Inventory Manager(PIM)](https://github.com/openbmc/phosphor-inventory-manager)
[Entity Manager(EM)](https://github.com/openbmc/entity-manager/)

This document proposes ways in which these relations can be represented in the
BMC's D-Bus model, and what choosing one over the other means for how inventory
objects are hosted on D-Bus.

The Redfish schema also has mechanisms to tie entities together in one or more
ways. This section describes some examples, taking the [Chassis Schema](https://redfish.dmtf.org/schemas/v1/Chassis.v1_15_0.json)
as a reference.

* ContainedBy - This is a containment relation that essentially refers to
  the parent chassis that contains the one in question.
* Contains - This again is a containment relation, all chassis contained within
  the one in question. There are other such relations for example Processors
  contained within the chassis (with the Processor schema having the reverse
  link)
* CooledBy - Links to a collection of Fans/Chassis used to cool the one in
  question. This may or may not indicate physical containment and must be
  treated as a logical association.
* ManagedBy - Links to the Managers that manage this chassis. The manager need
  not physically live in the chassis in question, so this is another logical
  relation.
* ManagersInChassis - This links to Managers that are *inside* the said chassis.
  Thus a physical containment.

The above list is not exhaustive, but it illustrates the point that certain
relationships denote containment (either contained by or contains), and certain
relationships are logical (even if the related entities are contained
within/by one another).

### Proposal I

This proposes that we use D-Bus object paths to represent containment type
relationships and use the Associations interface for anything else. For example:
an assembly with D-Bus object path `/system/chassis/motherboard/assembly0`
is contained within the chassis represented by `/system/chassis`.

In terms of Redfish schemas, any link that in its description has a *contained
in*/*contained by* would be a candidate to use path hierarchy for. The
[ObjectMapper](https://github.com/openbmc/docs/blob/master/architecture/object-mapper.md)
`GetSubTree` and `GetAncestors` methods can be used to discover the
contained/contained by entities.

This does mean, however, that the inventory management program needs to maintain
this nomenclature around D-Bus object paths.

This is something PIM can support because it does not actually determine the
paths of the D-Bus objects it hosts, instead relying on other programs to
discover and `Notify` it to host these objects.

For EM, this means we need to invent a way to create D-Bus objects with paths
that represent hierarchy - something it does not do today. There would also be
complexity around entities getting dynamically added/removed which would
necessitate the recreation of the entire tree of D-Bus objects underneath.


### Proposal II

This proposes that we use the Associations interface for denoting all and any
kind of relation between D-Bus objects. This approach has the advantage that a
client code can just always lookup associations.

With this approach, PIM would have to use entries in its associations JSON to
create relations between objects.

For EM, we would still need a way to represent inventory-inventory item
associations in its JSON configuration files.


### Proposal III

We let implementations decide which of the above two methods to use, and code
such as the Redfish server on the BMC can determine which one to use based
on either of the following:

* A compile time switch.
* At runtime, use associations if you find them, else fall back to using path
  hierarchy.

### Required Associations:

1. We should have an association in powersupply to indicate which chassis it powers.

  Example:

  "path": "system/chassis/motherboard/powersupply0",
  "endpoints":
  [
     {
          "types":
          {
              "rType": "inventory",
              "fType": "chassis"
          },
          "paths":
          [
              "/xyz/openbmc_project/inventory/system/chassis"
          ]
     }

  ]
