# Google Specific APIs - A New Service Root for Google

Author: Feras Aldahlawi (faldahlawi)

Other contributors: None

Created: March 30, 2021

## Problem Description

Redfish API does not have a resource type that is similar to Google's Root of
Trust (RoT) chips. Google needs APIs that are not in the Redfish standard yet.
There are working groups dedicated to bring RoT chips support to the Redfish
standard already. Hence adding this support under a Google namespace would avoid
conflict with those working groups. This document provides the schema of what
Google needs for its new service root.

## Background and References

At Google, we rely on communicating with RoT chips using a variety of transport
mechanisms. Google wants to extend the support to include REST based APIs. The
future of RoT devices at Google will adopt the
[SPDM protocol](https://www.dmtf.org/sites/default/files/PMCI_Security-Architecture_12-11-2019.pdf).
However, this design doc is targeting a group of RoT devices that will never be
capable of supporting standards based interface.

## Requirements

- Create a new service root of Google specific APIs.
- Create a schema for a RootOfTrust resource.
- Be able to execute RoT actions (attestation etc) from the API.

## Proposed Design

A new service root under `google/v1`. This service root will contain a
collection of `RootOfTrust` entities that have the following properties and
Actions:

- Chip type string
- Unique Hardware id string
- Firmware version map string to string
- Mailbox API

This new API is designed to forward calls to RoT devices and avoid and
inspections of data. An example call would be:

```
{
  "#RootOfTrust.Mailbox": {
      "target": "/redfish/v1/RootsOfTrust/0/Actions.Mailbox",
      "@Redfish.ActionInfo": "/redfish/v1/RootsOfTrust/0/Actions.Mailbox"
    }
  "RawRequest": "some_bytes_to_be_parsed_by_receiver"
}
```

This new service root is very similar to `/ibm/v1`. This would require a new
dbus interface to service this API:

```
description: >
    Forward bytes to Google RoT devices.
methods:
    - name: Mailbox
      description: >
          A method to forward bytes to RoT device.
      parameters:
        - name: rawRequest
          type: array[byte]
          description: >
              Value to be updated for the keyword.
      errors:
        - xyz.openbmc_project.Common.Error.InvalidArgument
        - xyz.openbmc_project.Common.Error.InternalFailure
```

## Alternatives Considered

Considered adding the new APIs as an OEM extension to the TPM resource. However,
it was an unnatural extension. Here are the reason why it is somewhat unnatural
to use TPM for Google's RoT:

- FirmwareVersion1/2
  - Somewhat closely fixed to the design of TPM. TPM 1.2 had 32-bit firmware
    version and TPM 2.0 extended it clumsily by just tacking on another firmware
    version 32-bit field.
  - TPM "Firmware 1" and "Firmware 2" together refer to the 64-bit firmware
    version number. Most TPM2.0 vendors divide this into 4 fields each 2 bytes
    wide: (big-endian, each character is a byte:) 0xMMmmrrbb (M major, m minor,
    r rev, b build). Infineon uses a different convention for firmware version
    numbers than the rest of the TPM vendors, reserving some bits and expressing
    only a 1-byte wide "build number" as 0xMMmm_rrb
  - These being exposed as a string out to the Redfish interface works for
    Google. But I am just trying to provide info on how uniform this currently
    is (not) within the TPM ecosystem.
- InterfaceType
  - Currently closely fixed to the ecosystem of TPM variants.
  - Which flavor of TPM interface is implemented. TCM is the "China version" of
    TPM 1.2. The Chinese TPM switched over to TPM 2.0 after that version of the
    spec was available.
  - TPM 1.2 and 2.0 are entirely different API surfaces, analogous to the
    difference between any TPM and Google's RoT chips.
- InterfaceTypeSelection
  - Currently closely fixed to the ecosystem of TPM variants.
  - Some TPMs can be switched between TPM 1.2 and 2.0. This could be ignorable
    by Google unless Google start shipping an open sourced RoT that could be
    switched into a TPM mode via firmware update. (Which would be a good move)

Though we can put everything under TPM's OEM (e.g. version numbers and other
functionality), most of the fields will be unusable for Google's RoT.

## Impacts

New daemon, new Rest API.

## Testing

Testing will be done in Google internally. This feature should not impact CI
testing. We will try golden paths and bad paths. We will also implement fuzz
tests as well.
