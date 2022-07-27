# Network manager NCSI discovery support
Author:
  Naveen Moses S(naveen.moses), [naveen.mosess@hcl.com](mailto:naveen.mosess@hcl.com)
  Velumani T(velu),  [velumanit@hcl](mailto:velumanit@hcl.com)

Other contributors:

Created:
  July 27, 2022

## Problem Description

Right now there is no option in network manager to know about the BMC Network
Controller's NCSI capabilities by other processes.
There may be some apps which may need to know about the network controller's
NCSI support, available  NCSI package and channel info which are active.

The phosphor network manager has a CLI utility named ncsi-netlink. This NCSI
utility can be used to query the availability of a specific package/channel
info. There is no option to discover all supported packages and channels part
of the Network controller. Also, there is no DBUS objects represents the NCSI
parameters which is may be required by other processes such as PLDM apps
which need to know before startup that what are the available devices in NIC
which supports PLDM over NCSI based on RBT.

Example :
PLDM applications such as sensors based on NCSI over RBT will need to check
the availability and readiness of the NCSI interface associated with the
Network controller before communicating through PLDM commands to the
Network controller.

## Background and References

**PLDM design for NCSI over RBT**

https://gerrit.openbmc.org/c/openbmc/docs/+/47519

**NCSI DMTF specification**
https://www.dmtf.org/sites/default/files/standards/documents/DSP0222_1.1.1.pdf

**PLDM NIC MODEL**
https://www.dmtf.org/sites/default/files/standards/documents/DSP2054_1.0.0.pdf

## Requirements

The following are the requirements regarding NCSI supported by phosphor-networkd.

The main requirement for this design is that the Network manager
should be able check if the NIC has NCSI support, discover the NCSI
available packages, discover available channels under respective
packages and populate these details on DBUS.

1. Network manager should introduce a new DBUS interface/object path for each
   NCSI package discovered under network controller interface.

2. Network manager should introduce a new DBUS interface/object path for each
   NCSI channel discovered under respective NCSI package interface.

3. For each available NCSI package interface the selected state and package
   id should be stored as DBUS property under NCSI package DBUS interface.

4. For each channel available under NCSI package interface the active state and
   link state should be stored as DBUS property under NCSI channel DBUS
   interface.

## Proposed Design

1. As part of the network manager process a separate interface is created
  for discovering the NCSI capabilities and updating it in the DBUS object.

2. It uses the ncsi-netlink library to check the availability of
   NCSI interface driver.

3. Then using the NCSI command NCSI_CMD_PKG_INFO via ncsi-netlink socket,
   it polls the possible packages and channel numbers in a loop to get the
   supported package numbers and channel numbers for the specific IF interface.

4. After identifying current selected package and active channel,query for the
   package selected state, channel active status and link state
   stores it in the respective DBUS properties.

### Proposed DBUS object tree

The proposed NCSI dbus interfaces are submitted in the following review :

https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/56591

The following object path is created for storing discoverd NCSI
packages and channels.
under phosphor-networkd DBUS service : xyz.openbmc_project.Network

```
`-/xyz
  `-/xyz/openbmc_project
    `-/xyz/openbmc_project/network
      `-/xyz/openbmc_project/network/ethX/ncsi
```
### NCSI package object path
Based on the number of available NCSI packages respective
dbus object path is created under the "NCSI" node.

```
`-/xyz
  `-/xyz/openbmc_project
    `-/xyz/openbmc_project/network
      `-/xyz/openbmc_project/network/ethX/ncsi
       `-/xyz/openbmc_project/network/ethX/ncsi/package0
        .
        .
        .
     `-/xyz/openbmc_project/network/ethX/ncsi/packageN

```
The following interface is created for NCSI package info.
```
xyz.openbmc_project.Network.NCSIPackage.Interface

```

The following properties are part of the NCSI package object path

.Selected  - This property shows the package selected status
                     (selected/not-selected).

.PackageId - This property displays id of the package if supported.

### NCSI channel object path

For each package object path, the available NCSI channels
are listed as sub tree nodes as given below.
```
`-/xyz
  `-/xyz/openbmc_project
    `-/xyz/openbmc_project/network
      `-/xyz/openbmc_project/network/ethX/ncsi
       `-/xyz/openbmc_project/network/ethX/ncsi/package0
          `-/xyz/openbmc_project/network/ethX/ncsi/package0/channel0
            .
            .
            .
          `-/xyz/openbmc_project/network/ethX/ncsi/package0/channelN

```
The following interface is created for NCSI channel info.

```
xyz.openbmc_project.Network.NCSIChannel.Interface

```

The following properties can be part of the NCSI channel object path.

.Active    - This property shows the ready state of the current channel
             (ready,not-ready).
.LinkState - This property shows the NCSI channel link state.

## Impacts
The change only adds new DBUS interface to discover the NCSI parameters of BMC NIC.
so minimal impact is expected in the existing implementation of network manager.
## Testing
The proposed design can be tested in a bmc platform which has NCSI capable
 Network controller.
The change shall be tested on bmc systems whose NIC don't support NCSI as well
to ensure it doesn't affect anything.