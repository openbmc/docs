# Correlating the inventory items of one device

Author: Ender Hsieh (EnderH)

Other contributors:

Created: October 30, 2026

## Problem Description

In this document, an inventory item is a D-Bus object that implements
`xyz.openbmc_project.Inventory.Item`, usually together with a more specific
interface such as `Inventory.Item.Accelerator`. A device can have several,
because it is often managed by more than one service, each over its own
protocol, and each service creates its own inventory item for it. A GPU with an
integrated root of trust, for example, can be a PLDM type 2 and type 5
responder, an SPDM responder, and be managed through vendor defined messages
over MCTP, so pldmd, an SPDM service and the vendor defined message daemon each
publish an inventory item for it. These items are different views of one device,
not copies of one record, and this design does not merge them.

Nothing on D-Bus says that these items belong to the same device, so items that
implement the same interface look like separate devices. The items from pldmd
and the vendor defined message daemon both implement
`Inventory.Item.Accelerator`, and a client such as bmcweb presents them as two
processors. The problem is not specific to MCTP. A CPU board configuration in
EM, such as the [Anacapa CPU][anacapa-cpu], publishes an item that implements
`Inventory.Item.Cpu`, and on a system that also runs smbios-mdr, smbios-mdr
publishes another item with the same interface for the same CPU from the SMBIOS
tables of the host.

This design lets the services agree on the device through Entity Manager (EM):
the board configuration describes the device once, with one entry per service
that all share a `Name`, and each service links the inventory items it creates
to the resulting configuration object. The goal is a way to tell which inventory
items belong to one device that works for any service creating them from EM
configuration, whatever protocol it uses to talk to the device. How bmcweb
presents the correlated items is a non-goal of this design; it is left to bmcweb
experts, and this document only gives an example of what it could look like.
Correlating devices that have no EM configuration is also a non-goal.

## Background and References

EM publishes each `Exposes` entry of a board configuration as a D-Bus object
named after the entry's `Name`, under the path of the board, with an interface
for the entry's `Type`. Entries of different types that share a `Name` are
therefore published on one object. [phosphor-dbus-interfaces][pdi-object] adds
`xyz.openbmc_project.Configuration.Object`, which [EM publishes][em-object] on
every such object as one interface to associate with whatever the entry's type,
and a `configured_by` association from `xyz.openbmc_project.Inventory.Item` to
it. The [MCTP device discovery design][mctp-discovery] uses these for services
that find their devices through MCTP, and lets each such service watch for a
configuration type of its own, such as the [NVIDIA vendor defined message
types][mctpvdm-types] or an [SPDM type][spdm-mctp]. pldmd has no configuration
type yet. A [`PLDMTerminus` type][pldm-terminus] is proposed as a starting
point, and the PLDM types are best defined by PLDM experts.

The object mapper keeps the reverse direction of every association, so the items
that are `configured_by` an object are listed by the object's `configures`
association. bmcweb builds the ID of a Redfish resource from the last segment of
the D-Bus path of the item it is built from.

This document uses the following terms:

- device: one physical part, such as a GPU.
- configuration entry: an `Exposes` entry of a board configuration, and the
  object EM publishes for it. The entries of one device share a `Name`, so they
  are published on one object, which is the single source of the device's
  identity.
- inventory item: as defined above, an object a service creates for the device.
  A device can have several, from one or more services.

## Requirements

Services that create inventory items for the same device must not have to agree
on object paths or item names. Each service keeps creating its items as it does
today, and adds one association that names the device.

A client must be able to find every item of a device from any one of them, and
must be able to choose one stable name for the device that does not depend on
how a service names its items.

The design must work for any service that creates inventory items from EM
configuration, whatever protocol it uses to talk to the device.

The configuration stays optional for a service, but a device whose inventory
items are created by more than one service has to be described in the EM
configuration to be correlated. A device without an EM entry, such as a PLDM
terminus discovered without configuration, keeps today's behavior and its items
are not correlated.

## Proposed Design

### Describing the device

The board configuration describes the device once, with one entry for each
service that manages it, and gives all of them the same `Name`. For a GPU
managed by pldmd, an SPDM service and a vendor defined message daemon:

```json
{
  "Exposes": [
    { "Name": "GPU_0", "Type": "PLDMTerminus" },
    { "Name": "GPU_0", "Type": "<SPDM type>" },
    { "Name": "GPU_0", "Type": "NvidiaMctpVdmGpu" }
  ],
  "Name": "GPU Board 0"
}
```

`PLDMTerminus` stands for the configuration type pldmd would need, which does
not exist yet.

EM publishes the three entries on one object, which carries
`Configuration.Object` and one interface per type. Each service watches for its
own type, and the settings it needs for the device are properties of its own
entry, so a service does not read the entries of another.

### Linking the items

Each service links every item it creates for the device to the configuration
object with `configured_by`, and to the board with `contained_by`. The items
keep the paths the services give them:

```text
                        +--------------------------------+
                        | board/GPU_Board_0/GPU_0        |
                        | Configuration.Object           |
                        | Configuration.PLDMTerminus     |
                        | Configuration.<SPDM type>      |
                        | Configuration.NvidiaMctpVdmGpu |
                        +--------------------------------+
                           ^             ^             ^
                           |       configured_by       |
                           |             |             |
+--------------------------+--+ +--------+---------+ +-+-------------------+
| item created by pldmd       | | item created by  | | item created by the |
|                             | | the SPDM service | | vendor defined      |
|                             | |                  | | message daemon      |
+-----------------------------+ +------------------+ +---------------------+
```

Two items are of the same device when their `configured_by` association points
to the same object. From any one item, a client follows `configured_by` to the
object and then `configures` back to every item of the device. The object mapper
already provides both directions, so it needs no change.

### Example

For the GPU above, the objects on D-Bus are:

```text
/xyz/openbmc_project/inventory/system/board/GPU_Board_0/GPU_0     (EM)
    Configuration.Object, Configuration.PLDMTerminus,
    Configuration.<SPDM type>, Configuration.NvidiaMctpVdmGpu
    configures -> the three inventory items below

<path chosen by pldmd>                                           (pldmd)
    Inventory.Item.Accelerator          configured_by -> GPU_0
<path chosen by the SPDM service>                                (SPDM)
    Inventory.Item.TrustedComponent     configured_by -> GPU_0
/xyz/openbmc_project/inventory/GPU_0                             (VDM)
    Inventory.Item.Accelerator          configured_by -> GPU_0
```

The items from pldmd and the vendor defined message daemon implement the same
interface, but their `configured_by` associations point to the same object, so a
client can tell that they are one GPU rather than two. The item from the SPDM
service implements a different interface, and the same association is how a
client finds the GPU it belongs to. Each service chooses its own path, and a
client that starts from the configuration object does not need to know any of
them.

### Presenting the device

This section is an example to show that the design gives a client what it needs;
how bmcweb implements it, and how each Redfish resource handles correlated
items, is left to bmcweb experts.

The Redfish model has one resource per physical entity, so the two
`Inventory.Item.Accelerator` items above would appear as one
`/redfish/v1/Systems/system/Processors/GPU_0` rather than two processors. Its ID
can be the name of the configuration object, the last segment of its path, which
does not depend on how each service names its items. An item without
`configured_by` keeps the ID of its own path. Data specific to one protocol
stays on the resource the schema defines for it, linked to the processor, such
as a `ComponentIntegrity` resource whose `TargetComponentURI` points to the
processor for SPDM.

Since EM names the object after the entry within its board, two boards can have
entries with the same `Name`. A configuration that wants unique IDs across the
system has to give the entries unique names, for example with `$index`, though
`$index` follows the order of discovery rather than the position of the board.

## Alternatives Considered

One entry could carry several types, so that the device is one entry rather than
several entries that share a `Name`. EM validates each entry against the schema
of its type and publishes one interface per entry, so this changes how both
work, and the settings of different services would mix in one entry. Sharing a
`Name` works with the schemas and the behavior of EM today.

The services could agree to name their items after the `Name` of the entry, so
that the items share the last segment of their paths. This cannot be enforced,
and items with the same last segment still have different paths, so a client
still needs an association to know that they are one device.

The items could be matched by an identifier the device reports, such as a UUID
or a serial number. Not every protocol reports the same identifier, and a client
would have to read the identifier from each item before it can group them. It
also tells identical devices apart, but not which position in the system each
one fills.

## Impacts

API impact: EM gains a configuration type for pldmd, such as the proposed
`PLDMTerminus`. Beyond that, the design uses the `Configuration.Object`
interface and the `configured_by` association of `Inventory.Item`, which
phosphor-dbus-interfaces already defines. Services add the association to the
items they create.

Performance impact: one association per item.

Security impact: None.

Documentation impact: the EM documentation describes giving the entries of one
device the same `Name`.

Developer impact: a board configuration describes a device managed by several
services once, with one entry per service sharing a `Name`. A service that
adopts the design links its items with `configured_by`.

Upgradability impact: None. Items without the association are presented as
before.

### Organizational

- Does this proposal require a new repository? No
- Who will be the initial maintainer(s) of this repository? N/A
- Which repositories are expected to be modified to execute this design?
  - entity-manager
  - services that create inventory, such as dbus-sensors, pldm and spdm
  - bmcweb

## Testing

On a system with a device described by entries of several types that share a
`Name`, check that EM publishes one object carrying all the types, that every
item the services create for the device is `configured_by` that object, and that
the `configures` association of the object lists all of them. Unit tests in
entity-manager cover entries that share a `Name` being published on one object.

[pdi-object]:
  https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/96104
[em-object]: https://gerrit.openbmc.org/c/openbmc/entity-manager/+/96105
[mctp-discovery]: https://gerrit.openbmc.org/c/openbmc/docs/+/96378
[mctpvdm-types]: https://gerrit.openbmc.org/c/openbmc/entity-manager/+/93517
[spdm-mctp]: https://gerrit.openbmc.org/c/openbmc/entity-manager/+/83263
[pldm-terminus]: https://gerrit.openbmc.org/c/openbmc/entity-manager/+/96379
[anacapa-cpu]:
  https://github.com/openbmc/entity-manager/blob/master/configurations/meta/anacapa/anacapa_cpu.json
