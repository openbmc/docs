# Dynamic Redfish Authorization

Author: Nan Zhou (nanzhoumails@gmail.com)

Created: 08/08/2022

## Problem Description

The Redfish authorization subsystem controls which authenticated users have
access to resources and the type of access that users have.

DMTF has already defined the authorization model and schemas. This design is to
enhance the current implementation in BMCWeb Redfish interface so that OpenBMC
systems exposes interfaces to change authorization behavior at runtime without
Redfish service restart.

## Background and References

### Redfish Authorization Model

The Redfish authorization model consists of the privilege model and the
operation-to-privilege mapping.

In the privilege model, there are fixed set of standard Redfish roles and each
of them is assigned a fixed array of standard privileges (e.g., `Login`,
`ConfigureManager`, etc). A service may define custom OEM roles (read-only). A
service may even allow custom client-defined roles to be created, modified, and
deleted via REST operations on `RoleCollection`.

The operation-to-privilege mapping is defined for every resource type and
applies to every resource the service implements for the applicable resource
type. It is used to determine whether the identity privileges of an
authenticated Redfish role are sufficient to complete the operation in the
request. The Redfish Forum provides a Privilege Registry definition in its
official registry collection as a base operation-to-privilege mapping. It also
allows mapping overrides (Property override, Subordinate override, and Resource
URI override) to customize mapping.

For example, a peer who is an `Operator` that has both `Login`,
`ConfigureComponents`, and `ConfigureSelf` privileges, is authorized to send a
`GET` request to the `ChassisCollection`, since the `GET` operation on this
resource only requires the `Login` privilege. On the other hand, the same peer
gets denied access if they send a POST request to `CertificateService`, as the
POST operation on certificates requires `ConfigureManager` privilege that the
peer is missing.

**Note**, in the Redfish spec, OEM roles can be added via POST to the
`RoleCollection` in the `AccountService`; `PrivilegesUsed`,
`OEMPrivilegesUsed`, and properties of `Mappings` are all read-only.

References:
1. https://redfish.dmtf.org/schemas/DSP0266_1.15.1.html#privilege-model
2. https://redfish.dmtf.org/schemas/DSP0266_1.15.1.html#redfish-service-operation-to-privilege-mapping
3. https://redfish.dmtf.org/schemas/DSP0266_1.15.1.html#roles
4. https://redfish.dmtf.org/registries/v1/Redfish_1.3.0_PrivilegeRegistry.json

### Phosphor-user-manager

Phosphor-user-manager is an OpenBMC daemon that does user management.

It exposes DBus APIs to dynamically add users, manage users' attributes (e.g.,
group, privileges, status, and account policies). It has a hardcoded list of
user groups (SSH, IPMI, Redfish, Web) and a hardcoded list of privileges
("priv-admin", "priv-operator", "priv-user", "priv-noaccess"). These privileges
are implemented as Linux secondary groups.

It also integrates LDAP (supports either ActiveDirectory or OpenLDAP) for
remote user management, where BMC acts as a LDAP client and uses nslcd to
automatically convert Linux system calls to LDAP queries.

References:
1. https://github.com/openbmc/docs/blob/master/architecture/user-management.md
2. https://github.com/openbmc/phosphor-user-manager
3. https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/yaml/xyz/openbmc_project/User
4. https://linux.die.net/man/8/nslcd
5. https://linux.die.net/man/8/nscd

### BMCWeb

BMCWeb is an OpenBMC Daemon which implements the Redfish service (it implements
other management interfaces as well besides Redfish).

BMCWeb supports various "authentication" options, but under the hood, to verify
the user is who they claim they are, there are two main authentication methods:
1. PAM based: use Linux-PAM to do username/password style of authentication
2. TLS based: use the Public Key infrastructure to verify signature of peer's
certificates; then use identities (in X509 certificates, these are Common Name
or Subject Alternative Name).

After getting the peer's username, BMCWeb sends DBus queries to
phosphor-user-manager to query the User's privileges and uses a hardcoded map
to convert the privileges to Redfish roles. The hardcoded map is listed below:

| Phosphor-user-manager privileges (implemented as groups) | BMCWeb Redfish Roles |
|----------------------------------------------------------|----------------------|
| priv-admin                                               | Administrator        |
| priv-operator                                            | ReadOnly             |
| priv-user                                                | Operator             |
| priv-noaccess                                            | NoAccess             |

To map Redfish role to their assigned Redfish privileges, BMCWeb implements the
following hardcoded map:

| BMCWeb Redfish Roles | Assigned Redfish Privileges                                                            |
|----------------------|----------------------------------------------------------------------------------------|
| Administrator        | "Login", "ConfigureManager", "ConfigureUsers", "ConfigureSelf", "ConfigureComponents"} |
| ReadOnly             | "Login", "ConfigureSelf", "ConfigureComponents"                                        |
| Operator             | "Login", "ConfigureSelf"                                                               |
| NoAccess             | NA                                                                                     |

At compile time, BMCWeb assigns each operation of each entity a set of required
Redfish Privileges. An authorization action is performed when a BMCWeb route
callback is performed: check if the assigned Redfish Privileges is a superset
of the required Redfish Privileges.

In the following section, we refer a BMCWeb route as the handler of an
operation of an given resource URI, which is what the BMCWEB_ROUTE macro has
defined.

References:
1. https://github.com/openbmc/bmcweb/blob/d9f6c621036162e9071ce3c3a333b4544c6db870/include/authentication.hpp
2. https://github.com/openbmc/bmcweb/blob/d9f6c621036162e9071ce3c3a333b4544c6db870/http/http_connection.hpp
3. https://github1s.com/openbmc/bmcweb/blob/d9f6c621036162e9071ce3c3a333b4544c6db870/redfish-core/lib/roles.hpp

### Gaps
As mentioned above, majority of the current Redfish authorization settings are
configured at compile time including:

1. the set of Phosphor-user-manager privileges
2. the mapping from Phosphor-user-manager privileges to Redfish roles
3. the set of Redfish roles
4. the mapping from Redfish roles to Redfish Privileges
5. the operation-to-privilege mapping

However, modern systems has use cases where Redfish roles, Redfish privileges,
and operation-to-privilege mapping needs to change when the system keeps
running. E.g., a new micro-service is introduced and needs to talk to existing
BMCs in the fleet, we need to propagate a configuration so that this new peer
gets a proper Redfish role and is authorized to access certain resources
without rolling out a new BMC firmware.

Another gap is that current Redfish roles and operation-to-privilege mapping
lead to a very coarse-grained access control, and doesn't implement the
principle of least privilege. Though these configurations are defined by DMTF,
it is explicitly called out in the standard that implementation implement their
own OEM roles and privileges if "the standard privilege is overly broad".

For systems which have requirement where a given user only has access to the
resources it needs. For example, a micro-service responsible for remote power
control can only send GET to Chassis and ComputerSystems, and POST to
corresponding ResetActions. With the existing implementation, such
micro-service has at least ConfigureComponents Redfish privilege, which leads
to being able to patch an EthernetInterface resource.

## Requirements
BMCWeb implements a dynamic Redfish Authorization system:

1. Clients shall be able to add new OEM Redfish privileges without recompile
2. Clients shall be able to add new OEM Redfish roles and assign it with any
existing Redfish privileges without recompile
3. Clients shall be able to modify existing operation-to-privilege mappings
without recompile
4. Above changes on systems shall be atomic; that is, once changed, all new
queries will use the latest configurations
5. BMC shall perform sanity check on above modification; that is
    * It rejects ill formatted modification
    * It rejects modification of non-OEM privileges
    * It rejects deletion of OEM Redfish roles if any user (either local or
      remote) maps such roles
    * It rejects deletion of OEM Redfish privileges if any OEM Redfish role is
      assigned such privileges
6. BMC shall persist all the above modifications and recover from crash
7. Existing systems with the static authorization shall work as if this feature
is not introduced, including non-Redfish routes (e.g., KVM websocket)
8. Default OEM roles and Redfish privileges must be selectable on a per system
basis at compile time; default Redfish PrivilegeRegistry must be settable on a
per system basis at compile time
9. The total storage used by this feature shall be limited; this is
    * The total rwfs disk usage increase is less than 100 KB on systems with
      the dynamic authorization feature enabled
    * The runtime memory usage increase is less than 1 MB on systems with
      the dynamic authorization feature enabled
    * The binary size increase of modified daemons is less than 100 KB on all
      systems
10. BMC implements a complete privilege registry; that is
    * It shall implement all overrides in the Redfish base Privilege registries
   at compile time; it shall support configuring overrides at runtime but
   implementation may begin with static configuring and reject runtime
   modification
    * BMC exposes PrivilegeRegistry which represents the current configuration
   and reflects runtime changes
    * Changes to resource entities shall be propagated to the current privilege
   registries automatically
11. New Redfish resource can be implemented without modifying custom
    PrivilegeRegistry
12. All the above shall be testable; CI must be capable of finding issues

## Proposed Design

### Mapping: Users, Redfish Roles, and Redfish Privileges

As mentioned in the background section, existing Redfish roles are stored as
Linux secondary groups with prefix "priv" (includes "priv-admin",
"priv-operator", and "priv-user"). And a Linux user is in one of these groups
representing that a user is a specific Redfish role. BMCWeb then uses a
hardcoded table to map Redfish role to Redfish privileges.

However, modeling roles only as static Linux secondary groups doesn't worked
well with OEM Redfish Roles where a Redfish role maps to a dynamic set of OEM
privileges: secondary group can't represent associations.

To solve this, we propose the following solution:

**Store Redfish Roles As Linux Users and Secondary Groups**
We propose to store Redfish Roles as both Linux users and secondary groups.
Storing as secondary groups is to associate users with Redfish roles. On the
other hand, storing as users is to associate Redfish roles with Redfish
privileges. See below section for these mappings.

Users for Redfish roles won't be any predefined groups (web, redfish, ipmi). We
can add extra attributes to distinguish them with real local and LDAP users.
Users for Redfish roles won't have SSH permission as well.

Redfish roles will have a fixed prefix "openbmc-rfr-". "rfr" refers to Redfish
role. OEM redfish roles will get prefix "openbmc-orfr-". "orfr" refers to OEM
Redfish role. For example, the base Redfish role "Administrator" will result in
a Linux secondary group "openbmc-rfr-administrator" and a local user
"openbmc-rfr-administrator". We can also make the vendor name ("openbmc")
configurable at compile time. Using acronym is to save characters since Linux
username has 31 characters limit.

**Store Redfish Privileges as Secondary Groups**
Redfish privileges will be stored as Linux secondary groups with a fixed prefix
"openbmc-rfp". rfr" refers to Redfish privilege. OEM privileges will have fixed
prefix "openbmc-orfp". "orfr" refers to OEM Redfish privilege.

**Username to Redfish Role Mapping**
Mapping a username to Redfish role becomes searching the group starting with
"openbmc-rfr" that the user is in.

**Redfish Role to Redfish Privileges Mapping**
Mapping a Redfish Role to Redfish privileges becomes searching all the groups
starting with "openbmc-rfp" or "openbmc-orfp" of the user.

A user maps be in multiple linux secondary groups meaning they are assigned
certain privileges; for example, user "PowerService" is in "openbmc-orfr-power"
group meaning its Redfish role is "openbmc-orfr-power"; local user
"openbmc-orfr-power" is in secondary groups "openbmc-rfp-configureself" and
"openbmc-orfp-power" groups meaning "openbmc-orfr-power" is assigned to Redfish
privileges "ConfigureSelf" and "OemPrivPower".

The following diagram shows how assigned privileges of a power control service
with identity (username in PAM, or CN/SAN in TLS) "power-service" is resolved.

```
 +-----------------+
 |  power service  |
 |                 |
 +--------|--------+
          |
          |
 +-----------------+
 |  authentication |
 |   (PAM or TLS)  |
 |      BMCWeb     |
 +--------|--------+
          |
          |username:power-service
          |
 +-----------------+                        +-----------------+            +----------------------------+
 |      BMCWeb     |DBus: redfishPrivileges |  phosphor-      |            |     Linux                  |
 |                 ------------------------>|   user-manager  |            |user: power-service         |
 |                 |<-----------------------|                 |            |group:                      |
 |                 | privileges:            |                 <----------->|  openbmc-orfr-power        |
 |                 |  Login, ConfigureSelf  |                 |            |                            |
 |                 |  OemPrivPower          |                 |            |user: openbmc-orfr-power    |
 +-----------------+                        +-----------------+            |group:                      |
                                                                           |  openbmc-rfp-configureself |
                                                                           |  openbmc-orfp-power        |
                                                                           |  openbmc-rfp-login         |
                                                                           +----------------------------+
```

The above diagram works for LDAP users as well. A remote role or group can
map to base Redfish roles or OEM Redfish roles via RemoteRoleMapping: an LDAP
group maps to a single Redfish role stored as local users.

We propose to extend the existing phosphor-user-manager:

1. It returns AllPrivileges dynamically by looking up the current groups
2. Phosphor-user-manager provides DBus APIs to query Redfish privileges of a
given user

### Creation/Deletion: Users, Redfish Roles, and Redfish Privileges

Base privileges and roles won't be allowed to change at runtime.

#### OEM Redfish Privileges

PATCH OEMPrivilegesUsed in PrivilegeRegistry creating/deleting OEM privileges
to create or delete OEM Privileges at runtime.

We propose to extend the existing phosphor-user-manager:

1. Phosphor-user-manager provides DBus APIs to create/delete OEM Redfish
   privileges; under the hood, it stores privileges as Linux groups, as today
   how base Redfish roles are stored
2. Phosphor-user-manager keeps a maximum number of Redfish privileges; we
   propose 32 as the first iteration considering fast bit manipulation
3. Phosphor-user-manager performs validation:
   * Names of OEM Redfish privileges are unique and valid; e.g., start with
     "openbmc-orfp-"
   * Reject deletion of a privilege that's currently in use (assigned to any
     Redfish roles that have a user associated with)

Systems can also optionally add OEM Privileges at compile time via Yocto's
GROUPADD_PARAM in the useradd class.

#### OEM Redfish Roles

POST on the RoleCollection in the AccountService to create an OEM role, with
assigned Privileges in the AssignedPrivileges and OemPrivileges properties in
the Role resource.

DELETE on the specific Role in the RoleCollection to delete an OEM role.
Predefined roles are not allowed to be deleted.

We propose to extend the existing phosphor-user-manager:
1. Phosphor-user-manager provides DBus APIs to create Redfish role
2. Phosphor-user-manager keeps a maximum number of Redfish roles; we propose 32
   as the first iteration considering fast bit manipulation
3. Phosphor-user-manager performs validation:
   * Names of OEM Redfish roles are unique and valid; e.g., start with
     "openbmc-orfr-"
   * Reject deletion of a RedfishRole that's currently in use (associated with
     users)

#### Users

POST on the ManagerAccountCollection to create a user, with a RoleId to the
assigned Redfish role.

DELETE on the specific user in the ManagerAccountCollection to delete a user.

Note: modification on ManagerAccountCollection need authorization as well. For
example, a user with only "ConfigureSelf" permission is not allowed to delete
other users.

#### Typical Workflow

In summary, a typical workflow to create a new local user with an new OEM
Redfish role which is assigned a new set of OEM Redfish Privileges is mapped
out below.

```
         Root User                    BMCWeb                  Phosphor-User-Manager             Linux
              |   PATCH PrivilegeRegistry |                              |                        |
              |-------------------------->|   DBus: createGroup          |                        |
              |   Add OemAddPowerService  |----------------------------->|     groupadd           |
  Create      |                           |                              |----------------------->|
  OemPrivilege|                           |            Okay              |<-----------------------|
              |           Okay            |<-----------------------------|        Okay            |
              |<--------------------------|                              |                        |
              |                           |                              |                        |
              |                           |                              |                        |
              | POST RoleCollection       |                              |                        |
              |-------------------------->|  DBus: createUser            |                        |
              |                           |----------------------------->|  useradd + groupadd    |
  Create      |                           |                              |----------------------->|
  OemRole     |                           |            Okay              |<-----------------------|
              |          Okay             |<-----------------------------|         Okay           |
              |<--------------------------|                              |                        |
              |                           |                              |                        |
              |                           |                              |                        |
              |POST                       |                              |                        |
              | ManagerAccountCollection  |                              |                        |
              |-------------------------->| DBus: createUser             |                        |
              |                           |----------------------------->|       useradd          |
  Create      |                           |                              |----------------------->|
  User        |                           |                              |<-----------------------|
              |                           |<-----------------------------|        Okay            |
              |<--------------------------|            Okay              |                        |
              |           Okay            |                              |                        |
  Time        |                           |                              |                        |
              v                           v                              v                        v
```

### Non-Redfish Routes or OEM Resources
We still keep the current `privileges` C++ API to add explicit Redfish
privileges for non-redfish routes via `BMCWEB_ROUTE`.

### Redfish Routes
We propose to create a new macro `REDFISH_ROUTE` so existing `REDFISH_ROUTE`
stay unchanged for non-redfish routes.

For each `REDFISH_ROUTE`, instead of taking a privileges array (or reference to
a privileges array), this design proposes to create the following extra C++
APIs:

1. `entity`: it takes a string representing the Resource name (the same
definition as it in the PrivilegeRegistry); for example,
"/redfish/v1/Managers/${ManagerId}/EthernetInterfaces/" will provide a string
"EthernetInterfaceCollection" as the entity
2. `subordinateTo`: it takes an array of string representing what resource this
router is subordinate to (the same definition as it in the PrivilegeRegistry);
for example, a route with URL
"/redfish/v1/Managers/${ManagerId}/EthernetInterfaces/" will provide an array
of {"Manager", "EthernetInterfaceCollection"} as the value of `subordinateTo`.

Any Redfish route must provide these attributes. Non Redfish route shall not
provide them, instead, they specify `privileges` directly. The values of these
attributes can be generated from the schema at compile time. To guarantee this
in C++ code, we can make them required parameters in constructors.

See below for how we propose to get required Redfish privileges for a given
method on a given resource by using the proposed `entity` & `subordinateTo`,
the existing `methods`, and the URL from the request.

See the alternatives section for how we can get rid of setting these attributes
at manually.

### Operation-to-Privilege Mapping Data Structure in Memory
BMCWeb keeps a JSON like data structure in memory to represent the whole
Operation-to-Privilege Mapping. For a given route with known entity name, HTTP
method, and resource URL, the required set of privileges can be retrieved
efficiently.

The operation to check if a user is authorized to perform a Redfish operation
is still the existing bit-wise `isSupersetOf` between the required privileges
of a given operation on a given resource and the assigned privileges of a role.

### Generate Operation-to-Privilege Mapping Data Structure at Compile Time
BMCWeb has requirements that it doesn't prefer to load a large file at service
start time since it slows down the service, and whatever services rely on it.

Thus, we propose to generate the data structure at compile time, it takes a
PrivilegeRegistry JSON, and generate necessary headers and sources files to
hold a variable that represent the whole Operation-to-Privilege Mapping. The
input JSON can change across systems.

This can be implemented as a customized Meson generator.

We will delete the current static privileges header, but the generated header
will increase the binary size. We shall limit the increase to <= 100KB. The
size of `Redfish_1.3.0_PrivilegeRegistry.json` is about 275 KB; the minified
version of it (no whitespace) is about 62 KB. When parsing this JSON and
converting it to C++ codes, we shall not increase the binary size a lot
otherwise we can just store the whole registry as a Nlohmann JSON object.

### Operation-to-Privilege Mapping Overrides

In routing codes, we can parse the Operation-to-Privilege Mapping Data
Structure and look for SubordinateOverrides and ResourceURIOverrides, combine
them with the attributes from route and request, and perform authorization.

Most part of the Authorization codes run before resource handlers. However,
PropertyOverrides can only be executed when response is ready since we need to
inspect the response attributes.

A example execution flow is mapped out below.

```
    +---------------+
    |   BMCWeb      | POST
    |    routing    | /redfish/v1/Managers/${ManagerId}/EthernetInterfaces/
    +-------|-------+
            | Known information:
            |  Request.URL
            |  Request.method
            |  Route.entity
            |  Route.subordinateTo
            |
+-----------------------+       +--------------------------------------------------------------+
| Any                   |       |Operation-to-Privilege Mapping in-memory Data Structure       |
| ResourceURIOverrides? <------>|                                                              |
|                       |       |{                                                             |
+-----------|-----------+       |   "Entity": "EthernetInterface",                             |
            |Updated            |   "OperationMap": {                                          |
            |RequiredPrivileges |      "POST": [{                                              |
+-----------------------+       |         "Privilege": ["ConfigureComponents"]                 |
| Any                   |       |      }]                                                      |
| SubordinateOverrides? |<----->|   },                                                         |
|                       |       |   "SubordinateOverrides": [{                                 |
+-----------------------+       |      "Targets": ["Manager", "EthernetInterfaceCollection"],  |
            |Updated            |      "OperationMap": {                                       |
            |RequiredPrivileges |         "POST": [{                                           |
            v                   |            "Privilege": ["ConfigureManager"]                 |
+-----------------------+       |         }]                                                   |
| Authorization         |       |      }                                                       |
+-----------|-----------+       |   }]                                                         |
            |Okay               |}                                                             |
            |Got Response       |                                                              |
            |                   +--------------------------------------------------------------+
+-----------------------+                       ^
| Any                   |                       |
| PropertyOverrides?    |<----------------------+
+-----------|-----------+
            |
            |
            v
        Final Response
```

### PrivilegeRegistry Resource

BMCWeb will implement PrivilegeRegistry in a new route. The route will reflect
the current PrivilegeRegistry in used including dynamic changes.

With the proposed Dynamic Operation-to-Privilege Mapping Data Structure, and
DBus APIs that phosphor-user-manager provides, the implementation is trivial:
convert the JSON data structure into a JSON response.

### PATCH on PrivilegeRegistry

Though the Redfish spec declares PrivilegeRegistry to be read-only, this design
still proposes implementing PATCH on PrivilegeRegistry: users can PATCH any
attributes directly, e.g., patch the POST privilege array of OperationMap of
Entity EthernetInterface so that users with "OemEthernetManager" can also send
GET to EthernetInterface.

```
{
    Entity": "EthernetInterface",
    "OperationMap": {
        "GET": [
            {
                "Privilege": [
                    "Login"
                ]
            },
            {
                "Privilege": [
                    "OemEthernetManager"
                ]
            }
        ]
    }
}
```

The implementation might reject modification on certain attributes when
corresponding implementation is not ready. E.g., it rejects modifying
SubordinateOverrides when the service doesn't have SubordinateOverrides
implemented.

Changes against the base PrivilegeRegistry will be rejected, e.g., deleting
ConfigureSelf from a OperationMap. This design is for OEMPrivileges and
OEMRoles.

### Persistent Data

OEM Redfish roles, Redfish privileges, and users are persisted by Linux. With a
maximum number of roles and privileges being set, the total persistent data
will be very small (around several KB).

To minimize size of persistent data, we propose that BMCWeb only stores the
serial of PATCH requests on the PrivilegeRegistry. This data can be stored in
the existing `bmcweb_persistent_data.json`. When restart from crash or reset,
BMCWeb will be able to execute the serial of PATCH requests to recover the
PrivilegeRegistry.

We can set a maximum number of PATCH requests that BMCWeb allows to limit the
disk usage. Most upstream systems also have several MB of read-write partition
configured. For example, `romulus` as of the design was written, 4194304 bytes
for rwfs. We propose to start with allowing 1000 requests.

Systems without enabling dynamic authorization feature won't have any new
persistent data added.

## Alternatives Considered

### Infer Entity and SubordinateTo from URL
We can infer the entity from the URL by building a Trie like data structure.
However, it's not a big deal to hardcode an entity for a route, since entity
and SubordinateTo never change at runtime.

## Impacts
1. New DBus interfaces on phosphor-user-manager
2. New persistent data managed by BMCWeb will be added on BMCs
3. Binary size will increase on BMCWeb
4. Existing systems with the static authorization shall work as if this feature
is not introduced

## Organizational
No new repository is required. Phosphor-user-manager and BMCWeb will be
modified to implement the design.

## Testing
Existing tests shall still pass as if this feature is not introduced.
New Robot Framework test can be added to test runtime modification of
PrivilegeRegistry.

Test cases should include:

1. verify base Redfish roles, privileges, and base operation-to-privilege
   respect the Redfish spec when no runtime modification is made
2. verify Redfish OemPrivilege can be added via PATCH to PrivilegeRegistry and
   reflected in the PrivilegeRegistry resource
3. verify Redfish OemRole can be added via POST to ManagerAccountCollection
   with assigned OemPrivilege and reflected in the ManagerAccountCollection
4. verify operation-to-privilege can be modified via PATCH on
   PrivilegeRegistry; mapping of an action on a resource can be added with the
   above OemPrivilege, and finally the user of that OemRole now can access the
   resource
5. verify the 3 dynamic overriding is working as expected; e.g., a new override
   can be added to PrivilegeRegistry and should be reflected in new requests