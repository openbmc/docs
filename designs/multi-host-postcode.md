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

The below component diagram shows the design for single-host postcode and
history at high-level overview. The single-host design is updated slightly from
the original to better comply with community conventions (using suffix 0 on
D-Bus objects).

Diagram Legend: |Label|Signifies| |-----|---------| |`I:` |D-Bus interface|
|`S:` |D-Bus service name (well-known bus name)| |`R:` |Repository name| |`U:`
|Systemd service unit name| |`A:` |Executable name| |`H:` |HW Module|

```
                                         +-----------+
                       +------+          | 7 segment |
                       | Host |          |  display  |
                       +--+---+          +-----^-----+
                          |                    |
+-------------------------+--------------------+--------+
| BMC                     |                    |        |
|                      +--v----+         +-----+-----+  |
|                      | H:LPC +---------> H:(S)GPIO |  |
|                      +--+----+         +-----------+  |
|                         |                             |
| +-----------------------v--------------+              |
| | U:lpcsnoop                           |              |
| |                                      |              |
| | Current POST code:                   |              |
| | S:xyz.openbmc_project.State.Boot.Raw |              |
| | /xyz/openbmc_project/state/boot/raw0 |              |
| | I:xyz.openbmc_project.State.Boot.Raw >----+         |
| +--------------------------------------+    |         |
|                                             |         |
| +-------------------------------------------v---+     |
| | U:xyz.openbmc_project.State.Boot.PostCode     |     |
| | POST code history:                            |     |
| | +-------------------------------------------+ |     |
| | | S:xyz.openbmc_project.State.Boot.PostCode0| |     |
| | | /xyz/openbmc_project/State/Boot/PostCode0 | |     |
| | | I:xyz.openbmc_project.State.Boot.PostCode >----+  |
| | +-------------------------------------------+ |  |  |
| +-----------------------------------------------+  |  |
|                                                    |  |
|      +--------------------------------+            |  |
|      | Other consumers of POST codes: <------------+  |
|      | Redfish, etc...                |               |
|      +--------------------------------+               |
+-------------------------------------------------------+
```

Since multiple hosts cannot coherently write their POST codes to the same place,
an additional datapath is needed to receive POST codes from each host. Since
this new datapath would not have built-in support in ASPEED hardware, additional
logic is also needed drive the 7-segment display.

## Requirements

- Read postcode from all servers.
- Display the host postcode to the 7 segment display based on host position
  selection.
- Provide a command interface for user to see any server(multi-host) current
  postcode.
- Provide a command interface for user to see any server(multi-host) postcode
  history.
- Support for hot-plug-able host.

## Proposed Design

This document proposes a new design engaging the IPMB interface to read the
port-80 post code from multiple-host. The existing single host LPC snooping
mechanism remains unaffected, and is not shown in diagrams below. This design
also supports host discovery including the hot-plug-able host connected in the
slot.

Following modules will be updated for this implementation

- phosphor-host-postd.
- phosphor-post-code-manager.
- platform specific OEM handler (fb-ipmi-oem).
- bmcweb (redfish logging service).

**Interface Diagram**

Provided below the post code interface diagram with flow sequence

```
+---------------------------------------------------+
| BMC                                               |
|                                                   |
|  +-----------------------+    +----------------+  |         +----+-------------+
|  | A:ipmid               |    | IPMB Bridge    <--+--IPMB-+-|BIC |             |
|  |                       |    | (R:ipmbbridge) |  |       | |    |     Host1   |
|  | +-------------------+ |    +--------+-------+  |       | +------------------+
|  | | OEM IPMI Handlers | |             |          |       |           .
|  | | (R:fb-ipmi-oem)   <-+-------------+          |       |           .
|  | --------------------+ |   +----------------+   |       |           .
|  +-+---------------------+   | Host selection |   |       | +------------------+
|    |                         | monitoring     <---+--+    +-|BIC |             |
|    |                         +---------+------+   |  |      |    |     HostN   |
|    |                                   |          |  |      +------------------+
|    | +---------------------------------v------+   |  |
|    | | R:phosphor-host-postd (1 process)      |   |  |      +---------------+
|    | | Per-host POST code object:             |   |  +-GPIO-| Button/switch |
|    | | /xyz/openbmc_project/state/boot/raw<N> |   |         | input device  |
|    +-> I:xyz.openbmc_project.State.Boot.Raw   >-+ |         +---------------+
|      +---------------------------------+------+ | |
|                                        |        | |         +-------------------+
|                                        +--------+-+----GPIO-> 7-segment display |
|                                                 | |         +-------------------+
|                                                 | +------+
|                                                 |        |
|  +----------------------------------------------v---+    |
|  | R:phosphor-post-code-manager (N processes)       |    |
|  | Per-host POST code history service:              |    |
|  | +----------------------------------------------+ |    |
|  | | S:xyz.openbmc_project.State.Boot.PostCode<N> | |    |
|  | | /xyz/openbmc_project/State/Boot/PostCode<N>  | |    |
|  | | I:xyz.openbmc_project.State.Boot.PostCode    >-+-+  |
|  | +----------------------------------------------+ | |  |
|  |                        ...                       | |  |
|  +--------------------------------------------------+ |  |
|                                                       |  |
|       +--------------------------------+              |  |
|       | Other consumers of POST codes: <--------------+  |
|       | Redfish, CLI, etc...           |                 |
|       +--------------------------------+                 |
|                                                          |
+----------------------------------------------------------+
```

**Postcode Flow:**

- BMC power-on the host.
- Host starts sending the postcode IPMI message continuously to the BMC.
- The ipmbbridged (phosphor-ipmi-ipmb) passes along the message to IPMI daemon.
- The ipmid (phosphor-ipmi-host) appends host information with postcode and
  writes value to appropriate D-Bus object hosted by phosphor-host-postd.
- phosphor-host-postd displays postcode in the seven segment display based on
  host position reads through D-bus interface.
- phosphor-post-code-manager receives new POST codes via D-Bus signal and stores
  the postcode as history in the /var directory.

## Platform Specific OEM Handler (fb-ipmi-oem)

This library is part of the
[phosphor-ipmi-host](https://github.com/openbmc/phosphor-host-ipmid) and gets
the postcode from host through
[phosphor-ipmi-ipmb](https://github.com/openbmc/ipmbbridge).

- Register IPMI OEM postcode callback handler.
- Extract postcode from IPMI message (phosphor-ipmi-host/phosphor-ipmi-ipmb).
- Sets `Value` property on appropriate D-Bus `Raw` object hosted by
  `phosphor-host-postd`. Other programs (e.g. `phosphor-post-code-manager`) can
  subscribe to `PropertiesChanged` signals on this object to get the updates.

## phosphor-host-postd

This implementation involves the following changes in the phosphor-host-postd.

- `phosphor-host-postd` handles property set events for the `Value` property on
  each instance of the `xyz.openbmc_project.State.Boot.Raw` interface.
- phosphor-host-postd reads the host selection from the dbus property.
- Display the latest postcode of the selected host read through D-Bus on a
  7-segment display.

**D-Bus interface**

The following D-Bus names need to be created for the multi-host post-code.

    Service name      -- xyz.openbmc_project.State.Boot.Raw

    Obj path name     -- /xyz/openbmc_project/State/Boot/RawX(1,2..N)

    Interface name    -- xyz.openbmc_project.State.Boot.Raw

## phosphor-post-code-manager

The phosphor-post-code-manager is a multi service design for multi-host. The
single host postcode handling D-bus naming conventions will be updated to comply
with the community naming scheme.

- Create D-Bus service names for single-host and multi-host system accordingly.
  The community follows conventions Host0 for single host and Host1 to N for
  multi-host.
- phosphor-post-code-manager subscribes to changes on the `Raw<N>` object hosted
  by `phosphor-host-postd`.
- Store/retrieve post-code from directory (/var/lib/phosphor-post-code-manager/
  hostX(1,2,3..N)) based on event received from phosphor-host-postd.

**D-Bus interface**

The following D-Bus names needs to be created for multi-host post-code.

    Service name     -- xyz.openbmc_project.State.Boot.PostCodeX(1,2..N)

    Obj path name    -- /xyz/openbmc_project/State/Boot/PostCodeX(1,2..N)

    Interface name   -- xyz.openbmc_project.State.Boot.PostCode

## bmcweb

The postcode history needs to be handled for the multi-host through redfish
logging service.

## Alternate design

**phosphor-post-code-manager single process approach**

This implementation consider single service to handle multi-host postcode. In
this approach, all D-Bus handling will taken care by the single process.

Single service is different than existing x86-power-control and obmc-console
where multi-service approach is used.

Multi-service approach is more scalable to handle multi-host than the single
service.
