# User Management - OpenBMC - Design document #

## Scope ##
This document covers the architectural, interface, and design details needed for
user-management components. The implementation detail is beyond the scope of
this document.

## Basic principles in design ##
1. Use common user-management (e.g. phosphor-user-manager) rather than
application-based user-management. Especially, avoid IPMI based user-management.
2. For security reasons, avoid transmitting passwords over any D-Bus API.
Observe this rule even while creating, modifying or authenticating the user.
3. Have applications use the PAM module to authenticate the user instead of
relying on a D-Bus API-based approach.
4. User creation has to be generic in nature wherever possible.
5. As IPMI requires clear text password, new module (e.g. openbmc-password) has
to be used instead of common-password PAM, which will also store the password
in encrypted form along with storing the hashed password in /etc/shadow file.
Implementation can elect to skip this if the IPMI daemon is not the part of the
distribution or if the user created doesn't have an 'ipmi' group role.
6. User name, Password, Group and Privilege roles are maintained by the common
user-management (e.g. phosphor-user-manager), whereas individual user-related
settings for any application has to be managed by that application. In other
words, with the exception of User Name, Password, Group and Privileged Role,
the rest of the settings needed has to be owned by the application in question.
(e.g. IPMI daemon has to manage settings like channel based restriction etc.
for the corresponding user). Design is made to cover this scenario.

## Supported Group Roles ##
The purpose of group roles is to determine the first-level authorization of the
corresponding user. This is used to determine at a high level whether the user
is authorized to the required interface.
In other words, users should not be allowed to login to SSH if they only belong
to webserver group and not to group SSH. Also having group roles in common
user-management (e.g. phosphor-user-manager) allows different application to
create roles for the other (e.g. Administrative user will be able to create a
new user through webserver who will be able to login to webserver/REDFISH &
IPMI etc.)

*Note: Group names are for representation only and can be modified/extended
 based on the need*

|Sl. No| Group Name | Purpose / Comments                |
|-----:|------------|-----------------------------------|
|1     | ssh        | Users in this group are only allowed to login through SSH.|
|2     | ipmi       | Users in this group are only allowed to use IPMI Interface.|
|3     | redfish    | Users in this group are only allowed to use REDFISH Interface.|
|4     | web        | Users in this group are only allowed to use webserver Interface.|

## Supported Privilege Roles
OpenBMC supports privilege roles which are common across all the supported
groups (i.e. User will have same privilege for REDFISH / Webserver / IPMI /
SSH).  User can belong to any one of the following privilege roles at any point
of time.

*Note: Privileges are for representation only and can be modified/extended
 based on the need*

|Sl. No| Privilege roles | Purpose / Comments                |
|-----:|-----------------|-----------------------------------|
|1     | admin           | Users are allowed to configure all OpenBMC (including user-management, network and all configurations). Users will have full administrative access.|
|2     | operator        | Users are allowed to view and control basic operations. This includes reboot of the host, etc. But users are not allowed to change other configuration like user, network, etc.|
|3     | user            | Users only have read access and can't change any behavior of the system.|
|4     | callback        | Lowest privilege level, only allowed to initiate IPMI 2.0 specific callbacks.|

## Common User Manager - D-Bus API (phosphor-user-manager) ##
User Manager service exposes D-Bus methods for user-management operations.
It exposes `xyz.openbmc_project.User.Manager` as a service and handles
objects through `org.freedesktop.DBus.ObjectManager`.
### User Manager Interface
User manager interface `xyz.openbmc_project.User.Mgr` provides following
methods, properties and signals.

|Sl. No| Method /<br> Property /<br> Signal | Parameters | Return Codes / Error / Data | Comments |
|-----:|-------------------------|-------------------------|------------|----------------------|----------|
|1     |CreateUser (Method)      | **UserName** - STRING <br> **GroupName** - STRING[] - List of groups to which the user should be added <br> **Privilege** - STRING - Privilege of the user to be added. <br> **Enabled** - BOOL - true / false to indicate enabled / disabled state. Note: Implementation can elect to accept NULL (empty) Privilege. | Success<br>UserNameExists<br>UserNameGrpPrivFail<br>NoResource<br>InvalidArgument<br>InternalFailure<br>InsufficientPermission|1. UserName has to be unique (if UserName already exists, then UserNameExists will be thrown)<br>2. If 'ipmi' Group role is chosen, then UserName should not exceed 16 bytes otherwise UserNamePrivFail will be thrown |
|2     |RenameUser (Method)      | **UserName** - STRING <br> **NewUserName** - STRING | Success<br>UserNameDoesNotExist<br>UserNameExists<br>InvalidArgument<br>InternalFailure<br>InsufficientPermission||
|3     |AllGroups (Property)     |             | **Groups** - STRING[] - Lists all supported Groups in the system. Only Groups in this list will be accepted for AddUser method | |
|4     |AllPrivileges (Property) |             | **Privileges** - STRING[] - List all supported privileges in the system. One privilege in this list or NULL(empty) will be accepted for AddUser method | |
|5     |UserRenamed (Signal)     | **UserName** -STRING <br> **NewUserName** -STRING ||Special signal sent out when user is renamed along with InterfacesRemoved & InterfacesAdded signals|

### Users Interface
User manager daemon will create Users objects for each and every user existing
in the system with object path as follows:
`xyz.openbmc_project.User.Users.<user name>`
Each and every Users object will have the following properties associated with
it and the objects can be handled through `org.freedesktop.DBus.ObjectManager`

|Sl. No| Method /<br> Property /<br> Signal | Privilege <br>Required  | Parameters | Return Codes / Error / Data | Comments |
|-----:|--------------------------|-------------------------|------------|----------------------|----------|
|1     |DeleteUser (Method)       | USER-MANAGER            |            |                      | Deletes the current user.|
|2     |UserPrivilege (Property)  | USER-MANAGER            |            | **Privilege** - STRING - User privilege. ||
|3     |UserGroups (Property)     | USER-MANAGER            |            | **GroupNames** - STRING[] - Groups to which user belongs. ||
|4     |UserEnabled (Property)    | USER-MANAGER            |            | **Enabled**- BOOL - User enabled / disabled. |||

## OpenBMC - User Management - High Level architectural diagram ##


```
                                WEBSERVER/REDFISH
     ========================================================================
    ||  ________________  |  ________________  |   |**********************| ||
    ||  |PAM for user   | |  |Create new user| |   | Redfish specific 1:1 | ||
    ||  |authentication | |  |or delete or   | |   | user settings storage| ||
    ||  |_|_____________| |  |update________|| |   |**********************| ||
    ||====|===============|=================|===========^===================||
          |                                 |           |
          V   Storage                       |           |
    |**************|********************|   V           ^              NET-IPMID
    |  pam_unix -  | pam_ipmi- encrypted|   |           |    ===========================
    |  /etc/shadow | password (only if  |   |           |    ||                        ||
    |  (hashed)    | user in ipmi group)|   |           |    || _____________________  ||
    |***********************|***********|   |           |    || | RMCP+ login using  | ||
                            +---<-----------------<---------<---- clear text password| ||
                                            |           |    || |____________________| ||
                                            |           |    ||________________________||
                                D-Bus Call  |           |    || _____________________  ||
                                +------------+          ^    || | Create new user    | ||
                                |                       |    || | or delete or       | ||
           Common user manager  |      D-Bus Call       |    || | update             | ||
    ||==========================V==||<---------------------<----|(Note: Host-IPMID   | ||
    ||     phosphor-user-manager   ||                   |    || | must use same logic| ||
    ||                             ||                   |    || |____________________| ||
    ||======================|======||                   |    ||                        ||
                            V                           |    || |********************| ||
                            |                           ^    || | IPMI specific 1:1  | ||
                            |                           |    || | user mappings      | ||
                            +------>-------------->-----+------>| storage            | ||
                            PropertiesChanged /              || | Note: Either Host  | ||
                            InterfacesAdded /                || | / Net IPMID must   | ||
                            InterfacesRemoved /              || | implement signal   | ||
                            UserRenamed Signals              || | handler            | ||
                                                             || |********************| ||
                                                             ||========================||

```

## OpenBMC - User Management - User creation from webserver flow - with all groups ##

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
                                    | return corresponding error.        |                             |
                                    | Can be exhausted user list or      |                             |
                                    | or User Name greater than 16 bytes |                             |
                                    | etc.                               |                             |
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

## OpenBMC - User Management - User creation from IPMI - 'ipmi' Group only ##

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
                                    | return corresponding error.        |                             |
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
                                    |                                    |hashed password, when the    |
                                    |                                    |user belongs to IPMI Group   |
                            <----------------------------------------------(SUCCESS)                   |
7. User created successfully        |                                    |                             |
but will allow only when user is    |                                    |                             |
enabled. IPMI Shouldn't allow user  |                                    |                             |
to login if user is disabled        |                                    |                             |
(Note: stored in IPMI NV).          |                                    |                             |
--------------------------------------------------------------------------------------------------------
```

## OpenBMC - User Management - User deletion from webserver flow - with all groups ##

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

## OpenBMC - User Management - User deletion from IPMI - 'ipmi' Group only ##

```
------------------------------------|---------------------------------- -|-----------------------------|
IPMI                                | Common User Manager                |   openbmc-pam storage       |
------------------------------------|------------------------------------|-----------------------------|
1. User sends Set User Name command |                                    |                             |
to clear user name. Send D-Bus API  |                                    |                             |
to user-manager with User Name      |                                    |                             |
to delete                      (REQ)--------->                           |                             |
(Note: Configurations for other     |                                    |                             |
commands like Set User Access /     |                                    |                             |
Channel access has to be stored     |                                    |                             |
in IPMI NV along with User Name)    |                                    |                             |
                                    | 2. Validate inputs, including      |                             |
                                    | group restrictions.                |                             |
                                    | if successful go to step 4, else   |                             |
                                    | return corresponding error.        |                             |
                                    | Can be user does not exist etc.    |                             |
                            <-------------(FAILURE)                      |                             |
3. Return error to the Set User     |                                    |                             |
Name command. User deletion failed. |                                    |                             |
                                    |                                    |                             |
                            <-------------(SIGNAL) 4. User Name deleted  |                             |
5. IPMI can register for            | (InterfacesRemoved)                |                             |
'UserUpdate' signal handler and use |                                    |                             |
the same to maintain 1:1 mapping,   |                                    |                             |
if required                         |                                    |                             |
                            <-------------(SUCCESS)                      |                             |
6. User deleted successfully        |                                    |                             |
                                    |                                    |                             |
--------------------------------------------------------------------------------------------------------
```


## Recommended Implementation ##
1. As per IPMI spec the max user list can be 15 (+1 for NULL User). Hence
implementation has to be done in such a way that no more than 15 users are
getting added to the 'ipmi' Group. Phosphor-user-manager has to enforce this
limitation.
2. Should add IPMI_NULL_USER by default and keep the user in disabled state.
This is to prevent IPMI_NULL_USER from being created as an actual user. This is
needed as NULL user with NULL password in IPMI  can't be added as an entry from
Unix user-management point of it.
3. User creation request from IPMI / REDFISH must be handled in
the same manner as described in the above flow diagram.
4. Adding / removing a user name from 'ipmi' Group role must force a Password
change to the user. This is needed as adding to the 'ipmi' Group of existing
user requires clear text password to be stored in encrypted form. Similarly
when removing a user from IPMI group, must force the password to be changed
as part of security measure.
5. IPMI spec doesn't support groups for the user-management. Hence the
same can be implemented through OEM Commands, thereby creating a user in
IPMI with different group roles.
6. Recommend not to add user in 'ipmi' group through IPMI interface
which already exists as part of other group roles (except IPMI) through regular
Set User Name command. This is to avoid confusion, as existing IPMI commands
won't be able to convey / treat the request as either new user request or
request to extend existing user name to IPMI Group. Extend of IPMI Group to
existing user, through IPMI interface is allowed only through OEM Commands.
