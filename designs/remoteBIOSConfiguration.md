# Remote BIOS Configuration(RBC) via BMC

Author:
  Suryakanth Sekar! ssekar

Primary assignee:
  Suryakanth Sekar! ssekar

Other contributors:
  Jia Chunhui
  Deepak Kodihalli

Created: 19-Nov-2019

## Problem Description
Current OpenBMC doesn't provide mechanism to configure the BIOS remotely.
This is needed in data center to maintain several systems under
same configuration.

Remote BIOS Configuration provides ability for the user to view and modify
BIOS setup configuration parameters remotely via a BMC at any Host state.
New BIOS configuration parameters take effect immediately or next
system reboot based on the host firmware support model.

## Background and References
[1] https://www.dmtf.org/sites/default/files/standards/documents/DSP0247_1.0.0.pdf
[2] https://redfish.dmtf.org/schemas/v1/Bios.v1_1_0.json
[3] https://redfish.dmtf.org/schemas/v1/AttributeRegistry.v1_3_2.json

## Requirements
1. Mechanism to configure BIOS settings remotely over network interface.
2. BMC should support the ability to set the value of all BIOS variables
   to the factory default state.
3. Based on the host firmware support model, BMC should support both
   Immediate Update or Deferred update.
4. In deferred model, When the system is in S0, S3 or S4 state.
   BMC should mark any new BIOS sets as pending and should only apply
   new BIOS configuration on the next reboot of the host.
5. In immediate model, BMC should send message to the system firmware (BIOS)
   Whenever settings are changed.
6. BMC should support bios/ attribute registry in redfish schema for
   BIOS configuration.
7. BMC should provide secure way for updating BIOS setup password settings.
Ex: Updating the BIOS password should be support only before end of post.


## Proposed Design

```
+----------------------------------------------------------------------------------------------------------------+
| Remote BIOS configuration (RBC) via BMC                                                                        |
|                                                                                                                |
|                                                                                                                |
| +-------------+       +-------------+       +--------------------------------+      +-------+                  |
| |             |       |             |       |   RBC daemon                   |      |       |    +----------+  |
| | NET/ Tools  +<-LAN->+ LAN-IPMID/  +<Dbus->+  1- maintain Payload transfer  |      |       |    |Web client|  |
| |             |       | REDFISH     |       |  2- Unzip file -Payload 0      |      |       |    |          |  |
| +-------------+       +-------------+       |  3- Validate the user input    |      |       |    +----^-----+  |
|                                             |    Attribute variable and value|      |       |         |        |
| +-------------+       +-------------+       |  4- Validate the XML type 0    |      |       |        LAN       |
| |             |       |             |       |     Or PLDM data send via MCTP |      |       |         |        |
| | HOST/ BIOS  +<-KCS->+  HOST-IPMID +<Dbus->+  5- Parse and convert XML type0|      |Redfish|    +----V-----+  |
| |             |       |             |       |      into BIOS Attribute       +<Dbus>+  API  |    |Redfish & |  |
| +-----+-------+       +-------------+       |      Registry format           |      |       +<-->+BMCWeb    |  |
|       |                                     |  6- Receive the BIOS Attribute |      |       |    +----^-----+  |
|       |                                     |     Registry  from BIOS        |      |       |         |        |
|       |                                     | (BIOS Redfish Host is defined) |      |       |         |        |
|       |               +--------------+      |  7- Receive the BIOS table or  |      |       |    +----V-----+  |
|       |               | FILES        |      |     Registry format from PLDM  |      |       |    | Redfish  |  |
|       |               | 1-XML TYPE 0 +<---->+                                |      |       |    |  Host    |  |
|       |               | 2-XML TYPE 1 |      |                                |      |       |    | Interface|  |
|       |               |  OR          |      +----^-----------------+---------+      +-------+    +----------+  |
|       |               | 3. PLDM files|           |       AttributeChanged Signal                               |
|       |               | 4. Redfish   | SetPayload/UpdatePayloadInfo|                  Payload Type             |
|       |               |    format    |           |                 |                  -------------            |
|       |               +--------------+      +----V-----------------V---------+        0- XML type 0            |
|       |                                     | PLDM daemon(Payload Type 3-4-5)|        1- XML type 1            |
|       |                                     | Collect the BIOS data & convert|        2- Attribute Registry    |
|       +---------MCTP----------------------->| into BIOS Attribute Registry   |        3- BIOS string table     |
|                                             | Payload Type 2 to the RBC  or  |        4- Attribute name table  |
|                                             | PLDM  send Payload type 3-4-5  |        5- Attribute value table |
|                                             | RBC will generate the registry |        6- Pending value table   |
|                                             +--------------------------------+                                 |
+----------------------------------------------------------------------------------------------------------------+
```

##BIOS send data in Intel proprietary format:

There are two types of proprietary XML format files in BIOS configuration.
Type-0 contain full BIOS variables in XML format. (Generated by BIOS)
Type-1 contain modified BIOS variables in XML format. (Generated by BMC)

BIOS must provide BIOS RBC capability via KCS interface in early boot stage.
BIOS must send compressed proprietary file via IPMI command to the BMC.
IPMI interface will send the collected data to the RBC daemon.

RBC daemon should decompress & validate the received XML Type 0.
RBC daemon should validate the XML type 0.
RBC daemon should convert the XML Type 0 into attribute Registry Redfish format.
RBC daemon should create XML type 1 format based on the Dbus interface method
call from Redfish / Webserver / IPMI command.
RBC daemon should support below Dbus Method interface for redfish support.

During BDS phase in BIOS. BIOS must get the existing XML info from BMC.
If XML version/checksum is mismatch or XML type-0 is not present in BMC, then
BIOS must send XML type 0 to the BMC. If XML version and XML checksum matched
& pending BIOS knobs change list exist (XML type 1) in BMC then BIOS must get
pending BIOS configuration (XMLtype-1) from BMC & update in BIOS region and
send updated XML type-0 to the BMC in order to intact again and then BIOS reset
the system to reflect the updated values in BIOS boot.

BIOS have default bios settings in BIOS non-volatile memory. BIOS can restore
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
   | |                       |             |  |IPMI Interface (kcs)           |        |RBC daemon Manager            |                      ||
   | |                       |             |  | -Responsible for send /recv   |        |-Responsible for handling BIOS|                      ||
   | |                       |             |  |  data between BIOS and BMC    |        | configuration attributes.    |                      ||
   | |                       |             |  |                               |        | -BIOSCapability              |                      ||
   | |                       |             |  |                               |        | -FactoryDefaultSetting       |                      ||
   | |                       |             |  |                               |        | -BIOSPwdHashData             |                      ||
   | |                       |             |  |                               |        | -inBandIntf(IPMI)            |                      ||
   + |                       |             |  +-------------------------------+        +------------------------------+                      ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+                      ||
   | | Set BIOS capability   |<---Req-/Res--> | Forward to the RBC daemon     |--dbus-->| Set the BIOSCapability.     |                      ||
   | |                       |             |  |                               |         |  &inBandIntf                |                      ||
   | |Set BIOS Pwd hash/Seed |<---Req-/Res--> | Forward to the RBC daemon     |--dbus-->| Set BIOSPwdHash data.       |                      ||
   | |                       |             |  |                               |         | Set the IsBIOSInitDone.     |                      ||
   | |                       |             |  |                               |         |                             |                      ||
   | |Check factory settings |<---Req-/Res--> |  Get the FactoryDefault prop  |<-dbus-- | Send FactoryDefaultSettings |                      ||
   | |Init the BIOS config   |             |  |                               |         |                             |    +---------------+ ||
   | |Based on value.        |             |  |                               |         |                             |    |Payload 0      | ||
   | |Get the XML Type0 info |<---Req-/Res--> | Get the data from RBC daemon  |<-dbus-- | Payload 0 is not present.   |    | Object        | ||
   | |Generate & compress    |             |  |                               |         |                             |    |-FilePath      | ||
   | |XML type 0 file        |             |  |                               |         |Validate the payload 0.      |--->|-FileSize      | ||
   | |                       |             |  |                               |--dbus-->|Create the Payload 0 object& |    |-Status        | ||
   | | Send the payload file |<---Req-/Res--->| Forward to the RBC daemon     |         |Del the Payload 1 if exist.  |    |-CreationTime  | ||
   | | via SetPayload command|             |  |                               |         |Use the Payload 0 and create |    |-Chksum        | ||
   | |                       |             |  |                               |         | Attribute Registry format.  |    +---------------+ ||
   | | Continue the BIOS boot|             |  |                               |         |                             |           |          ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+           |          ||
   |                                       +-------------------------------------------------------------------------------------------------+|
   +---------------------------------------+--------------------------------------------------------------------------------------------------+
```
```
#BIOS reset
   +------------------------------------------------------------------------------------------------------------------------------------------+
   |                                                                                                                                          |
   | +-----------------------+             +-------------------------------------------------------------------------------------------------+|
   | | BIOS                  |             |   BMC                                                                                           ||
   | |                       |             |  +-------------------------------+        +------------------------------+                      ||
   | |                       |             |  |IPMI Interface (kcs)           |        |RBC daemon Manager            |                      ||
   | |                       |             |  | -Responsible for send /recv   |        |-Responsible for handling BIOS|                      ||
   | |                       |             |  |  data between BIOS and BMC    |        | configuration attributes.    |                      ||
   | |                       |             |  |                               |        | -BIOSCapability              |                      ||
   | |                       |             |  |                               |        | -FactoryDefaultSetting       |                      ||
   | |                       |             |  |                               |        | -BIOSPwdHashData             |  +-----------------+ ||
   | |                       |             |  |                               |        | -MaxPayloadSupported =15     |  | Payload 0       | ||
   | |                       |             |  |                               |        | -inBandIntf(IPMI)            |  | Object          | ||
   + |                       |             |  +-------------------------------+        +------------------------------+  | -FilePath       | ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+  | -FileSize       | ||
   | |Set BIOS capability    |<---Req-/Res--> | Forward to the RBC daemon     |--dbus-->| Set the BIOSCapability      |  | -Status         | ||
   | |                       |             |  |                               |         |  &inBandIntf                |  | -CreationTime   | ||
   | |Set BIOS Pwd hash/Seed |<---Req-/Res--> | Forward to the RBC daemon     |--dbus-->| Set BIOSPwdHash data        |  | -Chksum         | ||
   | |                       |             |  |                               |         | Set the IsBIOSInitDone      |  |                 | ||
   | |                       |             |  |                               |         |                             |  +-----------------+ ||
   | |Check factory settings |<---Req-/Res--> | Get the FactoryDefault prop   |<-dbus-- | Send FactoryDefaultSettings |                      ||
   | |Init the BIOS config   |             |  |                               |         |                             |    +---------------+ ||
   | |Based on value.        |             |  |                               |         |                             |    |Payload 1      | ||
   | |Get the XML Type0 info |<---Req-/Res--> | Get the data from RBC daemon  |<-dbus-- | Payload 0 is present.       |    | Object        | ||
   | |Generate & compress    |             |  |                               |         |                             |    |-FilePath      | ||
   | |XML type 0 file.       |             |  |                               |         |                             |--->|-FileSize      | ||
   | |Check Payload0 Chksum. |             |  |                               |--dbus-->|Create the Payload 0 object  |    |-Status        | ||
   | |If Chksum mismatch     |<---Req-/Res--->| Forward new data to the RBC   |         |Delete the Payload 1.        |    |-CreationTime  | ||
   | |then send the payload  |             |  | daemon                        |         | Use Payload 0 and create    |    |-Chksum        | ||
   | |via SetPayload.        |             |  |                               |         | Attribute registry format   |    +---------------+ ||
   | |Continue the BIOS boot |             |  |                               |         |                             |           |          ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+           |          ||
   |                                       +-------------------------------------------------------------------------------------------------+|
   +---------------------------------------+--------------------------------------------------------------------------------------------------+
```
```
#BIOS reset and BMC have new values
   +------------------------------------------------------------------------------------------------------------------------------------------+
   |                                                                                                                                          |
   | +-----------------------+             +-------------------------------------------------------------------------------------------------+|
   | | BIOS                  |             |   BMC                                                                                           ||
   | |                       |             |  +-------------------------------+        +------------------------------+                      ||
   | |                       |             |  |                               |        |                              |                      ||
   | |                       |             |  |IPMI Interface (kcs)           |        |RBC daemon Manager            |                      ||
   | |                       |             |  | -Responsible for send /recv   |        |-Responsible for handling BIOS|                      ||
   | |                       |             |  |  data between BIOS and BMC    |        | configuration attributes.    |                      ||
   | |                       |             |  |                               |        | -BIOSCapability              |                      ||
   | |                       |             |  |                               |        | -FactoryDefaultSetting       |                      ||
   | |                       |             |  |                               |        | -BIOSPwdHashData             |  +-----------------+ ||
   | |                       |             |  |                               |        | -MaxPayloadSupported =15     |  | Payload 0       | ||
   | |                       |             |  |                               |        | -inBandIntf(IPMI)            |  | Object          | ||
   + |                       |             |  +-------------------------------+        +------------------------------+  | -FilePath       | ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+  | -FileSize       | ||
   | |Set BIOS capability    |<---Req-/Res--> | Forward to the RBC daemon     |---dbus->| Set the BIOSCapability      |  | -Status         | ||
   | |                       |             |  |                               |         |  &inBandIntf                |  | -CreationTime   | ||
   | |Set BIOS Pwd hash/Seed |<---Req-/Res--> | Forward to the RBC daemon     |---dbus->| Set BIOSPwdHash data        |  | -Chksum         | ||
   | |                       |             |  |                               |         | Set the IsBIOSInitDone      |  |                 | ||
   | |                       |             |  |                               |         |                             |  +-----------------+ ||
   | |Check factory settings |<---Req-/Res--> |  Get the FactoryDefault prop  |<--dbus--| Send FactoryDefaultSettings |                      ||
   | |Init the BIOS config   |             |  |                               |         |                             |    +---------------+ ||
   | |Based on value.        |             |  |                               |         |                             |    |Payload 1      | ||
   | |Get the XML Type0 info |<---Req-/Res--> | Get the data from RBC daemon  |<--dbus--| Payload 0 is present.       |    | Object        | ||
   | |Generate & compress    |             |  |                               |         |                             |    |-FilePath      | ||
   | |XML type 0 file.       |             |  |                               |         |                             |--->|-FileSize      | ||
   | |Check Payload0 Chksum. |             |  |                               |         |                             |    |-Status        | ||
   | |If Chksum   match      |<---Req-/Res--->| Forward new data to the RBC   |<--dbus--|Send the Payload 1 info      |    |-CreationTime  | ||
   | |then get the payload 1 |             |  | daemon                        |         | which is created by user    |    |-Chksum        | ||
   | |via GetPayload.        |             |  |                               |         | ( Contain new values)       |    +---------------+ ||
   | |Get  the XML type 1    |             |  |                               |         |                             |           |          ||
   | |via GetPayload command.|<---Req-/Res--->| Get the data from RBC daemon  |<--dbus--| Payload 1 is present        |           |          ||
   | |                       |             |  |                               |         |                             |           |          ||
   | |Update the new value   |             |  |                               |         |                             |           |          ||
   | |and new chksum in      |             |  |                               |         |                             |           |          ||
   | |BIOS.                  |             |  |                               |         |                             |           |          ||
   | |Reset the system.      |             |  |                               |         |                             |                      ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+                      ||
   |                                       +-------------------------------------------------------------------------------------------------+|
   +---------------------------------------+--------------------------------------------------------------------------------------------------+

```

##BIOS send the data in BIOS configuration PLDM via MCTP

Following Payload types are supported in PLDM via MCTP:
Payload Type 2- Stringified JSON format contain all attributes related details.
This information can be used for set/get attribute Dbus call to modify
the particular attribute value.
Payload Type 3- BIOS string table
Payload Type 4- BIOS Attribute name table
Payload Type 5- BIOS Attribute value table
Payload type 6- BIOS Attribute Pending value table

BIOS should update the BIOS settings via Set BIOS table PLDM command-
BIOS string table, Attribute name table, Attribute value table via MCTP.
PLDM daemon should set the RBC support capability/ PLDM capability
RBC capability
ex:
 update model-immediate or deferred model
PLDM capability
ex:
PLDM send all bios tables & RBC can generate Payload 2 or PLDM send Payload 2.
Payload 2 is stringfied JSON format contain all attribute related details.

PLDM daemon should send all BIOS tables or payload type 2 to the RBC daemon.
PLDM daemon should register for attribute changed signal and RBC daemon should
generate the signal with changed attribute name whenever attribute value changed by
set attribute Dbus call & it can be captured  by PLDM and read the new value by
get attribute Dbus call or Wait for Payload Type 6 created signal and pull
the Pending values tables from RBC daemon.PLDM should delete the Payload Type 6
once its not valid or updated in BIOS.
SetPayload Dbus call-Payload Type,UserAbort Status used to delete that payload.

RBC daemon should generate the Attribute Registry format based on received
tables or should send payload type 2(Attribute Registry-stringified JSON).
RBC daemon should create pending BIOS configuration value table based on the
user input configuration. PLDM daemon should retrieve the pending data
(changed value) from RBC daemon and send to the BIOS via PLDM command via MCTP.

If BIOS provide the BIOS configuration PLDM via MCTP interface, then PLDM daemon
should set the RBC BIOS capability & PLDM capability.
Collected data in Attribute Registry format or tables to the RBC daemon.

BIOS must collect the new attribute values from BMC and update the same in BIOS.

#BIOS first boot
   +------------------------------------------------------------------------------------------------------------------------------------------+
   |                                                                                                                                          |
   | +-----------------------+             +-------------------------------------------------------------------------------------------------+|
   | | BIOS                  |             |   BMC                                                                                           ||
   | |                       |             |  +-------------------------------+         +-----------------------------+                      ||
   | |                       |             |  |PLDM Interface (MCTP)          |         |RBC Daemon Manager           |                      ||
   | |                       |             |  | -Responsible for send /recv   |         |-Responsible for handle BIOS |                      ||
   | |                       |             |  |  data between BIOS and BMC    |         | configuration attributes.   |                      ||
   | |                       |             |  |                               |         | -RBCCapability              |                      ||
   | |                       |             |  |                               |         | -inBandIntf(PLDM)           |                      ||
   + +-----------------------+             |  +-------------------------------+         +-----------------------------+                      ||
   | |Get the Payload info   |<---Req-/Res--> | Get the data from RBC Daemon  |<-dbus---|Payload 3-4-5 is not present |                      ||
   | |or Check Payload Tag   |             |  |                               |         |                             |                      ||
   | |using GetBIOS table    |             |  |                               |         |                             |                      ||
   | |                       |             |  |                               |         |                             |                      ||
   | |Set BIOS string table  |<---Req-/Res--> | Forward to the RBC Daemon     |--dbus-->| Create the Payload 3 Object |   +---------------+  ||
   | | via Set BIOS table    |             |  |                               |         | &inBandIntf /BIOSCapability |   |Payload 3-4-5  |  ||
   | |SetBIOSAttribute table |<---Req-/Res--> | Forward to the RBC Daemon     |--dbus-->| Create the Payload 4 Object |   | Objects       |  ||
   | |SetBIOSAttributeValue  |<---Req-/Res--> | Forward to the RBC Daemon     |--dbus-->| Create the Payload 5 Object |   |-Chksum        |  ||
   | |table via SetBIOStable |             |  |                               |         |                             |   |-FilePath      |  ||
   | |table via SetBIOStable |             |  |                               |         |                             |   |-FilePath      |  ||
   | |Init the BIOS config   |             |  |                               |         |                             |   |-FileSize      |  ||
   | |Based on value.        |             |  |                               |         |                             |   |-Status        |  ||
   | |via Set BIOS table     |             |  |                               |         |                             |   |-CreationTime  |  ||
   | |                       |             |  |                               |         |Use Payload 3-4-5 and create |   +---------------+  ||
   | | Continue the BIOS boot|             |  |                               |         | the AttributeRegistry format|           |          ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+           |          ||
   |                                       +-------------------------------------------------------------------------------------------------+|
   +---------------------------------------+--------------------------------------------------------------------------------------------------+


#BIOS reset
   +------------------------------------------------------------------------------------------------------------------------------------------+
   |                                                                                                                                          |
   | +-----------------------+             +-------------------------------------------------------------------------------------------------+|
   | | BIOS                  |             |   BMC                                                                                           ||
   | |                       |             |  +-------------------------------+         +-----------------------------+                      ||
   | |                       |             |  |PLDM Interface (MCTP)          |         |RBC Daemon Manager           |                      ||
   | |                       |             |  | -Responsible for send /recv   |         |-Responsible for handle BIOS |                      ||
   | |                       |             |  |  data between BIOS and BMC    |         | configuration attributes.   |                      ||
   | |                       |             |  |                               |         | -BIOSCapability             |                      ||
   | |                       |             |  |                               |         | -inBandIntf(PLDM)           |                      ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+                      ||
   | |Get the Payload info   |<---Req-/Res--> | Get the data from RBC Daemon  |<-dbus---|Payload 3-4-5 is present     |                      ||
   | |or Check Payload Tag   |             |  |                               |         |                             |                      ||
   | |using GetBIOS table    |             |  |                               |         |                             |                      ||
   | |                       |             |  |                               |         |                             |                      ||
   | | Get the Payload 6     |<---Req-/Res--> | Get the data from RBC Daemon  |<-dbus---|Payload 6-Pending value table|                      ||
   | | using Get BIOS table  |             |  |                               |         |Object created by user       |   +---------------+  ||
   | |                       |             |  |                               |         |                             |   |Payload 3-4-5  |  ||
   | |SetBIOSAttributeValue  |<---Req-/Res--> | Forward to the RBC Daemon     |--dbus-->| Create the Payload 5 Object |   |-Chksum        |  ||
   | |table via SetBIOStable.|             |  |                               |         | Del the Pending value table |   |-FilePath      |  ||
   | |Init the BIOS config   |             |  |                               |         | & Payload 6                 |   |-FileSize      |  ||
   | |Based on value.        |             |  |                               |         |                             |   |-Status        |  ||
   | |                       |             |  |                               |         |                             |   |-CreationTime  |  ||
   | |                       |             |  |                               |         |Use Payload 3-4-5 and create |   +---------------+  ||
   | | Reset the BIOS boot   |             |  |                               |         | the AttributeRegistry format|           |          ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+           |          ||
   |                                       +-------------------------------------------------------------------------------------------------+|
   +---------------------------------------+--------------------------------------------------------------------------------------------------+

##Complete BIOS BMC flow for BIOS configuration in deferred update model
```
+----------------------------------------+                    +----------------------------------------+
|                BIOS                    |                    |                  BMC                   |
|                                        |                    |                                        |
|  +----------------------------------+  |                    | +------------------------------------+ |
|  | Send the BIOS capability  Support|  |--------KCS-------->| |1.Get the complete atttribute data. | |
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
|  | If new attribute value exist     |<-|-----------------------|  Send the new value Attribute data| |
|  |           then                   |  |                    |  |                                   | |
|  | Get & Update the BIOS variables  | -| -----+             |  |                                   | |
|  |                                  |  |      |             |  +-----------------------------------+ |
|  +---------------+------------------+  |      |             |                                        |
|                  |                     |      |             |                                        |
|                  YES                   |      |             |                                        |
|                  |                     |      |             |  +----------------------------------+  |
|   +--------------V------------------+  |      |             |  |                                  |  |
|   |  Send the updated data to BMC   |  |      |             |  | Update the BIOS Attribute data   |  |
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

##Complete BIOS BMC flow for BIOS configuration in immediate update model
```
+----------------------------------------+                    +----------------------------------------+
|                BIOS                    |                    |                  BMC                   |
|                                        |                    |                                        |
|  +----------------------------------+  |                    | +------------------------------------+ |
|  | Send the BIOS capability  Support|  |--------KCS-------->| |1.Get the complete atttribute data. | |
|  | Send the compressed BIOS file(or)|  |-MCTP/KCS/Redfish-->| |2.Validate and convert into         | |
|  | Send PLDM data via MCTP  (or)    |  |                    | |  BIOS Attribute Registry format.   | |
|  | Send the Redfish host interface  |  |                    | |3.Expose the Dbus interface         | |
|  +----------------------------------+  |                    | +------------------------------------+ |
|                                        |                    |                                        |
|                                        |                    | +------------------------------------+ |
|                                        |                    | |PLDM can send complete all tables   | |
|                                        |                    | | or send the attribute registry     | |
|                                        |                    | | (Payload 2) via SetPayload Method  | |
|                                        |                    | | or UpdatePayloadInfo Method in dBus| |
|                                        |                    | +------------------------------------+ |
|                                        |                    | +------------------------------------+ |
|                                        |                    | | If attribute  value changed then   | |
|                                        |                    | | Biosconfig send signal to PLDM     | |
|                                        |                    | | PLDM read the Pending value table  | |
|                                        |                    | | PLDM send the message to the host  | |
|                                        |                    | | PLDM send the data to the host     | |
|                                        |                    | +------------------------------------+ |
|  +----------------------------------+  |                    | +------------------------------------+ |
|  | Read the new Atribute value      |<-|----------------------| Send the new Pending value table   | |
|  |           and                    |  |                    | |                                    | |
|  | Update in the BIOS structure     |  |                    | +------------------------------------+ |
|  +---------------+------------------+  |                    |                                        |
|                  |                     |                    |                                        |
|                  YES                   |                    |                                        |
|                  |                     |                    | +-----------------------------------+  |
|   +--------------V------------------+  |                    | | Update the BIOS Atrribute data.   |  |
|   |  Send the updated data to BMC   |  |                    | | Discard the Pending value table   |  |
|   |                                 |------------------------>| (Payload type 6)                  |  |
|   +---------------------------------+  |                    | +-----------------------------------+  |
|                                        |                    |                                        |
+----------------------------------------+                    +----------------------------------------+
```
#Redfish interfaces for remote Bios configuration

1. Get Current Attributes name and value list:
   Get the current BIOS settings attribute name and value pair list.
   GET Method - "https://<BMC IP address>/redfish/v1/Systems/system/Bios"

2. Get Attribute Registry:
   Get the detailed information about Bios Attribute like current value,
   supported value, description, Menupath, Default value.
   GET Method - "https://<BMC IP address>/redfish/v1/Registries/Bios"

3. Change BIOS password:
   ACTION - "https://<BMC IP address>/redfish/v1/Systems/system/Bios/Actions/Bios.ChangePassword"

4. Reset the BIOS settings:
   ACTION - "https://<BMC IP address>/redfish/v1/Systems/system/Bios/Actions/Bios.ResetBios"

5. Update new BIOS settings:
   PATCH Method - "https://<BMC IP address>/redfish/v1/Systems/system/Bios/Settings"

6. Get the new pending value list:
   GET Method - "https://<BMC IP address>/redfish/v1/Systems/system/Bios/Settings"
   -Valid only in deferred model

```
   +---------------------------------------------------------------------------------------------------------+
   | +-----------------------+             +----------------------------------------------------------------+|
   | | RBCWeb tool -POSTMAN  |             |   BMC                                                          ||
   | |                       |             |  +-----------------------+       +---------------------------+ ||
   | |                       |             |  |Redfish Daemon         |       |RBC Daemon Manager         | ||
   | |                       |             |  |-Responsible for handle|       |-Parse Bios Data,convert to| ||
   | |                       |             |  |all Redfish request    |       | required format & return  | ||
   | |                       |             |  +-----------------------+       +---------------------------+ ||
   | +-----------------------+             |  +-----------------------+       +---------------------------+ ||
   | |Get Current Attributes |<---Req-/Res--> | Call RBC dbus Method  |<-dbus-| GetAllAttributes()        | ||
   | | name & value list     |             |  |                       |       |      -Attributeslist      | ||
   | |Get Attribute Registry |<---Req-/Res--> | Call RBC dbus Method  |<-dbus-| GetAllAttributes()        | ||
   | |                       |             |  |                       |       |      -AttributesRegistry  | ||
   | |Change BIOS Password   |<---Req-/Res--> | Call RBC dbus Method  |-dbus->| ChangePassword()          | ||
   | |                       |             |  |                       |       |                           | ||
   | |To default settings    |<---Req-/Res--> | Call RBC dbus Method  |-dbus->| ResetBiosSettings()       | ||
   | |                       |             |  |                       |       |     -ResetFlag            | ||
   | |Update BIOS settings   |<---Req-/Res--->| Call RBC dbus Method  |-dbus->| SetAttribute()            | ||
   | |                       |             |  |                       |       |                           | ||
   | |Get attribute by name  |<---Req-/Res--->| Call RBC dbus Method  |<-dbus-| GetAttribute()            | ||
   | |                       |             |  |                       |       |                           | ||
   | |Set attribute by name  |<---Req-/Res--->| Call RBC dbus Method  |-dbus->| SetAttribute()            | ||
   | |                       |             |  |                       |       |                           | ||
   | |Get Pending Attributes |<---Req-/Res--->| Call RBC dbus Method  |<-dbus-| GetAllAttributes()        | ||
   | |         list          |             |  |                       |       |    -PendingAttributeslist | ||
   | |                       |             |  |                       |       |                           | ||
   | |                       |             |  |                       |       | SetPayload()              | ||
   | |                       |             |  |                       |       |  -PayloadTransferState    | ||
   | |                       |             |  |                       |       |    :StartTransfer         | ||
   | |                       |             |  |                       |       |    :InProgress            | ||
   | |                       |             |  |                       |       |    :EndTransfer           | ||
   | |                       |             |  |                       |       |    :UserAbort             | ||
   | |                       |             |  |                       |       |  -PayloadType             | ||
   | |                       |             |  |                       |       |  -PayloadData             | ||
   | |                       |             |  |                       |       |                           | ||
   | |                       |             |  |                       |       |  UpdatePayloadInfo()      | ||
   | |                       |             |  |                       |       |   -PayloadType            | ||
   | |                       |             |  |                       |       |   -FilePath               | ||
   | |                       |             |  |                       |       |   -FileVersion(optional)  | ||
   | |                       |             |  |                       |       |   -FileSize               | ||
   | |                       |             |  |                       |       |   -FileChecksum           | ||
   | |                       |             |  |                       |       |                           | ||
   | +-----------------------+             |  +-----------------------+       +---------------------------+ ||
   |                                       +---------------------------------------------------------------+||
   +---------------------------------------+-----------------------------------------------------------------+
```

## Alternatives Considered
Syncing the BIOS configuration via Redfish Host interface.
Redfish Host specification definition is not completed and ready
BIOS support also not available.
There are 1000+ bios variables and storing in phosphor-settingsd is not optimal.

we prefered to do data validation and data processing in RBC daemon itself instead
of doing in each interfaces separately.
RBC daemon should process the BIOS configuration data and IPMI should act as interface
only.

## Impacts
BIOS must support and follow RBC BIOS configuration flow.

## Testing
Able to change the BIOS configuration via BMC through LAN
Able to change the BIOS setup password via BMC
Compliance with Redfish will be tested using the Redfish Service Validator
