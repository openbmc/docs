# Network manager NCSI capability support
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
NCSI capabilities and availability of the current NCSI channel which is enabled.

The phosphor network manager has a CLI utility using it we can know about
the NCSI capabilities of the Network controller. but the CLI util needs to be
polled for each specific package,channel, IfIndex combination every time to
check if that specific channel is active or not rather than invoking it once
and getting the available list of packages and it's channels along with
currently active channel.

Example :
PLDM applications such as sensors based on NCSI over RBT will need to check
the availability and readiness of the NCSI interface associated with the
Network controller before communicating through PLDM commands to the
Network controller.

## Background and References

**PLDM design for NCSI over RBT**

https://gerrit.openbmc.org/c/openbmc/docs/+/47519

**NCSI DMTF specification**
https://www.dmtf.org/sites/default/files/standards/documents/DSP0222_1.0.0.pdf

**PLDM NIC MODEL**
https://www.dmtf.org/sites/default/files/standards/documents/DSP2054_1.0.0.pdf

## Requirements

The following are the requirements regarding NCSI supported by phosphor-networkd.

The Network manager should be able check if the NIC has NCSI support and
populate the network controller's NCSI capabilities and current active
NCSI link parameters.

1. Network manager should introduce a new DBUS interface/object path for NCSI
   capability.
2. It should then identify the selected NCSI package.
3. It should be able to identify the active NCSI channel in the package.
4. In case of the channel is deactivated then the respective channel related
   property should be reflect the channel deactivation status so that other
   processes will be made aware.

## Proposed Design

1. As part of the network manager process a separate handler is created
for monitoring the NCSI capabilities and updating it in the DBUS object.
2. It uses the ncsi-netlink library to check the availability of
NCSI interface driver.
3. Then using the NCSI commands via ncsi-netlink socket, it polls the possible
packages and channel numbers in a loop to get the supported package numbers and
channel numbers for the specific IF interface.
4. after identifying current selected package and active channel,query for the
package status(ready,not ready) and channel status(enabled,disabled,ready,
not-ready) and store it in the respective DBUS properties.

### Proposed DBUS object tree

The proposed NCSI dbus interfaces are submitted in the following review :

https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/56591

The following object path is created for keeping NCSI capabilities.
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
The following interface is created for NCSI package capabilities
```
xyz.openbmc_project.Network.NCSIPackage.Interface

```

The following properties are part of the NCSi package object path

.Selected      - This property shows the package selected status
                     (selected/not-selected).
.Ready         - This property displays ready state of package (ready,
                   not-ready).
.HWArbitration - This property indicates whether hardware arbitration is
                  enabled for the package.

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
The following interface is created for NCSI channel capabilities.

```
xyz.openbmc_project.Network.NCSIChannel.Interface

```

The following properties can be part of the NCSI channel object path.

.Ready    - This property shows the ready state of the current channel
            (ready,not-ready).
.Enabled  - This property displays if the channel is enabled or not-enabled.

## Impacts
The change only adds new DBUS interface to monitor the NCSI capabilities of BMC NC.
so minimal impact is expected in the existing implementation of network manager.
## Testing
The proposed design can be tested in a bmc platform which has NCSI capable
 Network controller.
