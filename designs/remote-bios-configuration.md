# Remote BIOS Configuration (RBC) via BMC

Author: Suryakanth Sekar! ssekar

Other contributors: Jia Chunhui Deepak Kodihalli Patrick Williams

Created: 19-Nov-2019

## Problem Description

Current OpenBMC doesn't provide mechanism to configure the BIOS remotely. This
is needed in data center to maintain several systems under same configuration.

Remote BIOS Configuration provides ability for the user to view and modify BIOS
setup configuration parameters remotely via a BMC at any Host state. New BIOS
configuration parameters take effect immediately or next system reboot based on
the host firmware support model.

## Background and References

[1]
https://www.dmtf.org/sites/default/files/standards/documents/DSP0247_1.0.0.pdf
[2] https://redfish.dmtf.org/schemas/v1/Bios.v1_1_0.json [3]
https://redfish.dmtf.org/schemas/v1/AttributeRegistry.v1_3_2.json

## Requirements

1. Mechanism to configure BIOS settings remotely over network interface.
2. BMC should support the ability to set the value of all BIOS variables to the
   factory default state.
3. Based on the host firmware support model, BMC should support both Immediate
   Update or Deferred update.
4. In deferred model, When the system is in S0, S3, S4 or S5 state. BMC will
   send the updated BIOS variables on next BIOS boot only. It will not initiate
   a BIOS boot immediately.
5. In immediate model, BMC should send message to the system firmware (BIOS)
   Whenever settings are changed.
6. BMC should support BIOS attribute registry in redfish schema for BIOS
   configuration.
7. BMC should provide secure way for updating BIOS setup password settings.
   Detailed password handling design -TBD(will be resolve using ARM TrustZone)
   Ex: Updating the BIOS password should be support only before end of post.
8. Remote BIOS configuration daemon should be independent of interface specific
   data format.
9. BMC should able to take default / current settings from host and store &
   expose that for out of band updates. 10.BMC should provide the new values to
   the host.

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
| +-------------+       +-------------+       |     -SetAttribute()            |      |       |    +----^-----+  |
|                                             |     -GetAttribute()            |      |       |         |        |
| +-------------+       +-------------+       |     -VerifyPassword()          |      |       |        LAN       |
| |             |       |             |       |     -ChangePassword()          |      |       |         |        |
| | HOST/ BIOS  +<-KCS->+  HOST-IPMID +<Dbus->+                                |      |Redfish|    +----V-----+  |
| |             |       |             |       | Properties                     +<Dbus>+  API  |    |Redfish & |  |
| +-----+-------+       +-------------+       |     -BaseBIOSTable             |      |       +<-->+BMCWeb    |  |
|       |                                     |     -PendingAttributes         |      |       |    +----^-----+  |
|       |                                     |     -ResetBIOSSettings         |      |       |         |        |
|       |                                     |     -IsPasswordInitDone        |      |       |         |        |
|       |                                     |                                |      |       |    +----V-----+  |
|       |                                     |                                |      |       |    | Redfish  |  |
|       |                                     |                                |      |       |    |  Host    |  |
|       |                                     |                                |      |       |    | Interface|  |
|       |                                     +----^-----------------+---------+      +-------+    +----------+  |
|       |                                          |       PendingAttributesUpdated                              |
|       |                                          |                 |    Signal                                 |
|       |                                          |                 |                                           |
|       |                                     +----V-----------------V---------+                                 |
|       |                                     | PLDM daemon                    |                                 |
|       |                                     | Collect the BIOS data & convert|                                 |
|       +---------MCTP----------------------->| into native D-bus format and   |                                 |
|                                             | send to the RBC                |                                 |
|                                             +--------------------------------+                                 |
+----------------------------------------------------------------------------------------------------------------+
```

##Intel uses the following logic

BIOS send data in as Proprietary format to the BMC via IPMI interface. There are
two types of proprietary XML format files in BIOS configuration. Type-0 contain
full BIOS variables in XML format. (Generated by BIOS) Type-1 contain modified
BIOS variables in XML format. (Generated by BMC)

BIOS must provide BIOS capability via KCS interface in early boot stage. BIOS
must send compressed proprietary XML type 0 file via IPMI command to the BMC.

IPMI interface should decompress & validate the received XML Type 0. IPMI
interface should convert the XML Type 0 into native to D-bus format and send to
the RBC daemon.

During BDS phase in BIOS. BIOS must get the existing XML info from BMC. If XML
version/checksum is mismatch or XML Type 0 is not present in BMC, then BIOS must
send XML type 0 to the BMC. If XML version and XML checksum matched & pending
BIOS attributes list exist (XML Type 1) in BMC then BIOS must get pending BIOS
configuration (XML Type 1) from BMC & update in BIOS region and send updated XML
Type 0 to the BMC in order to intact again and then BIOS reset the system to
reflect the updated values in BIOS boot.

BIOS have default BIOS settings in BIOS non-volatile memory. BIOS can restore
the default BIOS configuration based on the flag setting in OEM IPMI command
issued during BIOS booting. So, restore default BIOS configuration can be done
by this mechanism.

RBC daemon should preserve the AllBiosTables, PendingAttributes list in
non-volatile storage. Pending attributes list will be cleared whenever new
attributes data received.

```
#Intel uses the following logic for BIOS first boot
   +---------------------------------------------------------------------------------------------------------------------+
   |                                                                                                                     |
   | +-----------------------+             +----------------------------------------------------------------------------+|
   | | BIOS                  |             |   BMC                                                                      ||
   | |                       |             |  +-------------------------------+        +------------------------------+ ||
   | |                       |             |  |IPMI Interface (kcs)           |        |RBC daemon Manager            | ||
   | |                       |             |  | -Responsible for send /recv   |        |-Responsible for handling BIOS| ||
   | |                       |             |  |  data between BIOS and BMC    |        | configuration attributes.    | ||
   | |                       |             |  |                               |        | -AllBaseAttributes           | ||
   | |                       |             |  |                               |        | -Pending Attributes          | ||
   | |                       |             |  |                               |        | -FactoryDefaultSetting       | ||
   | |                       |             |  |                               |        | -BIOSPwdHashData             | ||
   | |                       |             |  +-------------------------------+        +------------------------------+ ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+ ||
   | | Set BIOS capability   |<---Req-/Res--> | Set the BIOS capability       |         |                             | ||
   | |                       |             |  |                               |         |                             | ||
   | |Set BIOS Pwd hash/Seed |<---Req-/Res--> | Forward to the RBC daemon     |--dbus-->| Set BIOSPwdHash data.       | ||
   | |                       |             |  |                               |         |                             | ||
   | |                       |             |  |                               |         |                             | ||
   | |Check factory settings |<---Req-/Res--> |  Get the FactoryDefault prop  |<-dbus-- | Send FactoryDefaultSettings | ||
   | |Init the BIOS config   |             |  |                               |         |                             | ||
   | |Based on value.        |             |  |                               |         |                             | ||
   | |Get the XML Type0 info |<---Req-/Res--> | Provide the XML Type 0 Info   |         |                             | ||
   | |Generate & compress    |             |  |                               |         |                             | ||
   | |XML type 0 file        |             |  | Validate the XML Type 0       |         |Collect the Attributes Info  | ||
   | |                       |             |  | Unzip the XML Type 0          |--dbus-->| AllBaseAttributes           | ||
   | | Send the XML Type 0   |<---Req-/Res--->| Convert into D-bus format     |                                       | ||
   | | via SetPayload command|             |  | and send to the RBC.          |         |                             | ||
   | |                       |             |  |                               |         |                             | ||
   | | Continue the BIOS boot|             |  |                               |         |                             | ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+ ||
   |                                       +----------------------------------------------------------------------------+|
   +---------------------------------------+-----------------------------------------------------------------------------+
```

```
#Intel uses the following logic for BIOS reset
   +---------------------------------------------------------------------------------------------------------------------+
   |                                                                                                                     |
   | +-----------------------+             +----------------------------------------------------------------------------+|
   | | BIOS                  |             |   BMC                                                                      ||
   | |                       |             |  +-------------------------------+        +------------------------------+ ||
   | |                       |             |  |IPMI Interface (kcs)           |        |RBC daemon Manager            | ||
   | |                       |             |  | -Responsible for send /recv   |        |-Responsible for handling BIOS| ||
   | |                       |             |  |  data between BIOS and BMC    |        | configuration attributes.    | ||
   | |                       |             |  |                               |        | -AllBaseAttributes           | ||
   | |                       |             |  |                               |        | -Pending Attributes          | ||
   | |                       |             |  |                               |        | -FactoryDefaultSetting       | ||
   | |                       |             |  |                               |        | -BIOSPwdHashData             | ||
   + |                       |             |  +-------------------------------+        +------------------------------+ ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+ ||
   | |Set BIOS capability    |<---Req-/Res--> | Set the BIOS capability       |         |                             | ||
   | |                       |             |  |                               |         |                             | ||
   | |Set BIOS Pwd hash/Seed |<---Req-/Res--> | Forward to the RBC daemon     |--dbus-->| Set BIOSPwdHash data        | ||
   | |                       |             |  |                               |         |                             | ||
   | |                       |             |  |                               |         |                             | ||
   | |Check factory settings |<---Req-/Res--> | Get the FactoryDefault prop   |<-dbus-- | Send FactoryDefaultSettings | ||
   | |Init the BIOS config   |             |  |                               |         |                             | ||
   | |Based on value.        |             |  |                               |         |                             | ||
   | |Get the XML Type 0 info|<---Req-/Res--> | Provide the XML Type 0 Info   |         |                             | ||
   | |Generate & compress    |             |  |                               |         |                             | ||
   | |XML type 0 file.       |             |  |                               |         |Collect the Attributes Info  | ||
   | |Check XML file Chksum. |             |  | Validate the XML Type 0       |--dbus-->| BaseBIOSTable               | ||
   | |If Chksum mismatch     |<---Req-/Res--->| Unzip XML Type 0 & convert    |         |                             | ||
   | |then send the payload  |             |  | into D-bus format and send to |         |                             | ||
   | |via SetPayload.        |             |  | the RBC                       |         |                             | ||
   | |Continue the BIOS boot |             |  |                               |         |                             | ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+ ||
   |                                       +----------------------------------------------------------------------------+|
   +---------------------------------------+-----------------------------------------------------------------------------+
```

```
#Intel uses the following logic for BIOS reset and BMC have new values
   +---------------------------------------------------------------------------------------------------------------------+
   |                                                                                                                     |
   | +-----------------------+             +----------------------------------------------------------------------------+|
   | | BIOS                  |             |   BMC                                                                      ||
   | |                       |             |  +-------------------------------+        +------------------------------+ ||
   | |                       |             |  |                               |        |                              | ||
   | |                       |             |  |IPMI Interface (kcs)           |        |RBC daemon Manager            | ||
   | |                       |             |  | -Responsible for send /recv   |        |-Responsible for handling BIOS| ||
   | |                       |             |  |  data between BIOS and BMC    |        | configuration attributes.    | ||
   | |                       |             |  |                               |        | -AllBaseAttributes           | ||
   | |                       |             |  |                               |        | -Pending Attributes          | ||
   | |                       |             |  |                               |        | -FactoryDefaultSetting       | ||
   | |                       |             |  |                               |        | -BIOSPwdHashData             | ||
   + |                       |             |  +-------------------------------+        +------------------------------+ ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+ ||
   | |Set BIOS capability    |<---Req-/Res--> | Set the BIOS capability       |         |                             | ||
   | |                       |             |  |                               |         |                             | ||
   | |Set BIOS Pwd hash/Seed |<---Req-/Res--> | Forward to the RBC daemon     |---dbus->| Set BIOSPwdHash data        | ||
   | |                       |             |  |                               |         |                             | ||
   | |                       |             |  |                               |         |                             | ||
   | |Check factory settings |<---Req-/Res--> | Get the FactoryDefault prop   |<--dbus--| Send FactoryDefaultSettings | ||
   | |Init the BIOS config   |             |  |                               |         |                             | ||
   | |Based on value.        |             |  |                               |         |                             | ||
   | |Get the XML Type 0 info|<---Req-/Res--> | Provide the XML Type 0 Info   |         |                             | ||
   | |Generate & compress    |             |  |                               |         |                             | ||
   | |XML Type 0 file.       |             |  |                               |         |                             | ||
   | |Check XML file Chksum. |             |  |                               |         |                             | ||
   | |If Chksum   match      |<---Req-/Res--->| Provide the XML Type 0        |         |                             | ||
   | |then get the XML Type 1|             |  |                               |         |                             | ||
   | |via GetPayload.        |             |  |                               |         |                             | ||
   | |Get  the XML Type 1    |             |  |                               |         |                             | ||
   | |via GetPayload command.|<---Req-/Res--->| Provide the XML Type 1        |         |                             | ||
   | |                       |             |  |                               |         |                             | ||
   | |Update the new value   |             |  |                               |         |                             | ||
   | |and new chksum in      |<---Req-/Res--->| Get new XML Type 0 & convert  |--dbus-->| Collect the Atributes info  | ||
   | |BIOS.                  |             |  | into native to D-bus format   |         | AllBaseAttributes           | ||
   | |Reset the system.      |             |  |                               |         |                             | ||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+ ||
   |                                       +----------------------------------------------------------------------------+|
   +---------------------------------------+-----------------------------------------------------------------------------+

```

##BIOS send the data in BIOS configuration PLDM via MCTP

BIOS should update the BIOS settings via Set BIOS table PLDM command- BIOS
string table, Attribute name table, Attribute value table via MCTP.

RBC daemon should create pending BIOS attributes list based on the user input
configuration and send PendingAttributesUpdated signal.

PLDM daemon should register for PendingAttributesUpdated signal & RBC daemon
should generate signal whenever attribute value changed by SetPendingAttributes,
set attribute D-bus call. PLDM should Wait for PendingAttributesUpdated signal
and pull the pending attributes value from RBC daemon. PLDM should delete the
Pending attributes value table once its not valid or updated in BIOS. RBC should
clear pending attributes list whenever new attributes data received.

RBC daemon should preserve the AllBaseAttributes, PendingAttributes list in
non-volatile storage. PLDM daemon should preserve BIOS tables in non-volatile
storage. RBC and PLDM should restored the data whenever BMC reset.

#BIOS first boot

```

   +--------------------------------------------------------------------------------------------------------------------+
   | +-----------------------+             +---------------------------------------------------------------------------+|
   | | BIOS                  |             |   BMC                                                                     ||
   | |                       |             |  +-------------------------------+         +-----------------------------+||
   | |                       |             |  |PLDM Interface (MCTP)          |         |RBC Daemon Manager           |||
   | |                       |             |  | -Responsible for send /recv   |         |-Responsible for handle BIOS |||
   | |                       |             |  |  data between BIOS and BMC    |         | configuration attributes.   |||
   | |                       |             |  |                               |         | -AllBaseAttributes          |||
   | |                       |             |  |                               |         | -Pending Attributes         |||
   | |                       |             |  |                               |         | -FactoryDefaultSetting      |||
   | |                       |             |  |                               |         | -BIOSPwdHashData            |||
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
   | |Based on value.        |             |  | native to D-bus format        |--dbus-->| AllBaseAttributes           |||
   | |via Set BIOS table     |             |  | by Setting AllBaseAttributes  |         |                             |||
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
   | |                       |             |  |                               |         | -AllBaseAttributes          |||
   | |                       |             |  |                               |         | -Pending Attributes         |||
   | |                       |             |  |                               |         | -FactoryDefaultSetting      |||
   | |                       |             |  |                               |         | -BIOSPwdHashData            |||
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
   | |                       |             |  | native to D-bus format by     |--dbus-->| AllBaseAttributes           |||
   | | Reset the BIOS boot   |             |  | setting AllBaseAttributes     |         |                             |||
   | +-----------------------+             |  +-------------------------------+         +-----------------------------+||
   |                                       +---------------------------------------------------------------------------+|
   +---------------------------------------+----------------------------------------------------------------------------+

```

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
|  |           then                   |  |                    |  |  (Pending Attributes list)        | |
|  | Get & Update the BIOS variables  | -| -----+             |  |                                   | |
|  |                                  |  |      |             |  +-----------------------------------+ |
|  +---------------+------------------+  |      |             |                                        |
|                  |                     |      |             |                                        |
|                  YES                   |      |             |                                        |
|                  |                     |      |             |  +----------------------------------+  |
|   +--------------V------------------+  |      |             |  |                                  |  |
|   |  Send the updated data to BMC   |  |      |             |  | Update the BIOS attributes       |  |
|   |                                 |------------------------->| (BaseBIOSTable)                  |  |
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
|                                        |                    | | by Setting AllBaseAttributes       | |
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

RBC should create the Pending Attribute list whenever SetPendingAttributes &
SetAttribute Method called and raise the PendingAttributesUpdated signal. RBC
should preserve the Pending Attributes list across the BMC reset and RBC should
clear the Pending Attributes list whenever new AllBaseBIOSTables received from
BIOS.

#Redfish interfaces for remote Bios configuration

```
 +-----------------------------------------------------------------------------------------------------------+
 | +-------------------------+             +----------------------------------------------------------------+|
 | | RBC Web tool - POSTMAN  |             |   BMC                                                          ||
 | |  (Please refer the      |             |  +-----------------------+       +---------------------------+ ||
 | |   below redfish example |             |  |Redfish Daemon         |       |RBC Daemon Manager         | ||
 | |   for each request)     |             |  |-Responsible for handle|       |-Parse Bios Data,convert to| ||
 | |                         |             |  |all Redfish request    |       | required format & return  | ||
 | |                         |             |  +-----------------------+       +---------------------------+ ||
 | +-------------------------+             |  +-----------------------+       +---------------------------+ ||
 | |                         |             |  |                       |       |                           | ||
 | |1.Get Current attributes |<---Req-/Res--> | Read BaseBIOSTable    |<-dbus-| BaseBIOSTable             | ||
 | |   name & value list     |             |  |                       |       |                           | ||
 | |                         |             |  |                       |       |                           | ||
 | |2.Get Attribute Registry |<---Req-/Res--> | Read BaseBIOSTable    |<-dbus-| BaseBIOSTable             | ||
 | |                         |             |  |                       |       |                           | ||
 | |3.Change BIOS Password   |<---Req-/Res--> | Call RBC D-bus Method |-dbus->| ChangePassword()          | ||
 | |                         |             |  |                       |       |                           | ||
 | |4.Reset To default       |<---Req-/Res--> | Set ResetBIOSSettings |-dbus->| ResetBiosSettings         | ||
 | |            settings     |             |  |                       |       |     -ResetFlag            | ||
 | |5.Update new BIOS setting|<---Req-/Res--->| Call RBC D-bus Method |-dbus->| SetAttribute()            | ||
 | |  (For single attribute) |             |  |                       |       |                           | ||
 | |                         |             |  |                       |       |                           | ||
 | |6.Get Pending attributes |<---Req-/Res--->| Get PendingAttributes |<-dbus-| PendingAttributes         | ||
 | |           list          |             |  |                       |       |                           | ||
 | |7.Update new BIOS setting|<---Req-/Res--->| Set PendingAttributes |<-dbus-| PendingAttributes         | ||
 | |           list          |             |  |                       |       |                           | ||
 | |  For multiple attributes|             |  |                       |       |                           | ||
 | +-------------------------+             |  +-----------------------+       +---------------------------+ ||
 |                                         +---------------------------------------------------------------+||
 +-----------------------------------------+-----------------------------------------------------------------+
```

1. Get Current Attributes name and value list: Get the current BIOS settings
   attribute name and value pair list. GET Method -
   "https://<BMC IP address>/redfish/v1/Systems/system/Bios"

2. Get Attribute Registry: Get the detailed information about Bios Attribute
   like current value, supported value, description, Menupath, Default value.
   GET Method - "https://<BMC IP address>/redfish/v1/Registries/Bios"

3. Change BIOS password: ACTION -
   "https://<BMC IP address>/redfish/v1/Systems/system/Bios/Actions/Bios.ChangePassword"

4. Reset To default settings: ACTION -
   "https://<BMC IP address>/redfish/v1/Systems/system/Bios/Actions/Bios.ResetBios"

5. Update new BIOS settings (single attribute): Use to send the new value for
   particular attribute or list of attributes. PATCH Method -
   "https://<BMC IP address>/redfish/v1/Systems/system/Bios/Settings" Ex:
   Attribute name and new value : { "DdrFreqLimit" : 2400}

6. Get the new pending value list: Use to get the new pending attributes list.
   GET Method -
   "https://<BMC IP address>/redfish/v1/Systems/system/Bios/Settings" -Valid
   only in deferred model. For immediate update model, It will be empty. Ex:
   Attribute name and new value : { "DdrFreqLimit" : 2400,"QuietBoot",0x1 }

7. Update new BIOS settings (multiple attributes): Use to send the new value for
   particular attribute or list of attributes. PATCH Method -
   "https://<BMC IP address>/redfish/v1/Systems/system/Bios/Settings" Ex:
   Attribute name and new value list : { "DdrFreqLimit" : 2400},"QuietBoot",0x1
   }

## Alternatives Considered

Redfish Host specification definition is not completed and ready BIOS support
also not available. There are 1000+ BIOS variables and storing in
phosphor-settingsd is not optimal.

## Impacts

BIOS must support and follow RBC BIOS configuration flow.

## Testing

Able to change the BIOS configuration via BMC through LAN Able to change the
BIOS setup password via BMC Compliance with Redfish will be tested using the
Redfish Service Validator
