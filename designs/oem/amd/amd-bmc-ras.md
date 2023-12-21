# AMD BMC RAS Design

Author: Abinaya Dhandapani (<Abinaya.Dhandapani@amd.com>) , Supreeth Venkatesh
(<Supreeth.Venkatesh@amd.com>)

Other contributors:

Created:Dec 21, 2023

## Problem Description

Collection of crash data at runtime in a processor agnostic manner presents a
challenge and an opportunity to standardize.

## Glossary

- **Common Platform Error Record (CPER)**: Format to represent platform hardware
  errors which is defined in the UEFI specification.
- **Reliability, availability and serviceability (RAS)**: Computer hardware
  engineering term that describes the robustness of the system.

- **Unified Extensible Firmware Interface(UEFI)**: Specification that defines
  the architecture of the platform firmware.

- **Advanced Platform Management Link (APML)**: SMBus v2.0 compatible 2-wire
  processor slave interface.

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
[via the APML interface (mailbox) in case of AMD processor family].

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

### Configuring RAS config file

A configuration file is created in the /var/lib/bmc-ras application which allows
the user to configure the below values.

“Max data collection retries” – Maximum number of retries if the crashdata
collection fails.

“Harvest PPIN enable” – If enabled, harvest PPIN and dump into the CPER file.

“Harvest Microcode enable” – If enabled, harvest microcode and dump into the
CPER file.

“System recovery” – Warm reset or cold reset or no reset as per User’s
requirement.

The configuration file values can be viewed and changed via redfish GET/SET
command.

The redfish URI to configure the BMC config file:

https://<BMC-IP>/redfish/v1/Systems/system/LogServices/Crashdump/
Actions/Oem/Crashdump.Configuration

Sample redfish output:

curl -s -k -u <user>:<password> -H"Content-type: application/json" -X GET
<https://<BMC-IP>/redfish/v1/Systems/system/LogServices/Crashdump/>
Actions/Oem/Crashdump.Configuration {

"@odata.id": "/redfish/v1/Systems/system/LogServices/Crashdump/Actions/Oem/
Crashdump.Configuration",

"@odata.type": "#LogService.v1_2_0.LogService",

"maxDataCollectionRetries": 10,

"harvestPpinEnable": true,

"systemRecovery": 2, // 0 - warm reset, 1 - cold reset, 2 - no reset.

"uCodeVersionEnable": true

}

Dbus interface for crashdump service:

BMC ras application is started by the systemd service
xyz.openbmc_project.crashdump

A dbus interface xyz.openbmc_project.Crashdump.Configuration is maintained which
has the config file info and the file handle for the CPER files currently in the
system via xyz.openbmc_project.Dump.Entry interface Which can be downloaded from
the redfish interface.

## Alternatives Considered

In-band mechanisms using System Management Mode (SMM) exists.

However, out of band method to gather RAS data is processor specific.

## Impacts

Since crash dump data is as per common platform error record (CPER) format as
per UEFI specification [https://uefi.org/specs/UEFI/2.10/],No security impact.

This implementation takes off the host processor workload by offloading the data
collection process to BMC and thereby improving the system performance as a
whole.

## Organizational

This design will require a new repository.

Initial maintainers : github id - abinayaddhandapani, supven01

Changes required in phosphor-dbus-interfaces repo where a new configuration
interfaces xyz.openbmc_project.Crashdump.Configuration will store the user
configured values.

Changes required in bmcweb to allow get and set operations for the configuration
interface.

## Testing

Fatal errors can be simulated aritifically using the error injection mechanism
and the error conditions will be logged by the bmc ras module. An analyzer tool
can be used to decode the datas available in the CPER record to get a detailed
analysis about the errors.
