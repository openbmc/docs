# NC-SI core dump

Author: DelphineCCChiu (<Delphine_CC_Chiu@wiwynn.com>)

Other contributors: Jagpal Singh Gill paligill@gmail.com

Created: 03/12/2024

## Problem Description

NIC core-dump is essential for NIC issue debugging, and it could be retrieved
via both in-band and out of band method. The design here is providing the
solution for NIC out of band dumping from BMC over NC-SI protocol.

## Background and References

NC-SI command for dump retrieval: Reference: NC-SI spec v1.2: section: 8.4.114
<https://www.dmtf.org/sites/default/files/standards/documents/DSP0222_1.2.0.pdf>

NC-SI over MCTP:
<https://www.dmtf.org/sites/default/files/standards/documents/DSP0261_1.3.0.pdf>

## Requirements

This feature requires Linux kernel to support transferring new NC-SI command
(0x4D) in net/ncsi module
<https://github.com/torvalds/linux/commits/master/net/ncsi>

## Proposed Design

This design will utilize the existing
[phosphor-debug-collector](https://github.com/openbmc/phosphor-debug-collector)
module.

### D-Bus Interface

This will use the existing D-Bus interface `xyz.openbmc_project.Dump.Create` and
`xyz.openbmc_project.Dump.Manager` service. The objectPath for the D-Bus
interface will be `/xyz/openbmc_project/dump/nic/<NIC_ID>`, where <NIC_ID>
refers to the Name field from entity manager configuration in next section.

To specify that the entity manager configuration is for a NIC, a new inventory
decorator called `xyz.openbmc_project.Inventory.Decorator.NIC` will be used.
More details about this decorator will be provided in the next section.

### Entity Manager

The schema definition for `xyz.openbmc_project.Inventory.Decorator.NIC`
decorator is as follows:

```
"NIC": {
    "additionalProperties": false,
    "properties": {
        "DeviceIndex": {
            "type": ["string", "number"]
        },
        "NCSITransport" {
            "enum": ["MCTP", "NETLINK", "NONE"]
        },
    },
    "required": ["DeviceIndex", "NCSITransport"],
    "type": "object"
},
```

- `DeviceIndex`: This field can take on values such as "eth0" or "0".
- `NCSIOverMCTP`: This field indicates whether the NIC uses MCTP as a transport
  for NCSI commands, or if it uses Netlink instead.

The following serves as an example of configuration:

```
{
    "Name": "BRCM OCP NIC FRU $bus",
    "xyz.openbmc_project.Inventory.Decorator.NIC": {
        "DeviceIndex": "eth0",
        "NCSITransport": "MCTP"
    }
}
```

### Dump Retrieval

Using standard NC-SI command: Retrieve Data From NC(0x4D) to get the dumps by
NC-SI over RBT or NC-SI over MCTP protocol. All NC-SI dump procedure will be
implemented in
[ncsi-netlink](https://github.com/openbmc/phosphor-networkd/blob/master/src/ncsi_netlink_main.cpp)
utility in phosphor-networkd with support for both crashdump and coredump.

### Integrate with phosphor-debug-collector

A new extension for NIC needs to be added in phosphor-debug-collector that can
implement the D-Bus interface mentioned above. Since phosphor-debug-collector
using shell scripts for data collection, a new collector script named
"ncsicoredump" will be added. This script will help to call ncsi-netlink or
ncsi-mctp by different NICTarget and generate dump file under specific folder.

The following block diagram illustrate entire dump procedure and relationship
between modules:

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
      |  DumpFile  |<------+  NCSI-NetLink  |          |  NCSI-MCTP   +------->|  DumpFile  |
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

For full system dumps, the phosphor-debug-collector will utilize the
`xyz.openbmc_project.Inventory.Decorator.NIC` inventory decorator to determine
the number of NICs in the system and gather other relevant information. Based on
this information, the Dreport flow will be called accordingly.

### BMCWeb

Currently, the Ethernet Interface does not support LogService. However, this
feature has been requested through the
[Redfish Forum](https://redfishforum.com/thread/1081/logservice-entry-nic-logdiagnosticdatatypes),
and once it becomes available, the LogService will be extended to support
Redfish URIs for EthernetInterfaces as well.

## Alternatives Considered

We shall block duplicated dump procedure by the reception ordering of NC-SI
command(0x4d) shall be maintained. Since the core dump will contain up to 2
crash dump inside, we only support core dump now by it's sufficient for current
usage.

## Impacts

None.

## Testing

Co-work with NIC vendor(Broadcom) for dump process/file validation.
