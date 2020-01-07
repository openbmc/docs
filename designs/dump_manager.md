# Dump Manager Design

Author:
  Dhruvaraj S

Primary assignee:
  Dhruvaraj S

Other contributors:

Created: 12/12/2019

## Problem Description
A crash or event monitor mechanism will generate an error log in the event of a
crash or failure.  But the size of the error log is limited to few kilobytes,
so all the data from the crash or failure may not fit into an error log.  The
additional data required for the debugging needs to be collected as a dump. The
dump content varies by source of the issue. An orchestrator is necessary to
obtain the dump from various sources based on the failures and offload it to an
external tool which collects these dumps.

## Glossary

- **System Dump**: A dump of the main memory and registers. [Read More](https://en.wikipedia.org/wiki/Core_dump)
- **Memory Preserving Reboot(MPIPL)**: A method of reboot with preserving the
    contents of the volatile memory
- **BMC**: A special service processor that monitors the physical state of the
    computer, network, and storage units.
- **PLDM**: An interface and data model to access low-level platform inventory,
    monitoring, control, event, and data/parameters transfer functions.
    [ReadMore](https://github.com/openbmc/docs/blob/master/designs/pldm-stack.md)
- **PHAL**: Power Hardware Abstraction Layer provides an abstraction for
    OPENPower Hardware in OpenBMC.
- **Self Boot Engine(SBE)**: SBE is the first processor runs when the CPU is
    powered up in OpenPOWER based processors, and it is responsible for
    initializing the main cores in the processor.
- **Hostboot**: The hostboot firmware performs all processor, bus, and memory
    initialization within OpenPOWER based systems. It prepares the hardware to
    load and run an appropriate payload and provides runtime services to the
    payload. [Read More](https://github.com/openbmc/docs/blob/master/designs/pldm-stack.md)
- **Checkstop**: A severe error inside a processor core that causes a processor
    core to stop all processing activities.
- **Dreport**: A tool to capture debug-data in OpenBMC.
- **BMCWeb**: An embedded webserver for OpenBMC. [More Info](https://github.com/openbmc/bmcweb/blob/master/README.md)

## Background and References
Various types of dumps created based on the type and source of failure, the dump
manager which orchestrate the collection and offload needs to provide methods to
create, store the dump details and offload it. Additionally, some sources allows
the dump to be extracted manually without a failure to understand the current
state or analyze a suspected problem.


## Requirements

![Dump usecases](https://user-images.githubusercontent.com/16666879/70888651-d8f44080-2006-11ea-8596-ed4c321cfaa6.png)
#### Dump manager needs to provide interfaces for
- Create a dump: Initiate the creation of the dump, based on an error condition
  or a user request
- List the dumps: List all dumps present in the BMC.
- Get a dump: Offload the dump to an external entity.
- Notify: Notify the dump manager that a new dump is created in the case dump is
  created asynchronously
- Delete the dump.
- Mark a dump as offloaded.
- Set the dump policies.

## Proposed Design
There are various types of dumps, interfaces are standard for most of the dumps,
but huge dumps which cannot be stored on the BMC needs additional support.
This document will explain the design of different types of dumps. The dumps are
classified based on where it is collected, stored, and how it is extracted. Two
major types are

- Collected by BMC and stored on BMC.
- Collected by Host and stored in Host memory.

### Dump manager interfaces.
- Dump Manager DBus object provides interfaces for creating and managing dump

- Interfaces
    - Initiate
        - Type
        - Errorlog Id
    - Create:
        - Type
        - Errorlog Id
        - Return: ID
    - Get:
        - ID
        - Type
    - Notify:
        - ID
        - Type
        - Size
    - Policy
        - Dump Overwrite Policy.

A dump entry dBus object with the following fields will be created for each dump
created.
- ID: Id of the dump
- Type: Type of the dump
- Timestamp: Dump creation timestamp
- Error log: Identifier to the failure, 0 for manually created dumps.
- Size: Total size of the dump
- Status: Status of the dump: In progress, Ready to Offload, Requested for
  offloading and Offloaded

### Flow of dumps collected and stored on BMC
![Synchrousous dump](https://user-images.githubusercontent.com/16666879/71806586-29534100-308f-11ea-80b2-c0f5180071f9.jpeg)

- Create dump: An external user or application calls dump manager to create a
  dump with type and error log.
- Collect Dump: Dump manager calls respective modules based on the type of dump
  to execute the collection.
- The collected dump get stored to a file in BMC.
- Dreport collects the dump file based on the file create notification and
  package it.
- Dreport notifies dump manager and creates a new entry for the created dump.
- Get: Request for offloading the dump
- Delete: Delete entry, the file will be removed.

### Flow of dumps collected and stored in host
![Async Dump flow](https://user-images.githubusercontent.com/16666879/71807797-23ab2a80-3092-11ea-9220-48a6d404da71.jpeg)

- Initiate dump: A dump will be initiated asynchronously
- Generating the dump in host
- Host notifies the creation of dump
- PLDM call Notify to create the dump entry
- Get: Initiate a dump download, dump manager sets the status of the dump entry
  to requested to download and configure the dump NBD path.
- PLDM sends a request to the host to download the dump.


### System dump flow in detail

The system dump is a disruptive dump collected while a fatal error encountered
in the hypervisor. The system needs to be restarted during such failures; the
hardware states and main memory will be collected as part of this dump.

The System dump creation: Once the system dump collection is initiated system
will do a memory preserving reboot to collect the hardware data and main memory.
Refer to the following diagram for the steps followed while starting the dump.
![Memory Preserving Reboot Flow](https://user-images.githubusercontent.com/16666879/70892490-44daa700-200f-11ea-96fc-335fa6d0bcb4.png)

Dump Offload: The system dump gets collected by the host and reside in the host
memory. The dump will be offloaded through the PLDM interface, which
communicates to the host an BMCWeb, which is for connecting to external tools.

![Dump Offload flow](https://user-images.githubusercontent.com/16666879/70892576-75badc00-200f-11ea-90fb-b9993095ff50.png)



## Alternatives Considered


## Impacts
- Existing BMC dump interface needs to be re-used.  The current interface is not
  accepting a dump type, so a new interface to create the dump with type will be
  provided for BMC dump also without changing the existing interface.
- Modifying the BMC dump infrastructure to support additional dumps.
- openpower-boot-control will be updated to call memory preserving chip-ops and
  to handle memory preserving reboot.
- Additional system state to indicate the system is collecting debug data while
  performing memory preserving reboot.


## Testing
- Test creating different types of dumps.
- Test memory preserving reboot.
- Test listing of the dump and downloading a dump.
