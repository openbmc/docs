# OCP Multi-host Platform specific OEM command Implementation

Author:
   Kumar Thangavel(kumar), [thangavel.k@hcl.com](mailto:thangavel.k@hcl.com)
   Velumani T(velu),  [velumanit@hcl](mailto:velumanit@hcl.com)

other contributors:

created:
    Mar 9, 2022

## Problem Description

The Bridge IC is specific to OCP multi host platforms where in this BIC act as
a bridge between host and BMC. All the communication between host and BMC is
through BIC. The interface between the BMC and BIC is IPMB. This is applicable
for yosemitev2. Going forward we will have other interfaces as well redfish
also can be possible. There are standard and oem specific ipmb commands
supported in BIC, many oem commands are specific to some oem feature. These
oem commands should initiated from new daemon with new repository in openbmc.
for example OEM BIOS upgrade through IPMB, OEM CPLD upgrade etc. So to handle
this we may need a specific service to handle the BIC related features.

This design document focusing on oem commands for OCP multi host platforms, it
supports features such as firmware upgrade of CPLD, VR, BIOS and BIC. The first
platform which is going to supported is Yosemite V2. The proposed repository
name is ocp-bridge-ic.

## Background and References

The BMC to Host communication is happening via ipmbbridge and ipmid daemons.
These daemnos handling all the ipmb commands request and response for BMC to
host communication. These ipmb and OCP specific oem commands are sending and
receiving via Bridge-IC.

```
                                                        HOST1
     +-----------------------------------+     +----------------------+
     |                BMC                |     |        BIC           |
     |                                   |     |  +----------------+  |
     |  +------------+    +----------+   |     |  |   OEM COMMANDS |  |
     |  |            |    |          |   |IPMB |  |                |  |
     |  |            |    |          +---+-----+-->Ex:CPLD,BIC,VR, |  |
     |  |            |    |          |   |     |  | BIOS FW Update |  |
     |  |            |    |          |   |     |  +----------------+  |
     |  |            |    |          |   |     |                      |
     |  | ocp_bicd   |    |ipmb      |   |     +----------------------+
     |  |            |Dbus|   bridged|   |             H0ST2
     |  |            +---->          |   |     +----------------------+
     |  |            |    |          |   |     |        BIC           |
     |  |            |    |          |   |     |  +----------------+  |
     |  |            |    |          |   |IPMB |  | IPMB COMMANDS  |  |
     |  |            |    |          +---+-----+-->                |  |
     |  |            |    |          |   |     |  |Ex: getDeviceId |  |
     |  +------------+    +----------+   |     |  |                |  |
     |                                   |     |  +----------------+  |
     |                                   |     |                      |
     +-----------------------------------+     +----------------------+

```

The ipmbbriged is going to be the same all the ipmb commands to BIC is routed
through this daemon. The ocp-bridge-ic will have a implementation for the
complete features like firmware upgrade.

Firmware update OEM commands are differ from standard oem commands as standard
oem commands are initiated from host and send to BMC and handling in
b-ipmi-oem which has handler function for each standard oem command. Firmware
update OEM commands are initiated from ocp-bicd and send to host/devices for
firmware update and response will be sent back to ocp-bicd.

Below is the link for OCP TwinLake Design Spec.
https://www.opencompute.org/documents/facebook-twin-lakes-1s-server-spec

## Requirements

* Implementing firmware upgrade using oem commands.
* exposing dbus interface to communicate with other modules (need to decide)
* updating progress and completion status in dbus interface

## Proposed Design

This document proposes a new design engaging the OEM commands request and
response for firmware update of CPLD, VR, BIOS, etc, and it's implementation
details.

The implementation flow of OEM firmware update:

1) Seperate daemon will be created in a new repository for this OEM firmware
update.
2) Image can be read from the path and it has been validated.
3) If image is valid, then get the device details from the user. The device
which needs to updated. Ex CPLD, VR, BIOS, etc.
4) Target can be set for firmware update. Target will be CPLD, VR, BIOS, etc
5) The OEM command will be framed as Ipmb command with netfn and cmd
6) The OEM commands are initiated from ocp-bicd deamon and it will directly
sent to ipmbbridged using sendRequest dbus method call.
Ex: busctl call xyz.openbmc_project.Ipmi.Channel.Ipmb
/xyz/openbmc_project/Ipmi/Channel/Ipmb org.openbmc.Ipmb sendRequest yyyyay
0 6 0 0x1 0

```
#busctl tree xyz.openbmc_project.Ipmi.Channel.Ipmb
└─/xyz
  └─/xyz/openbmc_project
    └─/xyz/openbmc_project/Ipmi
      └─/xyz/openbmc_project/Ipmi/Channel
        └─/xyz/openbmc_project/Ipmi/Channel/Ipmb

#busctl introspect xyz.openbmc_project.Ipmi.Channel.Ipmb
                   /xyz/openbmc_project/Ipmi/Channel/Ipmb
NAME                                TYPE      SIGNATURE RESULT/VALUE FLAGS
org.freedesktop.DBus.Introspectable interface -         -            -
.Introspect                         method    -         s            -
org.freedesktop.DBus.Peer           interface -         -            -
.GetMachineId                       method    -         s            -
.Ping                               method    -         -            -
org.freedesktop.DBus.Properties     interface -         -            -
.Get                                method    ss        v            -
.GetAll                             method    s         a{sv}        -
.Set                                method    ssv       -            -
.PropertiesChanged                  signal    sa{sv}as  -            -
org.openbmc.Ipmb                    interface -         -            -
.sendRequest                        method    yyyyay    (iyyyyay)    -

```

7) Then ipmbbridged forwards that oem commands to BIC.
8) BIC process those oem commads and sends the response to the ipmbbridged.
9) Ipmbbridged forrwards the response the ocp-bicd daemon.
10) If any error response, then write flashing and update is considered as
failed.
11) If no errors, then flashing and firmware update is success.

## BIOS Update Procedure

1) User can use custom oem flashing tool "ocp-bicd" daemon in the BMC to do the
   BIOS Firmware update.
2) BIOS image path is passing as one of the paramter to the "ocp-bicd" daemon.
3) ocp-bicd daemon read the image file and calculate the file size.
4) This daemon sends "Firmware Update"(09h) command to Bridge IC for BIOS update
   request. It sends as IPMB command to Bridge IC.
5) Bridge-IC will check IPMB packet checksum and ensure data correctness.
6) If checksum is correct, Bridge IC starts to write BIOS firmware via SPI.
7) BMC then send 224 bytes per packet to do BIOS update and wait for Bridge IC
   response or IPMB timeout.
8) If timeout happens, BMC will re-send the IPMB package. The maximum retry is
   3 times. If retry time reaches 3, the update procedure will stop. In this
   case, user needs to issue update command again to re-initiate update
   procedure.
9) After full BIOS Firmware image update is complete, BMC sends
   “Firmware Verify”(0Ah) command that contents verify offset and data length
   to Bridge IC.
10) Bridge IC will then read corresponding image data from BIOS ROM, calculate
    the data checksum and send the checksum back to BMC.
11) BMC will check the correctness of the checksum. Repeat the process till the
    whole image was verified.

```
        +---------+     SPI      +-----------+
        |BIOS ROM <--------------+ BRIDGE-IC |
        |         |              |           |
        +---------+              +-----^-----+
                                       |
                                       |
                                       | I2C(IPMB)
                                       |
                                       |
                                 +-----+-----+
                                 |           |
                                 |    BMC    |
                                 +-----------+
```

## CPLD  Update procedure - TBD


## Redfish Support
Redfish DMTF standard (DSP2062_1.0.0) supports for firmware update of devices
like BMC, BIOS, CPLD, etc. User can specify the host which needs to be updated
using Redfish. The device list can be generated using Inventory.

## Alternatives Considered

1) fb-ipmi-oem - An approach has been tried with fb-ipmi-oem repository. This
   is library code and part of ipmid daemon. This libraray code handles platform
   specific and oem commands request and response. But this code, cannot do
   firmware update. OEM commands for firmware update cannot initiated from
   fb-ipmi-oem. Because fb-ipmi-oem code handles only incoming oem commands
   request using handler functions and send the response back to the requester
   command. So, we can keep only oem command handler functions.

2) openbmc-tools - An another approach has been tried with openbmc-tools
   repository. This openbmc-tools repository is mainly used for having debug
   and build tools and scripts. This implementation is specific to OCP
   platforms and does not having C++ code. Also, MAINTAINERS not suggested to
   use this repository.

## Impacts

This is only for OCP platform specific implementation and these codes lies in
new repository. So, there is no impact for others repositories and other
platforms.

## Testing

Testing this OEM commands like firmware update for CPLD, VR and Bios with yosemitev2
platform.

