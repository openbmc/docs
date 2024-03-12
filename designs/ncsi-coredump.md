# NC-SI core dump
Author: DelphineCCChiu (Delphine_CC_Chiu@wiwynn.com)

Created: 03/12/2024

## Problem Description

NIC core-dump is essential for NIC issue debugging, and it could be retrieved via both in-band and out of band method. The design here is providing the solution for NIC out of band dumping from BMC over NC-SI protocol.

## Background and References

NC-SI command for dump retrieval:
Reference: NC-SI spec v1.2: section: 8.4.114
<https://www.dmtf.org/sites/default/files/standards/documents/DSP0222_1.2.0.pdf>

NC-SI over MCTP:
<https://www.dmtf.org/sites/default/files/standards/documents/DSP0261_1.3.0.pdf>

## Requirements

This feature requires Linux kernel to support transferring new NC-SI command (0x4D) in net/ncsi module
<https://github.com/torvalds/linux/commits/master/net/ncsi>

## Proposed Design

This design will reuse existing phosphor-debug-collector module: <https://github.com/openbmc/phosphor-debug-collector>
User could call CreateDump Dbus-method with new NC-SI dump type under
"xyz.openbmc_project.Dump.Manager /xyz/openbmc_project/dump/bmc xyz.openbmc_project.Dump.Create" for dump creation.

The CreateDump method need one additional input parameter: "NICTarget" to indicate which NC-SI link to target.
An EID or network interface, such as eth0 could be a valid value.

A new collector script named "ncsicoredump" will be added for dump file generation.
And this script will use ncsi-netlink or ncsi-mctp(decided by NICTarget) utility to create the dump file and put it into dump folder for package preparation.

All NC-SI dump procedure will be implemented in ncsi-netlink and ncsi-mctp utility in phosphor-networkd:
<https://github.com/openbmc/phosphor-networkd/blob/master/src/ncsi_netlink_main.cpp>

The following block diagram illustrate entire dump procedure and relationship between modules:

```text

                           +----------------+           +-----------+                                                          
                           |                |           |           |                                                          
              ------------->  Dump manager  +-----------> DumpEntry |                                                          
               CreateDump  |                |           |           |                                                          
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
                           +--------v-------+                                                                                  
                           |                |                                                                                  
                           |  Plugin:       +------------------+                                                               
                           |  ncsicoredump  |                  |                                                               
                           |                |                  |                                                               
                           +--------+-------+                  |                                                               
                                    |                          |                                                               
                                    |                          |                                                               
                                    |                          |                                                               
                                    |                          |                                                               
      +------------+       +--------v-------+          +-------v------+        +------------+                                  
      |            |       |                |          |              |        |            |                                  
      |  DumpFile  <-------+  NCSI-NetLink  |          |  NCSI-MCTP   +-------->  DumpFile  |                                  
      |            |       |                |          |              |        |            |                                  
      +------------+       +--------^-------+          +-------^------+        +------------+                                  
                                    |                          |                                                               
    --------------------------------+--------------------------+-----------------------------                                  
        Kernel                      |Netlink                   |MCTP                                                           
                           +--------v-------+         +--------v-------+                                                       
                           |                |         |                |                                                       
                           |Net/NC-SI module|         |   I2C driver   |                                                       
                           |                |         |                |                                                       
                           +--------^-------+         +--------^-------+                                                       
                                    |                          |                                                               
                                    |NC-SI                     |SMBUS                                                          
                                    |                          |                                                               
                           +--------v--------------------------v-------+                                                       
                           |                                           |                                                       
                           |                     NIC                   |                                                       
                           |                                           |                                                       
                           +-------------------------------------------+                                                       

```

## Alternatives Considered

We shall block duplicated dump procedure by the reception ordering of NC-SI command(0x4d) shall be maintained.
Since the core dump will contain up to 2 crash dump inside, we only support core dump now by it's sufficient for current usage.

## Impacts

None.

## Testing

Co-work with NIC vendor(Broadcom) for dump process/file validation.

