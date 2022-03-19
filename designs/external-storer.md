# ExternalStorer in bmcweb

Author: Josh Lehan[^1]

Created: March 3, 2022

## Introduction

The OpenBMC Redfish server, bmcweb[^2], can already serve a variety of
internally-generated information, but currently, there is no way to serve
information provided by an external source.

This document proposes a new *ExternalStorer* feature, to be added to bmcweb, to
store externally-provided data for later retrieval. This allows the Redfish tree
to serve as a convenient stash-point for a variety of user-provided data, while
still remaining compliant with Redfish schemas.

This storage is not intended to be permanent, as the BMC has little storage
space. However, the lack of permanent storage should not be a limitation, as the
intention is to store data that is ephemeral in nature, such as logging
messages, error notifications, performance counters, status updates, and so on.

The ExternalStorer feature is designed in a generic way, intended to be plugged
into various Redfish endpoints, of which *LogServices*[^3] is the first.

The Redfish tree is governed by schemas, so the entire tree can not be
arbitrarily modified. However, subsets of the tree are allowed to be extended by
the implementation, and these are opportunities where ExternalStorer should be
inserted, as needed.

The rationale for this feature is similar to the *ExternalSensor*[^4] feature of
*dbus-sensors*, hence the intentionally similar name.

## Design

As Redfish is a specialization of a Web server, it is assumed that the reader is
already familiar with HTTP, JSON, GET and POST operations, and other Web
concepts and standards.

ExternalStorer has three layers of operation. To avoid confusion with other
Redfish terminology, new terms are introduced here, to be unambiguous.

The three layers are, from outermost to innermost:

*   ExternalStorer Hook: This is an implementation detail, to provide linkage
    between ExternalStorer and the rest of the Redfish service.

*   ExternalStorer Instance: Within a hook, this is a container to hold
    externally-added items.

*   ExternalStorer Entry: Within an instance, this is an externally-added item
    of arbitrary content.

Each will be discussed in turn, with before-and-after examples to show the
effect of creating them.

### Hook

The topmost layer of ExternalStorer is the hook. This is how ExternalStorer
hooks into the existing Redfish URL hierarchy and JSON contents. The challenge
is that these are strictly defined by Redfish schemas: there are very few
opportunities to insert user-defined content into the framework provided by
these strict schemas. The schema must be followed, so content can only be
inserted where allowed.

The hook consists of a well-known Redfish URL, as defined in a schema somewhere,
and, somewhere within the GET output of this URL, a well-known JSON data
structure that is allowed by schema to be extendable, such as a JSON array or
dictionary that can accept additional items inserted into it.

As an example, consider the following URL to be a valid Redfish URL, compliant
with the various Redfish schemas (ignoring the example.com server which is
obviously an example).

```
https://example.com/redfish/v1/Systems/system/LogServices
```

It is assumed the reader is familiar with commands such as `curl` or `wget` to
work with this URL. Here is a command line, performing a GET operation of an
ExternalStorer hook (the password on the command line is obviously an example):

```
% curl -u root:Passw0rd -X GET https://example.com/redfish/v1/Systems/system/LogServices
{
  "@odata.id": "/redfish/v1/Systems/system/LogServices",
  "@odata.type": "#LogServiceCollection.LogServiceCollection",
  "Description": "Collection of LogServices for this Computer System",
  "Members": [
    {
      "@odata.id": "/redfish/v1/Systems/system/LogServices/EventLog"
    },
    {
      "@odata.id": "/redfish/v1/Systems/system/LogServices/HostLogger"
    }
  ],
  "Members@odata.count": 2,
  "Name": "System Log Services Collection"
}
```

The hook, in this case, is within the implementation of the `Members` element,
which is a JSON array that is allowed to be extensible. New ExternalStorer
instances, underneath this hook, will be represented as array elements here.

Each array element is a JSON dictionary containing one field, `@odata.id`, with
the location of the ExternalStorer instance. Note that `EventLog` and
`HostLogger` are not ExternalStorer instances, but appear similarly. This is by
design: an ExternalStorer instance should look and feel like a native part of
the Redfish implementation.

Clients will not have to create their own ExternalStorer hooks. An
ExternalStorer hook is an implementation detail, established at compile time.
Instead, clients create ExternalStorer instances, underneath the desired hooks.

### Instance

An ExternalStorer instance is a container, which can be used to hold an
arbitrary amount of externally-provided data, in the form of ExternalStorer
entries.

To create an instance, issue a POST operation[^5], targeting the URL with the
desired ExternalStorer hook.

The content of the POST operation must be a JSON dictionary. Within that
dictionary are some special-case fields:

*   `Id`: The unique identifier of the ExternalStorer instance to be created.
    The identifier will form a suffix to be appended to the URL. This field is
    optional: if omitted, the Redfish server will generate a random string,
    typically a UUID.

*   `@odata.id`: This field must not be included within the POST request. It
    will be auto-populated by the Redfish server.

*   `Entries`: This field is optional. If included, it must be a JSON
    dictionary. The name of this field is a keyword, a special case, as defined
    within the `LogServices` schema.

The `Id` field must be unique. If it already exists within this Redfish hook,
the creation attempt will fail. However, each ExternalStorer hook has its own
unique namespace, so it is perfectly OK to create instances with the same `Id`
value, as long as they are in different hooks.

The `Entries` field corresponds to the `Entries` keyword within the
`LogServices` schema. Other schemas may vary, in their choice of keyword. Some
schemas might not require a keyword at all.

The `Entries` JSON dictionary may be populated with arbitrary content, with one
exception: do not include the `@odata.id` field. It also will be auto-populated
by the Redfish server.

Each instance contains two layers of URL that can be retrieved with the GET
operation:

*   The outer layer (omitting the `Entries` keyword): Customize this by adding
    additional fields, as desired, to the JSON dictionary pushed to the Redfish
    server during the POST request.

*   The inner layer (suffixing the URL with the `Entries` keyword): Customize
    this by adding a special field, a JSON dictionary named `Entries`, then by
    adding additional fields within that, as desired.

Both of these layers can be customized, by adding additional fields. These will
be included in the GET output, when this instance is later retrieved. By
appropriate customization, a variety of Redfish data can be stored. Recommended
additional fields include `Name`, `Description`, and so on[^6].

This will become clear with an example:

```
% curl -u root:Passw0rd -X POST https://example.com/redfish/v1/Systems/system/LogServices -d '
{
  "Id": "ExampleAlerts",
  "Name": "Example Alerts",
  "Description": "Holds externally-provided alert notifications",
  "Entries": {
    "Id": "InnerLayer",
    "Name": "Example Alerts Inner Layer",
    "Description": "Content can be customized for each of the two GET layers"
  }
}'
```

The reply, with HTTP headers included (additional content may appear, edited for
brevity):

```
HTTP/1.1 201 Created
Location: /redfish/v1/Systems/system/LogServices/ExampleAlerts

{
  "Location": "/redfish/v1/Systems/system/LogServices/ExampleAlerts"
}
```

The `Location:` field, in the HTTP reply header, will contain the location of
this newly-created ExternalStorer instance. The instance is now ready for use.

As a convenience to the user, the reply content (which is a JSON dictionary)
will also contain the field `Location`, with the same information. The purpose
of this redundancy is to make this easier for external services to use, which
might not have the ability to view headers.

Performing another GET operation on the ExternalStorer hook reveals the effect
of creating this instance:

```
% curl -u root:Passw0rd -X GET https://example.com/redfish/v1/Systems/system/LogServices
{
  "@odata.id": "/redfish/v1/Systems/system/LogServices",
  "@odata.type": "#LogServiceCollection.LogServiceCollection",
  "Description": "Collection of LogServices for this Computer System",
  "Members": [
    {
      "@odata.id": "/redfish/v1/Systems/system/LogServices/EventLog"
    },
    {
      "@odata.id": "/redfish/v1/Systems/system/LogServices/HostLogger"
    },
    {
      "@odata.id": "/redfish/v1/Systems/system/LogServices/ExampleAlerts"
    }
  ],
  "Members@odata.count": 3,
  "Name": "System Log Services Collection"
}
```

The addition to the `Members` array is visible here, and the
`Members@odata.count` number has been incremented. The display order of the
array is arbitrary, as there is no requirement for sorting.

Drilling down to take a look at the newly-created instance:

```
% curl -u root:Passw0rd -X GET https://example.com/redfish/v1/Systems/system/LogServices/ExampleAlerts
{
  "@odata.id": "/redfish/v1/Systems/system/LogServices/ExampleAlerts",
  "Description": "Holds externally-provided alert notifications",
  "Entries": {
    "@odata.id": "/redfish/v1/Systems/system/LogServices/ExampleAlerts/Entries"
  },
  "Id": "ExampleAlerts",
  "Name": "Example Alerts"
}
```

This corresponds to the outer layer, as given during the POST operation. The
customized fields `Name` and `Description`, provided by the external user during
the POST operation, are visible here. The order of these fields is also
arbitrary, with no requirement for sorting.

The automatically-generated fields `Id` and `@odata.id` are present. As required
by the `LogServices` schema, the special `Entries` field contains a link to
another URL.

Drilling down further, following that link:

```
% curl -u root:Passw0rd -X GET https://example.com/redfish/v1/Systems/system/LogServices/ExampleAlerts/Entries
{
  "@odata.id": "/redfish/v1/Systems/system/LogServices/ExampleAlerts/Entries",
  "Description": "Content can be customized for each of the two GET layers",
  "Id": "Entries",
  "Members": [],
  "Members@odata.count": 0,
  "Name": "Example Alerts Inner Layer"
}
```

This corresponds to the inner layer, as given during the POST operation. The
automatically-generated fields `Id` and `@odata.id` are here. The customized
fields `Name` and `Description`, provided by the external user during the POST
operation, have different content. This illustrates the difference between the
outer layer and the inner layer.

There are no ExternalStorer entries yet, so the `Members` array is empty, and
`Members@odata.count` is zero. These special fields will be automatically
updated by the Redfish server, as entries are added.

### Entry

An ExternalStorer entry is simply a JSON dictionary. It contains arbitrary data
that an external user might wish to store, for optional later retrieval.

Each ExternalStorer entry must be added to an already-created ExternalStorer
instance. To create an ExternalStorer entry, POST it to the inner layer URL of
the targeted instance.

Continuing with this example, to add an entry to the `ExampleAlerts` instance:

```
% curl -u root:Passw0rd -X POST https://example.com/redfish/v1/Systems/system/LogServices/ExampleAlerts/Entries -d '
{
  "Name": "On Fire",
  "Description": "The computer is on fire!"
}'
```

Similar to the POST operation which created the instance, there are also some
special fields within this POST operation to create the entry:

*   `Id`: The unique identifier of the ExternalStorer entry to be created. The
    identifier will form a suffix to be appended to the URL. This field is
    optional: if omitted, the Redfish server will generate a random string,
    typically a UUID.

*   `@odata.id`: This field must not be included within the POST request. It
    will be auto-populated by the Redfish server.

Similar to each ExternalStorer hook, each ExternalStorer instance has its own
unique namespace. It is perfectly OK to create entries with the same `Id` value,
as long as they are in different instances. In this example, the `Id` field was
omitted, so the Redfish server will choose a random string, a UUID, to identify
this newly-created entry. This random string will always be unique.

The reply (edited for brevity):

```
HTTP/1.1 201 Created
Location: /redfish/v1/Systems/system/LogServices/ExampleAlerts/Entries/49ae5069-3375-45b9-adfb-bc16c40ce0a6

{
  "Location": "/redfish/v1/Systems/system/LogServices/ExampleAlerts/Entries/49ae5069-3375-45b9-adfb-bc16c40ce0a6"
}
```

The entry has been successfully created. The randomly-generated identifier is
revealed to the client, so they are aware of the location of their newly-created
entry.

Performing another GET operation on the instance will now show the existence of
this entry:

```
% curl -u root:Passw0rd -X GET https://example.com/redfish/v1/Systems/system/LogServices/ExampleAlerts/Entries
{
  "@odata.id": "/redfish/v1/Systems/system/LogServices/ExampleAlerts/Entries",
  "Description": "Content can be customized for each of the two GET layers",
  "Id": "Entries",
  "Members": [
    {
      "@odata.id": "/redfish/v1/Systems/system/LogServices/ExampleAlerts/Entries/49ae5069-3375-45b9-adfb-bc16c40ce0a6"
    }
  ],
  "Members@odata.count": 1,
  "Name": "Example Alerts Inner Layer"
}
```

The created entry now appears within the `Members` array, and the
`Members@odata.count` field has been incremented.

Drilling down, to retrieve that entry:

```
% curl -u root:Passw0rd -X GET /redfish/v1/Systems/system/LogServices/ExampleAlerts/Entries/49ae5069-3375-45b9-adfb-bc16c40ce0a6
{
  "@odata.id": "/redfish/v1/Systems/system/LogServices/ExampleAlerts/Entries/49ae5069-3375-45b9-adfb-bc16c40ce0a6",
  "Description": "The computer is on fire!",
  "Id": "49ae5069-3375-45b9-adfb-bc16c40ce0a6",
  "Name": "On Fire"
}
```

With the exception of the automatically-added `Id` and `@odata.id` fields, the
content of the entry is identical to what was originally posted.

## Intended Usage

The above examples can be refined, to produce a recommended workflow for an
external service. Follow these steps, to efficiently read or write data.

### Reading Data

To read data, an external user should issue GET requests to walk the tree,
descending from the desired instance, to collect the desired data:

*   Decide on an ExternalStorer hook to use. As of now, the only hook is
    LogServices, available at this location:
    `/redfish/v1/Systems/system/LogServices`

*   Issue a GET request to discover all available ExternalStorer instances, to
    that hook. Inspect the contents of the resulting `Members` array. Decide on
    the instance of interest. Use it to form a new URL, following the
    recommended link in its `@odata.id` field. The example from above:
    `/redfish/v1/Systems/system/LogServices/ExampleAlerts`

*   Issue a GET request, to that URL, to retrieve the desired ExternalStorer
    instance. The `LogServices` schema requires the keyword `Entries`, so the
    resulting `Entries` field (a JSON dictionary) will contain an `@odata.id`
    field with the appropriate URL to follow. The example from above:
    `/redfish/v1/Systems/system/LogServices/ExampleAlerts/Entries`

*   Issue a GET request, to that URL, to discover all available ExternalStorer
    entries, under that instance. Inspect the contents of the resulting
    `Members` array. Decide on the entry of interest. The example from above:
    `/redfish/v1/Systems/system/LogServices/ExampleAlerts/Entries/49ae5069-3375-45b9-adfb-bc16c40ce0a6`

*   Issue a GET request, to that URL, to retrieve the contents of that desired
    entry.

See the examples above, for details of these GET requests.

### Writing Data

To write data, an external user should issue two POST requests, first, to create
the instance, and second, to create an entry under that instance:

*   Decide on an ExternalStorer hook to use. As of now, the only hook is
    LogServices, available at this location:
    `/redfish/v1/Systems/system/LogServices`

*   Issue a POST request to create an ExternalStorer instance, under that hook.
    Note the resulting location, as provided by the Redfish server, in the POST
    reply. The example from above: POST to
    `/redfish/v1/Systems/system/LogServices` creating
    `/redfish/v1/Systems/system/LogServices/ExampleAlerts` as the resulting
    location.

*   Issue a POST request to create an ExternalStorer entry, under that instance.
    As mentioned earlier, follow the schema, appending the `Entries` keyword.
    The example from above: POST to
    `/redfish/v1/Systems/system/LogServices/ExampleAlerts/Entries` creating
    `/redfish/v1/Systems/system/LogServices/ExampleAlerts/Entries/49ae5069-3375-45b9-adfb-bc16c40ce0a6`
    as the resulting location.

See the earlier examples, for details of these POST requests.

Arbitrarily many instances can be created (limited only by available memory).

Arbitrarily many entries can be added to an instance (limited only by available
memory).

## Backing Storage

The backing store of `ExternalStorer` will be in memory only. Restarting the
`bmcweb` server will not cause loss of data, but rebooting or power cycling
will.

In the current implementation, the `tmpfs` filesystem will be used.

The directory `/tmp/bmcweb` will be used as backing storage for ExternalStorer.
This corresponds to the top-level `/redfish/v1` location[^7]. There will be a
directory tree underneath. The structure of this directory tree will mirror the
structure of the URL hierarchy served by ExternalStorer.

Although a URL resembles a directory path, there is an important difference: the
"directories" in a URL can return HTTP content of their own, when queried. This
is not possible within a typical filesystem. So, an additional file will be
created, to hold this content, with a special reserved filename: `index.json`

The `index.json` file will be stored within the directory it applies to. This
implies that an external user will not be allowed to create a new ExternalStorer
instance or entry with this special reserved filename.

As with the HTTP operations earlier, examples will make this clear:

```
% ls /tmp/bmcweb
Systems

% ls /tmp/bmcweb/Systems
system

% ls /tmp/bmcweb/Systems/system
LogServices

% ls /tmp/bmcweb/Systems/system/LogServices
ExampleAlerts

% ls /tmp/bmcweb/Systems/system/LogServices/ExampleAlerts
Entries
index.json

% ls /tmp/bmcweb/Systems/system/LogServices/ExampleAlerts/Entries
49ae5069-3375-45b9-adfb-bc16c40ce0a6
index.json
```

Viewing the content of the files:

```
% cat /tmp/bmcweb/Systems/system/LogServices/ExampleAlerts/index.json
{"Description":"Holds externally-provided alert notifications","Name":"Example Alerts"}

% cat /tmp/bmcweb/Systems/system/LogServices/ExampleAlerts/Entries/index.json
{"Description":"Content can be customized for each of the two GET layers","Name":"Example Alerts Inner Layer"}

% cat /tmp/bmcweb/Systems/system/LogServices/ExampleAlerts/Entries/49ae5069-3375-45b9-adfb-bc16c40ce0a6
{"Description":"The computer is on fire!","Name":"On Fire"}
```

The files are stored in a compact form, eliminating whitespace. The fields `Id`
and `@odata.id` need not be stored, as they are already implied by the filenames
of the files.

The current implementation of `ExternalStorer` intentionally does no caching.
Each query will be pushed through, or pulled from, the filesystem. This avoids
problems with the contents of the filesystem getting out of sync with what is in
the memory of the Redfish server.

Although the lack of caching will result in a performance reduction, this also
allows a locally-running service to freely manipulate the contents of the
backing storage, avoiding the overhead of having to do HTTP operations.

## Future Direction

The above describes the current implementation of ExternalStorer as of this
writing, but to be a truly useful and complete tool, more features are needed.

### Additional Operations

PUT, PATCH, and DELETE operations will be added.

The semantics of DELETE are obvious, but the semantics of PUT and PATCH are
still unsettled.

PUT should target an existing URL, in contrast to POST, which creates a new URL.
The content of the PUT request will completely overwrite the content already
existing at that URL.

PATCH is similar to PUT, but does a more selective replacement of the content.
The JSON dictionary will be merged, field by field. In the case of nested JSON
dictionaries, this will continue down, iterating as needed. If a field already
exists at the URL but not in the replacement content, that field will be left
unchanged. This is in contrast to PUT, in which that field would have been
removed.

### Storage Limits

To avoid a trivial denial of service attack, storage limits will be established.

Each ExternalStorer hook will have a limit on the number of instances that can
be created underneath it.

Each instance will have a limit on the number of entries that can be created
underneath that instance. Each instance will also have a limit on the total
bytes of storage taken by all of those entries.

These numerical limits will be established in the ExternalStorer hook, at
compilation time. They will not be tunable at runtime, unless a use case is
found for this, as to do so would introduce considerable complexity.

Unlike traditional storage systems, an ExternalStorer instance will not disallow
new content from being stored, if these limits would be exceeded. Instead, old
content will be dropped, as needed. The rationale for this fits into the
intended use of ExternalStorer for ephemeral content.

### Sliding Window

With the automatic dropping of old content as storage limits are reached, it
becomes practical to have a sliding window for retrieving desired content. This
is also known as paging, or a rolling log.

A future ExternalStorer implementation will have the option of using an integer
number instead of a string, for the `Id` field. This will allow ID numbers to be
easily sorted and kept in sequence. Using integer numbers will also make it
appropriate for use with Redfish URL query parameters, such as `$filter`[^8].

The intent is to allow clients to cleanly collect data in sequence, without
dropping or duplicating data. Note that the existing `$top` and `$skip`
operators are insufficient for this on their own: in the case of an
actively-used system, in which old data is rapidly being dropped, the identity
of the entry to be considered the earliest is rapidly changing. So, it is not
possible to simply begin counting from the earliest entry, as would typically be
done in a more stable system, as the count would not be stable. Output would be
skipped, or doubled-up. A recognizable and stable sequence number is needed, for
client and server to agree on a common reference point.

## Footnotes

[^1]: https://gerrit.openbmc-project.xyz/q/owner:krellan%2540google.com
[^2]: https://github.com/openbmc/bmcweb
[^3]: https://redfish.dmtf.org/schemas/v1/LogService_v1.xml
[^4]: https://github.com/openbmc/docs/blob/master/designs/external-sensor.md
[^5]: https://redfish.dmtf.org/schemas/DSP0266_1.7.0.html#post-create-a-id-post-create-a-
[^6]: https://redfish.dmtf.org/schemas/DSP0266_1.7.0.html#resources
[^7]: https://redfish.dmtf.org/schemas/DSP0266_1.7.0.html#service-root-request
[^8]: https://redfish.dmtf.org/schemas/DSP0266_1.7.0.html#query-parameters
