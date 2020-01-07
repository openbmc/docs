# Dump Manager Design

Author:
  Dhruvaraj S

Primary assignee:
  Dhruvaraj S

Other contributors:

Created: 12/12/2019

## Problem Description
An event monitor mechanism will generate an error log in the event of a crash or
a failure in the host.  But the size of the error log is limited to few
kilobytes, so all the data from the crash or failure may not fit into an error
log.  The additional data required for the debugging needs to be collected as a
dump. The existing OpenBMC dump interfaces supports only the dumps generated on
the BMC and doesnt support download operations.

## Glossary

- **System Dump**: A dump of the host's main memory and registers. [Read More](https://en.wikipedia.org/wiki/Core_dump)
- **Memory Preserving Reboot(MPR)**: A method of reboot with preserving the
    contents of the volatile memory
- **PLDM**: An interface and data model to access low-level platform inventory,
    monitoring, control, event, and data/parameters transfer functions.
    [ReadMore](https://github.com/openbmc/docs/blob/master/designs/pldm-stack.md)
- **PHAL**: Power Hardware Abstraction Layer provides an abstraction for
    OPENPower Hardware in OpenBMC.
- **Self Boot Engine(SBE)**: SBE is the first processor runs when the CPU is
    powered up in OpenPOWER based processors, and it is responsible for
    initializing the main cores in the processor.
- **Hostboot**: The hostboot firmware performs all processor, bus, and memory
    Initialization within OpenPOWER based systems. It prepares the hardware to
    load and run an appropriate payload and provides runtime services to the
    payload. [Read More](https://github.com/openbmc/docs/blob/master/designs/pldm-stack.md)
- **Checkstop**: A severe error inside a processor core that causes a processor
    core to stop all processing activities.
- **Dreport**: A tool to capture debug-data in OpenBMC.
- **BMCWeb**: An embedded webserver for OpenBMC. [More Info](https://github.com/openbmc/bmcweb/blob/master/README.md)

## Background and References
Various types of dumps created based on the type and source of failure, the dump
manager which orchestrate the collection and offload needs to provide methods to
create, store the dump details, and offload it. Additionally, some sources allows
the dump to be extracted manually without a failure to understand the current
state or analyze a suspected problem.


## Requirements

![Dump usecases](https://user-images.githubusercontent.com/16666879/70888651-d8f44080-2006-11ea-8596-ed4c321cfaa6.png)
#### Dump manager needs to provide interfaces for
- Create a dump: Initiate the creation of the dump, based on an error condition
  or a user request.
- List the dumps: List all dumps present in the BMC.
- Get a dump: Offload the dump to an external entity.
- Notify: Notify the dump manager that a new dump is created in the case dump is
  created asynchronously
- Delete the dump.
- Mark a dump as offloaded to an external entity.
- Set the dump policies like disabling a type of dump or dump overwriting policy.

## Proposed Design
There are various types of dumps; interfaces are standard for most of the dumps,
but huge dumps which cannot be stored on the BMC needs additional support.
This document will explain the design of different types of dumps. The dumps are
classified based on where it is collected, stored, and how it is extracted. Two
major types are

- Collected by BMC and stored on BMC.
- Collected by Host and stored in Host memory.

This proposal focuses on re-using the existing phosphor-debug-collector, which
collects the dumps from BMC.

#### Brief design points of existing phosphor-debug-collector
![phosphor-debug-collector](https://user-images.githubusercontent.com/16666879/72070844-7b56c980-3310-11ea-8d26-07d33b84b980.jpeg)
- A create interface which assumes the type is BMC dump and returns an ID to the
  caller for the user-initiated dumps.
- An external request of dump is considered as a manual dump and initiate manual
  dump collection with a tool named DReport with type manual dump.
- DReport writes the dump content to a file path with which contains the id of
  the dump.
- A watch process is forked by the dump manager to catch the signal for the file
  close on that path and assume the dump collection is completed.
- The watch process calls an internal dbus interface to create the dump entry
  with size and timestamp.
- The path of dump is based on the predefined base path and the id of the dump.
- When the request comes for offload, the file is downloaded from the dump base
  path +id, no update in the entry whether dump is offloaded
- Deleting a dump by deleting the entry and internally file also will be deleted.
- There are system generated dumps based on error log or core dump, which works
  similar to user-initiated dumps with the following difference.
- No external create D-Bus interface is needed, but a monitor application will
  monitor for specific conditions like the existence of a core file or a signal
  for new error log creation.
- Once the event occurred, the monitor will call an internal D-Bus interface
  dump manager to create the dump with the type of dump.
- The dump manager calls dreport with a dump type got from the monitor and write
  data to a path based on dump id.

#### Updates proposed to the existing phosphor-debug-collector.
- External D-Bus interface needs to specify the user can request the type of the
  dump since multiple types of dumps.
- Create will be returning an id which can be mapped to the actual dump once it
  is created.
- The internal D-Bus interface, which accepts type and size, will be external
  and renamed to Notify, and the dump entry will be created after a notify.
- The Delete will delete the entry but not the file, the file cleanup should be
  taken care separately.
- Unlike now, the GET function will be implemented to download the dump.
- Status of the dump will be added to the entry.

### Dump manager interfaces.
- Dump Manager DBus object provides interfaces for creating and managing dump

- Interfaces
    - **Create**: The interface to create a dump, called by clients to initiate
      user initiated dump.
        - Type: Type of the dump needs to be created

    - **Notify**: Notify the dump manager that a new dump is created.
        - ID: ID of the dump, if not 0 this will be the external id of the dump
        - Type: Type of dump got created.
        - Size: Size of the dump
    -  **Get**: Get or download the dump
	    - ID of the dump

A dump entry dBus object with the following fields will be created for each dump
created.
- ID: Id of the dump
- Type: Type of the dump
- Timestamp: Dump creation timestamp
- Size: Total size of the dump
- Source ID: Id provided by the source of the dump, if any.
- Reason: Reason for the dump creation
	- Manual: A manually initiated dump.
	- System: A system generated dump.
- Status: Status of the dump
	- Created: The initial state after creating entry with notify
	- OffloadComplete: Sets when the offload is completed

### Flow of dumps collected and stored on BMC

A generic flow for a open power specifc dump which gets collected on BMC, The
flow fits into the existing dump manager implemenation.
![Dump collected on BMC](https://user-images.githubusercontent.com/16666879/72232086-fce67a00-35e4-11ea-8d2b-8a3fe9a5a3d2.jpeg)

![Dumps collected on BMC](https://user-images.githubusercontent.com/16666879/72065999-8bb57700-3305-11ea-9ce5-4455d9fe2cd1.jpeg)

- Create dump: An external user calls dump manager to create a
  dump with type.
- OpenPOWER applications call OpenPOWER specific dump creation interface.
- Collect Dump:  OpenPOWER Dump manager calls respective modules based on the
  type of dump to execute the collection.
- Store the dump on BMC and create an error log with specific error type.
- Dreport collects the dump file based on the error log creation and type of
  error.
- Dreport notifies dump manager and creates a new entry for the created dump.
- Get: Request for offloading the dump
- Delete: Delete entry, the file will be removed immidiatly if only one dump is
  allowed at a time.

### Flow of dumps collected and stored in host
![Dump created on the host](https://user-images.githubusercontent.com/16666879/72066390-742abe00-3306-11ea-9c05-2e7dc8171abf.jpeg)

- Create: Set the diag mode target to create the dump
- Generating the dump in host
- Host notifies the creation of dump
- PLDM call Notify to create the dump entry
- Get: Initiate a dump download, dump manager sets the status of the dump entry
  to requested to download and configure the dump NBD path.
- PLDM sends a request to the host to download the dump.

### Types of Dumps
- System Dump - A disruptive dump of host memory and hardware states, collected
  by the host and stored in the host memory. A special memory preserving reboot
  is needed to complete this dump.
- SBE Dump - Dump collected from SBE when it encountered an error or it is
  unresponsive. Dump is collected by BMC and stored in the BMC.
- Checkstop Dump - Dump collected after a checkstop condtion, it is collected by
  BMC and stoted in the BMC
- BMC Dump - Dump of the debug information on the BMC.

### System dump flow in detail

The system dump is a disruptive dump collected while a fatal error encountered
in the hypervisor. The system needs to be restarted during such failures; the
hardware states and main memory will be collected as part of this dump.

The System dump creation:
- Attention sets the memory preserve reboot target
- This triggers memory preserve reboot service and calls openpower-proc-control
  enter_mpipl to start memory preserving reboot
- Once the powering off completes, MPIPL poweron target is set.
- Openpower-proc-control startHost_mpipl is called to executed special istep0
  for memory preserving reboot.
Refer to the following diagram for the steps followed while starting the dump.
![Memory Preserving Boot](https://user-images.githubusercontent.com/16666879/72066712-4bef8f00-3307-11ea-8098-14fab668f859.jpeg)

Dump Offload: The system dump gets collected by the host and reside in the host
memory. The dump will be offloaded through the PLDM interface, which
communicates to the host an BMCWeb, which is for connecting to external tools.


![Dump offload](https://user-images.githubusercontent.com/16666879/72532628-c1c0a100-3899-11ea-8e6f-277ea4cf9a7f.jpeg)

## Alternatives Considered
- New dump manager in open power for all OpenPOWER specific dumps
- All dump creates are handled by phosphor dump manager.

## Impacts
- Existing BMC dump interface needs to be re-used.  The current interface is not
  accepting a dump type, so a new interface to create the dump with type will be
  provided for BMC dump also without changing the existing interface.
- Modifying the BMC dump infrastructure to support additional dumps.
- openpower-boot-control will be updated to call memory preserving chip-ops and
  to handle memory preserving reboot.
- Additional system state to indicate the system is collecting debug data
  While performing memory preserving reboot.


## Testing
- Test creating different types of dumps.
- Test memory preserving reboot.
- Test listing of the dump and downloading a dump.
