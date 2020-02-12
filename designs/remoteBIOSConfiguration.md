# Remote BIOS Configuration (RBC) via BMC

Author:
  Suryakanth Sekar! ssekar

Primary assignee:
  Suryakanth Sekar! ssekar

Other contributors:
  Jia Chunhui
  Deepak Kodihalli
  Patrick Williams

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
4. In deferred model, When the system is in S0, S3, S4 or S5 state.
   BMC will send the updated BIOS variables on next BIOS boot only.
   It will not initiate a BIOS boot immediately.
5. In immediate model, BMC should send message to the system firmware (BIOS)
   Whenever settings are changed.
6. BMC should support bios/ attribute registry in redfish schema for
   BIOS configuration.
7. BMC should provide secure way for updating BIOS setup password settings.
   Detailed password handling design  -- TBD
Ex: Updating the BIOS password should be support only before end of post.


## Proposed Design

```
+----------------------------------------------------------------------------------------------------------------+
| Remote BIOS configuration (RBC) via BMC                                                                        |
|                                                                                                                |
|                                                                                                                |
| +-------------+       +-------------+       +--------------------------------+      +-------+                  |
| |             |       |             |       |   RBC daemon                   |      |       |    +----------+  |
| | NET/ Tools  +<-LAN->+ LAN-IPMID/  +<Dbus->+                                |      |       |    |Web client|  |
| |             |       | REDFISH     |       |  Provide following Methods     |      |       |    |          |  |
| +-------------+       +-------------+       |     -GetAllBaseAttributes()    |      |       |    +----^-----+  |
|                                             |     -SetAllBaseAttributes()    |      |       |         |        |
| +-------------+       +-------------+       |     -SetAttribute()            |      |       |        LAN       |
| |             |       |             |       |     -GetAttribute()            |      |       |         |        |
| | HOST/ BIOS  +<-KCS->+  HOST-IPMID +<Dbus->+     -SetPendingAttributes()    |      |Redfish|    +----V-----+  |
| |             |       |             |       |     -GetPendingAttributes()    +<Dbus>+  API  |    |Redfish & |  |
| +-----+-------+       +-------------+       |                                |      |       +<-->+BMCWeb    |  |
|       |                                     |                                |      |       |    +----^-----+  |
|       |                                     |                                |      |       |         |        |
|       |                                     |                                |      |       |         |        |
|       |                                     |                                |      |       |    +----V-----+  |
|       |                                     |                                |      |       |    | Redfish  |  |
|       |                                     |                                |      |       |    |  Host    |  |
|       |                                     |                                |      |       |    | Interface|  |
|       |                                     +----^-----------------+---------+      +-------+    +----------+  |
|       |                                          |       AttributeChanged Signal                               |
|       |                                          |                 |                                           |
|       |                                          |                 |                                           |
|       |                                     +----V-----------------V---------+                                 |
|       |                                     | PLDM daemon                    |                                 |
|       |                                     | Collect the BIOS data & convert|                                 |
|       +---------MCTP----------------------->| into native D-bus format and   |                                 |
|                                             | send to the RBC                |                                 |
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

IPMI interface should decompress & validate the received XML Type 0.
IPMI interface should validate the XML type 0.
IPMI interface should convert the XML Type 0 into native to D-bus format
and send to the RBC daemon.
RBC daemon should support below D-bus Method interface for redfish support.
GetAllBaseAttributes() --> To get the all base attributes details.
SetAllBaseAttributes() --> To set the all base atribute details.
SetAttribute() --> To set the single attribute new value.
GetAttribute() --> To get the single attribute value.
GetPendingAttributes() -->  To get the pending attributes list.
SetPendingAttributes() --> To set the pending atributes list.

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
   +---------------------------------------------------------------------------------------------------------------------+
   |                                                                                                                     |
   | +-----------------------+             +----------------------------------------------------------------------------+|
   | | BIOS                  |             |   BMC                                                                      ||
   | |                       |             |  +-------------------------------+        +------------------------------+ ||
   | |                       |             |  |IPMI Interface (kcs)           |        |RBC daemon Manager            | ||
   | |                       |             |  | -Responsible for send /recv   |        |-Responsible for handling BIOS| ||
   | |                       |             |  |  data between BIOS and BMC    |        | configuration attributes.    | ||
   | |                       |             |  |                               |        | -BIOSCapability              | ||
   | |                       |             |  |                               |        | -FactoryDefaultSetting       | ||
   | |                       |             |  |                               |        | -BIOSPwdHashData             | ||
   | |                       |             |  |                               |        | -inBandIntf(IPMI)            | ||
   + |                       |             |  +-------------------------------+        +------------------------------+ ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+ ||
   | | Set BIOS capability   |<---Req-/Res--> | Forward to the RBC daemon     |--dbus-->| Set the BIOSCapability.     | ||
   | |                       |             |  |                               |         |  &inBandIntf                | ||
   | |Set BIOS Pwd hash/Seed |<---Req-/Res--> | Forward to the RBC daemon     |--dbus-->| Set BIOSPwdHash data.       | ||
   | |                       |             |  |                               |         | Set the IsBIOSInitDone.     | ||
   | |                       |             |  |                               |         |                             | ||
   | |Check factory settings |<---Req-/Res--> |  Get the FactoryDefault prop  |<-dbus-- | Send FactoryDefaultSettings | ||
   | |Init the BIOS config   |             |  |                               |         |                             | ||
   | |Based on value.        |             |  |                               |         |                             | ||
   | |Get the XML Type0 info |<---Req-/Res--> | Provide the Payload 0 Info    |         |                             | ||
   | |Generate & compress    |             |  |                               |         |                             | ||
   | |XML type 0 file        |             |  | Validate the Payload 0        |         |Collect the Attributes Info  | ||
   | |                       |             |  | Unzip the Payload 0           |--dbus-->|                             | ||
   | | Send the payload file |<---Req-/Res--->| Convert into D-bus format     |                                       | ||
   | | via SetPayload command|             |  | and send to the RBC.          |         |                             | ||
   | |                       |             |  |                               |         |                             | ||
   | | Continue the BIOS boot|             |  |                               |         |                             | ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+ ||
   |                                       +----------------------------------------------------------------------------+|
   +---------------------------------------+-----------------------------------------------------------------------------+
```
```
#BIOS reset
   +---------------------------------------------------------------------------------------------------------------------+
   |                                                                                                                     |
   | +-----------------------+             +----------------------------------------------------------------------------+|
   | | BIOS                  |             |   BMC                                                                      ||
   | |                       |             |  +-------------------------------+        +------------------------------+ ||
   | |                       |             |  |IPMI Interface (kcs)           |        |RBC daemon Manager            | ||
   | |                       |             |  | -Responsible for send /recv   |        |-Responsible for handling BIOS| ||
   | |                       |             |  |  data between BIOS and BMC    |        | configuration attributes.    | ||
   | |                       |             |  |                               |        | -BIOSCapability              | ||
   | |                       |             |  |                               |        | -FactoryDefaultSetting       | ||
   | |                       |             |  |                               |        | -BIOSPwdHashData             | ||
   | |                       |             |  |                               |        | -inBandIntf(IPMI)            | ||
   + |                       |             |  +-------------------------------+        +------------------------------+ ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+ ||
   | |Set BIOS capability    |<---Req-/Res--> | Forward to the RBC daemon     |--dbus-->| Set the BIOSCapability      | ||
   | |                       |             |  |                               |         |  &inBandIntf                | ||
   | |Set BIOS Pwd hash/Seed |<---Req-/Res--> | Forward to the RBC daemon     |--dbus-->| Set BIOSPwdHash data        | ||
   | |                       |             |  |                               |         | Set the IsBIOSInitDone      | ||
   | |                       |             |  |                               |         |                             | ||
   | |Check factory settings |<---Req-/Res--> | Get the FactoryDefault prop   |<-dbus-- | Send FactoryDefaultSettings | ||
   | |Init the BIOS config   |             |  |                               |         |                             | ||
   | |Based on value.        |             |  |                               |         |                             | ||
   | |Get the XML Type0 info |<---Req-/Res--> | Provide the Payload 0 Info    |         |                             | ||
   | |Generate & compress    |             |  |                               |         |                             | ||
   | |XML type 0 file.       |             |  |                               |         |Collect the Attributes Info  | ||
   | |Check Payload0 Chksum. |             |  | Validate the Payload 0        |--dbus-->|                             | ||
   | |If Chksum mismatch     |<---Req-/Res--->| Unzip Payload 0 and convert   |         |                             | ||
   | |then send the payload  |             |  | into D-bus format and send to |         |                             | ||
   | |via SetPayload.        |             |  | the RBC                       |         |                             | ||
   | |Continue the BIOS boot |             |  |                               |         |                             | ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+ ||
   |                                       +----------------------------------------------------------------------------+|
   +---------------------------------------+-----------------------------------------------------------------------------+
```
```
#BIOS reset and BMC have new values
   +---------------------------------------------------------------------------------------------------------------------+
   |                                                                                                                     |
   | +-----------------------+             +----------------------------------------------------------------------------+|
   | | BIOS                  |             |   BMC                                                                      ||
   | |                       |             |  +-------------------------------+        +------------------------------+ ||
   | |                       |             |  |                               |        |                              | ||
   | |                       |             |  |IPMI Interface (kcs)           |        |RBC daemon Manager            | ||
   | |                       |             |  | -Responsible for send /recv   |        |-Responsible for handling BIOS| ||
   | |                       |             |  |  data between BIOS and BMC    |        | configuration attributes.    | ||
   | |                       |             |  |                               |        | -BIOSCapability              | ||
   | |                       |             |  |                               |        | -FactoryDefaultSetting       | ||
   | |                       |             |  |                               |        | -BIOSPwdHashData             | ||
   | |                       |             |  |                               |        | -inBandIntf(IPMI)            | ||
   + |                       |             |  +-------------------------------+        +------------------------------+ ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+ ||
   | |Set BIOS capability    |<---Req-/Res--> | Forward to the RBC daemon     |---dbus->| Set the BIOSCapability      | ||
   | |                       |             |  |                               |         |  &inBandIntf                | ||
   | |Set BIOS Pwd hash/Seed |<---Req-/Res--> | Forward to the RBC daemon     |---dbus->| Set BIOSPwdHash data        | ||
   | |                       |             |  |                               |         | Set the IsBIOSInitDone      | ||
   | |                       |             |  |                               |         |                             | ||
   | |Check factory settings |<---Req-/Res--> | Get the FactoryDefault prop   |<--dbus--| Send FactoryDefaultSettings | ||
   | |Init the BIOS config   |             |  |                               |         |                             | ||
   | |Based on value.        |             |  |                               |         |                             | ||
   | |Get the XML Type0 info |<---Req-/Res--> | Provide the Payload 0 Info    |         |                             | ||
   | |Generate & compress    |             |  |                               |         |                             | ||
   | |XML type 0 file.       |             |  |                               |         |                             | ||
   | |Check Payload0 Chksum. |             |  |                               |         |                             | ||
   | |If Chksum   match      |<---Req-/Res--->| Provide the Payload 0         |         |                             | ||
   | |then get the payload 1 |             |  |                               |         |                             | ||
   | |via GetPayload.        |             |  |                               |         |                             | ||
   | |Get  the XML type 1    |             |  |                               |         |                             | ||
   | |via GetPayload command.|<---Req-/Res--->| Provide the Payload 1         |         |                             | ||
   | |                       |             |  |                               |         |                             | ||
   | |Update the new value   |             |  |                               |         |                             | ||
   | |and new chksum in      |<---Req-/Res--->| Get the Payload 0 and convert |--dbus-->| Collect the Atributes info  | ||
   | |BIOS.                  |             |  | into native to D-bus format   |         |                             | ||
   | |Reset the system.      |             |  |                               |         |                             | ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+ ||
   |                                       +----------------------------------------------------------------------------+|
   +---------------------------------------+-----------------------------------------------------------------------------+

```

##BIOS send the data in BIOS configuration PLDM via MCTP

BIOS should update the BIOS settings via Set BIOS table PLDM command-
BIOS string table, Attribute name table, Attribute value table via MCTP.
PLDM daemon should set the RBC support capability/ PLDM capability
RBC capability
ex:
 update model-immediate or deferred model
PLDM capability
ex:
PLDM should generate the attributes data in native to D-bus format from
all BIOS tables and send to the RBC daemon by SetAllBaseAttributes Method.

PLDM daemon should register for attribute changed signal and RBC daemon should
generate the signal with changed attribute name whenever attribute value changed by
set attribute D-bus call & it can be captured by PLDM and read the new value by
get attribute D-bus call or Wait for Pending attributes value signal and pull
the Pending attributes value from RBC daemon. PLDM should delete the Pending
attributes value table once its not valid or updated in BIOS.

RBC daemon should create pending BIOS configuration value table based on the
user input configuration. PLDM daemon should retrieve the pending data
(changed value) from RBC daemon and send to the BIOS via PLDM command via MCTP.

If BIOS provide the BIOS configuration PLDM via MCTP interface, then PLDM daemon
should set the RBC BIOS capability & PLDM capability.
Collected data in native to D-bus format to the RBC daemon.

BIOS must collect the new attribute values from BMC and update the same in BIOS.

#BIOS first boot
   +--------------------------------------------------------------------------------------------------------------------+
   | +-----------------------+             +---------------------------------------------------------------------------+|
   | | BIOS                  |             |   BMC                                                                     ||
   | |                       |             |  +-------------------------------+         +-----------------------------+||
   | |                       |             |  |PLDM Interface (MCTP)          |         |RBC Daemon Manager           |||
   | |                       |             |  | -Responsible for send /recv   |         |-Responsible for handle BIOS |||
   | |                       |             |  |  data between BIOS and BMC    |         | configuration attributes.   |||
   | |                       |             |  |                               |         | -RBCCapability              |||
   | |                       |             |  |                               |         | -inBandIntf(PLDM)           |||
   + +-----------------------+             |  +-------------------------------+         +-----------------------------+||
   | |Get the table info     |<---Req-/Res--> | Provide the table information |         |                             |||
   | | & Check table Tag     |             |  |                               |         |                             |||
   | |using GetBIOS table    |             |  |                               |         |                             |||
   | |                       |             |  |                               |         |                             |||
   | |Set BIOS string table  |<---Req-/Res--> | Get the string table          |         |                             |||
   | | via Set BIOS table    |             |  |                               |         |                             |||
   | |SetBIOSAttribute table |<---Req-/Res--> | Get the attributes table      |         |                             |||
   | |SetBIOSAttributeValue  |<---Req-/Res--> | Get the attributes value table|         |                             |||
   | |table via SetBIOStable |             |  |                               |         |                             |||
   | |table via SetBIOStable |             |  |                               |         |                             |||
   | |Init the BIOS config   |             |  | Send the attributes data in   |         |                             |||
   | |Based on value.        |             |  | native to D-bus format        |--dbus-->| Attributes data             |||
   | |via Set BIOS table     |             |  |  by SetAllBaseAttributes()    |         |                             |||
   | |                       |             |  |                               |         |                             |||
   | | Continue the BIOS boot|             |  |                               |         |                             |||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+||
   |                                       +---------------------------------------------------------------------------+|
   +---------------------------------------+----------------------------------------------------------------------------+


#BIOS reset
   +--------------------------------------------------------------------------------------------------------------------+
   | +-----------------------+             +---------------------------------------------------------------------------+|
   | | BIOS                  |             |   BMC                                                                     ||
   | |                       |             |  +-------------------------------+         +-----------------------------+||
   | |                       |             |  |PLDM Interface (MCTP)          |         |RBC Daemon Manager           |||
   | |                       |             |  | -Responsible for send /recv   |         |-Responsible for handle BIOS |||
   | |                       |             |  |  data between BIOS and BMC    |         | configuration attributes.   |||
   | |                       |             |  |                               |         | -BIOSCapability             |||
   | |                       |             |  |                               |         | -inBandIntf(PLDM)           |||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+||
   | |Get the table info     |<---Req-/Res--> | Provide the table information |         |                             |||
   | | & Check table Tag     |             |  |                               |         |                             |||
   | |using GetBIOS table    |             |  |                               |         |                             |||
   | |                       |             |  |                               |         |                             |||
   | | Get the Pending table |<---Req-/Res--> | Provide the Pending attributes|         |                             |||
   | | using Get BIOS table  |             |  | table                         |         |                             |||
   | |                       |             |  |                               |         |                             |||
   | |SetBIOSAttributeValue  |<---Req-/Res--> | Get the attributes table      |         |                             |||
   | |table via SetBIOStable.|             |  | Delete the Pending attributes |         |                             |||
   | |Init the BIOS config   |             |  | table                         |         |                             |||
   | |Based on value.        |             |  |                               |         |                             |||
   | |                       |             |  | Send the attributes data in   |         |                             |||
   | |                       |             |  | native to D-bus format by     |--dbus-->| Attributes data             |||
   | | Reset the BIOS boot   |             |  |  SetAllBaseAttributes()       |         |                             |||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+||
   |                                       +---------------------------------------------------------------------------+|
   +---------------------------------------+----------------------------------------------------------------------------+

##Complete BIOS BMC flow for BIOS configuration in deferred update model
```
+----------------------------------------+                    +----------------------------------------+
|                BIOS                    |                    |                  BMC                   |
|                                        |                    |                                        |
|  +----------------------------------+  |                    | +------------------------------------+ |
|  | Send the BIOS capability  Support|  |--------KCS-------->| |1.Get the complete atttributes data.| |
|  | Send the compressed BIOS file(or)|  |-MCTP/KCS/Redfish-->| |2.Validate and convert into         | |
|  | Send PLDM data via MCTP  (or)    |  |                    | |  native to D-bus format.           | |
|  | Send the Redfish host interface  |  |                    | |3.Expose the D-bus interface        | |
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
|  | If new attribute value exist     |<-|-----------------------|  Send the new value attributes    | |
|  |           then                   |  |                    |  |                                   | |
|  | Get & Update the BIOS variables  | -| -----+             |  |                                   | |
|  |                                  |  |      |             |  +-----------------------------------+ |
|  +---------------+------------------+  |      |             |                                        |
|                  |                     |      |             |                                        |
|                  YES                   |      |             |                                        |
|                  |                     |      |             |  +----------------------------------+  |
|   +--------------V------------------+  |      |             |  |                                  |  |
|   |  Send the updated data to BMC   |  |      |             |  | Update the BIOS attributes       |  |
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
|  | Send the BIOS capability  Support|  |--------KCS-------->| |1.Get the complete atttributes data | |
|  | Send the compressed BIOS file(or)|  |-MCTP/KCS/Redfish-->| |2.Validate and convert into         | |
|  | Send PLDM data via MCTP  (or)    |  |                    | |  native to D-bus format.           | |
|  | Send the Redfish host interface  |  |                    | |3.Expose the D-bus interface        | |
|  +----------------------------------+  |                    | +------------------------------------+ |
|                                        |                    |                                        |
|                                        |                    | +------------------------------------+ |
|                                        |                    | |PLDM can send complete all tables   | |
|                                        |                    | | & send the native to D-bus format  | |
|                                        |                    | | by SetAllBaseAttributes() Method   | |
|                                        |                    | +------------------------------------+ |
|                                        |                    | +------------------------------------+ |
|                                        |                    | | If attribute  value changed then   | |
|                                        |                    | | Biosconfig send signal to PLDM.    | |
|                                        |                    | | PLDM read Pending attributes table | |
|                                        |                    | | PLDM send the message to the host  | |
|                                        |                    | | PLDM send the data to the host     | |
|                                        |                    | +------------------------------------+ |
|  +----------------------------------+  |                    | +------------------------------------+ |
|  | Read the new Atributes value     |<-|----------------------| Send new Pending attributes table  | |
|  |           and                    |  |                    | |                                    | |
|  | Update in the BIOS structure     |  |                    | +------------------------------------+ |
|  +---------------+------------------+  |                    |                                        |
|                  |                     |                    |                                        |
|                  YES                   |                    |                                        |
|                  |                     |                    | +-----------------------------------+  |
|   +--------------V------------------+  |                    | | Update the BIOS atrributes data.  |  |
|   |  Send the updated data to BMC   |  |                    | | Discard the Pending attributes    |  |
|   |                                 |------------------------>|                        table      |  |
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
   | |                       |             |  |                       |       |                           | ||
   | |Get Current attributes |<---Req-/Res--> | Call RBC D-bus Method |<-dbus-| GetAllBaseAttributes()    | ||
   | | name & value list     |             |  |                       |       |                           | ||
   | |                       |             |  |                       |       |                           | ||
   | |Get Attribute Registry |<---Req-/Res--> | Call RBC D-bus Method |<-dbus-| GetAllBaseAttributes()    | ||
   | |                       |             |  |                       |       |                           | ||
   | |Change BIOS Password   |<---Req-/Res--> | Call RBC D-bus Method |-dbus->| ChangePassword()          | ||
   | |                       |             |  |                       |       |                           | ||
   | |To default settings    |<---Req-/Res--> | Call RBC D-bus Method |-dbus->| ResetBiosSettings()       | ||
   | |                       |             |  |                       |       |     -ResetFlag            | ||
   | |Update BIOS settings   |<---Req-/Res--->| Call RBC D-bus Method |-dbus->| SetAttribute()            | ||
   | |                       |             |  |                       |       |                           | ||
   | |Get attribute by name  |<---Req-/Res--->| Call RBC D-bus Method |<-dbus-| GetAttribute()            | ||
   | |                       |             |  |                       |       |                           | ||
   | |Get Pending attributes |<---Req-/Res--->| Call RBC D-bus Method |<-dbus-| GetPendingAttributes()    | ||
   | |         list          |             |  |                       |       |                           | ||
   | |Set Pending attributes |<---Req-/Res--->| Call RBC D-bus Method |<-dbus-| SetPendingAttributes()    | ||
   | |         list          |             |  |                       |       |                           | ||
   | |                       |             |  |                       |       | SetAllBaseAttributes()    | ||
   | |                       |             |  |                       |       | This Method is used to    | ||
   | |                       |             |  |                       |       | set all attributes data.  | ||
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

RBC daemon should maintain attributes data in native-D-bus format.
Each interface should validate the received data and provide the attributes data
in native to D-bus format by SetAllBaseAttributes.

## Impacts
BIOS must support and follow RBC BIOS configuration flow.

## Testing
Able to change the BIOS configuration via BMC through LAN
Able to change the BIOS setup password via BMC
Compliance with Redfish will be tested using the Redfish Service Validator
