# Dynamic Redfish Authorization
Author: Nan Zhou (nanzhoumails@gmail.com)
Primary assignee: Nan Zhou
Created: 08/08/2022

## Problem Description

The Redfish authorization subsystem controls which authenticated users have
access to resources and the type of access that users have.

DMTF has already defined the authorization model and schemas. This design is
to enhance the current implementation in BMCWeb Redfish interface so that
BMCWeb exposes methods to change authorization behavior without BMC or
service reset.

## Background and References

### Redfish Authorization Model
The Redfish authorization model consists of the [privilege model](https://redfish.dmtf.org/schemas/DSP0266_1.15.1.html#privilege-model) and
the [operation-to-privilege mapping](https://redfish.dmtf.org/schemas/DSP0266_1.15.1.html#redfish-service-operation-to-privilege-mapping).

In the privilege model, there are fixed set of standard [Redfish roles](https://redfish.dmtf.org/schemas/DSP0266_1.15.1.html#roles) and
each of them is assigned a fixed array of standard privileges (e.g., `Login
`, `ConfigureManager`, etc). A service may define custom OEM roles (read
-only). A service may even allow custom client-defined roles to be created,
modified, and deleted via REST operations on `RoleCollection`.

The operation-to-privilege mapping is defined for every resource type
and applies to every resource the service implements for the applicable
resource type. It is used to determine whether the identity privileges of an
authenticated Redfish role are sufficient to complete the operation in the
request. The Redfish Forum provides a [Privilege Registry definition](https://redfish.dmtf.org/registries/v1/Redfish_1.3.0_PrivilegeRegistry.json) in
its official [registry collection](https://redfish.dmtf.org/registries/) as a base
operation-to-privilege mapping. It also allows mapping overrides (Property
override, Subordinate override, and Resource URI override) to customize
mapping.

For example, a peer who is an `Operator` that has both `Login`,
`ConfigureComponents`, and `ConfigureSelf` privileges, is authorized to send
a `GET` request to the `ChassisCollection`, since the `GET` operation on this
resource only requires the `Login` privilege. On the other hand, the same peer
gets denied access if they send a POST request to `CertificateService`, as
the POST operation on certificates requires `ConfigureManager` privilege that
the peer is missing.

**Note**, in the Redfish spec, OEM roles can be added via POST to the
`AccountService`; `PrivilegesUsed`, `OEMPrivilegesUsed`, and properties of
`Mappings` are all read-only.

### Status Quo and Gaps

As of this design is proposed, BMCWeb implements the Redfish authorization
in a **static** way: any updates to the authorization, e.g., an existing
resource now needs a new privilege, needs a new firmware roll out.

However, Google has use cases where Redfish roles, privileges, and
operation-to-privilege mapping needs to change when the system keeps running.
E.g., a new micro-service is introduced and needs to talk to existing BMCs in
the fleet, we need to propagate a configuration so that this new peer gets a
proper Redfish role and is authorized to access certain resources without
rolling out a new BMC firmware.

## Requirements

BMCWeb implements a dynamic Redfish Authorization model:

1. Clients shall be able to add new OEM privileges without service
restarts; these OEM privileges can be company specific; upstream BMCWeb
won't notice any downstream systems are using customized OEM privileges
2. Clients shall be able to add new OEM Redfish roles with any
privileges assigned without service restarts; upstream BMCWeb won't notice
any downstream OEM roles
3. Clients shall be able to modify existing operation-to-privilege
mappings without service restarts; upstream BMCWeb won't notice any
downstream mappings
4. Changes to above 
5. BMCWeb shall persist all the above modifications and recover from crash; the
persisted data shall not take a significant amount of disk nor memory
6. BMCWeb shall provide a predefined maximum number of OEM privileges
and OEM roles; it rejects any additional changes when reaching the limit
7. Existing systems with the static authorization shall work as if this feature
is not introduced
8. All the above shall be testable

## Proposed Design

### BMCWeb

#### Privileges
Instead of storing string arrays and references to arrays to represent a
set of privileges, this design proposes to store privileges as bitmasks

#### Roles
BMCWeb keeps a list of Redfish roles in memory. Each OEM role has a bitmask
associated with it representing the assigned privileges for this role.

#### Route
For each `BMCWEB_ROUTE`, instead of taking a privileges array (or reference to
a privileges array), this design proposes to take an `Entity` for the given
route. The `Entity` has the same definition as it in the PrivilegeRegistry
schema, which refers to Resource name.

The non-Redfish routes will have some customized entity.

#### Operation-to-Privilege Mapping
BMCWeb keeps a map like data structure. For a given route with known entity
name and HTTP method, the required set of privileges (again, a bitmask) can be
retrieved efficiently.

An authorization operation can be implemented as a bit-wise `IsSubset` between
the required privileges of a given operation on a given resource and the
assigned privileges of a role.

#### Base Privileges

The above proposed data structure will have default values. Those default
values will be generated at compile time by parsing DMTF's base
PrivilegeRegistry.

#### Persistent Data
We propose store the whole PrivilegeRegistry as a JSON. A simple optimization
that we propose do is to store `Privilege` as integers instead of array of
strings. The compact version of DMTF's `Redfish_1.3.0_PrivilegeRegistry` is
about 62 KB. With the bitmask optimization, the implementation will take less
size. Another common sense is that adding new Operation-to-Privilege mappings
won't increase the JSON size a lot given that a system only grants limited
resources to a specific role. We also limit the number of maximum roles to be
added, so the size of the persistent data is limited and predictable.

Systems that don't use this feature will not store any persistent data. BMCWeb
starts up using the default values generated at compile time.

## APIs

### Option 1: phosphor-dbus-interfaces
BMCWeb exposes the following high level DBus APIs,

1. `reloadPrivilegeRegistry`: BMCWeb reads a path to new PrivilegeRegistry
, builds the in-memory data structure, and use it to authorize new queries,
including sub-queries when clients use query parameters. See the note below.

Note: `$expand` queries might get inconsistent view of PrivilegeRegistry. To
make all sub-queries of an `$expand` query use the same PrivilegeRegistry
, we need to copy PrivilegeRegistry per connection which is too expensive. This
design accepts inconsistent view of PrivilegeRegistry for queries that might
generate sub-queries.

This is very similar to loading a new root certificate or a new server key
and certificate pair. It is simple and effective. Other RPC servers, e.g. gRPC,
provides similar APIs to reload authorization configurations.

Any invalid PrivilegeRegistry will be rejected and BMCWeb keeps the old one.

### Option 2: REST APIs on PrivilegeRegistry
Though the Redfish spec declares PrivilegeRegistry to be read-only, OpenBMC's
implementation can still implement REST APIs: users can PATCH any attributes
directly, e.g., patch to OEMPrivilegesUsed to create a new OEM privilege.

The upside of this is it makes testing straightforward and doesn't need BMC
console access. However, the implementation is more complicated and not
documented in the spec yet.

### Preference
This design prefers to implement Option 1 as an MVP. We will contact DMTF for
clarification of option 2 and implement certain REST APIs. Codes developed
for option 1 can be reused for option 2.

## Alternatives Considered
We can define our own data structure of the persisted data. However,
maintaining our own schema increases maintenance burden and tech debt.

We might also compress the JSON, but given that the total size is less than
100KB, it is not worth the effort.

## Impacts
1. New DBus interfaces on BMCWeb
2. New persistent data managed by BMCWeb will be added on BMCs
3. Existing systems with the static authorization shall work as if this
feature is not introduced

## Testing
Existing tests shall still pass as if this feature is not introduced.
Google will implement automated integration test for downstream systems.
