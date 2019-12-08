# GARD requirements and design

## Problem Description
The OpenPOWER based systems have multiple processors and memory associated with
each of these processing units. The processing unit has multiple cores, memory,
and IO interfacing units. There can be failures on these units due to a hard
error, which can cause an outage or soft errors - which may eventually become
hard errors. The outage of the system can be limited by isolating these faulty
components and keeping them out of service until a repair. The units will be kept
out of service when an error is encountered. Some units can be manually marked
for isolation in the case of suspected error condition. A record for each faulty
unit will be maintained until the replacement. The role of gard on the BMC is to
manage thses records.

## Glossary
**GARD**: Short form for guard, guarding the system from failures by permanently
isolating faulty units.
**GARD Records**: A file in the persistent storage with the list of permanently
isolated units.


## Background and References
The GARD in the OpenPOWER based servers are for managing a record of faulty
components to keep them out of service.  These records will be created when an
error is encountered on a unit which is gardable. The decision to create a
record is based on the type of error, usecase and the availability of a
redundant hardware resource to keep the system in a usable state. The record
will be deleted after a repair action or manually by service personnel. In
OpenPOWER based servers, the BMC creates gard records on a limited set of units
during the early boot time or after a system checkstop. The BMC should be able
to retrive the gard records for presenting to external interfaces for the review
of customers or service personnel.

## Requirements
### Primary requirements
![Gard Usecases](https://user-images.githubusercontent.com/16666879/70852658-0edfda80-1eca-11ea-9d70-c81a690c78f2.jpeg)
- Create a record in the common records based on an external request.
- The records should not be altered when the host is running.
- An option should be given to the user to create or delete a gard record.
- An option should be given to list the gard records.
- An option should be given to delete all gard records
### Assumptions
- The host applies the gard record and isolate the units.
- The host owns the gard record, and BMC updates it only when the host is not
  running.
- Host clears the gard record for repaired units.
- Device tree will contain all the gardable units with Entity Path as an
  attribute.
### Open issues
- Do we need Redfish interfaces for create, delete, and query gard records?
- Translating the unit id in host to an ID understandable by BMC
## Proposed Design
The gard contains both gard record handling and a set of interfaces for other
components to manipulate gard records
### Gard Record
A gard record is the information about failing units, and it consists of the
following fields.
  - Unit ID - An ID to the unit being garded
  - Error Log ID - ID of the error which caused gard
  - Error Type - The type of gard, e.g., FATAL, Manual
  - Serial Number - Serial Number of the failing unit.

### Gard Interfaces
- Create Gard: Create gard record for a unit
       Inputs:
           - ID of the unit to be garded
           - Error log id, 0 for manual gard
           - Gard error type
- Delete Gard: Delete an existing gard record
       Inputs:
          - ID of the device to delete gard record

- Clear all gard: Clear all gard records in the system.

- List Gard:  List all the garded units

### The flow
On BMC, there will be a gard manager dBus object and entries for each gard
record. The updates and deletes to gard record from the BMC will be done through
the dBus calls, but the updates can happen by host too. Which needs to be
reflected in the dBus object.

BMC Updating or deleting a gard record
![BMC updating or deleting gard record](https://user-images.githubusercontent.com/16666879/70852661-130bf800-1eca-11ea-9099-9bc982b4bf35.jpeg)

Host updating or deleting a gard record
![Host updating or deleting gard records](https://user-images.githubusercontent.com/16666879/70852662-169f7f00-1eca-11ea-9432-ba24bfa2d690.jpeg)

### Command line interface
A command line inteface is provided to list the gard records, create a new record, delete a record and clear all records.

- List all records
	- Each record will be shown with following details
		- Device Tree ID of the unit
		- Physical Path as string
		- Gard Error Type
		- Error Log ID associated with the record
		- Serial number of the failing unit.
- Create a new record
	- Create a record for given Device Tree ID
- Delete a gard record
	- Delete the record with Device Tree ID of the unit
- Clear all records

## Alternatives Considered

## Impacts

## Testing
