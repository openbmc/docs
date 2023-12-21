# AMD BMC RAS Design

Author: Abinaya Dhandapani (<Abinaya.Dhandapani@amd.com>) , Supreeth Venkatesh
(<Supreeth.Venkatesh@amd.com>)

Other contributors:

Created:Dec 21, 2023

## Problem Description

Collection of crash data at runtime in a processor agnostic manner presents a
challenge and an opportunity to standardize the crashdump record.

## Glossary

- **Common Platform Error Record (CPER)**: Format to represent platform hardware
  errors which is defined in the UEFI specification.

- **Reliability, availability and serviceability (RAS)**: Computer hardware
  engineering term that describes the robustness of the system.

- **Unified Extensible Firmware Interface(UEFI)**: Specification that defines
  the architecture of the platform firmware.

- **Advanced Platform Management Link (APML)**: SMBus v2.0 compatible 2-wire
  processor slave interface.

- **Machine Check Architecture (MCA)**: defines facilities by which processor
  and system hardware errors are logged and reported to system software.

- **Protected Processor Inventory Number (PPIN)**: Unique 64-bit number assigned
  to the processor.

- **Boot-Error Record Table (BERT)**: Record and report error informations to
  the operating system.

## Background and References

Host processors allow an external management controller (i.e BMC) to harvest CPU
crash data over the vendor specific interface during fatal errors [APML
interface in case of AMD processor family].

This feature allows more efficient real-time diagnosis of hardware failures
without waiting for Boot-Error Record Table (BERT) logs in the next boot cycle.

The crash data collected may be used to triage, debug, or attempt to reproduce
the system conditions that led to the failure.

The CPER record contains vendor specific informations dumped to the file along
with the crashdata collected.

Initial proposal was submitted through the mail list discussion
<https://lists.ozlabs.org/pipermail/openbmc/2023-March/033157.html>

This design was discussed in the github technical oversight forum
<https://github.com/openbmc/technical-oversight-forum/issues/24>

## Requirements

1. Provide a mechanism to collect the AMD processor specific crashdump data from
   BMC. APML interface is used for collecting Host processor specific data.
2. Provide a mechanism to format crash data as per the UEFI specification
   [https://uefi.org/specs/UEFI/2.10/].

3. Provide an additional AMD processor specific interface extension that allows
   the users to get/set the configuration parameters.

## Proposed Design

When one or more processors register a fatal error condition, then an interrupt
is generated to the host processor.

The host processor in the failed state asserts the signal to indicate to the BMC
that a fatal hang has occurred [APML_ALERT# in case of AMD processor family].

BMC RAS application listens on the event [APML_ALERT# in case of AMD processor
family ].

Upon detection of FATAL error event, BMC will check the status register of the
host processor [implementation defined method] to see if the assertion is due to
the fatal error.

Upon fatal error, BMC will attempt to harvest crash data from all processors.
[via the APML interface (mailbox) in case of AMD processor family]. All Valid
MCA banks found after crash/Syncflood is collected via apml.

BMC will generate a single raw crashdump record and saves it in non-volatile
location /var/lib/bmc-ras.

As per the BMC policy configuration, BMC initiates a system reset to recover the
system from the fatal error condition.

The generated crashdump record will be in Common Platform Error Record (CPER)
format as defined in the UEFI specification [https://uefi.org/specs/UEFI/2.10/].

Application has configurable number of records (N).If the number of records
exceed N, the records are rotated.

Crashdump records saved in the /var/lib/bmc-ras which can be retrieved via
redfish interface.

### AMD BMC RAS application - High Level architectural diagram

```ascii
+----------+             +------------+                +--------+  +---------+
|  SMU/    |             |   APML     |                | BMC    |  | Redfish |
|  PMFW    |             |  LIBRARY   |                |        |  | Client  |
+---+------+             +-----+------+                +---+----+  +-----+---+
    |                          |                           |             |
    |                          |                          +++            |
   +++                         |                          | +<--System +-+
   | +---+Fatal error          |                          | |   Recovery |
   | |   |detected             |                          | |   Policy   |
   | +^--+                     |                          | |Configuration
   | +-------------------- APML ALERT-------------------->+ |            +
   | |                         |                          | |            |
   | |   SBRMI Mailbox        +++       Get RAS Status    | |            |
   | +<-----Read+-------------+ +----------Register-------+ |            |
   | |                        | |                         | |            |
   | +------RAS Status+-------+ +--------RAS Status------>+ +---+        |
   | |                        | |                         | |If RAS Fatal|
   | |                        | |                         | |  Error     |
   | <-----Get # of Valid +---+ +<---Get # of Valid MCA---+ <---+        |
   | |      MCA banks         | |          Banks          | |            |
   | |                        | |                         | |            |
   | |                        | |                         | |            |
   | +<---Get All Valid MCA +-+ +<--Get all Valid MCA-----+ |            |
   | |       MSR Dumps        | |    MSR Dumps            | |            |
   | |                        | |                         | |            |
   | +--Valid MCA MSR Dumps+--+ +----Valid MCA MSR Dumps--> +---+        |
   | |                        | | +--------+              | |Generate    |
   | |                        | | | FPGA   +--+           | |  CPER      |
   | |                        | | |[Cold   |  |           | +<--+        |
   | |                        | | | Reset] |  |           | |            |
   | |                        | | +--------+  +-System----+ |            |
   | |                        | |             |  Recovery | |            |
   | |                        | |             |           | |            |
   | |                        | |             |           | |            |
   | +<---System Recovery+----+ +<--System----+           | |            |
   | |      [Warm]            | |  Recovery[Warm]         | |            |
   +-+                        +-+                         +-+            +
```

BMC ras application is started by the systemd service com.amd.Crashdump

The proposal focuses on re-using the existing [phosphor-debug-collector]
(https://github.com/openbmc/phosphor-debug-collector) to host the CPER file
informations to the dbus interface once the Crashdump files are created by the
amd-ras application. The Dbus object also populates FaultDataType of the
interface xyz.openbmc_project.Dump.Entry.FaultLog with "CPER" value.

A dbus interface com.amd.Crashdump.Configuration is also maintained which has
the config file info.

bmcweb will serve as the Redfish webserver that allows external users to
download the CPER files which are currently available in the system as an
attachment using AdditionalDataURI.

### Configuring RAS config file

A RAS configuration file, which customizes the key properties of the error
handling functionality, is maintained in the application.

An OEM Redfish interface is required which should allow the user to modify the
properties of RAS config file. The design document will be updated with the OEM
Redfish interface for configuration once the proposal is submitted to the
Redfish Forum.

## Alternatives Considered

In-band mechanisms using System Management Mode (SMM) exists.

However, out of band method to gather RAS data is processor specific.

The existing OpenBMC host error monitoring and data collection applications,
found in the host-error-monitor and openpower-hw-diags, are more processor
specific(x86/POWER) and are not well-suited for AMD processor error monitoring
mechanisms. AMD utilizes the APML interfaces for data collection and monitoring.
The MCA dump information that is required for CPER generation is specific to AMD
processors. They collect AMD processor-specific information such as Microcode,
CPUID, PPIN, APIC ID, Platform ID , etc., to generate a detailed report for
local analysis. Given these considerations, AMD would like to use a new
repository to host AMD-specific error monitoring and data collection
applications.

## Impacts

Since crash dump data is as per common platform error record (CPER) format as
per UEFI specification[https://uefi.org/specs/UEFI/2.10/], No security impact.

This implementation takes off the host processor workload by offloading the data
collection process to BMC and thereby improving the system performance as a
whole.

## Organizational

This design will require a new repository.

Initial maintainers : github id - ojayanth

Changes required in phosphor-dbus-interfaces repo where a new configuration
interfaces xyz.openbmc_project.Crashdump.Configuration will store the user
configured values.

Changes required in phosphor-debug-collector to create D-bus Objects for the
Crashdump records.

## Testing

Fatal errors can be simulated aritifically using the error injection mechanism
and the error conditions will be logged by the bmc ras module. An analyzer tool
can be used to decode the datas available in the CPER record to get a detailed
analysis about the errors.

Unit testing code will be supported as OpenBMC standards. There is no
significant impact expected with regards to CI testing.
