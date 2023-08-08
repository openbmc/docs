# Make StaticNTPServers network irrelevant

Author: Lei Yu <LeiYU!>

Created: Aug 08, 2023

## Problem Description

`StaticNTPServers` defined in [EthernetInterface][1] makes all the ethernet
interfaces implement the property and contain the same values. For NTP Servers
change in Redfish, `bmcweb` will have to query all the ethernet interfaces, and
set property for all of them.

The `StaticNTPServers` is considered a static setting related to the time
setting instead of the network setting. Implementing the property in network
service is a bit weird and makes the `bmcweb` complex on settings the property.

## Background and References

In [phosphor-dbus-interfaces' commit][2], `StaticNTPServers` is added to
`EthernetInterface` to hold the static configuration of NTP servers, and use
`NTPServers` to hold all the NTP servers including both DHCP and static NTP
server addresses.

## Requirements

Move `StaticNTPServers` out of `EthernetInterface` and make it a regular
setting. The existing `NTPServers` in `EthernetInterface` still represent all
the NTP servers including both DHCP and static NTP server addresses.

## Proposed Design

Make the `StaticNTPServers` really a static setting for BMC.

### phosphor-dbus-interfaces

Move the `StaticNTPServers` property to the `Time.Synchronization` interface to
represent the static NTP server addresses.

### phosphor-settingsd

Remove `/xyz/openbmc_project/time/sync_method` from the default yaml, and does
not implement `xyz.openbmc_project.Time.Synchronization` interface anymore.

### phosphor-time-manager

Implement the `xyz.openbmc_project.Time.Synchronization` interface and hold the
properties.

Remove the code related to handle callback from `settingsd`, and implement the
properties changed callbacks directly.

When the property `StaticNTPServers` is updated:

- It updates `/etc/systemd/timesyncd.conf.d/ntp.conf` to set the value for
  [NTP][3].
- It restarts the `systemd-timesyncd` service to load the config update. This is
  implemented by `SystemNTPServers` in the `systemd-timesyncd` service.

### phosphor-network-manager

There are several changes in `network-manager` and it simplifies the
implementation.

- It removes the interface of `StaticNTPServers`
- It removes the code related to writing the NTP config on settings this
  property.
- It removes the code related to the reload network on settings this property.
- In `getNTPServerFromTimeSyncd()` it shall merge the `LinkNTPServers` and
  `SystemNTPServers`.

### bmcweb

It could simplify the code on patching NTP Servers, that it does not need to get
all the network interfaces, instead, it simply sets the property to
`/xyz/openbmc_project/time/sync_method`.

## Alternatives Considered

Compared to the current design, this proposal simplifies the
`phosphor-network-manager` and `bmcweb`, and makes the `StaticNTPServers` a real
static setting for BMC.

## Impacts

Below services need changes:

- phosphor-dbus-interfaces
- phosphor-settingsd
- phosphor-time-manager
- phosphor-network-manager
- bmcweb

## Testing

There are no functional changes. The user should be able to set and get NTP
Servers by Redfish command as before.

An additional case shall be tested to verify the expected behavior:

- Enable DHCP on one of the interfaces;
- Verify the read-only property of NTPServers is updated with the dynamically
  provided NTP servers, and the time is getting synced.

[1]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Network/EthernetInterface.interface.yaml#L69
[2]:
  https://github.com/openbmc/phosphor-dbus-interfaces/commit/e11e2faa924243c298ade492a770a01c6179d6d0
[3]: https://www.freedesktop.org/software/systemd/man/timesyncd.conf.html
