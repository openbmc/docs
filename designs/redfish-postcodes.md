# Logging BIOS POST Codes Through Redfish

Author: Terry Duncan

Other contributors: Jason Bills, Zhikui Ren

Created: December 23, 2019

## Problem Description

BIOS Power-On Self-Test (POST) codes are exposed on DBUS but not currently over
Redfish. This describes a method to expose the BIOS POST codes over the Redfish
interface using the logging service.

## Background and References

The standard Redfish LogService and LogEntry schemas will be used to expose BIOS
POST codes. An additional log service (PostCodes) will be added to the
LogServiceCollection.

Sample [LogService](https://redfish.dmtf.org/schemas/LogService_v1.xml) entry:

```
https://obmc/redfish/v1/Systems/system/LogServices/PostCodes
{
    "@odata.context": "/redfish/v1/$metadata#LogService.LogService",
    "@odata.id": "/redfish/v1/Systems/system/LogServices/PostCodes",
    "@odata.type": "#LogService.v1_1_0.LogService",
    "Actions": {
        "#LogService.ClearLog": {
            "target": "/redfish/v1/Systems/system/LogServices/PostCodes/Actions/LogService.ClearLog"
        }
    },
    "Description": "POST Code Log Service",
    "Entries": {
        "@odata.id": "/redfish/v1/Systems/system/LogServices/PostCodes/Entries"
    },
    "Id": "BIOS POST Code Log",
    "Name": "POST Code Log Service",
    "OverWritePolicy": "WrapsWhenFull"
}
```

Events will be exposed using the
[LogEntry](https://redfish.dmtf.org/schemas/LogEntry_v1.xml) schema.

```
https://obmc/redfish/v1/Systems/system/LogServices/PostCodes/Entries
{
    "@odata.context": "/redfish/v1/$metadata#LogEntryCollection.LogEntryCollection",
    "@odata.id": "/redfish/v1/Systems/system/LogServices/PostCodes/Entries",
    "@odata.type": "#LogEntryCollection.LogEntryCollection",
    "Description": "Collection of POST Code Log Entries",
    "Members": [
        {
            "@odata.context": "/redfish/v1/$metadata#LogEntry.LogEntry",
            "@odata.id": "/redfish/v1/Systems/system/LogServices/PostCodes/Entries/B1-03",
            "@odata.type": "#LogEntry.v1_4_0.LogEntry",
            "Created": "2019-12-06T14:10:30+00:00",
            "EntryType": "Event",
            "Id": "B1-03",
            "Message": "Boot Count: 4: TS Offset: 0.0033; POST Code: 0x43",
            "MessageArgs": [
                4,
                0.0033,
                "0x43"
            ],
            "MessageId": "OpenBMC.0.1.BiosPostCode",
            "Name": "POST Code Log Entry",
            "Severity": "OK"
        },
        ...
    ],
    "Members@odata.count": 10000,
    "Name": "BIOS POST Code Log Entries"
}
```

A new [MessageRegistry](https://redfish.dmtf.org/schemas/MessageRegistry_v1.xml)
schema entry defines the format for the message.

```
https://obmc/redfish/v1/Registries/OpenBMC/OpenBMC
{
    "@Redfish.Copyright": "Copyright 2018 OpenBMC. All rights reserved.",
    "@odata.type": "#MessageRegistry.v1_0_0.MessageRegistry",
    "Description": "This registry defines the base messages for OpenBMC.",
    "Id": "OpenBMC.0.1.0",
    "Language": "en",
    "Messages": {
        "BiosPostCode": {
            "Description": "BIOS Power-On Self-Test Code received.",
            "Message": "Boot Count: %1: TS Offset: %2; POST Code: %3",
            "NumberOfArgs": 3,
            "ParamTypes": [
                "number",
                "number",
                "string"
            ],
            "Resolution": "None.",
            "Severity": "OK"
        },
        ...
    }
    "Name": "OpenBMC Message Registry",
    "OwningEntity": "OpenBMC",
    "RegistryPrefix": "OpenBMC",
    "RegistryVersion": "0.1.0"
}
```

## Requirements

The Redfish Interface shall be Redfish compliant and pass the Redfish compliancy
tests.

The Redfish interface shall expose POST codes tracked on DBUS since the last BMC
reset.

## Proposed Design

Currently, OpenBMC exposes BIOS POST codes on DBus using the
xyz.openbmc_project.State.Boot.PostCode service. The existing interface tracks
POST codes for the past 100 host boot events and the current boot cycle index.

```
xyz.openbmc_project.State.Boot.PostCode
    GetPostCodes(q undefined) → [uint64] undefined
    CurrentBootCycleIndex = 1
    MaxBootCycleIndex = 100
```

The GetPostCodes method is called using the boot cycle index to retrieve the
codes for the boot cycle.

```
{
  "call": "GetPostCodes",
  "interface": "xyz.openbmc_project.State.Boot.PostCode",
  "obj": "/xyz/openbmc_project/State/Boot/PostCode",
  "result": [
    1,
    2,
    3,
    4,
    5,
    6,
    17,
    50,
    4,
    173
  ],
  "status": "ok"
}
```

The existing DBus GetPostCodes method will remain for backward compatibility. A
new method GetPostCodesTS will be added to include an ISO formatted time stamp
with micro-second resolution along with each POST code.

```
{
  "call": "GetPostCodesTS",
  "interface": "xyz.openbmc_project.State.Boot.PostCode",
  "obj": "/xyz/openbmc_project/State/Boot/PostCode",
  "result": [
    {"20191223T143052.632591", 1},
    {"20191223T143052.634083", 2},
    {"20191223T143053.928719", 3},
    {"20191223T143053.930168", 4},
    {"20191223T143054.512488", 5},
    {"20191223T143054.513945", 6},
    {"20191223T143054.960246", 17},
    {"20191223T143054.961723", 50},
    {"20191223T143055.368219", 4},
    {"20191223T143055.369680", 173}
  ],
  "status": "ok"
}
```

The DBus DeleteAll interface will be implemented to remove entries. The Redfish
ClearLog action will call the DBus DeleteAll interface.

```
{
  "call": "DeleteAll",
  "interface": "xyz.openbmc_project.Collection.DeleteAll",
  "obj": "/xyz/openbmc_project/State/Boot/PostCode",
  "result": "POST Codes Cleared"
  "status": "ok"
}
```

## Alternatives Considered

Consideration was given to using the existing DBus method and not exposing
associated time stamps. In this case, a single log entry could be used per boot
cycle exposing a string with all POST codes associated with that boot cycle.
Time stamp data was considered valuable even if the data may be skewed by DBUS.

Consideration was also given to expose the POST codes through a OEM extension
rather than using the LogEntry schema. Use of OEM extensions to Redfish are
discouraged. It can be revisited if DMTF adds a schema for POST codes.

## Impacts

Backward compatibility remains with the existing DBUS interface method. Minimal
performance impact is expected to track timestamps.

## Testing

Compliance with Redfish will be tested using the Redfish Service Validator.
