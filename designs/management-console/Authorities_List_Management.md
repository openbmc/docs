# Authorities List Management

Author: Nan Zhou (nanzhoumails@gmail.com)

Created: 12/01/2021

## Problem Description

There are use cases where a system has multiple root certificates installed to
verify different clients. For example, In Google, a trust bundle file ( which is
a list of authorities) is installed on BMC for mTLS authentication.

The current phosphor-certificate-manager doesn't have good support to manage
multiple root certificates:

1. It only allows replacing a single Authority object in dbus; however, Google's
   use case requires bulk replacement (see the ReplaceAll interface below)

2. It only extracts the first certificate given a PEM encoded file with multiple
   certs; however, Google's trust bundle file contains multiple PEM encoded
   certificates

## Requirements

Phosphor-certificate-manager (only the Authority Manager) and BMCWeb will
support authorities list:

1. Bulk Installation: given a PEM file with multiple root certificates, it
   validates & installs all of them and returns a list of created objects

2. Bulk Replacement: given a PEM file with multiple root certificates, it will
   firstly delete all current root certificates and redo the installation

3. Redfish: BMCWeb will export all authorities as Redfish Certificate

4. Recovery at boot up: when the phosphor-certificate-manager gets instantiated,
   if it finds a authorities list in the installation path, it will recover from
   the list via a bulk installation

5. Atomic: Bulk Installation and Bulk Replacement are atomic; that is, if there
   is an invalid certificate in the list, the service won't install any of the
   certificates in the list

## Proposed Design

We propose two new interfaces:

1. InstallAll
2. ReplaceAll

### xyz.openbmc_project.Certs.InstallAll

When certificate type is Authority, rather than just extract the first
certificate, we will iterate through each certificate, validate it, create
corresponding object in DBus, dump individual certificates into PEM files in the
installation path, creates alias according to subject hash (requirements from
boost's `ssl_context`) for each certificate, and finally copy the PEM file to
the installation path(the PEM file will have a fixed name)

We return all created object paths as a vector of strings.

For other types of certificates (server & client), the service throws a NOT
ALLOWED error.

### xyz.openbmc_project.Certs.ReplaceAll

The new interface contains a ReplaceAll method which takes a path to the input
PEM file.

The certificate manager will implement the new ReplaceAll interface. Upon
invocation, it deletes all current authority objects, takes the input PEM, and
redo the installation.

For other types of certificate manager (server & client), the service throws a
NOT ALLOWED error.

### xyz.openbmc_project.Certs.Replace

No changes. Individual authority certificate can still be replaced respectively.
It only extracts the first certificate even if the PEM contains multiple root
certificates.

## Impacts

None besides new APIs are added

## Alternatives Considered

We can also create a trust bundle interface (instead of using multiple
Certificates) and implement its standalone manager daemon. It has less impact in
existing codes. However, trust bundle isn't in BMCWeb, neither in Redfish
schema.

## Testing

Enhance existing unit tests in phosphor-certificates-manager to test bulk
installation and replacements.
