# Redfish Aggregation Design

Author: Deepak Kodihalli <dkodihal>

Primary assignee: Deepak Kodihalli <dkodihal>

Other contributors:

Created: June 21st, 2021

## Problem Description
On systems with multiple BMCs, typically a single platform/central BMC talks to
connected satellite/subordinate BMCs for managing the entire system.
The platform BMC uses one ore more management protocols to talk to the satellite
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

While this design mostly focuses on heterogenous BMCs, most of it applies to a
homoegenous BMC case as well. For example a case where multiple BMCs, each
managing a host node, and one of the BMCs is elected as the point of contact for
the external world. The mechanisms described in this design for discovery,
choosing the Aggregator, and aggregation, will apply to homogenous BMCs. In
addition, the homogenous BMCs case would need a way to failover when the elected
Aggregator goes down.

This design is about on-system aggregation vs an external aggregator (which the
Redfish AggregationService and related schemas enable). This design does talk
about optinally using the AggregationService and related schemas to group
certain Redfish actions.

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

1) The Aggregator shall be able to discover, or have prior knowledge about,
satellite BMCs that will participate in Redfish aggregation.

2) The Aggregator shall be able to act as a Redfish client to talk to the
satellite BMCs.

3) The Aggregator shall be able implement authentication and authorization flows
to access the Redfish service on the satellite BMCs, as per the Redfish spec.

4) Post discovery, the aggregator shall combine top-level Redfish resources
retrieved from the satellite BMCs to its own collection. Following is a
non-exhaustive list:
    * Add Chassis to `ChassisCollection`
    * Add System to `ComputerSystemCollection`
    * Add Fabric to `FabricCollection`
    * Add Manager to `ManagerCollection`

5) The Aggregator shall perform a URI fix-up of Resources added to Resource
Collections. This is needed because the satellite BMCs could report the same
URIs.

6) The Aggregator shall fix-up the `Links` property, and update fields such as
`Contains`, `ContainedBy`,`CooledBy`, `PoweredBy`, etc,  in the satellite BMC's
Redfish resource model and/or its own model.

7) The Aggregator shall be able to proxy for incoming Requests directed at URIs
in a Resource Collection that map to a satellite BMC.

8) The aggregator shall be able to aggregate Redfish services from satellite
BMCs to be able to unify the functions provided by these services for the
Redfish client. Following is a non-exhaustive list:
    * `UpdateService`
    * `EventService`
    * `TelemetryService`
    * `AccountService`
    * `CertificateService`

9) The Aggregator may indicate to a Redfish client that it enables Redfish
aggregation by implementing the `AggregationService` schema and including an
`AggregationSourceCollection`. The latter would typically point to the satellite
BMCs' Manager resource, connection information for the same. It is of course
possible that these satellite BMCs can be accessed only through the Aggregator.

10) The Aggregator may implement Actions as defined in the `AggregationService`
schema, if they're relevant. This is to be able to perform group operations (
such as `Reset`), if applicable.

11) The Aggregator may implement the `Aggregate` schema and related Actions to
let a Redfish client perform group actions such as `Reset`.

12) The Aggregator shall roll up health statuses of aggregated resources onto
itself.

13) The Aggregator may autonomously send Redfish requests to the satellite
BMCs and convert some of the Redfish resource information retrieved into it's
native (typically D-Bus) model, to be able to perform  functions such as fan
control.

### Requirements for the satellite BMC

1) Redfish implementations should strive to use unique values for the `Id` of
various resources, for example a serial number for the `System` resource.
This simplifies URI fix-up for the Aggregator.

## Proposed Design

### Main Components
Based on the Requirements above, the aggregator design can be comprised of the
following components (the implementations of these could be composed within
bmcweb, the OpenBMC Redfish server[^4]):

* Redfish Client
To be able to send Redfish requests to satellite BMCs and to receive responses
and events the Aggregator needs to have a Redfish client implementation. For
this purpose, bmcweb can be linked with the DMTF libredfish C library[^5].

* Discovery Agent
This component involves identification of the Aggregator and the satellite BMCs,
enabling aggregation, and determining the authentication mechanism(s) for the
satellite BMCs.

    * Denoting the Aggregator
    It is possible that multiple BMCs in a system might be running the same code
    image but one of them has to act as an Aggregator. For this reason, a
    dynamic method to designate am Aggregator is preferred. A common way to do
    differentiate this is some FRU EEPROM property which would vary across the
    BMCs, and once such a property and a specific value is found, a D-Bus
    interface such as the following can be added by an entity-manager
    configuration file[^6]:
    ```
    xyz.openbmc_project.Configuration.BMC.Aggregator
    ```
    If the Redfish server finds this D-Bus interface implemented,
    then it can execute aggregation flow paths in the code.

    * Addresses of satellite BMCs
    The Aggregator and the satellite BMCs typically have point-to-point
    connections with static IP addresses for the satellite BMCs, but this need
    not always be the case. The specific neighbor discovery protocol used and
    the method of IP address assignment for the satellite BMCs is out of scope
    for this document, but a simple implementation in case of static IPs could
    be a service that makes use of `ip-neighbor`[^7], which phosphor-networkd
    supports already. The satellite BMCs shall be represented on D-Bus by
    adding the following existing D-Bus interface:
    ```
    xyz.openbmc_project.Network.Neighbor
    ```
    Post knowing the addresses, the Aggregator can check if these
    neighbors implement Redfish (for example by trying to access the service
    root). If they do, the Aggregator shall add the corresponding Manager
    resources to the `AggregationSourceCollection`.

    * Authenticating to satellite BMCs
        * As per the Redfish spec it is expected that the satellite BMCs shall
        implement the HTTP basic authentication and Redfish session login
        authentication. Both of these are password based (see Impact section).
        This implies the credentials of the satellite BMCs need to be
        provisioned to the Aggregator. One way is to include this (username and
        password), again via an entity-manager config, as properties added to
        the Network.Neighbor or the BMC.Aggregator interface.

        * Alternate (non-Redfish-standard) authentication mechanims may also be
        supported, such as mTLS[^8]. With mTLS, the authentication can be
        password-less, however a password is required for the initial
        provisioning of mTLS.

        * Use of the HostInterface::AuthNone - if the HostInterface schema can
        be applied to an Aggregator[^9], then a satellite BMC can implement this
        and this presents the option of unauthenticated (but still being able to
        define privilege levels) access to the Redfish service on the satellite
        BMCs. However, based on the response to the forum thread, the
        applicability of this seems restricted to host CPU based in-band
        management.

* Proxy Agent
    * As a proxy Redfish service, the Aggregator needs to be able to build
    top-level resource collections (see Requirement #4). For this purpose a
    composite pattern may be implemented in bmcweb:
    ```
                              +------------+
                              |Collection  |
                              |            |
                              |addEntries()|
                              |addChild()  |
            +---------------->-------------+<-------------------+
            |                       ^                           |
            |                       |                           |
    +-------+-------+               |                   +-------+--------+
    |Chassis        | child         |         child     |Proxy           |
    |               +-----------+   |    +--------------+                |
    |addEntries()   |           |   |    |              |addEntries()    |
    +---------------+           |   |    |              +-------+--------+
                            +---v--------v-----+                |
                            |ChassisCollection |                |
    +-------------+  add    |                  |                |
    |Discovery    +-------->+addEntries()      |                v
    |Agent        |         +------------------+         +------+----+
    |             |                                      |libredfish |
    +-------------+                                      +-----+-----+
                                                               |
                                                               |
                                                       +-------v---------+
                                                       |Satellite BMC    |
                                                       |                 |
                                                       |ChassisCollection|
                                                       +-----------------+
    
    ```
    The figure above is an example with the `ChassisCollection`. In general, the
    flow below can be employed:
    1) Post discovery, the Discovery Agent will create instances of the `Proxy`
    class, one per satellite BMC. These objects accept connection (to the
    satellite BMC) information and make use of libredfish to transmit and
    receive Redfish, via their `addEntries` method.
    2) The Discovery Agent will also, for each Collection instance, such as
    `ChassisCollection`, add the `Proxy` instances as children via the
    `addChild` method.
    3) When a GET request to retrieve `ChassisCollection` comes in to the
    Aggregator, this will be processed by the `ChassisCollection` instance, by
    the `addEntries` method:
        a) For each child, the corresponding `addEntries` method will be
        invoked.
        b) `Proxy's` `addChild` will route the request to a satellite BMC and
        return the response.
        c) `Chassis'` `addChild` will behave as today in bmcweb (this is for the
        Aggregator's own chassis). The
        d) `ChassiCollection` object will aggregate all these responses. It will
        perform URI fix-ups if required (if multiple satellite BMCs returns same
        URIs) - in this case it would need to maintain a local mapping of
        fixed-up URI to original URI. Also, the `Contains` property of the
        Aggregator's `ChassisCollection` and the `ContainedBy` of the satellite
        BMC's `ChassisCollection` entries need to be populated (via a PATCH
        operation).
    This can also be performed right after discovery, as opposed to doing it
    lazily on receiving a GET request. In addition the `ChassisCollection`
    object or the `Proxy` object may implement a cache of retrieved read-only
    responses.
    Following is an example output of aggregated `ComputerSystemCollection`:
    ```
    {
        "@odata.type": "#ComputerSystemCollection.ComputerSystemCollection",
        "Name": "Computer System Collection",
        "Members@odata.count": 2,
        "Members": [{
                "@odata.id": "/redfish/v1/Systems/529QB9450R6"
            },
            {
                "@odata.id": "/redfish/v1/Systems/529QB9451R6"
            }
        ],
        "@odata.context": "/redfish/v1/$metadata#ComputerSystemCollection.ComputerSystemCollection",
        "@odata.id": "/redfish/v1/Systems"
    }
    ```
    As noted in the Requirements, the Aggregator may also implement the
    `Aggregate` schema, for example to group the two Systems above in order to
    perform Redfish Actions such as `Reset` on the group:
    ```
    {
        "@odata.type": "#Aggregate.v1_0_1.Aggregate",
        "Id": "Aggregate1",
        "Name": "Aggregated System",
        "ElementsCount": 2,
        "Elements": [
            {
                "@odata.id": "/redfish/v1/Systems/529QB9450R6"
            },
            {
                "@odata.id": "/redfish/v1/Systems/529QB9450R6"
            }
        ],
        "Actions": {
            "#Aggregate.Reset": {
            "target": "/redfish/v1/AggregationService/Aggregates/Aggregate1/Actions/Aggregate.Reset",
            "@Redfish.ActionInfo": "/redfish/v1/AggregationService/Aggregates/Aggregate1/ResetActionInfo"
            }
        },
        "@odata.id": "/redfish/v1/AggregationService/Aggregates/Aggregate1"
    }
    ```

    * As a proxy Redfish service, the Aggregator needs to be able to route or
    map a URI to either a local handler or a remote handler on a satellite BMC.
    There could be multiple options to implement this:
        * One (naive) way is to check in every existing route handler whether
        the URI corresponds to a local or remote resource, and if it's the
        latter then route the request using libredfish. The `Proxy` class
        instances shall maintain a set of URIs for the top-level resources that
        are housed by the Redfish service on a satellite BMC. This will help
        check if a URI is local or remote.
        * Another option would be to dynamically add routes as remote
        entries are discovered and added to the Aggregator's collection. For
        example, if a remote /redfish/v1/Chassis/Foo is discovered, then a
        handler can be added for `/redfish/v1/Chassis/Foo` and
        `/redfish/v1/Chassis/Foo/<str>`. The handler in this case would route
        the URI using libredfish. Such a handler can be implemented in the
        `Proxy` class.
        * Others?

    Read-only resources may be cached in the `Proxy` instances.
    The figure below shows a GET being handled for a local vs remote URL:
    ```
    Redfish Client                             Aggregator                                                Satellite BMC
    +                                         +------------------------------------------------+         +------------+
    |                                         |bmcweb                                          |         |bmcweb      |
    |GET /redfish/v1/Chassis/Local/LogServices|                                                |         |            |
    |                                         | URL=/redfish/v1/Chassis/Local/LogServices ->   |         |            |
    +---------------------------------------->+ local GET handler                              |         |            |
    |              Response                   |                                                |         |            |
    <-----------------------------------------+                                                |         |            |
    |                                         |                                                |         |            |
    |GET /redfish/v1/Chassis/Remote/LogSvc    |                                                |         |            |
    |                                         |                                                |  GET    |            |
    +---------------------------------------->+ URL=/redfish/v1/Chassis/Remote/<string> ->     +-------->+            |
    |                                         | Proxy GET handler                              |         |            |
    |             Response                    |                                                <---------+            |
    <-----------------------------------------+                                                | Response|            |
    |                                         +------------------------------------------------+         +------------+
    +
    ```

    * As a proxy Redfish service, the Aggregator needs to be able to unify
    certain services. The implementation may have to be service specific:
        * `UpdateService`
        The Aggregator can receive an image the corresponds to firmware that it
        can update or a satellite BMC can update. One way to distinguish this
        would be to expect the Redfish client to populate the
        `HttpPushUriTargets` field when POSTing an image to the Aggregator. The
        values should map to the resources that the Aggregator adds to the
        `AggregatorSourceCollection`, or basically a remote Manager URI. This
        can be used by the Aggregator to route the image to the appropriate
        satellite BMC. The following figure depicts this:
        ```
        Redfish Client                Aggregator                                                   Satellite BMC
        +                             +----------------------------------------------+             +--------------------+
        |                             | Bmcweb                                       |             |                    |
        |                             |                                              |             |/redfish/v1/Managers|
        |                             | AggregateSourceCollection                    |             |/Remote             |
        |                             | .Links                                       |             |                    |
        |                             |   .ResourcesAccessed                         |             |                    |
        |                             |     /redfish/v1/Managers/Remote              |             |                    |
        |                             |                                              |             |                    |
        |                             |                                              |             |                    |
        | POST image to /redfish/v1/  |                                              |             |                    |
        | UpdateSerVice               |      +---------------+                       |             |                    |
        +----------------------------------->+ UpdateService |                       |             |                    |
        |      Response               |      | Implementation|                       |             |                    |
        +<-----------------------------------+               |                       |             |                    |
        |                             |      |               |                       |             | +----------------+ |
        |                             |      |               |                       |             | | UpdateService  | |
        | POST image to /redfish/v1/  |      |               | POST image to /redfish/v1/          | | Implementation | |
        | UpdateSerVice               |      |               | UpdateSerVice         |             | |                | |
        +----------------------------------->+               +-------------------------------------> |                | |
        | HttpPushUriTargets=         |      |               |        Response       |             | |                | |
        | /redfish/v1/Managers/Remote |      |               +<--------------------------------------+                | |
        |                             |      |               |                       |             | |                | |
        +<-----------------------------------+---------------+                       |             | +----------------+ |
        |        Response             |                                              |             |                    |
        +                             +----------------------------------------------+             +--------------------+
        ```

        * `AccountService`, `ManagerAccount`
        The Aggregator can proxy a satellite BMC's `AccountService` or
        `ManagerAccount` under
        `/redfish/v1/Manager/<ManagerId>/RemoteAccountService` with the
        `AccountProviderType` set to `RedfishService`.

        * `CertificateService`
        The Aggregator can proxy in the `Links` property of it's
        `CertificateLocations` resource the URIs pointing to Certficates on the
        satellite BMCs.

        * `EventService`
        The Aggregator shall maintain a single event stream. Subscriptions
        POSTed to the Aggregator's EventService by a Redfish client will be
        relayed by the Aggregator to the satellite BMCs' EventService, by
        POSTing the same `EventDestination`. Events received by satellite BMCs
        will be relayed back to the Redfish client by the Aggregator using the
        mechanism the Redfish client requested, for example SSE.
        The existing Event Service design[^10] can be leveraged here.

    * Error handling and health roll-up
    The Aggregator shall aggregate errors from satellite BMCs. It is possible
    that failures might happen only on certain satellite BMCs. The error
    messages from the platform BMC must detail which specific resources/requests
    failed.

* Redfish to D-Bus Conversion Agent
This is primarily to be able to consume sensors on satellite BMCs into the fan
control algorithm of the Aggregator. This involves Redfish requests sent
autonomously by the Aggregator to poll sensors from the satellite BMC (or
subscribe to a `MetricReport` if the satellite BMC supports the
`TelemetryService`). These sensors can be represented on D-Bus using the
external/virtual sensor[^11] mechanism, which will allow incorporating these
into phosphor-pid-control[^12] zones. The Aggregator will have to parse Redfish
responses (JSON) and relay those to appropriate D-Bus interfaces.

## Alternatives
Since this design is specific to the BMCs talking Redfish, it doesn't cover the
fact the BMCs could talk other protocols, for example PLDM, and the platform BMC
could still serve as the out-of-band Redfish service. Based on how the BMCs are
connected, any of the following could work for BMC to BMC communications:
- Redfish
- PLDM (with one or more BMCs acting as PLDM responders)
- IPMI (for example over IPMB)

## Impacts

* If the Aggregator uses a password-based authentication mechanism with the
satellite BMCs, then it shall be responsible for password management, expiry,
etc.

* If the HostInteface based AuthNone mechanism is usable, a threat model
exercise may be needed to determine threats. A threat model will be needed in
general.

* libredfish will have to be assessed in terms of OpenBMC's code standards,
binary size, etc.

## Testing
Most of this can be tested with two instances of bmcweb, for example running on
two instances of QEMU, with one instance of bmcweb acting as the Aggregator and
the other as a regular Redfish service.

## Footnotes
[^1]: https://www.dmtf.org/sites/default/files/standards/documents/DSP0266_1.13.0.pdf
[^2]: https://lore.kernel.org/openbmc/?q=redfish+aggregation
[^3]: https://redfishforum.com/thread/457/redfish-aggregation-bmcs
[^4]: https://github.com/openbmc/bmcweb
[^5]: https://github.com/DMTF/libredfish
[^6]: https://github.com/openbmc/entity-manager/tree/master/configurations
[^7]: https://man7.org/linux/man-pages/man8/ip-neighbour.8.html
[^8]: https://github.com/openbmc/docs/blob/master/designs/redfish-tls-user-authentication.md
[^9]: https://redfishforum.com/thread/501/applicability-hostinterface-schema
[^10]: https://github.com/openbmc/docs/blob/master/designs/redfish-eventservice.md
[^11]: https://gerrit.openbmc-project.xyz/c/openbmc/docs/+/41452
[^12]: https://github.com/openbmc/phosphor-pid-control#zone-specification
