# MCTP device discovery for upper-layer services

Author: Ender Hsieh (EnderH)

Other contributors:

Created: October 28, 2026

## Terms

- **configuration file**: An Entity Manager (EM) JSON file, either a board
  configuration or a platform configuration.
- **configuration entry object path**: The D-Bus object path EM publishes for
  each item in the `Exposes` array of a configuration file,
  `/xyz/openbmc_project/inventory/system/<type>/<Board>/<Name>`, carrying a
  `Configuration.<Type>` interface. Items that share a `Name` are published on
  one object path, as described in the [inventory correlation
  design][inventory-correlation].
- **board object path**: The parent D-Bus object path of a configuration entry
  object path, which represents the board the configuration file is bound to.
- **MCTP record object path**: A configuration entry object path carrying
  `Configuration.MCTPI2CTarget`, `Configuration.MCTPI3CTarget` or
  `Configuration.MCTPUSBDevice`, from which mctpreactor sets up an endpoint.
- **device entry**: An item in `Exposes` that describes how a device is managed
  over MCTP, such as `NvidiaMctpVdmGpu` for a GPU that implements the NVIDIA
  vendor defined messages, or `PLDMTerminus` for a device that is a PLDM
  terminus. EM publishes it as a `Configuration.<Type>` interface.
- **endpoint object path**: The D-Bus object path mctpd publishes for each
  endpoint, `/au/com/codeconstruct/mctp1/networks/<net>/endpoints/<eid>`.
- **inventory item object path**: A D-Bus object path under
  `/xyz/openbmc_project/inventory/` that a producer service creates for a
  device.
- **producer service**: A service that talks to MCTP devices and creates
  inventory item object paths for them, such as pldmd, spdmd or nvidia-gpu.

## Problem Description

Services that talk to MCTP devices, such as pldmd, spdmd and daemons using
vendor defined messages over MCTP, for example nvidia-gpu, find their devices by
walking the endpoints that mctpd publishes. The message types an endpoint
supports are not enough to tell what it is. For example, an endpoint that
supports SPDM may act as a requester, a responder or both, and its message types
do not say which. This information is therefore usually provided in EM
configuration files. Each service currently finds its configuration and EID in
its own way, so the discovery steps should be unified, letting every service
follow the same steps to obtain the configuration, the EID and the associations
it needs.

There are two kinds of MCTP device. One is a device connected directly to the
BMC over any transport, such as I2C, I3C or USB. The other is a bridged endpoint
device, which sits behind an MCTP bridge. Only directly connected devices are
linked to their configuration today, as described under Background.

This design lets a producer service find the devices described by the device
entries it supports, follow the `configured_by` association to the endpoint
object path and its EID, and link the inventory item object paths it creates
back to the same configuration entry object path. The goals are one discovery
flow for both kinds of device, and a flow that keeps working when an endpoint is
lost and rediscovered with another EID. Changing how mctpd and mctpreactor set
up endpoints and associations, and changing the behavior for devices without a
configuration entry object path, are non-goals. Telling which inventory items
that several services create belong to one device is left to the [inventory
correlation design][inventory-correlation], which also requires such a device to
be described in an EM configuration file.

## Background and References

[mctpd][mctpd] sets up MCTP endpoints and publishes an endpoint object path for
each, with its EID and the message types it supports. [mctpreactor][mctpreactor]
in dbus-sensors reads the MCTP record object paths from EM, which come from
`MCTPI2CTarget` and `MCTPI3CTarget` in board configurations and `MCTPUSBDevice`
in the platform configuration. It asks mctpd to set up an endpoint for each, and
links the endpoint object path to the MCTP record object path with a
`configured_by` association. Bridged endpoint devices do not have this
association yet; the [bridged MCTP endpoint inventory associations
design][physical-topology], currently in review, proposes adding it. This design
assumes the association exists, and how it is created is out of scope.

pldmd is one service that already uses this association. It [discovers termini
from mctpd][pldm-discovery] by taking every endpoint that supports PLDM, and
when it finds an MCTP record object path through `configured_by`, it names the
terminus after the `Name` of that record.

## Requirements

A producer service must be able to start from the configuration entry object
path of a device, follow the association to the current endpoint object path of
that device, and read the EID from it, without depending on the order in which
EM, mctpd and mctpreactor publish their objects. The same flow must work for
both kinds of device, directly connected devices and bridged endpoint devices.
The flow depends only on the `configured_by` association between the endpoint
object path and the configuration entry object path, not on the transport, so it
works for any transport mctpreactor supports, and a new transport needs no
change to the flow once its record type is defined in the EM schemas and in
phosphor-dbus-interfaces.

The identity of a device is its configuration entry object path, not its EID. A
producer service must not keep an EID beyond the life of the endpoint it came
from, and must pick up the new EID when the device is rediscovered after a loss.
The links between configuration entry, endpoint and inventory item object paths
are D-Bus associations defined in phosphor-dbus-interfaces.

This design covers only devices that have a configuration entry object path. A
device without one, such as an endpoint that was not set up by mctpreactor,
keeps the discovery each service does today. The associations change only when
configuration entry or endpoint object paths appear or go away, so the design
adds no polling. No build option is needed: a producer service uses the flow for
the devices that have a configuration entry object path carrying a device entry
it supports.

## Proposed Design

### Associations

The design builds on one association that already exists:

- `configured_by` from an endpoint object path to a configuration entry object
  path, created by mctpreactor. Directly connected devices have it today, and
  the [bridged MCTP endpoint inventory associations design][physical-topology]
  adds it for bridged endpoint devices.

It adds two associations, created by the producer service for each inventory
item object path it creates. Both are already defined in
phosphor-dbus-interfaces:

- `configured_by` to the configuration entry object path. The configuration
  entry object path carries `Configuration.Object`, published by EM, which
  phosphor-dbus-interfaces requires at the far end of this association.
- `contained_by` to the board object path, which is the parent of the
  configuration entry object path, as dbus-sensors already does for the items
  and sensors it creates.

The association from mctpreactor points to the MCTP record object path, while
the producer service watches for the configuration entry object path carrying a
device entry it supports. These are the same object path when the two items in
the configuration file share a `Name`, since EM publishes them on one object
path, as described in the [inventory correlation design][inventory-correlation].

```text
+--------------------------------+
| EM                             |
| board object path              |<------------------------------------+
| .../board/<Board>              |                                     |
+--------------------------------+                                     |
               ^ parent                                                |
+--------------------------------+                                     |
| EM                             |                                     |
| configuration entry            |                                     |
| object path                    |                                     |
| .../board/<Board>/<Name>       |                                     |
| Configuration.MCTPI2CTarget    |                                     |
| Configuration.<Type>           |                                     |
+--------------------------------+                                     |
       ^                      ^                                        |
       | configured_by        | configured_by         contained_by     |
       | (mctpreactor)        | (producer service)    (producer        |
       |                      |                        service)        |
+----------------+  +-----------------------------+                    |
| mctpd          |  | producer service            |                    |
| endpoint       |  | inventory item object path  |--------------------+
| object path    |  +-----------------------------+
+----------------+
```

### Discovery flow

A producer service keeps the objects of EM, mctpd and mctpreactor in a cache, as
described under [Ordering](#ordering). A device is ready when all three of the
following hold:

1. A configuration entry object path carries a device entry the producer service
   supports.
2. An endpoint object path points to that configuration entry object path
   through `configured_by`.
3. mctpd still publishes that endpoint object path.

When the device is ready, the producer service reads the EID from the endpoint
object path, talks to the device, and creates its inventory item object paths
with `configured_by` and `contained_by`. When any of the conditions stops
holding, the producer service stops using the EID until they hold again. The
inventory item object paths stay, and the producer service marks the device as
unavailable through their decorator interfaces. The producer service removes the
inventory item object paths only when the configuration entry object path goes
away. The conditions can come to hold in any order, so their numbering is the
order in which they are checked, not the order of events.

### Endpoint loss and rediscovery

When a producer service cannot reach an endpoint, it asks mctpd to
[recover][endpoint-recovery] it. If the endpoint answers within Treclaim, it
keeps its EID and the conditions do not change. Otherwise mctpd removes the
endpoint and condition 3 stops holding. When mctpreactor sets the device up
again, the new endpoint object path, usually with another EID, gets its
`configured_by`, and conditions 2 and 3 hold again. To a producer service, a
loss is the conditions no longer holding and a rediscovery is them holding
again, the same event as the first discovery, so it never holds an EID that is
no longer valid.

### Ordering

EM, mctpd and mctpreactor publish their objects independently, so a producer
service cannot assume the order in which they appear. Following the associations
step by step through the object mapper is racy: the mapper learns of each change
separately from the producer service, and each lookup depends on the result of
the previous one, so an object can go away between them.

[One way to avoid this][espy] is for the producer service to keep a cache of the
objects of EM, mctpd and mctpreactor, kept current from their ObjectManager
signals and NameOwnerChanged, to follow the associations within the process, and
to check the conditions above again whenever any source changes. mctpreactor
publishes the association on the same path as the endpoint object path, and the
association holds the configuration entry object path, so the cache can look up
either side from the other. Since every producer service would keep such a
cache, whether each producer service implements it or a shared library provides
it is still open for discussion.

## Alternatives Considered

Starting from the endpoint object path is what pldmd does today: it takes every
endpoint that supports its message type and follows `configured_by` to the
configuration entry object path. With the same cache this is free of races as
well, but the producer service has to check every endpoint. This design starts
from the device entry instead, because the device entry, not the message types,
describes what the device is.

Following the associations through the object mapper without a cache is also
what pldmd does today. It is the simplest to implement, but it has the race
described under [Ordering](#ordering).

[Matching the UUID a device reports][pldm-device] against a UUID written in the
configuration file would link the endpoint and the configuration without an
association. However, each card is given its own UUID when it is made, so the
UUID tells which card it is but not where the card is installed. The
configuration then no longer matches once a card is replaced, and machines of
the same model cannot share one configuration file.

## Impacts

API impact: the `configured_by` association of `Inventory.Item` and the
`Configuration.Object` interface are already defined in
phosphor-dbus-interfaces. EM needs to publish `Configuration.Object` on every
configuration entry object path, as the interface the `configured_by`
association of an inventory item object path requires. Existing interfaces and
associations are unchanged.

Performance impact: one more association per inventory item object path. Each
producer service that adopts the design keeps a cache of the objects of EM,
mctpd and mctpreactor, which grows with the number of configuration entry and
endpoint object paths. A system has only tens of such devices.

Security impact: None.

Documentation impact: the association descriptions in phosphor-dbus-interfaces
already cover `configured_by`.

Developer impact: in a configuration file, the MCTP record of a device and its
device entry share a `Name`. A producer service that adopts the design defines
the device entries it supports and follows the discovery flow to find the
endpoint, instead of walking the endpoints itself.

Upgradability impact: None. Services that do not adopt the design keep working
as before.

### Organizational

- Does this proposal require a new repository? No
- Who will be the initial maintainer(s) of this repository? N/A
- Which repositories are expected to be modified to execute this design?
  - entity-manager
  - dbus-sensors (nvidia-gpu)
  - pldm
  - spdm

## Testing

On a system with directly connected MCTP devices, check that:

- every configuration entry object path carries `Configuration.Object`;
- for every configuration entry object path carrying a device entry, the
  producer service finds the endpoint object path through `configured_by` and
  talks to its EID;
- every inventory item object path the producer service creates has
  `configured_by` to the configuration entry object path and `contained_by` to
  the board object path.

Then restart EM, mctpd and mctpreactor in different orders and check that the
result is the same each time, which shows that discovery does not depend on the
order of publication.

Then reset a device for longer than Treclaim and check that:

- the inventory item object paths stay during the loss and are marked as
  unavailable;
- after mctpreactor sets the endpoint up again, the producer service resumes on
  the new EID.

The same checks can run in CI as integration tests, using qemu's usbredir
support with [mctp-dev][mctp-dev] to emulate the MCTP devices, and for bridged
endpoint devices once mctp-dev can also emulate a bridge. Unit tests in
entity-manager cover publishing `Configuration.Object`, including entries added
and deleted at run time. Unit tests in the producer service provide the objects
of the three sources in any order and check how the conditions are evaluated.

[mctpd]: https://github.com/CodeConstruct/mctp
[mctpreactor]: https://github.com/openbmc/dbus-sensors/tree/master/src/mctp
[mctp-dev]: https://github.com/CodeConstruct/mctp-dev
[inventory-correlation]: https://gerrit.openbmc.org/c/openbmc/docs/+/96526
[endpoint-recovery]:
  https://github.com/CodeConstruct/mctp/blob/main/docs/endpoint-recovery.md
[pldm-discovery]:
  https://github.com/openbmc/pldm/blob/master/requester/mctp_endpoint_discovery.cpp
[physical-topology]: https://gerrit.openbmc.org/c/openbmc/docs/+/94137
[pldm-device]: https://gerrit.openbmc.org/c/openbmc/entity-manager/+/80059
[espy]: https://gist.github.com/amboar/5485218c708c554599d235988d71e85b
