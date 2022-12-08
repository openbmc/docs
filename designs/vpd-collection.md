# VPD collection app on OpenBMC

Author: Santosh Puranik <santosh.puranik@in.ibm.com> <santosh.puranik>

Created: 2019-06-11

## Problem Description

On OpenBMC, Vital Product Data (VPD) collection is limited to only one or two
Field Replaceable Units (FRUs) today - one example is the BMC FRU. On OpenPower
systems, the BMC also supports just one VPD format, the [OpenPower VPD] [1]
format. As a part of its enterprise class servers, IBM will use the IPZ format
VPD, which the BMC currently does not support. Certain FRUs also have keyword
format VPD.

The BMC requires to read VPD for all FRUs for several reasons:

- Some of the VPD information such as FRU part number, serial number need to be
  included in the Platform Error Logs (PELs) for calling out FRUs for service.

- Several use cases on the BMC require that the applications decide on a certain
  code path based on what level of FRU is plugged in. For example, the
  application that controls voltage regulators might need to set different
  registers based on the version of the voltage regulator FRU.

- There are use cases for the BMC to send VPD data to the host
  hypervisor/operating system (over PLDM). This is mainly so that the host can
  get VPD that is not directly accessible to it.

The VPD data itself may reside on an EEPROM (typical) or may be synthesized out
of certain parameters of the FRU (atypical - for FRUs that do not have an
EEPROM).

This design document aims to define a high level design for VPD collection and
VPD access on OpenBMC. It does _not_ cover low level API details.

## Background and References

Essentially, the IPZ VPD structure consists of key-value pairs called keywords.
Each keyword can be used to contain specific data about the FRU. For example,
the SN keyword will contain a serial number that can uniquely identify an
instance of a part. Keywords themselves can be combined or grouped into records.
For example, a single record can be used to group keywords that have similar
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

## Requirements

The following are requirements for the VPD function on OpenBMC:

- The BMC must collect VPD for all FRUs that it has direct access to by the time
  the BMC reaches Standby (aka the Ready state, a state from where the BMC can
  begin CEC poweron). This is needed for several reasons:

  BMC applications need to be able to read VPD for FRUs to determine, for ex.,
  the hardware level of the FRU.

  Some of the VPD will need to be exchanged with the host.

  Manufacturing and Service engineers need the ability to view and program the
  FRU VPD without powering the system ON.

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
  FRU. For example, the BMC needs to have the ability to update the system VPD
  in scenarios where the FRU containing the system VPD is repaired in the field.

- There needs to be a tool/API that allows viewing and updating VPD for any
  given FRU. This includes FRUs that the BMC does not directly collect VPD for
  (such as FRUs that can be accessed both by the BMC and the host, but the host
  collects VPD for)

## Proposed Design

This document covers the architectural, interface, and design details. It
provides recommendations for implementations, but implementation details are
outside the scope of this document.

The proposal here is to build upon the existing VPD collection design used by
open power. The current implementation consists of the following entities:

- [op-vpd-parser] [2] service, which parses the contents of an EEPROM containing
  VPD in the OpenPower VPD format.

- A udev [rule] [3] that is used by systemd to launch the above service as
  EEPROM devices are connected.

- A set of config [files] [4] that describe the inventory objects and D-Bus
  properties to update for a given FRU.

In order to meet the requirements noted in the previous section, the following
updates will be made:

- Two new services will be created to handle the new VPD formats. ipz-vpd-parser
  and a keyword-vpd-parser. These services shall be templated to allow for
  multiple instances to be run.

- Another service will be created to update the inventory with location code
  information. Since the location code expansion comes from the system VPD, this
  service can only be launched after the system VPD (stored on the backplane) is
  collected.

- There will be one udev rule per EEPROM on the system from which the BMC has to
  collect VPD. We could also have just a single rule, but that would mean we
  would have to filter out non-VPD EEPROMs somehow.

- Each udev rule will be setup to launch an instance of one of the VPD parser
  services (The format of the VPD in any given EEPROM are known at build time as
  they are system specific)

- The service (one instance of ipz-vpd-parser or keyword-vpd-parser), when
  launched, will read the EEPROM, parse its contents and use config files to
  determine what VPD contents to store in the inventory.

- The service will update the inventory D-Bus object with the contents of the
  VPD in the following format: There will be one interface per record (ex, VINI
  record) which will have each keyword as a property (ex, FN, PN). This will
  allow us to support multiple records that can have the same keyword and will
  also serve as means to logically group keywords in the inventory, quite
  similar to how they are grouped in the actual VPD. For example (some names
  here are made up, but they help illustrate the point), for the VINI record
  containing keywords SN, FN and CCIN, the representation in D-Bus would look
  like:

```
Interface: com.ibm.ipzvpd.VINI
Properties:
    SN
    FN
    CCIN
```

- In case of keyword format VPD, all keywords shall be placed as properties
  under a single interface.

- The parser services will not format or transform the data in VPD in any way
  before updating the properties noted above, the properties shall be stored as
  byte arrays. Note, however, that the services will continue updating the
  commonly used FRU properties such as SerialNumber, PartNumber as strings, just
  as the openpower-vpd-parser does.

- To handle VPD writes, another systemd service will be launched once all the
  VPD read services have completed. This service shall be a daemon that will
  manage parallel writes to EEPROMs. The VPD writer service will expose D-bus
  interfaces to update VPD for a FRU given its inventory path.

- Generation of the udev rules and configs shall be layered such that they can
  be tweaked on a per-system basis.

### Open topics

Some open questions:

- Some more thought is needed on how concurrent maintenance (replacing a FRU
  when the host is up and running) will be handled. That will be presented in
  its own design document.

## Alternatives Considered

The following alternative designs were considered:

### Write a standalone VPD server app

One option considered was to develop a standalone, do-it-all VPD application on
the BMC that collects all of the VPD by BMC standby. The application has to be a
daemon that will expose a set of D-bus interfaces to:

- Collect/recollect all VPD.
- Query a given FRU for its VPD. Example read the serial number of the VRM.
- Update the VPD keyword(s) for a given FRU.
- Concurrently maintain a FRU, which will in turn perform a remove/add/replace
  operation.

The application would be driven off of a configuration file that lists the FRUs
available for a given system, their I2C bus address, slave address etc.

This option was rejected for the following reasons:

- Design would make the application very specific to a given system or a set of
  VPD formats. Although the application could be written in a way that allows
  plugging in support for different VPD formats, it was deemed that the current
  approach of small applications that target a very specific requirement is
  better.
- The design does not leverage upon the layered design approach that the chosen
  option allows us to do.

### Build upon the entity manager

Using the entity manager: https://github.com/openbmc/entity-manager. The Entity
manager has an application called the FruDevice, which probes /dev/i2c/ for
EEPROMs, reads (IPMI format) VPD and stores it on DBUS.

The application could be enhanced to:

- Add support for other VPD formats such as the IPZ and keyword format.
- Perhaps update a different set of data into a different DBUS object, like the
  Inventory manager.
- Change the external DBUS interfaces that read/write FRU data to take an
  inventory path (instead of the I2C path, address it takes in today).

This option was rejected for the following reasons:

- We do not need the full spectrum of functions offered by the entity manager,
  that is we do not want to replace the existing inventory manager. Moving away
  from the inventory manager for Power systems is outside of the scope of this
  document.
- The code did not appear very pluggable to add support for new VPD formats, we
  might have to end up just utilizing #ifdef's to separate functions.
- Does not have a way to determine system blueprint for FRU devices, scans the
  entire /dev/ tree to pick out EEPROMs.

## Impacts

The following impacts have been identified:

- The services to parse VPD and store it in the inventory will add some time to
  the BMC boot flow. The impact should be kept to a minimum by achieving maximum
  possible parallelism in the launching of these services.
- Applications that need to read VPD keywords will be impacted in the sense that
  they would have to use the inventory interfaces to fetch the data they are
  interested in.

## Testing

VPD parsing function can be tested by faking out the VPD EEPROMs as files on the
filesystem. Such testing can also ensure that the right set of VPD data makes
its way into the OpenBMC Inventory. There is also a proposal to build in a file
mode into the application. The file mode will not need real hardware to test the
code functions, but can use files on the BMC to mimic the EEPROMs.

VPD writes can be tested by writing a small command line utility that can invoke
the VPD write application's APIs to write VPD.

[1]:
  https://www-355.ibm.com/systems/power/openpower/posting.xhtml?postingId=1D060729AC96891885257E1B0053BC95
[2]:
  https://github.com/openbmc/meta-openpower/blob/master/recipes-phosphor/vpd/openpower-fru-vpd/op-vpd-parser.service
[3]:
  https://github.com/openbmc/meta-openpower/blob/master/recipes-phosphor/vpd/openpower-fru-vpd/70-op-vpd.rules
[4]:
  https://github.com/openbmc/meta-openpower/blob/master/recipes-phosphor/vpd/openpower-fru-vpd-layout/layout.yaml
