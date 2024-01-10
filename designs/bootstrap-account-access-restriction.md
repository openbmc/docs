# Credential bootstrapping account access restriction

Author: Thu Nguyen

Other contributors: Nhat Hieu Le, Thang Q.Nguyen, Andrew Jeffery, Quang Nguyen,
Prithvi Pai,

Created: Mar 28th, 2025

Old design: <https://gerrit.openbmc.org/c/openbmc/docs/+/68746>

## Problem Description

Credential bootstrapping accounts, known as bootStrap accounts,
[DSP0270](https://www.dmtf.org/dsp/DSP0270), "is a temporary account for the
host interface using IPMI commands. This is to allow a host with no initial
knowledge of the Redfish interface to create an initial account from which it
can create a permanent account for the life of the system."

Bootstrap accounts are exclusively granted access to Redfish services via the
in-band host interface, strictly prohibiting out-of-band access. Following
DSP0270 specification, the host interface must support a TCP/IP network
connection, typically facilitated through an ethernet connection. Consequently,
the system should implement a mechanism linking bootStrap accounts to a specific
ethernet device, allowing access solely through the designated connection.

This design addresses the accessibility of bootStrap accounts based on the
ethernet device.

## Requirements

Redfish host interface specification, published by DMTF
[DSP0270](https://www.dmtf.org/dsp/DSP0270) (known as DSP0270).

As detailed in DSP0270, specifically in section 6, the Redfish host interface is
designed for use during the pre-boot (firmware) stage, as well as by drivers and
applications within the host operating system. Its architecture prioritizes
accessibility without reliance on external networking.

According to DSP0270, section 9, these bootStrap accounts are designated for use
solely on the host interface, within the host system itself. Therefore, there is
no need to employ these accounts for external access. Consequently, enforcing
access restrictions from outside the system (out-of-band access) is necessary to
minimize potential external attacks.

### Host interface

Redfish host interface specification, section 9 mentioned:

> Services that support credential bootstrapping via IPMI commands shall:
>
> - Implement the CredentialBootstrapping property defined in the HostInterface
>   schema.
> - Implement the CredentialBootstrappingRole property in the Links property
>   defined in the HostInterface schema.

In OpenBMC, the one of `Services` in this case is `phosphor-host-ipmid`.

As
[Redfish schema](https://github.com/openbmc/bmcweb/blob/4491419217d426791003facc749f600692d9b2a6/redfish-core/schema/dmtf/json-schema/HostInterface.v1_3_3.json),
`CredentialBootstrapping` interface includes three properties
`EnabledAfterReset`, `Enabled`, `RoleId`. And `CredentialBootstrappingRole` is
string property in `Link` interface.

To meet this requirement, the `HostInterface.CredentialBootstrapping` D-Bus
interface should be added to
[phosphor-dbus-interface](https://github.com/openbmc/phosphor-dbus-interfaces).
The interface will implement the CredentialBootstrapping properties
`EnabledAfterReset`, `Enabled`, `RoleId` which are defined in the HostInterface
schema.

### Create Bootstrap account and IPMI commands

Section 9.1 in Redfish host interface specification, the service shall implement
the IPMI commands:

- Get Manager Certificate Fingerprint (NetFn 2Ch, Command 01h)
- Get Bootstrap Account Credentials (NetFn 2Ch, Command 02h)

`phosphor-host-ipmid` will be updated to:

- Handle these two IPMI commands to create bootStrap accounts. The Privilege of
  account will base on the RoleId property in the
  `HostInterface.CredentialBootstrapping` D-Bus interface.
- Update the `Enabled` property in `HostInterface.CredentialBootstrapping` D-Bus
  interface base on the `Disable credential bootstrapping control` option in
  `Get Bootstrap Account Credentials` IPMI command.

### Bootstrap accounts accessibility

[Redfish host interface specification](https://www.dmtf.org/dsp/DSP0270),
section 9.2 mentioned:

> Bootstrap accounts shall be usable only on the host interface.

To satisfy this requirement, allow access the Redfish interface for specific
user with a specific Ethernet device should be supported.

### Bootstrap accounts life cycle

DSP0270, section 9.2 specifies the life cycle of bootStrap accounts. All
bootStrap accounts are deleted in the following conditions:

- when the service is reset.
- when host is reset.

## This design includes

- A methodology to create bootStrap accounts
- A methodology to configure the ethernet interface for bootStrap account.
- A methodology to distinguish bootStrap account with other accounts.

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

[ipmiUserSetPrivilegeAccess](https://github.com/openbmc/phosphor-host-ipmid/blob/8c974f76411cb40f2c9c25cddd86814087d0eddf/user_channel/user_layer.cpp#L161)
does not support set the `UserGroups` for specific user. The newly added user
will always has `UserGroups` value as all possible groups which is defined in
the value of `AllGroups` property in `xyz.openbmc_project.User.Manager`
interface. This interface is hosted by phosphor-user-manager. In current
implement of Phosphor-user-manager all possible groups are {`ssh`, `ipmi`,
`redfish`, `hostconsole`}.

### BMCWeb and Redfish

`BMCWeb` is an OpenBMC Daemon which implements the Redfish service. It supports
two main authentication methods `PAM based` (use Linux-PAM to do
username/password style of authentication) and `TLS based` (use the Public Key
infrastructure). The PAM-based method uses username/password or token in Redfish
request to authenticate. So BMCWeb will respond for the Redfish requests
regardless the physical interface which the request come from.

`BMCWeb` can get the User info of one `username` uses the user properties in
`User.Attributes` interface of the `/xyz/openbmc_project/user/<username>` path
of `xyz.openbmc_project.User.Manager` service.

By default, BMCWeb is support two security postures:

- HTTPS + Redirect on both ports 443 and port 80 if http-redirect is enabled And
- HTTPS only if http-redirect is disabled

Commit
[Enable HTTP additional sockets](https://github.com/openbmc/bmcweb/commit/796ba93b48f7866630a3c031fd6b01c118202953)
allows administrators to support essentially any security posture by using the
build options:

- additional-ports: Adds additional ports that bmcweb should listen to. This is
  always required when adding new ports.
- additional-bind-to-device: Accepts values that fill the SO_BINDTODEVICE flag
  in systemd/linux, and allows binding to a specific device.
- additional-auth: Allows specifying an authentication profile for each socket
  created with additional-ports.

BMCWeb supports multiple socket, each socket will listen on the specific HTTP
port. One BMCWeb socket will be added to response for `redfish host interface`
requests use
[`additional-ports`, `additional-bind-to-device` and `additional-auth` options](https://github.com/openbmc/bmcweb/blob/127afa70bfa236b2949f5c385704e00de99557e4/meson.options#L369).

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
  enabled after a reset for this interface.
- `Enabled`: An indication of whether credential bootstrapping is enabled for
  this interface. The value type is boolean.
- `RoleId`: The role assigned to the created bootstrap account. The privileges
  of the created bootstrap account will be updated accordingly based on the
  `RoleId` property.

### IPMI in-band commands to create bootStrap accounts

Which IPMI in-band interface and how IPMI handle below IPMI command is out of
the scope of this spec. There are two IPMI commands relate creating BootStrap
accounts.

#### Get Manager Certificate Fingerprint (NetFn 2Ch, Command 01h)

This command is used to get the fingerprint for the manager's TLS certificate
for the host interface.

> BMC will only handle this command when `Enabled` property values in
> `HostInterface.CredentialBootstrapping` interface are `true`. BMC will respond
> "Fingerprint of the manager's TLS certificate" for this command.

#### Get Bootstrap Account Credentials (NetFn 2Ch, Command 02h)

This command is used to get the user name and password of the bootStrap account.

> BMC will only handle this command when `Enabled` property values in
> `HostInterface.CredentialBootstrapping` interface are `true`. BMC will respond
> the user name and password from BMC shall:

- Be valid UTF-8 strings consisting of printable characters.
- Not include single quote (') or double quote (") characters.
- Not include backslash (\) characters.
- Not include whitespace characters.
- Not include control characters.
- Be at most 16 characters long.

After responding for the command, The `Enabled` property within the
`HostInterface.CredentialBootstrapping` D-Bus interface shall be set to `true`
when the "Disable credential bootstrapping control" value is `A5h`. Otherwise
`Enabled` will be `false`. The first responded bootStrap account user name will
be `bootstrap0`.

### Restrict the access of bootStrap account to Redfish host interface

Because the `BMCWeb` which implements the Redfish service can get the
`UserGroups` from `User.Attributes` interface. It can identify if the one
username which in the `redfish` userGroups should be allowed to authenticate for
the Redfish request. This restrict should already implemented by `BMCWeb`.

So the remain problem to `Restrict the access of bootStrap account` are

- Support add the bootStrap user account with only `redfish` group in
  `UserGroups` using `phosphor-host-ipmid`
- How identify if one user account is bootStrap account
- How identify the Redfish request is from `Redfish host interface`

#### Support add the user with only `redfish` group

Add `ipmiUserSetUserGroups` API in `user_layer` of `phosphor-host-ipmid` to
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
Cc ipmiUserSetUserGroups(const uint8_t userId, const uint8_t chNum,
                         const std::vector<std::string>& groupAccess);
```

#### Differentiate a bootStrap account and other accounts

The `User.Attributes` interface is already included the User Info of one
`username`, adding one more property to `User.Attributes` to identify the
bootStrap account is reasonable.

The proposed property name is `BootStrapAccount` with below description.

```text
    - name: BootStrapAccount
      type: boolean
      description: >
          True for bootStrap users.
      errors:
          - xyz.openbmc_project.Common.Error.InternalFailure
```

This property will be `true` for the username is BootStrap account.

Add `ipmiUserSetBootStrapAccount` API in `user_layer` of `phosphor-host-ipmid`
to create one user with the true `BootStrapAccount` flag.

```text
/** @brief sets wherether user is BootStrap account
 *
 *  @param[in] userId - user id
 *  @param[in] bootStrapState - is boot strap account state
 *
 *  @return ccSuccess for success, others for failure.
 */
Cc ipmiUserSetUserBootStrapAccountState(const uint8_t userId,
                                        const bool& bootStrapState);
```

#### Bind the Redfish host interface to specific device and port

Use the existing `additional-ports`, and `additional-bind-to-device`
[BMCWeb options](https://github.com/openbmc/bmcweb/commit/796ba93b48f7866630a3c031fd6b01c118202953)
to set the listening port 443 and bind to specific device for redfish host
interface. Example: Add `additional-ports="443"`,
`additional-auth="bootstrapedauth"` and `additional-bind-to-device="usb0"` to
specify that the request to interface `usb0` should using port `443`. BMCWeb
will base on these options to create new BMCWeb socket. The option
`additional-auth` will updated to support new authentication mode
`bootstrapedauth` to identify the added BMCWeb socket will be support
authentication for bootStrap accounts.

#### Flow diagram

```text
@startuml
participant HOST as Host
participant IPMI as Ipmi
participant BMCWeb as Web
participant "User Managers" as User

Web -> Web: +Add options in bmcweb_%.bbappend\n-Dadditional-bind-to-device="usb0"\n-Dadditional-ports="443"\n-Dadditional-auth="bootstrapedauth"
User -> User: + Add option phosphor-user-manager_%.bbappend\n -Dbootstrap_accounts=enabled
User -> User: + Host HostInterface.CredentialBootstrapping D-Bus interface\n at path /xyz/openbmc_project/user\n+ "EnableAfterReset" equal `bootstrap_accounts` option\n+ "Enable" = "EnableAfterReset\n+ RoleId="ReadOnly"\n+ Update the properites base on the latest saved setting
Host -> Ipmi: [Step 1] Get Boot Strap Account
Ipmi -> Ipmi: "Enabled" property of\nCredentialBootstrapping Dbus Interface is "True"?
alt "Enabled" is True
Ipmi -> Ipmi: [Step 2.0] Create BootStrap account &\n random password\n First BootStrap account user is `bootstrap0`
Ipmi -> Ipmi: [Step 2.1] Create linux account uses LinuxPam\n+ "UserGroups" is ["Redfish"]\n+ "UserPrivilege" is based on `RoleId` property in\n HostInterface.CredentialBootstrapping D-Bus interface
Ipmi -> User: [Step 2.2] Create BootStrap account D-Bus Object
User -> User: [Step 2.3] Create user object with\n+"BootStrapAccount" to True
User -> Web: [Step 2.4] Update the BootStrapAccount list
Ipmi -> User: [Step 2.5] Update "Enabled" property of HostInterface.CredentialBootstrapping D-Bus interface base on\n "Get Boot Strap Account" request
Ipmi -> Ipmi: Created the UserName D-Bus object and\n created linux account?
alt User D-Bus object and Linux account Created
Ipmi -> Host: [Step 3] Account Name + Password
else Failed to create the UserName D-Bus objected
Ipmi -> Host: [Step 4] Report failure
end
else "Enabled" is False
Ipmi -> Host: [Step 5] "Credential bootstrapping via IPMI commands is disabled"
end
Host -> Web: [Step 6] Redfish requests
Web -> Web: [Step 6.2] The request target to Redfish Host Interface?
alt To Redfish Host Interface
Web -> Web: [Step 6.3] Is user account in BootStrapAccount list?
alt No
Web -> Host: Unauthenticated
else No
Web -> Web: Valid authentication?
alt Invalid authentication
Web -> Host: Unauthenticated
else Valid authentication
Web -> Host: Collect the data and response for the request
end
end
else No
Web -> Web: [Step 6.3] Is user account in BootStrapAccount list?
alt No
Web -> Web: Valid authentication?
alt Invalid authentication
Web -> Host: Unauthenticated
else Valid authentication
Web -> Host: Collect the data and response for the request
end
else Yes
Web -> Host: Unauthenticated
end
end
@enduml
```

### Bootstrap accounts life cycle

Bootstrap accounts shall be deleted in the below conditions:

- On the reset of host
- On the reset of `phosphor-user-manager` service

If the `EnableAfterReset` property within the
`HostInterface.CredentialBootstrapping` D-Bus interface of is `true`, the
`Enabled` property will be set to `true` after reset. Otherwise, the `Enabled`
property will be set to `false`.

#### On the reset of host

The change state of `CurrentHostState` property in
`xyz.openbmc_project.State.Host` interface of `/xyz/openbmc_project/state/host0`
object path from the `xyz.openbmc_project.State.Host.HostState.Running` to other
state will be used to identify the reset of the host. `phosphor-user-manager` is
required to register the propertyChanged signal of property
CurrentHostState`property in`xyz.openbmc_project.State.Host ` interface.

#### On the reset of `phosphor-user-manager` service

The `phosphor-user-manager` hosts the users as the user object paths at the
`/xyz/openbmc_project/user/<username>`. The object path of bootStrap accounts
will be removed when the service is restart.

## Impacts

This change will:

- Add a one more property `BootStrapAccount` to `User.Attributes` D-Bus
  interface of `phosphor-dbus-interfaces`.
- Add two APIs `ipmiUserSetUserGroups` and
  `ipmiUserSetUserBootStrapAccountState` in `phosphor-host-ipmid`.
- Redfish Host Interface needs to be bond to specific port and network interface
  such as port 443 and `usb0` in the example.
- Add `additional-auth` option to `BMCWeb` to configure the socket support
  Redfish Host Interface.

### Organizational

The following repositories are expected to be modified to implement this design:

- `phosphor-dbus-interfaces`
- `phosphor-host-ipmid`
- `phosphor-user-manager`
- `bmcweb`
- `openbmc/meta-<oem>`

## Testing

Add below build options in `bmcweb_%.bbappend`

```text
EXTRA_OEMESON:append = " \
     -Dadditional-bind-to-device="usb0" \
     -Dadditional-ports="443" \
     -Dadditional-auth="bootstrapedauth" \
"
```

### Test 1: Handle `Disable credential bootstrapping control` option

- Use `get manager certificate fingerprint` to get 32 bytes finger print.
  `ipmitool raw 0x2c 0x2 0x52 0x1`
- Use "Get Bootstrap Account Credentials" IPMI command to get bootStrap account
  with option `Disable credential bootstrapping control` is `0xa5`(Keep
  credential bootstrapping enabled). `ipmitool raw 0x2c 0x2 0x52 0xa5`
- One new bootStrap account with password will be responded.
- Use "Get Bootstrap Account Credentials" IPMI command to get bootStrap account
  with option `Disable credential bootstrapping control` is not `0xa5`(Disable
  credential bootstrapping). `ipmitool raw 0x2c 0x2 0x52 0xa4`
- One new bootStrap account with password will be responded.
- BMC will repond `80h: Credential bootstrapping via IPMI commands is disabled.`
  for any ipmitool command to `Get bootstrap account credentials` or
  `Get manager certificate fingerprint` after above command.

### Test 2: Manage bootStrap account

Because phosphor-user-manager limits number of none-ipmi user to 15.

- Use ipmitool command to create 15 bootStrap accounts
  `ipmitool raw 0x2c 0x2 0x52 0xa5`
- Create the 16th bootStrap account should be failed.
- Delete one bootStrap account uses ipmitool/Redfish/BmcWeb.
- Create new bootStrap account should be success.

### Test 3: Bootstrap accounts life cycle

- Use ipmitool command to create bootStrap accounts
  `ipmitool raw 0x2c 0x2 0x52 0xa5`
- The bootStrap accounts should be removed from
  `xyz.openbmc_project.User.Manager` D-Bus, /etc/passwd, `ipmi user list`, linux
  account list in below cases:
  - Restart of phosphor-user-manager service
  - Reboot of BMC
  - Host change to not `Running` state

### Test 4: Bootstrap account only works on the Redfish Host Inband interface

- Use "Get Bootstrap Account Credentials" IPMI command to get bootStrap account.
  `ipmitool raw 0x2c 0x2 0x52 0xa5`
- Check the bootStrap account in `/xyz/openbmc_project/user/<accountName>`.
  Value of property `BootStrapAccount` of `User.Attributes` D-Bus interface
  should be `true`. Value of `UserGroups` of `User.Attributes` D-Bus interface
  should only `redfish`.
- Check the IP Address of BMC address in `usb0`.
- BMC will only respond `success` for the in-band Redfish request with the IP
  Address of `usb0` interface at port 443 and bootStrap accounts.
- BMC will respond `error` for the out-of-band Redfish, IPMI and WebUI requests
  with bootStrap accounts. BMC will respond `error` for the ssh requests with
  bootStrap accounts.

### Test 5: Misc testing

BMC should respond `error` for below cases:

- Inband or out-of-band IPMI requests + BootStrap account
- Out-of-band Redfish request + BootStrap account
- Inband Redfish request + None bootStrap account
- Inband Redfish request + bootStrap account + port is not 443
- Inband Redfish request(usb2 for multiple inband interface ) + bootStrap
  account + port is 443

BMC should respond `success` for this case:

- Inband Redfish request to IP Address of the `usb0` interface + bootStrap
  account + port is 443
