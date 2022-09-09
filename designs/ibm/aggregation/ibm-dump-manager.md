# Dump Manager Design

Author:
  Dhruvaraj Subhashchandran <dhruvaraj@in.ibm.com>

Other contributors:
  Aravind T Nair <aravindnair@in.ibm.com>

Created: 09/09/2022

## Problem Description
When an error is encountered on the BMC or the host, a dump file with the debug
information will be generated and listed using redfish interfaces, and users can
offload these dump files. On the composite systems, the dump's scope can be from
the entire composite system or limited to a local BMC. Therefore, there should
be an orchestration mechanism on the composite systems to coordinate between
different BMCs to generate the dumps, list dumps, offload dumps or delete dumps
from different BMCs in the system level or BMC level dumps. 

## Glossary


## Background and References
More details on the dump can be found here: https://github.com/openbmc/docs/blob/master/designs/dump-manager.md

There are two major types of dumps: the BMC dump, which contains the debug data
from the BMC, and the host dump, which contains the debug data from various host
components. BMC dumps are always generated from the local BMC, but host dumps
can be further classified as follows.
#### Scope:
**System level**: The scope of the dump is system level; the data is collected
either from the host firmware of the composite system or from all BMCs in the
composite system.
**BMC level**: The dumps are collected from a satellite BMC and kept on the same
BMC.
#### Creation:
**BMC created**: The dumps are created and packaged on the BMC.
**Host created**: These dumps are created and packaged by host firmware.

Table 1: Classification various supported dumps
|  Dump type| Scope | Creation | Type
|--|--|--|--|
| System Dump | System |System |System |
| Resource Dump | System |System |System |
| Checkstop Dump | System |BMC |System |
| Hostboot Dump | BMC |BMC |System |
| SBE Dump | BMC |BMC |System |
| BMC Dump | BMC |BMC |BMC |

## Requirements
- One  dump manager  service  per  BMC, and one  among  them will act as SCA dump manager
- All BMCs will  collect & package the BMC  scope dumps
- SCA BMC will package system-wide dumps
- SCA dump  manager  will orchestrate  the triggering  & collection dumps from  the satellites in case of Checkstopdump
- All BMCs are  required  to inform  SCA on the completion  of  dumps
- SCA need  to grab  dump  pieces from  all BMCs to package a Checkstopdump
- BMC scope dumps(BMC,  HB, SBE))  will stay locally in its respective  BMC
- The delete/offload operation  on  satellite BMCs will  be forwarded  to respective  BMCs for BMC scope dumps
- No  change is required  for  resource  dumps  or related  dumps(e.g., LPA dump)
- MPIPL flow  needs to orchestrate  with satellite BMCs for  collecting disruptive  System dump
- Should  dump  entries & dump files  available  on BMCs get synced across all BMCs for failover  scenarios
- When/If  the system is Host managed,  SCA will be  responsible  for  offloading  dumps  to host
- Deletion  of  a dump  will not be allowed  if the dump  is already  being  offloaded
- Multiple parallel  offloads  of  different  types of  dumps  should be  allowed 



## Proposed Design
TBD

## Alternatives Considered
TBD

## Impacts
TBD

## Testing
TBD
