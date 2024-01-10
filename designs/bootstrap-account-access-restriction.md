# Credential bootstrapping account access restriction

Author: Quang Nguyen (quang_nguyen)

Other contributors: Thang Q.Nguyen

Created: Jan 5th, 2024

## Problem Description

Credential bootstrapping accounts (bootstrap accounts) are created as a
temporary account via IPMI commands which enables host to create a permanent
account with no initial knowledge about Redfish interface.

Bootstrap accounts are only allowed to access to Redfish services through host
interface which is in-band access and are restricted to out-of-band access. It
means that system should have a mechanism to bind bootstrap accounts to a
specific ethernet device, and only allow access through that ethernet device.

This design aims to solve bootstrap accounts accessibility based on ethernet
device.

## Background and References

The nature of the bootstrap accounts is that they are temporary accounts; and
they are used to initialize permanent accounts only. The requesters who use
bootstrap accounts are inside the host system; there is no need to use those
accounts for accessing from outside of the host system. Therefore, access
restriction from outside of the system (out-of-band access) is necessary and
also to reduce potential attacks from outside.

Redfish host interface specification, published by DMTF
[DSP0270](https://www.dmtf.org/dsp/DSP0270).

## Requirements

Redfish host interface specification, (https://www.dmtf.org/dsp/DSP0270),
section 9.2 mentioned
`Bootstrap accounts shall be usable only on the host interface`. To satisfy this
requirement, allow access for specific user with a specific ethernet device
should be supported.

This design includes:

- A methodology to distinguish bootstrap account with other accounts.
- A methodology to bind a group of users with a specific ethernet interface to
  enable access restriction.
- A dbus property `UserConfigInfo` of interface
  `xyz.openbmc_project.User.Attributes`.
- A configuration file for `phosphor-user-manager` to configure user data for
  each user group.

## Proposed Design

To restrict the access of bootstrap account, there are two issues need to be
addressed:

1. Differentiate a bootstrap account and other accounts.

**Setup time**

Adding a user group name `bootstrap`. All bootstrap accounts when created are
categorized into `bootstrap` group.

**Run time**

BMCWeb, when receiving a connection request from a user, it shall do bootstrap
account authentication by checking the the dbus property `UserGroups` of the
dbus interface `xyz.openbmc_project.User.Attributes`, to know if the user of
current connection request is a bootstrap account user.

2. Recognizing a connection request is from host interface.

**Setup time**

Each platform shall have a pre-configured ethernet interface and an IP address
which is dedicated for host interface. This ethernet interface needs to be
configured in a json configuration file `group_config.json`. When
`phosphor-user-manager` create a bootstrap account, it shall update the ethernet
interface name of the host interface to the dbus property `UserConfigInfo`.

**Run time**

When a connection request is sent to BMCWeb, BMCWeb shall read the dbus property
`UserConfigInfo` to know the host interface. It then uses this interface name to
query for IP address through dbus service `xyz.openbmc_project.Network`. Now,
comparing IP addresses of the connection request and host interface IP address,
BMCWeb has enough information to block or grant access for this bootstrap
account.

3. The `group_config.json` configuration file is formatted as below

```json
{
    "groups":
    [
        {
            "group": bootstrap,
            "HostInterface": usb0,
        },
    ]
}
```

This configuration file shall add common configured data for users within a
group. In the above example, the `HostInterface` data `usb0` shall be used to
update dbus property `UserConfigInfo`.

## Alternatives Considered

- Instead of creating a new `bootstrap` user group, we can add a dbus boolean
  property `bootstrap` to each user under dbus interface
  `xyz.openbmc_project.User.Attributes`. When a bootstrap account is created,
  this property will be set to `true`, otherwise set to `false`. BMCWeb shall
  also check for this property to distinguish bootstrap accounts. But
  considering it is not necessary to add too many new dbus properties, adding
  new group is proposed since it leverages the dbus property `UserGroups`.

- Define the host interface in the `meson.build` file. This is a simpler
  solution and quite straghtforward. But take into account that more configured
  data needs to be added to a user group for other use cases in the future, so
  adding configured file is proposed instead of this solution. User needs to add
  a bitbake append file (.bbappend) to modify host interface configuration for
  each platform before using this feature. `bitbake` append file configures
  ethernet host interface as below:

```
EXTRA_OEMESON:append = "-Dbootstrap-interface=usb0"
```

- Users provide the host interface information when a bootstrap account is
  created to update the dbus property `UserConfigInfo`. For this approach, users
  have no knowledge about the host interface, even they know it, this approach
  is not user-friendly.

## Impacts

This change will:

- Add a new user group.
- Add a new property `UserConfigInfo` to the dbus interface
  `xyz.openbmc_project.User.Attributes`
- Add a configuration file to service `xyz.openbmc_project.User.Manager`

### Organizational

The following repositories are expected to be modified to implement this design:

- `https://github.com/openbmc/bmcweb`
- `https://github.com/openbmc/phosphor-dbus-interfaces`
- `https://github.com/openbmc/phosphor-user-manager`

## Testing

### Unit Test

Unit test shall cover the following cases:

- Create a bootstrap account and ensure that the account has the value bootstrap
  in its `UserGroups` property.
- Ensure that the new property `UserConfigInfo` is added to dbus interface
  `xyz.openbmc_project.User.Attributes`.

### Integration Test

- Use bootstrap account to make connection requests from (1)out-of-band access,
  (2)WebUI, (3)in-band via host interface and ensure that only access via
  in-band host interface is allowed.
