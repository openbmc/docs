# VPD collection app on OpenBMC

Author: Santosh Puranik <santosh.puranik@in.ibm.com> <santosh.puranik>

Primary assignee: Santosh Puranik

Created: 2019-06-11

## Problem Description
On OpenBMC, Vital Product Data (VPD) collection is limited to only one or two
Field Replaceable Unit (FRU) today - one example is the BMC FRU.
The BMC also supports just one VPD format, the OpenPower VPD format.
As a part of its enterprise class servers, IBM will use the IPZ format VPD,
which the BMC currently does not support. Certain FRUs also have keyword format
VPD.

The BMC requires to read VPD for all FRUs for several reasons:

- Some of the VPD information such as FRU part number, serial number need to be
  included in the Platform Error Logs (PELs) for calling out FRUs for service.

- Several use cases on the BMC require that the applications decide on a certain
  code path based on what level of FRU is plugged in. For example, the
  application that controls voltage regulators might need to set
  different registers based on the version of the voltage regulator FRU.

- There are use cases for the BMC to send VPD data to the host
  hypervisor/operating system (over PLDM). This is mainly so that the host can
  get VPD that is not directly accessible to it.

The VPD data itself could reside on an EEPROM (typical) or could have to be
synthesized out of certain parameters of the FRU (atypical - for FRUs that do
not have an EEPROM).

This design document aims to define a high level design for VPD collection and
VPD access on OpenBMC. It does *not* cover low level API details.

## Background and References
The OpenPower VPD specification that is currently supported on OpenBMC is
described in this document:

https://www-355.ibm.com/systems/power/openpower/posting.xhtml?postingId=1D060729AC96891885257E1B0053BC95

Essentially, the VPD structure consists of key-value pairs called keywords. Each
keyword can be used to contain specific data about the FRU. For example, the
SN keyword will contain a serial number that can uniquely identify an instance
of a part. Keywords themselves can be combined or grouped into records. For
example, a single record can be used to group keywords that have similar
function, such as serial number, part number.

The keyword format VPD does not contain records, but instead has just keywords
laid out one after another.

The IPZ format is quite similar in its structure to the OpenPower format except
for the following details:

- IPZ VPD has different records and keywords.

- IPZ VPD is required to implement and validate ECC as defined in the OpenPower
  specification. The BMC code currently does not validate/use ECC although the
  specification does define it, but will need to use the ECC for IBM's
  enterprise class of servers.

- The keyword format VPD is also something that consists of key-value pairs, but
  has no concept of a record to group keywords together. The ECC for the keyword
  format VPD is simply a CRC.

For details on how the OpenBMC implements VPD collection for the BMC FRU, please
refer to openbmc/openpower-vpd-parser repo and the corresponding YAMLs/systemd
service files in the openbmc/openbmc repo.

## Requirements
The following are requirements for the VPD function on OpenBMC:

- The BMC must collect VPD for all FRUs that it has direct access to by the time
  the BMC reaches Standby (aka the Ready state, a state from where the BMC can
  begin CEC poweron). This is needed for several reasons:

  BMC applications need to be able to read VPD for FRUs to determine, for ex.,
  the hardware level of the FRU.

  Some of the VPD will need to be exchanged with the host.

  Manufacturing and Service engineers need the ability to view and program
  the FRU VPD without powering the system ON.

  Certain system level VPD is also used by applications on the BMC to determine
  the system type, model on which it is running.


- If a FRU does not have a VPD store such as an EEPROM, the BMC should be able
  to synthesize VPD for such FRUs. Details on VPD synthesis will be in its own
  design document and are not covered here.

- The BMC should be able to recollect VPD for FRUs that can be hotplugged or
  serviced when the BMC is running.

- The BMC must log errors if any of the VPD cannot be properly parsed or fails
  ECC checks.

- The BMC must create/update FRU inventory objects for all FRUs that it collects
  VPD for. The inventory D-Bus object must contain (among other details), the
  serial number, part number and CCIN (an IBM way of differentiating different
  versions of a FRU) of the FRU.

- Applications on the BMC need to have the ability to query any given VPD
  keyword for a given FRU.

- Applications also need to have the ability to update the VPD contents in a
  FRU.

- There needs to be a tool/API that allows viewing and updating VPD for any
  given FRU. This includes FRUs that the BMC does not directly collect VPD for.

## Proposed Design
This document covers the architectural, interface, and design details. It
provides recommendations for implementations, but implementation details are
outside the scope of this document.

The proposal here is to develop a standalone do-it-all VPD application on the
BMC that collects all of the VPD by BMC standby. The application has to be a
daemon that will expose a set of D-bus interfaces to:

- Collect/recollect all VPD.
- Query a given FRU for its VPD. Example read the serial number of the VRM.
- Update the VPD keyword(s) for a given FRU.
- Concurrently maintain a FRU, which will in turn perform a remove/add/replace
  operation.

The application will read a JSON config file that contains the system
blueprint (set of FRUs for which VPD is to be read, their I2C bus address, etc).
The JSON config file will contain the following details for each FRU:

- The inventory path of the FRU. This path will be the key to access any
  information for the FRU.
- The I2C bus and address of the VPD EEPROM (if applicable)
- A list of records/keywords that will need to be put into the inventory.
- Location code of the FRU. The location code will also be updated to the
  inventory.
- Whether the FRU is pluggable at BMC Standby.
- Presence detect information for the FRU.
- In case the FRU has no VPD EEPROM, a flag to indicate that the VPD needs to be
  synthesized.
- An array of the above structure for all of the FRU's children. For example,
  the entry for the motherboard will contain sub-entries for all the cards that
  plug into the motherboard.

On startup, the application will:

- Read and parse the JSON into memory.
- For each FRU, it will first check if the FRU is present. If present, it will
  read and parse the VPD by invoking the right VPD parser.
- Update the BMC Inventory with specific details of each FRU (see requirements
  section)
- Once VPD is collected for all FRUs, it will indicate that the server is ready
  to accept read/write requests by publishing the read/write service on DBUS.
- Applications can wait on this service to show up on the bus to know that all
  VPD is now collected.

### Open topics
Some open questions:

- Should the VPD data that is read be cached? How often will there be accesses?
  Will some level of caching be performed by the device driver?
- How will the app know all the sysfs paths to use for VPD collection are
  available?

## Alternatives Considered
The following alternative designs were considered:

### Build upon the current VPD collection design
Continue with the current BMC design of invoking the open power vpd parser using
a systemd service via a udev rule. The service will have be templated to allow
for multiple instances. The VPD parser will have to be updated to add support
for the new VPD formats.

- Update the BMC inventory with the FRU VPD path/FD/blob to allow applications
  to get hold of the entire VPD data.
- Separate the VPD parser as a library that be run against a VPD blob to
  read/write VPD data.

This option was rejected for the following reasons:

- The design may not scale well. If we have a large system with several VPD
  EEPROMs, this may result in a lot of processes being spawned by the udev
  rules.
- There is no easy way to group or to order the VPD collection for FRUs without
  complex systemd service file dependencies.
- Would be hard to perform conditional collection, for example, collect VPD for
  FRU y only if FRU x has a particular type.
- Would be difficult to serialize writes to the same FRU.

### Build upon the entity manager
Using the entity manager: https://github.com/openbmc/entity-manager.
The Entity manager has an application called the FruDevice, which probes
/dev/iic/ for EEPROMs, reads (IPMI format) VPD and stores it on DBUS.

The application could be enhanced to:
- Add support for other VPD formats such as the IPZ and keyword format.
- Perhaps update a different set of data into a different DBUS object, like the
  Inventory manager.
- Change the external DBUS interfaces that read/write FRU data to take an
  inventory path (instead of the I2C path, address it takes in today).

This option was rejected for the following reasons:

- We do not need the full spectrum of functions offered by the entity manager,
  that is we do not want to replace the existing inventory manager.
- The code did not appear very pluggable to add support for new VPD formats, we
  might have to end up just utilizing #ifdef's to separate functions.
- Does not have a way to determine system blueprint for FRU devices, scans the
  entire /dev/ tree to pick out EEPROMs.

## Impacts
N/A

## Testing
VPD parsing function can be tested by faking out the VPD EEPROMs as files on the
filesystem. Such testing can also ensure that the right set of VPD data makes
its way into the OpenBMC Inventory. There is also a proposal to build in a file
mode into the application. The file mode will not need real hardware to test the
code functions, but can use files on the BMC to mimic the EEPROMs.

VPD reads and writes can be tested by writing a small command line utility
that can invoke the VPD server APIs to read/write VPD.

