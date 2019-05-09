# IBM Platform Event Log Design

Author:
  Matt Spinler <mspinler!>

Primary assignee:
  Matt Spinler

Other contributors:
  None

Created:
  May 8, 2019

## Problem Description
IBM systems must support its Platform Event Log (PEL) format on the BMC.
This includes:
* Generating events in that format.
* Sending them up to the host via PLDM commands
* Emitting a Redfish representation of them.
* Including PELs sent down from the host in these outputs.

The desire is to still use the phosphor-logging APIs as if creating the
`xyz.openbmc.Logging.Entry` objects, though part of this work is to create a
new phosphor-logging API to allow one to pass in extra data that will be part
of a PEL but can just be ignored if the code isn't configured to use it.

The intent of this document is to describe how the PEL handler interacts with
the rest of the world, and not on the internal implementation of the handler
itself.

## Background and References
A PEL entry consists of sections, some required and some optional, that can
describe several types of events - error, customer attention, user action, OS
notification, and informational. For example, there is a FRU call-out section
that contains which pieces of hardware need to be replaced to resolve the
error.

PELs have the ability to contain FFDC (first failure data capture) data for the
event.

A PEL is designed to be stored and transferred in its flattened state - i.e
as a raw array of bytes, and then can be parsed when necessary by finding
sections via eyecatchers, offsets, and size fields. The maximum size of PEL
is 16KB, though some subsystems may support less.

Further details on the contents of a PEL are beyond the scope of this document.

## Requirements
* PEL specific code should not be present in any common OpenBMC code, unless it
  can be compiled out.

* Code must still use the phosphor-logging APIs for creating events, even
  if only the PEL form of the event is important.  Due to the requirement of
  keeping PEL code out of common code, these logging calls should be able to
  ignore any extra data added for PELs if desired.

* Event logs created by common OpenBMC code must be converted into PEL logs.
  This will most likely require something like an IBM message registry that
  contains the hardcodable PEL fields that are outside the scope of
  the `xyz.openbmc_project.Logging.Entry` interface.

* The lifetime of a PEL must match the lifetime of the OpenBMC event log.

* The code that manages PELs must also manage the retention policy for the
  phosphor-logging logs.

* The host firmware needs the BMC to send it the PELs created on the BMC.
  (See sequence diagrams below.) This drives the following requirements.
  * The PEL code must call a specific PLDM command when a PEL is created
    on the BMC to tell the host a new log is available to retrieve.
  * There must be an API to retrieve a PEL by PEL ID.
  * There must be APIs on the PEL handler for 2 notifications that come
    down from the host:
      1) That the host firmware has received a specific PEL.
      2) That the operating system has received that PEL.

    This status affects if/when the PEL handler needs to resend PELs,
    however the details of that are outside the scope of this design.

* The host firmware, including PowerVM and hostboot, will send down their
  own PELs for the BMC to store.  This drives the following requirements:
    * An OpenBMC event log must be created for this PEL.
      * There must be a way to pass a full PEL to the PEL handler via the
        phosphor-logging API.
    * The PEL handler must update the following fields in these PELs:
        1) The commit timestamp
        2) The PEL ID
    * The PEL handler must then treat this PEL the same as any other PELs,
      including notifying the host that it just received a new PEL.

* There must be a way to display PEL fields in some type of Redfish log.
  * The Redfish format has not been finalized yet, so the requirements
    from this will be handled outside of this document.

* There must be a way to delete a PEL log, given its ID.

### Possible Future Requirement
* It still isn't quite decided yet if systems that employ PELs still need
  the `xyz.openbmc_project.Logging.Entry` objects on D-Bus at all.  If not,
  then some of the above requirements about matching log lifetimes go away.
  Even if this is the case, the same APIs will still be used for creating
  the logs.

## Proposed Design

* Add extension support to phosphor-logging.  This creates a framework for
  code that manages other log types to reside in that repository and allow
  that code to register for callback hooks.  A configure option would be
  used to enable it.  The hooks would be something like:
    * On startup
    * After a log is created
    * After a log is deleted
    * Add support to disable the default error log capping
    * Add support to allow the extension to choose which old logs to purge.

  An example of calling a hook could look something like:
  ```
  for (auto& createFunc : Extensions::getCreateFunctions())
  {
      createFunc(eventLoop);
  }
  ```

* Add a new D-Bus API to phosphor-logging to create a log entry that bypasses
  any need to define the error in YAML, and also allows one to pass in extra
  extension data.

  * Create(Message, Severity, AdditionalData, ExtensionParameters)
    * The first 3 arguments are the same as the properties in a log
      entry.
    * The ExtensionParameters argument is a file descriptor that points to a
      JSON file whose contents can be used by any interested extensions.

  This call bypasses the journal metadata searching, and the need for the
  error to have been predefined in the phosphor-dbus-interfaces yaml.
  Doing so frees the developer from having to include both elog-errors.hpp
  and error.hpp(from phosphor-dbus-interfaces) that have that error defined,
  which in the past has been a major bottleneck in trying to write new
  code.

  The ExtensionParameters JSON is a flexible way for data to be passed
  on to any extensions, and will be ignored by any that don't care about it,
  including the `xyz.openbmc_project.Logging.Entry` objects.
  It will be used by the PEL extension, for example, to point to a file
  containing the raw PEL sent down from the host so it can be added to the
  PEL repository.

* Add an OpenPower PEL extension to phosphor-logging.  This will allow PELs to
  be created/deleted when OpenBMC logs are created/deleted, and to receive
  host PELs.

* Add OpenPower D-Bus APIs to the PEL extension for use by the PLDM daemon.
    * Delete(PEL ID) - Delete a PEL, and its corresponding OpenBMC log entry.
    * SentToHost(PEL ID) - Mark a PEL as being sent to the host.
    * AckedByOS(PEL ID) - Mark a PEL as being acknowledged by the OS.
    * array[byte] GetPEL(PEL ID) - Get a PEL.

* Add support to the PEL extension to notify the host, via PLDM, when new PELs
  are available.

Note: There will be other APIs required when the designs to handle
    REST/Redfish/management console notification are done.

### Host Error Flows
The passing of error logs between the OpenPower host and the BMC has been
defined in the PLDM documentation.  The sequence diagrams below summarize them.
The communication between the host and PLDM daemon is PLDM, while D-Bus is used
between the PLDM and logging daemons.

#### BMC -> Host
```
   +---------+      +-----------------+      +-----------------+
   |  Host   |      |   PLDM Daemon   |      | phosphor-log-mgr|
   +----+----+      +--------+--------+      +-------+---------+
        |                    |                       |
        |                    +<----------------------+
        |                    |  SendPLDMCmd:         |
        +<-------------------+  * NewLogAvailable    |
        | NewLogAvailable    |     * ID, size        |
        |   * ID, size       |                       |
        +------------------->+                       |
        | GetErrorLog(ID)    |                       |
        |                    +---------------------->+
        |                    |  GetPEL(ID)           |
        +<-------------------+                       |
        | send PEL via       |                       |
        | sideband           |                       |
        |                    |                       |
        +------------------->+                       |
        | response to        |                       |
        | GetErrorLog:       +---------------------->+
   +----+   Success          |  SentToHost(ID)       |
   |    |                    |                       |
   +--->+                    |                       |
send to +------------------->+                       |
OS      | AckErrorLog(ID)    |                       |
        |                    +---------------------->+
        |                    |  AckedByOS(ID)        |
        +                    +                       +
```

#### Host -> BMC
```
+---------+      +-----------------+  +------------------+
|  Host   |      |   PLDM Daemon   |  | phosphor log mgr |
+----+----+      +--------+--------+  +----------+-------|
     |                    |                      |
     +------------------->+                      |
     | StoreErrorLog(PEL) |                      |
     |                    +--------------------->+
     |                    |  Create()            |
     |                    |                      |
     |                    | Proceed to           |
     +                    + BMC -> Host flow     +
```

## Alternatives Considered
An alternative is to create a standalone PEL daemon.  While this would be much
cleaner for phosphor-logging, it would create extra D-Bus calls on every call
to create or delete an OpenBMC log.

## Impacts
For systems that use it, there will of course be extra code running that
creates PELs for every OpenBMC error logs.

For systems that don't, the hook lists in phosphor-logging would be empty and
there should be no impact.

## Testing
Standard unit tests will be made.  If IBM's CI wanted to, it could look to
ensure that PELs were created if an OpenBMC error log was created.
