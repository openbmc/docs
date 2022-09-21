# Time management on Distributed systems

Author:
  Pavithra B <pavithra.b@ibm.com>

Other contributors:
  Manojkiran Eda <manojeda@in.ibm.com>

Created: 21/09/2022

## Problem Description

Currently phosphor-time-manager [time-manager] service is a wrapper around the
systemd-timesyncd.service. The systemd-timesyncd.service which is available
with systemd as an option configured via systemd-networkd.service.
systemd-timesyncd is a daemon that has been added for synchronizing the
system clock across the network. It implements an SNTP client. In contrast to
NTP implementations such as chrony or the NTP reference server this only
implements a client side, and does not bother with the full NTP complexity,
focusing only on querying time from one remote server and synchronizing the
local clock to it.
The chrony daemon, chronyd [chrony], runs in the background and monitors the
time and status of the time server specified in the chrony.conf file.
When multiple BMCs are considered within a particular system, the time
synchronisation with the current system-timesyncd service is not possible.
The chrony daemon makes sure that time is synchronised between all the BMCs.
Use cases for Chrony that are not covered by systemd-timesyncd:
* running NTP server (so that other hosts can use this host as a source
                      for synchronization);
* getting NTP synchronization information from multiple sources (hosts
                      getting information from public servers)
The goal of this document is to highlight the challenges (and solutions) that
phosphor-time-manager (PTM) will have.

## Background and References

systemd-timesyncd is a system service that may be used to synchronize the local
system clock with a remote Network Time Protocol (NTP) server. It also saves
the local time to disk every time the clock has been synchronized and uses this to
possibly advance the system realtime clock on subsequent reboots to ensure it
monotonically advances even if the system lacks a battery-buffered RTC chip.
The systemd-timesyncd service implements SNTP only. This minimalistic service
will step the system clock for large offsets or slowly adjust it for smaller deltas.
Complex use cases that require full NTP support (and where SNTP is not sufficient) are
not covered by systemd-timesyncd.
Phosphor-time-manager acts as a wrapper around this. The systemd-timesyncd in turn makes
use of systemd-netword.service for configuring the time and network.  Whenever the
systemd-networkd service is restarted the systemd-timesyncd service is also
restarted and the whole of the time-manager gets restarted and the time is set back.
There are 2 modes in which time can be set right now. The method used is the
“TimeSyncMethod”
   - NTP: The time is set via NTP server.
   - MANUAL: The time is set manually.

A summary of which cases the time can be set on BMC or HOST:
Mode    Set BMC Time
NTP     Fail to set
MANUAL  OK

[time-manager] : https://github.com/openbmc/phosphor-time-manager
[Redfish-network-spec]:
           https://redfish.dmtf.org/schemas/ManagerNetworkProtocol.v1_8_1.json
[chrony] : https://chrony.tuxfamily.org/doc/devel/chrony.conf.html
           and https://opensource.com/article/18/12/manage-ntp-chrony

## Requirements
 1. Multiple BMC must sync their time correctly.
 2. No time drift should be present when we configure the NTP servers in each BMC.
 3. In the failover case the new server configuration should be able to sync
    time with all its client servers.

 <few more to be updated>

## Proposed Design
<this section will be updated shortly>

## Alternatives Considered
NA

## Impacts
TBD

### Organizational
- Does this repository require a new repository?
  No
- Which repositories are expected to be modified to execute this design?
  phosphor-time-manager

## Testing
TBD

