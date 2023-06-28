# Redfish SPDM Attestation Support

Author: Zhichuang Sun

Other contributors: Jerome Glisse, Ed Tanous

Created: June 27th, 2023 Last Updated: Oct 30th, 2023

## Problem Description

Redfish added schema for
[ComponentIntegrity](https://redfish.dmtf.org/redfish/schema_index), which
allows users to use [SPDM](https://www.dmtf.org/standards/spdm) or
[TPM](https://trustedcomputinggroup.org/resource/trusted-platform-module-tpm-summary/)
to authenticate device identity, hardware configuration and firmware integrity.
It would be useful to add SPDM attestation support in BMCWeb, which provides
unified interface for device security attestation in data centers.

This design focuses on SPDM.

## Background and References

SPDM(Security Protocols and Data Models) is a spec published by
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
  `TrustedComponent`;
- BMCWeb changes for supporting the above Redfish resources;
- Design for SPDM Attestation D-Bus Daemon, demonstrating how to fetch the
  attestation results over D-Bus;

## Proposed Design

### Attestation related D-Bus Interfaces

There are three type of information we will need from an attestation daemon on
D-Bus:

1.  Basic information, like attestation protocol, enablement status, update
    timestamp, etc.;
2.  Identity information, e.g., device identity certificates;
3.  Measurments information, e.g., measurements of the component firmware,
    hardware configuration, etc.;

So far, D-Bus lacks interfaces defined for attestation purpose. Thus, we propose
three new interfaces:

- `Attestation.ComponentIntegrity`
- `Attestation.IdentityAuthentication`
- `Attestation.MeasurementSet`

`Attestation.ComponentIntegrity` provides basic component integrity information,
including the protocol to measure the integrity, last updated time, attestation
enablement status, and so on. Associations have been added to this interface,
including a link to the trusted component that the component integrity object is
reporting, and a link to the systems that the component integrity object is
protecting.

`Attestation.IdentityAuthentication` provides identity verification information.
Two associations have been defined for it to link to the requester and the
responder certificates.

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
  integrated into the device or is discrete from the device;

A `TrustedComponent` is typically associated with other hardware components
which it is protecting. It should also be associated with the component
integrity object reported by this `TrustedComponent`.

The proposed Phosphor D-Bus Interfaces for `TrustedComponent` is here:
[trusted-component](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/64355).

### SPDM Attestation D-Bus Daemon

[Experimental support for SPDM exists for
`bmcweb`](https://github.com/openbmc/bmcweb/compare/master...sunzc:bmcweb:spdm),
which adds routes in the BMCWeb for `ComponentIntegrity` and `TrustedComponent`
to support it. But BMCWeb collects the information from D-Bus. The SPDM
Attestation D-Bus Daemon does the actual work.

SPDM protocol needs to be bound to a transport layer protocol, which transmits
SPDM messages between the BMC and the device. The transport layer protocol can
be MCTP, PCIe-DOE, or even TCP socket. For MCTP, the lower physical layer can be
PCI-VDM, SMBus/I2C, and so on. Note, [`libspdm` already provides transport
layer protocol
binding](https://github.com/DMTF/libspdm/blob/main/include/internal/libspdm_common_lib.h#L445-L446)
with message encoding/decoding support. The device send/receive function is
left for SPDM daemon to implement If the transport layer is using standard
MCTP or PCIe-DOE, setting up the transport layer connection could be easy. In
this design, we only consider SPDM over standard MCTP and PCIe-DOE connection.

For SPDM-over-MCTP, SPDM daemon can query the mctpd for information about MCTP
endpoint, including the endpoint id(eid) and upper layer responder, and create a
connection only for endpoint that has SPDM as its upper layer responder.

For SPDM-over-PCIe-DOE, SPDM daemon need the PCIe device BDF info to handle
DOE mailbox discovery. Given that not all PCIe devices support DOE support
SPDM, we can not query about whether a DOE capable device supports SPDM.
Therefore, we need a way to pass the device info to the dameon.
However, PCIe device BDF(Bus:Device:Function) info are dynamically assigned
during system boot. The same device may get assigned different BDF on
different machine. What the daemon needs should be the PCIe device ID, which
is identified by `VendorId:DeviceId`. For the convenience of configuration, we
should pass PCIe device ID to the daemon, so that the daemon can enumerate all
the PCIe devices and find the matching devices by their device ID.
There are different ways to pass PCIe device ID info to the dameon, e.g.,
configuration file, command line parameters, or Entity Manager exposes schema. 

SPDM daemon talks to the trusted component to get certificates or measurements
of the protected devices. There should be an Entity Manager configuration file
created for each of the trusted component. Part of the configuration should
include information like MCTP bus address, or PCIe device ID. For the
information provided by the configuration file, if it is MCTP devices, we can
query the `mctpd` to double check whether those devices are enumerated by
mctpd correctly.

A reference D-Bus Daemon workflow would be like this:

0. (Probing phase) Entity Manager will parse the configuration file for
    trusted components; mctpd finish discovery of mctp endpoints. These are
    prerequisites for SPDM daemon.
1. Check transport layer protocol. For MCTP, it queries mctpd to gather all eids
   that support SPDM; For PCIe-DOE, it performs DOE mailbox discovery with the
   PCIe device ID.
2. For each endpoint, which could be MCTP or PCIe-DOE, SPDM daemon query Entity
   Manger for the matching trusted component configuration. It then creates and
   initializes the corresponding D-Bus object for `TrustedComponent` and
   `ComponentIntegrity` with device specific information.
3. Create the associations between the above objects and associations with other
   objects, e.g., protected components, active software images;
4. Set up a connection between the BMC and each SPDM-capable device;
5. Initialize the SPDM context on top of the connection.
6. Exchange SPDM messages to get device certificates.
7. Create device certificate objects and create certificate associations for
   trusted component object and component integrity object.
8. Wait on D-Bus and serve any runtime `SPDMGetSignedMeasurement` requests.

### Device Certificate

In OpenBMC, there is a
[certificate manager](https://github.com/openbmc/phosphor-certificate-manager),
which allows users to install or replace server/client certificates. However,
the existing certificates manager is designed for managing server/client
certificates for HTTPS/LDAP services. It's not suitable for device certificates.
Existing cert manager has several limitations:

- Each manager can only manage one certificate;
- Each manager assumes access to both the private key and the public key (e.g.,
  for completing CSR request);

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
`/xyz/openbmc_project/certs/devices/{ChassisId}/{certsId}`. So that for all
device certificates on a chassis, we can find those certificates with its
chassisId on D-Bus.

### BMCWeb Support

In BMCWeb, we need to add routes handler for `ComponentIntegrity`,
`TrustedComponent` and `Certificates`. The corresponding URI are specified as
follows according to Redfish spec:

- `/redfish/v1/ComponentIntegrity/`
- `/redfish/v1/Chassis/{ChassisId}/TrustedComponent/`
- `/redfish/v1/Chassis/{ChassisId}/Certificates/`

On the D-Bus Daemon side, we propose that the dbus objects are organized in the
following way:

- `/xyz/openbmc_project/ComponentIntegrity/{ComponentIntegrityId}`
- `/xyz/openbmc_project/TrustedComponent/{TrustedComponentId}`
- `/xyz/openbmc_project/certs/devices/{ChassisId}/{CertId}`

In BMCWeb, we can reconstruct the following redfish URI by querying the
associated Chassis from the trusted component:

- `/redfish/v1/Chassis/{ChassisId}/TrustedComponent/{TrustedComponentId}`

## Alternatives Considered

Alternative way to manage device certificates would be modifying existing
[phosphor-certificate-manager](https://github.com/openbmc/phosphor-certificate-manager).

Device certificates management has two steps:

- Step 1: fetch device certificate by exchange SPDM messages with device;
- Step 2: create/update a dbus certificate object;

Step 1 can be only be done by each device specific SPDM D-Bus daemon. Step 2 is
simple enough to be handled within the D-Bus daemon. Sounds like a over-kill to
modify existing phosphor-certificate-manager for the sole purpose.

## Impacts

This change will:

- Extend existing certificate service in BMCWeb;
- Add SPDM support in BMCWeb with new routes;
- Add `ComponentIntegrity` and `TrustedComponent` related D-Bus interfaces in
  phosphor-dbus-interfaces.

### Organizational

This repository does not require a new repository so far. In the future, we plan
to create a new repository for the SPDM daemon. For now, we focus on the D-Bus
interfaces and BMCWeb changes to support SPDM attestation. The following
repositories are expected to be modified to execute this design:

- https://github.com/openbmc/bmcweb
- https://github.com/openbmc/phosphor-dbus-interfaces

## Testing

### Unit Test

For the BMCWeb changes, unit test can be done with the Redfish Service
Validator.

For the SPDM Attestation D-Bus Daemon, unit tests should cover the following
cases:

- Set up a transport layer connection with the device;
- SPDM connection setup, including get capabilities, negotiate algorithms;
- Get device certificates from device and create D-Bus object;
- `SPDMGetSignedMeasurements` method test;
- Enumberate trusted component D-Bus objects and check properties and
  associations;
- Enumberate component integraty D-Bus objects and check properties and
  associations;

### Integration Test

BMCWeb/D-Bus Daemon integration test should cover the following type of
requests:

- Get a collection of `ComponentIntegrity` resources;
- Get a collection of `TrustedComponent` resources;
- Get properties of a `ComponentIntegrity` resources;
- Get properties of a `TrustedComponent` resources;
- Follow the resouces link to get the device certificates;
- Call Action on the `ComponentIntegrity` resource to get measurements.
