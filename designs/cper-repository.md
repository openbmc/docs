# CPER ingress / egress

Author: Brad Bishop - radsquirrel

Created: Sept 16, 2025

## Problem Description

Some server CPUs implement a manageability standard referred to as Common
Platform Error Record (CPER). Users expect to access these records via the BMC's
Redfish implementation.

## Background and References

CPER is defined as an industry standard in the [UEFI Specification][uefi_spec].

In Redfish specification drop 2021.3, support for CPER was added to [LogEntry
resources][logentry]. LogEntry resources expose a section by section decoded
CPER instance, as well as the complete CPER in raw form as either a base64
encoded DiagnosticData or a downloadable attachment specified in
AdditionalDataUri. In addition to Redfish, sending CPER events to the BMC using
[MCTP/PLDM][cperevent] was added to DMTF's [DSP0248][DSP0248] in version 1.3.0,
and is specified as a common to ARM OEM IPMI command in the [ARM
SBMR][arm_sbmr].

Decoding CPER has been discussed previously [in another
design][cper-records.md]; however, that design deferred exactly how CPER should
flow into and out of the BMC. This design picks up where the previous stopped.

It should be noted that CPER has been a part of previous broader design
proposals that failed in one way or another (never merged or partially
implemented) such as those from [AMD][amd_ras] and [Google][google_ras]. This
design limits its scope to CPER ingress/egress. It should be straightforward to
build upon the implementation of this design; for example, for AMD to store CPER
encoded crash dumps, or for Google to finish the implementation of a Redfish
'FaultLog' LogService that points to entries in other Redfish LogService
instances (like the one proposed by this design).

## Requirements

- A BMC should be able to receive and store from a CPU, and subsequently recall
  CPERs.

## Proposed Design

A new D-Bus service `xyz.openbmc_project.CPERRepository1` shall implement the
`xyz.openbmc_project.CPERRepository1` interface at
`/xyz/openbmc_project/CPERRepository1`, which shall provide the following
methods:

`xyz.openbmc_project.CPERRepository1.Delete(in=ao,out=)` shall delete the CPER
interfaces on the paths specified.

`xyz.openbmc_project.CPERRepository1.DeleteAll(in=,out=)` shall delete all CPER
interfaces in the `CPER` path namespace.

`xyz.openbmc_project.CPERRepository1.Store(in=ay,out=o)` shall store the client
provided CPER to the repository, and return a path representing the CPER which
implements the `xyz.openbmc_project.CPERRepository1.CPER` interface.

As new CPERs are added to the repository, the service shall implement instances
of `xyz.openbmc_project.CPERRepository1.CPER` under
`/xyz/openbmc_project/CPERRepository1/CPER/` which provide the following methods
and properties:

`xyz.openbmc_project.CPERRepository1.CPER.Read(in=,out=ay)` shall return the
CPER to the client.

`xyz.openbmc_project.CPERRepository1.CPER.Delete(in=,out=)` shall purge the CPER
from the repository.

`xyz.openbmc_project.CPERRepository1.CPER.RawSizeBytes(t,readonly)` shall
indicate the size of the raw CPER binary in bytes.

With the `xyz.openbmc_project.CPERRepository1` service in-place, the pldm,
bios-bmc-smm-error-logger, and phosphor-host-ipmid repositories will all be
enhanced to provide CPER ingress. The bmcweb repository will be enhanced to
provide a single 'CPER' LogService.

## Alternatives Considered

The first alternative is to continue with the existing attempts:

- [https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/53018]
- [https://gerrit.openbmc.org/c/openbmc/phosphor-debug-collector/+/62581]

that implement CPER ingress/egress with phosphor-debug-collector as the CPER
hosting application, and conceptualize CPER as 'Dumps'. From the perspective of
the BMC, phosphor-debug-collector _collects_ data, packages it, and makes it
available on D-Bus. It problem domain is an orchestrator of a data collection
process. From the perspective of the BMC, CPER is not collected or packaged -
this has already occurred when the BMC receives it. Nothing is orchestrated -
the BMC is simply a repository for the CPER data produced elsewhere in the
larger system.

Using file descriptors for the Read and Store D-Bus APIs was considered. This
approach was not selected because the ergonomics of a byte array are preferred
and in this case is feasible given CPER are typically less than 64KB in size and
thus can easily fit in BMC memory.

## Impacts

### API impact

The bios-bmc-smm-error-logger and pldm repositories will be migrated to the new
`xyz.openbmc_project.CPERRepository1.Store` API provided by the
`xyz.openbmc_project.CPERRepository1` service.

### Security impact

Fundamentally, `xyz.openbmc_project.CPERRepository1` is just an object store. It
should be simple for `xyz.openbmc_project.CPERRepository1` to run with minimal
(non-root) privileges.

### Documentation impact

The `xyz.openbmc_project.CPERRepository1` API will be documented in the libcper
git repository.

### Performance impact

It is assumed by this design that CPER ingress/egress is an infrequent event and
thus introduces negligible performance impact.

### Upgradability impact

Pending the implementation of this design, the PLDM type 2 CPEREvent handler in
the pldm git repository writes inbound CPERs to `/var/cper` and invokes a D-Bus
method implemented in phosphor-debug-collector. bios-bmc-smm-error-logger emits
a signal. There are no (upstream) consumers of any of these actions, thus:

- Fully upstream system implementations do not have any devices emitting PLDM
  CPEREvents (and thus have no files in /var/cper).

-or-

- Downstream implementations have patches elsewhere that manage accumulation of
  files in `/var/cper`.

Therefore, this design has no other choice but to leave compatibility concerns
to be addressed in the downstream implementations.

## Organizational

- Does this repository require a new repository? No
- Which repositories are expected to be modified to execute this design?
  - openbmc/openbmc
  - openbmc/libcper
  - openbmc/bmcweb
  - openbmc/bios-bmc-smm-error-logger
  - openbmc/pldm
  - openbmc/phosphor-host-ipmid

## Testing

`xyz.openbmc_project.CPERRepository1` will have a full suite of unit tests.
CPU-model specific error generators will be used to simulate the full path, once
this design is implemented.

[amd_ras]: https://gerrit.openbmc.org/c/openbmc/docs/+/68440
[google_ras]: https://gerrit.openbmc.org/c/openbmc/docs/+/46567
[arm_sbmr]: https://developer.arm.com/documentation/den0069/latest/
[cperevent]:
  https://www.dmtf.org/sites/default/files/PMCI_CPEREvent_Proposal_v3.pdf
[cper-records.md]:
  https://github.com/openbmc/docs/blob/master/designs/cper-records.md
[DSP0248]: https://www.dmtf.org/dsp/DSP0248
[uefi_spec]: https://uefi.org/specifications
[logentry]:
  https://github.com/DMTF/Redfish-Publications/blob/5b217908b5378b24e4f390c063427d7a707cd308/csdl/LogEntry_v1.xml#L1403
