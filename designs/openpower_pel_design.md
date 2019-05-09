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
* PEL handling code should not be present in any common OpenBMC code, unless it
  can be compiled out.

* Code must still use the phosphor-logging APIs for creating events, even
  if only the PEL form of the event is important.  Due to the requirement of
  keeping PEL code out of common code, these logging calls should be able to
  ignore any extra data added for PELs if desired.

* Event logs created by common OpenBMC code must be converted into PEL logs.
  This will most likely require something like an IBM message registry that
  contains the hardcodable PEL fields that are outside the scope of
  phosphor-logging.

* The lifetime of a PEL must match the lifetime of the OpenBMC event log.

* IBM has a specific log retention policy that involve knowing the internal
  PEL contents.  This implies that the code that manages PELs must also
  manage the retention policy for the phosphor-logging logs.

* The host firmware needs the BMC to send it the PELs created on the BMC.
  This drives the following requirements.
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
* Create a daemon for managing PELs.  It will provide the following D-Bus
  APIs to manipulate PELs:
    * Create(Message, ID, Severity, Timestamp, AdditionalData, json)
      * All arguments but 'json' are from the corresponding openbmc
        log entry. The json argument is a JSON string of extra data
        to go into the PEL.  Its contents are outside the scope of
        this design.
    * Delete(PEL ID) - Delete a PEL, and its corresponding OpenBMC log entry.
    * SentToHost(PEL ID) - Mark a PEL as being sent to the host.
    * AckedByOS(PEL ID) - Mark a PEL as being acknowledged by the OS.
    * array[byte] GetPEL(PEL ID) - Get a PEL.
    * interfacesRemoved watch on `xyz.openbmc_project.Logging.Entry`:
      * When that interface is removed, delete the corresponding PEL.

    When a new PEL is created, the Daemon will send the `NewLogAvailable` PLDM
    command to the host to let it know the ID of the PEL just created.

    Note: There will be other APIs required when the designs to handle
          REST/Redfish/management console notification are done.

* Add new compile time selectable functionality to phosphor-log-manager.

    * Turn off the default purging algorithm that deletes old logs before new
      ones are added if certain conditions are met.

    * Add a new D-Bus API to create a log entry that bypasses any need to
      define the error in YAML, and also allows one to pass in extra data
      that will be ignored unless a compile flag says not to:

        * Create(Message, Severity, AdditionalData, OEMData)
          * The first 3 arguments are the same as the properties in a log
            entry.
          * The OEMData parameter is a file descriptor that points to a file
            whose contents will be sent to the PEL daemon. (See next bullet).

      This call bypasses the journal metadata searching, and the need for the
      error to have been predefined in the phosphor-dbus-interfaces yaml.
      Doing so frees the developer from having to include both elog-errors.hpp
      and error.hpp(from phosphor-dbus-interfaces) that have that error defined,
      which in the past has been a major bottleneck in trying to write new
      code.  The downside is it doesn't require the error to be documented,
      though the PEL handler will handle error definitions in a completely
      different way anyway as it needs additional details about errors.

    * Add the ability for phosphor-log-manager to call the Create() D-Bus API
      on the PEL daemon after a new log has been created, using the method in
      the section above.   It will read the contents of the OEMData JSON file
      and pass that JSON as string in the 'json' argument.  When
      phosphor-log-manager creates errors from the existing report() and
      commit() APIs, it will just pass an empty JSON string.


### Sequence Diagram
This shows how an event propagates from a BMC application to the PEL daemon.
```
+---------------+           +-----------------------+        +-------------+
| BMC App       |           | phosphor log manager  |        | PEL Daemon  |
+------+--------+           +-----------+-----------+        +--------+----+
       |                                |                             |
       +------------------------------->+                             |
       |                                +------+                      |
       |       Create()                 |      |                      |
       |       * Message                <------+                      |
       |       * Severity               | /xyz/.../logging/entry/5    |
       |       * AdditionalData         |                             |
       |       * OEMData:               |  #ifdef PEL_SUPPORT         |
       |       {                        +---------------------------->+
       |         "FFDC": "/tmp/myffdc"  |        Create()             |
       |       }                        |        * Message            |
       |                                |        * ID                 |
       |                                |        * Severity           +-------+
       |                                |        * Timestamp          |       |
       |                                |        * AdditionalData     |Create |
       |                                |        * json:              |PEL    |
       |                                |        "{ "FFDC": ...}"     |       |
       |                                |                             |       |
       |                                |  #endif                     +<------+
       |                                |                             |
       +                                +                             +
```

Note that in the case of when a PEL is sent down from the host, the PLDM
handler will be the BMC app calling the Create API, and the actual PEL data
would in a file and pointed to in the OEMData json.  The PEL daemon would
then read that file to get the PEL without needing to send it over D-Bus.

## Alternatives Considered
One alternative is to just have the PEL daemon listen on D-Bus for new log
creations and grab everything from that signal.  The downside is then the
additional contents for the PEL, such as possibly several KB of FFDC, would
need to go onto D-Bus in the OpenBMC log so it can be obtained by the handler,
(or put a path to a file in the D-Bus log).  Additionally, this would preclude
the possible future enhancement to remove the OpenBMC logs from D-Bus.

Another alternative is to just make PELs natively in code using the PEL
daemon's D-Bus APIs, and not use phosphor-logging at all, but that would
preclude any of this code from being used in systems that don't use PELs.

## Impacts
The PEL daemon will consume CPU cycles building the PEL blob files for each
error log.  Also, when an error log is created, if the PEL feature is enabled,
the logging daemon will make a new D-Bus call to the PEL daemon when error logs
are created.

## Testing
Standard unit tests will be made.  If IBM's CI wanted to, it could look to
ensure that PELs were created if an OpenBMC error log was created.
