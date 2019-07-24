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
[CA law SB-327 Information privacy: connected devices][SB-327],
paragraph 1798.91.04(b)(2), suggests a "feature that requires a user
to generate a new means of authentication before access is granted to
the device for the first time" would be a "reasonable security
feature".

[SB-327]: https://leginfo.legislature.ca.gov/faces/billTextClient.xhtml?bill_id=201720180SB327

Various computer systems ship with default userid and passwords and
require the password be changed on the initial signon.  This reduces
the time window when the system is accessible with a default password.

## Requirements
The requirements are:
 - The BMC's initial password must be expired when the new
   EXPIRED_PASSWORD image or distro feature is used.
 - An account with an expired password must not be allowed to use the
   BMC (except to change the password).
 - There must be a way to change the expired password.

The use case is:
 - The BMC has a default password built in.
 - The BMC automatically connects to its management network which
   offers administrative or operational interfaces (whether or not the
   BMC is normally operated via its host).
 - The network the BMC connects to is not fully trusted.

## Proposed Design
This design creates a new feature EXPIRED_PASSWORD:
 - When EXPIRED_PASSWORD is enabled, the BMC's default password will
   initially be expired.
 - The BMC's network interfaces (such as REST APIs, SSH, and IPMI)
   will not allow access via an account with an expired password.
 - There is a way to change an expired password, for example:
    - Redfish API
    - The `ipmitool user set password` command
    - SSH
    - Access via the BMC's host

The feature will be disabled by default.

## Alternatives Considered
The following alternate designs were considered:
- Unique password per machine.  That approach requires additional
  effort, for example, to set and track assigned passwords.
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
  That approach requires an additional server.

## Impacts
Having to change an expired password is annoying and breaks
operational procedures and scripts.  Documentation, lifecycle review,
and test is needed.  Expect pain when this is enabled.

## Testing
Scenarios:
1. Ensure that when the BMC is in its initial state:
    - All available network interfaces deny access.
    - Selected interfaces allow the password to be changed.
2. Ensure factory reset resets the password to its initial expired
state (repeat those tests).
3. Ensure code update does not cause the password to expire.
4. Ensure the BMC continues to operate its host, for example, when the
BMC is factory reset while the host is running.  Ensure the power
button can be used to power off the host while the BMC's password is
expired.
