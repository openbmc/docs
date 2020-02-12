# Remote BIOS configuration via BMC - (Out of Band) OOB Bios Configuration

Author:
  Suryakanth Sekar! ssekar

Primary assignee:
  Suryakanth Sekar! ssekar

Other contributors:
  Jia Chunhui

Created: 19-Nov-2019

## Problem Description
Current OpenBMC doesn't provide mechanism to update BIOS setup page in a
scalable & by avoiding user intervention. This is needed in data center
to maintain several systems under same configuration.

Remote BIOS Configuration provides ability for the user to view and modify
BIOS setup configuration parameters remotely via a BMC LAN channel
at any Host state. New BIOS configuration parameters take place upon the
next system reboot.

## Background and References
[1] https://www.dmtf.org/sites/default/files/standards/documents/DSP0247_1.0.0.pdf
[2] https://redfish.dmtf.org/schemas/v1/Bios.v1_1_0.json
[3] https://redfish.dmtf.org/schemas/v1/AttributeRegistry.v1_3_2.json

## Requirements
1. Mechanism to configure BIOS settings remotely over network interface.
2. BMC should support the ability to set the value of all BIOS variables
   to the factory default state.
3. When the system is in S0, S3 or S4 state. BMC should mark any new BIOS sets
   as pending and should only apply new BIOS configuration on the next reboot
   of the host.
4. BMC should support BIOS schema for BIOS configuration.
5. BMC should provide secure method for updating BIOS setup password settings.

## Proposed Design

```
+----------------------------------------------------------------------------------------------------------------+
| Remote BIOS configuration via BMC                                                                              |
|                                                                                                                |
|                                                                                                                |
| +-------------+       +-------------+       +--------------------------------+    +--------+                   |
| |             |       |             |       |   OOB Daemon                   |    |        |   +-----------+   |
| | NET/ Tools  +<-LAN->+ LAN-IPMID   +<Dbus->+  1- maintain the file transfer |    |        |   |Web Server |   |
| |             |       |             |       |  2- Unzip the XML zipped file  |    |        +<->+           |   |
| +-------------+       +-------------+       |  3- Validate the user input    |    |        |   +-----------+   |
|                                             |    Attribute variable and value|    |        |                   |
| +-------------+       +-------------+       |  4- Validate the XML type 0    |    |        |                   |
| |             |       |             |       |     Or PLDM data send via MCTP |    |        |                   |
| | HOST/ BIOS  +<-KCS->+  HOST-IPMID +<Dbus->+  5- Parse and convert XML type0|    |Redfish |   +------------+  |
| |             |       |             |       |      into BIOS Attribute       +<-D->+  API  |   |   Redfish  |  |
| +-----+-------+       +-------------+       |      Registry format           |  b |        +<->+   Daemon   |  |
|       |                                     |  6- Receive the BIOS Attribute |  u |        |   +-----+------+  |
|       |                                     |     Registry  from BIOS        |  s |        |         ^         |
|       |                                     | (BIOS Redfish Host is defined) |    |        |         |         |
|       |               +--------------+      |  7- Receive the  BIOS Attribute|    |        |   +-----+--------+|
|       |               | FILES        |      |     Registry format from PLDM  |    |        |   |BIOS Redfish  ||
|       |               | 1-XML TYPE 0 +<---->+                                |    |        |   |Host interface||
|       |               | 2-XML TYPE 1 |      |                                |    |        |   +--------------+|
|       |               |  OR          |      +---------------^----------------+    +--------+                   |
|       |               | 3. PLDM data |                      |                                                  |
|       |               | 4. Redfish   |                      |                                                  |
|       |               |    format    |                      |                                                  |
|       |               +--------------+      +---------------V----------------+                                 |
|       |                                     | PLDM Daemon                    |                                 |
|       |                                     | Collect the BIOS data & convert|                                 |
|       +---------MCTP----------------------->| into BIOS Attribute Registry   |                                 |
|                                             +--------------------------------+                                 |
|                                                                                                                |
|                                                                                                                |
|                                                                                                                |
+----------------------------------------------------------------------------------------------------------------+
```

#BIOS send the data in proprietary BIOS attribute file format:
BIOS must provide BIOS OOB capability via KCS interface in early boot stage.
BIOS must send compressed proprietary file via IPMI command to the BMC.
IPMI interface will send the collected data to the OOB daemon.

There are two types of proprietary XML format files in BIOS configuration.
Type-0 contain full BIOS variables in XML format. (Generated by BIOS)
Type-1 contain modified BIOS variables in XML format. (Generated by BMC -OOB Daemon)

OOB daemon should decompress & validate the received XML Type 0.
OOB daemon should validate the XML type 0.
OOB daemon should convert the XML Type 0 into attribute Registry Redfish format.
OOB daemon should create XML type 1 format based on the Dbus interface method call
from Redfish / Webserver / IPMI command.
OOB daemon should support below Dbus interface. 

https://gerrit.openbmc-project.xyz/c/openbmc/phosphor-dbus-interfaces/+/18242

During BDS phase in BIOS. BIOS must get the existing XML info from BMC.
If XML version/XML checksum is mismatch or XML type-0 is not present in BMC, then
BIOS must send XML type 0 to the BMC. If XML version and XML checksum matched
& pending BIOS knobs change list exist (XML type 1) in BMC then BIOS must get
pending BIOS configuration (XMLtype-1) from BMC & update its Hiidatabase and
send updated XML type-0 to the BMC in order to intact again and then BIOS reset
the system to reflect the updated values in BIOS boot.

BIOS have default bios settings in BIOS nonvolatile memory. BIOS can restore
the default bios configuration based on the flag setting in OOB OEM IPMI command
issued during BIOS booting.
So, restore default bios configuration can be done by this mechanism.

#BIOS send the data in BIOS configuration PLDM via MCTP:
PLDM daemon should set the OOB support capability via Dbus call.
PLDM daemon should collect all BIOS configuration PLDM data convert into
attribute Registry Redfish format and send it to the OOB Daemon.

OOB daemon should create pending BIOS configuration value table based on the
user input configuration. PLDM daemon should retrieve the pending data
(changed value) from OOB daemon and send to the BIOS via PLDM command via MCTP.

BIOS must send the master BIOS attributes file (Intel proprietary - XML type 0)
via KCS interface or all attributes details via PLDM via MCTP or
Redfish host interface during first boot.

BIOS must send supported proprietary BIOS attribute file format or BIOS config PLDM 
via MCTP or Redfish BIOS attribute registry format to the BMC whenever new settings
applied in BIOS.

If BIOS provide the BIOS configuration PLDM via MCTP interface, then PLDM daemon
should set the OOB BIOS capability and collected data in Attribute Registry format
to the OOB daemon.

BIOS must collect the new attribute values from BMC and update the same in BIOS.
BIOS must send the updated master attributes PLDM via MCTP to the BMC and 
reset the system to reflect the BIOS configuration.

#BIOS BMC flow for BIOS configuration update
```
+----------------------------------------+                    +----------------------------------------+
|                BIOS                    |                    |                  BMC                   |
|                                        |                    |                                        |
|  +----------------------------------+  |                    | +------------------------------------+ |
|  | Send the BIOS capability  Support|  |--------KCS-------->| |1.Get the File & unzipped if zipped.| |
|  | Send the compressed BIOS file(or)|  |-MCTP/KCS/Redfish-->| |2.Validate and convert into         | |
|  | Send PLDM data via MCTP  (or)    |  |                    | |  BIOS Attribute Registry format.   | |
|  | Send the Redfish host interface  |  |                    | |3.Expose the Dbus interface         | |
|  +----------------------------------+  |                    | +------------------------------------+ |
|                                        |                    |                                        |
|  +----------------------------------+  |                    |                                        |
|  | Get the file info & config status|   <-Get config status-|                                        |
|  | - Any config changed or not      |  |                    |                                        |
|  | - File checksum in BMC           |  |                    |                                        |
|  | - New attribute values exist     |  |                    |                                        |
|  +----------------------------------+  |                    |                                        |
|                                        |                    |  +-----------------------------------+ |
|  +----------------------------------+  |                    |  |                                   | |
|  | If new attribute  value exist    |<-|-----------------------|  Send the BIOS Attribute data     | |
|  |           then                   |  |                    |  |                                   | |
|  | Get & Update the BIOS variables  | -| -----+             |  |                                   | |
|  |                                  |  |      |             |  +-----------------------------------+ |
|  +---------------+------------------+  |      |             |                                        |
|                  |                     |      |             |                                        |
|                  YES                   |      |             |                                        |
|                  |                     |      |             |  +----------------------------------+  |
|   +--------------V------------------+  |      |             |  |                                  |  |
|   |  Send the updated file to BMC   |  |      |             |  | Update the BIOS Attribute data   |  |
|   |                                 |------------------------->|                                  |  |
|   +---------------------------------+  |      |             |  +----------------------------------+  |
|                                        |      |             |                                        |
|                                        |      |             |                                        |
|   +---------------------------------+  |      |             |                                        |
|   | Reset the BIOS for BIOS conf    |  |     NO             |                                        |
|   | update                          |  |      |             |                                        |
|   +---------------------------------+  |      |             |                                        |
|                                        |      |             |                                        |
|  +----------------------------------+  |      |             |                                        |
|  |  Continue the BIOS boot          | <-------+             |                                        |
|  +----------------------------------+  |                    |                                        | 
+----------------------------------------+                    +----------------------------------------+
```
 
## Alternatives Considered
Syncing the BIOS configuration via Redfish Host interface.
Redfish Host specification definition is not completed and ready
BIOS support is not available.

## Impacts
BIOS must support and follow OOB BIOS configuration flow.
BIOS must change the existing flow for support set/update BIOS password.

## Testing
Able to change the BIOS configuration via BMC even BIOS is S0/ S5 state
Able to change the BIOS setup password via BMC
