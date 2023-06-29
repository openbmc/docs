# Initial expired passwords

Author: Joseph Reynolds <josephreynolds1>

Other contributors: None

Created: 2019-07-24

## Problem Description

OpenBMC has a default password, connects to the network via DHCP, and does not
have a mechanism to require administrators to change the BMC's password. This
may lead to BMCs which have default passwords being on the network for long time
periods, effectively giving unrestricted access to the BMC.

## Background and References

Various computer systems ship with default userid and passwords and require the
password be changed on the initial signon. This reduces the time window when the
system is accessible with a default password.

Various BMC interfaces require authentication before access is granted. The
authentication and account validation steps typically result in outcomes like
this:

1. Success, when the access credentials (such as username and password) are
   correct and the account being accessed is valid.
2. Failure, when either the access credentials are invalid or the account being
   accessed is invalid. For example, the account itself (not merely its
   password) may be expired.
3. PasswordChangeRequired, when the access credentials are correct and the
   account is valid except the account's password is expired (such as indicated
   by PAM_CHANGE_EXPIRED_AUTHTOK).

OpenBMC currently implements the first two outcomes; it treats
PasswordChangeRequired the same as an account that is invalid for any other
reason. Some servers (such as the OpenSSH server) handle the
PasswordChangeRequired by implementing a "password change dialog".

The [Redfish Specification version 1.7.0][] section 13.2.6.1 ("Password change
required handling") provides the ManagerAccount resource v1.3 with a
PasswordChangeRequired property which supports a password change dialog.

[redfish specification version 1.7.0]:
  https://www.dmtf.org/sites/default/files/standards/documents/DSP0266_1.7.0.pdf

Note the terminology: An "expired password" is a special case of "password
change required".

The meaning of the term "access" varies by context. It could mean:

1.  Access to the BMC's network interfaces.
2.  Access to the BMC's authentication mechanisms together with correct
    credentials, whether or not those credentials have expired and must be
    changed.
3.  Access to the BMC's function via an authenticated interface, for example,
    such as establishing a session after you've changed your initial password.
4.  Access to the BMC's function via an unauthenticated interface such as host
    IPMI or physical control panel (example: power button).

This design uses meanings 3 and 4 except where indicated.

## Requirements

The requirements are:

- The BMC's initial password must be expired when the new EXPIRED_PASSWORD image
  feature is used.
- An account with an expired password must not be allowed to use the BMC (except
  to change the password).
- There must be a way to change the expired password using a supported
  interface.

The use case is:

- The BMC automatically connects to its management network which offers
  administrative or operational interfaces (whether or not the BMC is normally
  operated via its host).
- The BMC is operated from its management network.

Preconditions for the BMC include:

- The BMC has at least one account with a default password built in.
- The BMC can update the password; for example, the `/etc/passwd` file is
  writeable.

## Proposed Design

This design has three main parts:

1. There is a new image feature EXPIRED_PASSWORD. When EXPIRED_PASSWORD is
   enabled, the BMC's default password will initially be expired as if via the
   `passwd --expire root` command. This administratively expires the password
   and is not based on time. An account with an expired password is neither
   locked nor disabled.

   This feature is intended to be enabled by default, with a staging plan: the
   feature will be disabled to give time for the continuous integration (CI) and
   test automation efforts to adapt, then enabled for the overall project.

2. The BMC's network interfaces (such as REST APIs, SSH, and IPMI) disallow
   access via an account which has an expired password. If the access
   credentials are otherwise correct and the reason for the authentication
   failure is an expired password (determined by the usual Linux practices),
   where possible, the interface should indicate the password is expired and how
   to change it (see below). Otherwise the usual security protocols apply
   (giving no additional information).

   The `/login` URI is enhanced. If it is used with correct credentials (userid
   and password) and the password needs to be changed the request will fail to
   create a session and indicate a password change is needed. If it is used with
   correct userid and incorrect password, or with an incorrect userid, the
   behavior must not change. Note the `/login` URI is deprecated.

   The '/redfish/v1/SessionService/Sessions/<session>' and
   '/redfish/v1/AccountService/Accounts/<account>' resources are enhanced to
   indicate PasswordChangeRequired per the Redfish spec.

   The `ipmitool` command treats an expired password the same as an invalid
   password. Note the RMCP+ standard, such as used for the BMC's network IPMI
   interface, does not support changing the password when establishing a
   session. The ipmitool is not being enhanced by this design.

   The Secure Shell access (via the `ssh` command) already correctly indicates
   when the password is expired. No change is needed. But see the next bullet
   for the expired password dialog.

3. There is a way for an account owner to change their own expired password.
   This can be either from a network-facing or in-band password changing
   protocol. For example:
   - Redfish: This design adds the Redfish PasswordChangeRequired handling to
     BMCWeb. See below for details.
   - SSH server: The SSH servers may have an expired password change dialog. For
     example, OpenSSH implement this feature. However, the Dropbear SSH server
     announces the password is expired, but does not implement the dialog to
     change it.
   - Access via the BMC's host: for example, via the
     `ipmitool user set password` command when accessed in-band.

When Redfish creates a session with PasswordChangeRequired, every response
contains a PasswordChangeRequired message. The session effectively has only the
ConfigureSelf privilege which allows it to only change the password and
terminate the session. The usage pattern is:

1.  Create a session.
2.  If the PasswordChangeRequired message is present:
    1.  PATCH the new password into the ManagerAccount object.
    2.  Any other use of the session will get HTTP status code 403 Forbidden.
    3.  DELETE the Session object to terminate the session.
    4.  Create a new session and continue.

This design is intended to cover any cause of expired password, including both
the BMC's initial expired password and password expired for another cause such
as aging or via the `passwd --expire` command.

This design is intended to enable the webui-vue web application to implement a
password change dialog for the signon screen.

Per the above design, when the web app uses either `/login` or
`/redfish/v1/SessionService` to establish a session and the account has an
expired password:

- If the `/login` URI was used, the HTTP response indicates the password must be
  changed, and will not establish a session. In this case, the web app must use
  the Redfish API to establish a session.
- POST to `/redfish/v1/SessionService/Sessions` will establish a session which
  will have the PasswordChangeRequired message.
- At this point the web app can display a message that the password is expired
  and must be changed, then get the new password.
- PATCH the password to the account specified in the PasswordChangeRequired
  message.
- DELETE the Session object to terminate the session.
- Create a new session and continue.

## Alternatives Considered

The following alternate designs were considered:

- Unique password per machine. That approach requires additional effort, for
  example, to set and track assigned passwords.
- Default to having no users with access to the BMC via its network. When
  network access is needed, a technician would create or modify the userid to
  have network authority and establish a password at that time. This may be
  through the BMC's host system or via the BMC's serial console. That approach
  requires the tech to have access, and requires re-provisioning the account
  after factory reset
- Disable network access by default. That approach requires another BMC access
  vector, such as physical access or via the BMC's host, to enable network
  access.
- Provision the BMC with a certificate instead of a password, for example, an
  SSH client certificate. That approach suffers from the same limitations as a
  default password (when the matching private certificate becomes well known)
  and requires the BMC provide SSH access.
- Require physical presence to change the password. For example, applying a
  jumper, or signing in via a serial cable. That approach is not standard.
- Have LDAP (or any authentication/authorization server) configured and have no
  local users which have default passwords configured in the BMC firmware image.
  That approach requires the customer have an LDAP (or similar) server. Also,
  how we can configure the LDAP, as we don't know the customer LDAP server
  configuration?
- Have a new service to detect if any password has its default value, and write
  log messages for that condition, with the idea to alert the system owner of
  this condition. That approach doesn't solve the problem and burdens BMC
  resources.

Warning. This design may leave the BMC with its default password for an extended
period of time if the use case given in the requirements section of this design
does not apply. For example, when the host is operated strictly via its power
button and not through the BMC's network interface. In this case, the
alternatives listed above may mitigate this risk. Another alternative is a
design where the BMC is initially in a provisioning mode which does not allow
the BMC to operate its host. The idea is that you have to establish access to
the BMC (which includes changing its password) before you can leave provisioning
mode.

The BMCWeb Redfish server could be enhanced so that when the Password is
successfully patched, the session no longer asserts the PasswordChangeRequired
condition and re-asserts the user's usual authority immediately without
requiring a new session. This is allowed by the Redfish spec, but was not
implemented because it would be more difficult to code and test.

## Impacts

Having to change an expired password is annoying and breaks operational
procedures and scripts. Documentation, lifecycle review, and test are needed.
Expect pain when this is enabled.

To help with this, the [REDFISH-cheatsheet][] will be updated with commands
needed to detect and change an expired password.

[redfish-cheatsheet]:
  https://github.com/openbmc/docs/blob/master/REDFISH-cheatsheet.md

This design does not affect other policies such as password aging.

## Testing

Scenarios:

1. Ensure that when the BMC is in its initial state:
   - All available network interfaces deny access.
   - Selected interfaces allow the password to be changed.
2. Ensure factory reset resets the password to its initial expired state (repeat
   the tests above).
3. Ensure the password change is effective for users entering from all supported
   interfaces. For example, change the password via the Redfish API, and
   validate that the old password does not work and the new password does work
   via IPMI for the same user.
4. Handle BMC code update scenarios. For example, (A) Ensure code update does
   not cause a previously set password to change to default or to expire. (B)
   Validate what happens when the BMC has a default password and does code
   update to a release which has the default expired password design (this
   design).
5. Ensure the BMC continues to operate its host, for example, when the BMC is
   factory reset while the host is running. Ensure the power button can be used
   to power off the host while the BMC's password is expired.
6. Test on BMC using Linux PAM both with and without LDAP or ActiveDirectory
   configured.
7. Validate you can to change an IPMI user's expired password, such as with:
   ipmitool user set password 1 NEWPASSWORD. This can be from another IPMI
   user's session or from unauthenticated access.
