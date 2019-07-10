# File Management through the REST interface.
Author: Ratan Gupta <ratagupt@linux.vnet.ibm.com>

Primary assignee: Ratan Gupta

Created: 2019-07-10

## Problem Description

Each partition data will be saved as partition file in the BMC file system.

partition data is not being stored on the host firmware due to
the following reason.
- host firmware doesn't have storage
- host firmware doesn't have interfaces to manage the files.

In a fully configured system, there can be 1000 partitions which will require
an equal number of partition files BMC should provide the interface for CRUD
(create/read/update/delete) operations for partition files.

BMC should notify other management consoles which are connected to the BMC
in case if there is any change in the partition files.

This shall be done via asynchronous event handling mechanisms.

BMC doesn't need to understand or parse these files, BMC will act as a storage
for the partition files.

OpenBMC is moving to [Redfish] as its standard for out of band management.
Redfish don't have the schema files for file management, this is a
kind of IBM specific requirement.

#### Use cases:

- When the virtualization firmware is down, the management console needs to
  create the logical partitions on a destination system using the
  configuration details stored in the partition files.

- Restore configuration using the partition files when the system is in an error 
  state. The system may have up to 1,000 LPARs that need to be restored.

The goal of this document is to define the REST interfaces for the files.


## Requirements
- BMC shall provide 15MB reserved space for the partition files.
- Individual partition file size can not be more than 10KB.
- list of all the partition files.
- read a specific partition file.
- delete a specific partition file.
- create/update of the partition file.
- Send the event notification to the connected clients(MC) for
  creation/deletion/updation of the partition file.

## Rest Interfaces

### List of all the partition files
The following interface would list all the partition files.
```
GET https://${bmc}/ibm/v1/host/partitions/enumerate
```
- Success – The JSON response with 200 OK status. The data field will
  contain the objects which represents the partition files.
```
{
  "data": ["ibm/v1/host/partitions/<objectID>",
           "ibm/v1/host/partitions/<objectID>",
           ...],
  "message": "200 OK",
  "status": "ok"
}
```
- Failure – The JSON response with 4xx or 5xx status. The message field
  will contain the failure reason description. For example :
```
{
  "data": {
    "description": "No JSON object could be decoded"
  },
  "message": "400 Bad Request",
  "status": "error"
}
```
### Create/upload the partition file
The following interface would create/upload the partition file.
```
PUT https://${bmc}/ibm/v1/host/partitions/<filename> <filedata as octet
stream>
```
We have two ways

- Make this blocking API, and returns once it writes the file
                  or
- Return it immediately saying that operation in progress, then
  send the event once the file has been created.
- As the individual file can not be more than 10KB so go with first
  approach for simplicity.
- Return error if the file size is greater then 10KB.

- Success – The JSON response with 200 OK status.
```
{

 "data": {
    "description": "File created"
  },

  "message": "200 OK",
  "status": "ok"
}
```
- Failure – The JSON response with 4xx or 5xx status. The message field will
  contain the failure reason description. For example :
```
{
  "data": {
    "description": "No JSON object could be decoded"
  },
  "message": "4XX Bad Request",
  "status": "error"
}
```
### Delete the partition file
Deletion of partition file will be supported and it will be initiated
by the connected client by issuing the following request.
```
DELETE https://${bmc}//ibm/v1/host/partitions/<filename>
```
This will delete the partition file from the persistent location.
The application which is Inotifing to the directory location
will delete the corresponding D-bus object.

- Success – The JSON response with 200 OK status.
```
{
 "data": {
    "description": "File deleted"
  },

  "message": "200 OK",
  "status": "ok"
}
```
- Failure – The JSON response with 4xx or 5xx status. The message field will
  contain the failure reason description. For example :
```
{
  "data": {
    "description": "No JSON object could be decoded"
  },
  "message": "4XX Bad Request",
  "status": "error"
}
```
### Read the partition file
Partition file can be read by the following request.
```
GET https://${bmc}/ibm/v1/host/partitions/<filename>
```
- Success – The JSON response with 200 OK status. The data field will contain
  the actual file data.
```
{
  "data": "<actual file data>",
  "message": "200 OK",
  "status": "ok"
}
```
- Failure – The JSON response with 4xx or 5xx status. The message field will
  contain the failure reason description. For example :
```
{
  "data": {
    "description": "No JSON object could be decoded"
  },
  "message": "400 Bad Request",
  "status": "error"
}
```
## Proposed Design

The proposal here is to at the startup of the bmcweb,
- some module in bmcweb will Inotify the location having these files
- Monitor the file system events.
- Send the Redfish notification to the connected clients
  about the partition file updates.

## List the partition files:
Write the functionality as part of bmcweb route handling.
- Iterate over the location having the files.
- Prepare a rest response which has a list of the partition files.

## Read the partition file
Write the functionality as part of bmcweb route handling.
- Verify the file exist.
- Read the file from the persistent location and send
  it as part of the response.

## Delete the partition file
Write the functionality as part of bmcweb route handling.
- Verify the file exist.
- Delete the file from the persistent location.

## Alternatives Considered
The following alternative designs were considered:

### create separate D-Bus app to do CRUD operations on the partition files

An application would provide the D-Bus interfaces to
- Read the partition file.
- Write the partition file.
- Create/update the partition file.
- Delete the partition file(DeleteFile(fileName))

