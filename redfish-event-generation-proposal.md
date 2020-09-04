Author: ratagupt@linux.vnet.ibm.com

Created: September 4, 2020

## Problem Description
Currently in existing implementation, BMC can not send Redfish event based on the Resource Type. 
most of the openBMC application are D-bus application where on any D-bus Resource/Property update generates
a D-bus signal

Hence when any Dbus Resource/Property gets changed, Redfish server can get this info and send the event
to the interested clients depends on the event subscription.

Eg: Say if any user account gets created, BMC needs to send a Resource Event
      "ResourceCreated"  with "origin of condition = /redfish/v1/AccountService/Accounts/<id>"
      "ResourceUpdated"  with "origin of condition = /redfish/v1/AccountService/Accounts/<id>"
      "ResourceDeleted"  with "origin of condition = /redfish/v1/AccountService/Accounts/<id>"
     In some of the redfish event registries  we need to send the redfish property name also

How to map the Dbus Resources/DbusProperties to Redfish Resources/Properties
      
## Background and References
There is existing design in openBMC where each repo has to do journal baesd logging
https://github.com/openbmc/docs/blob/master/architecture/redfish-logging-in-bmcweb.md

### journal-based Redfish Logging

The journal is the current mechanism used to log Redfish Messages. bmcweb looks for two fields in the journal metadata:
    REDFISH_MESSAGE_ID: A string holding the MessageId
    REDFISH_MESSAGE_ARGS: A string holding a comma-separated list of args

These fields can be added to a journal entry using either the phosphor::logging::entry() command or directly using the sd_journal_send() command.
     eg:
        phosphor::logging::log<log::level>(
        "journal text",
        phosphor::logging::entry("REDFISH_MESSAGE_ID=%s",
        "ResourceEvent.1.0.ResourceErrorThresholdExceeded"),
        phosphor::logging::entry("REDFISH_MESSAGE_ARGS=%s,%d",
        "Property Name", propertyValue));

Cons: 
- Each openbmc repo have to write the messages like above which suggest changes are needed in almost all the repos to add those message.
- Journal will be filled with such transport specific logs.
- This design is still not solving the problem of how to map the Dbus property with Redfish property.
- How the Dbus path will be mapped to Redfish Resource Path

## Proposal
- In bmcweb each Redfish node represents a Redfish Resource.
- Each node will be having it's own mapping between Redfish properties and the Dbus Resources(MAP1).
- Some code on bmcweb will walkthrough on each node during startup , get this mapping from each node and generate 
   two mappings.
   1) Reverse mapping (Dbus Resource to Redfish Resource)(MAP3) and
   2) mapping between Resource Types to the interested Dbus interfaces(MAP2)
- To start with we will support few resource types and then scale it up as needed.
- MAP2 would be used when the Redfish client subscribe for the ResourceType to get the Dbus mappings.
- MAP3 would be used when the Dbus signal gets generated and need the Redfish mappings.
- Once we have all thses mapping gets generated and loaded into the memory, bmcweb would start listening
  for the interfaces listed in map2.
- Once any Dbus signal gets generated map3 would be used to get the Redfish mapping.

### Few examples for the mapping
https://gist.github.com/ratagupt/0aa4da098a60d49af90a7e4a6ea6d5f2
1) Map1: Mapping between redfish resources to Dbus resources
2) Map2: Mapping between redfish resource types to the ineterested Dbus interfaces
3) Map3: Mapping between Dbus resources to redfish resources
I tried to cover the following scenario in the above mapping.
- Redfish resource is mapped to multiple Dbus Resources
- Redfish Property is mapped to single Dbus property
- Redfish Property(complex property) is mapped to multiple dbus property.
- Same type of Redfish Resources are mapped to different Dbus Resources
- Redfish node url having multiple regex : Yet to take a look on this.

## Alternate Proposal

Currently each redfish node gets the data from backend(d-bus repo) once the request comes from Redfish client.
- Can we cache all the propertoes and keep all the redfish resources updated ?
- To do so we need to have monitoring logic inside each Redfish Node, if we does so we can 
  generate the event whenever any redfish properties gets updated, Event would be sent to the client
  depends on the subscription.

