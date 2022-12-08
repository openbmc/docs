# Guard on BMC

## Problem Description

On systems with multiple processor units and other redundant vital resources,
the system downtime can be prevented by isolating the faulty components. This
document discusses the design of the BMC support for such isolation procedures.
The defective components can be kept isolated until a replacement. Most of the
actions required to isolate the parts will be dependant on the architecture and
taken care of by the host. But the BMC needs to support a few steps like
notifying users about the components in isolation, clearing isolation, isolating
a suspected part, or isolating when the host is down due to a severe fault. The
process of isolation is mentioned as guard in this document, which means
guarding the system from faulty components.

## Glossary

**Guard**: Guarding the system against failures by permanently isolating faulty
units.

**Guard Records**: A file in the persistent storage with the list of permanently
isolated components.

**Manual guard**: An operation to manually add a unit to the list of isolated
units. This operation is helpful in isolating a suspected component without
physically removing it from the server.

## Background and References

The guard in the servers is for managing a record of faulty components to keep
them out of service. The list of faulty but guarded components can be stored in
multiple locations based on the ownership of the component. How to store the
record or manage the record will be decided by the respective component. Some of
the examples are guard on motherboard components managed by the host, guard on
the fans can be managed by the fan control application, or the guard on the
power components can be managed by the power management application.

These records will be created when an error is encountered on an element that
can be isolated. The decision to create a record is based on the type of error,
usecase, and the availability of a redundant hardware resource to keep the
system in a usable state. The record which gets created to isolate the component
is named as Guard Record. The guard record will be deleted after a repair action
or manually by service personnel. Most of the time, the host creates the guard
record since the host is responsible for the initialization and use of the
hardware resources in a server system. The BMC creates guard records on a
limited set of units during the early boot time, after a severe fault, which
brings the host down or on the components like a power supply or fan which can
be controlled by BMC. The BMC retrieves the guard records for presenting to an
external interface for the review of customers and service personnel form
various guard record sources.

## Requirements

### Primary requirements

![Guard Usecases](https://user-images.githubusercontent.com/16666879/70852658-0edfda80-1eca-11ea-9d70-c81a690c78f2.jpeg)

- When user requests, create a record in the right guard record repository,
  based on the hardware component.
- An option should be given to user to create guard record for DIMM and
  Processor core to manually keep it out of service.
- An option should be given to the user to delete a guard record.
- An option should be given to list the guard records.
- An option should be given to delete all guard records
- Industry standard interfaces should be provided to carry out these operations
  on the guard records.

### Assumptions

- The guard records on the units which are owned by the host will be applied
  only in a subsequent boot.
- The sub-system which owns the hardware resource will apply the guard record
  and isolates the units.
- The clearing of the records after the replacement of the faulty component
  should be done by the component owning the guard records.
- There should be a mapping between key in the guard record to the key used in
  BMC for the components.

## Proposed Design

The guard proposed here is an aggregator for the guard records from various
sources and provide a common interface for creating, deleting and listing those
records.

### D-Bus Interfaces

On BMC, There will be a guard manager object and objects for each entry.

#### Guard Manager

Guard manager is for providing the external interfaces for managing the guard
records and retrieving information about guard records. The methods and
properties of the guard manager.

- Create Guard: Create guard record for a DIMM or Processor core Inputs: -
  Inventory path of the DIMM or Processor core to be guarded.

- Delete Guard: Delete an existing guard record Deleting the guard record D-Bus
  entry should delete the underlying record.

- Clear all guard: Clear all guard records in the system.

- List Guard: List all the guarded components.

**Note:** In few platforms may be the system or hardware is not in a state where
guard operation can be performed either "permanently" or "temporarily".

#### Guard Entry

The properties of each guard entry will be part of this object Properties:

- ID: Id of the record which is part of the entry object path.
- Associations:

  - Guarded hardware inventory path:
    - Forward Name must be "isolated_hw".
    - Reverse Name must be "isolated_hw_entry".
  - BMC Error Log:

    - Forward Name must be "isolated_hw_errorlog".
    - Reverse Name must be "isolated_hw_entry".

      Error Log association can be optional because a user may be tried guard
      hardware and it is not an error case.

- Severity: Type of Guard
  - Manual - Manually Guarded
  - Critical - Guarded based on a critical error
  - Warning - Guarded based on an error which is not critical, but eventually,
    there can be critical failures.
- Resolved: Status of guarded hardware

  - Used to indicate whether guarded hardware is repaired or replaced.

    **Note:** Setting this to "true" will not delete this entry because in a few
    system platforms guarded hardware records may not be deleted and used for
    further analysis.

The underlying guard function is implemented by the applications managing the
hardware units. The application which implements the common guard entry
interface should map between entry to the underlying guard record in the
original guard record store.

## Redfish interface

### Manual guard

Creating manual gurad record, set the "Enabled" property to "false" to manually
guard a unit which is present in the inventory.

#### redfish » v1 » Systems » system » Processors » CPU1

```
{
  "@odata.type": "#Processor.v1_7_0.Processor",
  "Id":view details "CPU1",
  "Name": "Processor",
  "Socket": "CPU 1",
  "ProcessorType": "CPU",
  "ProcessorId":
   {
       "VendorId": "XXXX",
       "IdentificationRegisters": "XXXX",
   } ,
   "MaxSpeedMHz": 3700,
   "TotalCores": 8,
   "TotalThreads": 16,
   "Status":
   {
        "State": "UnavailableOffline",
        "Health": "Critical"
        "Enabled": "False" <--- guarded a CPU1
   } ,
   "@odata.id":view details "/redfish/v1/Systems/system/Processors/CPU1"
}
```

### Listing the guard record

#### redfish >> v1 >> Systems >> system >> LogServices >> IsolatedHardware

#### >> Entries

```
{
  "@odata.id": "/redfish/v1/Systems/system/LogServices/IsolatedHardware/Entries",
  "@odata.type": "#LogEntryCollection.LogEntryCollection",
  "Description": "Collection of Isolated Hardware Components",
  "Members": [
    {
      "@odata.id":
"/redfish/v1/Systems/system/LogServices/IsolatedHardware/Entries/1",
      "@odata.type": "#LogEntry.v1_7_0.LogEntry",
      "Created": "2020-10-15T10:30:08+00:00",
      "EntryType": "Event",
      "Id": "1",
      "Resolved": "false",
      "Name": "Processor 1",
      "links":  {
                 "OriginOfCondition": {
                        "@odata.id": "/redfish/v1/Systems/system/Processors/cpu1"
                  }
                },
      "Severity": "Critical",
      "SensorType" : "Processor",
      "AdditionalDataURI": “/redfish/v1/Systems/system/LogServices/EventLog/attachement/111"
      “AddionalDataSizeBytes": "1024"
    }
  ],
  "Members@odata.count": 1,
  "Name": "Isolated Hardware Entries"
  }
```

## Alternatives Considered

The guard records can be created for any components which are redundant and
isolatable to prevent any damage to the hardware or data. Once the record is
created, an isolation procedure is needed to isolate the units from service.
Some of the units like which are controlled by the host can be isolated only
after a reboot, but the units controlled by BMC can be immediately taken out of
service. The alternatives are

- The host creates, applies, and present guard records: In this case, BMC has no
  control, and the host needs to provide the user interface, so there may not be
  a common interface across different types of hosts. Different user interfaces
  are required for guard records created by BMC and host.
- BMC manages the external interfaces for guard: There will be one common point
  for presenting or managing the guard records created by multiple hosts or BMC
  itself. There are some guard records created after a severe failure in the
  host; as a system control entity, it will be easier for BMC to handle such
  situations and create the records.

## Impacts

- The guard records will be presented as an extension to logs
- Redfish implementation will have an impact to do the operations required for
  the guard record management by the user. A request for standardization is
  planned for the method to list the isolated units in the redfish since that is
  not yet available in the redfish standard.

## Testing

The necessary tests needed are creating, deleting, and listing the guard records
and that should be automated, further tests on the isolation of each type of the
unit is implementation-specific.
