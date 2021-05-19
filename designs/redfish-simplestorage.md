# Redfish SimpleStorage design
Author: JunLin Chen

Primary assignee: JunLin Chen

Created: 2021-05-11

## Problem description

Redfish has resources that describe simple storage devices available on a system. It would be useful to provide these resources to users out-of-band over Redfish from OpenBMC.

## Background and references

The Redfish SimpleStorage resource is here.
[SimpleStorage](https://redfish.dmtf.org/schemas/v1/SimpleStorage_v1.xml)

The support service is below,

[SimpleStorage Manager](https://gerrit.openbmc-project.xyz/c/openbmc/docs/+/43351) (Still in Process)

## Requirements

The proposed implementation will follow the standard D-Bus producer-consumer model used in OpenBMC. The producer will provide the required SimpleStorage system configuration information read from hardware. The consumer will retrieve and parse the D-Bus data to provide the Redfish SimpleStorage resources.

The SimpleStorage Manager service will be a new D-Bus daemon that will be responsible for gathering simplestorage data and maintaining the D-Bus interfaces and properties.

bmcweb will be the consumer. It will be responsible for retrieving the Redfish SimpleStorage devices presence data from the D-Bus properties and providing it to the user.

## Proposed design

The proposed implementation will follow the standard D-Bus producer-consumer model used in OpenBMC. The producer will provide the required SimpleStorage system configuration information read from hardware. The consumer will retrieve and parse the D-Bus data to provide the Redfish SimpleStorage resources.

The SimpleStorage Manager service will be a new D-Bus daemon that will be responsible for gathering simplestorage data and maintaining the D-Bus interfaces and properties.

bmcweb will be the consumer. It will be responsible for retrieving the Redfish SimpleStorage devices presence data from the D-Bus properties and providing it to the user.

### Redfish resource schema's

Redfish will have two major resources to support SimpleStorage.

  1. Simple Storage Collections
  2. Simple Storage Controller

**Simple Storage Collections**

  This resource is used to get collection of all simple storage controllers.

   - URI : `/redfish/v1/Systems/System/SimpleStorage`
   - SCHEMA : [SimpleStorageCollection](https://redfish.dmtf.org/schemas/v1/SimpleStorageCollection_v1.xml)
   - METHODS : `GET`

Refer Testing section for example.

**Simple Storage Controller**
  This resource contains the simple storage controllers details. Used to get the devices information for each controller.

   - URI : `/redfish/v1/EventService/SimpleStorage/<ID>`
   - SCHEMA : [SimpleStorage](https://redfish.dmtf.org/schemas/v1/SimpleStorage_v1.xml)
   - METHODS : `GET`

Refer Testing section for supported properties and example.

## Alternatives considered

None

## Impacts

A new daemon should implement to collection the D-bus information for devices of the simple storage controllers.


## Testing

This can be tested using the Redfish Service Validator.


**Simple Storage Collections**

Example GET Response:
```
{
  "@odata.id": "/redfish/v1/Systems/system/SimpleStorage",
  "@odata.type": "#SimpleStorageCollection.SimpleStorageCollection",
  "Members": [
    {
      "@odata.id": "/redfish/v1/Systems/system/SimpleStorage/0"
    },
    {
      "@odata.id": "/redfish/v1/Systems/system/SimpleStorage/1"
    }
  ],
  "Members@odata.count": 2,
  "Name": "Simple Storage Collection"
}
```

**Simple Storage Controller**

Example GET output:
```
{
  "@odata.id": "/redfish/v1/Systems/system/SimpleStorage/1",
  "@odata.type": "#SimpleStorage.v1_2_4.SimpleStorage",
  "Description": "System SATA",
  "Device": [
    {
      "Name": "SATA_HDD_1",
      "Status": {
        "Heatlth": "OK",
        "State": "Enabled"
      }
    },
    {
      "Name": "SATA_HDD_2",
      "Status": {
        "State": "Absent"
      }
    }
  ],
  "Id": "1",
  "Links": {
    "Chassis": [
      {
        "@odata.id": "/redfish/v1/Chassis/chassis"
      }
    ]
  },
  "Name": "Simple Storage Controller",
  "Status": {
    "Health": "OK",
    "State": "Enabled"
  }
}
```
