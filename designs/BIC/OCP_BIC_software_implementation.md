# OCP BIC(Bridge IC) Multi-host Platform specific Implementation

Author:
   Kumar Thangavel(kumar), [thangavel.k@hcl.com](mailto:thangavel.k@hcl.com)
   Naveen Moses(naveen), [naveen.mosess@hcl.com](mailto:naveen.mosess@hcl.com)
   Velumani T(velu),  [velumanit@hcl](mailto:velumanit@hcl.com)

other contributors:

created:
    Jul 28, 2022

## Problem Description

The following tasks requirements are not fit in the any of the existing OpenBMC
process and repositories.

1. OEM specific firmware upgrades through BIC: 

In some multi-host systems, The firmware upgrade of devices like BIOS, CPLD, VR
are OEM specific and IPMB based. The firmware upgrade utility is mainly based on
OEM specific IPMB commands which are sent through BIC. These OEM commands should
be initiated from a separate process. Since, these firmware upgrades are OEM
specific, this firmware utility code cannot fit in any of the existing
repositories in OpenBMC.

2. IPMB based power line status and monitoring :

In some multi-host platforms, there are use cases where power lines such as
POWER_GOOD are not direct mapped gpios. These power line status which are
specific to each hosts can only accessed by sending IPMB BIC commands which is
routed via BIC and response is sent as a IPMI BIC response.

These IPMI commands to request power line states are not generic but OEM
specific so this cannot be added part of the power-control process or any other
similar process.

3. BIC uses OEM specific IPMB commands for quering BIC related information such
as Get/Set host gpio states through ipmb commands, Get SDR information, Get host
postcode information etc.

To address the above issues, the new seperate repository 'ocp-bridge-ic' required
in OpenBMC.

## Background and References

BMC on the sled baseboard, and It manages the multi hosts in the sled. Each host
has a "Bridge Interconnect" (BIC) which is a micro controller that manages host
CPU etc. BIC not only talk to hosts, it can talk to other BICs on add-in-cards.
Each host BIC is connected through I2C interface with BMC.

The Bridge IC is specific to OCP multi host platforms where in this BIC act as a
bridge between host and BMC. The communication interface between the BMC and BIC
is IPMB.  This is applicable for facebook yosemitev2 platform.

```

      +--------------------------+             +--------------+
      |                          |             |     HOST1    |
      |                          |             |              |
      |                          |             |  +--------+  |
      |                          |     I2C     |  |        |  |
      |                          +-------------+-->  BIC   |  |
      |                          |             |  |        |  |
      |                          |             |  +--------+  |
      |                          |             |              |
      |           BMC            |             +--------------+
      |                          |
      |                          |
      |                          |             +--------------+
      |                          |             |    HOSTN     |
      |                          |             |              |
      |                          |             |  +---------+ |
      |                          |     I2C     |  |         | |
      |                          +-------------+-->  BIC    | |
      |                          |             |  |         | |
      |                          |             |  +---------+ |
      |                          |             |              |
      +--------------------------+             +--------------+

```

Below is the link for OCP TwinLake Design Spec.
https://www.opencompute.org/documents/facebook-twin-lakes-1s-server-spec

## Requirements

* Introducing new repository 'ocp-bridge-ic' in OpenBMC.
* ocp_bicd supports OEM firmware update of BIOS, CPLD, VR etc.
* 'ocp-bridge-ic' repository shall have a daemon to monitor IPMB powerlines
   status and dbus control
* Add a BIC cli utility for quering BIC related information.

## Proposed Design

This document proposes a new design engaging to introduce the new daemon
'ocp_bicd' in the new proposed repository 'ocp-bridge-ic'. The first platform we
plan to support is Yosemite V2. Support can be added for other OEM specific BIC
related platforms. 

This is the high level diagram for proposed ocp-bicd process in the repostitory
'ocp-bridge-ic'.

```
                      BMC
+--------------------------------------------+
|        ocp_bicd                            |
| +---------------------+    +-------------+ |
| |                     |    |             | |
| | +-----------------+ |    |             | |
| | |                 | |    |             | |
| | |                 | |    |             | |
| | | Firmware Update | |    |             | |
| | |      Util       | |    |             | |              HOST 1 - N
| | |                 | |    |             | |      +-------------------------+
| | |                 | |    |             | |      |          BIC            |
| | +-----------------+ |    |             | |      | +---------------------+ |
| |                     |    |             | |      | |                     | |
| |                     |    |             | |      | |   IPMB COMMANDS     | |
| | +-----------------+ |    |             | |      | |                     | |
| | |                 | |    |             | |      | |Ex : getDeviceId     | |
| | |                 | |Dbus|             | | IPMB | |                     | |                   
| | | IPMB Powerline  | +--->| Ipmbbrdiged +-+------> |                     | |
| | | Status and Dbus | |    |             | |      | |                     | |
| | |    control      | |    |             | |      | |   OEM COMMANDS      | |
| | |                 | |    |             | |      | |                     | |
| | |                 | |    |             | |      | |Ex : Bios Fw update  | |
| | +-----------------+ |    |             | |      | |     Powerline status| |
| |                     |    |             | |      | |                     | |
| | +-----------------+ |    |             | |      | +---------------------+ |
| | |                 | |    |             | |      |                         |
| | |                 | |    |             | |      +-------------------------+
| | |                 | |    |             | |
| | |  BIC Utility    | |    |             | |
| | |                 | |    |             | |
| | |                 | |    |             | |
| | |                 | |    |             | |
| | +-----------------+ |    |             | |
| |                     |    |             | |
| +---------------------+    +-------------+ |
|                                            |
+--------------------------------------------+

```

1. OEM specific firmware upgrades - 'ocp-bicd' a separate process which is
invoked by systemd service. This performs the firmware upgrades of devices like
BIOS, CPLD, VR etc. For each firmware upgrades, we have the following seperate
design doc which explains in detail.

https://gerrit.openbmc.org/c/openbmc/docs/+/51950

2. IPMB based power line status and monitoring - The following design doc which
explains in detail.

3. BIC cli utility - proposing new utility for OEM specific IPMB commands for
quering BIC related information such as Get/Set host gpio states through ipmb
commands, Get SDR information, Get host postcode information etc.

## Alternatives Considered

1) dbus-sensors - An approach has been tried with dbus-sensors repository. This
repo is mainly used for having sensor related operations and it is common for
all the platforms. OEM firmware update and other BIC related functionalites like
power-control and IPMB based GPIO state are mostly platform specific. So,
ocp_bicd may not fit in dbus-sensors.
   
2) fb-ipmi-oem - An approach has been tried with fb-ipmi-oem repository. This is
library code and part of ipmid daemon. This library code handles platform
specific and OEM commands request and response. But this code, cannot do
firmware update. OEM commands for firmware update cannot be initiated from
fb-ipmi-oem. Unfortunately, fb-ipmi-oem code only handles incoming OEM commands.
fb-ipmi-oem requests use handler functions and send responses back to the
requester. So, we can keep only OEM command handler functions.

3) openbmc-tools - An another approach has been tried with openbmc-tools
repository. This openbmc-tools repository is mainly used for debug and build
tools and scripts. This implementation is specific to OCP platforms and does not
use C++ code. Also, the MAINTAINERS have discouraged use of openbmc-tools for
our needs.

4) x86-power-control - The x86-power-control has option to monitor such not
directly mapped GPIO power lines over a dbus object which should be polling for
such power line state change and update its status on the dbus object which
power control requires. At present there is no specific location/repository
which can hold/monitor the states which needs to be.

## Impacts

This BIC design and implementations is only for OCP platform specific and this
code will need to be placed in a new repository. So, there is no impact for
others repositories and other platforms.

