### ECC Error SEL for BMC

Author: Will Liang

Created: 2019-02-26

#### Problem Description

The IPMI SELs only define memory Error Correction Code (ECC) errors for host
memory rather than BMC.

The aim of this proposal is to record ECC events from the BMC in the IPMI System
Event Log (SEL). Whenever ECC occurs, the BMC generates an event with the
appropriate information and adds it to the SEL.

#### Background and References

The IPMI specification defines memory system event log about ECC/other
correctable or ECC/other uncorrectable and whether ECC/other correctable memory
error logging limits are reached.[1]. The BMC ECC SEL will follow IPMI SEL
format and creates BMC memory ECC event log.

OpenBMC currently support for generating SEL entries based on parsing the D-Bus
event log. It does not yet support the BMC ECC SEL feature in OpenBMC project.
Therefore, the memory ECC information will be registered to D-Bus and generate
memory ECC SEL as well.

[[1]Intelligent Platform Management Interface Specification v2.0 rev 1.1, section 41](https://www.intel.com/content/www/us/en/servers/ipmi/ipmi-second-gen-interface-spec-v2-rev1-1.html)

#### Requirements

Currently, the OpenBMC project does not support ECC event logs in D-Bus because
there is no relevant ECC information in the OpenBMC D-Bus architecture. The new
ECC D-Bus information will be added to the OpenBMC project and an ECC monitor
service will be created to fetch the ECC count (ce_count/ue_count) from the EDAC
driver. And make sure the EDAC driver must be loaded and ECC/other correctable
or ECC/other uncorrectable counts need to be obtained from the EDAC driver.

#### Proposed Design

ECC-enabled memory controllers can detect and correct errors in operating
systems (such as certain versions of Linux, macOS, and Windows) that allow
detection and correction of memory errors, which helps identify problems before
they become catastrophic faulty memory module.

Many ECC memory systems use an "external" EDAC between the CPU and the memory to
fix memory error. Most host integrate EDAC into the CPU's integrated memory
controller.

According to Section 42.2 of the IPMI specification, Table 42 [2], these SEL
sensor types will be defined as `Memory` and `Event Data 3` field can be used to
provide an event extension. Therefore, the BMC ECC event sets "Event Data 3"
with the value FEh to identify the BMC ECC error.

[[2] Intelligent Platform Management Interface Specification v2.0 rev 1.1, section 42.2](https://www.intel.com/content/www/us/en/servers/ipmi/ipmi-second-gen-interface-spec-v2-rev1-1.html)

The main purpose of this function is to provide the BMC with the ability to
record ECC error SELs.

There are two new applications for this design:

- poll the ECC error count
- create the ECC SEL

It also devised a mechanism to limit the "maximum number" of logs to avoid
creating a large number of correctable ECC logs. When the `maximum quantity` is
reached, the ECC service will stop to record the ECC log. The `maximum quantity`
(default:100) is saved in the configuration file, and the user can modify the
value if necessary.

##### phosphor-ecc.service

This will always run the application and look up the ECC error count every
second after service is started. On first start, it resets all correctable ECC
counts and uncorrectable ECC counts in the EDAC driver.

It also provide the following path on D-Bus:

- bus name : `xyz.openbmc_project.Memory.ECC`
- object path : `/xyz/openbmc_project/metrics/memory/BmcECC`
- interface : `xyz.openbmc_project.Memory.MemoryECC`

The interface with the following properties: | Property | Type | Description | |
-------- | ---- | ----------- | | isLoggingLimitReached | bool | ECC logging
reach limits| | ceCount| int64 | correctable ECC events | | ueCount| int64 |
uncorrectable ECC events | | state| string | bmc ECC event state |

The error types for `xyz::openbmc_project::Memory::Ecc::Error::ceCount` and
`ueCount` and `isLoggingLimitReached` will be created which generated the error
type for the ECC logs.

##### Create the ECC SEL

Use the `phosphor-sel-logger` package to record the following logs in BMC SEL
format.

- correctable ECC log : when fetching the `ce_count` from EDAC driver parameter
  and the count exceeds previous count.
- uncorrectable ECC log : when fetching the `ue_count` from EDAC driver
  parameter and the count exceeds previous count.
- logging limit reached log : When the correctable ECC log reaches the
  `maximum quantity`.

#### Alternatives Considered

Another consideration is that there is no stopping the recording of the ECC
logging mechanism. When the checks `ce_count` and value exceeds the previous
value, it will record the ECC log. But this will encounter a lot of ECC logs,
and BMC memory will also be occupied.

#### Impacts

This application implementation only needs to make some changes when creating
the event log, so it has minimal impact on the rest of the system.

#### Testing

Depending on the platform hardware design, this test requires an ECC driver to
make fake ECC errors and then check the scenario is good.
