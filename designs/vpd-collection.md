# VPD collection app on OpenBMC

Author: Santosh Puranik <santosh.puranik@in.ibm.com> <santosh.puranik>

Primary assignee: Santosh Puranik

Created: 2019-06-11

## Problem Description
On OpenBMC, Vital Product Data (VPD) collection is limited to just one
Field Replaceable Unit (FRU) today - which is the BMC FRU.
The BMC also supports just one VPD format, the OpenPower VPD format.
As a part of it's enterprise class servers, IBM will use the IPZ format VPD,
which the BMC currently does not support.

The BMC requires to read VPD for all FRUs for several reasons:

- Some of the VPD information such as FRU part number, serial number need to be
  included in the Platform Error Logs (PELs) for calling out FRUs for service.

- Several use cases on the BMC require that the applications decide on a certain
  code path based on what level of FRU is plugged in. For example, the
  application that controls voltage regulators, might have needs to set
  different registers based on the version of the voltage regulator FRU.

- There are use cases for the BMC to send VPD data to the host
  hypervisor/operating system (over PLDM). This is mainly so that the host can
  get VPD that is not directly accessible to it.

The VPD data itself could reside on a EEPROM (typical) or could have to be
synthesized out of certain parameters of the FRU (atypical - for FRUs that do
not have an EEPROM).

This design document aims to define a high level design for VPD collection and
VPD access on OpenBMC. It does *not* cover low level API details.

## Background and References
The OpenPower VPD specification that is currently supported on OpenBMC is
described in this document:

https://w3-connections.ibm.com/forums/html/topic?id=282491df-07c0-4eec-a5f6-05716de62f19

The IPZ format is quite similar in it's structure to the OpenPower format except
for the following details:

- IPZ VPD has different records and keywords.

- IPZ VPD is required to implement and validate ECC as defined in the OpenPower
  specification.

For details on how the OpenBMC implements VPD collection for the BMC FRU, please
refer to openbmc/openpower-vpd-parser repo and the corresponding YAMLs/systemd
service files in the openbmc/openbmc repo.

## Requirements
The requirements listed here do not cover
The following are requirements for the VPD function on OpenBMC:

- The BMC must collect VPD for all FRUs that it has direct access to by the time
  the BMC reaches standby or Ready state.

- If a FRU does not have a VPD store such as an EEPROM, the BMC should be able
  to synthesize VPD for such FRUs.

- The BMC should be able to recollect VPD for FRUs that can be hotplugged or
  serviced when the BMC is running.

- The BMC must log errors if any of the VPD cannot be properly parsed or fails
  ECC checks.

- The BMC must create/update FRU inventory objects for all FRUs that it collects
  VPD for. The inventory D-Bus object must contain (among other details), the
  serial number, part number and CCIN (an IBM way of differentiating different
  versions of a FRU) of the FRU.

- Applications on the BMC need to have the ability to query any given keyword
  for a given FRU.

- Applications also need to have the ability to update the VPD contents in a
  FRU.

- There needs to be a tool/API that allows viewing and updating VPD for any
  given FRU. This includes FRUs that the BMC does not directly collect VPD for.

## Proposed Design
This document covers the architectural, interface, and design details. It
provides recommendations for implementations, but implementation details are
outside the scope of this document.

Two alternatives designs are presented here. Once we decide on one of them (or
something else), the rejected designs will be moved to the "Alternatives"
section below.

### Option 1 - Standalone VPD application.
The proposal here is to develop a standalone do-it-all VPD application on the
BMC that collects all of the VPD by BMC standby. The application has to be a
daemon that would expose a set of D-bus interfaces to:

- Collect/recollect all VPD.
- Query a given FRU for it's VPD. Example read the serial number of the VRM.
- Update the VPD keyword(s) for a given FRU.

The application would read a JSON/YAML config file that contains the system
blueprint (set of FRUs for which VPD is to be read, their sysfs paths, etc).

The application would have the smarts to synthesize VPD


### Option 2 - Modify the op-vpd-parser application
The proposal here is to continue with the current BMC design of invoking the
open power vpd parser using a systemd service via a udev rule. The service would
have be templated to allow for multiple instances.

- Update the BMC inventory with the FRU VPD path/FD/blob (discuss which) to
  allow applications to get hold of the entire VPD data.
- Separate the VPD parser as a library that be run against a VPD blob to
  read/write VPD data.

### Design points common to both the options
There are certain design points that are common to both the options discussed
above:

- Update the BMC Inventory with specific details of each FRU (see requirements
  section)
- Enhance the config YAMLs to cover new FRU types and new VPD keywords.
- Enhance/Update the recipes/config YAMLs to provide sysfs paths for
- Enhance the open power VPD parser to support the IPZ VPD format.

### Open topics
Some open questions:

- Should the VPD data that is read be cached? How often would there be accesses?
- How do we set up a "sync point" with option 2? For ex. stop until all VPD is
  collected.
- With option 2, how do we handle VPD writes?

## Alternatives Considered
Leaving this blank for now, since the proposed design section covers both
alternatives. Once the right design choice is made, this section will be updated
with the designs were rejected.

## Impacts
TBD

## Testing
VPD parsing function can be tested by faking out the VPD EEPROMs as files on the
filesystem. Such testing can also ensure that the right set of VPD data makes
it's way into the OpenBMC Inventory.

VPD reads and writes can be tested by writing a small tool that can invoke the
VPD parser library to parse and display data.

