# Redfish SPDM Attestation Support

Author: Zhichuang Sun

Other contributors: Jerome Glisse, Ed Tanous

Created: June 27, 2023

## Problem Description

Redfish added schema for
[ComponentIntegrity](https://redfish.dmtf.org/redfish/schema_index),
which allows users to use [SPDM](https://www.dmtf.org/standards/spdm) or
[TPM](https://trustedcomputinggroup.org/resource/trusted-platform-module-tpm-summary/)
to authenticate device identity, hardware configuration and firmware integrity.
It would be useful to add SPDM attestation support in BMCWeb, which provides
unified interface for device security attestation in data centers.

This design focuses on SPDM.

## Background and References

SPDM(Security Protocols and Data Models) is a spec published by
[DMTF](https://www.dmtf.org/sites/default/files/standards/documents/DSP0274_1.3.0.pdf).
It is designed for secure attestation of devices. GitHub repo
[libspdm](https://github.com/DMTF/libspdm) provides an
open-source implementation of the SPDM protocol. Redfish Schema
[ComponentIntegrity](https://redfish.dmtf.org/schemas/v1/ComponentIntegrity.v1_2_1.json)
adds support for doing SPDM-based device attestation over Redfish API.

## Requirements

This feature aims at supporting SPDM attestation through Redfish API and
providing system administrators and operators an easy way to remotely verify
the identity and integrity of devices.

This design includes:
  - New D-Bus interfaces for Redfish resources `ComponentIntegrity` and
    `TrustedComponent`;
  - A reference design for SPDM Attestation D-Bus Daemon, demonstrating
    how to fetch the resource properties over D-Bus;
  - BMCWeb changes for supporting the above Redfish resources;

## Proposed Design

### Attestation related D-Bus Interfaces

Redfish provides resource `ComponentIntegrity` to support attestation.
It defines properties like the status (enabled or not), security protocol
type, type version, update time and so on. `ComponentIntegrity` defines
an Action `SPDMGetSignedMeasurements` which allows a remote user to fetch
signed measurements from the device.

`ComponentIntegrity` also defines properties that are links to other Redfish
resources, including the target component of this integrity report, components
that are being protected, requester/responder identity certificates, and so on.
All these properties are implemented as associations in D-Bus interfaces.

In D-Bus, we propose three new interfaces to support device attestation:
  - `Attestation.ComponentIntegrity`
  - `Attestation.IdentityAuthentication`
  - `Attestation.MeasurementSet`

`Attestation.ComponentIntegrity` provides integrity information
including the protocol to measure the integrity,
last updated time, integrity measurement enablement status, and so on.
Associations have been added to this interface, including a link
to the trusted component that the integrity object is reporting,
and a link to the systems that the integrity object is protecting.

`Attestation.IdentityAuthentication` provides identity verification
information. Two associations have been defined for it to link to
the requester and the responder certificates.

`Attestation.MeasurementSet` defines the dbus method to get SPDM measurements.
It can be extended to add other methods for measurements in the future.

The proposed Phosphor D-Bus Interfaces is here:
[component-integrity](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/64354).

### TrustedComponent related D-Bus Interfaces

Redfish defines
[TrustedComponent](https://redfish.dmtf.org/schemas/v1/TrustedComponent.v1_1_0.json).
`TrustedComponent` represents a trusted hardware component. It is linked to
the `ComponentIntegrity` resource who reports its integrity information.
`TrustedComponent` defines many properties that used to identify the
component, including Name, Type, Model, Part Number, SKU, Serial Number,
UUID, Firmware Version, and so on.

In D-Bus, properties like Model, Part Number, Serial Number have been defined
in `Inventory.Decorator.Asset` interface. UUID has been defined in
`Common.UUID` interface. The only missing property is `SKU`.
The proposal is to add `SKU` to `Inventory.Decorator.Asset` as it is just like
`Part Number` that used by vendor to track the stocking of a unit.

`TrustedComponent` resource also defines links to other Redfish
resources, including device identity certificates, active software image,
component integrity, protected components, and so on.
All these properties can be implemented as associations in D-Bus.

We propose to add one Inventory interface for TrustedComponent, namely
`Inventory.Item.TrustedComponent`. This interface defines properties that are
unique to TrustedComponent, like
AttachmentType, which gives information on whether this trusted compoennt is
integrated into the device or is discrete from the device.
Associations are defined for this interface as mentioned above.

A trusted component inventory object can implement
`Inventory.Item.TrustedComponent`, `Inventory.Decorator.Asset`, `Common.UUID`
to expose the information defined in the Redfish resource. Firmware
Version can be retrieved from the associated software object.

The proposed Phosphor D-Bus Interfaces for `TrustedComponent` is here:
[trusted-component](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/64355).

### SPDM Attestation D-Bus Daemon

As we know, BMCWeb provides SPDM Attestation support over Redfish.
We will add routes in the BMCWeb for ComponentIntegrity and TrustedComponent
to support it.
But BMCWeb collects the information from D-Bus.
The SPDM Attestation D-Bus Daemon does the actual work.

It would be ideal to have a generic D-Bus daemon dedicated for SPDM
attestation. However, the D-Bus daemon will need to manage the
trusted component D-Bus object, the component integrity D-Bus object, and
the associations created between them and other device-dependent
D-Bus objects, like what components they are protecting.

Besides, SPDM protocol needs to be binded to a transport layer protocol, which
transmits SPDM messages between the BMC and the device.
The transport layer protocol can be MCTP, PCIe-DOE, or other protocols.
For MCTP, the lower physical layer can be PCI-VDM, SMBus/I2C, Serial and so on.
Setting up the transport layer connection is also device-specific. Sometimes,
only the device D-Bus agent knows the nuance of how to make the connection
work efficiently, e.g., using a OEM-specific way to multiplex the use of a
connection.

Given the above reasons,
we do not intend to provide a generic D-Bus daemon for SPDM attestation.
For this design doc, we will only provide the generic workflow of a typical
SPDM D-Bus daemon. In the future, we plan to provide a library
to help with the SPDM D-Bus daemon development.

A reference D-Bus Daemon workflow would be like this:

1. Create and initialize D-Bus object for TrustedComponent with device
   specific information.
2. Create and initialize D-Bus object for ComponentIntegrity with device
   specific information.
3. Create the associations between the above objects and associations with
   other objects, e.g., protected components, active software images;
4. Set up a connection between the BMC and the device;
5. Initialize the SPDM context on top of the connection.
6. Exchange SPDM messages to get device certificates.
7. Create device certificate objects and create certificate associations for
   trusted component object and component integrity object.
8. Wait on D-Bus and serve any runtime SPDMGetSignedMeasurement requests.

### Device Certificate

In OpenBMC, there is already a
[certificate manager](https://github.com/openbmc/phosphor-certificate-manager),
which allows users to install or replace server/client certificates. However, the
existing certificates manager is designed for managing server/client
certificates for HTTPS/LDAP services. It's not suitable for
device certificates. Existing cert manager has several limitations:
- Each manager can only manage one certificate;
- Each manager assumes access to both the private key and the public key
  (e.g., for completing CSR request);

Device certificates has quite different requirements:
- Device certificate manager manages several certificates for a
  group of devices, for example, four GPUs would have four
  certificates.
- Device certificate does not assume private key access. It is used for
  identity authentication only. It does not provide CSR signing services. The
  private key should never leave the device Root of Trust(RoT) component.

For device certificates, we only need to create/replace certificate objects,
no need for a global cert manager that "manages" the device certificates.
Each SPDM D-Bus daemon can simply talk to the devices, get the
certificates from them, and create D-Bus object for the certificates.

Existing cert manager constructs cert path following the pattern
`"/xyz/openbmc_project/certs/{type}/{endpoint}".`
To comply with it, we propose a new certificate
type called `systems` and construct device cert path with
`/xyz/openbmc_project/certs/systems/{systemId}/{certsId}`.

Note, this will be a flat structure for the certificate objects. For a
single-system platform, all device  certificates will be in one directory.
For muti-systems platform, we make sure the resource shared by both system
also appear in both system-specific directories.
This is also consistant with how Redfish managing system/device certificates,
`/redfish/v1/Systems/{SystemId}/Certificates/{CertId}`.

### BMCWeb Support

In BMCWeb, we need to add routes handler for
ComponentIntegrity, TrustedComponent and Certificates. The
corresponding URI are specified as follows according to Redfish spec:

- `/redfish/v1/ComponentIntegrity/`
- `redfish/v1/Chassis/{ChassisId}/TrustedComponent/`
- `/redfish/v1/Systems/{SystemId}/Certificates/`

On the D-Bus Daemon side, we propose that the dbus objects are
organized in the following way:

- `/xyz/openbmc_project/ComponentIntegrity/{ComponentIntegrityId}`
- `/xyz/openbmc_project/TrustedComponent/{TrustedComponentId}`
- `/xyz/openbmc_project/certs/systems/{SystemId}/{CertId}`

In BMCWeb, we can reconstruct the following redfish URI by querying the
associated Chassis from the trusted component:

- `/redfish/v1/Chassis/{ChassisId}/TrustedComponent/{TrustedComponentId}`

## Alternatives Considered
Alternative way to manage device certificates would be modifying existing
[phosphor-certificate-manager](https://github.com/openbmc/phosphor-certificate-manager).

Device certificates management has two steps:
- Step 1: fetch device certificate by exchange SPDM messages with device;
- Step 2: create/update a dbus certificate object;

Step 1 can be only be done by each device specific SPDM D-Bus daemon. Step 2
is simple enough to be handled within the D-Bus daemon. Sounds like a over-kill
to modify existing phosphor-certificate-manager for the sole purpose.

## Impacts

This change will:
- Extend existing certificate service in BMCWeb;
- Add SPDM support in BMCWeb with new routes;
- Add `ComponentIntegrity` and `TrustedComponent` related D-Bus interfaces in
  phosphor-dbus-interfaces.

### Organizational
This repository does not require a new repository so far. In the future, we
plan to create a new repository for the attestation library. For now, we focus
on the D-Bus interfaces and BMCWeb changes to support SPDM attestation.
The following repositories are expected to be modified to execute this design:
  - https://github.com/openbmc/bmcweb
  - https://github.com/openbmc/phosphor-dbus-interfaces

## Testing

### Unit Test
For the BMCWeb changes, unit test can be done with the Redfish Service Validator.

For the SPDM Attestation D-Bus Daemon, unit tests should cover the following
cases:
- Set up a transport layer connection with the device;
- SPDM connection setup, including get capabilities, negotiate algorithms;
- Get device certificates from device and create D-Bus object;
- SPDMGetSignedMeasurements method test;
- Enumberate trusted component D-Bus objects and check properties and
  associations;
- Enumberate component integraty D-Bus objects and check properties and
  associations;

### Integration Test

BMCWeb/D-Bus Daemon integration test should cover the following type of requests:
- Get a collection of `ComponentIntegrity` resources;
- Get a collection of `TrustedComponent` resources;
- Get properties of a `ComponentIntegrity` resources;
- Get properties of a `TrustedComponent` resources;
- Follow the resouces link to get the device certificates;
- Call Action on the `ComponentIntegrity` resource to get measurements.
