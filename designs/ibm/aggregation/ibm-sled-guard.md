# Sled Guard Design

Author:

Dhruvaraj Subhashchandran <dhruvaraj@in.ibm.com>

Other contributors:

Aravind T Nair <aravindnair@in.ibm.com>

Created: 09/09/2022

## Problem Description
Composite systems contain more than one computing unit called sleds, and each
sled contains one BMC to manage the sled. One among the BMC will have
applications to manage the composite system. On such systems, one or more sleds
may encounter intermittent problems or fatal issues. In such cases, the affected
sleds may have to be disabled permanently from booting until replacement or
disabled only for the current boot. This document discusses the requirements and
design for those operations.

## Glossary

## Background and References
More information on guard can be found here https://github.com/openbmc/docs/blob/master/designs/guard-on-bmc.md

The guard functionality enables the isolation of one more faulty redundant unit
from service to prevent the system crash at a later stage. A sled on the
composite system can encounter such faults and needs to be isolated for booting
the composite system with remaining good sleds.
Following are examples of faults encountered on a sled making it non-bootable

 - Power Fault
 - SMP cable errors
 - One or more vital components in the sled is faulty

## Requirements

 - Prevent booting a sled with power fault(?)
 - Prevent booting the sled if it cannot participate in SMP
 - Prevent booting of the sled if it is not meeting the minimum hardware to start hostboot.
 - Store the information about guarded sleds and present it to the user
 - Manual guarding/unguarding of sleds
 - Detect the sled replacement/repair and remove the guard

## Proposed Design
#### Deconfigure sleds based on SMP
- Execute SMP validation in step0
- Mark sled status in the device tree
- If any SMP connection fails, restart IPL
- Sled with non-bootable status should not attempt to boot


#### Guard on power fault
- The power fault is detected during booting or at runtime
- A guard is created for the sled
- Check the guard state of the sled before chassis poweron


## Alternatives Considered

## Impacts

## Testing