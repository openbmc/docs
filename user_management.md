# User Management - OpenBMC - Design document #

## Scope ##
This document covers the architectural, interface, and design details needed for
user management components. The implementation detail is beyond the scope of
this document.

## Basic principle in design ##
1. To use common user management (phosphor-user-manager) across applications
instead of any application centric approach (esp. Instead of IPMI centric user
management).
2. For security reasons, password over any D-Bus API has to be avoided. This
will be the case even during creating / modifying / authenticating the user.
3. Applications must use the PAM module in order to authenticate the user,
instead of relying on a D-Bus API based approach. There is no plan to provide
a D-Bus API to authenticate the user.
4. User creation has to be generic in nature wherever possible (with
few exceptions).
5. As IPMI requires clear text password, new module (say openbmc-password) has
to be used instead of common-password PAM, which will also store the password
in encrypted form along with storing the hashed password in /etc/shadow file.
Implementation can elect to skip this, if the IPMI daemon is not the part of the
distribution or if the user created doesn't have an IPMI group role.
6. User name, Password, and  Group roles are maintained by the common user
management (phosphor-user-manager), where-as individual user related settings
for any application has to be managed by that application. I.e. other than,
user name, password and the group privileged role, rest of the settings needed,
has to be owned by that application. (e.g. IPMI daemon has to manage setting's
like channel based restriction etc. for the corresponding user). Design is
made to cover this scenario.

## Group Roles ##
Purpose of Group roles is to determine the first level authorization of the
corresponding user. This is used to determine in high level whether the user
is authorized for the required interface.
I.e. User should not be allowed to login to SSH, if they only belong to
webserver group and not to group SSH. Also having group roles in
common user management (phosphor-user-manager) allows different application to
create roles for the other (e.g. Administrative user will be able to create a
new user through webserver who will be able to login to webserver/REDFISH
& IPMI etc.)

*Note: Group names are for representation only and can be modified/extended
 based on the need*

|Sl. No| Group Name    | Purpose / Comments                |
|-----:|---------------|-----------------------------------|
|1     | NETWORKD      | Users belonging to this group are allowed to make change in the network <br> *Note: IPMI / REDFISH doesn't need to depend on the same and can directly depend on its group role to determine the ADMIN / USER / other privileges* |
|2     | SSH           | Users in this group are only allowed to use SSH   |
|3     | USER-MANAGER  | Users belonging to this group are allowed to create / delete users <br> *Note: IPMI / REDFISH doesn't need to depend on the same and can directly depend on its group role to determine the ADMIN / USER / other privileges* |
|4     | IPMI | Users in this group are only allowed to use IPMI Interface. Privilege of the user has to be determined and maintained by IPMI itself |
|5     | REDFISH | Users in this group are only allowed to use REDFISH Interface. Privilege of the user has to be determined and maintained by REDFISH itself |
|6     |root/others   | No user is allowed to be in this group              |

## Common User Manager - D-Bus API (phosphor-user-manager) ##

|Sl. No| Method /<br> Signal Name | Privilege <br>Required  | Parameters | Return Codes / Error | Comments |
|-----:|--------------------------|-------------------------|------------|----------------------|----------|
|1     |CREATE_USER               | USER-MANAGER            | **UserName** - STRING <br> **Roles** - STRING[] - List of Group roles | SUCCESS<br>ERR_USERNAME_EXIST<br>ERR_USERNAME_ROLE_FAIL<br>ERR_NO_RESOURCE<br>ERR_UNKNOWN<br>ERR_AUTHORIZATION_FAIL|1. UserName has to be unique (if UserName already exists, then ERR_USERNAME_EXIST will be thrown)<br>2. If IPMI Group role is chosen, then UserName should not exceed 16 bytes |
|2     |DELETE_USER               | USER-MANAGER            | **UserName** - STRING | SUCCESS<br>ERR_USERNAME_DOESNT_EXIST<br>ERR_UNKNOWN<br>ERR_AUTHORIZATION_FAIL||
|3     |CHANGE_USER_ROLE          | USER-MANAGER            | **UserName** - STRING <br> **CurrentRoles** - STRING[] - List of Group Roles <br> **UpdatedRoles** - STRING[] - List of Group roles to which the change has to be made | SUCCESS<br>ERR_USERNAME_DOESNT_EXIST<br>ERR_UNKNOWN<br>ERR_USERNAME_ROLE_FAIL<br>ERR_NO_RESOURCE<br><br>ERR_AUTHORIZATION_FAIL| |
|4     |List User Details         | USER-Manager            | **UserName** - STRING | **UserName** - STRING <br> **Roles** - STRING[] <br>*ReturnCodes:*<br> SUCCESS<br>ERR_USERNAME_DOESNT_EXIST<br>ERR_UNKNOWN<br>ERR_AUTHORIZATION_FAIL|If requested user name is null then all User details will be returned in [] format |
|5     |LIST_GROUP_DETAILS        | USER-MANAGER            | N/A        | **Roles** - STRING[] - List of Group roles available | |
|6     |USER_UPDATED - **SIGNAL** |                         | **UserName** <br> **Roles** <br> Signal - CREATED/DELETED/UPDATED||||

## OpenBMC - User Management - High Level architectural diagram ##


```
                                                           REDFISH
                                 ========================================================================
                                ||  ________________  |  ________________  |   |**********************| ||
      WEBSERVER                 ||  |PAM for user   | |  |Create new user| |   | Redfish specific 1:1 | ||
=======================         ||  |authentication | |  |or delete or   | |   | user settings storage| ||
|| ________________  ||         ||  |_|_____________| |  |update________|| |   |**********************| ||
|| |PAM for user   | ||         ||====|===============|=================|===========^===================||
|| |authentication | ||               |                                 |           |
|| |or password    | ||               V    openbmc-pam - storage        |           |              NET-IPMID
|| |update         | ||         |**************|********************|   |           |    ===========================
|| |_______________------------>|  /etc/shadow | encrypted_passwd   |   |           |    ||                        ||
||___________________||         |  (hashed)    | (only if IPMI used)|   |           |    || _____________________  ||
|| ________________  ||         |***********************|***********|   |           |    || | RMCP+ login using  | ||
|| |Create new user| ||                                 +---<-----------------<---------<---- clear text password| ||
|| |or delete or   | ||                                                 |           |    || |____________________| ||
|| |update         -----------------+                                   |           |    ||________________________||
|| |_______________| ||             |                                   |           |    || _____________________  ||
||                   ||             |                       +------------+          ^    || |Create new user     | ||
||                   ||             |                       |                       |    || |or delete or        | ||
||===================||             |  Common user manager  |                       |    || |update              | ||
                                ||==V=======================V==||<---------------------<-----____________________| ||
                                ||     phosphor-user-manager   ||                   |    ||________________________||
                                ||                             ||                   |    ||                        ||
                                ||======================|======||                   |    || |********************| ||
                                                        |                           |    || | IPMI specific 1:1  | ||
                                                        |                           |    || | user mappings      | ||
                                                        +------>--------------------+------>| storage            | ||
                                                                                         || |********************| ||
                                                                                         ||========================||

```

## OpenBMC - User Management - User creation from webserver flow - with all groups ##

```
------------------------------------|---------------------------------- -|-----------------------------|
WEBSERVER                           | Common User Manager                |   IPMI & REDFISH(webserver) |
------------------------------------|------------------------------------|-----------------------------|
1.Request to add new user           |                                    |                             |
with IPMI & REDFISH Group.          |                                    |                             |
Webserver sends D-Bus command       |                                    |                             |
to user-manager with User Name      |                                    |                             |
and Roles (No Password)  (REQ)--------->                                 |                             |
                                    | 2. If IPMI / REDFISH roles         |                             |
                                    | requested then send D-Bus command  |                             |
                                    | to IPMI / REDFISH with User Name   |                             |
                                    | and Roles requested (after basic   |                             |
                                    | verification), else go to step 6 ---(REQ)->                      |
                                    |                                    | 3. Both IPMI & REDFISH      |
                                    |                                    | tries to add the same along |
                                    |                                    | with other settings if any  |
                                    |                                    | (any 1:1 mappings has to be |
                                    |                                    | done here)                  |
                                    |                                    | Note: can elect to have temp|
                                    |                                    | update at this point of time|
                                    |                                    | and clean database if       |
                                    |                                    | there is no signal.         |
                                    |                               <-----(SUCCESS/FAILURE)--          |
                                    | 4. If failure throw corresponding  |                             |
                                    | error. Can be exhausted IPMI user  |                             |
                                    | list or IPMI username greater than |                             |
                                    | 16 bytes etc. If successful        |                             |
                                    | go to step 6                       |                             |
                            <-------------(FAILURE)                      |                             |
5. Throw error to the user          |                                    |                             |
                                    |                                    |                             |
                                    | 6. Add User Name and Group roles   |                             |
                                    | accordingly. Send signal for       |                             |
                                    | User Name created             ---(SIGNAL)->                      |
                                    |                                    |                             |
                            <-------------(SUCCESS)                      |                             |
6. User created successfully but    |                                    |                             |
still can't login, as there is no   |                                    |                             |
password set.                       |                                    |                             |
                                    |                                    |                             |
7.Set the password for              |                                    |                             |
the user using pam_chauthtok()      |                                    |                             |
(which will store clear-text        |                                    |                             |
password in encrypted form, if      |                                    |                             |
user is part of IPMI Group)         |                                    |                             |
                                    |                                    |                             |
8. User created successfully        |                                    |                             |
--------------------------------------------------------------------------------------------------------
```

## OpenBMC - User Management - User creation from IPMI - IPMI Group only ##

```
------------------------------------|---------------------------------- -|-----------------------------|
IPMI                                | Common User Manager                |   openbmc-pam storage       |
------------------------------------|------------------------------------|-----------------------------|
1. User sends Set User Name command.|                                    |                             |
IPMI Sends D-Bus command to         |                                    |                             |
to user-manager with User Name      |                                    |                             |
and Roles (IPMI) (No Password) (REQ)--------->                           |                             |
(Note: Configurations for other     |                                    |                             |
commands like Set User Access /     |                                    |                             |
Channel access has to be stored     |                                    |                             |
in IPMI NV along with User Name)    |                                    |                             |
                                    | 2. User-Manager adds the new user  |                             |
                                    | with roles (ONLY IPMI).            |                             |
                                    | Return error if User Name already  |                             |
                                    | exists, else return success        |                             |
                            <-------------(FAILURE)                      |                             |
3. Return error to the Set User     |                                    |                             |
Name command. User creation failed. |                                    |                             |
                            <-------------(SUCCESS)                      |                             |
4. User sends Set User Password     |                                    |                             |
command. IPMI uses openbmc-pam      |                                    |                             |
to set the password.                |                                    |                             |
(Note: can't accept password without|                                    |                             |
Set User Name command in first      |                                    |                             |
place as User Name has to exist     |                                    |                             |
before trying to store the password.|                                    |                             |
Implementation can elect to accept  |                                    |                             |
the password and set it through PAM |                                    |                             |
after Set User Name command)   (SET)----------------------------------------> pam-openbmc will store   |
                                    |                                    |clear text password in       |
                                    |                                    |encrypted form along with    |
                                    |                                    |hashed password, when the    |
                                    |                                    |user belongs to IPMI Group   |
                            <----------------------------------------------(SUCCESS)                   |
5. User created successfully        |                                    |                             |
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
with IPMI & REDFISH Group           |                                    |                             |
Webserver sends D-Bus command       |                                    |                             |
to user-manager with User Name      |                                    |                             |
to be deleted            (REQ)--------->                                 |                             |
                                    | 2. If belong to IPMI / REDFISH     |                             |
                                    | group then send D-Bus command      |                             |
                                    | to IPMI / REDFISH with User Name   |                             |
                                    | to be deleted (if user doesn't     |                             |
                                    | belong to IPMI & REDFISH, then     |                             |
                                    | go to step 6)                      |                             |
                                    |                                  ---(REQ)->                      |
                                    |                                    | 3. Both IPMI & REDFISH      |
                                    |                                    | tries to delete the same    |
                                    |                                    | along with other settings   |
                                    |                                    | if any (any 1:1 mappings has|
                                    |                                    | to be deleted here)         |
                                    |                                    | Note: can elect to have temp|
                                    |                                    | update at this point of time|
                                    |                                    | and update database if      |
                                    |                                    | there is no signal          |
                                    |                               <-----(SUCCESS/FAILURE)--          |
                                    | 4. If failure throw corresponding  |                             |
                                    | error. (User session in progress)  |                             |
                                    | If successful go to step 6         |                             |
                            <-------------(FAILURE)                      |                             |
5. Throw error to the user          |                                    |                             |
                                    |                                    |                             |
                                    | 6. Delete User Name and Group roles|                             |
                                    | along with the password.           |                             |
                                    | User Name deleted             ---(SIGNAL)->                      |
                                    |                                    |                             |
                            <-------------(SUCCESS)                      |                             |
6. User deleted successfully,       |                                    |                             |
                                    |                                    |                             |
--------------------------------------------------------------------------------------------------------
```

## OpenBMC - User Management - User deletion from IPMI - IPMI Group only ##

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
                                    | 2. User-Manager deletes the user   |                             |
                                    | with roles (ONLY IPMI).            |                             |
                                    | Remove both encrypted & hashed     |                             |
                                    | password                           |                             |
                                    | Return error if User Name can't be |                             |
                                    | deleted, else return success       |                             |
                            <-------------(FAILURE)                      |                             |
3. Return error to the Set User     |                                    |                             |
Name command. User deletion failed. |                                    |                             |
                            <-------------(SUCCESS)                      |                             |
4. User deleted successfully        |                                    |                             |
                                    |                                    |                             |
--------------------------------------------------------------------------------------------------------
```


## Recommended Implementation ##
1. As per IPMI spec the max user list can be 15 (+1 for NULL User). Hence
implementation has to be done in such a way that no more than 15 users are
getting added to the IPMI Group. Phosphor-user-manager can verify this
limitation itself before sending the D-Bus command to IPMI or can simply throw
the error, when the D-Bus API call to IPMI fails to add the new user.
2. Should add IPMI_NULL_USER by default and keep the user in disabled manner.
This is to avoid any user to mistakenly create IPMI_NULL_USER as a user. This is
needed as NULL user with NULL password in IPMI  can't be added as an entry from
Unix user management point of it.
3. User creation request from IPMI / REDFISH must be handled in
the same manner as described in the above flow diagram, where-by source can
itself throw the error, if it's own verification fails.
4. Adding / removing a user name from IPMI Group role must force a Password
change to the user. This is needed as adding to the IPMI Group of existing
user requires clear text password to be stored in encrypted form. Similarly
when removing a user from IPMI group, must force the password to be changed
as part of security measure.
5. IPMI spec doesn't support group roles for the user management. Hence the
same can be implemented through OEM Commands, thereby creating a user in
IPMI with different group roles.
6. Recommend not to add user in IPMI group through IPMI interface
which already exists as part of other group roles (except IPMI) through regular
Set User Name command. This is to avoid confusion, as existing IPMI commands
won't be able to convey / treat the request as either new user request or
request to extend existing user name to IPMI Group. Extend of IPMI Group to
existing user, through IPMI interface is allowed only through OEM Commands.
