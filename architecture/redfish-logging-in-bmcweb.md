# Redfish Event Logging in bmcweb

This guide is intended to help developers add new messages to the bmcweb Redfish
event log.

Redfish Message Objects can be represented in different ways. In bmcweb, we have
chosen to use Message Registries with Message Objects that are referenced using
a MessageId and MessageArgs fields.

Additional details can be found in the
[Redfish Specification](http://redfish.dmtf.org/schemas/DSP0266_1.6.1.html).

## Message Registries

The first step when adding a new message to the Redfish event log is to find or
add an appropriate message in a Message Registry.

The bmcweb Message Registries are located under
"\redfish-core\include\registries" in source code, and they can be examined
through Redfish under "/redfish/v1/Registries".

If an appropriate message exists, note the

1. Title of the Message Object (required as the MessageKey in the MessageId).
2. Args (notated as "%x") in the "Message" field (required for the MessageArgs).

If an appropriate message does not exist, new messages can be added as follows:

1. Choose a Message Registry for the new message
2. Choose a MessageKey to use as the title for the new Message entry
3. Insert a new Message entry (preferably with the title in alphabetical order)
4. Add a `description` and `resolution`
5. Set the `severity` to "Critical", "Warning", or "OK"
6. Define the `message` with any necessary args
7. Set the `numberOfArgs` and `paramTypes` to match the message args

## Logging Messages

Logging messages is done by providing a Redfish MessageId and any corresponding
MessageArgs to bmcweb.

A Redfish MessageId is represented in this format:

`RegistryName.MajorVersion.MinorVersion.MessageKey`

bmcweb will search the specified Message Registry for the MessageKey, construct
the final message using the MessageArgs, and display that in the event log.

### journal-based Redfish Logging

The journal is the current mechanism used to log Redfish Messages. bmcweb looks
for two fields in the journal metadata:

- `REDFISH_MESSAGE_ID`: A string holding the MessageId
- `REDFISH_MESSAGE_ARGS`: A string holding a comma-separated list of args

These fields can be added to a journal entry using either the
`phosphor::logging::entry()` command or directly using the `sd_journal_send()`
command.

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

In this example, `%1` is a string holding the property name and `%2` is a number
holding the threshold value.

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

### D-Bus-based Redfish Logging

Compiling bmcweb with `-DBMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES=ON` option,
bmcweb will look to
[phosphor-logging](https://github.com/openbmc/phosphor-logging) for D-Bus log
entries. The entries will be translated to Redfish EventLog Entries. The logs
can be done by calling the method `"Create"` from phosphor-logging. When an
entry is created, there is also a consistent log file generated at
`/var/lib/phosphor-logging/errors/`.

This implementation will disable the journal-based logging.

### Examples

#### Logging with MessageId and MessageArgs

The phosphor-logging should recognize `MessageId` and `MessageArgs` in
`AdditonalData`, and publish them on D-Bus.

Take the ResourceErrorThresholdExceeded event above for example:

```cpp
std::map<std::string, std::string> additionalData;
additionalData.emplace("REDFISH_MESSAGE_ID",
                       "ResourceEvent.1.0.ResourceErrorThresholdExceeded");
additionalData.emplace("REDFISH_MESSAGE_ARGS", "propertyName,proertyValue");
additionalData.emplace("other data", otherData);

auto bus = sdbusplus::bus::new_default();
auto reqMsg = bus.new_method_call("xyz.openbmc_project.Logging",
                                  "/xyz/openbmc_project/logging",
                                  "xyz.openbmc_project.Logging.Create",
                                  "Create");
reqMsg.append("journal text", <LOG_LEVEL>, additionalData);

try
{
    auto respMsg = bus.call(reqMsg);
}
catch (sdbusplus::exception_t& e)
{
    phosphor::logging::log<phosphor::logging::level::ERR>(e.what());
}
```

#### Logging without MessageId and MessageArgs

The logging can also be done without `MessageId` and `MessageArgs`. Redfish
will show the message provided instead of getting message from registries.

```cpp
std::map<std::string, std::string> additionalData;
additionalData.emplace("other data", otherData);

auto reqMsg = bus.new_method_call("xyz.openbmc_project.Logging",
                                  "/xyz/openbmc_project/logging",
                                  "xyz.openbmc_project.Logging.Create",
                                  "Create");
reqMsg.append("event message", <LOG_LEVEL>, additionalData);

try
{
    auto respMsg = bus.call(reqMsg);
}
catch (sdbusplus::exception_t& e)
{
    phosphor::logging::log<phosphor::logging::level::ERR>(e.what());
}
```
