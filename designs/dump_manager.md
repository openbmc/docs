# Dump Manager Design

Author:
  Dhruvaraj S

Primary assignee:
  Dhruvaraj S

Other contributors:

Created: 12/12/2019

## Problem Description
An event monitor mechanism will generate an error log in the event of a crash or
a failure in the host. But the size of the error log is limited to few kilobytes
so all the data from the crash or failure may not fit into an error log.
The additional data required for the debugging needs to be collected as a dump.
The existing OpenBMC dump interfaces support only the dumps generated on
the BMC and dump manager doesn't support download operations.

## Glossary

- **System Dump**: A dump of the host's main memory and processor registers.
    [Read More](https://en.wikipedia.org/wiki/Core_dump)
- **Memory Preserving Reboot(MPR)**: A method of reboot with preserving the
    contents of the volatile memory
- **PLDM**: An interface and data model to access low-level platform inventory,
    monitoring, control, event, and data/parameters transfer functions.
    [ReadMore](https://github.com/openbmc/docs/blob/master/designs/pldm-stack.md)
- **Checkstop**: A severe error inside a processor core that causes a processor
    core to stop all processing activities.
- **BMCWeb**: An embedded webserver for OpenBMC. [More Info](https://github.com/openbmc/bmcweb/blob/master/README.md)

## Background and References
Various types of dumps created based on the type and source of failure, the dump
manager which orchestrate the collection and offload needs to provide methods to
create, store the dump details, and offload it. Additionally, some sources allow
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
  created and stored by an external entity but offloaded through BMC.
  Eg: A dump created by host and stored in host but offloaed through BMC.
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

This proposal focuses on re-using the existing [phosphor-debug-collector](https://github.com/openbmc/phosphor-debug-collector), which
collects the dumps from BMC.


![phosphor-debug-collector](https://user-images.githubusercontent.com/16666879/72070844-7b56c980-3310-11ea-8d26-07d33b84b980.jpeg)

#### Brief design points of existing phosphor-debug-collector
- A create interface which assumes the type is BMC dump and returns an ID to the
  caller for the user-initiated dumps.
- An external request of dump is considered as a user initiated BMC dump and
  initiate BMC dump collection with a tool named dreport with type manual dump
- The dreport create dump file  in the dump path provided by dump manager code.
- A watch process is forked by the dump manager to catch the signal for the file
  close on that path and assume the dump collection is completed.
- The watch process calls an internal dbus interface to create the dump entry
  with size and timestamp.
- The path of dump is based on the predefined base path and the id of the dump.
- When the request comes for offload, the file is downloaded from the dump base
  path +id, no update in the entry whether dump is offloaded
- Deleting a dump by deleting the entry and internal file also will be deleted.
- There are system generated dumps based on error log or core dump, which works
  similar to user-initiated dumps with the following difference.
- No external create D-Bus interface is needed, but a monitor application will
  monitor for specific conditions like the existence of a core file or a signal
  for new error log creation of a few selected types.
- Once the event occurred, the monitor will call an internal D-Bus interface
  dump manager to create the dump with the type of dump.
- The dump manager calls dreport with a dump type got from the monitor and write
  data to a path based on dump id.

#### Updates proposed to the existing phosphor-debug-collector.
- External D-Bus interface needs to specify the type of the dump since a user
  can request multiple types of dumps
- Create will be returning an id which can be mapped to the actual dump once it
  is created.
- A Notify interface is provided for notifying the creation of a dump outside
  the BMC but offloaded through BMC.
- The InitiateOffload function will be implemented to download the dump.
- Status of the dump whether offloaded or not will be added to the dump entry.

### Dump manager interfaces.
- Dump Manager DBus object provides interfaces for creating and managing dump

- Interfaces
    - **Create**: The interface to create a dump, called by clients to initiate
      user-initiated dump.
        - Type: Type of the dump needs to be created

    - **Notify**: Notify the dump manager that a new dump is created.
        - ID: ID of the dump, if not 0 this will be the external id of the dump
        - Type: Type of dump got created.
        - Size: Size of the dump

 ### Dump entry interfaces
    -  **InitiateOffload**: Initiate the offload of the dump.
        - OffloadUri: The URL where the dump should be offloaded.

#### The properties common to all dumps
There will be a base dump entry with properties common to all types of dumps
- ID: Id of the dump
- Timestamp: Dump creation timestamp
- Size: Total size of the dump
- OffloadComplete: Set to true when offload is completed
- OffloadUri: The URL for offloading the dump, set while initiating the offload.
Specific types need to inherit from this common dump entry class
and add specific properties.

#### Additional propertries based on dump types

##### BMC Dump
- No Additional properties

##### System Dump
- External Source ID: ID provided by the host, this id will be used for all
  communication to the source of the dump, in this case, host.


### Flow of dumps collected and stored in the host
![Dump created on the host](https://user-images.githubusercontent.com/16666879/72066390-742abe00-3306-11ea-9c05-2e7dc8171abf.jpeg)

- Create: Initiate methods to create the dump in host.
- Generating the dump in host
- Host notifies the creation of dump through PLDM to BMC.
- PLDM call Notify to create the dump entry
- InitiateOffload: Dump manager request host to start offload
- The host sends the dump through PLDM, and PLDM on BMC sends it out.


## Alternatives Considered
- Offloading host dumps through host instead of BMC.

## Impacts
- The existing BMC dump interface needs to be re-used.  The current interface is
  not accepting a dump type, so a new interface to create the dump with type
  will be provided for BMC dump also without changing the existing interface.
- Modifying the BMC dump infrastructure to support additional dumps.
- openpower-boot-control will be updated to call memory preserving chip-ops and
  to handle memory preserving reboot.
- Additional system state to indicate the system is collecting debug data
  While performing memory preserving reboot.


## Testing
- Test creating host dumps and offloading it.
- Test deleting host dumps
- Run all existing test cases for BMC dump
