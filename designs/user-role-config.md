# Design: Configurable user roles

Author:
  Joseph Reynolds <Discord:josephreynolds#3558>

Primary assignee:
  None

Other contributors:
  None

Created:
  March 26, 2021

## Problem

OpenBMC user roles are hard-coded in source repositories and are not
configurable from bitbake recipes.  This makes it hard to customize roles, for
example, to create a distro feature which has a custom role.  It is similarly
hard to create Redfish OEM privileges.

## Background

The OpenBMC [user management doc][] defines four phosphor privilege roles
(admin, operator, etc.).  Every user is assigned exactly one role which
controls which operations that user is allowed to perform.  These correspond
with equivalent roles in the [Redfish spec DSP0266][].

The roles are hard-coded into the project in three places:
- The [phosphor user manager recipe][]
- The [phosphor user manager repo][]
- The [BMCWeb][] repo

The [Redfish spec DSP0266][] also defines a standard privilege system which
[BMCWeb][] implements, and it allows OEM privileges and custom roles which
this design seeks to implement.

Note the term "role" is ambiguous and can refer to either a Redfish role or to
a phosphor privilege role.  This design explicitly describes the correlation
between these two sets of roles.

[phosphor user manager recipe]: https://github.com/openbmc/openbmc/blob/master/meta-phosphor/recipes-phosphor/users/phosphor-user-manager_git.bb
[phosphor user manager repo]: https://github.com/openbmc/phosphor-user-manager
[BMCWeb]: https://github.com/openbmc/bmcweb
[Redfish spec DSP0266]: https://www.dmtf.org/dsp/DSP0266
[user management doc]: https://github.com/openbmc/docs/blob/master/architecture/user-management.md

## Requirements

There must be a place for bitbake data to specify custom phosphor privilege
roles.  These roles must be built into the image as Linux groups and available
for repositories which need them.

[phosphor-defaults]: https://github.com/openbmc/openbmc/blob/master/meta-phosphor/conf/distro/include/phosphor-defaults.inc

The [phosphor-defaults][] is the right place for this.
```fixed
PHOSPHOR_PRIVILEGE_ROLES ?= "priv-admin priv-operator priv-user priv-noaccess"
PHOSPHOR_PRIVILEGE_ROLES[doc] = "Defines Phosphor privilege roles."
```

There must be build-time BMCWeb configuration which controls BMCWeb's
behavior, as follows:
- Custom Redfish roles and OEM privileges.  These appear, for example, under
  URI /redfish/v1/AccountService/Roles/.
- The mapping between Redfish roles and phosphor privilege roles.  These
  appear, for example, under URI /redfish/v1/AccountService/Roles/USER.
- Role-to-privilege mapping.  This appears, for example, under URI
  /redfish/v1/AccountService/Roles/ROLE, and is applied to determine which
  operations are allowed.

This design does not affect BMCWeb's operation-to-privilege mapping.

This design proposes a new `redfish-role-info` JSON file.  The idea is to
transform the redfish-role-info into a configuration file at build-time, so it
can be built into the code.  BMCWeb uses a default file which specifies the
standard OpenBMC role configuration.  An alternate config file can be supplied
via a bmcweb_%.bbappend.

The redfish-role-info.json file format:
```fixed
Purpose:
 - Defines the Redfish standard and custom roles.
 - Defines the Redfish standard and OEM privileges.
 - Maps each Redfish role to its corresponding phosphor privilege role.
 - Provides information about each Redfish role, including the
   role-to-privilege mapping.

Note: Role refers to a Redfish role unless otherwise specified.

Syntax:
This must be a JSON dictionary with exactly 6 items:
  StandardRoles - array of string
      Defines the standard Redfish role names, plus the NoAccess role
      invented by OpenBMC.  This should not be customized.
  CustomRoles - array of string
      Defines Redfish custom roles.  Add custom role names here per the
      Redfish spec.
  StandardPrivileges - array of string
      Defines the standard Redfish privilege names.
      This should not be customized.
  OemPrivileges - array of string
      Defines OEM privileges.  Add OEM privileges here per the Redfish spec.
  RoleToPhosphorPrivilegeRoleMap - dictionary mapping to string
      Maps from Redfish role name to its corresponding phosphor privilege role
      name.  This is used also used to map from phosphor privilege roles back
      to Redfish roles.  The dictionary keys must include all Redfish roles
      (including both standard and custom roles).  Each phosphor privilege
      role name may only be used once.
  RoleInfo - dictionary of dictionary
      Provides information about each Redfish role, corresponding to the
      Redfish "Role" schema.  The dictionary keys must include all Redfish
      roles (including both standard and custom roles).  The role information
      is a dictionary with the following keys:
        - AssignedPrivileges (required) - array of string, each string must in
          StandardPrivileges
        - OemPrivileges (optional, default is []) - array of string, each
          string must in OemPrivileges

The sequence in which the roles and privileges are given here is carries into
the Redfish service behavior:
 - The roles are presented (such as via URI /redfish/v1/AccountService/Roles)
   in the sequence given here: StandardRoles first followed by CustomRoles.
  - The standard privileges are presented (such as via URI
    /redfish/v1/AccountService/Roles/Administrator property
    AssignedPrivileges) in the sequence given by StandardPrivileges.
 - The OEM privileges are presented (such as via URI
   /redfish/v1/AccountService/Roles/Administrator property OemPrivileges) in
   the sequence given by OemPrivileges.
```

bmcweb_%.bbappend:
```
# Pass in the phosphor role names and Redfish role info.
EXTRA_OEMESON += "-DPHOSPHOR_PRIVILEGE_ROLES=${PHOSPHOR_PRIVILEGE_ROLES}"
SRC_URI += "file://redfish-role-info.json"
```

Default redfish-role-info.json:
```
{
    "StandardRoles": [
        "Administrator",
        "Operator",
        "ReadOnly",
        "NoAccess"
    ],
    "CustomRoles": [ ],
    "StandardPrivileges": [
        "Login",
        "ConfigureManager",
        "ConfigureUsers",
        "ConfigureComponents",
        "ConfigureSelf"
    ],
    "OemPrivileges": [ ],
    "RoleToPhosphorPrivilegeRoleMap": {
        "Administrator": "priv-admin",
        "Operator": "priv-operator",
        "ReadOnly": "priv-user",
        "NoAccess": "priv-noaccess"
    },
    "RoleInfo": {
        "Administrator": {
            "AssignedPrivileges": [
                "Login",
                "ConfigureManager",
                "ConfigureUsers",
                "ConfigureComponents",
                "ConfigureSelf"
            ],
        },
        "Operator": {
            "AssignedPrivileges": [
                "Login",
                "ConfigureComponents",
                "ConfigureSelf"
            ],
        },
        "ReadOnly": {
            "AssignedPrivileges": [
                "Login",
                "ConfigureSelf"
            ],
        },
        "NoAccess": {
            "AssignedPrivileges": [ ],
        }
    }
}
```

Example showing how to create a custom role and OEM privilege:
```
PHOSPHOR_PRIVILEGE_ROLES += "priv-service"

redfish-role-info.json (excerpts showing additions to the default file):
{
    "StandardRoles": [ ... ],
    "CustomRoles": ["OemIBMServiceAgent"],
    "StandardPrivileges": [ ... ],
    "OemPrivileges": ["OemIBMPerformService"],
    "RoleToPhosphorPrivilegeRoleMap": {
        ...,
        "OemIBMServiceAgent": "priv-service"
    },
    "RoleInfo": {
        ...,
        "OemIBMServiceAgent": {
            "AssignedPrivileges": [
                "Login",
                "ConfigureManager",
                "ConfigureComponents",
                "ConfigureSelf"
            ],
            "OemPrivileges": [
                OemIBMPerformService"
            ],
        }
    }
}
```


## Alternatives

Implementations built on OpenBMC can patch their roles and mappings.

## Impacts

Trivial impacts to recipe complexity and built-time.

When custom roles and OEM privileges are defined, trivial impact to executable
size and runtime performance for BMCWeb and Phosphor User Manager.

Compatibility:
- There are no impacts to compatibility when defaults are used.
- Compatibility considerations from customizing an image with this mechanism
  must be carefully considered but are outside the scope of this design.

## Testing

The CI server will only test the default configuration.  Because this design
impacts built-time configuration, testing requires building the image multiple
times:
1. When project defaults are used, ensure roles and privileges are as before.
2. When custom roles are defined, ensure those roles show up properly on
   phosphor-user-manager and BMCWeb.
3. When role-to-privilege mappings are specified, ensure BMCWeb applies them
   correctly.

Tests for each resulting image:
- Ensure the intended Linux groups (representing phosphor privilege roles) are
  created.
- For each role, ensure users can be created and modified.  Ensure the user's
  privilege group is represented correctly.
- Ensure BMCWeb displays the intended Roles and the intended privileges for
  each role.
- Ensure BMCWeb's authorization mechanism is correct by using positive and
  negative tests.  For example, ensure a user who has the Operator role is not
  allowed to perform user management operations.
