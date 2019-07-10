# Save Area Management through the REST interface.
Author: Ratan Gupta <ratagupt@linux.vnet.ibm.com>

Primary assignee: Ratan Gupta

Created: 2019-07-10

## Problem Description

Each partition data will be saved as save area file in the BMC file system.
In a fully configured system, there can be 1000 partitions which will require
an equal number of save area files BMC should provide the interface for CRUD
operations for save area files.
BMC should notify other HMC which is connected if there is any change
in the save area files.
This shall be done via asynchronous event handling mechanisms.

BMC doesn't need to understand or parse these files, BMC will act as a storage
for the save area files.

OpenBMC is moving to [Redfish] as its standard for out of band management.
Redfish don't have the schema files for save area management, this is kind of
IBM specific requirement.

#### Use cases:

- When the virtualization firmware is down, the management console needs to create the
  logical partitions on a destination system using the configuration details stored in the save area files.

- Restore configuration using the save area files when the system is in an error
  state. The system may have up to 1,000 LPARs that need to be restored.

The goal of this document is to define the REST interfaces for the save area files.

## Requirements
- BMC shall provide 15MB reserved space for save area files.
- list of all the save area files.
- read a specific save area file.
- delete a specific save area file.
- create/update of the save area file.
- Send the event notification to the connected clients(MC) for creation/deletion/updation
of the save area file.

## Rest Interfaces

### List of all the save area files
The following interface would list all the save area files.
```
GET https://${bmc}/xyz/openbmc_project/HMC/savearea/enumerate
```
### Create/upload the save area file
The following interface would create/upload the save area file.
```
PUT https://${bmc}/xyz/openbmc_project/HMC/savearea/<filename> <filedata as octet
stream>
```
This will cause the following on the back-end
- save the file in the filesystem(persistent location) with the given name.
- Some application would Inotify to the directory location.
- create/update the D-Bus object for the save area file.

### Delete the save area file
Deletion of savearea file will be supported and it will be initiated
by the connected client by issuing the following request.
```
DELETE https://${bmc}/xyz/openbmc_project/HMC/savearea/<filename>
```
This will delete the save area file from the persistent location.
Application which is Inotifing to the directory location will delete the
corresponding D-bus object.


## Read the save area file
Save area file can be read by the following request.
```
GET https://${bmc}/xyz/openbmc_project/HMC/savearea/<filename>
```
## Proposed Design

The proposal here is to develop a standalone application on the
BMC that Inotify some directory location which creates the D-Bus
objects which represent the save area files.

If the D-bus object is already there representing the file
then update the existing D-bus object.

## Alternatives Considered
The following alternative designs were considered:

### Don't create the D-bus objects for the save area files
An application would provide the D-Bus interfaces to
- List the save area files(ListFiles)
- Read the save area file(ReadFile(fileName))
- Delete the save area file(DeleteFile(fileName))

One of the requirement is that MC needs an async event for the CRUD operation
on the save area file.

Currently, in openBMC, we don't have that infrastructure.
In openBMC, we have the existing infrastructure to send the event through
phosphor-Dbus-Monitor which monitors the D-Bus objects and send the event
if there is some property change or the interface added signal on the monitored object.

We can not use that infrastructure if we don't create the D-bus objects
for the save area files.

## Impacts
Need to see the behavior of the BMC if there are 1000 D-bus objects.
