# User management

User management enables administration of users with different authentication credentials. This provides the foundation for restricting access to REST API based on the user.

In order to access the user management REST API, an initial authentication with *root* credentials is required.

User management provides a *restricted/limited* set of options available through the native Unix user administration commands.

The DBus API for user management is a wrapper over the adduser, deluser, addgrp, delgrp and passwd commands.

## User Management DBus Application, Interface and Objects
UserManager (phosphor-host-userd) implements the org.openbmc.**Enrol**  interface and exposes the following objects:
- Groups
- Group
- Users
- User

The Groups object is the container object used to *add* a new group. Likewise Users object is the container object used to *add* a new user. Groups and Users objects also provide the iteration interface to list the members.

The Group object and the User object abstract the individual group/user and hence exposes the method to delete the specific entry and change the password.

## User Management DBus API
### /org/openbmc/UserManager/Groups Object
    org.openbmc.Enrol.GroupAddUsr string:"groupname"
    org.openbmc.Enrol.GroupListUsr
    org.openbmc.Enrol.GroupListSys
### /org/openbmc/UserManager/Group Object
    org.openbmc.Enrol.GroupDel string:"groupname"
### /org/openbmc/UserManager/Users Object
    org.openbmc.Enrol.UserAdd string:"comment" string:"username" string:"groupname" string:"passwd"
    org.openbmc.Enrol.UserList
### /org/openbmc/UserManager/User Object
    org.openbmc.Enrol.UserDel string:"username"
    org.openbmc.Enrol.Passswd string:"username" string:"passwd"


## Group Operations and Rules

| Method| Params | Description | Remarks|
|-------|:------:|-------------|-------:|
| GroupAddUsr | Group Name | Add a User Group.| GID is automatically selected >= 1000.|
||||- Group name must be unique.|
| GroupListUsr || List all User Groups.| List All groups with GID >= 1000.      |
| GroupListSys || List all Sys Groups. | List All groups with 100 < GID >= 1000.|
| GroupDel | Group Name | Delete this Group. |Group must have no member users|


## User Operations and Rules

| Method| Params | Description | Remarks|
|-------|:------:|-------------|-------:|
|UserAdd | GECOS, User Name, Group Name, Password | Add a general User Account.| UID is automatically selected >= 1000.|
||||User name must be unique and non-empty string.|
||||Group name, if specified, must be unique and non-empty string.|
|UserList|| List all general User Accounts.| Users with UID >= 1000.|
|UserDel |User name| Delete this user. | User name must be non-empty string.    |
| Passwd | User Name, Passwd | Reset the password of the user to this new passwrod. | |

## Group List semantics
| Group     | Description  |
|-----------|--------------|
| Sys Group | List all groups with 100 < GID < 1000. Common system groups with GID < 100 are hence filtered out in the list.|
| Usr Group | List all groups with GID >= 1000. These groups are typically those created by a group administration operation.|

## Group Add semantics
| Group     | Description  |
|-----------|--------------|
| Sys Group | Add a group with 100 < GID < 1000, which has system group priveleges.|
| Usr Group | Add a group with GID >= 1000. These groups are typically those created by an administrator to represent a role.|

## User List semantics
| User Account | Description  |
|--------------|--------------|
| Sys Account  | Not Supported|
| User Account | List all user accounts with UID >= 1000. These users are typically those created by a user administration operation.

## User Add semantics
| User/Group   | User Group    | Sys Group     |
|--------------|---------------|---------------|
| User Account | Supported     | Supported     |
| Sys Account  | Not Supported | Not Supported |

