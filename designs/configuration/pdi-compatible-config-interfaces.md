# PDI compatible configuration interfaces

Author: Alexander Hansen `alexander.hansen@9elements.com`

Other contributors: Various contributors, particularly EM maintainers, have
helped to consider tradeoffs, with significant time put into discussions. It
does not imply they endorse or support this design. See the
[discord channel](https://discordapp.com/channels/775381525260664832/1379890287614099638),
i am not sure if they would like to be named here. Let me know.

Created: September 1, 2025

## Problem Description

Entity-Manager (EM) is responsible for exposing system configuration discovered
from JSON files onto D-Bus. Historically, EM represented nested configuration
data in ways that are **not compatible** with phosphor-dbus-interfaces (PDI)
generated bindings and DBus design intent:

- **Interface explosion**: Arrays of objects generated interfaces lik
  `xyz.openbmc_project.Configuration.ADC.Thresholds0` `...Thresholds1`, etc... →
  unbounded interface names, impossible to model in PDI YAML.
- **Dropped properties**: Fields beyond a nesting depth were silently lost.
- **Type names embedded in interfaces**: e.g.
  `xyz.openbmc_project.Configuration.SPIFlash.FirmwareInfo` ,
  `xyz.openbmc_project.Configuration.${Type}.Threshold0` → no reuse across
  contexts
- **Breaks DBus design intent**: Typed information like a `Threshold` should map
  to a single DBus interface.

This prevented clients (in particular, those using sdbusplus with PDI-generated
bindings) from consuming EM configuration in a type-safe and stable way.

## Background and References

[DBus Specification](https://dbus.freedesktop.org/doc/dbus-specification.html)
[Lengthy discussions in discord channel](https://discordapp.com/channels/775381525260664832/1379890287614099638)
[varlink](https://varlink.org/)
[dbus-sensors (one of the more important EM clients)](https://github.com/openbmc/dbus-sensors/)
[phosphor-dbus-interfaces](https://github.com/openbmc/phosphor-dbus-interfaces/)
[gerrit change for implementation of this design](https://gerrit.openbmc.org/c/openbmc/entity-manager/+/79837)

## Requirements

sorted by subjectively perceived importance (feel free to suggest re-ordering)

- Compliance with DBus design intent
- Represent nested objects and arrays of objects in a PDI-compatible way.
- Reuse finite, well-defined interfaces across multiple records
- Solution should scale to an arbitrary depth of information to be represented
- Preserve backward compatibility with existing EM D-Bus layout.
- Remain discoverable and human-browsable via D-Bus introspection tools.
- Can be implemented over time, without heavy lifting
- Support evolving and extending config schemas and DBus interfaces
- Provide visible error reporting for unsupported schema depths.

## Proposed Design (nested object paths)

### Summary

- **Nested objects** → child object paths, each exposing a reusable
  configuration interface
- **Arrays of objects** → child directories with numeric indices as nodes
- **Primitive properties** stay on the parent object.
- **Arrays of primitive properties** stay on the parent object.

### Example 1: Firmware configuration

```json
{
  "Name": "MyRecord",
  "SPIControllerIndex": 1,
  "SPIDeviceIndex": 0,
  "FirmwareInfo": {
    "VendorIANA": 3733,
    "CompatibleHardware": "com.abcd.myboard.Host.SPI",
    "Type": "FirmwareInfo"
  },
  "MuxOutputs": [
    { "Name": "SPI_SEL_0", "Polarity": "High", "Type": "MuxOutput" },
    { "Name": "SPI_SEL_1", "Polarity": "Low", "Type": "MuxOutput" }
  ],
  "Type": "SPIFlash"
}
```

Old (still supported)

```text
Path: /xyz/openbmc_project/inventory/system/board/myboard/MyRecord
        Iface: xyz.openbmc_project.Configuration.SPIFlash.FirmwareInfo
        Iface: xyz.openbmc_project.Configuration.SPIFlash.MuxOutputs0
        Iface: xyz.openbmc_project.Configuration.SPIFlash.MuxOutputs1
```

New (preferred)

```text
Path: /xyz/openbmc_project/inventory/system/board/myboard/MyRecord/FirmwareInfo
        Iface: xyz.openbmc_project.Configuration.FirmwareInfo
Path: /xyz/openbmc_project/inventory/system/board/myboard/MyRecord/MuxOutputs/0
        Iface: xyz.openbmc_project.Configuration.MuxOutput
Path: /xyz/openbmc_project/inventory/system/board/myboard/MyRecord/MuxOutputs/1
        Iface: xyz.openbmc_project.Configuration.MuxOutput
```

### Example 2: ADC configuration

```json
{
  "CPURequired": 0,
  "EntityId": 19,
  "EntityInstance": 10,
  "Index": 28,
  "MaxValue": 2.4,
  "MinValue": 0,
  "Name": "S0_1V8_SOC",
  "PollRate": 15,
  "PowerState": "On",
  "ScaleFactor": 0.5,
  "Thresholds": [
    {
      "Direction": "greater than",
      "Name": "upper critical",
      "Severity": 1,
      "Value": 2.157
    },
    {
      "Direction": "greater than",
      "Name": "upper non critical",
      "Severity": 0,
      "Value": 2.062
    }
  ],
  "Type": "ADC"
}
```

Old (still supported)

```text
Path: /xyz/openbmc_project/inventory/system/board/myboard/S0_1V8_SOC
        Iface: xyz.openbmc_project.Configuration.ADC
        Iface: xyz.openbmc_project.Configuration.ADC.Thresholds0
        Iface: xyz.openbmc_project.Configuration.ADC.Thresholds1
```

New (preferred)

```text
Path: /xyz/openbmc_project/inventory/system/board/myboard/S0_1V8_SOC
        Iface: xyz.openbmc_project.Configuration.ADC
Path: /xyz/openbmc_project/inventory/system/board/myboard/S0_1V8_SOC/Thresholds/0
        Iface: xyz.openbmc_project.Configuration.Threshold
Path: /xyz/openbmc_project/inventory/system/board/myboard/S0_1V8_SOC/Thresholds/1
        Iface: xyz.openbmc_project.Configuration.Threshold
```

### Proposed Design: Pros

- PDI compatibility: finite set of reusable interfaces; generated code works.
- Discoverability: object tree mirrors structure; browsable with `busctl tree`.
- Granular changes: individual nested objects can emit property changes
  independently.
- Optional properties can be represented via extra DBus interfaces which may or
  may not be there. This is not part of the design but presents a possible
  extension, since there is only one DBus interface per object path, so it
  cannot conflict when using appropriate naming.
- Backwards-compatible: clients relying on legacy flattened layout have a
  migration strategy. This design allows reactors to be migrated one at a time.

### Proposed Design: Cons

- More objects in the bus tree (slightly larger ObjectManager payloads).
- Consumers must traverse subtrees to find nested items.
- Extra code needs to be generated to represent use-cases previously handled by
  a single `InterfacesAdded` handler
- Need to migrate EM `AddObject` API for runtime configuration changes

## Proposed Design: Diagram

Note that generating PDI yaml from EM schemas and supporting optional properties
is not explicitly part of the design but represents possible extensions to it.

These steps are shown here to better illustrate how a complete workflow for
updating configuration schemas and consuming configuration interfaces might look
like.

```text
                          config schema author

          write config schema  |       |  extend config schema
                               v       v
                           +--------------+
                           |config schemas|  generate DBus interfaces
                           +--------------+  from schemas
                   consume |             |
persist runtime            |             |   generate optional DBus interfaces
  config change            v             |   from optional properties
                    +--------------+     v   (possible design extension)
system.json <------ |entity-manager|+------------------------+
                    +--------------+|phosphor-dbus-interfaces|
                           |^       +------------------------+
          expose config    ||            |
          prop changed     ||            |  use generated code
          intf added       ||            |  to fetch config and
          intf removed     ||            |  expose interfaces
                           v|  expose    |
   +------+             +----+ intf    +--------------+
   |bmcweb| ----------> |DBus| <------ |client reactor|
   +------+             +----+ ------> +--------------+
          runtime config   |   query     |  query
           change          |             v
                           |        +------------+
                           +------->|ObjectMapper|
                                    +------------+
                         prop changed
                         intf added
                         intf removed
```

## Alternatives Considered

### 1. Status Quo (flattened interfaces)

- Arrays encoded as unique interfaces (`MuxOutputs0`, `MuxOutputs1`, …).
- Nested objects embedded in parent interface names.

### Alt 1: Pros

- Flat, easy to `grep` in `busctl`
- Clients have fewer signals
- Configuration is compact

### Alt 1: Cons

- Unbounded interface set breaks DBus design intent.
- Incompatible with PDI.
- Properties silently dropped at depth.
- Interfaces are inherently limited in the depth they can represent due to
  [DBus interface name length limitation](https://dbus.freedesktop.org/doc/dbus-specification.html#message-protocol-names-interface)
- Imprecise matching of interfaces
  [has already become a problem in upstream code](https://github.com/openbmc/dbus-sensors/blob/13b3cad1dca451e2c903035e524d77a5134e4376/src/Thresholds.cpp#L64)

### Alt 1: Verdict

unmaintainable, broken design

### 2. Single “blob” property (JSON)

Keep one interface; expose the entire nested config as a string.

### Alt 2: Pros

- Easy to implement.

### Alt 2: Cons

- No type safety
- No granular property-changed signals
- Clients must parse JSON.
- Requires migrating all configuration interfaces

### Alt 2: Verdict

Not type-safe, json parsing overhead, discarded

### 3. Single “blob” property (`a{sv}`)

Keep one interface; expose the entire nested config as a dict, possibly with
nested dicts.

### Alt 3: Pros

- Minimal D-Bus footprint.

### Alt 3: Cons

- No type safety
- No granular property-changed signals
- Clients must check for presence of any property
- Clients must check for type of any property
- Requires migrating all configuration interfaces

### Alt 3: Verdict

Discarded (see
`https://lists.ozlabs.org/pipermail/openbmc/2023-January/032801.html` which goes
into detail as to why this is bad)

### 4. Explicit Typed Containers (structs/arrays)

- Use D-Bus container types (`(...)`, `a(...)`) for nested objects.
- Define reusable structs and enums in PDI YAML.
- Keep all config on the parent object path.

yaml example:

```yaml
types:
  - name: FirmwareInfo
    type: struct
    members:
      - { name: VendorIANA, type: uint32 }
      - { name: CompatibleHardware, type: string }

  - name: MuxOutput
    type: struct
    members:
      - { name: Name, type: string }
      - { name: Polarity, type: enum[Low, High] }

interfaces:
  - name: xyz.openbmc_project.Configuration.SPIFlash
    properties:
      - { name: SPIControllerIndex, type: uint32 }
      - { name: SPIDeviceIndex, type: uint32 }
      - { name: FirmwareInfo, type: FirmwareInfo }
      - { name: MuxOutputs, type: array[MuxOutput] }
```

### Resulting D-Bus signatures

- `FirmwareInfo`: `(us)`
- `MuxOutputs`: `a(sy)`

### Alt 4: Pros

- Flat object graph, easy to query in one shot.
- Strongly typed with PDI.
- Structs enforce consistency.

### Alt 4: Cons

- Any field change emits a change for the entire struct/array.
- Updating one element usually requires rewriting the entire array.
- ABI rigidity: adding struct members breaks signatures → need versioning
  strategies.
- Less discoverable with generic D-Bus tools.
- Less compatible with ObjectMapper, e.g. Thresholds (or other config) at depth
  cannot be queried via their DBus interfaces.
- Need for extra code to be developed to e.g. "Query all Thresholds" since DBus
  does not know what the anonymous struct represents.
- Inconsistency: top-level properties have property names, nested properties do
  not.
- Hard to add optional properties at depth, the property type needs to be
  changed for each instance of schema re-use.

### Alt 4: Verdict

Technically viable, but less intuitive for browsing.

### 5. Explicit Typed Containers (structs/arrays) made consistent

To fix the inconsistency with alternative 3 (top-level properties having names,
nested ones not having names) would require all config properties lose their
names and only having one property `Configuration` on
`xyz.openbmc_project.Configuration.${Type}` interface.

### Alt 5: Pros

- consistent, strongly typed

### Alt 5: Cons

- requires all configuration interfaces to be migrated
- bad developer experience, require new cli tools to recover the property names
  and display things for debug.

### Alt 5: Verdict

Only useful as a thought experiment

### 6. Hybrid of Explicit Typed Containers (structs/arrays) and nested object paths

In this design every config schema must declare if it's to be exposed in a
compact way (inside a property value) or gets its own object path.

### Alt 6: Pros

- Can have separate object paths on an as-needed basis (e.g. `Thresholds`) while
  data which may not need separate signals and properties changed signals can be
  compacted into typed containers (e.g. `FirmwareInfo`)

### Alt 6: Cons

- Implementation overhead
- Slightly inconsistent

I am not seriously considering this design for now since it's twice the
implementation overhead of the two constituent designs.

### 7. Fully flat json schema, config objects linked via association

In this design alternative there is no nesting of json schemas, instead each
config schema gets new required properties `Id` and optionally `ParentNode` /
`ChildNodes`. The resulting DBus objects are not nested but instead get linked
together via `configuring`/`configured_by` association which represents the
child nodes adding additional configuration details. For array semantics, a
`Index` property or similar can be used.

#### Example

```json
[
  {
    "Id": "spi0-flash0",
    "Name": "MyRecord",
    "Type": "SPIFlash",
    "SPIControllerIndex": 1,
    "SPIDeviceIndex": 0,
    "ChildNodes": [
      "spi0-flash0-fwinfo",
      "spi0-flash0-mux-0",
      "spi0-flash0-mux-1"
    ]
  },
  {
    "Id": "spi0-flash0-fwinfo",
    "Type": "FirmwareInfo",
    "ParentNode": "spi0-flash0",
    "VendorIANA": 3733,
    "CompatibleHardware": "com.abcd.myboard.Host.SPI"
  },
  {
    "Id": "spi0-flash0-mux-0",
    "Type": "MuxOutput",
    "ParentNode": "spi0-flash0",
    "Index": 0,
    "Name": "SPI_SEL_0",
    "Polarity": "High"
  },
  {
    "Id": "spi0-flash0-mux-1",
    "Type": "MuxOutput",
    "ParentNode": "spi0-flash0",
    "Index": 1,
    "Name": "SPI_SEL_1",
    "Polarity": "Low"
  }
]
```

### Alt 7: Pros

- Extensible to map a child node to multiple parent notes (e.g. a Threshold
  which applies to multiple sensors, or a GPIO which is used by multiple
  daemons), enabling further re-use.

### Alt 7: Cons

- Easy to get wrong since the `Id` has to match exactly.
- Hard to configure deeper structures, have to keep track of the `Id`
- Hard to review in gerrit since related config objects might be outside diff
  context
- Finding child nodes for configurations requires extra CLI tooling or verbose
  mapper calls to query the associations.
- Clients have to recursively query associations to get the full configuration
  record.
- Possibility to have dangling configuration records which do not link, creating
  error logs
- Bloating configuration files
- All existing nested objects have to be migrated in json config, creating huge
  diff.

### Alt 7: Verdict

Technically viable, but painful to configure

## Impacts

The design has a _well-defined migration strategy_ and the migration can be done
_gradually_. There is no security impact. There is documentation impact, things
will look different in `busctl tree` output. There may be some minor performance
impact, it has not been investigated since we are talking about configuration
which does not usually change that frequently. For developers, there is a need
to learn different ways to consume and produce configuration.

### Organizational

- Does this proposal require a new repository?
  - No
- Who will be the initial maintainer(s) of this repository?
- Which repositories are expected to be modified to execute this design?
  - `entity-manager`, `dbus-sensors`, `phosphor-bmc-code-mgmt`, `bmcweb`,
    `phosphor-dbus-interfaces` and any other reactor which consumes or produces
    nested configuration.

## Testing

There are
[unit-tests](https://gerrit.openbmc.org/c/openbmc/entity-manager/+/82547/8) work
in progress for entity-manager.
