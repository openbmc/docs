# User Management - OpenBMC - Design document

## Scope

This document covers the architectural, interface, and design details needed for
user-management components. The implementation detail is beyond the scope of
this document.

## Basic principles in design

1. Use common user-management (e.g. phosphor-user-manager) rather than
   application-based user-management. Especially, avoid IPMI based
   user-management.
2. For security reasons, avoid transmitting passwords over any D-Bus API.
   Observe this rule even while creating, modifying or authenticating the user.
3. Have applications use the PAM module to authenticate the user instead of
   relying on a D-Bus API-based approach.
4. User creation has to be generic in nature wherever possible.
5. As IPMI requires clear text password (Refer IPMI 2.0 specification, section
   13.19-13.33 inclusive for more details), new PAM module (e.g. pam-ipmi
   modules) has to be used along with regular PAM password hashing modules (e.g.
   pam-unix), which will store the password in encrypted form. Implementation
   can elect to skip this if the IPMI daemon is not the part of the distribution
   or if the user created doesn't have an 'ipmi' group role.
6. User name, Password, Group and Privilege roles are maintained by the common
   user-management (e.g. phosphor-user-manager), whereas individual user-related
   settings for any application has to be managed by that application. In other
   words, with the exception of User Name, Password, Group and Privileged Role,
   the rest of the settings needed has to be owned by the application in
   question. (e.g. IPMI daemon has to manage settings like channel based
   restriction etc. for the corresponding user). Design is made to cover this
   scenario.

## Supported Group Roles

The purpose of group roles is to determine the first-level authorization of the
corresponding user. This is used to determine at a high level whether the user
is authorized to the required interface. In other words, users should not be
allowed to login to SSH if they only belong to webserver group and not to group
SSH. Also having group roles in common user-management (e.g.
phosphor-user-manager) allows different application to create roles for the
other (e.g. Administrative user will be able to create a new user through
webserver who will be able to login to webserver/REDFISH & IPMI etc.)

_Note: Group names are for representation only and can be modified/extended
based on the need_

| Sl. No | Group Name  | Purpose / Comments                                                  |
| -----: | ----------- | ------------------------------------------------------------------- |
|      1 | ssh         | Users in this group are only allowed to login through SSH.          |
|      2 | ipmi        | Users in this group are only allowed to use IPMI Interface.         |
|      3 | redfish     | Users in this group are only allowed to use REDFISH Interface.      |
|      4 | web         | Users in this group are only allowed to use webserver Interface.    |
|      5 | hostconsole | Users in this group are only allowed to interact with host console. |

## Supported Privilege Roles

OpenBMC supports privilege roles which are common across all the supported
groups (i.e. User will have same privilege for REDFISH / Webserver / IPMI / SSH
/ HostConsole). User can belong to any one of the following privilege roles at
any point of time.

_Note: Privileges are for representation only and can be modified/extended based
on the need_

| Sl. No | Privilege roles | Purpose / Comments                                                                                                                                                              |
| -----: | --------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
|      1 | admin           | Users are allowed to configure all OpenBMC (including user-management, network and all configurations). Users will have full administrative access.                             |
|      2 | operator        | Users are allowed to view and control basic operations. This includes reboot of the host, etc. But users are not allowed to change other configuration like user, network, etc. |
|      3 | user            | Users only have read access and can't change any behavior of the system.                                                                                                        |
|      4 | no-access       | Users having empty or no privilege will be reported as no-access, from IPMI & REDFISH point of it.                                                                              |

## Common User Manager - D-Bus API (phosphor-user-manager)

User Manager service exposes D-Bus methods for user-management operations. It
exposes `xyz.openbmc_project.User.Manager` as a service and handles objects
through `org.freedesktop.DBus.ObjectManager`. Please refer
https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/yaml/xyz/openbmc_project/User
for detailed user management D-Bus API and interfaces.

## OpenBMC - User Management - High Level architectural diagram

```
                                WEBSERVER/REDFISH
     ========================================================================
    ||  ________________  |  ________________  |   |**********************| ||
    ||  |PAM for user   | |  |Create new user| |   | Redfish specific 1:1 | ||
    ||  |authentication | |  |or delete or   | |   | user settings storage| ||
    ||  |_|_____________| |  |update________|| |   |**********************| ||
    ||====|===============|=================|===========^===================||            Network
          |                                 |           |                          ||**********************||
          V   Storage                       |           |--------------------------|| MaxPrivilege - max   ||
    |**************|********************|   V           ^                          || allowed privilege on ||
    |  pam_unix -  | pam_ipmi- encrypted|   |           |                          || channel              ||
    |  /etc/shadow | password (only if  |   |           |              NET-IPMID   **************************
    |  (hashed) or | user in ipmi group)|   |           |    ===========================       |
    |  pam_ldap    |                    |   |           |    || _____________________  ||      |
    |***********************|***********|   |           |    || | RMCP+ login using  | ||      |
                            +---<-----------------<---------<---- clear text password| ||      |
                                            |           |    || |____________________| ||      |
                                            |           |    ||________________________||      |
                                D-Bus Call  |           |    || _____________________  ||      |
                                +------------+          ^    || | Create new user    | ||      |
                                |                       |    || | or delete or       | ||      |
           Common user manager  |      D-Bus Call       |    || | update             | ||      |
    ||==========================V==||<---------------------<----|(Note: Host-IPMID   | ||      |
    ||     phosphor-user-manager   ||                   |    || | must use same logic| ||      |
    ||                             ||                   |    || |____________________| ||      |
    ||======================|======||                   |    ||                        ||      |
                            V                           |    || |********************| ||      |
                            |                           ^    || | IPMI specific 1:1  | ||      |
                            |                           |    || | user mappings      | ||      |
                            +------>-------------->-----+------>| storage            |<--------|
                            PropertiesChanged /              || | Note: Either Host  | ||
                            InterfacesAdded /                || | / Net IPMID must   | ||
                            InterfacesRemoved /              || | implement signal   | ||
                            UserRenamed Signals              || | handler            | ||
                                                             || |********************| ||
                                                             ||========================||

```

## User management - overview

```
                             user management
          +---------------------------------------------------------+
          |                phosphor-user-manager                    |
          |    +---------------------------------------------+      |
          |    | Local user management:                      |      |
          |    | I: Manager                                  |      |
          |    | M: CreateUser, RenameUser                   |      |
          |    | P: AllPrivileges, AllGroups                 |      |
          |    | S: UserRenamed                              |      |
          |    |                                             |      |
          |    | I: Attributes                               |      |
          |    | PATH: /xyz/openbmc_project/user/<name>      |      |
          |    | P: UserGroups, UserPrivilege, UserEnabled,  |      |
          |    | UserLockedForFailAttempt                    |      |
          |    |                                             |      |
          |    | I: AccountPolicy                            |      |
          |    | P: MaxLoginBeforeLockout, MinPasswordLength |      |
          |    | AccountUnlockTimeout, RememberOldPassword   |      |
          |    |                                             |      |
          |    | General API (Local/Remote)                  |      |
          |    | M: GetUserInfo()                            |      |
          |    |                                             |      |
          |    +---------------------------------------------+      |
          |                                                         |
          |        Remote User Management - Configuration           |
          |    +--------------------------+------------------+      |
          |    |  Provides interface for remote              |      |
          |    |  user management configuration              |      |
          |    |  (LDAP / NIS / KRB5)                        |      |
          |    +---------------------------------------------+      |
          |                                                         |
          +---------------------------------------------------------+

```

## OpenBMC - User Management - User creation from webserver flow - with all groups

```
------------------------------------|---------------------------------- -|-----------------------------|
WEBSERVER                           | Common User Manager                |   IPMI & REDFISH(webserver) |
------------------------------------|------------------------------------|-----------------------------|
1.Request to add new user           |                                    |                             |
with 'ipmi, redfish' Group.         |                                    |                             |
Webserver sends D-Bus command       |                                    |                             |
to user-manager with User Name,     |                                    |                             |
Groups and Privilege (No Password)  |                                    |                             |
                             (REQ)--------->                             |                             |
                                    | 2. Validate inputs, including      |                             |
                                    | group restrictions.                |                             |
                                    | if successful go to step 4, else   |                             |
                                    | return a corresponding error, (e.g.|                             |
                                    | too many users, user name is too   |                             |
                                    | long (say, more than IPMI required |                             |
                                    | 16 bytes etc.)                     |                             |
                            <-------------(FAILURE)                      |                             |
3. Throw error to the user          |                                    |                             |
                                    |                                    |                             |
                                    | 4. Add User Name, Groups and       |                             |
                                    | Privileges accordingly. Send signal|                             |
                                    | for User Name created         ---(SIGNAL)->                      |
                                    | (InterfacesAdded)                  | 5. IPMI & REDFISH can       |
                                    |                                    | register for 'UserUpdate'   |
                                    |                                    | signal handler and use      |
                                    |                                    | the same to maintain 1:1    |
                                    |                                    | mapping, if required        |
                            <-------------(SUCCESS)                      |                             |
6. User created successfully but    |                                    |                             |
still can't login, as there is no   |                                    |                             |
password set.                       |                                    |                             |
                                    |                                    |                             |
7.Set the password for              |                                    |                             |
the user using pam_chauthtok()      |                                    |                             |
(which will store clear-text        |                                    |                             |
password in encrypted form, if      |                                    |                             |
user is part of 'ipmi' Group)       |                                    |                             |
                                    |                                    |                             |
8. User created successfully        |                                    |                             |
--------------------------------------------------------------------------------------------------------
```

## OpenBMC - User Management - User creation from IPMI - 'ipmi' Group only

```
------------------------------------|---------------------------------- -|-----------------------------|
IPMI                                | Common User Manager                |   pam_unix/pam_ipmi storage |
------------------------------------|------------------------------------|-----------------------------|
1. User sends Set User Name command.|                                    |                             |
IPMI Sends D-Bus command to         |                                    |                             |
to user-manager with User Name      |                                    |                             |
Groups & privileges (No Password) (REQ)--------->                        |                             |
(Note: Configurations for other     |                                    |                             |
commands like Set User Access /     |                                    |                             |
Channel access has to be stored     |                                    |                             |
in IPMI NV along with User Name)    |                                    |                             |
                                    | 2. Validate inputs, including      |                             |
                                    | group restrictions.                |                             |
                                    | if success go to step 4, else      |                             |
                                    | return a corresponding error.      |                             |
                            <-------------(FAILURE)                      |                             |
3. Return error to the Set User     |                                    |                             |
Name command. User creation failed. |                                    |                             |
                                    | 4. Add User Name, Groups and       |                             |
                                    | Privileges accordingly. Send signal|                             |
                                    | for User Name created              |                             |
                            <-------------(SIGNAL) (InterfacesAdded)     |                             |
5. IPMI can register for            |                                    |                             |
'UserUpdate' signal handler and use |                                    |                             |
the same to maintain 1:1 mapping,   |                                    |                             |
if required                         |                                    |                             |
                            <-------------(SUCCESS)                      |                             |
6. User sends Set User Password     |                                    |                             |
command. IPMI uses pam-ipmi         |                                    |                             |
to set the password.                |                                    |                             |
(Note: can't accept password without|                                    |                             |
Set User Name command in first      |                                    |                             |
place as User Name has to exist     |                                    |                             |
before trying to store the password.|                                    |                             |
Implementation can elect to accept  |                                    |                             |
the password and set it through PAM |                                    |                             |
after Set User Name command)   (SET)----------------------------------------> pam-ipmi will store      |
                                    |                                    |clear text password in       |
                                    |                                    |encrypted form along with    |
                                    |                                    |hashed password by pam-unix, |
                                    |                                    |when the user belongs to IPMI|
                            <----------------------------------------------(SUCCESS)                   |
7. User created successfully        |                                    |                             |
but will allow only when user is    |                                    |                             |
enabled. IPMI shouldn't allow user  |                                    |                             |
to login if user is disabled        |                                    |                             |
(Note: stored in IPMI NV).          |                                    |                             |
--------------------------------------------------------------------------------------------------------
```

## OpenBMC - User Management - User deletion from webserver flow - with all groups

```
------------------------------------|---------------------------------- -|-----------------------------|
WEBSERVER                           | Common User Manager                |   IPMI & REDFISH(webserver) |
------------------------------------|------------------------------------|-----------------------------|
1.Request to delete existing user   |                                    |                             |
with 'ipmi, redfish' group          |                                    |                             |
Webserver sends D-Bus command       |                                    |                             |
to user-manager with User Name      |                                    |                             |
to be deleted            (REQ)--------->                                 |                             |
                                    | 2. Validate inputs, including      |                             |
                                    | group restrictions.                |                             |
                                    | if successful go to step 4, else   |                             |
                                    | return corresponding error.        |                             |
                                    | Can be user does not exist etc.    |                             |
                            <-------------(FAILURE)                      |                             |
3. Throw error to the user          |                                    |                             |
                                    |                                    |                             |
                                    | 4. Delete User Name, Group and     |                             |
                                    | privileges along with password.    |                             |
                                    | User Name deleted             ---(SIGNAL)->                      |
                                    | (InterfacesRemoved)                | 5. IPMI & REDFISH can       |
                                    |                                    | register for 'UserUpdate'   |
                                    |                                    | signal handler and use      |
                                    |                                    | the same to maintain 1:1    |
                                    |                                    | mapping, if required        |
                                    |                                    | IPMI must delete stored     |
                                    |                                    | encrypted password.         |
                            <-------------(SUCCESS)                      |                             |
6. User deleted successfully,       |                                    |                             |
                                    |                                    |                             |
--------------------------------------------------------------------------------------------------------
```

## OpenBMC - User Management - User deletion from IPMI - 'ipmi' Group only

```
------------------------------------|---------------------------------- -|
IPMI                                | Common User Manager                |
------------------------------------|------------------------------------|
1. User sends Set User Name command |                                    |
to clear user name. Send D-Bus API  |                                    |
to user-manager with User Name      |                                    |
to delete                      (REQ)--------->                           |
(Note: Configurations for other     |                                    |
commands like Set User Access /     |                                    |
Channel access has to be stored     |                                    |
in IPMI NV along with User Name)    |                                    |
                                    | 2. Validate inputs, including      |
                                    | group restrictions.                |
                                    | if successful go to step 4, else   |
                                    | return a corresponding error (e.g. |
                                    | User name doesn't exists etc.)     |
                            <-------------(FAILURE)                      |
3. Return error to the Set User     |                                    |
Name command. User deletion failed. |                                    |
                                    |                                    |
                            <-------------(SIGNAL) 4. User Name deleted  |
5. IPMI can register for            | (InterfacesRemoved)                |
'UserUpdate' signal handler and use |                                    |
the same to maintain 1:1 mapping,   |                                    |
if required                         |                                    |
                            <-------------(SUCCESS)                      |
6. User deleted successfully        |                                    |
                                    |                                    |
--------------------------------------------------------------------------
```

## Authentication flow

Applications must use `pam_authenticate()` API to authenticate user. Stacked PAM
modules are used such that `pam_authenticate()` can be used for both local &
remote users.

```
                +----------------------------------+
                |    Stacked PAM Authentication    |
                |     +-----------------------+    |
                |     | pam_tally2.so         |    |
                |     | user failed attempt   |    |
                |     | tracking module.      |    |
                |     +-----------------------+    |
                |                                  |
                |     +-----------------------+    |
                |     | pam_unix.so / local   |    |
                |     | user authentication   |    |
                |     | module.               |    |
                |     +-----------------------+    |
                |                                  |
                |     +-----------------------+    |
                |     | nss_pam_ldap.so / any |    |
                |     | remote authentication |    |
                |     | pam modules           |    |
                |     +-----------------------+    |
                +----------------------------------+
```

## Password update

Applications must use `pam_chauthtok()` API to set / change user password.
Stacked PAM modules allow all 'ipmi' group user passwords to be stored in
encrypted form, which will be used by IPMI. The same has been performed by
`pam_ipmicheck` and `pam_ipmisave` modules loaded as first & last modules in
stacked pam modules.

```
                +------------------+---------------+
                |      Stacked PAM - Password      |
                |                                  |
                |  +----------------------------+  |
                |  | pam_cracklib.so.           |  |
                |  | Strength checking the      |  |
                |  | password for acceptance    |  |
                |  +----------------------------+  |
                |                                  |
                |  +----------------------------+  |
                |  | pam_ipmicheck.so. Checks   |  |
                |  | password acceptance for    |  |
                |  | 'ipmi' group users         |  |
                |  +----------------------------+  |
                |                                  |
                |  +----------------------------+  |
                |  | pam_pwhistory.so. Checks   |  |
                |  | to avoid last used         |  |
                |  | passwords                  |  |
                |  +----------------------------+  |
                |                                  |
                |  +-------------+--------------+  |
                |  | pam_unix.so - to update    |  |
                |  | local user's password      |  |
                |  |                            |  |
                |  +----------------------------+  |
                |                                  |
                |  +-----------------+----------+  |
                |  | pam_ipmisave.so - stores   |  |
                |  | 'ipmi' group user's        |  |
                |  | password in encrypted form |  |
                |  +----------------------------+  |
                |                                  |
                +----------------------------------+
```

## Authorization flow (except IPMI)

```
                                +
                                |
                                |
                  +-------------v--------------+
                  |pam_authenticate() to       |
                  |authenticate the user       |
                  |(local / remote)            |
                  +-------------+--------------+
                                |
                                |
                  +-------------v--------------+
                  |Read user properties using  |
                  |GetUserInfo() (for local &  |
                  |remote users).              |
                  |Allow group access based on |
                  |group property              |
                  +-------------+--------------+
                                |
                                |
                  +-------------v--------------+
                  |Read Channel MaxPrivilege   |
                  |from /xyz/openbmc_project/  |
                  |network/ethX. Use the       |
                  |minimum of user & channel   |
                  |privilege as the privilege  |
                  |Note: Implementation can    |
                  |elect to skip the same, if  |
                  |authorization based on      |
                  |channel restriction is not  |
                  |needed.                     |
                  +----------------------------+


```

## LDAP

SSH, Redfish, Webserver and HostConsole interface allows the user to
authenticate against an LDAP directory. IPMI interface cannot be used to
authenticate against LDAP, since IPMI needs the password in clear text at the
time of session setup.

In OpenBMC, PAM based authentication is implemented, so for both LDAP users and
local users, the authentication flow is the same.

For the LDAP user accounts, there is no LDAP attribute type that corresponds to
the OpenBMC privilege roles. The preferred way is to group LDAP user accounts
into LDAP groups. D-Bus API is provided for the user to assign privilege role to
the LDAP group.

## Authorization Flow

This section explains how the privilege roles of the user accounts are consumed
by the webserver interface. The privilege role is a property of the user D-Bus
object for the local users. For the LDAP user accounts, the privilege role will
be based on the LDAP group. The LDAP group to privilege role mapping needs to be
configured prior to authenticating with the LDAP user accounts.

1. Invoke PAM API for authenticating with user credentials. Proceed, if the
   authentication succeeds.
2. Check if the user is a local user account. If the user account is local,
   fetch the privilege role from the D-Bus object and update the session
   information.
3. If the user account is not local, read the group name for the user.
4. Fetch the privilege role corresponding to the group name, update the session
   information with the privilege role.
5. If there is no mapping for group name to privilege role, default to `user`
   privilege role for the session.

## Recommended Implementation

1. As per IPMI spec the max user list can be 15 (+1 for NULL User). Hence
   implementation has to be done in such a way that no more than 15 users are
   getting added to the 'ipmi' Group. Phosphor-user-manager has to enforce this
   limitation.
2. Should add IPMI_NULL_USER by default and keep the user in disabled state.
   This is to prevent IPMI_NULL_USER from being created as an actual user. This
   is needed as NULL user with NULL password in IPMI can't be added as an entry
   from Unix user-management point of it.
3. User creation request from IPMI / REDFISH must be handled in the same manner
   as described in the above flow diagram.
4. Adding / removing a user name from 'ipmi' Group role must force a Password
   change to the user. This is needed as adding to the 'ipmi' Group of existing
   user requires clear text password to be stored in encrypted form. Similarly
   when removing a user from IPMI group, must force the password to be changed
   as part of security measure.
5. IPMI spec doesn't support groups for the user-management. Hence the same can
   be implemented through OEM Commands, thereby creating a user in IPMI with
   different group roles.
6. Do no use 'Set User Name' IPMI command to extend already existing non-ipmi
   group users to 'ipmi' group. 'Set User Name' IPMI command will not be able to
   differentiate between new user request or request to extend an existing user
   to 'ipmi' group. Use OEM Commands to extend existing users to 'ipmi' group.
   Note: 'Set User Name' IPMI command will return CCh 'Invalid data field in
   Request' completion code, if tried to add existing user in the system.

## Deployment - Out of factory

### Guidelines

As per
[SB-327 Information Privacy](https://leginfo.legislature.ca.gov/faces/billTextClient.xhtml?bill_id=201720180SB327),
Connected devices must avoid shipping with generic user name & password. The
reasonable security expected is

1. Preprogrammed password unique to each device
2. Forcing user to generate new authentication account, before using the device.

### Generating user during deployment:

To adhere above mentioned guideline and to make OpenBMC more secure, this design
specifies about forcing end-user to generate a new account, during deployment
through any of the system in-band interfaces (like KCS etc.). IPMI 2.0
specification provides commands like `SetUserName`, `SetUserPassword`,
`SetUserAccess`, which must be used to create a new user account instead of
using any generic default user name and password. Accounts created through this
method have access to IPMI, REDFISH & Webserver and can be used to create more
accounts through out-of-band interfaces.

### Special user - root – user id 0:

Exposing root account (user id 0) to end-user by default (other than debug /
developer scenario) is security risk. Hence current architecture recommends not
to enable root user by default for end-user. For general login for debug /
developer builds, a new default user with password can be created by specifying
the same in local.conf.sample file. This can be used to establish a session by
default (CI systems etc. can use this account). From OpenBMC package user name
`openbmc` with password `0penBmc$` can be added.

#### Debugging use-case

`root` user / sudo privilege access are required during development / debug
phase of the program. For this purpose a new IPMI OEM command (TBD) / REDFISH
OEM action(TBD) to can be used to set password for the root user, after which
`root` user can be used to login to the serial console and for further debugging
(Note: `root` user will not be listed as user account in any interfaces like
IPMI / REDFISH from user management point of view).

### Deployment for systems without in-band interfaces:

Any systems which doesn’t have in-band system interface can generate passwords
uniquely for each & every device or can expose a default user name & password
forcing end-user to update the same, before using the device (TBD).
