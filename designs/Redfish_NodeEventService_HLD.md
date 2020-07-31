# Redfish NodeEventLog-logging HLD

Author: Chen, Yugang

Primary assignee: Chen, Yugang

Other contributors: Bills, Jason

Created: 30-July-2020

## Problem

There are multi nodes in some kinds of system, each node with a BMC, and one BMC works as primary BMC.

Primary BMC needs to show slave node event log in redfish. Redfish in primary OpenBMC currently doesn’t support to show slave node’s event log in multi nodes system. This describes a method for primary BMC to expose the slave node’s event log over the Redfish interface using the logging service.

## Background and References

Refer to bmcweb Redfish logging.
[Link]( https://gerrit.openbmc-project.xyz/c/openbmc/docs/+/20110 )

Redfish provides a LogService resource that can be used to display different types of logs. These can come from any source and Redfish supports Event, SEL (IPMI), BIOS Post Codes, and OEM type logs.

Refer to doc describing Redfish BIOS Post Codes, in that both are under LogService.
https://gerrit.openbmc-project.xyz/c/openbmc/docs/+/28160

there is an existing IPMBBridge channel between primary BMC and slave node BMC, they can communicate each other by this channel.

## Proposed Design

Primary BMC exposes slave nodes on DBus using the
xyz.openbmc_project.State.MultiModular. The interface keeps the node count and node role.
```
xyz.openbmc_project.State.MultiModular
.NodeCount  property  y 4
.PrimaryEn  property  b true

Primary BMC shows the logs in redfish logService. It needs to add an entity for each BMC slave node in “Members” of LogServices “/redfish/v1/Systems/system/LogServices/EventLogNode<x>”.

An additional log service (EventLogNode<x>) will be added to
the LogServiceCollection.

Sample [LogService](https://redfish.dmtf.org/schemas/LogService_v1.xml) entry:
```
https://obmc/redfish/v1/Systems/system/LogServices/EventLogNode<x>)
{
    "@odata.context": "/redfish/v1/$metadata#LogService.LogService",
    "@odata.id": "/redfish/v1/Systems/system/LogServices/EventLogNode" + nodeId",
    "@odata.type": "#LogService.v1_1_0.LogService",
    "Actions": {
        "#LogService.ClearLog": {
            "target":                      "/redfish/v1/Systems/system/LogServices/EventLogNode" + nodeId + "/Actions/LogService.ClearLog"
        }
    },
    "Description": "Slave Node Event Service",
     "Entries":{
         "@odata.id",             "/redfish/v1/Systems/system/LogServices/EventLogNode" + nodeId + "/Entries"},
    "Id": " Slave Node Event Log",
    "Name": " Slave Node Event Service",
    "OverWritePolicy": "WrapsWhenFull"
}

## Alternatives Considered

It could be added to oem type of LogService, but OEM types are being discouraged for upstream OpenBMC.

## Impacts

None.

## Testing
Compliance with Redfish will be tested using the Redfish Service Validator.

