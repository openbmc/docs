# Certificate Revocation List on BMC

Author: Nan Zhou (nanzhoumails@gmail.com)

Created: 02/25/2022

## Problem Description

This design is to add management interfaces for certificate revocation list in
OpenBMC.

## Background and References

A certificate revocation list (CRL) is a list of digital certificates that have
been revoked by the issuing certificate authority (CA) before their actual or
assigned expiration date. In Google, there are use cases that BMC needs to
install CRLs to the Redfish server, so that clients with revoked certificates
will be rejected in TLS handshake. Supporting CRL is also recommended in most
applications.

Current OpenBMC certificate management architecture contains two main
components.

1. [phosphor-certificate-manager](https://github.com/openbmc/phosphor-certificate-manager)
   owns certificate objects and implements management interfaces; currently
   there are three types of certificates supported: client, server, and
   authority.

2. [BMCWeb](https://github.com/openbmc/bmcweb): the Redfish front-end which
   translates certificate objects into Redfish resources. BMCWeb is also a
   consumer of these certificates; it uses certificates in its TLS handshake.

DMTF doesn't support CRLs yet in the Redfish spec. Adding them is WIP. See
[this discussion](https://redfishforum.com/thread/618/resource-certificate-revocation-list?page=1&scrollTo=2173).
Google doesn't plan on using Redfish interfaces to manage certificates and CRLs.
Instead, Google has a dedicated daemon for credentials installation, and this
daemon interacts with the OpenBMC certificate management architecture via DBus
APIs.

## Requirements

OpenBMC supports management interface for CRLs:

1. clients shall be able to install/delete/replace CRLs via DBus APIs
2. whenever CRLs change, the certificate management system shall notify
   consumers which use old CRLs to refresh with the newly given CRLs
3. other daemons, e.g., BMCWeb shall consume CRLs the same way as existing
   authority/server/client certificates, that is, via file path or directory
   determined at compile time.

## Proposed Design

### phosphor-dbus-interfaces

We propose to introduce a new interface `CRL` in
[Certs](https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/yaml/xyz/openbmc_project/Certs).

Because no Redfish spec is available, we propose the only attribute of the
interface to be `CRLString`, which contains the PEM encoded CRL. We can add more
attributes as needed in the future.

### phosphor-certificate-manager

We propose to add a new type of certificate-manager (CRL-manager) to the
existing three types of Manager.

The CRL-manager will implement the following common interfaces:

1. [InstallAll](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Certs/InstallAll.interface.yaml):
   install multiple CRLs and notify consumers. The notification process is the
   existing behaviour which phosphor-certificate-manager uses to tell consumers
   to reload newly installed credentials.

2. [ReplaceAll](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Certs/ReplaceAll.interface.yaml):
   replace all existing CRLs with multiple new CRLs and notify consumers

3. [DeleteAll](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Collection/DeleteAll.interface.yaml):
   delete all existing CRLs and notify consumers

### BMCWeb

We propose to introduce CRLs into BMCWeb's SSL Context. Whenever BMCWeb reloads,
it not only refreshes authority and server certificates, but also CRLs. Example
codes can be found in many opensource projects, e.g., this
[snippet](https://github.com/Icinga/icinga2/blob/master/lib/base/tlsutility.cpp#L338).

## Alternatives Considered

We can model the whole CRLs list as a single object, but that's not aligned with
the existing authorities list design.

## Impacts

1. New DBus interfaces
2. More complete security support

## Testing

Add new unit tests in phosphor-certificate-manager.

Manual integration tests: install CRLs and verify clients' revoked certificates
are rejected.
