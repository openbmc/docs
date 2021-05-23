# Redfish Drive Properties
Author: Willy Tu

Primary assignee: Willy Tu

Created: 2021-05-21

## Problem description

Redfish in OpenBMC currently has limited support for monitoring/controlling Drive resources. It can be useful to have more out-of-band State and Power control over the Drives.

The current Drive in OpenBMC only includes the FRU information and `Enabled/Updating` state for the status. Some new properties to consider includes software controls, power monitoring, and resets actions.

The software will be managed by SoftwareInventory, however, it currently only supports for BMC/BIOS and Versions property. Some new properties to consider includes custom name, write protect, and updates.

The goal of this document is to add new resources/properties that can help represent Drive and related resources better with OpenBMC/Redfish.

## Background and references

[Redfish Specification](https://redfish.dmtf.org/schemas/DSP0268_2020.4.pdf)

Related Redfish Resources:

- [Drive](https://redfish.dmtf.org/schemas/v1/Drive_v1.xml)
- [SoftwareInventory](https://redfish.dmtf.org/schemas/v1/SoftwareInventory.v1_4_0.yaml)

Related WIP Redfish Drive Property:
- Drive Power Mode/Status: github.com/DMTF/Redfish/issues/4523

Related Existing D-Bus Interface:
- [Drive](github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Inventory/Item/Drive.Interface.yaml)
- [Software Version](github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Software/Version.interface.yaml)

Related WIP Dbus Interfaces:
- [Drive Software Version](https://gerrit.openbmc-project.xyz/c/openbmc/phosphor-dbus-interfaces/+/43458)
- [Drive State](https://gerrit.openbmc-project.xyz/c/openbmc/phosphor-dbus-interfaces/+/43155)
- [Software States](https://gerrit.openbmc-project.xyz/c/openbmc/phosphor-dbus-interfaces/+/43427)
- [Drive Power State](https://gerrit.openbmc-project.xyz/c/openbmc/phosphor-dbus-interfaces/+/43428) (Waiting on Redfish support for Drive Power Mode/State)

Existing Bmc Handler:
- [Drive](https://github.com/openbmc/bmcweb/blob/master/redfish-core/lib/storage.hpp)
- [Softwareinventory](https://github.com/openbmc/bmcweb/blob/master/redfish-core/lib/update_service.hpp)

## Requirements

This feature is intended to meet the Redfish requirements for the Drive and related resources above to provide basic monitoring/management of Drive devices.

This document will focus on the additional properties for bmcweb consumer and only have a brief overview on the D-Bus daemon.

For the bmcweb/Redfish, it will support Drive and SoftwareInventory. SoftwareInventory is included for the firmware management.

The new Drive properties in considation include:
- Physical Location
  - Slot number and such...
- Reset Actions
- Status (support for more states)
- Power Mode/State (if Redfish adds the support)

The new SoftwareInventory properties in consideration include:
- Name (Custom Software Name)
- RelatedItem (support Drives)
- Updateable
- WriteProtected

For the D-Bus Daemon, it will provide all the information listed above for bmcweb/Redfish.

## Proposed design

The proposed implementation will follow the standard D-Bus producer-consumer model used in OpenBMC. The producer will provide the required Drive configuration information read from hardware. The consumer will retrieve and parse the D-Bus data to provide the Redfish Drive resources.

The Drive service will be a new D-Bus daemon that will be responsible for gathering Drive data and maintaining the D-Bus interfaces and properties.

bmcweb will be the consumer. It will be responsible for retrieving the Redfish Drive device information from the D-Bus properties and providing it to the user.

### D-Bus Daemon

A brief overview of what the daemon needs to support the Drive resources.

The daemon will probe InventoryManager and EntityManager for FRU devices that is a Drive. The daemon will create a d-bus object under `/xyz/openbmc_project/inventory` for each Drive and include the `xyz.openbmc_project.Inventory.Item.Drive` interface in the FRU.

The Drive will have the following interfaces for information about the Drive.

Required for basic management/control:
- `xyz.openbmc_project.Inventory.Item.Drive`
- `xyz.openbmc_project.Inventory.Decorator.Asset`
- `xyz.openbmc_project.State.Drive`

Each Drive can also expose new d-bus objects to manage the power states.
- `xyz.openbmc_project.Inventory.Item.Drive.PowerGood`
- `xyz.openbmc_project.Inventory.Item.Drive.PowerSequence`

Optionaly for Software managements, create a object under `/xyz/openbmc_project/software` for the drive that includes:
- `xyz.openbmc_project.Software.Version`
- `xyz.openbmc_project.Software.State`


How the infomraitons are gathered are up to the device specific implementation.

### Bmcweb Design

The bmcweb will support the redfish endpoints described under `Redfish resource schema`.

The Drive endpoint will use existing structure in bmcweb to support all its need. More properties will be added on top of it.

For the SoftwareInventory endpoint, it will have to support Drive to be part of the `RelatedItem`. Currently it support BMC and BIOS with hardcoded paths.

The RelatedItem can be solved with [Association](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Association.interface.yaml) at the d-bus level.



For example, for the Software d-bus object it will have a association to the Drive/other devices.

```
xyz.openbmc_project.Association
  Endpoints: ['/xyz/openbmc_project/inventory/.../{DriveId}']
```

Then we can have the SoftwareInventory relate to `/redfish/v1/Systems/system/Storage/1/Drives/{DriveId}`.

### Redfish resource schema

The bmchandler already supported Drive and SoftwareInventory
- [Drive](https://github.com/openbmc/bmcweb/blob/master/redfish-core/lib/storage.hpp)
- [Softwareinventory](https://github.com/openbmc/bmcweb/blob/master/redfish-core/lib/update_service.hpp)

New Redfish endpoint to support for Drive:
    1. Drive Reset Action

**Drive Reset Action**

  This action is related to Drive Controller and used to reset the Drive.
   - URI : `/redfish/v1/Systems/{SystemId}/Storage/{StorageId}/Drives/{DriveId}/Actions/Drive.Reset`
   - SCHEMA : [Drive](https://redfish.dmtf.org/schemas/v1/Drive_v1.xml)
   - METHODS : `POST`

## Alternatives considered

N/A

## Impacts

A new daemon will be implemented to collection the D-bus information for devices of the Drive controllers.

New redfish property support for Drive and SoftwareInventory. The request will take a bit longer, because more information needs to be fetched.


## Testing

This can be tested using the Redfish Service Validator.

**Drive Collections**

Example GET Response:
```
{
    "@odata.id": "/redfish/v1/Systems/{SystemId}/Storage/{StorageId}/Drives",
    "@odata.type": "#DriveCollection.DriveCollection",
    "Name": "Drive Collection",
    "Members@odata.count": 2,
    "Members": [
        {
            "@odata.id": "/redfish/v1/Systems/{SystemId}/Storage/{StorageId}/Drives/{DriveId0}"
        },
        {
            "@odata.id": "/redfish/v1/Systems/{SystemId}/Storage/{StorageId}/Drives/{DriveId1}"
        },
    ]
}
```

**Drive Controller**

Example GET output:
```
{
    "@odata.id": "/redfish/v1/Systems/{SystemId}/Storage/{StorageId}/Drives/{DriveId}",
    "@odata.type": "#Drive.v1_12_0.Drive",
    "Id": "1",
    "Name": "Example Drive",
    "Status": {
        "State": "Enabled",
        "Health": "OK"
    },
    "Model": "This Drive",
    "Manufacturer": "Company0",
    "SerialNumber": "XXXXXXXXXX",
    "PartNumber": "XXXXXXXXXX",
    "Actions": {
        "#Drive.Reset": {
            "target": "/redfish/v1/Systems/{SystemId}/Storage/{StorageId}/Drives/{DriveId}/Actions/Drive.Reset"
        },
    },
    "PhysicalLocation": {
        "PartLocation": {
            "LocationType": "Slot",
            "ServiceLabel": "PE0"
        }
    }
}
```

***Drive Reset Action***

Example POST input:
```
{
    "@odata.id": "/redfish/v1/Systems/{SystemId}/Storage/{StorageId}/Drives/{DriveId}/Actions/Drive.Reset",
    "ResetType": "GracefulRestart"
}
```

**SoftwareInventory Collection**

Example GET Response:
```
{
    "@odata.type": "#SoftwareInventoryCollection.SoftwareInventoryCollection",
    "Name": "SoftwareInventory Collection",
    "Members@odata.count": 2,
    "Members": [
        {
            "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/{SoftwareInventoryId0}"
        },
        {
            "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/{SoftwareInventoryId1}"
        },
    ],
    "@odata.id": "/redfish/v1/3"
}
```

**SoftwareInventory Controller**

Example GET Response:
```
{
    "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/{SoftwareInventoryId}",
    "@odata.type": "#SoftwareInventory.v1_4_0.SoftwareInventory",
    "Description": "Software",
    "SoftwareId": "warthog_0_cpld",
    "Version": "1234",
    "RelatedItem": [
        {
            "@odata.id": "/redfish/v1/Systems/{SystemId}/Storage/{StorageId}/Drives/{DriveId}"
        }
    ]
}
```

Example PATCH Response:

Update the Version to `5678`
```
{
    "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/{SoftwareInventoryId}",
    "@odata.type": "#SoftwareInventory.v1_4_0.SoftwareInventory",
    "Version": "5678",
}
```
