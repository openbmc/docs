# GUARD requirements and design

## Problem Description
On systems with multiple processor units and other redundant vital resources,
the system downtime can be prevented by isolating the faulty components. This
document discusses the design of the BMC support for such isolation procedures.
The defective components can be kept isolated until a replacement. Most of the
actions required to isolate the parts will be dependant on the architecture and
taken care by the host. But the BMC needs to support a few steps like notifying
users about the isolated components, clearing isolation, isolating a suspected
part, or isolating when the host is down due to a severe fault. The process of
isolation is mentioned as GUARD in this document, which means guarding the
system from faulty components.

## Glossary
**GUARD**: Guarding the system against failures by permanently isolating faulty units.
**GUARD Records**: A file in the persistent storage with the list of permanently
isolated components.


## Background and References

The GUARD in the servers are for managing a record of faulty components to keep
them out of service.  These records will be created when an error is encountered
on an element that can be isolated. The decision to create a record is based on
the type of error, usecase, and the availability of a redundant hardware
resource to keep the system in a usable state. The record which gets created to
isolate the component is named as Guard Record. The Guard Record will be deleted
after a repair action or manually by service personnel. Most of the time,
the host creates the Guard Record since the host is responsible for the
initialization and use of the hardware resources in a server system. The BMC
creates guard records on a limited set of units during the early boot time or
after a severe fault, which brings the host down. The BMC retrieves the guard
records for presenting to external interfaces for the review of customers and
service personnel.

## Requirements
### Primary requirements
![Guard Usecases](https://user-images.githubusercontent.com/16666879/70852658-0edfda80-1eca-11ea-9d70-c81a690c78f2.jpeg)
- Create a record in the guard records owned by the host, based on an external
  request.
- The guard records should not be altered when the host is running.
- An option should be given to the user to create or delete a guard record.
- An option should be given to list the guard records.
- An option should be given to delete all guard records
### Assumptions
- The host applies the guard record and isolates the units.
- The host owns the guard record, and BMC updates it only when the host is not
  running.
- The host clears the guard record for repaired units.
- There should be a mapping between key in the guard record to the key used in
  BMC for the components.
### Open issues
- Do we need Redfish interfaces to create, delete, and query guard records?
- Translating the component id in host to an ID understandable by BMC
## Proposed Design
The guard contains both guard record handling and a set of interfaces for other
components to manipulate guard records
### Guard Record
The host owns the guard record format, and these are the minimum information
required to present to users.
  - Unit ID - An ID to the component being guarded
  - Component Name - A human-readable name of the element.
  - Error Log ID - ID of the error which caused the guard
  - Error Type - The type of guard.
  - Serial Number - Serial Number of the failing component.

### Guard Interfaces
- Create Guard: Create guard record for a unit
       Inputs:
           - ID of the unit to be guarded
           - Error log id, 0 for manual guard
           - Guard error type
- Delete Guard: Delete an existing guard record
       Inputs:
          - ID of the device to delete guard record

- Clear all guard: Clear all guard records in the system.

- List Guard:  List all the guarded components.

### Redfish Interface
Add the list of faulty units as part of log services
```json
"LogEntryTypes": {
			"enum": [
			"Event",
			"SEL",
			“OEM",
			“Multiple”,
			“Guard”
] }
```
Type of guard errors added as part of OEM
```
“OEM”: {
	“GuardError”: {
		“enum”: [
			“Manual",
			"Predictive”
			"Fatal”
	] }
}
```
TBD
### D-Bus Intrefaces
On BMC, There will be a guard manager object and objects for each entries.
#### Guard Manager
Guard manager creates and manages each guard entries, the main methods are
- Create : Create a guard record
	- ID: Id of the unit
#### Guard Entry
The properties of each guard entry will be part of this object
Properties:
	- ID: Id of the record
	- UnitID: Id of the component
	- Name: Name of the component
	- Type: Type of Guard
	- SerialNumber: Serial Number of the failing component.

#### Guard Library
The guard library provides functions to manage underlying guard actions.
The interfaces are
```
  createGuardRecord(componentId, guarderrorType, errodId)
  deleteGuardRecord(componentId)
  clearGuardRecords()
  getGuardRecords(GuardRecordList)
```
#### Guard Command line
Guard command line provides the capability to users to manage guard record
directly on the BMC, following functions are provided

- Manually guard a component
	- guard create --id=<id>
- Delete a guard record
	- guard delete --id=<id>
- Delete all guard records
	- guard clear
- List guard records
	$ guard list
```
	GUARDed Components
     ID              Name                 ERROR.        TYPE      Serial
  0) 50000     		/sys-0/Node-0/Proc-0  500386D7  Fatal.    ABCD1234
  1) D0001   		/sys-0/Node-0/Dimm-1  0         Manual.   CDEF1234
```
## Alternatives Considered

## Impacts

## Testing
- Create/delete/clear/list guard records using command line
- Make fault to create system guard records
- Test Create/delete/clear/list guard records using redfish.
