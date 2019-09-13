# Redfish resource supplement for Platform Firmware Resilience (PFR)

Author: AppaRao Puli

Primary assignee: AppaRao Puli !apuli

Other contributors: None

Created: 2019-09-12

## Problem description

The platform is a collection of fundamental hardware and firmware components
needed to boot and operate a system. The Platform Firmware Resiliency(PFR)
in NIST SP 800-193 provides technical guidelines and recommendations
supporting resiliency of platform firmware and data against potentially
destructive attacks.

Currently Redfish schema's defined by DMTF doesn't cover properties or
resources to represent the PFR provisioned and locked states.

This document covers the new OEM properties under ComputerSystem resource
to represent the PFR provisioning status such as platform firmware is
provisioned or not as well as provisioning is locked or not. This also covers
the Redfish OpenBMC message registry metadata for logging events associated
with PFR.

## Background and references

Platform Firmware Resilience technology in NIST SP 800-93 provide common
guidelines to implementers, including Original Equipment Manufacturers (OEMs)
and component/device suppliers, to build stronger security mechanisms into
platforms. BMC software running on server platform consist of multiple devices
that provide the functional capabilities and services, need to provide
resiliency by protecting, detecting and recovering platform assets.

 - [NIST.SP.180-193](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-193.pdf)
 - [Redfish schema supplement](https://www.dmtf.org/sites/default/files/standards/documents/DSP0268_2019.1a.pdf)
 - [Redfish Logging in bmcweb](https://github.com/openbmc/docs/blob/master/redfish-logging-in-bmcweb.md)

## Requirements

High level requirements:

  - BMC shall provide the way to represent Platform Firmware Resilience
    provisioning status over Redfish.

  - Event logs should be logged to redfish for Platform Firmware Resilience.

## Proposed design

Different OEM's has there own way of implementing the Platform Firmware
Resilience by using guidelines provided by NIST SP 800-193. Some of the
component protected under this includes(not limited) ES/SIO, BMC/ME Flash,
Host Processors, Trusted Platform Modules(TPM), PUS's, Memory etc...
For example Intel uses the "Intel PFR" design to resiliency platform
components.

NOTE: This document doesn't cover design aspects of how OEM's implements
the Platform Firmware Resilience. It covers only generic redfish ComputerSystem
OEM properties and Redfish message registry metadata entries which are
implementation agnostic.

OpenBMC is moving to Redfish as its standard for out of band management.
The bmcweb implements all the Redfish schema's to represent different
properties and resources. ComputerSystem schema doesn't cover the properties
associated with Platform Firmware Resilience but it provides OEM objects for
manufacturer/provider specific extension moniker.

Below are two OEM properties defined to represent the Platform Firmware
Resilience provisioning status.

  - Provisioned: The value of this property shall be a boolean indicating
    provisioned state of platform firmware.

  - Locked: The value of this property shall be a boolean indicating platform
    firmware provisioning is locked.

PFR enabled platforms can provision or re-provision the platform resilience
multiple times without Locking. But once the platform is locked by provisioning
agent after, it can not be re-provisioned.

New OemComputerSystem schema can be found at
[link](https://gerrit.openbmc-project.xyz/#/c/openbmc/bmcweb/+/24253/)

PFR implementation in OpenBMC also logs any events associated with
Platform Firmware Resilience. Below are two metadata entries in redfish
message registry for redfish event logging capability. For more details on
how to log redfish events can be found at document [Redfish logging in bmcweb
](https://github.com/openbmc/docs/blob/master/redfish-logging-in-bmcweb.md).


```
MessageEntry{
        "PlatformFirmwareError",
        {
            .description = "Indicates that the specific error occurred in "
                           "platform firmware.",
            .message = "Error occurred in platform firmware. ErrorCode=%1",
            .severity = "Critical",
            .numberOfArgs = 1,
            .paramTypes = {"string"},
            .resolution = "None.",
        }},
    MessageEntry{
        "PlatformFirmwareEvent",
        {
            .description = "Indicates that the platform firmware events like "
                           "panic or recovery occurred for the specified "
                           "reason.",
            .message = "Platform firmware %1 event triggered due to %2.",
            .severity = "Critical",
            .numberOfArgs = 2,
            .paramTypes = {"string", "string"},
            .resolution = "None.",
        }},
```


## Alternatives considered

None

## Impacts

None

## Testing

Provisiong status:

  - User can provision the PFR in OEM specific way and test using below URI
    and Method. Intel uses "Intel PFR" design (via BIOS) to provision and
    lock the PFR provisioning status.
```
URI: /redfish/v1/Systems/system
METHOD: GET
RESPONSE:

{
  "@odata.context": "/redfish/v1/$metadata#ComputerSystem.ComputerSystem",
  "@odata.id": "/redfish/v1/Systems/system",
  "@odata.type": "#ComputerSystem.v1_6_0.ComputerSystem",
  ...................
  ...................
  "Description": "Computer System",
  "Oem": {
    "OpenBmc": {
      "FirmwareProvisioning": {
        "Locked": true,
        "Provisioned": true
      }
    }
  },
  ...................
  ...................
}
```

Event logs:

  - User can induce security attack and validate the panic event logs as well as
    recovery mechanism using below URI.

    Few examples to attack Firmware components and validate PFR:

     1) Corrupt the BMC firmware and check Panic events and recovery
        actions events.

     2) Induce hardware watchdog trigger using known methods and check.
     etc..

     3) Corrupt the security key's by directly mocking hardware and validate.

```
URI: /redfish/v1/Systems/system/LogServices/EventLog/Entries
METHOD: GET
```
