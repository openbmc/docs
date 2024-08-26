# NC-SI core dump

Author: DelphineCCChiu (<Delphine_CC_Chiu@wiwynn.com>)

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

### Interface

This design will reuse existing phosphor-debug-collector module:
<https://github.com/openbmc/phosphor-debug-collector> and extend the dump
creation interface with a new "NC-SICoreDump" dump type.

The D-Bus interface for dump creation will be:"xyz.openbmc_project.Dump.Manager
/xyz/openbmc_project/dump/bmc xyz.openbmc_project.Dump.Create"

To indicate which NC-SI link to target, The CreateDump method need one
additional input parameter: "NICTarget". An EID or network interface, such as
eth0 could be a valid value.

### Dump Retrieval

Using standard NC-SI command: Retrieve Data From NC(0x4D) to get the dumps by
NC-SI over RBT or NC-SI over MCTP protocol. All NC-SI dump procedure will be
implemented in ncsi-netlink and ncsi-mctp utility in phosphor-networkd:
<https://github.com/openbmc/phosphor-networkd/blob/master/src/ncsi_netlink_main.cpp>

### Integrate with phosphor-debug-collector

Since phosphor-debug-collector using shell scripts for data collection, a new
collector script named "ncsicoredump" will be added. This script will help to
call ncsi-netlink or ncsi-mctp by different NICTarget and generate dump file
under specific folder.

The following block diagram illustrate entire dump procedure and relationship
between modules:

### Redfish Integration for NIC Core Dumps

The NIC core dump functionality will support interaction through the Redfish API as
part of this design, in addition to the existing DBus-based interface.
This enhancement allows users to trigger and manage NIC core dumps using a
standardized Redfish interface, making the process more accessible and easier to
integrate with broader system management tools that utilize Redfish.

When a user sends a Redfish request to create a NIC dump, the Redfish service
leverages the existing DBus CreateDump method by issuing a corresponding command
to the xyz.openbmc_project.Dump.Manager service. The specific target path is
determined by the NIC device, which is encoded in the URL rather than passed as
a parameter in the payload. The response from the DBus command, which includes
details such as the dump’s status and file location, is then formatted into a
JSON object and returned to the user via the Redfish API.

Example Usage Redfish Endpoint: Each NIC device should have a separate endpoint.
For example:

/redfish/v1/Managers/bmc/LogServices/NIC0Dump/Actions/LogService.CreateDump
/redfish/v1/Managers/bmc/LogServices/NIC1Dump/Actions/LogService.CreateDump

Mapped DBus Path:

/xyz/openbmc_project/dump/nic {"Target": "eth0"}
/xyz/openbmc_project/dump/nic {"Target": "eth1"}

Triggering a NIC Dump via Redfish:

curl -k -u 'username:password' -X POST \
-H "Content-Type: application/json" \
-d '{}' \
https://"bmc-ip"/redfish/v1/Managers/bmc/LogServices/NIC0Dump/Actions/LogService.CreateDump

Example Redfish Response:

```json
{
  "@odata.type": "#LogEntry.v1_16_2.LogEntry",
  "Id": "1",
  "Name": "NIC0 Dump",
  "EntryType": "Event",
  "Severity": "OK",
  "Created": "2024-08-23T10:00:00+00:00",
  "Message": "NIC dump completed",
  "MessageId": "NIC0DumpCompleted",
  "Oem": {
    "DownloadURL":
    "https://<bmc-ip>/redfish/v1/Managers/bmc/LogServices/NIC0Dump/Entries/1/Actions/Download"
  }
}
```

Downloading the Dump File: curl -k -u 'username:password' \
https://"bmc-ip"/redfish/v1/Managers/bmc/LogServices/NIC0Dump/Entries/1/Actions/Download \
--output
nic0_dump_1.log

Explanation of the Workflow

1. Redfish Request: The user initiates the creation of a NIC dump by sending a
   POST request to a NIC-specific Redfish endpoint.

2. DBus Command Execution: The Redfish service triggers the corresponding DBus
   CreateDump command, which creates the dump and returns important details,
   such as the dump status and file location.

3. Redfish Response: The Redfish service provides the user with a JSON response
   containing all necessary information, including the status of the dump and a
   direct URL to download the file.

4. Downloading the File: The user uses the provided download URL to retrieve the
   dump file directly from the BMC, following the standard Redfish download
   process.

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

We shall block duplicated dump procedure by the reception ordering of NC-SI
command(0x4d) shall be maintained. Since the core dump will contain up to 2
crash dump inside, we only support core dump now by it's sufficient for current
usage.

## Impacts

None.

## Testing

Co-work with NIC vendor(Broadcom) for dump process/file validation.
