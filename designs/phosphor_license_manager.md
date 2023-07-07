# Phosphor license service feature design

Author: Abhilash Raju

Other contributors: Raviteja Bailapudi

Created: 05/07/2023

## Glossary

- PLDM - Platform Level Data Model
- LM - License Manager

## Problem Description

Systems can be shipped with additional hardware than what was purchased in order
to support flexible consumption based model. The hardware can be released on
demand by purchasing licenses. Clients who aquired valid license needs a redfish
support in BMC for the transfer of license file to Host machine.

Scope of this design limited to

1. Installation of license file
2. Fetch details of installed licenses
3. Fetch progess status of license installation

This design document does not cover

1. License expiration use case
2. Expiry notification use case
3. Vallidation of license. 
4. Access control of licensed HW or FW

## Background and References

## Requirements

Implement redfish license install/delete interfaces for redfish clients based on
the schema defined
[here](https://redfish.dmtf.org/schemas/v1/LicenseService.v1_1_0.json)

## Proposed Design

The license manager creates DBus object necessory to 
transfer the license data from redfish clients to the Host. 
License manager or any other process runnning in BMC does not 
validate or control the access to any components of fw of BMC. 
The license will be validated at the Host side.Any sort of enablement, 
disablement or access control dictated by the license is done by host 
on the licensed HW and SW running in Host. 

### Design Flow

Below Diagram shows the data flow in the installation sequence.

```ascii
┌────────┐   ┌──────┐    ┌────────┐   ┌──────┐   ┌──────┐  ┌──────┐
│ Client │   │BmcWeb│    │DownMan │   │LicMan│   │ PLDM │  │ HOST │
└───┬────┘   └──┬───┘    └───┬────┘   └───┬──┘   └───┬──┘  └───┬──┘
    │           │            │            │          │         │
    │ Install   │            │            │          │         │
    ├──────────►│            │            │          │         │
    │           │            │            │          │         │
    │           ├────┐       │            │          │         │
    │           │    │Create │            │          │         │
    │           │    │ task  │            │          │         │
    │           │◄───┘       │            │          │         │
    │           │            │            │          │         │
    │           │ ReqDownload│            │          │         │
    │           ├───────────►│            │          │         │
    │     Start │Downloading │            │          │         │
    │◄──────────┼────────────┤            │          │         │
    │           │            │            │          │         │
    │           ├────┐       │            │          │         │
    │           │    │Update │            │          │         │
    │           │    │ Task  │            │          │         │
    │           │    │       │            │          │         │
    │ Progress  │◄───┘       │            │          │         │
    │   Event   │            │            │          │         │
    │◄──────────┤            │            │          │         │
    │           │            │            │          │         │
    │           │DownloadedFD│            │          │         │
    │           │◄───────────┤            │          │         │
    │           │            │            │          │         │
    │           │            │            │          │         │
    │           │Update Dbus │with FD     │          │         │
    │           │            │            │          │         │
    │           ├────────────┼──────────► │          │         │
    │           │            │            │Dbus Event│         │
    │           │            │            ├─────────►│         │
    │           │            │            │          │         │
    │           │            │            │          │ PLDM IO │
    │           │            │            │          ├────────►│
    │           │            │            │          │         │
    │           │            │            │          │         │
    │           │            │            │          │         │
    │           │            │            │          │         ├────────┐
    │           │            │            │          │         │        │Check
    │           │            │            │          │         │        │  &
    │           │            │            │          │         │        │Install
    │           │            │            │          │         │        │License
    │           │            │            │          │         │◄───────┘
    │           │            │            │          │License  │
    │           │            │            │          │Details  │
    │           │            │            │          │◄────────┤
    │           │            │            │ Update   │         │
    │           │            │            │  DBus    │         │
    │           │            │            │◄─────────┤         │
    │           │ Dbus Event │            │          │         │
    │           │◄───────────┼────────────┤          │         │
    │           │            │            │          │         │
    │           │            │            │          │         │
    │           ├────┐       │            │          │         │
    │           │    │Update │            │          │         │
    │           │    │ Task  │            │          │         │
    │           │    │       │            │          │         │
    │           │◄───┘       │            │          │         │
    │           │            │            │          │         │
    │ Complete  │            │            │          │         │
    │  Event    │            │            │          │         │
    │◄──────────┤            │            │          │         │
    │           │            │            │          │         │
```
1. Client will post a redfish install request with file path as parameter
2. BmcWeb hands file url over to Download Manager
3. BmcWeb creates a task to notify the progress of license installation
4. A Redfish event will be sent back to the client with progess status
5. Download Manager downloads the file and notifies back to Bmcweb
6. Bmcweb updates the downloaded file descriptor to Dbus property Hosted
   by License manager
7. License manager emits file descriptor change event
8. On notify PLDM reads the file and upload the content to HOST
9. Host will extract the details and validate the license
10. On successful validation host send back status and license data to PLDM
11. PLDM update the Dbus property with license details
12. On Dbus notify BmcWeb will send events to the client for the install status
## Alternatives considered:

 We can avoid License Manager Service and let PLDM  host the Dbus objects instead.
 But it will have impacts on the modularity and maintainability of PLDM in long run
### Alternate Design



## Impacts

### Bmcweb

Bmcweb expected to have changes to support new redfish endpoint for 
the installation of license. 

### license-manager

License manager creates Dbus objects and events necessory for the 
proper transfer of the license file to the Host. The Dbus properties will 
store license properties that can be retrieved by redfish clients. Redfish
clients can use this information to display details of the installed licenses
in some user interface elements. 

### pldm

Pldm expected to have changes required to monitor the events originated
from license Dbus object. Pldm initiates the license file transfer to
the Host. Pldm collects license details from host and updates the 
license Dbus propeties.

## Organizational

License Manager is new service in BMC. It needs new repository
Maintainers - Raviteja Bailapudi, Abhilash Raju 
License Manager depends on BcmWeb and PLDM. These repositories
expected to have modifications to support License Installation.
## Testing


