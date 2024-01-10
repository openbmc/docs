# Credential bootstrapping account access restriction

Author: Quang Nguyen (quang_nguyen)

Other contributors: Thang Q.Nguyen, Andrew Jeffery (andrew@codeconstruct.com.au)

Created: Jan 5th, 2024

## Problem Description

Credential bootstrapping accounts, known as bootstrap accounts,
[DSP0270](https://www.dmtf.org/dsp/DSP0270), are temporarily created through
IPMI command, enabling the host to establish a permanent account without prior
knowledge of the Redfish interface.

Bootstrap accounts are exclusively granted access to Redfish services via the
in-band host interface, strictly prohibiting out-of-band access. Following
DSP0270 specification, the host interface must support a TCP/IP network
connection, typically facilitated through an ethernet connection. Consequently,
the system should implement a mechanism linking bootstrap accounts to a specific
ethernet device, allowing access solely through the designated connection.

This design addresses the accessibility of bootstrap accounts based on the
ethernet device.

## Background and References

Redfish host interface specification, published by DMTF
[DSP0270](https://www.dmtf.org/dsp/DSP0270) (known as DSP0270).

As detailed in DSP0270, specifically in section six, the Redfish host interface
is designed for use during the pre-boot (firmware) stage, as well as by drivers
and applications within the host operating system. Its architecture prioritizes
accessibility without reliance on external networking.

Furthermore, bootstrap accounts are temporary and exclusively utilized to
initialize permanent accounts. According to DSP0270, section nine, these
bootstrap accounts are designated for use solely on the host interface, within
the host system itself. Therefore, there is no need to employ these accounts for
external access. Consequently, enforcing access restrictions from outside the
system (out-of-band access) is necessary to minimize potential external attacks.

## Requirements

### Bootstrap accounts accessibility

Redfish host interface specification, (https://www.dmtf.org/dsp/DSP0270),
section 9.2 mentioned:

> Bootstrap accounts shall be usable only on the host interface.

To satisfy this requirement, allow access for specific user with a specific
ethernet device should be supported.

### Bootstrap accounts life cycle

DSP0270, section 9.2 specifies the life cycle of bootstrap accounts. All
bootstrap accounts are deleted in the following conditions:

- when Redfish service is reset.
- when host is rebooted.

### This design includes

- A methodology to distinguish bootstrap account with other accounts.
- A methodology to bind a group of users with a specific ethernet interface to
  enable access restriction.
- `entity-manger` schema for host interface.

## Proposed Design

### entity-manager configuration for host interface

A new host interface schema is added to `entity-manager` to configure for host
interface.

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "definitions": {
    "HostInterface": {
      "title": "Host interface configuration",
      "description": [
        "A schema that correlates the host interface",
        " configuration of hosts with ethernet devices."
      ],
      "type": "object",
      "properties": {
        "Name": {
          "type": "string"
        },
        "Interface": {
          "description": [
            "The ethernet interface which is linked to host",
            "interface."
          ],
          "type": "string"
        },
        "Type": {
          "enum": ["HostInterface"]
        }
      }
    }
  }
}
```

Below is a sample of configuration for host interface.

```json
    {
        "Name": host0,
        "Interface": usb0,
        "Type": "HostInterface"
    },
    {
        "Name": host1,
        "Interface": eth2,
        "Type": "HostInterface"
    }
```

This configuration file shall add configured data for host interface of hosts.
In the above example, two hosts, `host0` and `host1` are configured with
ethernet devices `usb0` and `eth2` respectively.

### Two issues need to be addressed to restrict the access of bootstrap account

#### Differentiate a bootstrap account and other accounts.

Adding a user group name `obmc-bootstrap`. All bootstrap accounts when created
are categorized into `obmc-bootstrap` group.

BMCWeb, when receiving a connection request from a user, it shall do bootstrap
account authentication by checking if that user belongs to user group
`obmc-bootstrap`.

#### Recognizing a connection request is from host interface.

Each platform shall have a pre-configured ethernet interfaces and IP addresses
which are dedicated for host interface of hosts. These ethernet interfaces need
to be configured in json configuration file of `entity-manager`.

DSP0270, section nine, specify the implementation of ipmi command to create
bootstrap accounts with IPMI net function 0x2c, group extension identification
0x52. Therefore, user can create a bootstrap account by executing the command
`ipmitool raw 0x2c 0x02 0x52 0xa5`. When that command is executed,
`phosphor-host-ipmid` shall execute dbus call to method `CreateUser` via dbus
interface `xyz.openbmc_project.User.Manager`. Now, `phosphor-user-manager` shall
create and enable a bootstrap account.

BMCWeb , on startup and when receving signals which indicates configurations
change, shall read `entity-manager` host interface configuration and store all
ethernet devices of host interface. It then, queries for IP addresses of those
ethernet devices through dbus service `xyz.openbmc_project.Network` and store
that configuration.

When a connection request is sent to BMCWeb, it shall compares IP addresses of
the connection request and host interface IP address which are already stored;
BMCWeb, now, has enough information to block or grant access for this bootstrap
account.

### Bootstrap accounts life cycle

Bootstrap accounts shall be deleted in the below conditions

#### On the reset of host

When host is in transition to off, `phosphor-state-manager` shall send signal to
`phosphor-user-manager` to delete bootstrap accounts.

#### On the reset of Redfish service

BMCWeb, on the startup, shall send signal to `phosphor-user-manager` to delete
bootstrap accounts.

## Alternatives Considered

- Instead of creating a new `obmc-bootstrap` user group, we can add a dbus
  boolean property `bootstrap` to each user under dbus interface
  `xyz.openbmc_project.User.Attributes`. This method needs to add a new dbus
  property which is unnecessary comparing to leverage the dbus property
  `UserGroups` which is already available.

- Define the host interface in the `meson.build` file. This is not centralized
  management and scalable comparing to configuring in `entity-manager`.

- Considering the device naming of udev for ethernet interfaces may keep
  changing and are initialized in any order, we can use MAC address to
  distinguish host interface connection.

## Impacts

This change will:

- Add a new user group
- Add host interface schema for `entity-manager`
- `phosphor-user-manager` shall be changed to delete all users who belong to a
  specific group

### Organizational

The following repositories are expected to be modified to implement this design:

- "entity-manager"
- "phosphor-host-ipmid"
- "phosphor-user-manager"
- "bmcweb"
- "phosphor-state-manager"

## Testing

### Unit Test

Unit test shall cover the following cases:

- Add configurations for host interface in `entity-manager` configuration file
  and ensure that they are exposed on dbus.
- `phosphor-host-ipmid`, adds test cases to create bootstrap accounts.
- `phosphor-user-manager`, create a bootstrap account and ensure that the
  account has the value `bootstrap` in its `UserGroups` property.
- `bmcweb`, add unit tests for codes which support host interface.

### Integration Test

- Add test case for Robot regression test which uses a bootstrap account to make
  connection requests from (1)out-of-band access, (2)WebUI, (3)in-band via host
  interface and ensure that only access via in-band host interface is allowed.
