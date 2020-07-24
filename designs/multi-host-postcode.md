# Multi-host Postcode Support

Author: Manikandan Elumalai, [manikandan.hcl.ers.epl@gmail.com]

Other contributors: None

Created: 2020-07-02

## Problem Description

The current implementation in the phosphor-host-postd supports only single host
postcode access through LPC interface.

As the open BMC architecture is evolving, the single host support becomes
contingent and needs multiple-host post code access to be implemented.

## Background and References

**Existing postcode implementation for single host**

The below component diagram shows the present implementation for postcode and
history at high-level overview
```
+----------------------------------+              +--------------------+
|  +-------------------------------+              |                    |
|  |Phosphor-host-postd            |              |                    |
|  |                    +----------+              +------------+       |
|  |                    | LPC      |              |            |       |
|  |                    |          +<-------------+            |       |
|  |                    +----------+              |  LPC       |       |
|  |                               |              |            |       |
|  |xyz.openbmc_project.State.     +<-------+     +------------+       |
|  |Boot.Raw.Value                 |        |     |                    |
|  +------+------------------------+        |     |         Host       |
|         |                        |        |     |                    |
|         +                        |        |     |                    |
|   postcode change event          |        +     +--------------------+
|         +                        |  xyz.openbmc_project.State.Boot.Raw
|         |                        |        +
|         v                        |        |      +------------------+
|  +------+------------------------+        +----->+                  |
|  |Phosphor-postcode-manager      |               |   CLI            |
|  |                 +-------------+               |                  |
|  |                 |   postcode  +<------------->+                  |
|  |                 |   history   |               |                  |
|  |                 +-------------+               +------------------+
|  +-------------------------------+  xyz.openbmc_project.State.Boot.PostCode
|                                  |
|    BMC                           |
|  +-------------------------------+             +----------------------+
|  |                               |  8GPIOs     |                      |
|  |     SGPIO                     +-----------> |                      |
|  |                               |             |     7 segment        |
|  +-------------------------------+             |     Display          |
|                                  |             |                      |
+----------------------------------+             +----------------------+
```
## Requirements

 - Read postcode from all servers.
 - Display the host postcode to the 7 segment display based on host position 
   selection.
 - Provide a command interface for user to see any server(multi-host) current 
   postcode .
 - Provide a command interface for user to see any server(multi-host) postcode
   history.
 - Support for hot-plug-able host.

## Proposed Design

This document proposes a new design engaging the IPMB interface to read port-80
post code from multiple-host. This design also supports host discovery
including the hot-plug-able host connected in slot.

Following modules will be updated for this implementation

 - phosphor-host-postd.
 - phosphor-post-code-manager.
 - platform specific OEM handler (fb-ipmi-oem).
 - phosphor-dbus-interfaces.

**Interface Diagram**
Provided below the post code interface diagram with flow sequence
```
+-------------------------------------------+
|                  BMC                      |
|                                           |
| +--------------+     +-----------------+  |    I2C/IPMI  +----+-------------+
| |              |     |                 |  |  +----------->BIC |             |
| |              |     |   ipmbbridged   <--+--+           |    |     Host1   |
| |              |     |                 |  |  |           +----+-------------+
| | oem handlers |     +-------+---------+  |  | I2C/IPMI  +----+-------------+
| |              |             |            |  +----------->BIC |             |
| |              |             |            |  |           |    |     Host2   |
| |              |     +-------v---------+  |  |           +----+-------------+
| | (fb-ipmi-oem)|     |                 |  |  | I2C/IPMI  +----+-------------+
| |              <-----+     ipmid       |  |  +----------->BIC |             |
| |              |     |                 |  |  |           |    |     Host3   |
| +------+-------+     +-----------------+  |  |           +----+-------------+
|        |                                  |  | I2C/IPMI  +----+-------------+
|        | postcode                         |  +----------->BIC |             |
|        |                                  |              |    |     HostN   |
| +------v-------------------------------+  |              +----+-------------+
| |                                      |  |
| |     phosphor-host-postd              |  |             +-----------------+
| |                                      |  |             |                 |
| |       (ipmi snoop)                   |  |             | seven segment   |
| |     xyz.openbmc_project.state.       +--+-------------> display         |
| |     HostX(1,2,3..N).Boot.Raw.Value   <--+-+           |                 |
| |                                      |  | |           |                 |
| +-----------------------------+--------+  | |           +-----------------+
|                               |           | |
|                 postcode event|           | |  xyz.openbmc_project.
|                               |           | |  State.HostX(1,2,3..N).
| +-----------------------------|--------+  | |  Boot.Postcode        +-----+
| | +----------------+  +-------v------+ |  | |                       |     |
| | | host discovery |  |              | |  | +----------------------->     |
| | | (d-bus based)  |  |Histroy       | |  |                         | CLI |
| | | with hotplug   |  |   (1,2,3..N) | |  |                         |     |
| | |                |  |              <-+--+------------------------->     |
| | +----------------+  +--------------+ |  |                         |     |
| |                                      |  |                         +-----+
| | Phosphor-post-code-manager           |  |
| +--------------------------------------+  |
+-------------------------------------------+
```

**Postcode Flow:**

 - BMC power-on the Host.
 - Host starts send postcode IPMI message continuously to BMC.
 - The ipmbbridged(phosphor-ipmi-ipmb)  extract the postcode from
   the IPMI message.
 - The ipmbd(phosphor-ipmi-host) append host information and send
   to phosphor-host-postd.
 - phosphor-host-postd displays send  postcode to phosphor-post-code-manager
   as well display postcode in seven segment display.
 - phosphor-post-code-manager store the postcode in directory.

##  Platform Specific OEM Handler (fb-ipmi-oem)

This library is part of  the [phosphor-ipmi-host]
(https://github.com/openbmc/phosphor-host-ipmid) 
and get the postcode  from host through
[phosphor-ipmi-ipmb](https://github.com/openbmc/ipmbbridge).

 - Register IPMI OEM postcode callback interrupt handler.
 - Extract postcode from IPMI message (phosphor-ipm-host/phosphor-ipmi-ipmb).
 - Send extracted postcode to the phosphor-host-postd.

## phosphor-host-postd

**Host discovery**
This feature adds to detect, when the hot plug-able host connected in the slot.
Postcode D-bus interface needs to be created based on host present discovery
(Host state /xyz/openbmc_project/state/hostX(1,2,3.N) D-bus interface).

 - Create, register and add dbus connection for 
    "/xyz/openbmc_project/hostX(1,2,3.N)/state/boot/raw" 
   based on host discovery as mentioned above.
 - Read each hosts postcode from Platform Specific OEM ServicesIPMI OEM handler
   (fb-ipmi-oem, intel-ipmi-oem,etc).
 - Send event to post-code-manager based on which host's postcode received from
   IPMB interface (xyz.openbmc_project.State.HostX.Boot.Raw.Value)
 - Read host position from dbus property.
Display current post-code into the 7 segment display connected to BMC's 8 GPIOs
based on the host position.

 **D-Bus interface**
 - xyz.openbmc_project.State.Host1.Boot.Raw.Value
 - xyz.openbmc_project.State.Host2.Boot.Raw.Value
 - xyz.openbmc_project.State.Host3.Boot.Raw.Value
 - xyz.openbmc_project.State.HostN.Boot.Raw.Value

## phosphor-post-code-manager

phosphor-post-code-manager is the single process based on host discovery for
multi-host. This design shall not affect single host for post-code.

- Create, register and add the dbus connection for
  "xyz.openbmc_project.State.Hostx(1,2,3.N).Boot.PostCode based on the host
  discovery.
- Store/retrieve post-code from directory (/var/lib/phosphor-post-code-manager/
  hostX(1,2,3.N)) based on event received from phosphor-host-postd.

The below D-Bus interface needs to be created for multi-host post-code history.

**D-Bus interface**
 - xyz.openbmc_project.State.Host1.Boot.PostCode
 - xyz.openbmc_project.State.Host2.Boot.PostCode
 - xyz.openbmc_project.State.Host3.Boot.PostCode
 - xyz.openbmc_project.State.HostN.Boot.PostCode

## phosphor-dbus-interfaces

   All multi-host postcode related property and method need to create.

## Alternate design

**phosphor-post-code-manager**
       Change single process into multi-process  on phosphor-post-code-manager.

  **Platform specific service(fb-yv2-misc) alternate to phosphor-host-postd**
      Create and run the platfrom specific process daemon to
      handle IPMI postcode , seven segment  display  and
      host position specific feature.
