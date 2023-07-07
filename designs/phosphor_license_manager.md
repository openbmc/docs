# Phosphor license service feature design

Author: Abhilash Raju

Other contributors: Raviteja Bailapudi

Created: 05/07/2023

## Glossary

- PLDM - Platform Level Data Model

## Problem Description

Systems can be shipped with additional hardware than what was purchased in order
to support flexible consumption based model. The hardware can be released on
demand by purchasing licenses. Clients who aquired valid license needs a redfish
support in BMC for the transfer of license file to Host machine.

Scope of this design limited to

1. Installation of license file
2. Fetch details of installed licenses
3. Fetch progress status of license installation

This design document does not cover

1. License expiration use case
2. Expiry notification use case
3. Validation of license
4. Access control of licensed HW or FW

## Background and References

## Requirements

Implement redfish license install/delete interfaces for redfish clients based on
the schema defined
[here](https://redfish.dmtf.org/schemas/v1/LicenseService.v1_1_0.json)

## Proposed Design

The license manager creates DBus object necessory to transfer the license data
from redfish clients to the Host. License manager or any other process runnning
in BMC does not validate or control the access to any components of BMC. The
license will be validated at the Host side.Any sort of enablement, disablement
or access control dictated by the license is done by host on the licensed HW and
SW running in Host.

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
4. A Redfish event will be sent back to the client with progress status
5. Download Manager downloads the file and notifies back to Bmcweb
6. Bmcweb updates the downloaded file descriptor to Dbus property Hosted by
   License manager
7. License manager emits a property change event corresponding to the file
   descriptor
8. On event notification PLDM reads the file and upload the content to HOST
9. Host will extract the license details and validate the license
10. After successful validation of the license host send back validation status
    and subset of license data mentioned in
    [the Redfish schema](https://redfish.dmtf.org/schemas/v1/LicenseService.v1_1_0.json)
    to PLDM
11. PLDM updates the apropriate Dbus properties with the license details
12. Dbus events will be generated for the property changes.
13. After recieving the event notification BmcWeb will send a redfish events to
    the clients about the the installation status

## Alternatives considered:

We can avoid License Manager Service and let PLDM host the Dbus objects instead.
But it will have impacts on the modularity and maintainability of PLDM in long
run

### Alternate Design

None

## Impacts

### Bmcweb

Bmcweb expected to have changes to support new redfish endpoint for the
installation of license.

### License Manager

License manager creates Dbus objects and events necessary for the proper
transfer of the license file to the Host. The Dbus properties will store license
properties that can be retrieved by redfish clients. Redfish clients can use
this information to display details of the installed licenses in some user
interface elements.

### PLDM

PLDM expected to have changes required to monitor the events originated from
license Dbus object. PLDM initiates the license file transfer to the Host. PLDM
collects license details from host and updates the license Dbus properties.

### Host

Host will accept the license file, does the parsing, and send back extracted
license details to PLDM. Host can use the license data for the access control of
HW/SW resources inside Host machine. But that is not part of this Design
document.

## Organizational

License Manager is new service in BMC. It needs new repository Maintainers -
Raviteja Bailapudi, Abhilash Raju License Manager depends on BmcWeb and PLDM.
These repositories expected to have modifications to support License
Installation.

## Testing

The following BAT(basic acceptance test) has to be executed

1. Install License using Redfish API->verify the installation progress using
   Task tracker URI -> verify success
2. Query installed license informations using redfish API
3. Try to install malformed license->verify the rejection status

Automation of the above cases are not yet decided.
