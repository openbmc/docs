# NC-SI core dump
Author: DelphineCCChiu (Delphine_CC_Chiu@wiwynn.com)

Created: 03/12/2024
## Problem Description
NIC core-dump is essential for NIC issue debugging, and it could be retrieved via both in-band and out-band method. The design here is providing the solution for NIC out of band dumping from BMC over NC-SI protocol.

## Background and References
NC-SI command for dump retrieval:
Reference: NC-SI spec v1.2: section: 8.4.114
https://www.dmtf.org/sites/default/files/standards/documents/DSP0222_1.2.0.pdf

## Requirements
This feature requires Linux kernel to support transferring new NC-SI command (0x4D) in net/ncsi module
https://github.com/torvalds/linux/commits/master/net/ncsi

## Proposed Design
This design will reuse existing phosphor-debug-collector module: https://github.com/openbmc/phosphor-debug-collector
User could call CreateDump Dbus-method with new NC-SI dump type under 
"xyz.openbmc_project.Dump.Manager /xyz/openbmc_project/dump/bmc xyz.openbmc_project.Dump.Create" for dump creation

A new dreport plugin named "ncsidump" will be added for dump file generation.
And this plugin will use ncsi-netlink utility to create the dump file and put it into dump folder for pakage preparation.

All NC-SI dump procedure will be implemented in ncsi-netlink:
https://github.com/openbmc/phosphor-networkd/blob/master/src/ncsi_netlink_main.cpp

The following block diagram illustrate entire dump procedure and relationship between modules:

```
                           +----------------+           +-----------+         
                           |                |           |           |         
              ------------->  Dump manager  +-----------> DumpEntry |         
                Create     |                |           |           |         
                           +--------+-------+           +-----------+         
                                    |                                         
                                    |                                         
                                    |                                         
                           +--------v-------+                                 
                           |                |                                 
                           |    Dreport     |                                 
                           |                |                                 
                           +--------+-------+                                 
                                    |                                         
                                    |                                         
                                    |                                         
                           +--------v-------+           +-----------+         
                           |                |           |           |         
                           |  NCSI-NetLink  +-----------> DumpFile  |         
                           |                |           |           |         
                           +--------^-------+           +-----------+         
                                    |                                         
    --------------------------------+-----------------------------------------
        Kernel                      |Netlink                                  
                           +--------v-------+                                 
                           |                |                                 
                           |Net/NC-SI module|                                 
                           |                |                                 
                           +--------^-------+                                 
                                    |                                         
                                    |NC-SI                                    
                                    |                                         
                           +--------v-------+                                 
                           |                |                                 
                           |       NIC      |                                 
                           |                |                                 
                           +----------------+                                           
```
## Alternatives Considered
We shall block duplicated dump procedure by the reception ordering of NC-SI command(0x4d) shall be maintained.
## Impacts
None.
## Testing
Co-work with NIC vendor(Broadcom) for dump process/file validation.

