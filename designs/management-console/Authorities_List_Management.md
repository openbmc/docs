# Authorities List Management

Author:
  Nan Zhou (nanzhoumails@gmail.com)

Primary assignee:
  Nan Zhou

Created:
  12/01/2021

## Problem Description

Authorities list is a list of X.509 root certificates. It is totally valid for a
system to have multiple root certificates installed to verify different clients.

The current phosphor-certificate-manager doesn't have good support to manage
multiple root certificates: 

1. It only allows replacing a single Authority object in dbus; however, Google's use
case requires bulk replacement (see the ReplaceAll interface below)
2. It only extracts the first certificate given a PEM encoded file with multiple
certs; however, Google's trust bundle file contains multiple PEM encoded certificates

## Requirements

Phosphor-certificate-manager and BMCWeb will support authorities list,

1. Installation: given a PEM file with multiple root certificates, it will iterate
through each certificate, validate it, create corresponding object in DBus, and
creates alias according to subject hash (requirements from boost's `ssl_context`)
2. Bulk Replacement: given a PEM file with multiple root certificates, it will firstly
delete all current root certificates and redo the installation
3. Redfish: BMCWeb will export all authorities as Redfish Certificate

## Proposed Design

We propose to change the behavior of Install for Authority Manager.
We also propose a new interface `ReplaceAll` 
current interface in phosphor-certificate-manager.

### xyz.openbmc_project.Certs.Install

The interface itself doesn't need any change.

The only change is that when certificate type is Authority, rather than just extract
the first certificate, we iterate through every certificate in the given PEM
file and install it.

### xyz.openbmc_project.Certs.Replace

No changes. Individual authority certificate can still be replaced respectively. And it only extracts
the first certificate even if the PEM contains multiple root certificates.

### xyz.openbmc_project.Certs.ReplaceAll

ReplaceAll contains a ReplaceAll method which takes a path to the input PEM file.

The certificate manager will implement the new ReplaceAll interface. Upon invocation, it
deletes all current authority objects, takes and input PEM and redo the installation.

For other types of certificate manager (server & client), the behavior is the same as individual
cert replacement.


## Impacts
- Create new interface ReplaceAll
- Change the behavior of Authority Manager for Install
- Implement the ReplaceAll interface in Authority Manager

## Testing
- Change existing unit tests to test bulk installation and replacement
