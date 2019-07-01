# IBM Message Registry Design

Author: Matt Spinler <mspinler>

Primary assignee: Matt Spinler

Created: June 27, 2019

## Problem Description
IBM has a family of systems that are required to implement IBM's 'Platform
Event Log' (PEL) format for event logging.  To do so, IBM will convert OpenBMC
event logs into PELs, however PELs have more fields that OpenBMC event logs do.

In addition to PELs, IBM also has to have a Redfish log format that contains
some of the PELs fields, along with the Redfish message fields.

To accommodate this, this design is proposing making use of an IBM message
registry to hold these extra fields which can then be looked up based on the
OpenBMC event log when creating the PEL and Redfish items.

## Background and References
PELs have fields that are required for every error that are too unique to
IBM to support in the standard OpenBMC event log, like:
* A failing subsystem ID (about 30 available values)
* Event scope (2 values)
* Event severity (23ish values)
* Event Type, (14 values)
* Problem domain (5 values)
* Problem vector (2 values)
* Event action flags (9 values)
* A 2 byte SRC reason code value that is a key into the service documentation
  along with the other 2 bytes of the SRC word. E.g. 0xB1815200, where
  0xB1 means error or info event, 0x81 is the subsystem, and 0x5200 is the
  reason code.

The bmcweb web server has the ability (with maybe additional code), to combine a
Redfish message registry with an event log entry to display additional data for
the event.  One use of that is explained [here][1].  Message registries are
referenced in the Redfish spec [here][2] and [here][3].

The Redfish message registry contains message text that can contain argument
placeholders, which can be found via a key called the MessageID.  The event log
then has the key, plus the actual arguments, and so bmcweb could construct
the full message text by substituting in the arguments.  For example: "%1 %2
had %3 faults" -> "CPU 3 had 4 faults", when the event log has ["CPU", "3", "4"].
These message registries are designed to be localized when necessary.

In the Redfish log, the message ID needs to be the SRC.

Currently, Redfish message registries are hardcoded in header files in
[bmcweb][4].

## Requirements
* The message registry must contain values for the PEL fields that the code
  which is creating a PEL out of an OpenBMC event log cannot know itself, or
  get from the OpenBMC event log.

* The message registry must contain values for the Redfish event log that the
  code which is creating the event log cannot know itself, or get from the
  OpenBMC event log.  This event log will contain some of the fields from a PEL
  and is still in the process of being defined.

* The message registry must contain the 'Redfish message registry' values such
  that a Redfish message registry can be constructed from the IBM message
  registry. (Maybe this is up for debate?)

Note: The above requirements apply to all BMC event logs, not just the ones
      created by IBM specific code.

* The key into the message registry must be the `Message` property of the
  OpenBMC event log entry, as that is the only field that makes sense to use
  as one.

* As the new direction for OpenBMC development is to put data on the BMC
  instead of building it into the actual code, the message registry must
  be stored as data in BMC flash.

* The message registry's format should be able to be validated at build time so
  that a poorly formed one does not make it onto the BMC.

* If the code is not able to look up a message registry entry for an OpenBMC
  event log, there should be a default entry it can use so the PEL and
  Redfish log can still be created.

* For the case when a PEL comes down from the host, there still needs to be a
  Redfish event log created, with the corresponding Redfish message registry
  fields.  The PEL fields do not need to be defined in the IBM message registry
  for these.

* Allow the severity field of the OpenBMC event log to override the severity
  field listed in the message registry if they differ.  For example, if
  the registry had an error listed as 'Unrecoverable error, general', but the
  event log had a severity of 'Informational', then the resulting PEL severity
  would be 'Recovered' (0x10).  As there are only 8 OpenBMC event log severity
  values but 23 PEL severity values, some granularity may be lost, but it
  handles the most important case of being able to change an error log to
  be informational.  In this case, some other fields like the event type may
  also be changed to make the log be informational.

### Negative Requirements
* Callouts are not handled via the registry.  They are passed in via the
  OpenBMC event logs using a *TBD* mechanism.

## Proposed Design

### Registry Format
The registry is defined in JSON.  A JSON schema will be defined, and schema
validation will be enforced during the build with the python [jsonschema][5]
validator, for which there is already a recipe.  Checking for duplicates can
also be done then.

An example of an entry could look like the following, where the `name` field is
the key to lookup the entry:
```
{
    "name": "xyz.openbmc_project.Power.Error.PGOODFault",
    "Reasoncode": "0x2525",
    "Subsystem": "power_control_hardware",
    "Severity": "unrecoverable",
    "EventType": "not_applicable",
    "EventScope": "platform",
    "ProblemDomain": "not_applicable",
    "ProblemVector": "not_applicable",
    "ActionFlags": ["serviceable", "report"],
    "callhome": true,

    // Redfish message registry fields
    "Desc": "A PGOOD Fault",
    "Message": "%1 %1 had a PGOOD Fault",
    "MessageArgs":
    [
        {"type": "string", "desc": "power unit type"},
        {"type": "integer", "desc": "instance number"}
    ],
    Notes: "Real JSON doesn't allow comments, so any notes can go here."
}

```
The string values will be converted to the actual PEL field values, e.g.
'unrecoverable' to 0x40, during the build process.  Alternatives to this are:
1. Convert on the BMC at runtime.
2. Have the registry just always use the PEL values.

If it becomes necessary, a second search key could be added to the registry in
addition to the name field, which could then be passed in via the
AdditionalData field, so that 2 different registry entries could be used for
the same name field.

### Registry Location
All message registry files will be stored in the `openpower-logging`
repository.  The registry can be broken up into multiple files.  It can be left
to the repository maintainers to decide the best way to do so.

### Registry Installation
During the build, the schema validator will validate all registry entries and
will fail the build if there are any errors.  It will also convert the string
values into the actual PEL values.  Depending on the amount of errors in the
registry, breaking up the files on some sort of search boundary may aid in
search speed.

### Redfish Message Registry Installation
This will be left to a separate design.  This should not preclude defining the
Redfish message registry fields along with the PEL related fields in the IBM
message registry.

## Alternatives Considered
There are many ways to store data, but having it in JSON allows the final
product to be human readable, as opposed to in a database, and needs fewer
processing steps, such as building said database.

## Impacts
Impacts to the system should be minimal.  While the message registry will take
up flash space, the systems that it is targeted for have plenty.  The code will
only have the message registry loaded in memory while an error log lookup is in
progress.

## Testing
One can write unit tests to exercise the JSON validators and any other build
time validation.  Testcases can also be written to verify the correct PELs
and Redfish event logs are created from specific event logs for a specific
message registry.

[1]:https://github.com/openbmc/docs/blob/master/redfish-logging-in-bmcweb.md
[2]:http://redfish.dmtf.org/schemas/DSP0266_1.6.1.html#event-message-objects
[3]:https://www.dmtf.org/sites/default/files/Redfish%20School%20-%20Events.pdf
[4]:https://github.com/openbmc/bmcweb/blob/master/redfish-core/include/registries/openbmc_message_registry.hpp
[5]:https://github.com/Julian/jsonschema
