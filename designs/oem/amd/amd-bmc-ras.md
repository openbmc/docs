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
crash data over the vendor specific interface during fatal errors which is APML
interface in case of AMD processor family. The detailed information about the
APML is available in the AMD Preliminary Processor Programming Reference (PPR)
document.

This feature allows more efficient real-time diagnosis of hardware failures
without waiting for Boot-Error Record Table (BERT) logs in the next boot cycle.

The crash data collected may be used to triage, debug, or attempt to reproduce
the system conditions that led to the failure.

The Common Platform Error Record (CPER),as outlined in [the UEFI specification]
[uefi-specification-2.10]: https://uefi.org/specs/UEFI/2.10/ , contains vendor
specific informations dumped to the file along with the crashdata collected.

Initial proposal was submitted through the mail list discussion
<https://lists.ozlabs.org/pipermail/openbmc/2023-March/033157.html>

This design was discussed in the github technical oversight forum
<https://github.com/openbmc/technical-oversight-forum/issues/24>

## Requirements

1. Provide a mechanism to collect the AMD processor specific crashdump data from
   BMC. APML interface is used for collecting Host processor specific data.
2. Provide a mechanism to format crash data as per the UEFI specification.

3. Provide an additional AMD processor specific interface extension that allows
   the users to get/set the configuration parameters.

## Proposed Design

When one or more processors register a fatal error condition, then an interrupt
is generated to the host processor.

The host processor in the failed state asserts the APML_ALERT# signal to
indicate to the BMC that a fatal hang has occurred.

BMC RAS application listens on the APML_ALERT# event.

Upon detection of FATAL error event, BMC will check the RAS status register of
the host processor to see if the assertion is due to the fatal error.

Upon fatal error, BMC will attempt to harvest crash data via the APML interface
(mailbox) from all processors. All Valid MCA banks found after crash/Syncflood
is collected via apml.

BMC will generate a single raw crashdump record and saves it in non-volatile
location.

As per the BMC policy configuration, BMC initiates a system reset to recover the
system from the fatal error condition.

The generated crashdump record will be in Common Platform Error Record (CPER)
format.

Application has configurable number of records (N).If the number of records
exceed N, the records are rotated.

Crashdump records saved in the non-volatile loation can be retrieved via the
redfish interface.

### AMD BMC RAS application design diagram

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

### High level architectural flow

```ascii
    +--------------------------------------+
    |    +-------------------------+       |
    |    |Crashdump service started|       |
    |    +---------+---------------+       |
    |              |                       |
    |         Fatal Error Detected         |AMD RAS
    |              v                       |Application
    |     +--------+-------------+         |
    |     |CPER File Generated   |         |
    |     +--------+-------------+         |
    |              |                       |
    | +------------+---------------------+ |
    | |  CreateDump() API Called with    | |
    | |  CPER File path and FaultLogType | |
    | |     for the D-BUS object         | |
    | | xyz/openbmc_project/dump/faultlog| |
    | |                                  | |
    | +----------------------------------+ |
    |                                      |
    |                                      |
    |                                      |
    +---------------+----------------------+
                    |
               D-Bus Call
                    +
   +----------------+----------------------+
   |    Creates new Faultlog Dump Entry    |
   |  D-BUS object for the newly generated |phosphor
   |     CPER record under the obj path    +-debug-
   |xyz/openbmc_project/dump/faultlog/entry|collector
   +----------------+----------------------+
                    v
  +-----------------+------------------------+
  | D-BUS objects created for the CPER record|
  +-----------------+------------------------+
                    ^
+-------------------+----------------------------+
|     Reuse existing Redfish interface           |
| /redfish/v1/Systems/system/LogServices/FaultLog|bmcweb
|     to List/Delete/Download the CPER files     |
|                                                |
+------------------------------------------------+
```

BMC ras application is started by the systemd service com.amd.Crashdump

The proposal focuses on re-using the existing [phosphor-debug-collector]
https://github.com/openbmc/phosphor-debug-collector) to host the CPER file
informations to the dbus interface once the Crashdump files are created by the
amd-ras application. A new property "FaultLogType" will be added to the
interface xyz.openbmc_project.Dump.Create. The RAS application invokes the
CreateDump() API which is associated with the existing d-bus object path
(xyz/openbmc_project/dump/faultlog). The FaultLogType and CPER file path
informations is passed to the CreateDump() API to create new fautlog entry d-bus
object path.

The bmcweb will serve as the Redfish webserver that allows external users to
download the CPER files which are currently available in the system as an
attachment using AdditionalDataURI. The existing redfish interface
/redfish/v1/Systems/system/LogServices/FaultLog is used to list/delete/download
the files. The DiagnosticDataType is populated as "CPER" as defined in the
LogEntry redfish schema.

A dbus interface com.amd.Crashdump.Configuration is also maintained which has
the config file info.

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
mechanisms. APML interface is the AMD standard to perform CPU/SOC management.
AMD utilizes the APML interface for data collection and monitoring. The MCA dump
information that is required for CPER generation is specific to AMD processors.
They collect AMD processor-specific information such as Microcode, CPUID, PPIN,
APIC ID, Platform ID , etc., to generate a detailed report for local analysis.
Given these considerations, AMD would like to use a new repository to host
AMD-specific error monitoring and data collection applications.

In the initial design, the persistent memory location /var/lib/bmc-ras was
chosen to store the CPER files. However, as the CPER files are hosted to D-Bus
interface using the existing Faultlog Dump Entry D-Bus object, it was deemed
more appropriate to store them in /var/lib/phosphor-debug-collector/faultlog/
<dump id>/CPER_FILE.

## Impacts

Since crash dump data is as per common platform error record (CPER) format
defined in UEFI specification, No security impact.

This implementation takes off the host processor workload by offloading the data
collection process to BMC and thereby improving the system performance as a
whole.

## Organizational

This design will require a new repository.

Initial maintainers : github id - ojayanth

Changes required in phosphor-dbus-interfaces repo where a new configuration
interface xyz.openbmc_project.Crashdump.Configuration will store the user
configured values. An additional new property "FaultLogType" will be added to
the interface xyz.openbmc_project.Dump.Create.

Minimal changes required in phosphor-debug-collector to parse the CPER file path
in FaultLog CreateDump() API.

Minimal changes required in bmcweb to download the CPER files from the redfish
interface /redfish/v1/Systems/system/LogServices/FaultLog and populate the
DiagnosticDataType as "CPER".

## Testing

Fatal errors can be simulated aritifically using the error injection mechanism
and the error conditions will be logged by the bmc ras module. An analyzer tool
can be used to decode the datas available in the CPER record to get a detailed
analysis about the errors.

Unit testing code will be supported as OpenBMC standards. There is no
significant impact expected with regards to CI testing.
