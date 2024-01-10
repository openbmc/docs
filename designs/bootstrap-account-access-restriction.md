# Credential bootstrapping account access restriction

Author: Thu Nguyen

Other contributors: Nhat Hieu Le, Thang Q.Nguyen, Andrew Jeffery, Quang Nguyen,
Prithvi Pai,

Created: Mar 28th, 2025

Old design: <https://gerrit.openbmc.org/c/openbmc/docs/+/68746>

## Problem Description

Credential bootstrapping accounts, known as bootstrap accounts,
[DSP0270](https://www.dmtf.org/dsp/DSP0270), "is a temporary account for the
host interface using IPMI commands. This is to allow a host with no initial
knowledge of the Redfish interface to create an initial account from which it
can create a permanent account for the life of the system."

Bootstrap accounts are exclusively granted access to Redfish services via the
in-band host interface, strictly prohibiting out-of-band access. Following
DSP0270 specification, the host interface must support a TCP/IP network
connection, typically facilitated through an ethernet connection. Consequently,
the system should implement a mechanism linking bootstrap accounts to a specific
ethernet device, allowing access solely through the designated connection.

This design addresses the accessibility of bootstrap accounts based on the
ethernet device.

## Requirements

Redfish host interface specification, published by DMTF
[DSP0270](https://www.dmtf.org/dsp/DSP0270) (known as DSP0270).

As detailed in DSP0270, specifically in section 6, the Redfish host interface is
designed for use during the pre-boot (firmware) stage, as well as by drivers and
applications within the host operating system. Its architecture prioritizes
accessibility without reliance on external networking.

According to DSP0270, section 9, these bootstrap accounts are designated for use
solely on the host interface, within the host system itself. Therefore, there is
no need to employ these accounts for external access. Consequently, enforcing
access restrictions from outside the system (out-of-band access) is necessary to
minimize potential external attacks.

### Host interface

Redfish host interface specification(<https://www.dmtf.org/dsp/DSP0270>),
section 9 mentioned:

> Services that support credential bootstrapping via IPMI commands shall:
>
> - Implement the CredentialBootstrapping property defined in the HostInterface
>   schema.
> - Implement the CredentialBootstrappingRole property in the Links property
>   defined in the HostInterface schema.

In OpenBMC, the `Services` in this case is `phosphor-host-ipmid`.

As
[Redfish schema](https://github.com/openbmc/bmcweb/blob/4491419217d426791003facc749f600692d9b2a6/redfish-core/schema/dmtf/json-schema/HostInterface.v1_3_3.json),
`CredentialBootstrapping` interface includes three properties
`EnabledAfterReset`, `Enabled`, `RoleId`. And `CredentialBootstrappingRole` is
string property in `Link` interface.

To meet this requirement, the `HostInterface.CredentialBootstrapping` D-Bus
interface should be added to
[phosphor-dbus-interface](https://github.com/openbmc/phosphor-dbus-interfaces).

### Create Bootstrap account and IPMI commands

Section 9.1 in Redfish host interface specification, The service shall implement
the IPMI commands:

- Get Manager Certificate Fingerprint (NetFn 2Ch, Command 01h)
- Get Bootstrap Account Credentials (NetFn 2Ch, Command 02h)

`phosphor-host-ipmid` will be updated to:

- Handle these two IPMI commands to create bootstrap accounts
- Update the `Enabled` and `RoleId` property in
  `HostInterface.CredentialBootstrapping` D-Bus interface which can be hosted by
  phosphor-user-manager.

### Bootstrap accounts accessibility

[Redfish host interface specification](https://www.dmtf.org/dsp/DSP0270),
section 9.2 mentioned:

> Bootstrap accounts shall be usable only on the host interface.

To satisfy this requirement, allow access the redfish interface for specific
user with a specific Ethernet device should be supported.

### Bootstrap accounts life cycle

DSP0270, section 9.2 specifies the life cycle of bootstrap accounts. All
bootstrap accounts are deleted in the following conditions:

- when Redfish service is reset.
- when host is reset.

## This design includes

- A methodology to create bootstrap accounts
- A methodology to configure the ethernet interface for bootstrap account.
- A methodology to distinguish bootstrap account with other accounts.

## Background knowlegde

### Phosphor-dbus-interfaces

`phosphor-dbus-interfaces` include YAML descriptors of standard D-Bus
interfaces.

`User.Attributes` interface includes below properties.

- UserPrivilege - Privilege of the user.
- UserGroups - Groups to which the user belongs.
- UserEnabled - User enabled state.
- UserLockedForFailedAttempt - Locked or unlocked state of the user account.
- `User.Attributes` does not has the info to different the `bootStrap` account
  with other account.

`phosphor-dbus-interfaces` does not support
`HostInterface.CredentialBootstrapping` interface.

### phosphor-user-manager

The `phosphor-user-manager` hosts the users object paths at the
`/xyz/openbmc_project/user/<username>` path of
`xyz.openbmc_project.User.Manager` service. Each user object path hosts
`xyz.openbmc_project.User.Attributes` interface with: User-manager has a
hardcoded list of
[user groups](https://github.com/openbmc/docs/blob/master/architecture/user-management.md#supported-group-roles)
(`ssh`, `ipmi`, `redfish`, `hostconsole`) and a hardcoded
[list of privileges](https://github.com/openbmc/docs/blob/master/architecture/user-management.md#supported-privilege-roles)
("priv-admin", "priv-operator", "priv-user", "priv-noaccess"). The user only has
`redfish` group can't response for `ipmi`, `hostconsole` and `ssh`.

### phosphor-host-ipmid

`phosphor-host-ipmid` handles the in-band and out-of-band IPMI requests from
host and from user. The `user_layer` class in `phosphor-host-ipmid` implements
the APIs to create the user accounts.

`ipmiUserSetPrivilegeAccess`
(<https://github.com/openbmc/phosphor-host-ipmid/blob/8c974f76411cb40f2c9c25cddd86814087d0eddf/user_channel/user_layer.cpp#L161>)
does not support set the `UserGroups` for specific user. The newly added user
will always has `UserGroups` value as all possible groups which is defined in
the value of `AllGroups` property in `xyz.openbmc_project.User.Manager`
interface. This interface is hosted by phosphor-user-manager. In current
implement of Phosphor-user-manager all possible groups are {`ssh`, `ipmi`,
`redfish`, `hostconsole`}.

### BMCWeb and Redfish

BMCWeb is an OpenBMC Daemon which implements the Redfish service. It supports
two main authentication methods `PAM based` (use Linux-PAM to do
username/password style of authentication) and `TLS based` (use the Public Key
infrastructure). The PAM-based method uses username/password or token in Redfish
request to authenticate. So BMCWeb will respond for the Redfish requests
regardless the physical interface which the request come from.

Redfish requests include the `IPAddress`. It can be matched with the `Address`
D-Bus property in `xyz.openbmc_project.Network.IP` interface of the
`/xyz/openbmc_project/network/<InterfaceName>/<dhcptype>` network objects in
`xyz.openbmc_project.Network` in service to find `InterfaceName`. The
`InterfaceName` is specific in each platform and can be used to identify the
Redfish host interface.

`BMCWeb` can get the User info of one `username` uses the user properties in
`User.Attributes` interface of the `/xyz/openbmc_project/user/<username>` path
of `xyz.openbmc_project.User.Manager` service.

By default, BMCWeb is support two security postures:

- HTTPS + Redirect on both ports 443 and port 80 if http-redirect is enabled And
- HTTPS only if http-redirect is disabled

Commit [Enable HTTP additional sockets]
(<https://github.com/openbmc/bmcweb/commit/796ba93b48f7866630a3c031fd6b01c118202953>)
allows administrators to support essentially any security posture by using the
build options:

- additional-ports Adds additional ports that bmcweb should listen to. This is
  always required when adding new ports.
- additional-bind-to-device Accepts values that fill the SO_BINDTODEVICE flag in
  systemd/linux, and allows binding to a specific device

## Proposed Design

### D-Bus Host interface

Because the specification requires "Implement the `CredentialBootstrapping`
property defined in the `HostInterface` schema", the
`HostInterface.CredentialBootstrapping` interface will be added to
phosphor-dbus-interfaces. This interface will be hosted by
`xyz.openbmc_project.User.Manager` service at `/xyz/openbmc_project/user` object
path. The `xyz.openbmc_project.User.Manager` is implemented in
`phosphor-user-manager`.

The interface will include the properties:

- `EnabledAfterReset`: An indication of whether credential bootstrapping is
  enabled after a reset for this interface. This property is read-only. The
  default value of this property will be `true`.
- `Enabled`: An indication of whether credential bootstrapping is enabled for
  this interface. The value type is boolean. The default value of `Enabled` will
  be `true`. When `Enabled` is set to `false`, it can't be changed untill the
  restart of `xyz.openbmc_project.User.Manager` service.
- `RoleId`: The role used for the bootstrap account created for this interface.

### IPMI in-band commands to create bootstrap accounts

Which IPMI in-band interface and how IPMI handle below IPMI command is out of
the scope of this spec. There are two IPMI commands relate creating BootStrap
accounts.

#### Get Manager Certificate Fingerprint (NetFn 2Ch, Command 01h)

This command is used to get the fingerprint for the manager's TLS certificate
for the host interface.

> BMC will only handle this command when `EnabledAfterReset` property and
> `Enabled` property values in `HostInterface.CredentialBootstrapping` interface
> are `true`. BMC will respond "Fingerprint of the manager's TLS certificate"
> for this command.

#### Get Bootstrap Account Credentials (NetFn 2Ch, Command 02h)

This command is used to get the user name and password of the bootstrap account.

> BMC will only handle this command when `EnabledAfterReset` property and
> `Enabled` property values in `HostInterface.CredentialBootstrapping` interface
> are `true`. BMC will respond the user name and password from BMC shall:

- Be valid UTF-8 strings consisting of printable characters.
- Not include single quote (') or double quote (") characters.
- Not include backslash (\) characters.
- Not include whitespace characters.
- Not include control characters.
- Be at most 16 characters long.

After responding for the command, The `Enabled` property within the
`HostInterface.CredentialBootstrapping` D-Bus interface shall be set to `true`
when the "Disable credential bootstrapping control" value is `A5h`. Otherwise
`Enabled` will be `false`.

### Restrict the access of bootstrap account to Redfish host interface

Because the `BMCWeb` which implements the Redfish service can get the
`UserGroups` from `User.Attributes` interface. It can identify if the one
username which in the `redfish` userGroups should be allowed to authenticate for
the Redfish request. This restrict should already implemented by `BMCWeb`.

So the remain problem to `Restrict the access of bootstrap account` are

- Support add the user with only `redfish` group in `UserGroups` using
  `phosphor-host-ipmid`
- How identify if one user account is bootStrap account
- How identify the Redfish request is from `Redfish host interface`

#### Support add the user with only `redfish` group

Add `ipmiUserSetGroupAccess` API in `user_layer` of `phosphor-host-ipmid` to
allow set the groups data for one userId. This API should be called to set the
`UserGroups` to `redfish` for the bootStrap account.

```text
/** @brief sets user group access data
 *
 *  @param[in] userId - user id
 *  @param[in] chNum - channel number
 *  @param[in] groups - groups data
 *
 *  @return ccSuccess for success, others for failure.
 */
Cc ipmiUserSetGroupAccess(const uint8_t userId, const uint8_t chNum,
                              const GroupAccess& groupAccess);
```

#### Differentiate a bootstrap account and other accounts

The `User.Attributes` interface is already included the User Info of one
`username`, adding one more property to `User.Attributes` to identify the
bootStrap account is reasonable.

The proposed property name is `IsBootStrap` with below description.

```text
    - name: IsBootStrap
      type: bool
      default: false
      flags:
          - const
      description: >
          true for bootstrap users.
      description: >
          Indicates the bootstrap account.
```

This property will be `true` for the username is BootStrap account.

Add `ipmiUserSetBootStrapAccount` API in `user_layer` of `phosphor-host-ipmid`
to create one user with the true `IsBootStrap` flag.

```text
/** @brief sets bootStrap account flag
 *
 *  @param[in] userId - user id
 *  @param[in] chNum - channel number
 *  @param[in] isBootStrap - identify if account is bootStrap
 *
 *  @return ccSuccess for success, others for failure.
 */
Cc ipmiUserSetBootStrapAccount(const uint8_t userId, const uint8_t chNum,
                              const bool isBootStrap);
```

#### Recognizing a redfish request is from Redfish host interface

Because the interface names for `Redfish host interface` are specific for one
OxM platform, so these name can be configured in buid time. The
`redfish-host-interfaces` build option will be added to `BMCWeb` to configure
the interface name of `Redfish host interfaces`.

```text
option(
    'redfish-host-interfaces',
    type: 'array',
    value: [],
    description: '''The name of Redfish host interfaces in array of strings.''',
)
```

With two proposed solutions, `BMCWeb` can restrict the access of bootstrap
account to ony redfish request on specific the configured interface names.

#### Bind the Redfish host interface to specific device and port

Use the existing `BMCWeb` options additional-ports and
additional-bind-to-device(<https://github.com/openbmc/bmcweb/commit/796ba93b48f7866630a3c031fd6b01c118202953>)
to different the out-of-band redfish requests which will come from port 443 or
80 with the inband redfish request. Example: Add `additional-ports=199` and
`additional-bind-to-device=usb0` to specify that the request to interface `usb0`
should using port 199. This will be additional constraints to the
`redfish-host-interfaces` above option.

#### Flow diagram

```text
@startuml
participant HOST as Host
participant IPMI as Ipmi
participant BMCWeb as Web
participant "User Managers" as User

Web -> Web: +Option "redfish-host-interfaces" to "RHI list"\n+Option "additional-bind-to-device" is "RHI list"\n+Option "additional-ports" is [ports]
User -> User: Set "Enabled" property of CredentialBootstrapping Dbus Interface to True
Host -> Ipmi: [Step 1] Get Boot Strap Account
Ipmi -> Ipmi: "Enabled" property of\nCredentialBootstrapping Dbus Interface is "True"?
alt "Enabled" is True
Ipmi -> Ipmi: Create BootStrap account &\n random password
Ipmi -> User: [Step 2] Create BootStrap account D-Bus Object
User -> User: Update "Enabled" base on\n "Get Boot Strap Account" request
User -> User: Create user object with\n+"IsBootStrap" to True\n+"UserGroups" is ["Redfish"]\n+"UserPrivilege" is "Priv-admin"
Ipmi -> Ipmi: Created the UserName D-Bus objected
alt User D-Bus object Created
Ipmi -> Host: [Step 3] Account Name + Password
else Failed to create the UserName D-Bus objected
Ipmi -> Host: [Step 4] Report failure
end
else "Enabled" is False
Ipmi -> Host: [Step 5] "Credential bootstrapping via IPMI commands is disabled"
end

Host -> Web: [Step 6] Redfish requests
Web -> Web: + Redfish request to UserName\n+ Redfish request to IP Address
Web -> Web: "IsBootStrap" property\nof "xyz.openbmc_project.User.Attributes" interface\nof "/xyz/openbmc_project/user/<UserName>" object is True?
alt "IsBootStrap" is No
Web -> Web: Go to current BMC Web flow to authen and respond Refish request
Web -> Host: [Step 7] Redfish Responds
else "IsBootStrap" is Yes
Web -> Web: IP Address to Interface use xyz.openbmc-project.Network interface
Web -> Web: Interface belong to "RHI list"?
alt Is NOT RHI
Web -> Host: [Step 8] Invalid interface
else Is RFI
Web -> Web: + Interface belong to RHI Devices?
alt Is not RFI Device
Web -> Host: [Step 9] Invalid interface
else Is RFI Device
Web -> Web: + Port in request belong to RHI ports?
alt Is Not RHI Port
Web -> Host: [Step 10] Invalid port
else Is RHI Port
Web -> Web: Go to current BMC Web flow to authen and respond Refish request
Web -> Host: [Step 11] Redfish host interface response
end
end
end
end
@enduml
```

### Bootstrap accounts life cycle

Bootstrap accounts shall be deleted in the below conditions:

- On the reset of host
- On the reset of Redfish service

If the `EnableAfterReset` property within the
`HostInterface.CredentialBootstrapping` D-Bus interface of is `true`, the
`Enabled` property will be set to `true` after reset. Otherwise, the `Enabled`
property will be set to `false` and can't changeable.

#### On the reset of host

The change state of `CurrentHostState` property in
`xyz.openbmc_project.State.Host` interface of `/xyz/openbmc_project/state/host0`
object path from the `xyz.openbmc_project.State.Host.HostState.Running` to other
state will be used to identify the reset of the host. `phosphor-user-manager` is
required to register the propertyChanged signal of property
CurrentHostState`property in`xyz.openbmc_project.State.Host ` interface.

#### On the reset of Redfish service

`phosphor-user-manager` service shall watch for the job state change of `BMCWeb`
to ensure that when `BMCWeb` starts up, bootstap accounts shall be deleted.

## Impacts

This change will:

- Add a one more property `IsBootStrap` to `User.Attributes` D-Bus interface of
  `phosphor-dbus-interfaces`.
- Add `redfish-host-interfaces` option to `BMCWeb` to configure the redfish host
  interface names.
- Add two APIs `ipmiUserSetGroupAccess` and `ipmiUserSetBootStrapAccount` in
  `phosphor-host-ipmid`.

### Organizational

The following repositories are expected to be modified to implement this design:

- `phosphor-dbus-interfaces`
- `phosphor-host-ipmid`
- `phosphor-user-manager`
- `bmcweb`

## Testing

### Test 1: Bootstrap account only work on the Redfish Host Inband interface

- Set the `redfish-host-interfaces` build option in `BMCWeb` with specific
  redfish host inband interface names, such as {`usb0`}.
- Use "Get Bootstrap Account Credentials" IPMI command to get bootstrap account.
- Check the bootstrap account in `/xyz/openbmc_project/user/<accountName>`.
  Value of property `IsBootStrap` of `User.Attributes` D-Bus interface should be
  `true`. Value of `UserGroups` of `User.Attributes` D-Bus interface should only
  `redfish`.
- Check the IP Address of BMC address in `usb0`. BMC will only respond `success`
  for the in-band redfish request with the IP of `usb0` interface and bootstrap
  accounts. BMC will respond `error` for the out-of-band redfish, IPMI and WebUI
  requests with bootstrap accounts. BMC will respond `error` for the ssh
  requests with bootstrap accounts.

### Test 2: Misc testing

With BMCWeb build option `redfish-host-interfaces=usb0`,
`additional-ports=[979]` and `additional-bind-to-device=[usb0]` Bootstrap
account is `openbmcRedfish/Openbmc@2745` BMC should respond error for below
cases:

- Inband or out-of-band IPMI requests + BootStrap account
- Out-of-band Redfish request + BootStrap account
- Inband redfish request + None bootStrap account
- Inband redfish request + bootStrap account + port is not 979
- Inband redfish request(usb2 for multiple inband interface ) + bootStrap
  account + port is 979 BMC should respond success for this case:
- Inband redfish request at `usb0` + bootStrap account + port is 979
