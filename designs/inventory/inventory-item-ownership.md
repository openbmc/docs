# One inventory item per device across services

Author: Ender Hsieh (EnderH)

Other contributors:

Created: October 30, 2026

## Problem Description

A device is often described by more than one service. A CPU, for example, is
described by smbios-mdr from the SMBIOS tables the host sends, and by pldmd from
the PDRs of a microcontroller that monitors it. Maintainers agreed that the
services should not each create an inventory item for the device. One service
creates it, and the others attach their data to it
([discussion][discord-thread]).

That leaves a gap. Clients such as bmcweb find a device by filtering on an item
interface such as `xyz.openbmc_project.Inventory.Item.Cpu`, and then read only
the object path they found. A service that does not create the item has no
defined place to put the rest of the device's data:

- Data on an object path of its own is never read, because nothing leads the
  client from the item to that path.
- Data on the item's path is read for some Redfish resources but not others. For
  a PCIe device, bmcweb reads only the service that implements
  `Inventory.Item.PCIeDevice`.
- Nothing tells the services which of them creates the item, or on which path,
  so each one has to guess whether another service will do it.

This design defines rules that keep one item of each kind per device, and still
let the data of every service reach clients.

## Background and References

In this document:

- device: one physical part, such as a CPU.
- item type: the `<Type>` of an `xyz.openbmc_project.Inventory.Item.<Type>`
  interface, such as `Cpu`, `Accelerator` or `PCIeDevice`. It decides which
  Redfish resource the device becomes.
- configuration entry: an `Exposes` entry of a board configuration in Entity
  Manager (EM), and the object EM publishes for it.
- configuration type: the `Type` of a configuration entry. It decides which
  service reads the entry.
- item path: the object path that carries the items of a device.

EM publishes each configuration entry as an object named after the entry's
`Name`, under the path of its board:

```text
/xyz/openbmc_project/inventory/system/<board type>/<board name>/<entry name>
```

Both names are sanitized for D-Bus, and entries that share a `Name` are
published on one object. phosphor-dbus-interfaces defines a `configured_by`
association from an inventory item to that object ([96104][pdi-object]). Its
endpoint must implement `xyz.openbmc_project.Configuration.Object`, which EM
publishes on every entry with [96105][em-object]. phosphor-dbus-interfaces also
defines a `contained_by` association between items for the [physical
topology][physical-topology].

The object mapper merges the interfaces that different services publish on one
object path, so a client sees one object. An association whose endpoint does not
exist yet is kept pending, and the mapper creates it when the endpoint appears.
It moves the association back to pending when the endpoint goes away.

bmcweb builds the ID of a Redfish resource from the last element of the item's
object path. The table shows what bmcweb does today when two services publish
objects for one device:

| Case                                                  | Resource for the item type                                                                                                                                          |
| ----------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Same path, both services have the item type           | One resource. Its values come from either service, and for some item types the service can change from one request to the next.                                     |
| Same path, one service has the item type              | One resource. There is no common logic: for some item types bmcweb merges the interfaces of every service on the path, for others it reads only the item's service. |
| Different paths with the same last element, both have | The collection lists the same URL twice, and a GET returns only one of them.                                                                                        |
| Different paths with the same last element, one has   | One resource. The other path is not read.                                                                                                                           |

No existing document says how services share one inventory item. The [object
mapper][object-mapper] allows several services on one object path, and the
[eStoraged design][estoraged] expects several daemons to manage one device on
one path, split by interface.

## Requirements

- For each item type, a device has at most one item, published by one service.
- Data from every service that describes the device reaches clients through that
  item, either on the same object path or by association.
- Services agree on the item path without talking to each other.
- Which service owns the item, and which service produces each item type, is a
  per-platform decision made in EM configuration, not in service code. Every
  service that takes part therefore has its own configuration type, with fields
  that mark the owner.
- Devices that EM does not know, such as entities found only in a PLDM PDR,
  still get an item without any configuration.
- Two devices never end up with the same last element of the item path, because
  bmcweb uses it as the Redfish resource ID.
- No new association types are needed. `contained_by` from the physical topology
  design and `configured_by` from phosphor-dbus-interfaces are enough.

## Proposed Design

### Choosing the owner

A board configuration describes a device with one entry per service, and all of
them share a `Name`. Two fields on the entries decide which service creates the
items:

- `InventoryItem: true` marks the service that publishes
  `xyz.openbmc_project.Inventory.Item`. Exactly one entry of a device sets it.
  This service sets `PrettyName`, which becomes the Redfish `Name`, and
  `Present`, which decides whether the resource is `Absent`.
- `InventoryTypes` lists the item types the service publishes, such as
  `["Cpu"]`. Each item type appears in at most one entry of a device. The item
  type always comes from this field, so it can be checked that two services do
  not claim the same type.

The fields are required even when only one service describes the device, so that
the owner is always written down rather than assumed. A service whose entry sets
neither field publishes no item interface. It can still publish other interfaces
on the item path, as described below.

### Choosing the item path

Every service that has an entry for the device puts its interfaces on one path,
built from the last two elements of the entry's path:

```text
entry path: /xyz/openbmc_project/inventory/system/board/Baseboard/CPU_0
item path:  /xyz/openbmc_project/inventory/Baseboard_CPU_0
```

The services take both elements from the path EM gives them, so they get the
same string without sanitizing names themselves. The board name keeps the IDs
unique when two boards have entries with the same `Name`. In theory, board `A_B`
with entry `C` and board `A` with entry `B_C` collide, which is unlikely enough
to leave alone.

Boards that EM creates from a FRU, such as
`/xyz/openbmc_project/inventory/system/board/Baseboard`, keep their path. EM
publishes their items itself, and no other service creates one for them.

The item path differs from the entry path, so the owner links the item to its
entry with `configured_by`, and to the board with `contained_by`.
`configured_by` only points to the entry that describes the device itself, never
to the entry of a microcontroller or a bus that leads to it.

### Sharing interfaces

Each interface on an item path has exactly one producer. Two services do not
publish the same interface, and they do not fill different properties of one
interface. A service publishes an interface only when it can tell, from the
device, its PDRs or its configuration, that the device supports it. Developers
design their services and configurations so that two services never produce the
same interface for one device. `xyz.openbmc_project.Association.Definitions` is
the exception. The object mapper keeps the associations of each service apart,
so every service can publish its own associations on the item path.

A service meant only as a backup for another could mark what it publishes with a
new `Inventory.Decorator.Backup` interface, so that clients know to prefer the
other service. That breaks the rule of one producer per interface, so it needs
further discussion, and the recommendation is for developers to avoid the case.

A service does not move its data to a path of its own to avoid this rule. A
client cannot tell that two objects are one device, as the table above shows.

Objects that belong to the device but are not the device, such as its ports,
live under the item path, for example `<item path>/Port_0`. Each one is created
by one service, and links to the item only with the association that
phosphor-dbus-interfaces defines for it, such as `connected_to` for
`Inventory.Connector.Port`. If none is defined, phosphor-dbus-interfaces is
updated first.

Sensors stay on their own paths. A sensor's `inventory` association points to
the item it measures, and its `chassis` association points to the board.
Associations name paths only, never services, so a service that does not own the
item still knows where to point.

### Devices without configuration

A device that has no entry, such as an entity found only in a PLDM PDR, gets its
item from the service that finds it, on a path the service chooses. The last
element of that path has to be unique. The service links the item to its parent
with `contained_by`.

For PLDM, EM knows only the terminus. pldmd walks the PDRs below it and creates
an item for each physical entity. The terminus entry can hand an entity to
another service instead, as in the example below. It also lists the entities
that EM already describes, such as the board that carries the terminus when the
PDRs include it, so that pldmd never creates a second item for a FRU that EM
owns. pldmd points the sensors of those entities at the existing item.

### Presenting the device

bmcweb reads every service on the item path and merges their interfaces into one
resource. Processor already does this. PCIeDevice reads only the service that
implements `Inventory.Item.PCIeDevice` and needs to change, and NetworkAdapter
follows the same rule when it starts reading properties.

The other cases in the table do not need handling. Two services with the same
item type on one path, and two items whose paths end in the same element, are
both ruled out above. Data on another path stays unread, as it is today.

### Example

A baseboard carries a CPU socket and a microcontroller that speaks PLDM. The
platform wants smbios-mdr to create the CPU, and pldmd to report the CPU
temperature from the microcontroller:

```json
{
  "Name": "Baseboard",
  "Exposes": [
    {
      "Name": "uC_0",
      "Type": "PLDMTerminus",
      "DeferredEntities": [
        { "EntityType": 135, "EntityInstance": 1, "Item": "CPU_0" }
      ]
    },
    {
      "Name": "CPU_0",
      "Type": "SmbiosCpu",
      "InventoryItem": true,
      "InventoryTypes": ["Cpu"]
    }
  ]
}
```

`PLDMTerminus` and `SmbiosCpu` only illustrate the idea. `PLDMTerminus` is a
work in progress in [96379][pldm-terminus], and its fields would be designed
there if this design is accepted. Entity type 135 is a processor in DSP0249.

EM publishes:

```text
/xyz/openbmc_project/inventory/system/board/Baseboard/uC_0
    Configuration.PLDMTerminus
/xyz/openbmc_project/inventory/system/board/Baseboard/CPU_0
    Configuration.SmbiosCpu
```

smbios-mdr reads `CPU_0`, sees both fields, and publishes the CPU:

```text
/xyz/openbmc_project/inventory/Baseboard_CPU_0
    Inventory.Item, Inventory.Item.Cpu, Inventory.Decorator.Asset
    configured_by -> .../board/Baseboard/CPU_0
    contained_by  -> .../board/Baseboard
```

pldmd reads `uC_0` and walks its PDRs. Processor instance 1 is handed to
`CPU_0`, so pldmd creates no item for it. It finds `CPU_0` on the same board as
`uC_0`, builds the item path the same way smbios-mdr did, and points the sensor
at it:

```text
/xyz/openbmc_project/sensors/temperature/CPU_0_Temp
    Sensor.Value
    inventory -> /xyz/openbmc_project/inventory/Baseboard_CPU_0
    chassis   -> .../board/Baseboard
```

If the host is off, smbios-mdr has nothing to publish yet. The mapper keeps the
`inventory` association pending, the sensor already shows under the baseboard's
chassis, and the processor appears once the host sends its SMBIOS tables. A
platform that wants the CPU visible while the host is off gives both fields to
pldmd instead.

## Alternatives Considered

Each service could create its own item and link it to the configuration entry
with `configured_by`, so that a client groups the items of one device. This was
the first version of this design. Maintainers preferred one item per device, and
a client would still have to merge the items itself.

The item could live on the entry path, so that `configured_by` is not needed.
The last element of the entry path is only the entry's `Name`, so two boards
with a `CPU_0` give two resources the same ID.

The item path could be `/xyz/openbmc_project/inventory/<Name>`. That has the
same collision, and each service would have to sanitize the name the way EM
does.

EM could probe `xyz.openbmc_project.Inventory.Source.PLDM.Entity` objects from
pldmd and create an entry for each entity it should know about. The interface
does not say which terminus an entity belongs to, so identical devices cannot be
told apart, and pldmd would depend on EM for an entry that depends on pldmd.

A service that does not own the item could wait to see whether another service
creates one. The owner may never appear, for example while the host is off, so
the result would depend on timing.

## Impacts

API impact: the configuration types of the services that take part gain
`InventoryItem` and `InventoryTypes`, and pldmd gains a configuration type for
the terminus. No D-Bus interface changes. The design depends on
[96105][em-object], so that `configured_by` has an endpoint.

Performance impact: None.

Security impact: None.

Documentation impact: the EM documentation describes the two fields.

Developer impact: a service that takes part reads its entry, builds the item
path from it, and publishes only what its entry gives it. Services that do not
read EM configuration today, such as smbios-mdr, need a configuration type and
the code to read their entries, and pldmd needs one for the terminus. bmcweb
reads every service on the item path for each resource.

Upgradability impact: items move to the new item path, so their Redfish IDs
change. Clients should take URLs from collections rather than store IDs.

Existing configurations have neither field, so a service that only followed
these rules would stop creating items, and the devices would disappear from
Redfish. During a transition, a service keeps today's behavior for a device
whose entries set neither field. The transition lasts until every service that
creates inventory items and every EM configuration that describes their devices
has been updated, including devices that only one service describes.

### Organizational

- Does this proposal require a new repository? No
- Who will be the initial maintainer(s) of this repository? N/A
- Which repositories are expected to be modified to execute this design?
  - entity-manager
  - bmcweb
  - services that create inventory items, such as pldm and smbios-mdr

## Testing

The configurations can be checked so that each item type appears in at most one
entry of a device, and exactly one entry sets `InventoryItem`. On a system with
a device described by two services, check that both build the same item path,
that only the owner publishes item interfaces, and that the Redfish resource
shows the interfaces of both services.

[discord-thread]:
  https://discord.com/channels/775381525260664832/1534765185460338769/1564902079221207104
[pdi-object]:
  https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/96104
[em-object]: https://gerrit.openbmc.org/c/openbmc/entity-manager/+/96105
[physical-topology]:
  https://github.com/openbmc/docs/blob/master/designs/physical-topology.md
[object-mapper]:
  https://github.com/openbmc/docs/blob/master/architecture/object-mapper.md
[estoraged]: https://github.com/openbmc/docs/blob/master/designs/estoraged.md
[pldm-terminus]: https://gerrit.openbmc.org/c/openbmc/entity-manager/+/96379
