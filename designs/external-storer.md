# ExternalStorer in bmcweb

Author: Josh Lehan
\<[Krellan](https://gerrit.openbmc-project.xyz/q/owner:krellan%2540google.com)\>

Primary assignee: Josh Lehan

Other contributors: None

Created: March 3, 2022

## Problem Description

The OpenBMC Redfish server, [*bmcweb*](https://github.com/openbmc/bmcweb), can
already serve a variety of internally-generated information, but currently,
there is no way to serve information previously provided by an external source.
This document proposes a new *ExternalStorer* feature, to be added to bmcweb, to
store externally-provided data for later retrieval. This allows the Redfish tree
to serve as a convenient stash-point for a variety of user-provided data, while
still remaining compliant with Redfish schemas.

## Background and References

As Redfish is a specialization of a Web server, it is assumed that the reader is
already familiar with HTTP, JSON, GET and POST operations, and other Web
concepts and standards. The *bmcweb* implementation already has several places
in which externally-provided data can be provided, using the POST operation, but
this data is merely to provide context for an intended operation, not intended
to be arbitrarily stored for later query. It is desirable to have a way for
externally-provided data to be retained, and made available in the same way that
internally-generated data would be. This allows the same software to be used to
manage both internal and external data, for consistency and ease of operation.
This rationale is similar to the
[*ExternalSensor*](https://github.com/openbmc/docs/blob/master/designs/external-sensor.md)
feature of *dbus-sensors*, hence the intentionally similar name.

The Redfish tree is governed by schemas, so the entire tree cannot be
arbitrarily modified. Otherwise, a more generic Web editing protocol, such as
[*WebDAV*](https://datatracker.ietf.org/doc/html/rfc4918), could simply be
exposed for external users. The Redfish schemas only permit certain subsets of
the Redfish tree to be extended by the implementation. However, these provide
opportunities where ExternalStorer can be instantiated, as needed.

## Requirements

The primary requirement for ExternalStorer is to have some backing storage
allocated to it. The storage provided by ExternalStorer need not be permanent,
as the BMC has little permanent storage space. However, the lack of permanent
storage should not be a limitation, as the intention is to store data that is
ephemeral in nature, such as logging messages, error notifications, performance
counters, status updates, and so on. The backing storage can thus exist in
memory only, to be cleared whenever the BMC reboots. The amount of backing
storage should have a defined limit, to avoid exhausting available memory.
Unlike traditional storage systems, an ExternalStorer instance will still allow
new content to be stored, if this limit would be exceeded. To make room for this
new content, old content will be silently dropped, in order from oldest to
newest. The rationale for this fits into the intended use of ExternalStorer for
temporary storage.

As ExternalStorer is intended to be used to store externally-provided data, an
API is required to be provided for it, so that developers of external services
can easily use it. This API needs to be available, both for HTTP operations on
the Redfish server, and for direct manipulation of the underlying backing
storage. This is to avoid introducing the overhead and complexity of HTTP into
locally-running services on the BMC itself. This permits a locally-running
service to freely manipulate the contents of the backing storage, in lieu of
having to do equivalent HTTP operations. This implies that the backing storage
should be something readily accessible to other processes, such as an in-memory
filesystem, and that no caching should be done by the implementation of
ExternalStorer, lest they get out of sync. This also implies that a
locally-running service would have a high level of trust, as it would have
direct access to the backing storage, and thus be able to bypass any preflight
checks ExternalStorer might make on received data before storing it, such as
error checking, storage capacity checking, and so on.

The storage unit of ExternalStorer will be a JSON dictionary, as this is the
most fundamental JSON object, capable of containing any other desired JSON type,
and is already widely used within Redfish. The API will support HTTP operations
in a [RESTful](https://en.wikipedia.org/wiki/Representational_state_transfer)
manner. POST will publish data. GET will retrieve data previously published.
DELETE will delete it. PUT will completely replace existing published data,
while PATCH will replace it more selectively, preserving unchanged fields of the
JSON dictionary.

In addition to storing and retrieving data, that data must be organized into
containers. As required by Redfish schemas and intended usage, two layers are
necessary. ExternalStorer will allow a container to be created and manipulated,
and then, allow individual data elements to be created and manipulated within
that container. This will allow an external service to create a container for
data it might want to publish, such as error notifications, then write data into
that container as needed. Both the containers, and the elements placed within
these containers, will be identified by strings. These are usually assigned
internally, but external users can also choose to provide a string, with
appropriate security checking (an identifier of "../../../../../../etc/shadow"
will obviously not be accepted).

The usage requirements of ExternalStorer will be driven by external users. The
transaction rates, and bandwidth required, will depend on how heavily those
external users want to make use of the BMC. There are no resources consumed by
ExternalStorer when idle, except for the storage already taken by data.

## Proposed Design

The ExternalStorer feature is designed in a generic way, intended to integrate
with various Redfish endpoints, of which
[*LogServices*](https://redfish.dmtf.org/schemas/v1/LogService_v1.xml) is the
first. ExternalStorer has three layers of operation. To avoid confusion with
other Redfish terminology, new terms are introduced here, to be unambiguous. The
three layers are, from outermost to innermost:

*   ExternalStorer Hook: This is an implementation detail, to provide linkage
    between ExternalStorer and the rest of the Redfish service. It will make use
    of the "route" feature within bmcweb.

*   ExternalStorer Instance: Within a hook, this is a container to hold
    externally-added items. Each instance represents a collection of JSON
    objects.

*   ExternalStorer Entry: Within an instance, this is an externally-added item
    of arbitrary content. Each entry is a JSON object.

The topmost layer of ExternalStorer is the hook. This is how ExternalStorer
hooks into the existing Redfish URL hierarchy and JSON contents. The challenge
is that these are strictly defined by Redfish schemas: there are very few
opportunities to insert user-defined content into the framework provided by
these strict schemas. The schema must be followed, so content must be inserted
only as permitted by a schema. In other words, the Redfish object hierarchy
can't just be altered at will, as the schemas limit where changes are allowed to
be made, so care has to be taken as to the placement of the ExternalStorer hook.
The hook consists of a substring of a well-known Redfish URL, as defined in the
relevant schema for it, and, somewhere within the GET output of this URL, a
well-known JSON data structure that is allowed by the schema to be extendable,
such as a JSON array or dictionary that can accept additional items inserted
into it.

These additional items inserted are ExternalStorer instances, which are the
middle layer. An ExternalStorer instance is a container, which can be used to
hold an arbitrary amount of externally-provided data, in the form of
ExternalStorer entries, which are the bottommost layer. An ExternalStorer entry
is simply a JSON dictionary. It contains arbitrary data that an external user
might wish to store, for optional later retrieval. Each ExternalStorer entry
must be added to an already-created ExternalStorer instance.

The backing store of ExternalStorer will be in memory only. Restarting the
*bmcweb* server will not cause loss of data, but rebooting or power cycling
will. Taking advantage of existing BMC architecture, ExternalStorer will use an
existing `tmpfs` filesystem, creating and owning a directory on it, named
`/run/bmcweb/redfish/v1` which will correspond to the top-level `/redfish/v1`
Redfish
[root](https://redfish.dmtf.org/schemas/DSP0266_1.7.0.html#service-root-request).
There will be a directory tree underneath, created as needed. The structure of
this directory tree will mirror the structure of the entire URL hierarchy served
by the Redfish server, however, ExternalStorer will be the only user of this
directory tree. Although a URL superficially resembles a directory path in
appearance, there is an important difference underneath: the "directories" in a
URL can also contain original content of their own, to be returned when queried.
This is not possible within a typical filesystem. So, an additional file will be
created, to hold this content, named `index.json` which will be a reserved
filename. This file will be stored within the directory it applies to. It will
be disallowed to create a new ExternalStorer instance or entry that would cause
a naming conflict with this reserved filename, which should not be a significant
limitation.

## Alternatives Considered

The ExternalStorer hook is designed to be minimally intrusive, typically only a
few lines of code added to the container enumeration during the relevant GET
operation. To avoid a large disruptive change, it was decided to not change the
underlying Redfish Container object implementation used throughout bmcweb.
Another OpenBMC project, *dbus-sensors*, made the intentional decision to not
add generic support for all devices, instead deciding to add them only on an
allowlist basis, as those devices are tested and proven working. The same should
be done for ExternalStorer, selectively adding it to areas of bmcweb where it
would be useful.

The usage of ExternalStorer is designed to be generic. Although *LogServices* is
the first Redfish component targeted by ExternalStorer, the usage is generic,
not really having anything to with LogServices except for the location of the
Redfish endpoint, and it avoids a lot of code duplication to simply have the
ability to plug in ExternalStorer to other areas of the Redfish tree as desired.
However, as mentioned earlier the Redfish schemas must be obeyed, so it cannot
be too generic. This is why a more general protocol for accepting
externally-provided content into a URL hierarchy, such as WebDAV, could not be
used. Having strictly structured data is important for server manageability.

## Impacts

As for API impact, a companion document will be
[developed](https://gerrit.openbmc-project.xyz/c/openbmc/docs/+/52295),
describing the API for use by both Redfish HTTP operations and direct underlying
filesystem operations. Security concerns include exhausting all memory via
denial-of-service attack, so limits on memory consumption must be enforced by
the implementation. As clients will have some control over the identifying names
of ExternalStorer instances and entries added, this is also a security concern,
so identifiers must be validated to not have URL-sensitive characters and
filesystem-sensitive characters, as well as a length limit. As for performance
impact, as it was an intentional decision to not do caching of content from the
filesystem, the performance might not be optimal, but this is believed to be an
acceptable trade-off. As for developer impact, the ExternalStorer hook has
intentionally been designed to be minimal, requiring very little disruption.

## Testing

Unit tests will test the individual classes and functions that comprise the
implementation of the ExternalStorer feature. Functional tests will verify that
the API works as intended, for both the Redfish HTTP operations, and direct
manipulation of the underlying filesystem. As for integration tests, developers
using this API should also include it within their own integration tests as
needed. As for CI and CBTRM, there should be no effect, other than automatically
running these tests as needed, as it is expected that a well-written test will
clean up after itself and have no side effects.
