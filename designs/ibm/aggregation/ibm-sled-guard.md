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
sleds may have to be disabled only for the current boot or, in some cases,
permanently disabled from booting until replacement. This document discusses the
requirements and design for those operations.

## Glossary

## Background and References
More information on guard can be found here https://github.com/openbmc/docs/blob/master/designs/guard-on-bmc.md

The guard functionality enables the isolation of one more faulty redundant unit
from service to prevent the system crash at a later stage. A sled on the
composite system can encounter such faults and needs to be isolated for booting
the composite system with remaining good sleds.
Following are examples of faults encountered on a sled, making it non-bootable

Permanent disable
 - Power Fault
 - Manually disabling a sled

Temporary disable
 - SMP cable errors
 - One or more vital components in the sled is faulty

## Requirements

 - Prevent booting a sled with a power fault
 - Prevent booting the sled if it cannot participate in SMP
 - Prevent booting of the sled if it is not meeting the minimum hardware to
   start hostboot.
 - Store the information about guarded sleds and present it to the user
 - Manual guarding/unguarding of sleds
 - After repair/replacement user needs to remove the permanent disable from the
   sleds to boot it.
 - There should be a way to recover the system if all sleds are disabled due to
   non-fatal errors.

## Proposed Design
#### Deconfigure sleds based on SMP
- Execute SMP validation in step0
- Mark sled status in the device tree
- If any SMP connection fails, restart IPL
- Sled with non-bootable status should not attempt to boot

#### The guard record storage
- The information about a permanently disabled sled needs to be stored on the
  SCA
- SCA will check before starting the boot
- There should be a way to manually add the sleds to the guarded list
- Once the sleds are guarded or disabled, they won't be enabled or unguarded
   automatically, user intervention is needed to clear the guard.

#### Guard on power fault
- The power fault is detected during booting or at runtime
- A guard is created for the sled
- Check the guard state of the sled before chassis poweron


#### Resource Recovery
If the vital resources in the sleds are deconfigured due to non-fatal errors and
the system cannot boot, then such resources need to be recovered to boot the
system to prevent a high-impact outage.

##### Proposed flow
- The resource recovery is disabled on all multi-sled composite systems by
  default
- If the first boot failed, set resource recovery on bios attribute for all
  sledes
- Hostboot should honor the flag and attempt the resource recovery on its sled
- If the system is reaching runtime, disable the resource recovery flag
- The process needs to be repeated on any future cold boots
- If the system fails to boot even with resource recovery, the flag will be
  cleared
- The failed system will stay quiesced.

## Alternatives Considered

## Impacts

## Testing