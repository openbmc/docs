# Error and Event Logging

Author: [Patrick Williams][patrick-email] `<stwcx>`

[patrick-email]: mailto:patrick@stwcx.xyz

Other contributors:

Created: May 16, 2024

## Problem Description

There is currently not a consistent end-to-end error and event reporting design
for the OpenBMC code stack. There are two different implementations, one
primarily using phosphor-logging and one using rsyslog, both of which have gaps
that a complete solution should address. This proposal is intended to be an
end-to-end design handling both errors and tracing events which facilitate
external management of the system in an automated and maintainable manner.

## Background and References

### Redfish LogEntry and Message Registry

In Redfish, the [`LogEntry` schema][LogEntry] is used for a range of items that
could be considered "logs", but one such use within OpenBMC is for an equivalent
of the IPMI "System Event Log (SEL)".

The IPMI SEL is the location that the BMC can collect errors and events,
sometimes coming from other entities, such as the BIOS. Examples of these might
be "DIMM-A0 encountered an uncorrectable ECC error" or "System boot successful".
These SEL records are human readable strings which are typically unique to each
system or manufacturer, and could hypothethically change with a BMC or firmware
update, and are thus difficult to create automated tooling around. Two different
vendors might use different strings to represent a critical temperature
threshold exceeded: ["temperature threshold exceeded"][HPE-Example] and
["Temperature #0x30 Upper Critical going high"][Oracle-Example]. There is also
no mechanism with IPMI to ask the machine "what are all of the SELs you might
create".

In order to solve two aspects of this problem, listing of possible events and
versioning, Redfish has Message Registries. A message registry is versioned
collection of all of the error events that a system could generate and hints as
to how they might be parsed and displayed to a user. An [informative
reference][Registry-Example] from the DMTF gives this example:

```json
{
  "@odata.type": "#MessageRegistry.v1_0_0.MessageRegistry",
  "Id": "Alert.1.0.0",
  "RegistryPrefix": "Alert",
  "RegistryVersion": "1.0.0",
  "Messages": {
    "LanDisconnect": {
      "Description": "A LAN Disconnect on %1 was detected on system %2.",
      "Message": "A LAN Disconnect on %1 was detected on system %2.",
      "Severity": "Warning",
      "NumberOfArgs": 2,
      "Resolution": "None"
    }
  }
}
```

This example defines an event, `Alert.1.0.LanDisconnect`, which can record the
disconnect state of a network device and contains placeholders for the affected
device and system. When this event occurs, there might be a `LogEntry` recorded
containing something like:

```json
{
  "Message": "A LAN Disconnnect on EthernetInterface 1 was detected on system /redfish/v1/Systems/1.",
  "MessageId": "Alert.1.0.LanDisconnect",
  "MessageArgs": ["EthernetInterface 1", "/redfish/v1/Systems/1"]
}
```

The `Message` contains a human readable string which was created by applying the
`MessageArgs` to the placeholders from the `Message` field in the registry.
System management software can rely on the message registry (referenced from the
`MessageId` field in the `LogEntry`) and `MessageArgs` to avoid needing to
perform string processing for reacting to the event.

Within OpenBMC, there is currently a [limited design][existing-design] for this
Redfish feature and it requires inserting specially formed Redfish-specific
logging messages into any application that wants to record these events, tightly
coupling all applications to the Redfish implementation. It has also been
observed that these [strings][app-example], when used, are often out of date
with the [message registry][registry-example] advertised by `bmcweb`. Some
maintainers have rejected adding new Redfish-specific logging messages to their
applications.

[LogEntry]:
  https://github.com/openbmc/bmcweb/blob/master/static/redfish/v1/JsonSchemas/LogEntry/LogEntry.json
[HPE-Example]:
  https://support.hpe.com/hpesc/public/docDisplay?docId=sd00002092en_us&docLocale=en_US&page=GUID-D7147C7F-2016-0901-06CE-000000000422.html
[Oracle-Example]:
  https://docs.oracle.com/cd/E19464-01/820-6850-11/IPMItool.html#50602039_63068
[Registry-Example]:
  https://www.dmtf.org/sites/default/files/Redfish%20School%20-%20Events_0.pdf
[existing-design]:
  https://github.com/openbmc/docs/blob/master/architecture/redfish-logging-in-bmcweb.md
[app-example]:
  https://github.com/openbmc/phosphor-post-code-manager/blob/f2da78deb3a105c7270f74d9d747c77f0feaae2c/src/post_code.cpp#L143
[registry-example]:
  https://github.com/openbmc/bmcweb/blob/4ba5be51e3fcbeed49a6a312b4e6b2f1ea7447ba/redfish-core/include/registries/openbmc.json#L5

### Existing phosphor-logging implementation

The `sdbusplus` bindings have the capability to define new C++ exception types
which can be thrown by a DBus server and turned into an error response to the
client. `phosphor-logging` extended this to also add metadata associated to the
log type. See the following example error definitions and usages.

`sdbusplus` error binding definition (in
`xyz/openbmc_project/Certs.errors.yaml`):

```yaml
- name: InvalidCertificate
  description: Invalid certificate file.
```

`phosphor-logging` metadata definition (in
`xyz/openbmc_project/Certs.metadata.yaml`):

```yaml
- name: InvalidCertificate
  meta:
    - str: "REASON=%s"
      type: string
```

Application code reporting an error:

```cpp
elog<InvalidCertificate>(Reason("Invalid certificate file format"));
// or
report<InvalidCertificate>(Reason("Existing certificate file is corrupted"));
```

In this sample, an error named
`xyz.openbmc_project.Certs.Error.InvalidCertificate` has been defined, which can
be sent between applications as a DBus response. The `InvalidCertificate` is
expected to have additional metadata `REASON` which is a string. The two APIs
`elog` and `report` have slightly different behaviors: `elog` throws an
exception which can either result in an error DBus result or be handled
elsewhere in the application, while `report` sends the event directly to
`phosphor-logging`'s daemon for recording. As a side-effect of both calls, the
metadata is inserted into the `systemd` journal.

When an error is sent to the `phosphor-logging` daemon, it will:

1. Search back through the journal for recorded metadata associated with the
   event (this is a relative slow operation).
2. Create an [`xyz.openbmc_project.Logging.Entry`][Logging-Entry] DBus object
   with the associated data extracted from the journal.
3. Persist a serialized version of the object.

Within `bmcweb` there is support for translating
`xyz.openbmc_project.Logging.Entry` objects advertised by `phosphor-logging`
into Redfish `LogEntries`, but this support does reference a Message Registry.
This makes the events of limited utility for consumption by system management
software, as it cannot know all of the event types and is left to perform
(hand-coded) regular-expressions to extract any information from the `Message`
field of the `LogEntry`. Furthermore, these regular-expressions are likely to
become outdated over time as internal OpenBMC error reporting structure,
metadata, or message strings evolve.

[Logging-Entry]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/9012243e543abdc5851b7e878c17c991b2a2a8b7/yaml/xyz/openbmc_project/Logging/Entry.interface.yaml#L1

### Issues with the Status Quo

- There are two different implementations of error logging, neither of which are
  complete, and do not cover tracing events.

- The `REDFISH_MESSAGE_ID` log approach leads to differences between the Redfish
  Message Registry and the reporting application. It also requires every
  application to be "Redfish aware" which limits decoupling between applications
  and external management interfaces. This also leaves gaps for reporting errors
  in different management interfaces, such as inband IPMI and PLDM.

- The `phosphor-logging` approach does not provide compile-time assurance of
  appropriate metadata collection and requires expensive daemon processing of
  the `systemd` journal on each error report, which limits scalability.

- The `sdbusplus` bindings for error reporting does not currently handle
  lossless transmission of errors between DBus servers and clients.

- Similar applications can result in different Redfish `LogEntry` for the same
  error scenario. This has been observed in sensor threshold exceeded events
  between `dbus-sensors`, `phosphor-hwmon`, `phosphor-virtual-sensor`, and
  `phosphor-health-monitor`. One cause of this is two different error reporting
  approaches and disagreements amongst maintainers as to the preferred approach.

## Requirements

- Applications running on the BMC must be able to report errors and failure
  which are persisted and available for external system management through
  standards such as Redfish.

  - These errors must be structured, versioned, and the complete set of errors
    able to be created by the BMC should be available at built-time of a BMC
    image.
  - The set of errors, able to be created by the BMC, must be able to be
    transformed into relevant data sets, such as Redfish Message Registries.
  - Errors reported by the BMC should contain sufficient information to allow
    service of the system for these failures, either by humans or automation
    (depending on the individual system requirements).

- Applications running on the BMC should be able to report important tracing
  events relevant to system management and/or debug, such as the system
  successfully reaching a running state.

  - All requirements relevant to errors are also applicable to tracing events.
  - The implementation must have a mechanism for vendors to be able to disable
    specific tracing events to conform to their own system design requirements.

- Applications running on the BMC should be able to determine when a previously
  reported error is no longer relevant and mark it as "resolved", while
  maintaining the persistent record for future usages such as debug.

- The BMC should provide a mechanism for managed entities within the server to
  report their own errors and events. Examples of managed entities would be
  firmware, such as the BIOS, and satellite management controllers.

- The implementation on the BMC should scale to a minimum of
  [10,000][error-discussion] error and events without impacting the BMC or
  managed system performance.

- The implementation should provide a mechanism to allow OEM or vendor
  extensions to the error and event definitions (and generated artifacts such as
  the Redfish Message Registry) for usage in closed-source or non-upstreamed
  code. These extensions must be clearly identified, in all interfaces, as
  vendor-specific and not be tied to the OpenBMC project.

- APIs to implement error and event reporting should have good ergonomics. These
  APIs must provide compile-time identification, for applicable programming
  languages, of call sites which do not conform to the BMC error and event
  specifications.

  - The generated error classes and APIs should not require exceptions but
    should also integrate with the `sdbusplus` client and server bindings, which
    do leverage exceptions.

[error-discussion]:
  https://discord.com/channels/775381525260664832/855566794994221117/867794201897992213

## Proposed Design

The proposed design has a few high-level design elements:

- Consolidate the `sdbusplus` and `phosphor-logging` implementation of error
  reporting; expand it to cover tracing events; improve the ergonomics of the
  associated APIs and add compile-time checking of missing metadata.

- Add APIs to `phosphor-logging` to enable daemons to easily look up their own
  previously reported events (for marking as resolved).

- Add to `phosphor-logging` a compile-time mechanism to disable recording of
  specific tracing events for vendor-level customization.

- Generate a Redfish Message Registry for all error and events defined in
  `phosphor-dbus-interfaces`, using binding generators from `sdbusplus`. Enhance
  `bmcweb` implementation of the `Logging.Entry` to `LogEvent` transformation to
  cover the Redfish Message Registry and `phosphor-logging` enhancements;
  Leverage the Redfish `LogEntry.DiagnosticData` field to provide a
  Base64-encoded JSON representation of the entire `Logging.Entry` for
  additional diagnostics [[does this need to be optional?]].

### `sdbusplus`

The `Foo.errors.yaml` content will be combined with the content formerly in the
`Foo.metadata.yaml` files specified by `phosphor-logging` and specified by a new
file type `Foo.events.yaml`. This `Foo.events.yaml` format will cover both the
current `error` and `metadata` information as well as augment with additional
information necessary to generate external facing datasets, such as Redfish
Message Registries. The current `Foo.errors.yaml` and `Foo.metadata.yaml` files
will be deprecated as their usage is replaced by the new format.

The `sdbusplus` library will be enhanced to provide the following:

- JSON serialization and de-serialization of generated exception types with
  their assigned metadata; assignment of the JSON serialization to the `message`
  field of `sd_bus_error_set` calls when errors are returned from DBus server
  calls.

- A facility to register exception types, at library load time, with the
  `sdbusplus` library for automatic conversion back to C++ exception types in
  DBus clients.

The binding generator(s) will be expanded to do the following:

- Generate complete C++ exception types, with compile-time checking of missing
  metadata and JSON serialization, for errors and events. Metadata can be of one
  of the following types:

  - size-type and signed integer
  - floating-point number
  - string
  - DBus object path

- Generate a format that `bmcweb` can use to create and populate a Redfish
  Message Registry, and translate from `phosphor-logging` to Redfish `LogEntry`
  for a set of errors and events

For general users of `sdbusplus` these changes should have no impact, except for
the availability of new generated exception types and that specialized instances
of `sdbusplus::exception::generated_exception` will become available in DBus
clients.

### `phosphor-dbus-interfaces`

Refactoring will be done to migrate existing `Foo.metadata.yaml` and
`Foo.errors.yaml` content to the `Foo.events.yaml` as migration is done by
applications. Minor changes will take place to utilize the new binding
generators from `sdbusplus`. A small library enhancement will be done to
register all generated exception types with `sdbusplus`. Future contributors
will be able to contribute new error and tracing event definitions.

### `phosphor-logging`

> TODO: Should a tracing event be a `Logging.Entry` with severity of
> `Informational` or should they be a new type, such as `Logging.Event` and
> managed separately. The `phosphor-logging` default `meson.options` have
> `error_cap=200` and `error_info_cap=10`. If we increase the total number of
> events allowed to 10K, the majority of them are likely going to be information
> / tracing events.

The `Logging.Entry` interface's `AdditionalData` property should change to
`dict[string, variant[string,int64_t,size_t,object_path]]`.

The `Logging.Create` interface will have a new method added:

```yaml
- name: CreateEntry
  parameters:
    - name: Message
      type: string
    - name: Severity
      type: enum[Logging.Entry.Level]
    - name: AdditionalData
      type: dict[string, variant[string,int64_t,size_t,object_path]]
    - name: Hint
      type: string
      default: ""
  returns:
    - name: Entry
      type: object_path
```

The `Hint` parameter is used for daemons to be able to query for their
previously recorded error. These strings need to be globally unique and are
suggested to be of the format `"<service_name>:<key>"`.

A `Logging.SearchHint` interface will be created, which will recorded at the
same object path as a `Logging.Entry` when the `Hint` parameter was not an empty
string:

```yaml
- property: Hint
  type: string
```

The `Logging.Manager` interface will be added with a single method:

```yaml
- name: FindEntry
  parameters:
    - name: Hint
      type: String
  returns:
    - name: Entry
      type: object_path
  errors:
    - xyz.openbmc_project.Common.ResourceNotFound
```

The `phosphor::logging::commit` API will be enhanced to support the new
`sdbusplus` generated exception types, calling the new
`Logging.Create.CreateEntry` method proposed earlier. This new API will support
`sdbusplus::bus_t` for synchronous DBus operations and both
`sdbusplus::async::context_t` and `sdbusplus::asio::connection` for asynchronous
DBus operations.

> TODO: Is the `phosphor::logging::commit` an acceptable interface to use or
> should we use a shorter name like we did for `lg2`? `lg2::commit`?]]

### `bmcweb`

`bmcweb` already has support for build-time conversion from a Redfish Message
Registry, codified in JSON, to header files it uses to serve the registry; this
will be expanded to support Redfish Message Registries generated by `sdbusplus`.
`bmcweb` will add a Meson option for additional message registries, provided
from bitbake from `phosphor-dbus-interfaces` and vendor-specific event
definitions as a path to a directory of Message Registry JSONs. Support will
also be added for adding `phosphor-dbus-interfaces` as a Meson subproject for
stand-alone testing.

It is desirable for `sdbusplus` to generate a Redfish Message Registry directly,
leveraging the existing scripts for integration with `bmcweb`. As part of this
we would like to support mapping a `Logging.Entry` event to an existing
standardized Redfish event (such as those in the Base registry). The generated
information must contain the `Logging.Entry::Message` identifier, the
`AdditionalData` to `MessageArgs` mapping, and the translation from the
`Message` identifier to the Redfish Message ID (when the Message ID is not from
"this" registry). In order to facilitate this, we will need to add OEM fields to
the Redfish Message Registry JSON, which are only used by the `bmcweb`
processing scripts, to generate the information necessary for this additional
mapping.

The `xyz.openbmc_project.Logging.Entry` to `LoggingEvent` conversion needs to be
enhanced, to utilize these Message Registries, in four ways:

1. A Base64-encoded JSON representation of the `Logging.Entry` will be assigned
   to the `DiagnosticData` property.

2. If the `Logging.Entry::Message` contains an identifier corresponding to a
   Registry entry, the `MessageId` property will be set to the corresponding
   Redfish Message ID.

3. If the `Logging.Entry::Message` contains an identifier corresponding to a
   Registry entry, the `MessageArgs` property will be filled in by obtaining the
   corresponding values from the `AdditionalData` dictionary and the `Message`
   field will be generated from combining these values with the `Message` string
   from the Registry.

4. A mechanism should be implemented to translate DBus `object_path` references
   to Redfish Resource URIs. When an `object_path` cannot be translated,
   `bmcweb` will use a prefix such as `object_path:` in the `MessageArgs` value.

### `phosphor-sel-logger`

The `phosphor-sel-logger` has a meson option `send-to-logger` which toggles
between using `phosphor-logging` or the [`REDFISH_MESSAGE_ID`
mechanism][existing-design]. The `phosphor-logging`-utilizing paths will be
updated to utilize `phosphor-dbus-interfaces` specified errors and events.

### YAML format

Consider an example file in `phosphor-dbus-interfaces` as
`yaml/xyz/openbmc_project/Software/Update.events.yaml` with hypothetical errors
and events:

```yaml
version: 1.3.1

errors:
  - name: UpdateFailure
    severity: critical
    metadata:
      - name: TARGET
        type: string
        primary: true
      - name: ERRNO
        type: int64
      - name: CALLOUT_HARDWARE
        type: object_path
        primary: true
    description: >
      While updating the firmware on a device, the update failed.
    message:
      en: A failure occurred updating {TARGET} on {CALLOUT_HARDWARE}.
    resolution:
      en: Retry update.

  - name: BMCUpdateFailure
    severity: critical
    deprecated: 1.0.0
    description: Failed to update the BMC
    redfish-mapping: OpenBMC.FirmwareUpdateFailed

events:
  - name: UpdateProgress
    metadata:
      - name: TARGET
        type: string
        primary: true
      - name: COMPLETION
        type: double
        primary: true
    description: >
      An update is in progress and has reached a checkpoint.
    message:
      en: Updating of {TARGET} is {COMPLETION}% complete.
```

> TODO: Does `description` need translation support? If so, we should probably
> invert the language and description/message/resolution so we do not have to
> specify `en` so many times.

Each `foo.events.yaml` file would be used to generate both the C++ classes (via
`sdbusplus`) for exception handling and event reporting, as well as a versioned
Redfish Message Registry for the errors and events. The YAML information is as
follows:

- `version` : The version of the file, which will be used as the Redfish Message
  Registry version.
- `errors` / `events`: The list of error and events specified.
  - `name`: An identifier for the event in UpperCamelCase; used as the class and
    Redfish Message ID.
  - `description`: A developer-applicable description of the error reported.
    These form the "description" of the Redfish message.
  - `message`: A set of per-language message strings, which are intended for
    end-users. These form the message itself.
  - `resolution`: A set of per-language message strings, which are intended for
    end-users. These form the "Resolution" in the Registry.
  - `severity`: The `xyz.openbmc_project.Logging.Entry.Level` value for this
    error (only applicable to errors).
  - `redfish-mapping`: Used when a `sdbusplus` event should map to a specific
    Redfish Message rather than a generated one. This is useful when an internal
    error has an analog in a standardized registry.
  - `deprecated`: Indicates that the event is now deprecated and should not be
    created by any OpenBMC software, but is required to still exist for
    generation in the Redfish Message Registry. The version listed here should
    be the first version where the error is no longer used.
  - `metadata`: A list of metadata fields that must be populated for the event.
    - `name`: The name of the metadata field, which must be `UPPER_SNAKE_CASE`
      for backwards compatibility with existing `phosphor-logging`
      implementation.
    - `type`: The type of the metadata field; one of `string`, `size`, `int64`,
      `uint64`, `double` or `object_path`.
    - `primary`: set to true when the metadata field is expected to be part of
      the Redfish `MessageArgs` (and not only in the extended `DiagnosticData`).

### Generated `sdbusplus` classes

The above example YAML would generate C++ classes similar to:

```cpp
namespace sdbusplus::errors::xyz::openbmc_project::software::update
{

class UpdateFailure
{

    template <typename... Args>
    UpdateFailure(Args&&... args);
};

}

namespace sdbusplus::events::xyz::openbmc_project::software::update
{

class UpdateProgress
{
    template <typename... Args>
    UpdateProgress(Args&&... args);
};

}
```

The constructors here are variadic templates because the generated constructor
implementation will provide compile-time assurance that all of the metadata
fields have been populated (in any order). To raise an `UpdateFailure` a
developers might do something like:

```cpp
throw UpdateFailure("TARGET", "BMC Flash A", "ERRNO", rc, "CALLOUT_HARDWARE", bmc_object_path);
```

If one of the fields, such as `ERRNO` were omitted, a compile failure will be
raised indicating the first missing field.

### Versioning Policy

Assume the version follows semantic versioning `MAJOR.MINOR.PATCH` convention.

- Adjusting a description or message should result in a `PATCH` increment.
- Adding a new error or event, or adding metadata to an existing error or event,
  should result in a `MINOR` increment.
- Deprecating an error or event should result in a `MAJOR` increment.

### Generated Redfish Message Registry

[DSP0266][dsp0266], the Redfish specification, gives requirements for Redfish
Message Registries and dictates guidelines for identifiers.

The hypothetical events defined above would create a message registry similar
to:

```json
{
  "Id": "OpenBMC_Base_Xyz_OpenbmcProject_Software_Update.1.3.1",
  "Language": "en",
  "Messages": {
    "UpdateFailure": {
      "Description": "While updating the firmware on a device, the update failed.",
      "Message": "A failure occurred updating %1 on %2.",
      "Resolution": "Retry update."
      "NumberOfArgs": 2,
      "ParamTypes": ["string", "string"],
      "Severity": "Critical",
    },
    "UpdateProgress" : {
      "Description": "An update is in progress and has reached a checkpoint."
      "Message": "Updating of %1 is %2\% complete.",
      "Resolution": "None",
      "NumberOfArgs": 2,
      "ParamTypes": ["string", "number"],
      "Severity": "OK",
    }
  }
}
```

The prefix `OpenBMC_Base` shall be exclusively reserved for use by events from
`phosphor-logging`. Events defined in other repositories will be expected to use
some other prefix. Vendor-defined repositories should use a vendor-owned prefix
as directed by [DSP0266][dsp0266].

[dsp0266]:
  https://www.dmtf.org/sites/default/files/standards/documents/DSP0266_1.20.0.pdf

### Vendor implications

As specified above, vendors must use their own identifiers in order to conform
with the Redfish specification. The `sdbusplus` (and `phosphor-logging` and
`bmcweb`) implementation(s) will enable vendors to create their own events for
downstream code and Registries for integration with Redfish. Vendors are
responsible for ensuring their own versioning and identifiers conform to the
expectations in the [Redfish specification][dsp0266].

One potential bad behavior on the part of vendors would be forking and modifying
`phosphor-dbus-interfaces` defined events. Vendors must not add their own events
to `phosphor-dbus-interfaces` in downstream implementations because it would
lead to their implementation advertising support for a message in an
OpenBMC-owned Registry which is not the case. Similarly, if a vendor were to
_backport_ upstream changes into their fork, they would need to ensure that the
`foo.events.yaml` file for that version matches identically with the upstream
implementation.

## Alternatives Considered

Many alternatives have been explored and referenced through earlier work. Within
this proposal there are many minor-alternatives that have been assessed.

### Exception inheritance

The original `phosphor-logging` error descriptions allowed inheritance between
two errors and this is not supported by the proposal for two reasons:

- This introduces complexity in the Redfish Message Registry versioning because
  a change in one file should induce version changes in all dependent files.

- It makes it difficult for a develop to clearly identify all of the fields they
  are expected to populate without traversing multiple files.

### sdbusplus Exception APIs

There are a few possible syntaxes I came up with for constructing the generated
exception types. It is important that these have good ergonomics, are easy to
understand, and can provide compile-time awareness of missing metadata fields.

```cpp
    using Example = sdbusplus::error::xyz::openbmc_project::Example;

    // 1)
    throw Example().fru("Motherboard").value(42);

    // 2)
    throw Example(Example::fru_{}, "Motherboard", Example::value_{}, 42);

    // 3)
    throw Example("FRU", "Motherboard", "VALUE", 42);

    // 4)
    throw Example([](auto e) { return e.fru("Motherboard").value(42); });

    // 5)
    throw Example({.fru = "Motherboard", .value = 42});
```

1. This would be my preference for ergonomics and clarity, as it would allow
   LSP-enabled editors to give completions for the metadata fields but
   unfortunately there is no mechanism in C++ to define a type which can be
   constructed but not thrown, which means we cannot get compile-time checking
   of all metadata fields.

2. This syntax uses tag-dispatch to enables compile-time checking of all
   metadata fields and potential LSP-completion of the tag-types, but is more
   verbose than option 3.

3. This syntax is less verbose than (2) and follows conventions already used in
   `phosphor-logging`'s `lg2` API, but does not allow LSP-completion of the
   metadata tags.

4. This syntax is similar to option (1) but uses an indirection of a lambda to
   enable compile-time checking that all metadata fields have been populated by
   the lambda. The LSP-completion is likely not as strong as option (1), due to
   the use of `auto`, and the lambda necessity will likely be a hang-up for
   unfamiliar developers.

5. This syntax is has similar characteristics as option (1) but similarly does
   not provide compile-time confirmation that all fields have been populated.

The proposal therefore suggests option (3) is most suitable.

### Redfish Translation Support

The proposed YAML format allows future addition of translation but it is not
enabled at this time. Future development could enable the Redfish Message
Registry to be generated in multiple languages if the `message:language` exists
for those languages.

### Redfish Registry Versioning

The Redfish Message Registries are required to be versioned and has 3 digit
fields (ie. `XX.YY.ZZ`), but only the first 2 are suppose to be used in the
Message ID. Rather than using the manually specified version we could take a few
other approaches:

- Use a date code (ex. `2024.17.x`) representing the ISO 8601 week when the
  registry was built.

  - This does not cover vendors that may choose to branch for stabilization
    purposes, so we can end up with two machines having the same
    OpenBMC-versioned message registry with different content.

- Use the most recent `openbmc/openbmc` tag as the version.

  - This does not cover vendors that build off HEAD and may deploy multiple
    images between two OpenBMC releases.

- Generate the version based on the git-history.

  - This requires `phosphor-dbus-interfaces` to be built from a git repository,
    which may not always be true for Yocto source mirrors, and requires
    non-trivial processing that continues to scale over time.

### Existing OpenBMC Redfish Registry

There are currently 191 messages defined in the existing Redfish Message
Registry at version `OpenBMC.0.4.0`. Of those, not a single one in the codebase
is emitted with the correct version. 96 of those are only emitted by
Intel-specific code that is not pulled into any upstreamed machine, 39 are
emitted by potentially common code, and 56 are not even referenced in the
codebase outside of the bmcweb registry. Of the 39 common messages half of them
have an equivalent in one of the standard registries that should be leveraged
and many of the others do not have attributes that would facilitate a multi-host
configuration, so the registry at a minimum needs to be updated. None of the
current implementation has the capability to handle Redfish Resource URIs.

The proposal therefore is to deprecate the existing registry and replace it the
new generated registries. For repositories that currently emit events in the
existing format, we can maintain those call-sites for a time period of 1-2
years.

If this aspect of the proposal is rejected, the YAML format allows mapping from
`phosphor-dbus-interfaces` defined events to the current `OpenBMC.0.4.0`
registry `MessageIds`.

Potentially common:

- phosphor-post-code-manager
  - BIOSPOSTCode (unique)
- dbus-sensors
  - ChassisIntrusionDetected (unique)
  - ChassisIntrusionReset (unique)
  - FanInserted
  - FanRedundancyLost (unique)
  - FanRedudancyRegained (unique)
  - FanRemoved
  - LanLost
  - LanRegained
  - PowerSupplyConfigurationError (unique)
  - PowerSupplyConfigurationErrorRecovered (unique)
  - PowerSupplyFailed
  - PowerSupplyFailurePredicted (unique)
  - PowerSupplyFanFailed
  - PowerSupplyFanRecovered
  - PowerSupplyPowerLost
  - PowerSupplyPowerRestored
  - PowerSupplyPredictiedFailureRecovered (unique)
  - PowerSupplyRecovered
- phosphor-sel-logger
  - IPMIWatchdog (unique)
  - `SensorThreshold*` : 8 different events
- phosphor-net-ipmid
  - InvalidLoginAttempted (unique)
- entity-manager
  - InventoryAdded (unique)
  - InventoryRemoved (unique)
- estoraged
  - ServiceStarted
- x86-power-control
  - NMIButtonPressed (unique)
  - NMIDiagnosticInterrupt (unique)
  - PowerButtonPressed (unique)
  - PowerRestorePolicyApplied (unique)
  - PowerSupplyPowerGoodFailed (unique)
  - ResetButtonPressed (unique)
  - SystemPowerGoodFailed (unique)

Intel-only implementations:

- intel-ipmi-oem
  - ADDDCCorrectable
  - BIOSPostERROR
  - BIOSRecoveryComplete
  - BIOSRecoveryStart
  - FirmwareUpdateCompleted
  - IntelUPILinkWidthReducedToHalf
  - IntelUPILinkWidthReducedToQuarter
  - LegacyPCIPERR
  - LegacyPCISERR
  - `ME*` : 29 different events
  - `Memory*` : 9 different events
  - MirroringRedundancyDegraded
  - MirroringRedundancyFull
  - `PCIeCorrectable*`, `PCIeFatal` : 29 different events
  - SELEntryAdded
  - SparingRedundancyDegraded
- pfr-manager
  - BIOSFirmwareRecoveryReason
  - BIOSFirmwarePanicReason
  - BMCFirmwarePanicReason
  - BMCFirmwareRecoveryReason
  - BMCFirmwareResiliencyError
  - CPLDFirmwarePanicReason
  - CPLDFirmwareResilencyError
  - FirmwareResiliencyError
- host-error-monitor
  - CPUError
  - CPUMismatch
  - CPUThermalTrip
  - ComponentOverTemperature
  - SsbThermalTrip
  - VoltageRegulatorOverheated
- s2600wf-misc
  - DriveError
  - InventoryAdded

## Impacts

- New APIs are defined for error and event logging. This will deprecate existing
  `phosphor-logging` APIs, with a time to migrate, for error reporting.

- The design should improve performance by eliminating the regular parsing of
  the `systemd` journal. The design may decrease performance by allowing the
  number of error and event logs to be dramatically increased, which have an
  impact to file system utilization and potential for DBus impacts some services
  such as `ObjectMapper`.

- Backwards compatibility and documentation should be improved by the automatic
  generation of the Redfish Message Registry corresponding to all error and
  event reports.

### Organizational

- **Does this repository require a new repository?**
  - No
- **Who will be the initial maintainer(s) of this repository?**
  - N/A
- **Which repositories are expected to be modified to execute this design?**
  - `sdbusplus`
  - `phosphor-dbus-interfaces`
  - `phosphor-logging`
  - `bmcweb`
  - Any repository creating an error or event.

## Testing

- Unit tests will be written in `sdbusplus` and `phosphor-logging` for the error
  and event generation, creation APIs, and to provide coverage on any changes to
  the `Logging.Entry` object management.

- Unit tests will be written for `bmcweb` for basic `Logging.Entry`
  transformation and Message Registry generation.

- Integration tests should be leveraged (and enhanced as necessary) from
  `openbmc-test-automation` to cover the end-to-end error creation and Redfish
  reporting.
