# Initial expired passwords

Author:
  Joseph Reynolds <josephreynolds1>

Primary assignee:
  Joseph Reynolds <josephreynolds1>

Other contributors:
  None

Created:
  2019-07-24

## Problem Description
OpenBMC has a default password, connects to the network via DHCP, and
does not have a mechanism to require administrators to change the
BMC's password.  This may lead to BMCs which have default passwords
being on the network for long time periods, effectively giving
unrestricted access to the BMC.

## Background and References
Various computer systems ship with default userid and passwords and
require the password be changed on the initial signon.  This reduces
the time window when the system is accessible with a default password.

The [Redfish Specification version 1.7.0][] section 3.2.6.1 ("Password
change required handling") provides the ManagerAccount resource v1.3
with a PasswordChangeRequired property.

[Redfish Specification version 1.7.0]: https://www.dmtf.org/sites/default/files/standards/documents/DSP0266_1.7.0.pdf

Note the terminology:
An "expired password" is a special case of "password change required".

## Requirements
The requirements are:
 - The BMC's initial password must be expired when the new
   EXPIRED_PASSWORD image feature is used.
 - An account with an expired password must not be allowed to use the
   BMC (except to change the password).
 - There must be a way to change the expired password using a
   supported interface.

The use case is:
 - The BMC has at least one account with a default password built in.
 - The BMC automatically connects to its management network which
   offers administrative or operational interfaces (whether or not the
   BMC is normally operated via its host).
 - The network the BMC connects to is not fully trusted.
 - The BMC is operated from its management network.
 - The BMC can update the password; for example, the `/etc/passwd`
   file is writeable.

## Proposed Design
This design has three main parts:

1. There is a new image feature EXPIRED_PASSWORD. When
   EXPIRED_PASSWORD is enabled, the BMC's default password will
   initially be expired as if via the `passwd --expire root` command.
   This administratively expires the password and is not based on
   time.  An account with an expired password is neither locked nor
   disabled.

   This feature is intended to be enabled by default, with a staging
   plan: the feature will be disabled to give time for the continuous
   integration (CI) and test automation efforts to adapt, then enabled
   for the overall project.

2. The BMC's network interfaces (such as REST APIs, SSH, and IPMI)
   disallow access via an account which has an expired password.  If
   the access credentials are otherwise correct and the reason for the
   authentication failure is an expired password (determined by the
   usual Linux practices), where possible, the interface should
   indicate the password is expired and how to change it (see below).
   Otherwise the usual security protocols apply (giving no additional
   information).

   The `/login` URI is enhanced.  If it is used with correct
   credentials (userid and password) and the password needs to be
   changed the request will fail to create a session and indicate a
   password change is needed.  This is the only case where additional
   information is given, otherwise, for security reasons, the usual
   error message must be given.

   The '/redfish/v1/SessionService/Sessions' is enhanced to indicate
   PasswordChangeRequired per the Redfish spec.

3. There is a way for an account owner to change their own expired
   password.  Specifically, the expired password is accepted as part
   of a password changing protocol, or there is a non-network vector
   to change the password.  For example:
    - Redfish: Implement the PasswordChangeRequired handling.
      Specifically, PATCH to
      `/redfish/v1/AccountService/Accounts/${MyAccount}` with JSON
      data `{Password:${NEWPASSWORD}}`.
    - SSH server: Implment the expired password change dialog.
    - Access via the BMC's host: for example, via the `ipmitool user
      set password` command when accessed in-band.  Note the RMCP+
      standard, such as used for the BMC's network IPMI interface,
      does not support changing the password when establishing a
      session.

This design is intended to cover any cause of expired password,
including both the BMC's initial expired password and password expired
for another cause such a aging or via the `passwd --expire` command.

This design is intended to enable the phosphor-webui web application
to change an expired password.  The web app will need a little extra
logic for the signon screen.

Per the above design, when the web app uses either `/login` or
`/redfish/v1/SessionService` to establish a session and the account
has an expired password, the web app will recieve an indication that
the password must be changed:
 - The `/login` URI will indicate the password must be changed, and
   will not establish a session.  In this case, the web app must use
   the Redfish API to establish a session.
   Note the `/login` URI is deprecated.
 - The `/redfish/v1/SessionService` URI will indicate
   PasswordChangeRequired and establish a session.  The web app can
   use this session to change the password per the design above.  If
   desired, the web app can then terminate the Redfish session and
   signon again via `/login`.

## Alternatives Considered
The following alternate designs were considered:
- Unique password per machine.  That approach requires additional
  effort, for example, to set and track assigned passwords.
- Default to having no users with access to the BMC via its network.
  When network access is needed, a technician would create or modify
  the userid to have network authority and establish a password at
  that time.  This may be through the BMC's host system or via the
  BMC's serial console.  That approach requires the tech to have
  access, and requires re-provisoning the account after factory reset
- Disable network access by default.  That approach requires another
  BMC access vector, such as physical access or via the BMC's host, to
  enable network access.
- Provision the BMC with a certificate instead of a password, for
  example, a SSH client certificate.  That approach suffers from the
  same limitations as a default password (when the matching private
  certificate becomes well known) and requires the BMC provide SSH
  access.
- Require physical presence to change the password.  For example,
  applying a jumper, or signing in via a serial cable.  That approach
  is not standard.
- Have LDAP (or any authentication/authorization server) configured
  and have no default passwords configured in the BMC firmware image.
  That approach requires the customer would be having LDAP server.
  Also, how we can configure the LDAP, as we don't know the customer
  LDAP server configuration?
- Have a new service to detect if any password has its default value,
  and write log messages for that condition, with the idea to alert
  the system owner of this condition. That approach doesn't solve the
  problem and burdens BMC resources.

Warning.  This design may leave the BMC with its default password for
an extended period of time if the use case given in the requirements
section of this design does not apply.  For example, when the host is
operated strictly via its power button and not through the BMC's
network interface.  In this case, the alternatives listed above may
mitigate this risk.  Another alternative is a design where the BMC is
initially in a provisioning mode which does not allow the BMC to
operate its host.  The idea is that you have to establish access to
the BMC (which includes changing its password) before you can leave
provisioning mode.

## Impacts
Having to change an expired password is annoying and breaks
operational procedures and scripts.  Documentation, lifecycle review,
and test is needed.  Expect pain when this is enabled.

To help with this, the [REDFISH-cheatsheet][] will be updated with
commands needed to detect and change an expired password.

[REDFISH-cheatsheet]: https://github.com/openbmc/docs/blob/master/REDFISH-cheatsheet.md

This design does not affect other policies such as password aging.

## Testing
Scenarios:
1. Ensure that when the BMC is in its initial state:
    - All available network interfaces deny access.
    - Selected interfaces allow the password to be changed.
2. Ensure factory reset resets the password to its initial expired
   state (repeat the tests above).
3. Handle BMC code update and BMC FRU replacement scenarios.  For
   example, ensure code update does not cause the password to expire,
   for example, by attempting to sign in with default credentials, and
   observe the result (200 vs 401).  For example:
    - If the password was changed on the old release, validate the
      password is NOT expired on the new release.
    - If the password was not changed on the old release, validate the
      password IS expired on the new release.
4. Ensure the BMC continues to operate its host, for example, when the
   BMC is factory reset while the host is running.  Ensure the power
   button can be used to power off the host while the BMC's password is
   expired.
5. Test on BMC using Linux PAM both with and without LDAP or
   ActiveDirectory configured.