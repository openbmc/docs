# Phosphor license service feature design

Author: Abhilash Raju

Other contributors: Raviteja Bailapudi

Created: 05/07/2023

## Glossary
- PLDM - Platform level data model
- PLM - Phosphor license manager


## Problem Description


Systems can be shipped with additional hardware than what was purchased in order
to support flexible consumption based model. The hardware can be released on
demand by purchasing extra licenses. Firmware updates ,such as driver updates,
should be controlled by a license in this model.Thus a mechanism is need to
facilitate installation, monitoring and management of licenses. 

## Background and References



## Requirements

Implement redfish license install/delete interfaces for redfish clients 
based on the schema defined here [here](https://redfish.dmtf.org/schemas/v1/LicenseService.v1_1_0.json)

## Proposed Design

The license manager is a demon that facilitate license installation , management
and monitoring procedures. The demon hosts set of Dbus objects and properties in
order to keep track of installed license and its expiration dates.

The below Diagram shows the data flow in the installation sequence.

1. Client will post a redfish install request with file path as parameter
2. BmcWeb calls install Interface method hosted by PLM
3. PLM notifies back with progress status
4. A Redfish event will be sent back to the client
5. PLM Downloads the file from the URL
6. PLM send the file data to Host via using pldm file IO commands
7. Host parses the data, check validity, and Installs the license
8. Installed license details will be sent back to PLM. PLM parses the file
9. PLM creates/update license object based on the parsed details 
10. PLM notifies BmcWeb the install sucess/failure status
11. Corresponding Redfish event will be sent back to the client

### Design Flow

```ascii
┌─────────┐         ┌─────────┐          ┌─────────┐             ┌─────────┐
│Client   │         │ BmcWeb  │          │  PLM    │             │  Host   │
└────┬────┘         └─────┬───┘          └────┬────┘             └─────┬───┘
     │                    │                   │                        │
     │                    │                   │                        │
     │                    │                   │                        │
     │   Install          │                   │                        │
     ├────────────────────►                   │                        │
     │                    │                   │                        │
     │                    │ Install Method    │                        │
     │                    ├───────────────────►                        │
     │                    │                   │                        │
     │                    │  Update Status    │                        │
     │                    ◄───────────────────┤                        │
     │                    │                   │                        │
     │                    ├────┐              │                        │
     │                    │    │ Create Task  │                        │
                          ◄────┘              │                        │
     │ Progress Event     │                   │                        │
     ◄────────────────────┤                   │                        │
     │                    │                   │                        │
     │                    │ Dowload File      │                        │
     ◄────────────────────┼───────────────────┤                        │
     │                    │                   │ File Data (PLDM IO)    │
     │                    │                   ├────────────────────────►
     │                    │                   │                        │  Parse Data
     │                    │                   │                        ├───────┐
     │                    │                   │                        │       │
     │                    │                   │                        │       │  Check Validity  &
     │                    │                   │                        ◄───────┘  Install License
     │                    │                   │                        │
     │                    │                   │                        │
     │                    │                   │                        │
     │                    │                   │                        │
     │                    │                   │ License Details(PLDM IO)
     │                    │                   ◄────────────────────────┐
     │                    │                   │                        │
     │                    │                   │ Update Dbus            │
     │                    │                   ├─────┐                  │
     │                    │                   │     │                  │
     │                    │                   │     │                  │
     │                    │                   │     │                  │
     │                    │                   ◄─────┘                  │
     │                    │   Install status  │                        │
     │                    ◄───────────────────┤                        │
     │                    │                   │                        │
     │                    ├──────┐            │                        │
     │                    │      │            │                        │
     │                    │      │Update Task │                        │
     │                    ◄──────┘            │                        │
     │                    │                   │                        │
     │ Finished/Cancelled │                   │                        │
     │     Event          │                   │                        │
     ◄────────────────────┤                   │                        │
     │                    │                   │                        │
     │                    │                   │                        │
     │                    │                   │                        │
```

## Alternatives considered:

### Alternate Design

## Impacts

## Testing

