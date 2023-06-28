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
unified interface for device security attestation in data center.

This design doc focuses on SPDM.

## Background and References

SPDM(Security Protocols and Data Models) is a SPEC published by
[DMTF](https://www.dmtf.org/about). It is designed for secure attestation of
devices. GitHub repo [libspdm](https://github.com/DMTF/libspdm) provides an
open-source implementation of the SPDM protocol. Redfish Schema
[ComponentIntegrity](https://redfish.dmtf.org/schemas/v1/ComponentIntegrity.v1_2_1.json)
adds support for doing SPDM-based device attestation over Redfish API.

## Requirements

This feature aims at supporting SPDM attestation through Redfish API and 
providing system administrators and operators an easy way to remotely verify
the identity and integrity of devices.

This design doc includes:
  - new D-Bus interfaces for Redfish resources `ComponentIntegrity` and
    `TrustedComponent`;
  - a reference design for SPDM Attestation D-Bus Daemon, demonstrating
    how to fetch the resource properties over D-Bus;
  - BMCWeb routes for the above Redfish resources;

## Proposed Design

### D-Bus Interfaces

Redfish resource ComponentIntegrity defines properties that provides basic
information, which includes the status (enabled or not), security protocol
type, type version, update time and so on. ComponentIntegrity also defines
an Action `SPDMGetSignedMeasurements` which allow a remote user to fetch
signed measurements from the device. We need to add corresponding
D-Bus interfaces to cache/fetch the information. 

`ComponentIntegrity` also defines properties that are links to other Redfish
resources, including the target component of this integrity report, components
that are being protected, requester/responder identity certificates, and so on.
All these properties are implemented as associations in D-Bus interfaces.

The proposed Phosphor D-Bus Interfaces is here:
[component-integrity](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/64354).

Besides, ComponentIntegrity is also associated with another Redfish resource
called
[TrustedComponent](https://redfish.dmtf.org/schemas/v1/TrustedComponent.v1_1_0.json).
`TrustedComponent` resource represents a trusted hardware component. It
defines many properties that used to identify the component, includes name,
type, model, part number, SKU, serial number, UUID, firmware version, and so
on. Most of the property have been implemented in the
[FRU](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Inventory/Source/PLDM/FRU.interface.yaml) 
and
[Software/Version](//github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_projecti/Software/Version.interface.yaml)
interface.

`TrustedComponent` also defines properties that are links to other Redfish
resources, including device identity certificate, active software image,
component integrity, protected components, and so on.
All these properties are implemented as associations in D-Bus interfaces.

The proposed Phosphor D-Bus Interfaces for `TrustedComponent` is here:
[trusted-component](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/64355).

### SPDM Attestation D-Bus Daemon

A reference D-Bus Daemon workflow would be like this:

1. Create and initialize D-Bus objects for TrustedComponent,
   ComponentIntegrity;
2. Initialize the associations between the objects;
3. Initialize the SPDM context for the trusted component, and get device
   certificates. 
4. Create device certificate objects and create certificate associations for
   trusted component object and component integrity object. 
5. Start a loop and serve any runtime SPDMGetSignedMeasurement requests.

### Device Certificate Management

Certificate schema is defined by Redfish SPEC. BMCWeb support certificate
management for HTTPS Server, LDAP Client/Server and Certificate Authority. 
D-Bus interfaces for Certificate have already been added to
phosphor-dbus-interfaces.

In OpenBMC there is certificate manager, which 
allows users to install or replace server/client certificates. However, the
existing certificates manager is designed for managing server/client
certificates for HTTPS/LDAP services. It does not meet our use case for
managing device certificates. Existing cert manager has several limitations:
- Each manager can only manage one certificate;
- Each manager has to start with a private key and a public key;

Device certificates managemen has quite different requirements:
- Device certificate manager manages several certificates for a
  group of devices, for example, four GPUs would have four
  certificates.
- Device certificate management does not need the private key. It is used for
  identity authentication only and does not provide CSR signing services. The
  private key should never leave the device Root of Trust(RoT) component.

For device certificate management, we only need
to create/replace certificate objects without requiring a private key.
Each SPDM D-Bus daemon knows how to talk to managed devices and get the
certificates from them. Thus we don't need a global device cert manager to
manage all device certificates.

Existing cert manager construct cert path with 
`"/xyz/openbmc_project/certs/{type}/{endpoint}".`
To comply with the convention, we propose to extend it with a new certificate
type called `systems` and construct device cert path with 
`/xyz/openbmc_project/certs/systems/{systemId}/{certsId}`.
Note, this is consistant with how Redfish managing system/device certificates,
`/redfish/v1/Systems/{SystemId}/Certificates/{CertId}`.

### BMCWeb Support

Inside BMCWeb, we also need to add a route handler for
ComponentIntegrity, TrustedComponent and Certificates. The
corresponding URI are specified as follows according to Redfish SPEC:

- `/redfish/v1/ComponentIntegrity/`
- `redfish/v1/Chassis/{ChassisId}/TrustedComponent/`
- `/redfish/v1/Systems/{SystemId}/Certificates/`

On the D-Bus Daemon side, the proposal is to assume the dbus objects are
organized in the following way:

- `/xyz/openbmc_project/ComponentIntegrity/{ComponentIntegrityId}`
- `/xyz/openbmc_project/Chassis/{ChassisId}/TrustedComponent/{TrustedComponentId}`
- `/xyz/openbmc_project/certs/systems/{SystemId}/{CertId}`

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
- extend existing certificate service in BMCWeb;
- add SPDM support in BMCWeb with new routes;
- add ComponentIntegrity and TrustedComponent related D-Bus interfaces in
  phosphor-dbus-interfaces.

## Testing

This can be tested using the Redfish Service Validator.
