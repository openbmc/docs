# Correlating inventory from multiple producers

Author: Ender Hsieh (EnderH)

Other contributors:

Created: October 30, 2026

## Problem Description

A device is often managed by more than one service, each over its own protocol,
and each service creates its own inventory items for it. A GPU, for example, can
have items from a daemon using vendor defined messages over MCTP, from an SPDM
service and from pldmd. Nothing on D-Bus says that these items belong to the
same device, so a client such as bmcweb presents them as unrelated resources.
This design lets the services agree on the device through Entity Manager (EM):
the board configuration describes the device once, with one entry per service
that all share a `Name`, and each service links the items it creates to the
resulting configuration object. The goal is a way to tell which items belong to
one device that works for any service creating inventory from EM configuration,
not only services that talk MCTP. How bmcweb presents the correlated items is a
non-goal of this design; it is left to bmcweb experts, and this document only
gives an example of what it could look like. Correlating devices that have no EM
configuration is also a non-goal.

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

## Requirements

Services that create inventory for the same device must not have to agree on
object paths or item names. Each service keeps creating its items as it does
today, and adds one association that names the device.

A client must be able to find every item of a device from any one of them, and
must be able to choose one stable name for the device that does not depend on
how a service names its items.

The design must work for any service that creates inventory from EM
configuration, whatever protocol it uses to talk to the device.

The configuration stays optional for a service, but a device whose inventory is
created by more than one service has to be described in the EM configuration to
be correlated. A device without an EM entry, such as a PLDM terminus discovered
without configuration, keeps today's behavior and its items are not correlated.

## Proposed Design

### Describing the device

The board configuration describes the device once, with one entry for each
service that manages it, and gives all of them the same `Name`. For a GPU
managed by a vendor defined message daemon, an SPDM service and pldmd:

```json
{
  "Exposes": [
    { "Name": "GPU_0", "Type": "NvidiaMctpVdmGpu" },
    { "Name": "GPU_0", "Type": "<SPDM type>" },
    { "Name": "GPU_0", "Type": "PLDMTerminus" }
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
                        | Configuration.NvidiaMctpVdmGpu |
                        | Configuration.<SPDM type>      |
                        | Configuration.PLDMTerminus     |
                        +--------------------------------+
                           ^             ^             ^
                           |       configured_by       |
                           |             |             |
+--------------------------+--+ +--------+---------+ +-+-------------------+
| item created by the vendor  | | item created by  | | item created by     |
| defined message daemon      | | the SPDM service | | pldmd               |
+-----------------------------+ +------------------+ +---------------------+
```

Two items are of the same device when their `configured_by` association points
to the same object. From any one item, a client follows `configured_by` to the
object and then `configures` back to every item of the device. The object mapper
already provides both directions, so it needs no change.

### Presenting the device

This section is an example to show that the design gives a client what it needs;
how bmcweb implements it, and how each Redfish resource handles correlated
items, is left to bmcweb experts.

The Redfish model has one resource per physical entity, so the three items above
would appear as one `/redfish/v1/Systems/system/Processors/GPU_0` rather than
three processors. Its ID can be the name of the configuration object, the last
segment of its path, which does not depend on how each service names its items.
An item without `configured_by` keeps the ID of its own path. Data specific to
one protocol stays on the resource the schema defines for it, linked to the
processor, such as a `ComponentIntegrity` resource whose `TargetComponentURI`
points to the processor for SPDM.

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
