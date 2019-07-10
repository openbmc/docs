# Save Area Management through the REST interface.
Author: Ratan Gupta <ratagupt@linux.vnet.ibm.com>

Primary assignee: Ratan Gupta

Created: 2019-07-10

## Problem Description

Each partition data will be saved as save area file in the BMC file system.

partition data is not being stored on the host firmware due to
following reason.
- host firmware doesn't have storage
- host firmware doesn't have interfaces to manage the files.

In a fully configured system, there can be 1000 partitions which will require
an equal number of save area files BMC should provide the interface for CRUD
(create/read/update/delete) operations for save area files.

BMC should notify other management consoles which is connected to the BMC
in case if there is any change in the save area files.

This shall be done via asynchronous event handling mechanisms.

BMC doesn't need to understand or parse these files, BMC will act as a storage
for the save area files.

OpenBMC is moving to [Redfish] as its standard for out of band management.
Redfish doesn't have the schema files for save area management, this is a
kind of IBM specific requirement.

#### Use cases:

- When the virtualization firmware is down, the management console needs to
  create the logical partitions on a destination system using the
  configuration details stored in the save area files.

- Restore configuration using the save area files when the system is in an error
  state. The system may have up to 1,000 LPARs that need to be restored.

The goal of this document is to define the REST interfaces for the save area
files.

## Requirements
- BMC shall provide 15MB reserved space for save area files.
- list of all the save area files.
- read a specific save area file.
- delete a specific save area file.
- create/update of the save area file.
- Send the event notification to the connected clients(MC) for
  creation/deletion/updation of the save area file.

## Rest Interfaces

### List of all the save area files
The following interface would list all the save area files.
```
GET https://${bmc}/ibm/powerhmc/v1/host/partitions/enumerate
```
### Create/upload the save area file
The following interface would create/upload the save area file.
```
PUT https://${bmc}/ibm/powerhmc/v1/host/partitions/<filename> <filedata as octet
stream>
```
We have two ways

- Make this blocking API, and returns once it writes the file
                  or
- Return it immediately saying that opertaion in progress, then
  send the event once the file has been created.


### Delete the save area file
Deletion of savearea file will be supported and it will be initiated
by the connected client by issuing the following request.
```
DELETE https://${bmc}//ibm/powerhmc/v1/host/partitions/<filename>
```
This will delete the save area file from the persistent location.
Application which is Inotifing to the directory location will delete the
corresponding D-bus object.


## Read the save area file
Save area file can be read by the following request.
```
GET https://${bmc}/ibm/powerhmc/v1/host/partitions/<filename>
```
## Proposed Design

The proposal here is to at the startup of the bmcweb,
- some module in bmcweb will Inotify the location having these files
- Monitor the file system events.
- Send the Redfish notification to the connected clients
  about the save area updates.

To support all the CRUD opeartion on the save area file

## List the save area files:
Write the functionality as part of bmcweb route handling.
Iterate over the location having the files.
Preprare a rest response which have list of the save area resource path.

## Read the save area file
Write the functionality as part of bmcweb route handling.
- Verify the file exist.
- Read the file from the persistent location and send
  it as part of response.

## Delete the save area file
Write the functionality as part of bmcweb route handling.
- Verify the file exist.
- Delete the file from the persistent location.

## Alternatives Considered
The following alternative designs were considered:

### create seprate D-Bus app to do CRUD opeartions on the save area files

An application would provide the D-Bus interfaces to
- Read the save area file.
- Write the save area file.
- Create/update the save area file.
- Delete the save area file(DeleteFile(fileName))

