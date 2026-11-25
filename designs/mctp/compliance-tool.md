# DMTF Protocol Compliance Testing Tool for OpenBMC

Author: Rajeswari Ravichandran

Other contributors: Javaneh, Mahendra

Created: August 31, 2026

## Problem Description

As OpenBMC adoption grows across vendors, ensuring that BMC MCTP
endpoints conform to DMTF specifications becomes increasingly
important. This proposal introduces a standalone, open-source
compliance testing tool that validates MCTP (DSP0236), PLDM
(DSP0240), and SPDM (DSP0274) endpoint responses -- both mandatory
and optional commands -- against the published DMTF standards at
the byte level. The tool exercises endpoints with spec-derived test
vectors and produces structured pass/fail verdicts tied to spec
section citations, giving developers and CI systems a reliable,
automated way to verify protocol conformance.

An implementation is available for review and provides positive,
negative, truncated, and idempotency test variants across the
supported protocol command catalog.

## Background and References

The DMTF (Distributed Management Task Force) publishes a family of
management protocols used by BMCs to communicate with platform
devices such as GPUs, NICs, and storage controllers:

- **MCTP** (Management Component Transport Protocol,
  [DSP0236 v1.3.3][1]): Transport layer for BMC-to-device
  communication with endpoint addressing, routing, and message
  fragmentation. Each device gets an Endpoint ID (EID, range 1-254).
  MCTP delivers messages regardless of the physical wire (I2C, PCIe,
  USB, etc.).

- **PLDM** (Platform Level Data Model, [DSP0240 v1.1.0][2]):
  Higher-level request-response protocol riding on MCTP for platform
  monitoring, firmware update, and device discovery.

- **SPDM** (Security Protocol and Data Model, [DSP0274 v1.4.0][3]):
  Cryptographic device authentication and attestation over MCTP. The
  tool supports SPDM versions 1.0.x through 1.4.x, using the latest
  available revision for each supported version.

The tool uses the Linux AF_MCTP socket interface (family 45), which
enables userspace applications to send and receive MCTP messages
directly. Linux kernel 6.6 is the validated target kernel:

```
  User program (this tool)
       |
       | AF_MCTP socket (sendto / recvfrom)
       v
  Linux Kernel MCTP subsystem
       |
       | fragmentation, routing, tag management
       v
  Physical bus driver (I2C, PCIe, etc.)
       |
       v
  Hardware device (GPU, NIC, storage controller, ...)
```

OpenBMC already hosts MCTP infrastructure (`libmctp`, `mctpd`,
`dbus-sensors`) and PLDM/SPDM daemons. The `mctp` utilities in the
kernel tree provide basic endpoint management but no compliance
validation.

Existing approaches include manual scripts, vendor-specific test
harnesses, interoperability events, and protocol emulators such as
[spdm-emu][4]. The proposed tool complements these approaches by
providing automated, byte-level conformance validation with DMTF
section citations across MCTP, PLDM, and SPDM. Its use of AF_MCTP
allows the same validation workflow to be used with the OpenBMC
ecosystem and supported endpoint bindings.

[1]: https://www.dmtf.org/sites/default/files/standards/documents/DSP0236_1.3.3.pdf
[2]: https://www.dmtf.org/sites/default/files/standards/documents/DSP0240_1.1.0.pdf
[3]: https://www.dmtf.org/sites/default/files/standards/documents/DSP0274_1.4.0.pdf
[4]: https://github.com/DMTF/spdm-emu

## Requirements

### Users

The primary users are:

1. **BMC platform developers** validating that their MCTP endpoint
   implementations (in firmware or kernel drivers) conform to DMTF
   specifications before shipping.

2. **OpenBMC CI systems** running automated compliance checks as part
   of integration testing to catch protocol regressions.

3. **Device vendors** verifying that their PCIe/USB/I2C devices
   respond correctly to MCTP control, PLDM, and SPDM commands from
   the BMC.

### Functional Requirements

- Test MCTP Control Protocol commands defined in DSP0236 sections
  12.3 through 12.23 (18 commands, ~10 test variants per command),
  plus a composite discovery workflow.
- Test PLDM Base commands defined in DSP0240 sections 8.1 through
  8.4 (5 commands plus a multi-step discovery workflow), and the
  implemented PLDM Type 2 Platform Monitoring command catalog.
- Test SPDM commands defined in DSP0274 sections 10.2 through 10.10
  (7 commands with VCA state machine management), supporting SPDM
  versions 1.0.x through 1.4.x, where x is the latest available
  revision of the selected version.
- Support both **live mode** (AF_MCTP kernel sockets, sending real
  requests to endpoints) and **pcap mode** (offline analysis of
  captured MCTP traffic).
- Produce HTML, CSV, and log output with per-endpoint, per-command
  results, including byte-level annotations and spec section
  citations.
- Support binding-aware test filtering (e.g., discovery commands
  only applicable to PCIe VDM endpoints per DSP0236 section
  12.14-12.16).

### Non-Functional Requirements

- Must build with meson and compile with C++23 on Linux.
- Must not depend on vendor-specific MCTP daemons for core
  operation.
- Must use AF_MCTP sockets as the primary live transport. Linux
  kernel 6.6 is the validated target kernel.
- Must be extensible to add new protocol commands and test variants
  without modifying core framework code.

### Scope

- **In scope**: MCTP Control (DSP0236), PLDM Type 0 (Base) and
  Type 2 (Platform Monitoring) (DSP0240), and SPDM (DSP0274)
  compliance testing. CLI interface. HTML and CSV report generation,
  and log output. Live capture and pcap offline analysis.
- **Out of scope**: PLDM Types 1 and 3-6 (BIOS, FRU, firmware update,
  Redfish). SPDM session-based operations. GUI interface.
  Protocol emulation or endpoint implementation.

### Configuration

The tool is configured via CLI arguments or a JSON configuration
file. Users specify the protocol, target EIDs, commands, and test
IDs. This falls into the "system-specific" option class -- each
deployment selects which endpoints and tests to run based on their
platform topology.

## Proposed Design

The compliance tool uses a layered architecture with five major
subsystems. Each layer has a single responsibility:

```
+-----------------------------------------------------------+
|  1. CLI Interface (main.cpp)                              |
|     list, test, capture, and raw commands; parses CLI     |
|     arguments, merges JSON configuration, dispatches      |
|     to TestRunner.                                        |
+-----------------------------------------------------------+
|  2. Test Runner (runner.cpp/hpp)                          |
|     Loads catalog, filters tests, selects live or pcap    |
|     path, iterates endpoints, collects results, and       |
|     invokes report generators.                            |
+-----------------------------------------------------------+
|  3. Protocol Validators (protocols/MCTP, SPDM, PLDM)     |
|     Per-command descriptors encode requests and validate  |
|     responses byte-by-byte.                               |
|  +--------------+ +--------------+ +--------------+      |
|  | MCTP Control | |     SPDM     | |     PLDM     |      |
|  |              | |              | | Type 0/2     |      |
|  +--------------+ +--------------+ +--------------+      |
+-----------------------------------------------------------+
|  4. Live Transport (transport/)                           |
|     IMctpTransport interface implemented by               |
|     AfMctpTransport using kernel AF_MCTP sockets.         |
+-----------------------------------------------------------+
|  5. Input and Output Support                              |
|  +----------------+ +----------------+ +--------------+  |
|  | NETLINK_ROUTE  | | PcapParser and | | HTML, CSV,   |  |
|  | topology query | | MCTP reassembly| | and log output| |
|  +----------------+ +----------------+ +--------------+  |
+-----------------------------------------------------------+
```

### Transport Layer

The transport is abstracted behind the `IMctpTransport` interface.
This decouples test logic from the physical delivery mechanism.

**AfMctpTransport**: Opens a Linux AF_MCTP socket (family 45),
builds a `sockaddr_mctp` with the destination EID, and uses
`sendto()`/`recvfrom()` to exchange messages. The kernel handles
fragmentation, reassembly, message-tag allocation, and routing.
Supports optional extended addressing (specifying physical interface
index and hardware address) for setups that bypass the kernel
routing table.

**PcapParser**: Reads `.pcap` capture files (produced by
`tcpdump -i mctpN`), decodes SLL/SLL2 link-layer headers, and
extracts MCTP packets. The runner pairs requests with responses and
feeds them through the same validators used in live mode.

### External Interfaces

The command-line interface provides `list`, `test`, `capture`, and
`raw` commands. It accepts command-line or JSON configuration and
returns process exit codes suitable for scripting and CI use.

For live runs, the tool uses AF_MCTP sockets and `NETLINK_ROUTE`
queries to communicate with endpoints and discover kernel-known
topology, bindings, and network IDs. For offline runs, it accepts
pcap capture files. It writes HTML reports, `results.csv`, and log
output to the selected output directory. The tool introduces no
D-Bus interfaces and does not change existing OpenBMC API contracts.

### State Management

The tool is an on-demand process and does not own persistent platform
state. Per-run state consists of selected endpoints, filters,
captured exchanges, and execution records. Protocol state lasts only
for the active run; for example, the tool completes SPDM VCA
negotiation before running commands that require it. Endpoint state
is changed only by explicitly selected test commands, such as address
or rate-limit operations.

### Test Catalog and Descriptor Model

Tests are organized in a three-level hierarchy:

```
TestCatalog
  +-- TestDefinition (metadata: testId, specRef, description)
  +-- CommandDescriptor (commandCode, array of TestVariants)
        +-- TestVariant (payload, expected result, validator)
```

Each variant specifies what header bits to set, what payload to
append, whether a valid response is expected or the device should
silently drop the malformed request, and a validator function that
examines the response byte-by-byte.

Compliance verdicts:

| Verdict       | Meaning                                        |
|---------------|------------------------------------------------|
| Pass          | Response is fully spec-compliant               |
| Fail          | Response violates the specification            |
| NotTested     | Test could not run (unreachable, skipped)      |
| NotApplicable | Test does not apply (e.g., wrong binding)      |

### Handling of Optional and Conditional Commands

DSP0236, DSP0240, and DSP0274 each designate some commands, and some
features within otherwise-mandatory commands, as optional or
conditional on capabilities the endpoint advertises -- for example,
discovery commands that only apply to PCIe VDM bindings, or rate- and
interface-query commands a device is permitted to decline. The tool
treats "does not implement an optional command" as a distinct outcome
from "implements it incorrectly," so it does not report a Fail
against a compliant endpoint that has simply chosen not to implement
an optional feature.

The catalog encodes optionality per command using two mechanisms:

- **Binding-aware filtering**: commands that apply only to specific
  physical transports (e.g., Prepare-for-Endpoint-Discovery,
  Endpoint-Discovery -- PCIe VDM only) are marked NotApplicable when
  run against an endpoint on a binding that does not support them,
  and their malformed-request variants are skipped rather than
  executed.
- **Permissive completion-code handling**: for commands the spec
  allows an endpoint to reject or ignore (e.g., Query-Rate-Limit,
  Query-Supported-Interfaces), the valid-request test variant accepts
  either the fully spec-compliant response or the spec-defined "not
  supported" completion code (or, where the spec permits it, a silent
  drop) as a Pass. Any other response is a Fail.

Verdicts distinguish these cases explicitly in report output:
NotApplicable records that a test does not apply to the endpoint's
binding or configuration, and NotTested (with a ProtocolUnsupported
reason) records that the endpoint did not advertise the message type
or capability required to run the test at all. Both are reported
separately from Fail in the HTML, CSV, and log output so reviewers
can distinguish unimplemented optional behavior from spec violations.
The same verdict model and mechanisms apply uniformly across MCTP,
PLDM, and SPDM, since optionality handling is implemented in the
shared test-catalog and result types rather than per protocol.

### MCTP Control Protocol (DSP0236)

The initial catalog covers 18 MCTP Control commands from DSP0236
sections 12.3 through 12.23, plus a composite discovery workflow.
The workflow is a pseudo-command that coordinates multiple MCTP
Control exchanges; it is not counted as a separate DSP0236 command.
Each test validates the MCTP control header, completion code, and
command-specific response fields as applicable to the selected
command.

Commands 0x0B, 0x0C, and 0x0D are binding-aware: they apply only to
PCIe VDM endpoints. The tool detects each endpoint's physical
transport type and marks these tests NotApplicable on I2C/SMBus
endpoints.

### SPDM (DSP0274)

SPDM provides device authentication. Before most commands can be
issued, both sides must complete a VCA (Version-Capabilities-
Algorithms) handshake. The tool performs this sequence before
running tests that require negotiated protocol state.

The tool automatically performs the VCA handshake before running any
command that requires it. It negotiates and validates SPDM versions
1.0.x through 1.4.x, using the latest available revision for the
selected version. SPDM messages use MCTP message type 0x05.

The initial SPDM catalog covers version negotiation, capability and
algorithm negotiation, digest and certificate retrieval, challenge,
and measurement operations. It also validates the applicable DSP0274
response-time budgets.

### PLDM (DSP0240)

PLDM Type 0 (Base) provides the discovery layer. The catalog covers
the TID, version, type, and command discovery operations, including a
composite discovery workflow with cross-consistency checks.

The implementation also includes a PLDM Type 2 (Platform Monitoring)
catalog. It covers PDR repository queries, sensor and effecter reads
and updates, event receiver configuration, platform event messages,
and event-log operations. This coverage is complementary to Type 0
discovery and enables functional and regression testing of the wider
MCTP PLDM stack.

### MCTP Packet Reassembly

Multi-packet MCTP messages are reassembled per DSP0236 section
8.2.1. The reassembler tracks fragments by {srcEid, dstEid,
messageTag, tagOwner} and validates the 2-bit packet sequence
counter. This is used primarily in pcap mode where the tool sees
raw fragments from the capture file.

### Report Generation

The tool generates self-contained HTML reports with overall,
per-endpoint, detailed response, and issue views. It also produces a
CSV result file for CI processing and log output for troubleshooting
and audit use. Reports identify each verdict and include applicable
specification references and response annotations.

## Alternatives Considered

### Extend existing OpenBMC daemons with test modes

Adding compliance test capabilities to `mctpd` or `pldmd` was
considered. This was rejected because it couples test logic with
production daemon code, increasing complexity and risk. Daemons have
different lifecycle requirements (always-on service vs. on-demand
tool). A standalone tool can also be run from development hosts, not
just BMCs.

### Shell-script-based testing

Using shell scripts with `mctp` CLI utilities was considered. This
was rejected because shell scripts cannot perform byte-level
response validation with spec section citations, produce no
structured reporting, are difficult to maintain for 30+ commands
with 10+ variants each, and cannot handle SPDM state machines or
MCTP packet reassembly.

### Use DMTF spdm-emu

The DMTF [spdm-emu][4] provides SPDM protocol emulation. It was
rejected because it focuses on emulation rather than compliance
validation (no pass/fail verdicts against spec requirements), covers
only SPDM (not MCTP Control or PLDM), and does not integrate with
the OpenBMC AF_MCTP transport.

### Vendor-specific test harnesses

Several vendors maintain internal MCTP/PLDM test tools. These were
rejected because they are not publicly available, often depend on
vendor-specific transport layers, and no single tool covers MCTP +
PLDM + SPDM comprehensively.

## Impacts

### API Impact

None. The compliance tool is a standalone binary that uses existing
kernel AF_MCTP sockets. It does not expose D-Bus interfaces or
modify any existing OpenBMC APIs.

### Security Impact

Minimal. The tool sends protocol-compliant (and intentionally
malformed) requests to MCTP endpoints for testing purposes. It
should only be used in development and test environments. The tool
does not store credentials or sensitive data.

### Documentation Impact

A README documenting build instructions, usage examples, and
supported test coverage will be provided in the repository.

### Performance Impact

None. The tool runs on-demand and does not affect BMC runtime
performance.

### Developer Impact

Positive. Developers gain a standardized, openly available tool to
validate their MCTP endpoint implementations against DMTF
specifications. The tool is designed for easy extensibility: adding
a new protocol command requires creating a single `.cpp` file and
registering it in the test catalog -- no framework changes needed.
The pcap mode enables regression testing without hardware, lowering
the barrier for CI adoption.

### Upgradability Impact

None. The tool is a standalone utility with no persistent state or
configuration that needs migration.

### Organizational

- Does this proposal require a new repository? **Yes**
- Who will be the initial maintainer(s) of this repository?
  - Rajeswari Ravichandran
  - Balamurugank
- Source code maintainers:
  - Mahendra Kumar Kodidala
  - Balamurugank
  - Javaneh
- Which repositories are expected to be modified to execute this
  design?
  - New repository: `openbmc/compliance-tool` (the tool itself)
  - `openbmc/openbmc` (meta-layer recipe for Yocto integration,
    optional)

## Testing

The tool is validated at two levels: component-level unit testing and
end-to-end functional testing.

### Unit Testing

Component-level logic that does not require live hardware or a real
endpoint -- response validators, the pcap parser, MCTP packet
reassembly, and the JSON config parser -- is covered by unit tests
written with the GoogleTest (gtest/gmock) framework. Test binaries
are registered as Meson `test()` targets alongside the existing
`meson.build`, so the suite runs with the standard `meson test`
invocation and integrates with any CI system that already drives
Meson-based projects, without requiring a separate test runner or
hardware access. GoogleTest is the test framework; Meson is the build
system and test-execution mechanism that discovers and runs it. Unit
tests exercise each validator against canned request/response byte
sequences (valid, malformed, and truncated) so that regressions in
byte-level checks are caught without a live endpoint.

This unit-test suite is scoped as near-term follow-up work to this
design; it is not yet implemented in the snapshot under review, so
upstream reviewers can evaluate the architecture and test-catalog
design before the full GoogleTest suite lands.

### Functional Testing

Initial validation is performed through functional testing against
available MCTP endpoints. The live path verifies request transmission,
response validation, endpoint selection, and generated compliance
results end-to-end over AF_MCTP. The offline path replays captured
MCTP traffic (`.pcap`) through the same protocol validators used in
live mode, exercising packet parsing and reassembly without requiring
hardware.

OpenBMC CI integration is not addressed in this section; see the CI
and reporting requirements under separate review.
