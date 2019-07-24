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

## Requirements
The requirements are:
 - The BMC's initial password must be expired when the new
   EXPIRED_PASSWORD image feature is used.
 - An account with an expired password must not be allowed to use the
   BMC (except to change the password).
 - There must be a way to change the expired password using a
   supported interface.

The use case is:
 - The BMC has at least one acccount with a default password built in.
 - The BMC automatically connects to its management network which
   offers administrative or operational interfaces (whether or not the
   BMC is normally operated via its host).
 - The network the BMC connects to is not fully trusted.

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

3. There is a way to change an expired password.  Specifically, the
   expired password is accepted as part of a password changing
   protocol, or there is a non-network vector to change the password.
   For example:
    - Redfish: PATCH new password to AccountService.
    - SSH server: expired password change dialog.
    - Access via the BMC's host: for example, via the `ipmitool user
      set password` command when accessed in-band.  Note the RCMP+
      standard, such as used for the BMC's network IPMI interface,
      does not support changing the password when establishing a
      session.

A [Redfish Password Change Required support proposal][] work in
progress addresses this exact scenario.
[Redfish Password Change Required support proposal]: https://www.dmtf.org/sites/default/files/Redfish_2019.1_Work_in_Progess.pdf

TODO: Choose a direction:
 - via PATCH Redfish AccountService { password:NEW }
 - via some other API such as the /login URI

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

Have a new service to detect if any password has its default value,
and write log messages for that condition, with the idea to alert the
system owner of this condition. That approach doesn't solve the
problem and burdens BMC resources.

## Impacts
Having to change an expired password is annoying and breaks
operational procedures and scripts.  Documentation, lifecycle review,
and test is needed.  Expect pain when this is enabled.

For example, the [REDFISH-cheatsheet][] should be updated with commands
to update the inital password.

[REDFISH-cheatsheet]: ../REDFISH-cheatsheet.md

## Testing
Scenarios:
1. Ensure that when the BMC is in its initial state:
    - All available network interfaces deny access.
    - Selected interfaces allow the password to be changed.
2. Ensure factory reset resets the password to its initial expired
   state (repeat those tests).
3. Ensure code update does not cause the password to expire, for
   example, by attempting to sign in with default credentials, and
   observe the result (200 vs 401).  For example:
    - If the password was changed on the old release, validate the
      password is NOT expired on the new release.
    - If the password was not changed on the old release, validate the
      password IS expired on the new release.
4. Ensure the BMC continues to operate its host, for example, when the
   BMC is factory reset while the host is running.  Ensure the power
   button can be used to power off the host while the BMC's password is
   expired.
