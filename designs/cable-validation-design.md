# Cable Validation

Author: Dhruvaraj Subhashchandran
Other contributors: None
Created: July 10, 2026

## Problem Description

Modular systems can use externally serviceable cables to connect compute,
accelerator, storage, network, or I/O nodes. The physical cable topology is
part of the system configuration. Each required cable must connect the
expected endpoints and must provide the required working links or lanes for
the system to operate correctly.

Cable presence by itself is not sufficient to establish that the
configuration is correct. A cable can be present but connected to the wrong
peer, contain a break, have one or more failed lanes, or be unable to establish
the expected link. These failures can prevent nodes from joining a fabric,
reduce bandwidth or redundancy, or cause failures during system boot or
operation.

OpenBMC needs a common high-level procedure for actively validating a
collection of cables against the expected platform configuration and for
reporting the result of that validation. The first use case is validating the
fabric cables configured between computing nodes. The same procedure may also
be applicable to external PCIe, CXL, or other managed high-speed interconnect
cables for which the platform knows the expected endpoints and provides a
suitable diagnostic mechanism.

The procedure must not require one common hardware test for every platform.
The actual diagnostic mechanism can be specific to the cable type, protocol,
controller, hardware vendor, firmware, or platform. This design does not
define a hardware diagnostic algorithm, a platform configuration format, a
management API, or a guided cable installation and repair procedure.

## Background and References

OpenBMC already represents cables as inventory objects. Platforms can also
implement cable-presence detection and monitor whether expected cables are
inserted or removed. Cable validation is a separate operation because it
actively determines whether the observed connection and health match the
expected cable configuration.

A cable-validation implementation can obtain observations from one or more
platform-specific mechanisms, including:

- endpoint or peer discovery;
- fabric or protocol link-training results;
- per-link or per-lane status;
- PHY or electrical cable diagnostics;
- loopback testing;
- time-domain reflectometry;
- controller or device firmware commands; or
- vendor-specific cable-management interfaces.

This design does not select or standardize any of these mechanisms. It defines
the common validation procedure around them.

The expected cable configuration can come from any platform-owned source, such
as static platform data, generated inventory, a topology description, or
another configuration service. The format and ownership of that data are not
standardized by this design.

Cable validation can be used during manufacturing, installation, service,
cable replacement, system boot, or a maintenance operation. The implementation
is responsible for allowing validation only in a system state in which its
diagnostic can run safely.

## Requirements

A platform implementing cable validation shall provide a component that:

1. Knows the set of cable inventory objects managed by the platform.
2. Knows the expected endpoints and required connectivity for those cables.
3. Can perform, or delegate, the diagnostic needed to observe cable
   connectivity and health.
4. Can validate all managed cables in a selected scope as one coordinated
   operation. The scope can be based on the functional class of the cable,
   with fabric cables as the initial use case.
5. Compares the observed result with the expected cable configuration.
6. Produces an individual result for every cable selected for validation.
7. Records a serviceable failure when a cable does not match the expected
   configuration or does not meet the required health criteria.
8. Identifies the failed cable, connector, or associated replaceable resource
   using a fault LED when the platform supports such an indicator.
9. Continues validating the remaining cables after an individual cable failure
   when the diagnostic mechanism permits it.
10. Prevents validation from running when the required hardware is unavailable
    or when running the diagnostic would be unsafe.

Where the platform diagnostic supports the distinction, validation should be
able to identify the following conditions:

- an expected cable or connection is missing;
- a cable is connected to an unexpected endpoint;
- a cable is present but the expected link cannot be established;
- one or more required cable lanes are not functional;
- the connection is operational but provides less capacity than required; and
- the diagnostic could not determine a result.

The common design shall not define electrical thresholds, training algorithms,
lane-count requirements, endpoint-identification methods, or other
hardware-specific rules. Those rules belong to the cable-specific,
protocol-specific, or vendor-specific validation implementation.

Validation of a complete system can require multiple hardware operations and
can take a significant amount of time. It must run without blocking unrelated
BMC work. A failure of one selected cable is a validation result and should not
normally stop the overall operation. An operation-level failure occurs when
validation cannot start or continue, for example because configuration data is
unavailable, diagnostic hardware is unavailable, or the system is in an
invalid state.

Cable validation is an optional platform capability. Platforms without an
active cable diagnostic are not required to implement it.

## Proposed Design

### Components and ownership

The design separates the common validation procedure from the mechanism used
to test a cable.

```text
+------------------------+
| Management or platform |
| workflow               |
+-----------+------------+
            |
            | Request validation for a cable scope
            v
+-----------+------------+       +--------------------------+
| Cable validation       |<----->| Expected cable topology  |
| coordinator            |       | and inventory            |
+-----------+------------+       +--------------------------+
            |
            | Select and invoke the appropriate implementation
            v
+-----------+------------+
| Cable, protocol, or    |
| vendor-specific        |
| validation provider    |
+-----------+------------+
            |
            | Per-cable observations and results
            v
+-----------+------------+       +-------------+   +---------+
| Cable inventory state  |       | Event logs  |   | LEDs    |
+------------------------+       +-------------+   +---------+
```

The **cable validation coordinator** is the platform component that owns, or
can obtain, the expected cable configuration. It receives a request to validate
a cable scope, selects the affected cable inventory objects, invokes the
required diagnostic providers, correlates the returned observations, and
publishes the results. The coordinator can be part of an existing platform
service. This design does not require a new generic cable-validation daemon.

A **validation provider** is the cable-specific, protocol-specific, or
vendor-specific implementation that performs the actual diagnostic. It can
validate one cable at a time, several related cables together, or the complete
fabric in one hardware operation. The provider interface is internal to the
platform implementation and is not standardized by this design.

A platform can use more than one provider. For example, fabric cables can be
validated through a fabric-controller diagnostic, while an external PCIe or
CXL interconnect can use its own controller-specific diagnostic. The
coordinator selects the provider appropriate for each group of managed cables.

### Generic validation procedure

A validation operation follows this high-level procedure:

1. **Select cables**

   The coordinator selects all managed cable inventory objects in the
   requested scope. The selected set is captured for the duration of the
   operation so that every result refers to a consistent expected
   configuration.

2. **Load the expected configuration**

   For each selected cable, the coordinator obtains the expected endpoints and
   the connectivity or health requirements needed to determine success. A
   provider can consume this information directly when it performs the
   comparison internally.

3. **Check diagnostic preconditions**

   The coordinator verifies that the system is in a state in which validation
   can run safely, that the required endpoints and diagnostic hardware are
   accessible, and that no conflicting operation is active. Some platforms can
   validate a live link, while others can validate only during boot or
   maintenance.

4. **Plan and run the diagnostic**

   The coordinator groups, sequences, or parallelizes the selected cables
   according to platform requirements and invokes the appropriate providers.
   Each provider observes connectivity and health using its own cable-specific
   or vendor-specific mechanism.

5. **Evaluate each cable**

   The provider or coordinator compares the observed endpoints, link state,
   lane state, and other relevant information with the expected configuration.
   Every selected cable receives a normalized outcome.

6. **Publish results**

   The coordinator updates cable inventory state and records event logs for
   failed cables. Where the platform supports service indicators, it activates
   the fault LED associated with the cable, connector, or related replaceable
   resource.

7. **Complete the operation**

   The coordinator continues until all selected cables have a result or until
   an operation-level failure prevents further validation. Cables that were
   not tested because of an operation-level failure remain distinguishable
   from cables that were tested and failed.

### Normalized validation outcomes

The diagnostic details remain implementation-specific, but their meaning is
normalized into the following conceptual outcomes:

- **Passed:** The expected endpoints are connected and the required links or
  lanes are usable. The cable is recorded as present and functional. Any
  previous fault indication is reconciled according to platform policy.
- **Missing:** The expected cable or connection was not observed. The cable is
  recorded as missing, an event is logged, and the expected cable or connector
  is identified where possible.
- **Incorrect connection:** A connection was observed, but its peer does not
  match the expected topology. The affected connection is recorded as
  non-functional, the expected and observed endpoints are logged, and the
  relevant connectors are identified where possible.
- **Failed or degraded:** The expected endpoints are connected, but the cable,
  link, or one or more required lanes are unusable. The cable is recorded as
  non-functional or degraded, available diagnostic details are logged, and the
  associated fault indication is activated.
- **Not validated:** The diagnostic could not produce a conclusive result for
  the cable. This remains distinguishable from a tested cable failure. The
  diagnostic or operation-level problem is logged where appropriate.

The normalized outcome does not replace detailed diagnostic information. The
inventory state provides the current high-level cable condition, while the
event log can contain the failure reason, observed and expected endpoints,
failing links or lanes, and implementation-specific diagnostic data.

A successful revalidation can return a repaired cable to a functional state.
Clearing a fault LED or resolving an earlier event follows existing platform
policy and is not prescribed by this design.

### Error handling and concurrency

Individual cable failures do not stop the operation. The coordinator should
continue validating independent cables so that one operation provides the most
complete view possible.

The operation can stop early when a common dependency fails, such as the
validation provider or diagnostic hardware becoming unavailable. The
coordinator records the operation-level failure and does not report untested
cables as cable failures.

Only one validation operation should control the same diagnostic resources at
a time unless the platform explicitly supports concurrency. Independent cable
groups can be validated in parallel when their providers and hardware permit
it.

### Extension to other cable technologies

The common procedure is intentionally not tied to a particular fabric or
protocol. Another cable technology can use it when the platform can provide:

- cable inventory grouped by functional purpose;
- an expected configuration for the selected cables;
- a diagnostic provider capable of observing connectivity or health; and
- a mapping from provider results to inventory, event logs, and indicators.

For example, an external PCIe or CXL cable implementation could validate
expected endpoint connectivity and link health using platform-specific
controller diagnostics. It would use the same high-level procedure while
keeping all technology-specific diagnostic behaviour inside its provider.

The management interface used to initiate this procedure and observe its
completion will be defined separately. It must not expose or standardize the
internal diagnostic provider.

## Alternatives Considered

### Use cable-presence monitoring only

Presence monitoring cannot reliably determine that a cable is connected to the
expected endpoint, that the link can be established, or that every required
lane is functional. It is therefore insufficient for validating a fabric.

### Define one common hardware diagnostic

Cable technologies and platforms expose different observations and have
different safety and sequencing requirements. Standardizing the hardware test
would either exclude valid implementations or expose vendor details in the
common design. This proposal standardizes the procedure and result handling
while leaving the diagnostic implementation platform-specific.

### Validate each cable independently from the caller

Making the caller enumerate and validate each cable independently would move
topology knowledge and sequencing into the caller. Some platforms provide one
diagnostic for several related links or require a particular sequence across
endpoints. A coordinated collection-level procedure keeps those decisions with
the platform component that owns the expected topology and hardware access.

### Define a fabric-specific procedure

The initial implementation is for fabric cables, but the same high-level flow
may also be applicable to external PCIe, CXL, or other managed high-speed
interconnect cables. Keeping the common procedure independent of fabric
technology permits another suitable cable technology to reuse it while still
using a different validation provider.

## Impacts

### API

This design does not define a management or internal software API. A follow-on
proposal can define how a caller selects a cable scope, starts validation, and
observes completion. That API should treat validation as potentially
long-running and should distinguish an operation-level failure from a
completed operation that discovered one or more faulty cables.

### Security

Some diagnostics can interrupt traffic, retrain a link, or require access to
privileged hardware operations. The coordinator must verify the system state
and platform policy before starting validation. Any externally accessible
request is responsible for authorizing the caller before invoking the local
validation procedure.

The expected cable configuration is trusted platform data. Implementations
must not allow an untrusted caller to replace the expected topology or inject
provider-specific diagnostic parameters through the common request.

### Performance

Validation can require commands on many endpoints and can take a significant
amount of time. Implementations should avoid unnecessary repeated tests,
parallelize independent checks where safe, and serialize access to shared
hardware or firmware resources.

Existing cable inventory, cable-presence detection, and cable monitoring can
continue to operate without active validation. Platforms that do not support
active cable diagnostics are unaffected.

### Organizational

- Does this proposal require a new repository? No.
- Who will be the initial maintainer of a new repository? Not applicable.
- Repositories expected to be modified:
  - `openbmc/docs` for this design;
  - one or more platform implementation repositories; and
  - an interface repository only when a common API is proposed separately.

## Testing

The coordination logic should be unit-tested with mock validation providers.
Tests should cover cable selection, grouping by provider, correlation of
endpoint observations, normalization of results, continuation after an
individual cable failure, and termination after an operation-level failure.

A platform implementation should test the following scenarios where its
diagnostic supports them:

1. All selected cables are connected to the expected endpoints and pass.
2. An expected cable is missing.
3. A cable is connected to an unexpected endpoint.
4. A cable is present but cannot establish the expected link.
5. One or more required lanes fail or the connection is degraded.
6. One cable fails while validation continues for the remaining cables.
7. The validation provider is unavailable before the operation starts.
8. A common diagnostic dependency fails after some cables have been tested.
9. A conflicting validation operation is already active.
10. Revalidation after a repair returns the cable to the expected state and
    updates the fault indication according to platform policy.
11. The selected scope contains no managed cables.
12. Different cable classes use different providers without exposing
    provider-specific behaviour through the common procedure.

Tests should verify the resulting cable inventory state, event logs, available
diagnostic data, and fault LED behaviour. Hardware-specific diagnostic
accuracy and electrical thresholds are tested by the platform implementation.
Platforms that do not implement cable validation require no additional CI
coverage.
