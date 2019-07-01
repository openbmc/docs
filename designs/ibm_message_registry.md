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

### Open Questions
There are some fundamental questions that need to be answered about the
registry before a full design can be done:

#### Registry Location
1) Is the registry an all encompassing list that contains all possible errors
   and resides in a single repository so that all errors can be found in one
   place (though maybe across multiple files), or

2) Is it pieced together from other locations, and if it is, is it:
  * Pieced together by having the repositories that are included in a build
    bbappend some recipe to have their repo level message registries copied
    into a central location and then validated and combined during the build,
    or

  * Should the repo level registries reside in a bitbake layer so that a repo
    can change its registry based on system type, or

3) Some combination of the above 3 options?

The answers for 2) may depend on if we want to allow a single registry entry
to be different across systems, in which case it would need to go into a
layer, or if we need to know exactly which errors are possible on a system,
without any extras.

#### Callouts
Should the registry be allowed to contain callouts, or should those always
be passed in with the event log?

#### Registry Field Mutability
When OpenBMC event logs are created, they have the ability to pass data into
the event log via the AdditionalData property.  It would be possible to allow
a message registry field to be overridden this way.  For example, say that
there is a case where normally an error is an unrecoverable event, but for
some reason it can now just be made informational.  The calling code could
then pass in 'EventSeverity=Informational' for an event log that matches on
a normally unrecoverable error, and the PEL code could then make it
informational instead. The action flags would be another example of a field
that could be overridden.  Do we want to allow this?

It's outside the scope of this document, but it bears mentioning that there are
plans to allow event log creators to pass in fields via AdditionalData that
will show up in a PEL which are outside the scope of the message registry, for
example the user defined SRC words.

#### Getting a Redfish message registry from the IBM message registry
As mentioned above, bmcweb keeps its message registries in header files.  If we
want to be able to use fields from the IBM message registry in the Redfish
message registry, we may need to figure out a way to have bmcweb support
loading in message registry files.

## Proposed Design
Regardless of the answers to the questions above, this design proposes that:

1) The registry is defined in JSON.  There is the capability to define schemas
for JSON, with validators that can be run during a build, such as
[this one][https://github.com/Julian/jsonschema].  Checking for duplicates can
also be done.

An example of an entry could look the following, where the `name` field is
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

The remainder of the design will depend on how the open questions are resolved.

## Alternatives Considered
There are many ways to store data, but having it in JSON allows the final
product to be human readable, as opposed to in a database, and needs fewer
processing steps, such as building said database.

## Impacts
Impacts to the system should be minimal.  While the message registry will take
up flash space, the systems that it is targeted for have plenty.  The registry
can be sorted during the build process to allow for binary searches to speed
up access if necessary.

## Testing
One can write unit tests to exercise the JSON validators and any other build
time validation.  Testcases can also be written to verify the correct PELs
and Redfish event logs are created from specific event logs for a specific
message registry.

[1]:https://github.com/openbmc/docs/blob/master/redfish-logging-in-bmcweb.md
[2]:http://redfish.dmtf.org/schemas/DSP0266_1.6.1.html#event-message-objects
[3]:https://www.dmtf.org/sites/default/files/Redfish%20School%20-%20Events.pdf
[4]:https://github.com/openbmc/bmcweb/blob/master/redfish-core/include/registries/openbmc_message_registry.hpp
