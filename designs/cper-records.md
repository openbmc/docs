# CPER records - CPER

Author: Ed Tanous - edtanous

Created: 5-22-2024

## Problem Description

ARM CPUs expose a managability interface refered to as CPER records. A user
outside of the BMC would like to read these records in a decoded state, rather
than as a raw package.

## Background and References

CPER stands for Common Platform Error Record and is defined as an industry
standard in the [UEFI Specification][uefi_spec], with CPU ISA specific
definitions for Arm64, x64, Itanium, and IA32. Within this document there are
several architecture definitions for BMC, including section C.2.2 RAS IPMI
Message Format, and IPMI based RAS Event Receiver.

In Redfish specification drop 2021.3, Redfish added support for CPER records
into the [LogEntry resource][logentry]. These expose a section by section
decoded CPER instance. In addition to Redfish, there is a proposed DMTF
interface for sending CPER log events to the BMC using [MCTP/PLDM][cperevent],
which is proposed to be added in a future version of DMTF [DSP0248].

ARM has developed a reference library for decoding CPER records that does not
have a contribution mechanism, releases, or maintenance, and they have made
[clear][cper_examples] that they would like OpenBMC to be the long-term
custodian of this library.

This library hosts the meson-dev branch, which was added for the purpose of this
design, and passes the OpenBMC CI tests currently. This is the proposed branch
that will be pushed to openbmc/libcper, if approved.

## Requirements

- A BMC should be able to decode binary CPER records originated from a CPER
  compatible CPU.

- BMC should be able to recieve and decode CPER records from an ARM CPU per the
  [CPER specification][arm_sbr].

- A BIOS/EDK2 build should be able to share decoding code with OpenBMC, to the
  end that added records do not require manual effort to implement in each
  codebase.

- A CPU vendor should be able to add support for CPER extensions that OpenBMC
  will now be able to decode, without impacting users of other vendors, as
  promised in the CPER specification.

- CPER decoder should allow decoding of multiple CPU complexes.

## Proposed Design

While this design fits into a much more elaborate design alluded to in the
aformentioned ARM document, this document only requests the first step, creating
a shared library implementation within the OpenBMC organization that can be
built upon over time, but might not implement the complete implementation at
this time. It is expected that the ubiquity of CPER records in the BMC ecosystem
justifies the creation of the repository, even if the initial implementation
might not meet all design goals for all contributors, having a common
contribution model, CI testing, and license is beneficial as a whole.

Future design docs (or amendments to this design) will iterate on implementing
more of the design referenced in this [CPER specification][arm_sbr], for common
ARM platforms, but getting the custody transferred for the libcper repo, getting
the quality up to standards is the initial goal of this design.

## Alternatives Considered

Rewrite libcper decoding from a new design point. While this is certainly
possible given the small size of the libcper repo as it exists today, it would
bifurcate already existing implementations of the decode.

## Impacts

New repo will be created within the organization. New recipe will be added to
OpenBMC.

### Organizational

- Does this repository require a new repository? Yes
- Who will be the initial maintainer(s) of this repository? Ed Tanous
- Which repositories are expected to be modified to execute this design?

  - openbmc/openbmc
  - openbmc/libcper
  - openbmc/phosphor-logging

#### Potentionally in the future

- openbmc/phosphor-dump-collector
- openbmc/bmcweb

## Testing

Unit tests are already present in the repo to verify basic functionality.
CPU-model specific error generators will be used to simulate the full path, once
design is complete.

[arm_sbr]: https://developer.arm.com/documentation/den0069/latest/
[uefi_spec]: https://uefi.org/specifications
[logentry]:
  https://github.com/DMTF/Redfish-Publications/blob/5b217908b5378b24e4f390c063427d7a707cd308/csdl/LogEntry_v1.xml#L1403
[cperevent]:
  https://www.dmtf.org/sites/default/files/PMCI_CPEREvent_Proposal_v3.pdf
[DSP0248]: https://www.dmtf.org/dsp/DSP0248
[cper_examples]:
  https://gitlab.arm.com/server_management/libcper/-/blob/meson-dev/README.md?ref_type=heads#usage-examples
