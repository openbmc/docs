# Dynamic Redfish Authorization
Author: Nan Zhou (nanzhoumails@gmail.com)
Created: 08/08/2022

## Problem Description

The Redfish authorization subsystem controls which authenticated users have
access to resources and the type of access that users have.

DMTF has already defined the authorization model and schemas. This design is
to enhance the current implementation in BMCWeb Redfish interface so that
OpenBMC systems exposes interfaces to change authorization behavior at runtime
without Redfish service restart.

## Background and References

### Redfish Authorization Model

The Redfish authorization model consists of the privilege model and the
operation-to-privilege mapping.

In the privilege model, there are fixed set of standard Redfish roles and each
of them is assigned a fixed array of standard privileges (e.g., `Login`,
`ConfigureManager`, etc). A service may define custom OEM roles (read-only). A
service may even allow custom client-defined roles to be created, modified,
and deleted via REST operations on `RoleCollection`.

The operation-to-privilege mapping is defined for every resource type and
applies to every resource the service implements for the applicable resource
type. It is used to determine whether the identity privileges of an
authenticated Redfish role are sufficient to complete the operation in the
request. The Redfish Forum provides a Privilege Registry definition in its
official registry collection as a base operation-to-privilege mapping. It also
allows mapping overrides (Property override, Subordinate override, and
Resource URI override) to customize mapping.

For example, a peer who is an `Operator` that has both `Login`,
`ConfigureComponents`, and `ConfigureSelf` privileges, is authorized to send a
`GET` request to the `ChassisCollection`, since the `GET` operation on this
resource only requires the `Login` privilege. On the other hand, the same peer
gets denied access if they send a POST request to `CertificateService`, as the
POST operation on certificates requires `ConfigureManager` privilege that the
peer is missing.

**Note**, in the Redfish spec, OEM roles can be added via POST to the
`AccountService`; `PrivilegesUsed`, `OEMPrivilegesUsed`, and properties of
`Mappings` are all read-only.

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
("priv-admin", "priv-operator", "priv-user", "priv-noaccess").

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

BMCWeb is an OpenBMC Daemon which implements the Redfish service (it
implements other management interfaces as well besides Redfish).

BMCWeb supports various "authentication" options, but under the hood, to
verify the user is who they claim they are, there are two main authentication
methods:
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

To map Redfish role to their assigned Redfish privileges, BMCWeb implements
the following hardcoded map:

| BMCWeb Redfish Roles | Assigned Redfish Privileges                                                            |
|----------------------|----------------------------------------------------------------------------------------|
| Administrator        | "Login", "ConfigureManager", "ConfigureUsers", "ConfigureSelf", "ConfigureComponents"} |
| ReadOnly             | "Login", "ConfigureSelf", "ConfigureComponents"                                        |
| Operator             | "Login", "ConfigureSelf"                                                               |
| NoAccess             | NA                                                                                     |

At compile time, BMCWeb assigns each operation of each entity a set of
required Redfish Privileges. An authorization action is performed when a
BMCWeb route callback is performed: check if the assigned Redfish
Privileges is a superset of the required Redfish Privileges.

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

However, Google has use cases where Redfish roles, Redfish privileges, and
operation-to-privilege mapping needs to change when the system keeps running.
E.g., a new micro-service is introduced and needs to talk to existing BMCs in
the fleet, we need to propagate a configuration so that this new peer gets a
proper Redfish role and is authorized to access certain resources without
rolling out a new BMC firmware.

Another gap is that current Redfish roles and operation-to-privilege mapping
lead to a very coarse-grained access control, and doesn't implement the
principle of least privilege.

Google has requirement that a given user only has access to the resources it
needs. For example, a micro-service responsible for remote power control can
only send GET to Chassis and ComputerSystems, and POST to corresponding
ResetActions. With the existing implementation, such micro-service has at
least ConfigureComponents Redfish privilege, which leads to being able to
patch an EthernetInterface resource.

## Requirements
BMCWeb implements a dynamic Redfish Authorization system:

1. Clients shall be able to add new OEM privileges without recompile; these
OEM privileges can be company specific; a specific BMC system won't notice
other systems are using customized OEM privileges
2. Clients shall be able to add new OEM Redfish roles with any privileges
assigned without recompile; a specific BMC system won't notice OEM roles on
other systems
3. Clients shall be able to modify existing operation-to-privilege mappings
without recompile; a specific BMC system won't notice customized mappings on
other systems
4. Above changes on systems shall be atomic; that is, once changed, all new
queries will use new configuration
5. BMC shall persist all the above modifications and recover from crash; the
persisted data should not take more than 100KB flash space.
6. BMC shall provide a predefined maximum number of OEM privileges
and OEM roles; it rejects any additional changes when reaching the limit
7. Existing systems with the static authorization shall work as if this feature
is not introduced, including non-Redfish routes (e.g., KVM websocket)
8. Default roles and privilege registry must be selectable on a per system
basis at compile time
9. BMC implements a complete privilege registry; that is
    * it implements all overrides in the Redfish spec; it may support
   configuring overrides at runtime
    * BMC exposes PrivilegeRegistry which represents the current
   configuration and reflects runtime changes
    * Changes to resource entities shall be propagated to the current
   privilege registries automatically
10. All the above shall be testable; CI must be capable of finding issues

## Proposed Design

### 1 to 1 mapping: Group and OEM Redfish Role

To reuse the existing architecture as much as possible, simply implementation,
but still deliver the features, we propose the following 1 to 1 mapping:
one Linux group maps to exact one OEM Redfish role.

This is aligned with how existing Redfish privileges "priv-admin",
"priv-operator", and "priv-user" are managed today.

For example, a power control service with identity (username in PAM, or CN/SAN
in TLS) "oem-power-manager" will be a local user named "power-manager" in
group "priv-power-manager" whose Redfish privilege is "priv-power-manager".
The "priv-power-manager" privilege then will be added the using the dynamic
Operation-to-Privilege mapping we proposed below.

Note that an OEM Redfish role only supports being configured with a single
Redfish privilege. See alternative considered to map an OEM Redfish role to
multiple Redfish privileges (e.g., multiple OEM privileges + base privileges).

LDAP users share the same group and Redfish role mapping, except for that user
accounts are managed by LDAP servers directly.

### Phosphor-user-manager

To provide dynamic Redfish privileges and Redfish roles, we propose to extend
the existing phosphor-user-manager:

1. Phosphor-user-manager provides DBus interfaces to add local groups. These
interfaces are used when OEM roles are added via POST to the AccountService.
When a new OEM role is added, a new group (if necessary) and a new user is
created.
    * In Google's use cases, these APIs will also be called by a dedicated
configuration management daemon which has its own RPC interfaces (non-Redfish)
exposed. This daemon might be upstreamed, but it is out of the scope of this
design.
2. Phosphor-user-manager keeps a list of privileges (including predefined ones
and OEM ones added at runtime) in memory. It also returns AllPrivileges
dynamically. These privileges in phosphor-user-manager are actually Redfish
roles.
3. Phosphor-user-manager provides DBus APIs to query Redfish privileges of a
given user;

### BMCWeb

#### Redfish Privileges
BMCWeb still stores Redfish privileges as bitmasks. No changes expect for now
Redfish privileges are dynamic and include OEM ones.

#### Redfish Roles
BMCWeb still queries all the users via DBus APIs from phosphor-user-manager.
Instead of mapping phosphor-user-manager privileges (Linux groups) to Redfish
roles via a hardcoded table, we propose the following design:

1. When doing authorization, BMCWeb sends DBus queries to get the privilege
Linux group. If the returned group is the predefined base groups ("priv-admin"
"priv-operator", "priv-user"), BMCWeb adds the corresponding base Redfish
privileges according to the existing table. Otherwise, it's an OEM privilege
group (OEM Redfish role), the corresponding (the same string value) Redfish
privilege is added to the request.
2. in Privilege Registry, BMCWeb queries AllPrivileges from
phosphor-user-manager, the OEM privilege groups map to OEM Redfish roles.

#### Non-Redfish Routes or OEM Resources
We still keep the current `privileges` C++ API to add explict Redfish
privileges for non-redfish routes.

#### Redfish Routes
For each `BMCWEB_ROUTE`, instead of taking a privileges array (or reference to
a privileges array), this design proposes to create the following extra
C++ APIs:

1. `entity`: it takes a string representing the Resource name (the same
definition as it in the PrivilegeRegistry); a route must provide either `entity`
or `privileges`
2. `subordinateTargets`: it takes an array of string representing (the same
definition as it in the PrivilegeRegistry); it is optional; for example, a
route with URL "/redfish/v1/Managers/${ManagerId}/EthernetInterfaces/" will
provide an array of {"Manager", "EthernetInterfaceCollection"} as the value of
`subordinateTargets`.

Adding `subordinateTargets` doesn't introduce tech debts since routing URLs
are hardcoded in similar ways.

See below for how we propose to get required Redfish privileges for a given
method on a given resource by using the proposed `entity`, `subordinateTargets`,
and existing `methods` APIs, and their caveats.

#### Dynamic Operation-to-Privilege Mapping Data Structure
BMCWeb keeps a map like data structure. For a given route with known entity
name, HTTP method, and subordinateTargets, the required set of privileges
can be retrieved efficiently (see Mapping Overrides below for authorization
with override configurations).

An authorization operation still the existing bit-wise `isSupersetOf` between
the required privileges of a given operation on a given resource and the
assigned privileges of a role.

This means the required privileges in OperationMap now can be changed at
runtime.

#### Operation-to-Privilege Mapping Overrides

We propose to start with static Operation-to-Privilege Mapping Overrides and
support dynamic configuration as improvement.

To implement static overrides, routes can still use explict privileges set to
respect the base PrivilegeRegistry, and BMCWeb rejects any POST to overrides
configurations.

##### Dynamic Subordinate Override

With `subordinateTargets` added to routes, the authorization code in route
codes can easily look up the subordinate targets in the Operation-to-Privilege
Mapping Data Structure and compare them with what's configured in the route.

Note that this design has a caveat that any entities that support Dynamic
Subordinate Override has to configure corresponding routes with
`subordinateTargets` in advance at compile time.

##### Dynamic Resource URI Override & Dynamic Property Override

We can inject a new Redfish handler in route codes, similar to how query
parameters are implemented, to parse the Operation-to-Privilege Mapping Data
Structure and look for ResourceURIOverrides or PropertyOverrides, and perform
authorization based on properties or URI in request.

These handlers run before the normal authorization codes.

#### Default Privileges
The above proposed data structure will have default values. Those default
values will be generated at compile time by parsing a PrivilegeRegistry in the
read-only partition of the image.

The caveats is that until we implement all dynamic Operation-to-Privilege
Mapping Overrides, the system only supports dynamic privileges in
OperationMap and dynamic Subordinate Overrides in resources that has been
configured with `subordinateTargets`.

#### Persistent Data
We propose store the whole PrivilegeRegistry as a JSON. The compact version
of DMTF's `Redfish_1.3.0_PrivilegeRegistry` is about 62 KB. So the impact is
below:
1. systems that disable dynamic Redfish authorization adds less than 100KB
data to squashfs (read-only partition)
2. systems that enable dynamic Redfish authorization adds less than 100KB data
to jffs2 (read-write partition) in addition to 1

Adding 100KB data to squashfs is acceptable since most upstream systems have
several MB flash space left. Most upstream systems also have serveral MB of
read-write partition configured. For example, `romulus` as of the design was
written, has 3395584 bytes rootfs margin and 4194304 bytes for rwfs.

#### PrivilegeRegistry

BMCWeb will implement PrivilegeRegistry in a new route. The route will reflect
the current PrivilegeRegistry in used including dynamic changes.

With the proposed Dynamic Operation-to-Privilege Mapping Data Structure, the
implementation is trivial: convert the JSON data structure into a JSON
response.

#### REST APIs on PrivilegeRegistry
Though the Redfish spec declares PrivilegeRegistry to be read-only, this
design still proposes implementing REST APIs: users can PATCH any
attributes directly, e.g., patch to OperationMap of a given entity to change
the required privileges.

The upside of this is it makes testing straightforward and doesn't need BMC
console access. However, the implementation is more complicated and not
documented in the spec yet.

Note, not all the attributes are modifiable, for example:

1. OEMPrivilegesUsed can't be changed directly but should be added via
AccountService
2. Overrides can't be changed at runtime without corresponding proposed
handlers being implemented (which might not happen in the first iteration).

## Alternatives Considered
### Customized Persisted Data Format
We can define our own data structure of the persisted data. However,
maintaining our own schema increases maintenance burden and tech debt.
We might also compress the JSON, but given that the total size is less than
100KB, it is not worth the effort.

### Single Redfish Role Maps to Multiple Redfish Privileges
We can extend phosphor-user-manager so that it supports a single user being
multiple privilege Linux groups and exposes APIs for other processes to query
all the privileges.
This is not needed in Google's user case so we will prioritize the other part
of this design.

## Impacts
1. New DBus interfaces on phosphor-user-manager
2. New persistent data managed by BMCWeb will be added on BMCs
3. Existing systems with the static authorization shall work as if this
feature is not introduced

## Testing
Existing tests shall still pass as if this feature is not introduced.
Google will implement automated integration test for downstream systems.