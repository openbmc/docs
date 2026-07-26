# Logging Plugin Design

**Author:** Jayanth Othayoth <ojayanth@gmail.com>

**Other Contributors:**

- Patrick Williams <patrick@stwcx.xyz>

**Created:** July 25, 2026

---

## Problem Statement

`phosphor-logging` currently provides a fixed mechanism for creating and
managing log entries through the `xyz.openbmc_project.Logging.Entry` interface.

As platform requirements evolve, log entries increasingly need to expose
structured information that is not naturally represented by the base `Entry`
interface. Examples include Common Platform Error Record (CPER) diagnostics,
serviceability metadata, vendor-specific enrichments, and future
platform-specific capabilities.

Adding feature-specific handling directly into the logging framework does not
scale and gradually increases coupling between core logging infrastructure and
individual feature implementations.

This proposal introduces a generic Logging Plugin Framework that allows
additional functionality to participate in the log entry lifecycle through a
common and extensible mechanism while preserving the existing `phosphor-logging`
architecture.

---

## Requirements

### Functional Requirements

- Support producer-initiated plugins.
- Support framework-initiated plugins.
- Support multiple plugins per log entry.
- Support D-Bus interface creation by plugins.
- Support persistence and restoration.
- Support plugin-owned artifacts associated with a log entry.
- Support controlled access to plugin-owned artifacts independent of underlying
  filesystem permissions.
- Preserve existing behavior when no plugins are present.

### Non-Functional Requirements

- Core logging infrastructure shall remain independent of plugin-specific logic.
- Plugin-created interfaces shall be published together with the log entry.
- New plugins shall not require framework modifications.
- Plugin-specific state shall remain owned by the corresponding plugin.
- The framework shall support extension categories beyond diagnostics.

---

## Design Principles

- Core logging infrastructure remains plugin-agnostic.
- Plugins own plugin-specific functionality and state.
- Plugins participate during log entry construction.
- Multiple plugins may contribute to a single log entry.
- Framework concepts shall not be exposed through D-Bus APIs.
- Plugins may expose artifact access mechanisms for plugin-owned data that is
  not represented directly through D-Bus properties.

---

## Architecture Overview

```text
Producer / Framework
          |
          v
     Plugin Request
          |
          v
     Plugin Registry
          |
          v
     Log Entry Plugin(s)
          |
          v
 Additional Interfaces
          |
          v
 emit_object_added()
```

### Core Components

```text
+--------------------+
|  Plugin Registry   |
+--------------------+
          |
          v
+--------------------+
|  Log Entry Plugin  |
+--------------------+
          ^
          |
      CperPlugin
```

---

## Producer Interaction Model

Applications continue to create log entries through `lg2::commit()`.

Plugins are requested using extension objects supplied as additional commit
arguments.

Generic form:

```cpp
lg2::commit(
    Event,
    Metadata...,
    PluginExtensions...);
```

Example:

```cpp
CperInfo cperInfo;

lg2::commit(
    xyz::openbmc_project::State::Cper::Error(),
    Source(cpuInventoryPath),
    extend<Cper>(cperInfo));
```

Multiple plugins may participate in a single log entry:

```cpp
lg2::commit(
    Event,
    Metadata...,
    extend<Cper>(cperInfo),
    extend<PluginX>(pluginXInfo));
```

The framework converts extension requests into plugin requests which are
processed during log entry creation.

Framework-initiated plugins follow the same processing model and may be
generated from platform policy, metadata, or existing extension mechanisms.

---

## Framework Components

### Plugin Registry

Coordinates plugin participation throughout the log entry lifecycle, including
creation, restoration, and cleanup.

### Plugin Request

Represents a request for a plugin to participate in log entry processing.
Requests may originate from producers or framework policies.

### Plugin Context

Provides plugins access to log entry information required during processing.

### Log Entry Plugin

Common abstraction implemented by all plugins. Plugins may create additional
D-Bus interfaces and manage plugin-specific state throughout the log lifecycle.

---

## Interface Publication

Plugin-created interfaces shall exist before the log entry is published on
D-Bus.

Example object:

```text
/xyz/openbmc_project/logging/entry/123
```

Interfaces:

```text
xyz.openbmc_project.Logging.Entry
xyz.openbmc_project.Logging.Diagnostic.CPER
```

Publication flow:

```text
Create Entry (defer_emit)
          |
          +-- Process plugin requests
          |
          +-- Create plugin interfaces
          |
          +-- emit_object_added()
```

This ensures consumers observe a fully constructed log entry and all associated
plugin interfaces.

---

## Plugin Lifecycle

Plugins participate in the complete log lifecycle.

### Create

Create interfaces and initialize plugin-owned state.

### Serialize

Persist plugin-owned state required for restoration.

### Restore

Recreate plugin state and associated D-Bus interfaces.

### Delete

Cleanup plugin-owned resources and artifacts.

Plugin-specific state remains owned and managed by the corresponding plugin
implementation. The framework is responsible only for lifecycle coordination.

---

### Artifact Access

Some plugins may own artifacts associated with a log entry that are persisted
outside of D-Bus properties.

Examples include:

- Raw diagnostic records.
- Decoded diagnostic data.
- Vendor-specific artifacts.
- Serviceability reports.

The framework shall support controlled access to plugin-owned artifacts without
requiring consumers to have direct access to the underlying storage location.

Plugin-owned artifacts may be exposed through plugin-defined interfaces and
retrieval mechanisms. Such interfaces may provide references to artifacts,
direct access to artifact content, or both.

Artifact ownership, representation, and retrieval semantics remain the
responsibility of the corresponding plugin implementation.

---

## Multiple Plugin Support

A single log entry may contain contributions from multiple plugins.

Example:

```text
xyz.openbmc_project.Logging.Entry
xyz.openbmc_project.Logging.Diagnostic.CPER
xyz.openbmc_project.Logging.Serviceability
```

Each plugin remains independently owned and managed.

---

## Example Plugins

### CPER Diagnostic Plugin

The CPER plugin exposes structured CPER information through a dedicated D-Bus
interface.

Producer API:

```cpp
CperInfo cperInfo;

lg2::commit(
    xyz::openbmc_project::State::Cper::Error(),
    Source(cpuInventoryPath),
    extend<Cper>(cperInfo));
```

Result:

```text
xyz.openbmc_project.Logging.Entry
xyz.openbmc_project.Logging.Diagnostic.CPER
```

Representative properties:

```text
Summary
DecodedData
DiagnosticDataObject
Oem
```

Implementations may additionally expose plugin-owned diagnostic artifacts
through implementation-defined retrieval mechanisms.

---

## D-Bus Modeling Guidelines

Framework concepts are implementation details and shall not appear in D-Bus
APIs.

Framework concepts:

```text
Plugin Registry
Plugin Request
Plugin Context
Log Entry Plugin
```

D-Bus interfaces should model the functionality being exposed.

Examples:

```text
xyz.openbmc_project.Logging.Diagnostic.CPER
```

---

## Plugin Categories

The framework is intended to support multiple extension categories.

### Diagnostic Plugins

Examples:

```text
xyz.openbmc_project.Logging.Diagnostic.CPER
```

### Serviceability Plugins

Future plugins may expose serviceability-related information through dedicated
interfaces.

### Vendor Plugins

Examples:

```text
xyz.openbmc_project.Logging.Oem.*
```

Additional categories may be introduced without modifying the framework
architecture.

---

## Alternatives Considered

## AdditionalData-Based Transport

Use `AdditionalData` metadata to transport extension-specific information.

### Pros

- Small implementation footprint.
- Reuses existing metadata transport mechanisms.

### Cons

- No clear ownership or schema boundaries between extensions.
- Requires extension-specific parsing and interpretation within core logging
  code.
- Couples extension functionality to log metadata representation.
- Does not naturally support interface creation, persistence, restoration, or
  lifecycle management.

### Rationale for Rejection

The plugin framework provides clear separation between logging infrastructure
and extension-specific functionality. Plugins own their data, lifecycle, and
D-Bus interfaces, allowing `phosphor-logging` to remain independent of
feature-specific implementation details while supporting future extensibility.

---

## Impacts

### phosphor-logging

Introduces:

- Plugin Registry
- Plugin Request
- Plugin Context
- Log Entry Plugin

### Existing Extension Infrastructure

Existing extension mechanisms may participate by generating plugin requests.

### D-Bus Interfaces

Existing log entries continue to function without modification.

Plugins may attach additional D-Bus interfaces to log entries as required by the
functionality being implemented. These interfaces are defined and owned by the
corresponding plugin implementation.

Plugins may also expose mechanisms for accessing plugin-owned artifacts when
direct filesystem access is not appropriate or available.

---

## Testing

- Entry creation without plugins.
- Single-plugin entry creation.
- Multiple-plugin entry creation.
- Persistence and restoration validation.
- D-Bus interface validation.
- Lifecycle validation.
- Backward compatibility testing.

---

## Future Work

- Additional diagnostic plugins.
- Serviceability plugins.
- Vendor-specific plugins.
- Platform-specific enrichment plugins.

---

## Conclusion

The Logging Plugin Framework provides a generic and extensible mechanism for
augmenting log entries with additional functionality while keeping the core
logging infrastructure independent of feature-specific implementations. It
enables capabilities such as CPER diagnostics to be integrated through dedicated
interfaces without introducing feature-specific coupling into
`phosphor-logging`.
