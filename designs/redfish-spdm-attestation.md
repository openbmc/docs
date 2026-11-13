# Redfish SPDM Attestation Support

Author: Zhichuang Sun

Other contributors: Jerome Glisse, Ed Tanous, Manojkiran Eda, Ankita Gawade

Created: June 27th, 2023 Last Updated: Oct 30th, 2023

## Problem Description

Redfish added schema for
[ComponentIntegrity](https://redfish.dmtf.org/redfish/schema_index), which
allows users to use [SPDM](https://www.dmtf.org/standards/spdm) or
[TPM](https://trustedcomputinggroup.org/resource/trusted-platform-module-tpm-summary/)
to authenticate device identity, hardware configuration and firmware integrity.
It would be useful to add SPDM attestation support in BMCWeb, which provides
unified interface for device security attestation in data centers, and provide a
generic implementation for the SPDM D-Bus Daemon.

This design focuses on SPDM.

## Background and References

SPDM (Security Protocols and Data Models) is a spec published by
[DMTF](https://www.dmtf.org/sites/default/files/standards/documents/DSP0274_1.3.0.pdf).
It is designed for secure attestation of devices. GitHub repo
[libspdm](https://github.com/DMTF/libspdm) provides an open-source
implementation of the SPDM protocol. Redfish Schema
[ComponentIntegrity](https://redfish.dmtf.org/schemas/v1/ComponentIntegrity.v1_2_1.json)
adds support for doing SPDM-based device attestation over Redfish API.

## Requirements

This feature aims at supporting SPDM attestation through Redfish API and
providing system administrators and operators an easy way to remotely verify the
identity and integrity of devices.

This design includes:

- New D-Bus interfaces for Redfish resources `ComponentIntegrity` and
  `TrustedComponent`.
- BMCWeb changes for supporting the above Redfish resources.
- Design for SPDM Attestation D-Bus Daemon, demonstrating how to fetch the
  attestation results over D-Bus.

## Proposed Design

### Attestation related D-Bus Interfaces

There are three type of information we will need from an attestation daemon on
D-Bus:

1. Basic information, like attestation protocol, enablement status, update
   timestamp, etc.
2. Identity information, e.g., device identity certificates.
3. Measurements information, e.g., measurements of the component firmware,
   hardware configuration, etc.

So far, phosphor-dbus-interfaces lacks interfaces defined for attestation
purpose. Thus, we propose three new interfaces:

- `Attestation.ComponentIntegrity`
- `Attestation.IdentityAuthentication`
- `Attestation.MeasurementSet`

`Attestation.ComponentIntegrity` provides basic component integrity information,
including the protocol to measure the integrity, last updated time, attestation
enablement status, etc. There are also associations proposed for this interface,
including a link to the trusted component that the component integrity object is
reporting, and a link to the systems that the component integrity object is
protecting. Note, the "trusted component" in this doc refers to a trusted
device, such as a TPM, an integrated Root of Trust (ROT).

`Attestation.IdentityAuthentication` provides identity verification information.
Two associations are proposed to link it to the requester and the responder's
certificates.

`Attestation.MeasurementSet` defines the dbus method to get SPDM measurements.
It can be extended to measurements fetched using other protocol in the future.

The proposed Phosphor D-Bus Interfaces is here:
[component-integrity](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/64354).

### TrustedComponent related D-Bus Interfaces

We also propose to add one Inventory interface for `TrustedComponent`, namely
`Inventory.Item.TrustedComponent`. `TrustedComponent` represents a trusted
hardware component, typically known as Root of Trust(ROT). It can be an
externally attached security chip, like a TPM, or a hardware IP integrated into
a device. It can securely measure the integrity information of a device.

`Inventory.Item.TrustedComponent` interface defines the following properties

- `AttachmentType`, which gives information on whether this trusted component is
  integrated into the device or is discrete from the device.

A `TrustedComponent` is typically associated with other hardware components
which it is protecting. It should also be associated with the component
integrity object reported by this `TrustedComponent`.

The proposed Phosphor D-Bus Interfaces for `TrustedComponent` is here:
[trusted-component](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/64355).

### SPDM Attestation D-Bus Daemon

[Experimental support for SPDM exists for `bmcweb`](https://github.com/openbmc/bmcweb/compare/master...sunzc:bmcweb:spdm),
which adds routes in the BMCWeb for `ComponentIntegrity` and `TrustedComponent`
to support it. But BMCWeb collects the information from D-Bus. The SPDM
Attestation D-Bus Daemon does the actual work.

SPDM protocol needs to be bound to a transport layer protocol, which transmits
SPDM messages between the BMC and the device. The transport layer protocol can
be MCTP, PCIe-DOE, or even TCP socket. For MCTP, the lower physical layer can be
PCI-VDM, SMBus/I2C, and so on. Note,
[`libspdm` already provides transport layer protocol binding](https://github.com/DMTF/libspdm/blob/main/include/internal/libspdm_common_lib.h#L445-L446)
with message encoding/decoding support. The device send/receive function is left
for SPDM daemon to implement If the transport layer is using standard MCTP,
PCIe-DOE, or TCP setting up the transport layer connection could be easy. In
this design, we only consider SPDM over standard MCTP,TCP & PCIe-DOE connection.

For SPDM-over-MCTP, SPDM daemon can query the mctpd for information about MCTP
endpoint, including the endpoint id(eid) and upper layer responder, and create a
connection only for endpoint that has SPDM as its upper layer responder.

For SPDM-over-TCP, SPDM daemon can query [entity-manager][entity-source] for
information about remote TCP endpoints, including the `hostname / ip-address` &
`port` details from the `SpdmTcpResponder` interface and create a connection
either by using the reach out model (or) reach down model as stated in the [SPDM
over TCP specification][spdm-tcp-spec]

[entity-source]: https://github.com/openbmc/entity-manager
[spdm-tcp-spec]:
  https://www.dmtf.org/sites/default/files/standards/documents/DSP0287_1.0.0.pdf

For SPDM-over-PCIe-DOE, SPDM daemon need the PCIe device BDF to handle DOE
mailbox discovery. Given that not all PCIe devices support DOE support SPDM, we
cannot query about whether a DOE capable device supports SPDM. Therefore, we
need a way to pass the device info to the dameon. However, PCIe device
BDF(Bus:Device:Function) info are dynamically assigned during system boot. The
same device may get assigned different BDF on different machine. What the daemon
needs should be the PCIe device ID, which is identified by `VendorId:DeviceId`.
For the convenience of configuration, we should pass PCIe device ID to the
daemon, so that the daemon can enumerate all the PCIe devices and find the
matching devices by their device ID. There are different ways to pass PCIe
device ID info to the dameon, e.g., configuration file, command line parameters.

For PCIe DOE devices, SPDM daemon can enumerage all PCI devices under sysfs and
find out all BDFs with matching `VendorId:DeviceId`. SPDM daemon also needs to
query `InventoryManager` to get all PCI device inventory paths. By querying
`InventoryManager` managed objects and checking object interface
`xyz.openbmc_project.Inventory.Item.PCIeDevice`, which has property
`FunctionXVendorId` and `FunctionXDeviceId` (X represents numbers from 0 to 7),
we can find all PCI device inventory paths with matching `VendorId:DeviceId`. So
far, there is no universal way to map a given device's BDF to its inventory
path. It is up to the vendor to do the association.

For MCTP devices detection, the community has been working on `mctpreactor`
daemon in dbus-sensors to handle the configuration. The proposed
[implementation](https://gerrit.openbmc.org/c/openbmc/dbus-sensors/+/69111); the
related [PR discussion](https://github.com/CodeConstruct/mctp/pull/17). SPDM
daemon will monitor `mctpd` for `InterfacesAdded` signals providing the
`xyz.openbmc_project.MCTP.Endpoint` interface, which exposes the message types
supported by the endpoint in the `SupportedMessageTypes` member. SPDM daemon set
up a connection with the SPDM-capable endpoints to get certificates and
measurements. For signals sent before SPDM daemon launches, SPDM daemon should
query the `mctpd` for any detected endpoints after it gets launched.

For Network based device detection , SPDM daemon would monitor for
`InterfacesAdded` signal provided by
`xyz.openbmc_project.Configuration.SpdmTcpResponder` interface, which exposes
the `Hostname` & `Port` of the remote entity. For signals sent before SPDM
daemon launches, SPDM daemon should query the `entity-manager` for any detected
endpoints after it gets launched.

Below is a high-level diagram showing the relationship between different
components.

```ascii
    +------+            +---------+
    |Client|            |Inventory|
    +--+---+            |Manager  |
       |                +---^-----+           +-------+
       |                    |                 |PCIe   |
    +--v---+            +---+---+------------>|Device |
    |BMCWeb+----------->|SPDM   |             +-------+
    +------+            |Daemon |
                        +---+---+------------>+-------+
                            |                 |MCTP   |
                        +---v---+             |Device |
                        |mctpd  |             +-------+
                        +-------+
```

A reference D-Bus Daemon workflow would be like this:

0. (Probing phase) Entity Manager will parse the configuration file for trusted
   components; mctpd finish discovery of mctp endpoints. These are prerequisites
   for SPDM daemon.
1. Check transport layer protocol. For MCTP, it queries mctpd to gather all eids
   that support SPDM; For PCIe-DOE, it performs DOE mailbox discovery with the
   PCIe device ID; For TCP, it queries entity-manager to gather the remote IP
   address & Port.
2. For each endpoint, which could be MCTP, PCIe-DOE or TCP, SPDM daemon query
   Entity Manager for the matching trusted component configuration. It then
   creates and initializes the corresponding D-Bus object for `TrustedComponent`
   and `ComponentIntegrity` with device specific information. For MCTP
   responders, this configuration and the inventory associations of step 3 are
   defined in
   [Inventory Mapping for MCTP Responders](#inventory-mapping-for-mctp-responders).
3. Create the associations between the above objects and associations with other
   objects, e.g., protected components, active software images;
4. Set up a connection between the BMC and each SPDM-capable device;
5. Initialize the SPDM context on top of the connection.
6. Exchange SPDM messages to get device certificates.
7. Create device certificate objects and create certificate associations for
   trusted component object and component integrity object.
8. Wait on D-Bus and serve any runtime `SPDMGetSignedMeasurements` requests.

### Inventory Mapping for MCTP Responders

MCTP discovery tells the [SPDM daemon](#spdm-attestation-d-bus-daemon) which
endpoints are SPDM-capable. It does not tell it how a responder relates to
inventory: which Chassis the RoT is in, which components it protects and, for an
integrated RoT, which device it is built into.

This design adds one `SpdmMctpResponder` record, called the SPDM record below,
to the Entity Manager configuration of the board that carries the RoT. The SPDM
daemon uses the record to find the inventory objects and publishes associations
to them, and BMCWeb turns the associations into the Redfish links. TCP and
PCIe-DOE discovery are unchanged.

This section uses three Entity Manager terms:

- A board configuration describes one board or device.
- A record is one entry of the `Exposes` list of a configuration.
- The MCTP record of a device is the record that `mctpreactor` uses to set up
  its endpoint.

#### The SpdmMctpResponder record

The record names the RoT and, with three Protected properties, says what it
protects.

The three Protected properties are needed because BMCWeb must have the inventory
object of each protected component to build its Redfish link. The SPDM daemon
cannot find that object by itself. A protected component need not be an MCTP
device, so it may have no endpoint to discover, and its inventory object is
created by another service at a path the SPDM daemon does not know. The three
properties tell it where to look.

| Property                  | Required | Description                                                                                                                                                                       |
| ------------------------- | -------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `Name`                    | yes      | Must match the `Name` of the MCTP record of the device.                                                                                                                           |
| `Type`                    | yes      | `SpdmMctpResponder`.                                                                                                                                                              |
| `TrustedComponentType`    | yes      | `integrated` when the RoT is part of a device it protects, `discrete` when it is a device of its own.                                                                             |
| `TrustedComponentName`    | yes      | Name of the RoT. It becomes the name of the TrustedComponent and ComponentIntegrity, and their Redfish Id. Must be unique among the responders on the BMC.                        |
| `ProtectedBoards`         | no       | Name of the board configuration that holds each protected component. When not given or empty, it is the board configuration that contains this record.                            |
| `ProtectedTargets`        | no       | Name of the record, in that board configuration, that describes each protected component. When not given or empty, the component is the device of the board configuration itself. |
| `ProtectedInventoryTypes` | no       | Inventory interface that the protected component must have. Needed only when more than one inventory object is created from the selected record.                                  |

The three Protected properties are arrays, because one RoT can protect several
components. Entry N of each array describes the same component, so arrays that
are given have the same length and at least one entry. An empty string as an
entry counts as not given for that component. For an integrated RoT, the first
entry is the device it is built into. When no array is given, the RoT protects
the device that its board configuration describes.

The board configuration that contains the SPDM record must be one that BMCWeb
shows as a Chassis, such as a `Board` or `Chassis` configuration. That Chassis
holds the TrustedComponent.

#### Example: a network card with an integrated RoT

A network card has an integrated RoT that protects its network adapter. The MCTP
record of the card is in a platform configuration. A platform configuration
lists the MCTP devices of several boards, so each record in it names its board
with `Board`. The card also has a board configuration of its own:

```text
Platform configuration
    "CX_0"    MCTP record of the card, with Board "CX_Card_0"

Board configuration "CX_Card_0", Type "Board"
    "NIC_0"   record of the network adapter
    "CX_0"    SPDM record, shown below
```

The SPDM record takes the name of the MCTP record, `CX_0`, not the name of the
board:

```json
{
  "Name": "CX_0",
  "Type": "SpdmMctpResponder",
  "TrustedComponentType": "integrated",
  "TrustedComponentName": "IRoT_CX_0",
  "ProtectedTargets": ["NIC_0"],
  "ProtectedInventoryTypes": [
    "xyz.openbmc_project.Inventory.Item.NetworkAdapter"
  ]
}
```

The record gives two of the three arrays:

- `ProtectedBoards` is not given, because the adapter is on the board that
  contains the record.
- `ProtectedTargets` is given, because the RoT protects the adapter and not the
  whole card.
- `ProtectedInventoryTypes` is given, because two inventory objects are created
  from the record `NIC_0`, a NetworkAdapter and a PCIe device.

Redfish then has:

```text
TrustedComponent:    /redfish/v1/Chassis/CX_Card_0/TrustedComponents/IRoT_CX_0
IntegratedInto:      /redfish/v1/Chassis/CX_Card_0/NetworkAdapters/NIC_0
ComponentsProtected: /redfish/v1/Chassis/CX_Card_0/NetworkAdapters/NIC_0
ComponentIntegrity:  /redfish/v1/ComponentIntegrity/IRoT_CX_0
TargetComponentURI:  /redfish/v1/Chassis/CX_Card_0/TrustedComponents/IRoT_CX_0
```

The SPDM daemon gets from the MCTP endpoint to these links in two lookups.

#### Finding the SPDM record

For each SPDM-capable endpoint, the SPDM daemon follows the `configured_by`
association to the MCTP record of the device. `mctpreactor` already publishes
this association. The SPDM record is the record with the same `Name` as the MCTP
record, on the board that the MCTP record belongs to:

```ascii
 SPDM-capable MCTP endpoint
       |
       |  configured_by
       v
 MCTP record, Name = CX_0
       |
       +-- it is in a platform configuration, with Board = CX_Card_0:
       |       find the board configuration CX_Card_0,
       |       then take its SPDM record CX_0
       |
       +-- it is in a board configuration:
               take the SPDM record CX_0 of that same board
```

The [platform MCTP schema][platform-mctp] pairs a platform record with the
records of its board in the same way. An endpoint without a matching SPDM record
keeps the `<network>_<EID>` name that the SPDM daemon gives it and has no
inventory associations. It has no Chassis, so Redfish cannot show a
TrustedComponent for it.

#### Finding the protected inventory

For the example, the three Protected properties are followed like this:

```ascii
 SPDM record CX_0 on the board CX_Card_0
       |
       |  ProtectedBoards not given:  stay on the board CX_Card_0
       |  ProtectedTargets = NIC_0:   take the record NIC_0 of that board
       v
 Record NIC_0
       |
       |  configures
       v
 Inventory objects created from NIC_0: a NetworkAdapter and a PCIe device
       |
       |  ProtectedInventoryTypes = NetworkAdapter:  keep that one
       v
 NetworkAdapter object, the protected component
```

The record `NIC_0` is configuration only. Another service creates the inventory
object of the adapter from it, so the `configures` step is needed to reach that
object. A board configuration is itself an inventory object, so when it is the
protected component that step is not needed. In every case exactly one inventory
object must be left at the end, otherwise the SPDM daemon makes no association
for that component.

The two lookups depend on two `configured_by` associations:

| Published by                                  | Association                                        | Used to                                                        |
| --------------------------------------------- | -------------------------------------------------- | -------------------------------------------------------------- |
| `mctpreactor`, today                          | MCTP endpoint to its MCTP record                   | find the SPDM record                                           |
| The service that creates the inventory object | inventory object to the record it was created from | find the protected inventory, through the reverse `configures` |

The second is the one dependency this mapping adds. It is needed only for
inventory objects that the mapping selects and that a service other than Entity
Manager creates.

The SPDM daemon does not assume that the services that create inventory start
before it. It looks again when objects or associations appear.

Two cases are outside this mapping. Endpoints behind an MCTP bridge are outside
it until they have an endpoint-to-record `configured_by` association. Components
known only from protocol discovery, with no Entity Manager configuration to
select, need a separate mapping.

#### Associations and Redfish

These are the associations for each responder. The Redfish column shows where
each one appears on the [ComponentIntegrity][rf-ci] or [TrustedComponent][rf-tc]
resource of the From object:

| Association             | From                        | To                                   | Redfish                     |
| ----------------------- | --------------------------- | ------------------------------------ | --------------------------- |
| `reporting`             | ComponentIntegrity          | Its TrustedComponent                 | `TargetComponentURI`        |
| `reported_by`           | TrustedComponent            | Its ComponentIntegrity               | `Links.ComponentIntegrity`  |
| `authenticating`        | ComponentIntegrity          | Each protected component             | `Links.ComponentsProtected` |
| `protecting`            | TrustedComponent            | Each protected component             | `Links.ComponentsProtected` |
| `contained_by`          | TrustedComponent            | The board that holds the SPDM record | The Chassis in the URI      |
| `integrated_into` (new) | Integrated TrustedComponent | The first protected component        | `Links.IntegratedInto`      |

`reported_by` is the reverse of `reporting`.

`integrated_into` names the device that an integrated RoT is built into. A
discrete RoT has none. The association, with the reverse name `integrating`, is
a proposed addition to phosphor-dbus-interfaces. Redfish requires
`IntegratedInto` for an integrated TrustedComponent, and the existing
associations cannot give it. `contained_by` gives the Chassis, for discrete and
integrated RoTs alike ([physical-topology]), and `protecting` lists every
protected component without order.
[Alternatives Considered](#alternatives-considered) explains why containment is
not used in its place.

#### Summary of cases

How much the record says depends on what the RoT protects:

| The RoT protects                          | Example                                                              | The record has                                                                                                | Integrated into           |
| ----------------------------------------- | -------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------- | ------------------------- |
| The device of its own board configuration | Integrated RoT of a management controller                            | No Protected property                                                                                         | The controller            |
| One component on its own board            | The network card above                                               | `ProtectedTargets`, and `ProtectedInventoryTypes` when several inventory objects are created from that record | The network adapter       |
| The device of another board configuration | Discrete RoT for a BMC that has its own board configuration `BMC_0`  | `ProtectedBoards: ["BMC_0"]`. Add `ProtectedTargets` when the component is a record on that board             | None, the RoT is discrete |
| More than one component                   | Integrated RoT of a controller that also protects the board `CPLD_0` | `ProtectedBoards: ["", "CPLD_0"]`. The empty first entry is the controller                                    | The controller            |

### Device Certificate

In OpenBMC, there is a
[certificate manager](https://github.com/openbmc/phosphor-certificate-manager),
which allows users to install or replace server/client certificates. However,
the existing certificates manager is designed for managing server/client
certificates for HTTPS/LDAP services. It's not suitable for device certificates.
Existing cert manager has several limitations:

- Each manager can only manage one certificate.
- Each manager assumes access to both the private key and the public key (e.g.,
  for completing CSR request).

Device certificates have different requirements:

- Device certificate manager manages several certificates for a group of
  devices, for example, four GPUs would have four certificates.
- Device certificate does not assume private key access. It is used for identity
  authentication only. It does not provide CSR signing services. The private key
  should never leave the device Root of Trust(RoT) component.

For device certificates, we only need to create/replace certificate objects, no
need for a global cert manager that "manages" the device certificates. SPDM
D-Bus daemon can simply talk to the devices, get the certificates from them, and
create D-Bus object for the certificates.

In Redfish, device certificates are under Chassis, and are accessible via
`/redfish/v1/Chassis/{ChassisId}/Certificates/`. Existing cert manager
constructs cert path following the pattern
`"/xyz/openbmc_project/certs/{type}/{endpoint}".` To comply with it, we propose
to put device certificates under
`/xyz/openbmc_project/certs/chassis/{ChassisId}/{certsId}`. So that for all
device certificates on a chassis, we can find those certificates with its
chassisId on D-Bus.

### BMCWeb Support

In BMCWeb, we need to add routes handler for `ComponentIntegrity`,
`TrustedComponent` and `Certificates`. The corresponding URI are specified as
follows according to Redfish spec:

- `/redfish/v1/ComponentIntegrity/`
- `/redfish/v1/Chassis/{ChassisId}/TrustedComponents/`

The TrustedComponent represents a ROT or TPM, and its associated certificates
should be represented as subordinate resources under TrustedComponent. For the
proper URI structure for certificates, please refer to the [Redfish Data Model
Specification][1]. Look for the CertificatesCollection row in the specification.

- `/redfish/v1/Chassis/{ChassisId}/TrustedComponents/{TrustedComponentId}/`
  `Certificates`
- `/redfish/v1/Chassis/{ChassisId}/TrustedComponents/{TrustedComponentId}/`
  `Certificates/{CertificateId}`

On the D-Bus Daemon side, we propose that the dbus objects are organized in the
following way:

- `/xyz/openbmc_project/component_integrity/{component_integrity_id}`
- `/xyz/openbmc_project/trusted_components/{trusted_component_id}`
- `/xyz/openbmc_project/certs/devices/{chassis_id}/{cert_id}`

In BMCWeb, we can reconstruct the following redfish URI by querying the
associated Chassis from the trusted component:

- `/redfish/v1/Chassis/{ChassisId}/TrustedComponents/{TrustedComponentId}`

## Alternatives Considered

Alternative way to manage device certificates would be modifying existing
[phosphor-certificate-manager](https://github.com/openbmc/phosphor-certificate-manager).

Device certificates management has two steps:

- Step 1: fetch device certificate by exchange SPDM messages with device.
- Step 2: create or update a dbus certificate object.

Step 1 can only be handled by the SPDM daemon. Step 2 is simple enough to be
handled by the D-Bus daemon, too. It would be a over-kill to modify existing
phosphor-certificate-manager for the sole purpose.

For the inventory mapping of MCTP responders, these alternatives were
considered:

- Describe the protected components in a separate record, either next to the
  responder record or on each protected device. The protection list of one RoT
  would then be spread over two or more records. This design keeps one record
  for each responder.
- Require all three arrays, with explicit empty strings. This gives every record
  the same properties and catches a forgotten array, but most records would
  carry arrays that hold only empty strings.
- Point `contained_by` of an integrated RoT at the device it is built into and
  follow the containment of that device to the Chassis. This needs no new
  association, and an earlier review of the TrustedComponent interface preferred
  containment. The TrustedComponent would then have no Chassis, and no Redfish
  resource, until another service publishes that device and its containment.
- Describe each protected component as one object in a list. This keeps the
  values of one component together. Entity Manager publishes each object of a
  list on its own numbered D-Bus interface today. The three string arrays keep
  the selection on one interface, which the SPDM daemon reads with a generated
  D-Bus client, as it does for the `SpdmTcpResponder` record.
- Let Entity Manager create the associations from matching ports, as an earlier
  review of the SPDM record suggested. Ports connect two Entity Manager
  configurations today. Using them here would need Entity Manager to reach a
  record inside a board and objects that the SPDM daemon and other services own.
- Name the device an integrated RoT is built into with a property of its own
  instead of the first entry. This states the relationship explicitly, but
  repeats the selection of the first protected component.

## Impacts

This change will:

- Create a SPDM daemon that can do SPDM attestation for SPDM-capable devices
  over PCIe DOE, MCTP or TCP.
- Add `ComponentIntegrity` and `TrustedComponent` related D-Bus interfaces in
  phosphor-dbus-interfaces.
- Extend existing certificate service in BMCWeb.
- Add SPDM support in BMCWeb with new routes.

### Organizational

This repository requires creating a new repository for the SPDM daemon. In
addition, the following repositories are expected to be modified to execute this
design:

- <https://github.com/openbmc/bmcweb>
- <https://github.com/openbmc/phosphor-dbus-interfaces>

## Testing

### Unit Test

For the BMCWeb changes, unit test can be done with the Redfish Service
Validator.

For the SPDM Attestation D-Bus Daemon, unit tests should cover the following
cases:

- Set up a transport layer connection with the device.
- Unit tests should be implemented to ensure the infrastructure functions
  reliably, regardless of the underlying transport mechanisms.
- SPDM connection setup, including get capabilities, negotiate algorithms.
- Get device certificates from device and create D-Bus object.
- `SPDMGetSignedMeasurements` method test.
- Enumerate trusted component D-Bus objects and check properties and
  associations.
- Enumerate component integraty D-Bus objects and check properties and
  associations.

### Integration Test

BMCWeb/D-Bus Daemon integration test should cover the following type of
requests:

- Get a collection of `ComponentIntegrity` resources.
- Get a collection of `TrustedComponent` resource.
- Get properties of a `ComponentIntegrity` resources.
- Get properties of a `TrustedComponent` resource.
- Follow the resources link to get the device certificates and verify that they
  are correctly structured as subordinate resources under the TrustedComponent.
- Get properties of `Certificate` resource and verify responses match with leaf
  certificate in the certificate chain.
- Call Action on the `ComponentIntegrity` resource to get measurements.

[1]:
  https://www.dmtf.org/sites/default/files/standards/documents/DSP0268_2024.4.html#resource-collection-uris-in-redfish-v16-and-later
[physical-topology]:
  https://github.com/openbmc/docs/blob/master/designs/physical-topology.md
[platform-mctp]:
  https://github.com/openbmc/entity-manager/blob/master/schemas/platform/mctp.json
[rf-tc]: https://redfish.dmtf.org/schemas/v1/TrustedComponent.v1_5_0.json
[rf-ci]: https://redfish.dmtf.org/schemas/v1/ComponentIntegrity.v1_5_0.json
