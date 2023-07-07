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
demand by purchasing licenses. One of the examples would be to control the
firmware updates using licenses, by not allowing the new driver installation
which is released beyond certain date which system is entitled for. Thus a
mechanism is need to facilitate installation, monitoring and management of
licenses.

Scope of this design limited to

1. Installation of license file
2. Fetch details of installed license file
3. Fetch progess status of license installation

This design document does not cover

1. License expiration use case
2. Expiry notification use case
3. Vallidation of license. 

## Background and References

## Requirements

Implement redfish license install/delete interfaces for redfish clients based on
the schema defined
[here](https://redfish.dmtf.org/schemas/v1/LicenseService.v1_1_0.json)

## Proposed Design

The license manager is a demon that creates DBus object necessory to 
transfer the lincense data from clients to the Host. License manager or 
any other process runnning in BMC does not validate or access control any 
component of BMC. The license will be validated at the Host side. Any sort
of enablement, disablement or access control dictated by the license is 
done at host. 

The below Diagram shows the data flow in the installation sequence.

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


### Design Flow

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

## Alternatives considered:

Instead of adding new PLM we can incorporate changes directly to PLDM But PLDM
is trying to decouple all service specific logic from it in order to make it a
light weight component focusing only on host interfacing functionalities. So
attempt to add modification related to license manager in PLDM is discarded.

### Alternate Design



## Impacts

### Bmcweb

Redfish endpoint implementation. Convertiong of Redfish json to dbus properties
Implementation of Task monitor

### license-manager

Creation of lincense dbus objects, monitoring installation progress

### pldm

APIs for host interfacing

## Testing

The following BAT(basic acceptace test) has to be executed

1. Install Lincese using Redfish API->varify the intallation progress using Task
   tracker uri -> varify success
2. Query installed license informations using redfish API
3. Try to install malformed license->varify the rejection status
