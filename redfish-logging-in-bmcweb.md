# Redfish Event Logging in bmcweb

This guide is intended to help developers add new messages to the bmcweb
Redfish event log.

Redfish Message Objects can be represented in different ways. In bmcweb, we
have chosen to use Message Registries with Message Objects that are referenced
using a MessageId and MessageArgs fields.

Additional details can be found in the
[Redfish Specification](http://redfish.dmtf.org/schemas/DSP0266_1.6.1.html).

## Message Registries

The first step when adding a new message to the Redfish event log is to find
or add an appropriate message in a Message Registry.

The bmcweb Message Registries are located under
"\redfish-core\include\registries" in source code, and they can be examined
through Redfish under "/redfish/v1/Registries".

If an appropriate message exists, note the

1. Title of the Message Object (required as the MessageKey in the MessageId).
1. Args (notated as "%x") in the "Message" field
(required for the MessageArgs).

If an appropriate message does not exist, new messages can be added as follows:

1. Choose a Message Registry for the new message
1. Choose a MessageKey to use as the title for the new Message entry
1. Insert a new Message entry (preferably with the title in alphabetical order)
1. Add a `description` and `resolution`
1. Set the `severity` to "Critical", "Warning", or "OK"
1. Define the `message` with any necessary args
1. Set the `numberOfArgs` and `paramTypes` to match the message args

## Logging Messages

Logging messages is done by providing a Redfish MessageId and any
corresponding MessageArgs to bmcweb.

A Redfish MessageId is represented in this format:

`RegistryName.MajorVersion.MinorVersion.MessageKey`

bmcweb will search the specified Message Registry for the MessageKey,
construct the final message using the MessageArgs, and display that in the
event log.

### journal-based Redfish Logging

The journal is the current mechanism used to log Redfish Messages. bmcweb
looks for two fields in the journal metadata:

* `REDFISH_MESSAGE_ID`: A string holding the MessageId
* `REDFISH_MESSAGE_ARGS`: A string holding a comma-separated list of args

These fields can be added to a journal entry using either the
`phosphor::logging::entry()` command or directly using the
`sd_journal_send()` command.

### Examples

#### Logging a ResourceCreated event

The
[Resource Event Message Registry](https://redfish.dmtf.org/registries/ResourceEvent.1.0.0.json)
holds the ResourceCreated message:
```json
{
    "ResourceCreated": {
        "Description": "Indicates that all conditions of a successful
        creation operation have been met.",
        "Message": "The resource has been created successfully.",
        "NumberOfArgs": 0,
        "Resolution": "None",
        "Severity": "OK"
    }
},
```

Since there are no parameters, no MessageArgs are required, so this message
can be logged to the journal as follows:
```cpp
phosphor::logging::log<log::level>(
        "journal text",
        phosphor::logging::entry("REDFISH_MESSAGE_ID=%s",
        "ResourceEvent.1.0.ResourceCreated"));
```
or
```cpp
sd_journal_send("MESSAGE=%s", "journal text", "PRIORITY=%i", <LOG_LEVEL>,
                "REDFISH_MESSAGE_ID=%s",
                "ResourceEvent.1.0.ResourceCreated", NULL);
```

#### Logging a ResourceErrorThresholdExceeded event
The
[Resource Event Message Registry](https://redfish.dmtf.org/registries/ResourceEvent.1.0.0.json)
holds the ResourceErrorThresholdExceeded message:
```json
{
    "ResourceErrorThresholdExceeded": {
        "Description": "The resource property %1 has exceeded error
        threshold of value %2. Examples would be drive IO errors, or
        network link errors.",
        "Message": "The resource property %1 has exceeded error
        threshold of value %2.",
        "NumberOfArgs": 2,
        "ParamTypes": [
            "string",
            "value"
        ],
        "Resolution": "None.",
        "Severity": "Critical"
    }
},
```

This message has two parameters, `%1` and `%2`, as indicated in the
`"NumberOfArgs"` field. The meaning and types of these parameters are derived
from the `"Message"` and `"ParamTypes"` fields in the Message Registry.

In this example, `%1` is a string holding the property name and `%2` is a
number holding the threshold value.

The parameters are filled from a comma-separated list of the MessageArgs, so
this message can be logged to the journal as follows:
```cpp
phosphor::logging::log<log::level>(
        "journal text",
        phosphor::logging::entry("REDFISH_MESSAGE_ID=%s",
        "ResourceEvent.1.0.ResourceErrorThresholdExceeded"),
        phosphor::logging::entry("REDFISH_MESSAGE_ARGS=%s,%d",
        "Property Name", propertyValue));
```
or
```cpp
sd_journal_send("MESSAGE=%s", "journal text", "PRIORITY=%i", <LOG_LEVEL>,
                "REDFISH_MESSAGE_ID=%s",
                "ResourceEvent.1.0.ResourceErrorThresholdExceeded",
                "REDFISH_MESSAGE_ARGS=%s,%d", "Property Name",
                propertyValue, NULL);
```
