# Remote BIOS configuration via BMC - (Out of Band) OOB Bios Configuration

Author:
  Suryakanth Sekar! ssekar

Primary assignee:
  Suryakanth Sekar! ssekar

Other contributors:
  Jia Chunhui

Created: 19-Nov-2019

## Problem Description
Current OpenBMC doesn't provide mechanism to configure the BIOS remotely.
This is needed in data center to maintain several systems under
same configuration.

Remote BIOS Configuration provides ability for the user to view and modify
BIOS setup configuration parameters remotely via a BMC at any Host state.
New BIOS configuration parameters take place upon the next system reboot.

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
4. BMC should support bios/ attribute registry in redfish schema for
   BIOS configuration.
5. BMC should provide secure way for updating BIOS setup password settings.
Ex: Updating the BIOS password should be support only before end of post.


## Proposed Design

```
+----------------------------------------------------------------------------------------------------------------+
| Remote BIOS configuration via BMC                                                                              |
|                                                                                                                |
|                                                                                                                |
| +-------------+       +-------------+       +--------------------------------+    +--------+                   |
| |             |       |             |       |   OOB daemon                   |    |        |   +-----------+   |
| | NET/ Tools  +<-LAN->+ LAN-IPMID   +<Dbus->+  1- maintain the file transfer |    |        |   |Web Server |   |
| |             |       |             |       |  2- Unzip the XML zipped file  |    |        +<->+           |   |
| +-------------+       +-------------+       |  3- Validate the user input    |    |        |   +-----------+   |
|                                             |    Attribute variable and value|    |        |                   |
| +-------------+       +-------------+       |  4- Validate the XML type 0    |    |        |                   |
| |             |       |             |       |     Or PLDM data send via MCTP |    |        |                   |
| | HOST/ BIOS  +<-KCS->+  HOST-IPMID +<Dbus->+  5- Parse and convert XML type0|    |Redfish |   +------------+  |
| |             |       |             |       |      into BIOS Attribute       +<-D->+  API  |   |   Redfish  |  |
| +-----+-------+       +-------------+       |      Registry format           |  b |        +<->+   daemon   |  |
|       |                                     |  6- Receive the BIOS Attribute |  u |        |   +-----+------+  |
|       |                                     |     Registry  from BIOS        |  s |        |         ^         |
|       |                                     | (BIOS Redfish Host is defined) |    |        |         |         |
|       |               +--------------+      |  7- Receive the  BIOS Attribute|    |        |   +-----+--------+|
|       |               | FILES        |      |     Registry format from PLDM  |    |        |   |BIOS Redfish  ||
|       |               | 1-XML TYPE 0 +<---->+                                |    |        |   |Host interface||
|       |               | 2-XML TYPE 1 |      |                                |    |        |   +--------------+|
|       |               |  OR          |      +---------------^----------------+    +--------+                   |
|       |               | 3. PLDM files|                      |                                                  |
|       |               | 4. Redfish   |                      |                         Payload Type             |
|       |               |    format    |                      |                         -------------            |
|       |               +--------------+      +---------------V----------------+        0- XML type 0            |
|       |                                     | PLDM daemon(Payload Type 3-4-5)|        1- XML type 1            |
|       |                                     | Collect the BIOS data & convert|        2- Attribute Registry    |
|       +---------MCTP----------------------->| into BIOS Attribute Registry   |        3- BIOS string table     |
|                                             +--------------------------------+        4- Attribute name table  |
|                                                                                       5- Attribute value table |
|                                                                                       6- Pending value table   |
|                                                                                                                |
+----------------------------------------------------------------------------------------------------------------+
```

##BIOS send data in Intel properitay format:

There are two types of proprietary XML format files in BIOS configuration.
Type-0 contain full BIOS variables in XML format. (Generated by BIOS)
Type-1 contain modified BIOS variables in XML format. (Generated by BMC)

BIOS must provide BIOS OOB capability via KCS interface in early boot stage.
BIOS must send compressed proprietary file via IPMI command to the BMC.
IPMI interface will send the collected data to the OOB daemon.

OOB daemon should decompress & validate the received XML Type 0.
OOB daemon should validate the XML type 0.
OOB daemon should convert the XML Type 0 into attribute Registry Redfish format.
OOB daemon should create XML type 1 format based on the Dbus interface method
call from Redfish / Webserver / IPMI command.
OOB daemon should support below Dbus Method interface for redfish support.

During BDS phase in BIOS. BIOS must get the existing XML info from BMC.
If XML version/checksum is mismatch or XML type-0 is not present in BMC, then
BIOS must send XML type 0 to the BMC. If XML version and XML checksum matched
& pending BIOS knobs change list exist (XML type 1) in BMC then BIOS must get
pending BIOS configuration (XMLtype-1) from BMC & update its Hiidatabase and
send updated XML type-0 to the BMC in order to intact again and then BIOS reset
the system to reflect the updated values in BIOS boot.

BIOS have default bios settings in BIOS nonvolatile memory. BIOS can restore
the default bios configuration based on the flag setting in OEM IPMI command
issued during BIOS booting.
So, restore default bios configuration can be done by this mechanism.

```
#BIOS first boot
   +------------------------------------------------------------------------------------------------------------------------------------------+
   |                                                                                                                                          |
   | +-----------------------+             +-------------------------------------------------------------------------------------------------+|
   | | BIOS                  |             |   BMC                                                                                           ||
   | |                       |             |  +-------------------------------+        +------------------------------+                      ||
   | |                       |             |  |IPMI Interface (kcs)           |        |OOB daemon Manager            |                      ||
   | |                       |             |  | -Responsible for send /recv   |        |-Responsible for handling BIOS|                      ||
   | |                       |             |  |  data between BIOS and BMC    |        | configuration attributes.    |                      ||
   | |                       |             |  |                               |        | -BIOSCapability              |                      ||
   | |                       |             |  |                               |        | -FactoryDefaultSetting       |                      ||
   | |                       |             |  |                               |        | -BIOSPwdHashData             |                      ||
   | |                       |             |  |                               |        | -MaxPayloadSupported =15     |                      ||
   | |                       |             |  |                               |        | -inBandIntf(IPMI)            |                      ||
   + |                       |             |  +-------------------------------+        +------------------------------+                      ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+                      ||
   | | Set BIOS capability   |<---Req-/Res--> |  Forward to the OOB daemon    |--dbus-->| Set the BIOSCapability.     |                      ||
   | |                       |             |  |                               |         |  &inBandIntf                |                      ||
   | |Set BIOS Pwd hash/Seed |<---Req-/Res--> | Forward to the OOB daemon     |--dbus-->| Set BIOSPwdHash data.       |                      ||
   | |                       |             |  |                               |         | Set the IsBIOSInitDone.     |                      ||
   | |                       |             |  |                               |         |                             |                      ||
   | |Check factory settings |<---Req-/Res--> |  Get the FactoryDefault prop  |<-dbus-- | Send FactoryDefaultSettings |                      ||
   | |Init the BIOS config   |             |  |                               |         |                             |    +---------------+ ||
   | |Based on value.        |             |  |                               |         |                             |    |Payload 0      | ||
   | |Get the XML Type0 info |<---Req-/Res--> | Get the data from OOB daemon  |<-dbus-- | Payload 0 is not present.   |    | Object        | ||
   | |Generate & compress    |             |  |                               |         |                             |    |-FilePath      | ||
   | |XML type 0 file        |             |  |                               |         |Validate the payload 0.      |--->|-FileSize      | ||
   | |                       |             |  |                               |--dbus-->|Create the Payload 0 object& |    |-PayloadStatus | ||
   | | Send the payload file |<---Req-/Res--->| Forward to the OOOB daemon    |         |Del the Payload 1 if exist.  |    |-PayloadFlag   | ||
   | | via SetPayload command|             |  |                               |         |Use the Payload 0 and create |    |-PayloadChksum | ||
   | |                       |             |  |                               |         | Attribute Registry format.  |    +---------------+ ||
   | | Continue the BIOS boot|             |  |                               |         |                             |           |          ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+           |          ||
   |                                       +-------------------------------------------------------------------------------------------------+|
   +---------------------------------------+--------------------------------------------------------------------------------------------------+
```
```
#BIOS DC cycle
   +------------------------------------------------------------------------------------------------------------------------------------------+
   |                                                                                                                                          |
   | +-----------------------+             +-------------------------------------------------------------------------------------------------+|
   | | BIOS                  |             |   BMC                                                                                           ||
   | |                       |             |  +-------------------------------+        +------------------------------+                      ||
   | |                       |             |  |IPMI Interface (kcs)           |        |OOB daemon Manager            |                      ||
   | |                       |             |  | -Responsible for send /recv   |        |-Responsible for handling BIOS|                      ||
   | |                       |             |  |  data between BIOS and BMC    |        | configuration attributes.    |                      ||
   | |                       |             |  |                               |        | -BIOSCapability              |                      ||
   | |                       |             |  |                               |        | -FactoryDefaultSetting       |                      ||
   | |                       |             |  |                               |        | -BIOSPwdHashData             |  +-----------------+ ||
   | |                       |             |  |                               |        | -MaxPayloadSupported =15     |  | Payload 0       | ||
   | |                       |             |  |                               |        | -inBandIntf(IPMI)            |  | Object          | ||
   + |                       |             |  +-------------------------------+        +------------------------------+  | -FilePath       | ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+  | -FileSize       | ||
   | | Set BIOS capability   |<---Req-/Res--> | Forward to the OOB daemon     |--dbus-->| Set the BIOSCapability      |  | -PayloadStatus  | ||
   | |                       |             |  |                               |         |  &inBandIntf                |  | -Payloadflag    | ||
   | |Set BIOS Pwd hash/Seed |<---Req-/Res--> | Forward to the OOB daemon     |--dbus-->| Set BIOSPwdHash data        |  | -PayloadChksum  | ||
   | |                       |             |  |                               |         | Set the IsBIOSInitDone      |  |                 | ||
   | |                       |             |  |                               |         |                             |  +-----------------+ ||
   | |Check factory settings |<---Req-/Res--> | Get the FactoryDefault prop   |<-dbus-- | Send FactoryDefaultSettings |                      ||
   | |Init the BIOS config   |             |  |                               |         |                             |    +---------------+ ||
   | |Based on value.        |             |  |                               |         |                             |    |Payload 1      | ||
   | |Get the XML Type0 info |<---Req-/Res--> | Get the data from OOB daemon  |<-dbus-- | Payload 0 is present.       |    | Object        | ||
   | |Generate & compress    |             |  |                               |         |                             |    |-FilePath      | ||
   | |XML type 0 file.       |             |  |                               |         |                             |--->|-FileSize      | ||
   | |Check Payload0 Chksum. |             |  |                               |--dbus-->|Create the Payload 0 object  |    |-PayloadStatus | ||
   | |If Chksum mismatch     |<---Req-/Res--->| Forward new data to the OOB   |         |Delete the Payload 1.        |    |-PayloadFlag   | ||
   | |then send the payload  |             |  | daemon                        |         | Use Payload 0 and create    |    |-PayloadChksum | ||
   | |via SetPayload.        |             |  |                               |         | Attribute registry format   |     +---------------+||
   | |Continue the BIOS boot |             |  |                               |         |                             |           |          ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+           |          ||
   |                                       +-------------------------------------------------------------------------------------------------+|
   +---------------------------------------+--------------------------------------------------------------------------------------------------+
```
```
#BIOS Dc cycle and BMC have new values
   +------------------------------------------------------------------------------------------------------------------------------------------+
   |                                                                                                                                          |
   | +-----------------------+             +-------------------------------------------------------------------------------------------------+|
   | | BIOS                  |             |   BMC                                                                                           ||
   | |                       |             |  +-------------------------------+        +------------------------------+                      ||
   | |                       |             |  |                               |        |                              |                      ||
   | |                       |             |  |IPMI Interface (kcs)           |        |OOB daemon Manager            |                      ||
   | |                       |             |  | -Responsible for send /recv   |        |-Responsible for handling BIOS|                      ||
   | |                       |             |  |  data between BIOS and BMC    |        | configuration attributes.    |                      ||
   | |                       |             |  |                               |        | -BIOSCapability              |                      ||
   | |                       |             |  |                               |        | -FactoryDefaultSetting       |                      ||
   | |                       |             |  |                               |        | -BIOSPwdHashData             |  +-----------------+ ||
   | |                       |             |  |                               |        | -MaxPayloadSupported =15     |  | Payload 0       | ||
   | |                       |             |  |                               |        | -inBandIntf(IPMI)            |  | Object          | ||
   + |                       |             |  +-------------------------------+        +------------------------------+  | -FilePath       | ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+  | -FileSize       | ||
   | |Set BIOS capability    |<---Req-/Res--> | Forward to the OOB daemon     |---dbus->| Set the BIOSCapability      |  | -PayloadStatus  | ||
   | |                       |             |  |                               |         |  &inBandIntf                |  | -Payloadflag    | ||
   | |Set BIOS Pwd hash/Seed |<---Req-/Res--> | Forward to the OOB daemon     |---dbus->| Set BIOSPwdHash data        |  | -PayloadChksum  | ||
   | |                       |             |  |                               |         | Set the IsBIOSInitDone      |  |                 | ||
   | |                       |             |  |                               |         |                             |  +-----------------+ ||
   | |Check factory settings |<---Req-/Res--> |  Get the FactoryDefault prop  |<--dbus--| Send FactoryDefaultSettings |                      ||
   | |Init the BIOS config   |             |  |                               |         |                             |    +---------------+ ||
   | |Based on value.        |             |  |                               |         |                             |    |Payload 1      | ||
   | |Get the XML Type0 info |<---Req-/Res--> | Get the data from OOB daemon  |<--dbus--| Payload 0 is present.       |    | Object        | ||
   | |Generate & compress    |             |  |                               |         |                             |    |-FilePath      | ||
   | |XML type 0 file.       |             |  |                               |         |                             |--->|-FileSize      | ||
   | |Check Payload0 Chksum. |             |  |                               |         |                             |    |-PayloadStatus | ||
   | |If Chksum   match      |<---Req-/Res--->| Forward new data to the OOB   |<--dbus--|Send the Payload 1 info      |    |-PayloadFlag   | ||
   | |then get the payload 1 |             |  | daemon                        |         | which is created by user    |    |-PayloadChksum | ||
   | |via GetPayload.        |             |  |                               |         | ( Contain new values)       |    +---------------+ ||
   | |Get  the XML type 1    |             |  |                               |         |                             |           |          ||
   | |via GetPayload command.|<---Req-/Res--->| Get the data from OOB daemon  |<--dbus--| Payload 1 is present        |           |          ||
   | |                       |             |  |                               |         |                             |           |          ||
   | |Update the new value   |             |  |                               |         |                             |           |          ||
   | |and new chksum in      |             |  |                               |         |                             |           |          ||
   | |BIOS.                  |             |  |                               |         |                             |           |          ||
   | |Reset the system.      |             |  |                               |         |                             |                      ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+                      ||
   |                                       +-------------------------------------------------------------------------------------------------+|
   +---------------------------------------+--------------------------------------------------------------------------------------------------+

```

##BIOS send the data in BIOS configuration PLDM via MCTP:

Following Payload types are supported in PLDM via MCTP:
Payload Type 3- BIOS string table
Payload Type 4- BIOS Attribute name table
Payload Type 5- BIOS Attribute value table
Payload type 6-  BIOS Attribute Pending value table

BIOS should update the BIOS settings via Set BIOS table PLDM command-
BIOS string table, Attribute name table, Attribute value table via MCTP.
PLDM daemon should set the OOB support capability via Dbus call.
PLDM daemon should send all BIOS tables to the OOB daemon.
OOB daemon should generate the Attribute Registry foramt based on received
tables.

OOB daemon should create pending BIOS configuration value table based on the
user input configuration. PLDM daemon should retrieve the pending data
(changed value) from OOB daemon and send to the BIOS via PLDM command via MCTP.


If BIOS provide the BIOS configuration PLDM via MCTP interface, then PLDM daemon
should set the OOB BIOS capability & collected data in Attribute Registry format
to the OOB daemon.

BIOS must collect the new attribute values from BMC and update the same in BIOS.
BIOS must send the updated master attributes PLDM via MCTP to the BMC and
reset the system to reflect the BIOS configuration.

#BIOS first boot
   +------------------------------------------------------------------------------------------------------------------------------------------+
   |                                                                                                                                          |
   | +-----------------------+             +-------------------------------------------------------------------------------------------------+|
   | | BIOS                  |             |   BMC                                                                                           ||
   | |                       |             |  +-------------------------------+         +-----------------------------+                      ||
   | |                       |             |  |PLDM Interface (MCTP)          |         |OOB Daemon Manager           |                      ||
   | |                       |             |  | -Responsible for send /recv   |         |-Responsible for handle BIOS |                      ||
   | |                       |             |  |  data between BIOS and BMC    |         | configuration attributes.   |                      ||
   | |                       |             |  |                               |         | -BIOSCapability             |                      ||
   | |                       |             |  |                               |         | -MaxPayloadSupported =15    |                      ||
   | |                       |             |  |                               |         | -inBandIntf(MCTP)           |                      ||
   + +-----------------------+             |  +-------------------------------+         +-----------------------------+                      ||
   | |Get the Payload info   |<---Req-/Res--> | Get the data from OOB Daemon  |<-dbus---|Payload 3-4-5 is not present |                      ||
   | |or Check Payload Tag   |             |  |                               |         |                             |                      ||
   | |using GetBIOS table    |             |  |                               |         |                             |                      ||
   | |                       |             |  |                               |         |                             |                      ||
   | |Set BIOS string table  |<---Req-/Res--> | Forward to the OOB Daemon     |--dbus-->| Create the Payload 3 Object |   +---------------+  ||
   | | via Set BIOS table    |             |  |                               |         | &inBandIntf /BIOSCapability |   |Payload 3-4-5  |  ||
   | |SetBIOSAttribute table |<---Req-/Res--> | Forward to the OOB Daemon     |--dbus-->| Create the Payload 4 Object |   | Objects       |  ||
   | |SetBIOSAttributeValue  |<---Req-/Res--> | Forward to the OOB Daemon     |--dbus-->| Create the Payload 5 Object |   |-Payloadchksum |  ||
   | |table via SetBIOStable |             |  |                               |         |                             |   |-filePath      |  ||
   | |Init the BIOS config   |             |  |                               |         |                             |   |-fileSize      |  ||
   | |Based on value.        |             |  |                               |         |                             |   |Payload Status |  ||
   | |via Set BIOS table     |             |  |                               |         |                             |   |-Payload Flag  |  ||
   | |                       |             |  |                               |         |Use Payload 3-4-5 and create |   +---------------+  ||
   | | Continue the BIOS boot|             |  |                               |         | the AttributeRegistry format|           |          ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+           |          ||
   |                                       +-------------------------------------------------------------------------------------------------+|
   +---------------------------------------+--------------------------------------------------------------------------------------------------+
  
  
#BIOS DC Cycle
   +------------------------------------------------------------------------------------------------------------------------------------------+
   |                                                                                                                                          |
   | +-----------------------+             +-------------------------------------------------------------------------------------------------+|
   | | BIOS                  |             |   BMC                                                                                           ||
   | |                       |             |  +-------------------------------+         +-----------------------------+                      ||
   | |                       |             |  |PLDM Interface (MCTP)          |         |OOB Daemon Manager           |                      ||
   | |                       |             |  | -Responsible for send /recv   |         |-Responsible for handle BIOS |                      ||
   | |                       |             |  |  data between BIOS and BMC    |         | configuration attributes.   |                      ||
   | |                       |             |  |                               |         | -BIOSCapability             |                      ||
   | |                       |             |  |                               |         | -MaxPayloadSupported =15    |                      ||
   | |                       |             |  |                               |         | -inBandIntf(MCTP)           |                      ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+                      ||
   | |Get the Payload info   |<---Req-/Res--> | Get the data from OOB Daemon  |<-dbus---|Payload 3-4-5 is present     |                      ||
   | |or Check Payload Tag   |             |  |                               |         |                             |                      ||
   | |using GetBIOS table    |             |  |                               |         |                             |                      ||
   | |                       |             |  |                               |         |                             |                      ||
   | | Get the Payload 6     |<---Req-/Res--> |Get the data from OOB Daemon   |<-dbus---|Payload 6-Pending value table|                      ||
   | | using Get BIOS table  |             |  |                               |         |Object created by user       |   +---------------+  ||
   | |                       |             |  |                               |         |                             |   |Payload 3-4-5  |  ||
   | |SetBIOSAttributeValue  |<---Req-/Res--> | Forward to the OOB Daemon     |--dbus-->| Create the Payload 5 Object |   |-Payloadchksum |  ||
   | |table via SetBIOStable.|             |  |                               |         | Del the Pending value table |   |-filePath      |  ||
   | |Init the BIOS config   |             |  |                               |         | & Payload 6                 |   |-fileSize      |  ||
   | |Based on value.        |             |  |                               |         |                             |   |Payload Status |  ||
   | |                       |             |  |                               |         |                             |   |-Payload Flag  |  ||
   | |                       |             |  |                               |         |Use Payload 3-4-5 and create |   +---------------+  ||
   | | Reset the BIOS boot   |             |  |                               |         | the AttributeRegistry format|           |          ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+           |          ||
   |                                       +-------------------------------------------------------------------------------------------------+|
   +---------------------------------------+--------------------------------------------------------------------------------------------------+

##Complete BIOS BMC flow for BIOS configuration update
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
```
#Redfish interfaces for OOB Bios configuration

1. Get BIOS resource-GET command:
 "https://<BMC IP address>/redfish/v1/Systems/system/Bios"
2. Get Bios Attribute Registry-GET command:
"https://<BMC IP address>/redfish/v1/Registries/Bios"
3. Change BIOS password-POST command:
"https://<BMC IP address>/redfish/v1/Systems/system/Bios/Actions/Bios.ChangePassword"
4. Reset BIOS- POST command:
 "https://<BMC IP address>/redfish/v1/Systems/system/Bios/Actions/Bios.ResetBios"
5. Update new Bios setting, PATCH command:
"https://<BMC IP address>/redfish/v1/Systems/system/Bios/Pending"
6. Get the new Bios setting, GET command:
 "https://<BMC IP address>/redfish/v1/Systems/system/Bios/Pending"


   +---------------------------------------------------------------------------------------------------------+
   | +-----------------------+             +----------------------------------------------------------------+|
   | | OOBWeb tool -POSTMAN  |             |   BMC                                                          ||
   | |                       |             |  +-----------------------+       +---------------------------+ ||
   | |                       |             |  |Redfish Daemon         |       |OOB Daemon Manager         | ||
   | |                       |             |  |-Responsible for handle|       |-Parse Bios Data,convert to| ||
   | |                       |             |  |all Redfish request    |       | required format & return  | ||
   | |                       |             |  +-----------------------+       +---------------------------+ ||
   | +-----------------------+             |  +-----------------------+       +---------------------------+ ||
   | |Get BIOS resource      |<---Req-/Res--> |  Call OOB dbus Method |-dbus->| getAttributes()           | ||
   | |                       |             |  |                       |       |                           | ||
   | |Get Bios Attribute Reg |<---Req-/Res--> |  Call OOB dbus Method |-dbus->| getBiosAttributeRegistry()| ||
   | |                       |             |  |                       |       |                           | ||
   | |Change Bios Password   |<---Req-/Res--> |  Call OOB dbus Method |<dbus- | changeBiosPassword()      | ||
   | |                       |             |  |                       |       |                           | ||
   | |Reset the BIOS         |<---Req-/Res--> |  Call OOB dbus Method |<dbus- | resetBios()               | ||
   | |                       |             |  |                       |       |                           | ||
   | |Get/update BIOS setting|<---Req-/Res--->|  Call OOB dbus Method |-dbus->| get/update BiosSetting()  | ||
   | |                       |             |  |                       |       |                           | ||
   | +-----------------------+             |  +-----------------------+       +---------------------------+ ||
   |                                       +---------------------------------------------------------------+||
   +---------------------------------------+-----------------------------------------------------------------+
```

## Alternatives Considered
Syncing the BIOS configuration via Redfish Host interface.
Redfish Host specification definition is not completed and ready
BIOS support also not available.
There are 1000+ bios variables and storing in phosphor-dsetting is not optimal


## Impacts
BIOS must support and follow OOB BIOS configuration flow.

## Testing
Able to change the BIOS configuration via BMC even BIOS is S0/ S5 state
Able to change the BIOS setup password via BMC
