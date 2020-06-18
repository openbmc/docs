# Provide a default admin account

Author:
  Joseph Reynolds <josephreynolds1>

Primary assignee:
  None

Other contributors:
  None

Created:
  July 14, 2020

## Problem Description
The BMC's default account should not be root; it should be an administrator
account that may have the capability of gaining root privileges.  See the
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
membership in the Linux group `wheel` is a way to control access to the `sudo`
command.  (The `su` command is discussed as an alternative.)

The [OpenBMC user management][] design provides for multiple accounts, each
with a Privilege Role (such as `admin`) and Group Roles which determine the
interfaces the user is allowed to access (ssh, redfish, etc.).  These OpenBMC
roles are implemented using Linux groups.

[OpenBMC user management]: https://github.com/openbmc/docs/blob/master/architecture/user-management.md
[phosphor-user-manager enable_root_user_mgmt]: https://github.com/openbmc/phosphor-user-manager/blob/master/configure.ac

## Requirements
By default, root login to the BMC via any interface must not work.  There must
be a way to re-enable the pre-configured root login access.

There must be a pre-created admin account.  There must be a way to configure
the build so the admin account is not pre-created, and to pre-create different
accounts.

The pre-created admin account must have the administrator role and `sudo`
access.  There must be a way to specify the role for pre-created accounts and
there must be a way to specify which pre-created accounts have sudo access.

The `sudo` package must be present in the image by default.  There must be a
way to remove the sudo package.

There must be options for the following use cases:
 - As an admin, I need to enable root login for my tools to work.
 - For my BMC image, I need root login enabled by default.
 - For my BMC image, I need to lock out everyone from root access.

The project's continuous integration (CI) must transition smoothly from root
to admin.

## Proposed Design
The design has several parts:

1. Completely disable root logins.  The root account remains in the image in
the underlying `passwd` and `shadow` files to satisfy Linux requirements, but
the BMC does not allow root logins via any interface, and root does not appear
in any OpenBMC external interface.  This leverages the existing
[phosphor-user-manager enable_root_user_mgmt][] support.

   Document build configuration options:
    - How to re-enable root login.
    - How to set the root account initial password.

2. Create an "admin" account via the OpenEmbeded useradd class and customize
it per [OpenBMC User Management][] to have:
    - A default password (0penBmc anyone?).
    - The administrator role.
    - All supported group roles.
    - Access to the sudo command.

   Document build configuration options:
    - How to avoid pre-creating the admin user account.
    - How to pre-create additional custom accounts.
    - How to customize a pre-created account's initial password.
    - How to customize a pre-created account's privilege role and access
      privileges.
    - How to customize a pre-created account's group roles.
    - How to customize a pre-created account's sudo access.
    - How to expire an initial password so it must be changed at login such as
      via the `passwd --expire USER` command.

3. Build the `sudo` package into the image with access to the `sudo` command
   controlled by the `wheel` group.  Create the `wheel` group and configure
   password-less access to the `sudo` command.  Ensure use of the `sudo`
   command is logged.

   This design does NOT propose to create an external BMC interface to control
   which users are in the `wheel` group (such as via the `usermod -G
   groups... USERID` command).

   Document build configuration options:
    - How to remove the `sudo` package.  Mention that references to the
      "wheel" group must be removed when sudo is removed; link to

## Alternatives Considered

We considered creating an admin account but not transitioning the project
default away from root.  Instead various meta-machine layers could choose to
participate or not.  This will make the project less cohesive and may make
future CI changes more difficult.

Membership in the `sudo` group was considered as the access control for the
`sudo` command.  The `wheel` group was chosen because it has better support
in Yocto and is more widely used.

The `su` command was investigated as a way for an admin gain root privileges.
Indeed, the `su` support is already present in default OpenBMC image,
including its Linux-PAM configuration.  Use of `su` was (weakly) rejected for
two reasons:
1. With Selinux, the `su` command can only be used by the root user and cannot
   be used by another user to become root.  The solution is to use `sudo`.
2. Dynamic configuration of which users are allowed to use the `sudo` command
   can be accomplished by adding a removing a user from the `wheel` group.
   OpenBMC already has support for this in the form of User Management
   "privilege groups".  The `wheel` group fits this pattern.  To configure `su`
   similarly would require writing new code to update files under `/etc/pam.d`
   such as `/etc/pam.d/su`.

## Impacts
1. The account name change (root to admin) affects tool and processes.
Examples:
 - The project's continuous integration (CI) test servers.
 - Use of `ssh` or `scp` to test and debug.

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

The following tests are intended to test the default project configuration;
only the first variation is intended to test customized build configurations.

1. Test combinations of default user accounts and ensure each feature works as
   expected.  Ensure the privilege model is enforced.  Ensure `sudo` access is
   as intended.
 - Different Privilege Roles.
 - Different combinations of Group Roles.
 - Initial expired password.

2. Ensure the `sudo` support works.  Specifically, add and remove users from
the `wheel` group, and ensure users in the `wheel` group have `sudo`
privileges, and other users do not.  Ensure that uses and attempted uses of
the sudo command are logged.

3. Ensure the root account cannot login to any interfaces (REST APIs, IPMI,
SSH) and does not appear on any list of users (outside of the `/etc`
directory) and cannot be created via the BMC's supported REST API interfaces.

4. Try to delete the pre-created users and then re-create them.  Ensure
authority works as in test variation 1 above.
