# Redfish Aggregation Design

Author: Deepak Kodihalli <dkodihal>

Primary assignee: Deepak Kodihalli <dkodihal>

Other contributors:

Created: June 21st, 2021

## Problem Description
On systems with multiple BMCs, typically a single platform/central BMC talks to
connected satellite/subordinate BMCs for managing the entire system.
The platform BMC uses one or more management protocols to talk to the satellite
BMCs over one or more physical interconnects, to retrieve typical manageability
information - inventory, sensors, etc. The platform BMC can then aggregate this
information and present the same as a whole to a system administrator, via an
out of band management port. The platform BMC may also autonomously send control
operations to the satellite BMCs. The satellite BMCs are typically responsible
for managing a subsystem - a board, a card, etc. OpenBMC as of this writing
doesn't have a design in place to enable the platform BMC to aggregate
management information from connected satellite BMCs using Redfish. This problem
implies that the platform BMC and the subordinate BMCs run Redfish. The problem
is not specific to Redfish and exists with other management protocols as well.
For example, IPMI can do this via the IPMB bus between the central and satellite
BMCs. The goal of this document is to elaborate requirements around Redfish
aggregation, and propose a design for the same.

## Non-Goals
The goals are covered in the Requirements section. Following are non-goals.
These are related to/can build upon this design, but are large/complex enough to
be covered in design proposals of their own:
- Platform BMC failover to another BMC, which can then take up the task of
Aggregation.
- Discovery of satellite BMCs by the platform BMC using SSDP. This design
assumes peer to peer links (static IP addresses) between the platform and
satellite BMCs.
- Aggregation of Redfish Events that satellite BMCs may push using Server-Sent
events.
- External aggregation (by an outside the box entity).
- Redfish to D-Bus relay. This is, for example, how to consume a Redfish
property, aggregated onto the platform BMCs Redfish service, by an application
that's performing D-Bus property lookups. Sensor monitoring may build upon the
OpenBMC external-sensor design.
- Health rollup policy from satellite BMCs onto the platform BMC.

## Background and References
Most of the content in this section is inferred from the Aggregation section (
Chapter 18) of the Redfish specification, DSP0266 v1.13.0[^1]. The spec talks
about two classes of aggregators - implicit/simple (for eg enclosure managers)
and complex aggregators (involving multiple BMCs). This design deals with the
latter. In a complex aggregator, the Redfish service running on the platform BMC
proxies on behalf of the aggregated satellite BMCs, in order to provide access
to the Redfish service running on the satellite BMCs. Common aggregation use
cases are contained-chassis/board/sub-system management through the platform
BMC, for functions such as inventory reporting, telemetry, firmware update, etc.
This topic has been discussed on the OpenBMC mailing list on several occasions
[^2]. This has also been brought up in the Redfish Forum[^3].

There are two subtly different concepts involved:
1) Consider a system with a single platform BMC and a single connected satellite
BMC. One aspect of aggregation involves the platform BMC adding into its
Redfish ChassisCollection the Redfish Chassis object served by the satellite
BMC. This enables a Redfish client to get to either managed Chassis - one
managed by the platform BMC vs the one managed by the satellite BMC.

2) The other - it might also be beneficial for the Redfish client to perform
operations (such as `Reset`) on both Chassis objects in one shot. To enable
this, the Redfish specification talks about an `Aggregate` schema to perform
such grouping.

### Glossary
* Aggregator - the BMC whose Redfish service proxies on behalf of the connected
satellite BMCs.
* Platform BMC/Central BMC - The aggregator; connected to one or more satellite
BMCs.
* Satellite BMC/Subordinate BMC - The BMC whose Redfish service is proxied;
connected to a platform BMC.

## Requirements

### Requirements for the Aggregator

1) The Aggregator shall be able to proxy for incoming Redfish requests directed
at URIs that map to a satellite BMC.

2) The aggregator shall combine all Redfish resources retrieved from the
satellite BMCs to its own collection. Following are some examples:
    * Add Chassis to `ChassisCollection`
    * Add System to `ComputerSystemCollection`
    * Add Fabric to `FabricCollection`
    * Add Manager to `ManagerCollection`

3) The Aggregator shall be able to act as a Redfish client to talk to the
satellite BMCs.

4) The Aggregator shall have prior knowledge about satellite BMCs that will
participate in Redfish aggregation.

5) The Aggregator shall support cases where the satellite BMC has similar URIs
to the platform BMC.

6) The properties `Links`, `Contains`, `ContainedBy`,`CooledBy` and `PoweredBy`
shall be correct for a given system as a whole.

7) [Conditional] - If the Aggregator relies on a password to authenticate
to the satellite BMCs, then Aggregator shall be able to store the password as a
secret.

8) [Conditional] - If the HostInterface based AuthNone mechanism is used, a
threat model shall be required.

## Proposed Design

### Main Components
Based on the Requirements above, the aggregator design will be comprised of the
following components (the implementations of these could be composed within
Bmcweb, the OpenBMC Redfish server[^4]). The design proposes a stateless
solution, in line with the current architecture of Bmcweb.

1) Redfish Client
To be able to send Redfish requests to satellite BMCs and to receive responses
and events the Aggregator needs to have a Redfish client implementation. A
couple of implementation choices exist, with the first one being preferred:
    a) Repurpose the HTTP client implementation in Bmcweb to serve as a generic
    Redfish client. This is the preferred option because the implementation
    supports async IO and hence is aligned with Bmcweb doing async IO in
    general. The current implementation doesn't support connection pooling and
    the same may be needed to avoid the overhead of creating new TCP
    connections.

    b) Use DMTFs libredfish[^5]. There are a couple of problems with this. One,
    it uses libcurl which does synchronous IO. Second, the licensing of
    libredfish may have to be carefully examined to check for compatibility with
    OpenBMC licenses.

2) Discovery Agent
This component involves identification of the Aggregator and the satellite BMCs,
and determining the authentication mechanism for the satellite BMCs.

    a) Denoting the Aggregator and listing Satellite BMCs
    It is possible that multiple BMCs in a system might be running the same
    Redfish code but one of them has to act as an Aggregator. This will be the
    BMC that has a configuration file listing static IP addresses of satellite
    BMCs. See the section 2c.

    b) Authenticating to satellite BMCs
        * Password Based - as per the Redfish spec it is expected that the
        satellite BMCs shall implement the HTTP basic authentication and Redfish
        session login authentication. Both of these are password based. This
        implies the credentials of the satellite BMCs need to be provisioned to
        the Aggregator. This can be done in two ways. The first option is
        preferred:
        1) The Aggregator can implement the AggregationSource schema, which
        has a writeable Password property that can be provisioned to the
        Aggregator, for each satellite BMC. The Aggregator can store and use
        this password to authenticate to the satellite BMCs.

        2) Specify the default usernames of the satellite BMCs to the
        Aggregator in a configuration file.  See the section 'Configuration
        File Example'. This may not meet security guidelines.

        * Use of the HostInterface::AuthNone - if the HostInterface schema can
        be applied to an Aggregator[^6], then a satellite BMC can implement this
        and this presents the option of unauthenticated (but still being able to
        define privilege levels) access to the Redfish service on the satellite
        BMCs. However, based on the response to the forum thread, the
        applicability of this seems restricted to host CPU based in-band
        management.

    c) Configuration File Example
    An entity-manager or equivalent configuration, such as in the following
    example, can be used to both designate a BMC as an Aggregator and to provide
    information about satellite BMCs. This can be exposed on D-Bus as a property
    of type `dict[string,struct[string, string, string, string, string]]` for a
    `SatelliteId -> {IP address, authtype, username, password, authtoken}`
    mapping. The `SatelliteId` can be used to uniquely distinguish across
    satellite BMCs as well as from the platform BMC. This id can be used to also
    identify satellite BMC Redfish URIs (see 3b) by acting as a prefix or as an
    OEM URI segment. The Aggregator can look up a `SatelliteId` on D-Bus and
    retrieve connectivity (IP address and username) information for a satellite
    BMC. The password and authtoken fields can be set based on provisioning and
    for the purpose of session login authentication. See section 2b.
    ```
    xyz.openbmc_project.SatelliteController: {
        Controllers: {
            "SatteliteId1": {"10.0.0.1", "session-login", "root", "", ""},
            "SatteliteId2": {"10.0.0.2", "session-login", "root", "", ""}
        }
    }
    ```

3) Proxy Agent
    a) As a proxy Redfish service, the Aggregator needs to be able to aggregate
    resources from satellite BMCs into its own resource collection response (see
    Requirement #2). For example, a Redfish GET request on the
    /redfish/v1/Chassis URI made to the Aggregator should receive a
    ChassisCollection in response that contains local and remote Chassis URIs.
    For this purpose a composite pattern may be implemented in Bmcweb:
    ```
                              +------------+
                              |Collection  |
                              |            |
                              |getEntries()|
                              |            |
            +---------------->+-----+------+<-------------------+
            |                       ^                           |
            |                       |                           |
    +-------+-------+               |                   +-------+--------+
    |Chassis        | child         |         child     |Client          |
    |               +-----------+   |    +--------------+                |
    |getEntries()   |           |   |    |              |getEntries()    |
    +---------------+           |   |    |              +-------+--------+
                            +---v---+----v-----+                |
                            |ChassisCollection |         +------v-------+
    +-------------+         |                  |         |redfish client|
    |Discovery    +-------->+getEntries()      |         |              |
    |Agent        |         |                  |         |Connection    |
    |             |         |Response and Error|         |Pooling       |
    +-------------+         |Aggregation       |         +-----+--------+
                            +------------------+               |
                                                               |
                                                       +-------v---------+
                                                       |Satellite BMC    |
                                                       |                 |
                                                       |ChassisCollection|
                                                       +-----------------+

    ```
    The figure above is an example with the `ChassisCollection`. In general, the
    flow below can be employed. When a GET request to retrieve the
    `ChassisCollection` comes in to the Aggregator, this will be processed by
    the `ChassisCollection` route handler's `getEntries` method:
        a) For each child, the corresponding `getEntries` method will be
        invoked.
        b) `Client's` `getEntries` will route the request to a satellite BMC and
        return the response. The set of `Client`s will be determined based on
        D-Bus lookups. See the section 2c.
        c) `Chassis'` `getEntries` will behave as today in Bmcweb (this is for
        the Aggregator's own chassis).
        d) The `ChassisCollection` object will aggregate all these responses.
        The `Contains` property of the Aggregator's `ChassisCollection`
        will need to be populated to indicate containment of Chassis' that are
        being managed by the satellite BMCs. Similarly, the Aggregator will
        perform a PATCH operation on the `ContainedBy` property of the Chassis'
        that are being managed by the satellite BMCs.
        Not all satellite BMCs may respond with success status codes. In such
        cases the Aggregator shall a return a 200 OK but also populate
        @Message.ExtendedInfo objects for the failed members.
    Following is an example output of aggregated `ChassisCollection`. In this
    case, all the three BMCs call a Chassis they manage as 'MyChassis'. However,
    the Aggregator prefixes the Chassis IDs from the satellite BMCs with a
    unique id defined in the Aggregator's configuration to enable distinguishing
    the chassis'. This helps avoid URI collisions as well as the {SatelliteId}
    serves as a key to determine the corresponding client to which requests can
    be relayed to.
    ```
    {
        "@odata.type": "#ChassisCollection.ChassisCollection",
        "Name": "Chassis Collection",
        "Members@odata.count": 3,
        "Members": [{
                "@odata.id": "/redfish/v1/Chassis/MyChassis"
            },
            {
                "@odata.id": "/redfish/v1/Systems/{SatelliteId1}MyChassis"
            },
            {
                "@odata.id": "/redfish/v1/Systems/{SatelliteId2}MyChassis"
            }
        ],
        "@odata.context": "/redfish/v1/$metadata#ChassisCollection.ChassisCollection",
        "@odata.id": "/redfish/v1/Chassis"
    }
    ```

    b) As a proxy Redfish service, the Aggregator needs to be able to route or
    map a URI to either a local handler or a remote handler on a satellite BMC.
    This can be done in several ways in Bmcweb:
    1) In every route handler that exists, check if the URI in the request
    body contains the unique id defined in the Aggregator configuration. If
    it does then the handling of this should be to relay the request to a
    satellite BMC (associated with that id). The {SatelliteId} helps
    determine which satellite BMC to relay the request to. Once a response
    is received from the satellite BMC, the same can be relayed back to the
    Redfish user. For example, a URI /redfish/v1/{SatelliteId}MyChassis
    would be determined as a remote one by the existing handler of
    /redfish/v1/Chassis/<str>. The handler would determine the satellite BMC
    corresponding to  {SatelliteId}, strip off {SatelliteId} from the URI,
    and then relay the request to the satellite BMC. This would require
    modifications to several existing route handlers or this could be
    handled as a base routing function.

    2) Add new routes dynamically for URIs containing the unique id
    associated with a satellite BMC, For example:
    ```
    routeDynamic('/redfish/v1/{SatelliteId}MyChassis/<path>' {handler})
    ```
    The route can be added in the flow described in 3a). The handler would
    determine the satellite BMC corresponding to  {SatelliteId}, strip off
    {SatelliteId} from the URI, and then relay the request to the satellite
    BMC. The advantage of this option would be not needing to modify several
    existing route handlers.

    3) Define an OEM URI segment -
    /redfish/v1/<baseURI>/Oem/OpenBMC/Aggregation/{SatelliteId}/<path>. This
    would be a static route and the handler would relay the Redfish request
    to the appropriate satellite BMC based on the {SatelliteId} and would
    strip out /Oem/OpenBMC/Aggregation/{SatelliteId} before relaying the
    request. Every remote URI would contain the segment
    .../Oem/OpenBMC/Aggregation/{SatelliteId}/... with this approach (the
    flow described in 3a) should add this segment as well). The resource
    .../Oem/OpenBMC/Aggregation/ shall list all satellite Ids. This helps
    separate out remote URIs easily and also doesn't require dynamic route
    addition.

    4) Broadcast every request to all satellite BMCs. This requires careful
    handling of error codes when a remote Redfish service doesn't support a
    resource and also implies unwanted requests being made.
    The preferred options are 2 or 3.

    c) As a proxy Redfish service, the Aggregator needs to be able to unify
    certain services, unless option 3b3 is chosen. The implementation may have
    to be service specific.

    Following is an example for `UpdateService`. The URI in the request (unless
    option 3b3 is chosen) would be /redfish/v1/UpdateService irrespective of
    whether the request is directed at the platform BMC or a satellite BMC.
    The Aggregator can receive an image the corresponds to firmware that it
    can update or a satellite BMC can update. One way to distinguish this
    would be to expect the Redfish client to populate the
    `HttpPushUriTargets` field when POSTing an image to the Aggregator. The
    values should map to the resources that are managed by a satellite BMC.
    This information can be used by the Aggregator to route the image to the
    appropriate satellite BMC. The following figure depicts this:
    ```
    Redfish Client                Aggregator                                                   Satellite BMC
    +                             +----------------------------------------------+             +--------------------+
    |                             | Bmcweb                                       |             |                    |
    |                             |                                              |             |/redfish/v1/Managers|
    |                             |                                              |             |/{SatelliteId}BMC   |
    |                             |                                              |             |                    |
    |                             |                                              |             |                    |
    |                             |                                              |             |                    |
    |                             |                                              |             |                    |
    |                             |                                              |             |                    |
    | POST image to /redfish/v1/  |                                              |             |                    |
    | UpdateService               |      +---------------+                       |             |                    |
    +----------------------------------->+ UpdateService |                       |             |                    |
    |      Response               |      | Implementation|                       |             |                    |
    +<-----------------------------------+               |                       |             |                    |
    |                             |      |               |                       |             | +----------------+ |
    |                             |      |               |                       |             | | UpdateService  | |
    | POST image to /redfish/v1/  |      |               | POST image to /redfish/v1/          | | Implementation | |
    | UpdateService               |      |               | UpdateService         |             | |                | |
    +----------------------------------->+               +-------------------------------------> |                | |
    | HttpPushUriTargets=         |      |               |        Response       |             | |                | |
    | /redfish/v1/Managers/       |      |               +<--------------------------------------+                | |
    | {SatelliteId}BMC            |      |               |                       |             | |                | |
    +<-----------------------------------+---------------+                       |             | +----------------+ |
    |        Response             |                                              |             |                    |
    +                             +----------------------------------------------+             +--------------------+
    ```

## Alternatives
Instead of implementing the Aggregator in the Bmcweb, another option would be to
implement the same in another D-Bus service. At a high level, this would
comprise of the following:
* Bmcweb relies on mostly existing D-Bus interfaces to access resources and
services.
* A new service implements D-Bus interfaces that Bmcweb relies on, and
implements a D-Bus to Redfish relay. It translates a D-Bus call to a Redfish
request, relays the same to appropriate client, and convert the Redfish response
back to D-Bus.
There are some disadvantages of this alternative:
* D-Bus to Redfish request translation may not be straight forward and might
require newer D-Bus interfaces.
* Performance impact because there is an intermediate
Redfish Request->D-Bus->Redfish Response->D-Bus conversion.

Since this design is specific to the BMCs talking Redfish, it doesn't cover the
fact the BMCs could talk other protocols, for example PLDM, and the platform BMC
could still serve as the out-of-band Redfish service. Based on how the BMCs are
connected, any of the following could work for BMC to BMC communications:
- Redfish
- PLDM (with one or more BMCs acting as PLDM responders)
- IPMI (for example over IPMB)

## Impacts
* If used for BMC to BMC authentication, the AuthNone authentication mechanism
is a potential DSP0266 violation.
* If used, an OEM URI segment to separate out satellite BMC Redfish URIs would
require clients to support issuing requests with the OEM URI segments.
* Bmcweb will need to deserialize JSON to aggregate Redfish responses coming in
from satellite BMCs, which may be a security impact.

## Testing
This can be tested as follows using QEMU:
* Connect two (or more) instances of QEMU running the same OpenBMC code using a
netdev socket.
* One of the instances has to be provisioned with a configuration file as
discussed in this design so that it can act as an Aggregator. The remaining
instances will then be satellite BMCs.
* The QEMU image should have some Redfish resources and services which can be
used to test Redfish Aggregation.
* An HTTP client utility (like cURL) can be used to send Redfish GET requests
containing URIs that are supposed to return resource collections, for example
/redfish/v1/Chassis. This should help test that the Aggregator BMC is retrieving
resources from the satellite BMCs.
* An HTTP client utility (like cURL) can be used to send Redfish
GET/POST/PATCH/DELETE requests on remote URIs. This should help test that the
Aggregator BMC is proxying requests to the relevant satellite BMCs.
* An HTTP client utility (like cURL) can be used to send Redfish requests to
access services such as /redfish/v1/UpdateService. This should help test if the
Aggregator is able to relay the request to a satellite BMCs service.
* By not provisioning satellite BMC authentication information to the
Aggregator, it should be possible to test if the Aggregator returns an
appropriate status code.
* By sending Redfish requests that are bound to fail (for example a write
attempt to a read-only property) to remote URIs, it should be possible to test
that the Aggregator is relaying status code from the satellite BMCs back to the
user.
* Failure modes, such as non-existing resource, timeout, lack of privilege, etc.
can be simulated on the satellite BMC QEMU instances to help test if the
Aggregator is reporting failures for a case where certain satellite BMCs respond
with passing status code and the rest with failures.

## Footnotes
[^1]: https://www.dmtf.org/sites/default/files/standards/documents/DSP0266_1.13.0.pdf
[^2]: https://lore.kernel.org/openbmc/?q=redfish+aggregation
[^3]: https://redfishforum.com/thread/457/redfish-aggregation-bmcs
[^4]: https://github.com/openbmc/bmcweb
[^5]: https://github.com/DMTF/libredfish
[^6]: https://redfishforum.com/thread/501/applicability-hostinterface-schema
