# Provide a default admin account

Author:
  Joseph Reynolds <josephreynolds1>

Primary assignee:
  None

Other contributors:
  None

Created:
  June 17, 2020

## Problem Description
The BMC's default account should not be root; it should be an administrator
account that has the capability of gaining root privileges.  See the
background section for details.

Having root login capability comes with some ill effects:
 - It encourages password sharing which makes individual accountability more
   difficult.
 - It does not follow the principle of least privilege.
 - Running as root makes some Linux interfaces bypass security checks.

To clarify: This design is to shut off access to the BMC via the root account.
It is not intended to address BMC processes that run as root.

## Background and References
From the beginning the default OpenBMC admin account is `root`.  The project
has a way to partially disable the root account in the form of the
[phosphor-user-manager enable_root_user_mgmt][] support.  However, it is not
the default, and there is no standard way to create another account.

Many Linux distributions have disabled root login for various reasons, and
such references are not given here.

Root access (running a BMC command shell with effective uid=0) is needed for
various BMC use cases including firmware development.  The `sudo` command is a
standard way to become root in a way that preserves accountability.  And
membership in the Linux group `sudo` is a way to control access to the `sudo`
command.  (The `su` command is discussed as an alternative.)

The [OpenBMC user management][] design provides for multiple accounts, each
with a Privilege Role (such as `admin`) and Group Roles which determine the
interfaces the user is allowed to access (ssh, redfish, etc.).  These OpenBMC
roles are implemented using Linux groups.

[OpenBMC user management]: https://github.com/openbmc/docs/blob/master/architecture/user-management.md
[phosphor-user-manager enable_root_user_mgmt]: https://github.com/openbmc/phosphor-user-manager/blob/master/configure.ac

## Requirements
By default, root login to the BMC via any interface must not work.

There must be a default admin account that has the capability of gaining root
privileges.  Any admin account (including the default admin account and
dynamically created accounts) can use the `sudo` command to gain root access.

There must be options for the following use cases:
 - As an admin, I need to enable root login for my tools to work.
 - For my BMC image, I need root login enabled by default.
 - For my BMC image, I need to lock out everyone from root access.

The project's continuous integration (CI) must continue to work with only a
small glitch during the transition from root to admin.


## Proposed Design
The design has a staged implementation:

1. Create a standard way to create a default OpenBMC user account that can use
   the `sudo` command.  There are two parts.
   A. Create a bitbake class that leverages the `useradd` bitbake class and
      adds OpenBMC-specific elements (per [OpenBMC User Management][])
      including:
      - An easy way to set the initial password (or disallow password-based login).
      - Privilege Role.
      - Group Roles.
      - A new `sudo` role which grants password-less access to the `sudo`
        command.
      - An option to administratively expire the initial password (such as via the
        `passwd --expire USER` command).

   B. Add the `sudo` command to the default OpenBMC image, with access
      controlled by membership in the `sudo` group.  Add users created with
      the `admin` Privilege Role to the `sudo` group.  This design does NOT
      propose to create an external BMC interface to control which users are
      in the `sudo` group (such as via the `usermod -G groups... USERID`
      command).

2. Have a new image option to create a default "admin" user.  This would
initially be disabled by default, and enabled later.

3. Have a new image option to completely disable root logins.  When enabled,
the root account would still be present in the underlying `passwd` and
`shadow` files to satisfy Linux requirements, but the BMC would not allow root
logins via any interface, and root would not appear in any OpenBMC external
interface.  This would leverage the existing [phosphor-user-manager
enable_root_user_mgmt][] support.

4. Change OpenBMC project defaults to disable root and create admin by
default.  Do this only after general agreement of the project community,
including the CI teams, and with sensitivity to the OpenBMC release cycle.

## Alternatives Considered
We could omit the last stage (the cut over).  Instead various meta-machine
layers could choose to participate or not.  This will make the project less
cohesive and may make future CI changes more difficult.

The `su` command was investigated as a way for an admin gain root privileges.
Indeed, the `su` support is already present in default OpenBMC image,
including its Linux-PAM configuration.  Use of `su` was (weakly) rejected for
two reasons:
1. With Selinux, the `su` command can only be used by the root user and cannot
   be used by another user to become root.  The solution is to use `sudo`.

2. Dynamic configuration of which users are allowed to use the `sudo` command
   can be accomplished by adding a removing a user from the `sudo` group.
   OpenBMC already has support for this in the form of User Management
   "privilege groups".  The `sudo` group fits this pattern.  To configure `su`
   similarly would require writing new code to update files under `/etc/pam.d`
   such as `/etc/pam.d/su`.

## Impacts
1. The account name change (root to admin) affects tool and processes,
including the project's CI.  The staged implementation is intended to address
this by making it easy to try the proposed change and adapt to them before the
change becomes the default.

2. The account name change further impacts operational procedures that use SSH
access to the BMC which assume the default account has root authority.  For
example, even after all references to root are changed to admin, ssh sessions
will no longer have root authority and will have to use the `sudo` command.
The passwordless `sudo` support is intended to help address this impact.

3. The User Management doc should be updated, and have a new section that
outlines the options for default accounts.

4. No upgrade impacts are foreseen.  If a BMC that uses the root account is
upgraded with an image that provides the admin account, the root account will
remain usable as the BMC's admin account.

## Testing
The impacts to CI are outlined above.

Some basic tests include:

1. Test combinations of default user accounts and ensure each feature works as
   expected.
 - Different Privilege Roles.
 - Different combinations of Group Roles.
 - Initial expired password.

2. Ensure the `sudo` support works.  Specifically, add and remove users from
the `sudo` group, and ensure users in the `sudo` group have `sudo` privileges,
and other users do not.

3. Ensure the root account cannot login to any interfaces (REST APIs, IPMI,
SSH) and does not appear on any list of users (outside of the `/etc`
directory) and cannot be created via the BMC's supported REST API interfaces.
