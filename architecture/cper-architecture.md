# OpenBMC CPER Architecture

## Overview

This document describes the high-level architecture for Common Platform Error
Record (CPER) handling within OpenBMC.

The architecture separates CPER storage, decoding, presentation, and external
protocol exposure into independent components with clear ownership boundaries.

The goals are to:

- Support CPER ingestion from multiple platform interfaces.
- Maintain a single authoritative source for CPER artifacts.
- Enable efficient operator-facing diagnostics.
- Support standards-compliant Redfish CPER exposure.
- Allow platform-specific and vendor-specific decode extensions.

Detailed behavior and interfaces are defined by the corresponding design
documents.

---

## Architecture

```text
+----------------------+
|    CPER Producers    |
+----------------------+
| PLDM                 |
| IPMI                 |
| OEM Interfaces       |
| Future Sources       |
+----------+-----------+
           |
           v

+----------------------+
|    CPER Repository   |
+----------------------+
| Raw CPER Records     |
| Retention            |
| Lifecycle            |
+----------+-----------+
           |
           |
           +----------------------+
                                  |
                                  v

                      +----------------------+
                      |  CPER Decode Services |
                      +----------------------+
                      | Common Decoders      |
                      | Platform Extensions  |
                      +----------+-----------+
                                 |
                                 v

                      +----------------------+
                      | phosphor-logging     |
                      +----------------------+
                      | Log Entries          |
                      | Diagnostic Views     |
                      | Search Metadata      |
                      | Correlation Data     |
                      +----------+-----------+
                                 |
                                 v

                      +----------------------+
                      |       bmcweb         |
                      +----------+-----------+
                                 |
                                 v

                      +----------------------+
                      |       Redfish        |
                      +----------------------+
```

---

## Architectural Responsibilities

### CPER Producers

Platform components generate CPER records and deliver them to the BMC through
mechanisms such as PLDM, IPMI, OEM-specific interfaces, and future transport
methods.

### CPER Repository

The CPER Repository is the authoritative source of CPER artifacts within
OpenBMC.

Its responsibilities include:

- Raw CPER storage
- Artifact retrieval
- Retention management
- Lifecycle management

The repository owns the original CPER artifact and remains independent of
presentation and protocol concerns.

### CPER Decode Services

CPER Decode Services transform CPER artifacts into structured diagnostic
information.

The architecture intentionally does not mandate a specific decoding
implementation. Platforms may utilize common decoders, vendor-specific
diagnostic libraries, or platform-specific extensions through a common decode
abstraction.

This enables additional diagnostic capabilities while preserving common
repository and consumer interfaces.

### phosphor-logging

phosphor-logging provides the operator-facing representation of CPER
information.

Its responsibilities include:

- Log entry lifecycle
- Diagnostic summaries
- Decoded CPER representations
- Search metadata
- Event correlation

Decoded diagnostic representations may be persisted by the logging layer to
avoid repeated decode operations while preserving the repository as the
authoritative source of the original CPER artifact.

### bmcweb

bmcweb exposes CPER information through Redfish and maps repository and logging
information into standards-compliant Redfish resources.

---

## References

### OpenBMC

- CPER Repository Design
- Logging Plugin Design
- CPER Records Design

### Industry Standards

- UEFI Common Platform Error Record (CPER)
- DMTF DSP0248 Platform Level Data Model (PLDM)
- DMTF PLDM CPER Event Specification
- Arm Server Base Manageability Requirements (SBMR)
- DMTF Redfish LogEntry Schema
