# Multi-host IPMI design

Author: Velumani T(velu), [velumanit@hcl](mailto:velumanit@hcl.com) Kumar
T(kumar_t), [thangavel.k@hcl.com](mailto:thangavel.k@hcl.com)

Other contributors:

Created: June 26, 2020

## Problem Description

The current version of OpenBMC does not support multi-host implementation in
IPMI commands handling. We have a multi-host system and proposing the design to
support multi-host.

As detailed below the hosts are connected in the IPMB interface, all host
related communication is based on IPMB. The OpenBMC uses ipmbbridged to manage
IPMB buses and the IPMB messages are routed to ipmid.

Issue 1: ipmbridged does not send the channel number (ie HostId) Issue 2: ipmid
does not have the information on which IPMB channel the request has come from.
The ipmid handlers should have the host details to fetch the host specific
responses.

## Background and References

IPMI and IPMB System architecture:

```
 +------------------------------------+
 |               BMC                  |
 | +-----------+       +------------+ |      +--------+
 | |           |       |            | | IPMB1|        |
 | |           |       |            |-|------| Host-1 |
 | |           |       |            | |      |        |
 | |           |       |            | |      +--------+
 | |           |       |            | |
 | |           |       |            | |
 | |           | D-Bus |            | |      +--------+
 | | ipmid     |-------| ipmbbridged| | IPMB2|        |
 | |           |       |            |-|------| Host-2 |
 | |           |       |            | |      |        |
 | |           |       |            | |      +--------+
 | |           |       |            | |
 | |           |       |            | |
 | |           |       |            | |      +--------+
 | |           |       |            | | IPMBn|        |
 | |           |       |            |-|------| Host-N |
 | |           |       |            | |      |        |
 | +-----------+       +------------+ |      +--------+
 +------------------------------------+
```

Hosts are connected with IPMB interface, the hosts can be 1 to N. The IPMB
request coming from the hosts are routed to ipmid by the ipmbbridged. The IPMB
requests are routed from ipmid or any service by D-Bus interface and The
outgoing IPMB requests are routed by ipmbbridged to IPMB interface.

## Requirements

The current version of OpenBMC does not support multi-host implementation in
IPMI commands handling. We have a multi-host system and proposing the design to
support multi-host.

## Proposed Design

To address issue1 and issue2, we propose the following design changes in
ipmbbridged and ipmid. To address out-of-band IPMI command from the network,the
proposal is captured in section "Changes in netipmid".

## Issue1: Changes in ipmbbridged:

ipmbbridged to send the channel details from where the request is received.

**Change: Sending Host detail as additional parameter**

While routing the IPMB requests coming from the host channel, We will be adding
new entry in the json config file for the host ID '"devIndex": 0' ipmbbridged
will send '"devIndex": 0' as optional parameter(options) in D-Bus interface to
ipmid.This can be used to get the information on which IPMB bus the message
comes from.

The json file looks like below. Each devIndex can have one "me" and "ipmb"
channel.To ensure existing platforms does not get affected, if the "devIndex"
key is not present in the file ipmbbridged uses default "devIndex" as 0.

{ "type": "me", "slave-path": "/dev/ipmb-1", "bmc-addr": 32, "remote-addr": 64,
"devIndex": 0 }, { "type": "ipmb", "slave-path": "/dev/ipmb-2", "bmc-addr": 32,
"remote-addr": 64, "devIndex": 0 }, { "type": "me", "slave-path": "/dev/ipmb-3",
"bmc-addr": 32, "remote-addr": 64, "devIndex": 1 }, { "type": "ipmb",
"slave-path": "/dev/ipmb-4", "bmc-addr": 32, "remote-addr": 64, "devIndex": 1 },

## Issue2: Changes in ipmid:

Receive the optional parameter sent by the ipmbbridged as host details, while
receiving the parameter in the executionEntry method call the same will be
passed to the command handlers in both common and oem handlers.The command
handlers can make use of the host information to fetch host specific data.

For example, host1 send a request to get boot order from BMC, BMC maintains data
separately for each host. When this command comes to ipmid the commands handler
gets the host in which the command received. The handler will fetch host1 boot
order details and respond from the command handler. This is applicable for both
common and oem handlers.

## Changes in netipmid:

The "options" parameter can be used for sending the host information from
netipmid. The changes proposed for ipmbbridged can be used in netipmid as well.
The netipmid sends the "devIndex" on which channel the request comes from. There
will not be any further changes required in ipmid. The netipmid can have
multiple approaches to handle multi-host.Some of the approaches are listed down
and but not limited to this list.

1. Virtual Ethernet interfaces - One virtual interface per host.
2. Different port numbers - Can have different port numbers for each host.
3. VLAN Ids- VLAN IDs can be used to support multi host. The netipmid shall have
   a config file where in the interfaces can be configured for each host. Also
   one or more interfaces can be configured to each of the host. The interfaces
   can be virtual or physical. Below is the sample configuration file

{"Host":1, "Interface-1":"eth0", "Interface-2":"eth1", "Interface-3":"veth4",
"Interface-4":"veth5" }, {"Host":2, "Interface-1":"eth2", "Interface-2":"eth3",
"Interface-3":"veth1", "Interface-4":"veth2" },

Example implementation of approach1:Virtual ethernet interface.

```
+--------------------------------------------+
|           BMC                              |
| +--------+       +-----------+   +------+  |      +--------+
| |        | D-Bus |           |   |      |  |      |        |
| |        |-------| netipmid1 |---|veth1 |---------| Host-1 |
| |        |       |           |   |      |  |      |        |
| |        |       +-----------+   +------+  |      +--------+
| |        |                                 |
| |        |       +-----------+   +------+  |      +--------+
| | ipmid  | D-Bus |           |   |      |  |      |        |
| |        |-------| netipmid2 |---|veth2 |---------| Host-2 |
| |        |       |           |   |      |  |      |        |
| |        |       +-----------+   +------+  |      +--------+
| |        |                                 |
| |        |       +-----------+   +------+  |      +--------+
| |        | D-Bus |           |   |      |  |      |        |
| |        |-------| netipmidN |---|vethN |---------| Host-N |
| |        |       |           |   |      |  |      |        |
| +--------+       +-----------+   +------+  |      +--------+
+--------------------------------------------+
```

In the above diagram one instance of netipmid runs per host. Each instance is
tied to one virtual ethernet interface, The virtual interface ID can be used to
make a devIndex. This represents the HostId.

## Alternatives Considered

## Approach1:ipmbbridged to send host-id in the payload

The ipmbbridged shall be modified to send the host id in data payload. This
looks to be a simple change but impacts the existing platforms which are already
using it.This may not be a right approach.

## Approach2:Create multiple ipmid to handle multihost.One ipmid process per host.

This is a multi service appoach,one instance of ipmid service shall be spawned
to respond each host.The changes looks simple and no major design change from
the existing design. But many common handlers will be running as duplicate in
multiple instances.

## Approach3:Using a different IPMI channel for handling multiple host.

Using a different IPMI channel for handling multiple hosts, in the ipmbbridged
the channel id can be used to identify host. In this approach we will be having
multiple instances of ipmbbridged and each instance will be registered with the
a channel number.Maximum channel numbers are limited to 8 as per the
specification.This limits the maximum hosts to be supported.

## Impacts

There may not be an impact in ipmid command handler functions.This design will
not affect the current functionality.

## Testing

The proposed design can be tested in a platform in which the multiple hosts are
connected.The commands requests are received from all the hosts and responses
are host specific data.When the request coming from the host as IPMB command,
ipmbbridged appends devIndex and ipmid receives the respective devIndex.ipmid
responds based on the received devIndex(Host Id) and response reaches all the
way to host.The data can be validated in host.
