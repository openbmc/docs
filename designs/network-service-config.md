# Network Service Configuration Daemon

Author:
  Ryon Heichelbech <ryonh@ami.com>

Primary assignee:
  Ryon Heichelbech <ryonh@ami.com>

Other contributors:
  Aashray B <aashrayb@ami.com>

Created:
  September 8, 2021

## Problem Description
A background daemon is required to handle configuration of network services. It
must be able to set TCP/UDP listen port and enable/disable services in a way
that may need special handling on a per-service basis.

## Background and References
This design primarily leverages existing functionality in systemd and is
intended to manage unit contents and state.

## Requirements
The daemon must expose a consistent D-Bus API to abstract settings for multiple
different network services (such as bmcweb/HTTPS, SSH, and SOL SSH). This
design is to be compatible with bmcweb's D-Bus abstraction layer and will allow
system administrators to configure network services running on the BMC. These
settings must be able to persist across reboots and (optionally) firmware
upgrades.


## Proposed Design
The daemon is to manage a set of objects using a NetworkService interface,
which primarily exposes an `Enabled` boolean property and a `Port` uint16\_t
property. The configuration daemon is responsible for creating and maintaining
override systemd unit files with the following scenarios:

 - Socket-activated daemons: Override the ListenStream and ListenDatagram
   options in the socket unit file to change port. Mask socket and
   corresponding service to enable/disable (eg: SSH, SOL SSH)
 - Normal daemons: Provide a `LISTEN_PORT` environment variable in the service
   unit file, which the corresponding daemon is then patched to read when
   binding to a port. Mask to enable/disable (eg: bmcweb, obmc-ikvm)

The following objects implementing the NetworkService interface would be
managed by the daemon:

 - `Web`: bmcweb/https (default port: 443)
 - `SSH`: dropbear ssh daemon (default port: 22)
 - `SOL`: sol over ssh daemon (default port: 2200)
 - `KVM`: openbmc ikvm/vnc daemon (default port: 5900)

The current configuration of these objects at any given time is serialized and preserved as json in `/etc/service-settings/service-settings.json`.

busctl examples for `Web` (https):

 - Disable:
   `busctl set-property xyz.openbmc_project.Service /xyz/openbmc_project/service/Web xyz.openbmc_project.Service.NetworkService Enabled b false`
 - Re-enable:
   `busctl set-property xyz.openbmc_project.Service /xyz/openbmc_project/service/Web xyz.openbmc_project.Service.NetworkService Enabled b true`
 - Set port number (443->444):
   `busctl set-property xyz.openbmc_project.Service /xyz/openbmc_project/service/Web xyz.openbmc_project.Service.NetworkService Port q 444`

## Alternatives Considered
### Alternative: Read from JSON to determine port
This was considered but not implemented due to the portability concerns for
JSON parsers (what if a network service was written in C?) nlohmann JSON works
for C++, but other languages can cause complications.

### Alternative: Read from D-Bus to determine port
This was considered but not implemented due to increasing implementation
complexity for services not already connected to D-Bus.

Additionally, both of these solutions fail to offer an alternative for
socket-activated daemons, which will always have a hard-coded port in the
socket file.

## Impacts
The admin could potentially override the port of another service, which can
inhibit the functionality of other services on the system (eg: if SSH was set
to 443, bmcweb would not be able to start properly).

## Testing
This is tested by using busctl to manipulate the properties and verify that the
corresponding network services respond appropriately.
