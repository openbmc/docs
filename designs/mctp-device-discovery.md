# MCTP device discovery for upper-layer services

Author: Ender Hsieh (EnderH)

Other contributors:

Created: October 28, 2026

## Problem Description

Services that talk to MCTP devices, such as pldmd, spdmd and the MctpVDM daemon
in dbus-sensors, find their devices by walking the endpoints that mctpd
publishes. A service that cannot recognize its devices by message type alone
sends commands to every endpoint to learn what it is, which adds traffic on the
MCTP links and delays discovery, and still cannot tell two identical devices
apart or where each one sits. A device behind an MCTP bridge has no
configuration a service can read at all. This design lets each service watch for
an Entity Manager (EM) configuration type of its own, follow D-Bus associations
from that configuration to the MCTP endpoint and its EID, and link the inventory
it creates back to the same configuration object. The goals are one discovery
flow for devices reached directly, devices that are bridges and devices behind a
bridge, and a flow that keeps working when an endpoint is lost and rediscovered
with another EID. Changing how mctpd and mctpreactor set up endpoints, and
changing the behavior for devices without an EM configuration, are non-goals.
How a client such as bmcweb presents the items that several services create for
one device is left to a separate design.

## Background and References

[mctpd][mctpd] sets up MCTP endpoints and publishes one D-Bus object per
endpoint, with its EID and the message types it supports.
[mctpreactor][mctpreactor] in dbus-sensors reads the `MCTPI2CTarget`,
`MCTPI3CTarget` and `MCTPUSBDevice` records from EM, asks mctpd to set up an
endpoint for each, and links the endpoint to its record with a `configured_by`
association. Records for devices reached over USB belong to the [platform
configuration][platform-mctp-schema], because only the platform knows the USB
topology, and name the board the device is on in `Board`. A bridge assigns EIDs
from a pool to the endpoints behind it; DSP0236 does not define the order of
that assignment, but its Get Routing Table Entries command (13.13) reports a
static port and the physical address of each endpoint. When a service loses
contact with an endpoint it asks mctpd to [recover it][endpoint-recovery]: an
endpoint that answers within Treclaim keeps its EID, and one that does not is
removed and gets an EID from the pool when it is set up again.

pldmd [discovers termini from mctpd][pldm-discovery]. It takes every endpoint
that supports PLDM and names the terminus after the EM record it finds through
`configured_by`, if any. DSP0248 allows termini to be statically configured into
the platform (14.1) or discovered without configuration (16.3), and makes the
Entity Auxiliary Names PDR optional (28.18). Related proposals are the [physical
topology design][physical-topology], PLDM [sensor names][pldm-sensors] and
[sensor creation][pldm-create-sensors] set on MCTP records, a [PLDM device type
matched by UUID][pldm-device], [bridged endpoints matched by pool
offset][bridge-child], an [SPDM over MCTP configuration type][spdm-mctp], and
the [discussion on PLDM and SPDM device naming][naming-discussion].

## Requirements

A service must be able to find the EID of a device from the EM configuration of
that device, without tracking mctpd itself. The same flow must work for the
three kinds of MCTP device this design covers:

1. Bridge device: an endpoint reached directly over USB, described by an
   `MCTPUSBDevice` record of the platform configuration.
2. Bridged endpoint: an endpoint behind a bridge, which has no record of its own
   today.
3. Direct endpoint: an endpoint on an I2C or I3C bus of the BMC, described by an
   `MCTPI2CTarget` or `MCTPI3CTarget` entry of a board configuration.

The kinds differ in where the record of the endpoint lives, not in the
transport. Today the board schema defines `MCTPI2CTarget` and `MCTPI3CTarget`
and the platform schema defines only `MCTPUSBDevice`, so these are the
transports the examples use. A bridge or a direct endpoint on another transport
follows the same flow once that transport has a record type in the EM schemas
and in phosphor-dbus-interfaces. An endpoint is identified by its object in
mctpd, which carries the network ID along with the EID, so endpoints in
different MCTP networks are handled the same way.

The identity of a device is its configuration object, not its EID. A service
must not keep an EID beyond the life of the endpoint it came from, and must pick
up the new EID when the device is rediscovered after a loss. Each device is
exposed once, by the configuration of the board it is on, and the links between
configuration, endpoint and inventory are D-Bus associations defined in
phosphor-dbus-interfaces, so that services do not have to agree on object paths
or names.

The configuration stays optional. A device without an EM entry keeps today's
behavior, as DSP0248 16.3 expects for add-in cards. A system has tens of such
devices, each with a handful of entries; the associations change only when
configuration or endpoints appear or go away, so the design adds no polling. No
build option is needed: a service uses the flow for the devices that have a
configuration of its type.

## Proposed Design

EM publishes `xyz.openbmc_project.Configuration.Object` on every object it
creates from an `Exposes` entry, as one interface to associate with whatever the
entry's type. Entries that share a `Name` are published on one object, so the
entries that different services need for one device end up on the same object.
Each service has a configuration type of its own on the board entry of the
device: MctpVDM uses the types it already defines, pldmd gets a new
`PLDMTerminus` type, and spdmd uses the [type proposed for it][spdm-mctp]. Three
associations connect the configuration to the endpoint and to the inventory:

- `configured_by` from an MCTP endpoint to the record it was set up from,
  created by mctpreactor (exists today).
- `describes` from a platform record to the board entry with the same `Name` on
  the board named in `Board`, created by EM.
- `configured_by` from an inventory item to the configuration object it was
  created from, created by the service that creates the item.

The service also links each item with `contained_by` to the board that exposes
the configuration object, which is the parent of the object's path, as
dbus-sensors already does for the items and sensors it creates; EM links the
board to its chassis. Because the flow starts from the board entry rather than
from the platform record, the item lands on the board the device is on.

A bridge device is set up from its platform record, which describes the board
entry:

```text
 platform configuration                       board configuration
+-----------------------------+              +-----------------------------+
| platform/<Platform>/        |              | board/<SwitchBoard>/        |
|   Bridge_0                  |--describes-->|   Bridge_0                  |
| Configuration.MCTPUSBDevice |              | Configuration.Object        |
|   Board: <SwitchBoard>      |              | Configuration.<Service>     |
+-----------------------------+              +-----------------------------+
              ^                                            ^
              | configured_by                              | configured_by
+-----------------------------+              +-----------------------------+
| MCTP endpoint, EID 9        |              | item created by the service |
+-----------------------------+              +-----------------------------+
```

A bridged endpoint can follow the same chain once mctpd [shares the port and
physical address][routing-table] the bridge reports for each endpoint. The
device then needs a record of its own to match against. One way is to derive it
from the [physical topology design][physical-topology]; another is to turn the
endpoints nested in the bridge record today into records next to it, naming the
bridge, the port and the address:

```json
{
  "Board": "Switch Board",
  "BridgedEndpoints": [{ "Board": "GPU Board 0", "Name": "Gpu_0" }],
  "Name": "Bridge_0",
  "Type": "MCTPUSBDevice"
}
```

```json
[
  {
    "Board": "Switch Board",
    "Name": "Bridge_0",
    "Type": "MCTPUSBDevice"
  },
  {
    "Address": "0x52",
    "Board": "GPU Board 0",
    "Bridge": "Bridge_0",
    "BridgePort": 6,
    "Name": "Gpu_0",
    "Type": "MCTPBridgedEndpoint"
  }
]
```

```text
 platform configuration                       board configuration
                                             +-----------------------------+
                                             | board/<GpuBoard>            |<-+
                                             +-^---------------------------+  |
                                               | parent          contained_by |
+-----------------------------+              +-----------------------------+  |
| platform/<Platform>/Gpu_0   |              | board/<GpuBoard>/Gpu_0      |  |
| Configuration.              |--describes-->| Configuration.Object        |  |
|   MCTPBridgedEndpoint       |              | Configuration.<Service>     |  |
|   Board: <GpuBoard>         |              +-----------------------------+  |
|   Bridge: Bridge_0          |                            ^                  |
|   BridgePort, Address       |                            | configured_by    |
+-----------------------------+              +-----------------------------+  |
              ^                              | item created by the service |--+
              | configured_by                +-----------------------------+
+-----------------------------+
| MCTP endpoint, EID 10,      |
| from the pool of Bridge_0,  |
| matched by the port and     |
| address the bridge reports  |
+-----------------------------+
```

A direct endpoint is set up from an entry of the board configuration, which
shares the object with the service's entry when both have the same `Name`:

```text
 board configuration
+-----------------------------+
| board/<Board>/Mcu_0         |
| Configuration.Object        |
| Configuration.MCTPI2CTarget |
| Configuration.<Service>     |
+-----------------------------+
      ^                     ^
      | configured_by       | configured_by
+-----------------------+ +-----------------------------+
| MCTP endpoint, EID 12 | | item created by the service |
+-----------------------+ +-----------------------------+
```

A service handles the three kinds with one flow:

1. Watch for objects with its `Configuration.<Service>` interface.
2. If the object `configures` an MCTP endpoint, use that endpoint. Otherwise
   follow `described_by` to the platform record and use the endpoint the record
   `configures`.
3. Talk to the EID, create the inventory, and link each item to the object with
   `configured_by` and to the parent of the object, its board, with
   `contained_by`.

Settings a service needs for a device belong on the device's entry of that
service's type, on the board, rather than on the record that describes the
transport. The proposed [PLDM sensor names][pldm-sensors] and [sensor creation
switch][pldm-create-sensors] would then be properties of `PLDMTerminus`, which
also covers devices behind a bridge that have no transport record of their own.

Since the flow is the same for every service, it fits a shared helper that
reports, for each configuration object of a type, the EID it currently resolves
to and every change of it. The helper can be extracted once MctpVDM and pldmd
implement the flow and the interface has settled; where it lives is open for
review.

Endpoint loss and rediscovery. When a service cannot reach an endpoint it asks
mctpd to recover it. If the endpoint answers within Treclaim it keeps its EID
and the associations do not change. Otherwise mctpd removes the endpoint,
mctpreactor drops its `configured_by` and sets the device up again, and the new
endpoint, usually with another EID, gets the association. A service following
the associations therefore sees a loss as its endpoint going away and a
rediscovery as an endpoint appearing, the same event as the first discovery, and
never holds an EID that is no longer valid. mctpd cannot yet [recover an
endpoint behind a bridge][bridged-recover-issue] (a [fix][bridged-recover-pr] is
in review), and nothing re-associates a bridged endpoint until the matching
above exists.

Ordering. EM publishes the entries and the `describes` association, mctpd
publishes the endpoint, and mctpreactor adds `configured_by` only after the
setup of the endpoint completes, all independently. A service watches the
associations appear and go away through the object mapper rather than looking
them up once at startup.

## Alternatives Considered

Keeping discovery from mctpd, with EM providing only a name, is what pldmd does
today. It finds direct endpoints, but an endpoint behind a bridge has no record
to take a name from, and a service using a vendor defined message type has to
probe every endpoint to learn what it is, which cannot tell two identical
devices apart. [Matching a device by the UUID it reports][pldm-device] ties the
configuration to one physical part rather than to its position in the system,
and [matching by the order of a bridge's EID pool][bridge-child] relies on an
order DSP0236 does not define.

Within this design, the name and type of an entry could live on
`Configuration.Object` rather than on the type specific interface. Entries that
share a `Name` are published on one object, which is what lets services meet on
it, so one object can carry several types and the marker interface carries no
properties. The platform record could also refer to the board entry with a
property rather than an association, but an association gives the reverse
direction through the object mapper, which is the direction a service needs.

## Impacts

API impact: phosphor-dbus-interfaces gains `Configuration.Object`, the
`configured_by` association of `Inventory.Item`, and the `describes` association
of `Configuration.MCTPUSBDevice`. EM gains the `PLDMTerminus` configuration
type. Existing interfaces and associations are unchanged.

Performance impact: one more interface per configuration object and one
association per platform record. On a system with 42 configuration objects, the
memory use of EM and the object mapper and the time of `GetManagedObjects` and
`GetSubTree` did not change measurably.

Security impact: None.

Documentation impact: the EM D-Bus API and association documents describe the
new interface and association.

Developer impact: a board configuration exposes a device once and gives every
entry for it the same `Name`. A service that adopts the design defines its
configuration type and follows the associations instead of probing endpoints.

Upgradability impact: None. Services that do not adopt the design keep working
as before.

### Organizational

- Does this proposal require a new repository? No
- Who will be the initial maintainer(s) of this repository? N/A
- Which repositories are expected to be modified to execute this design?
  - phosphor-dbus-interfaces
  - entity-manager
  - dbus-sensors (mctpreactor and MctpVDM)
  - pldm
  - spdm
  - mctp (CodeConstruct)
- Make a list, and add listed repository maintainers to the gerrit review.

## Testing

On a system with a USB attached MCTP bridge and devices behind it, check that
every configuration object has `Configuration.Object`, that every platform
record `describes` the board entry with the same name, and that following
`described_by` and `configures` from a board entry reaches the expected EID.
Then reset a device for longer than Treclaim and check that the association
moves to the new endpoint and the service resumes on the new EID. Unit tests in
entity-manager cover publishing the interface and the association, including
entries added and deleted at run time. CI is not affected beyond these tests.

[mctpd]: https://github.com/CodeConstruct/mctp
[mctpreactor]: https://github.com/openbmc/dbus-sensors/tree/master/src/mctp
[platform-mctp-schema]:
  https://github.com/openbmc/entity-manager/blob/master/schemas/platform/mctp.json
[endpoint-recovery]:
  https://github.com/CodeConstruct/mctp/blob/main/docs/endpoint-recovery.md
[pldm-discovery]:
  https://github.com/openbmc/pldm/blob/master/requester/mctp_endpoint_discovery.cpp
[physical-topology]: https://gerrit.openbmc.org/c/openbmc/docs/+/94137
[pldm-sensors]: https://gerrit.openbmc.org/c/openbmc/entity-manager/+/86156
[pldm-create-sensors]:
  https://gerrit.openbmc.org/c/openbmc/entity-manager/+/88233
[pldm-device]: https://gerrit.openbmc.org/c/openbmc/entity-manager/+/80059
[bridge-child]: https://gerrit.openbmc.org/c/openbmc/entity-manager/+/93808
[spdm-mctp]: https://gerrit.openbmc.org/c/openbmc/entity-manager/+/83263
[naming-discussion]:
  https://discord.com/channels/775381525260664832/1534765185460338769
[routing-table]: https://github.com/CodeConstruct/mctp/issues/170
[bridged-recover-issue]: https://github.com/CodeConstruct/mctp/issues/149
[bridged-recover-pr]: https://github.com/CodeConstruct/mctp/pull/175
