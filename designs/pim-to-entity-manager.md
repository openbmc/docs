# Moving IBM Systems From PIM to EM

Author:
  Santosh Puranik (puranik)

Other contributors:
  Jinu Thomas (jinujoy)

Created:
  September 12, 2022

## Problem Description

IBM systems use phosphor-inventory-manager (PIM) as their inventory management
solution. There is desire to switch over to entity-manager (EM) for the next
generation of IBM systems. This document aims to describe the current users and
use-cases handled by PIM and how best we can switch over to using EM as the
inventory manager.

There are several issues to tackle here:

- How would applications such as fan and power supply monitors publish inventory
  data? (For FRUs that don't have VPD)
- How would platform-fru-detect publish NVMe drive data?
- How would we handle the scenario where applications expect the inventory D-Bus
  objects to follow certain hierarchies? For example, CPU object path needs a
  dual-chip-module (DCM) segment as the parent.
- How will associations between two inventory items be created?
- How will non inventory-inventory associations be created? Ex. inventory to LED
  group association.
- The VPD application creates a blueprint of all possible inventory items in
  PIM. How would we achieve that with EM?

## Background and References

The [VPD design document][vpd-design] describes how IBM systems detect inventory
items (FRUs) and use PIM to host the inventory D-Bus objects. There is also a EM
design [here][em-associations] that proposes to add inventory-inventory item
association support to EM. This capability to host inventory-inventory
associations within EM will be needed to replace the [associations JSON][assoc]
that PIM supports.

## Requirements

The following are the requirements that need to be handled when moving from PIM
to EM:

- Every inventory item with a valid location code should be made available on
  D-Bus irrespective of whether the item is actually present in the system. This
  is needed so that the list can be supplied to the HMC over Redfish APIs.
- External applications such as guard and PLDM need to be able to set states
  into the inventory item to indicate, for example, that the part is guarded or
  non-functional.
- We need support for creating associations between any two inventory items
  hosted by EM. For example, processor to PCIe slot, VRM to processor/DIMM, etc.
- We need to persist VPD data (this may have to be outside of EM). This
  persistency is something we get with PIM by default. We may have to persist
  the VPD D-Bus objects ourselves outside of the inventory manager. Note that we
  could also just choose to cache the entire VPD as files.
- Support associating sensors with inventory items. This is something EM already
  supports with dbus-sensors, but we need to ensure that all the various sensors
  associated to the objects hosted by PIM are moveable to EM config files.
- No application on the BMC should rely on the inventory D-Bus object path being
  a certain format or representing any hierarchy. Any relations, including
  parent-child relations must be represented by D-Bus associations.
- The FRU (not inventory) D-Bus objects must be hosted by the application that
  detects their presence/reads their FRU information.
- In general we want an alternative to the `Notify` call, but without providing
  a  new `Notify` - Applications calling `Notify` can host their own FRU D-Bus
  objects and write config files to have EM host the inventory items.

## Proposed Design

This section will split up the proposed changes on an application-by-application
basis.

### VPD Collection

The VPD parsers will parse VPD from EEPROMs and host the data as FRU D-Bus
objects within the vpd-manager daemon. EM config files will be written for every
FRU and these will expose the inventory information via EM. Any BMC applications
that want to get to the VPD data can follow the "probes" association from the
inventory item to get to the FRU D-Bus object.

### Other Applications

Any applications that create inventory items that need to be hosted by EM have
one of two options: They can either host the inventory item themselves (if they
do not want to take advantage of EM's ability to expose configurations) OR these
apps could host D-Bus objects that EM config file can probe on. For example, the
phosphor-power application "synthesizes" power supply VPD and pushes the data to
PIM to host. This could be handled by having the phosphor-power daemon host FRU
objects for the power supplies and have an EM config file that hosts the
inventory D-Bus object. Something similar might also work for the
platform-fru-detect code.

Users of inventory that are only interested in the inventory information such as
bmcweb and phosphor-logging can simply query these directly from EM.

## Open Questions and significant impacts.

There are several open items we need need to address. This section lists
requirements that would likely need enhancements to EM to support:

- EM inventory items need to host location codes. One possible approach here
  would be for the FRU objects to host their own location code properties and
  have the EM configuration copy it to the inventory D-Bus object.
- Certain inventory properties are writeable. For example, PLDM can write
  inventory properties such as core count under a processor, operational states
  for inventory items etc. To my knowledge, the properties exposed by EM are
  read-only. We would need to figure out how to get EM to host such properties
  for us. These properties would have to be writeable and need to be persisted.
- There is support planned in EM for inventory-inventory item associations,
  however, PIM supports defining any arbitrary associations. For example, the
  led-manager needs associations between the inventory D-Bus object and LED
  groups. For it to work with EM, led-manager would need to react to EM hosted
  inventory items and use (perhaps) something like the location code to form the
  association with the LED group.
- There are applications such as PLDM and some bmcweb code that still relies on
  D-Bus path hierarchies of inventory objects. These would have to switch over
  to using associations.
- How do we expose a "blueprint" of inventory/location codes via EM? I don't
  think EM config files have to capability of exposing other inventory items?
- What needs to be used as a common-key between various data constructs on the
  BMC applications? For example, how can a device tree target be associated with
  an inventory item? Location code?

## Alternatives Considered

The alternative is to continue with PIM as our inventory manager.

## Impacts

Moving to EM will impact the following applications:

- All users of the `Notify` D-Bus API: This includes the openpower-vpd-parser,
  pldm, platform-fru-detect,  phosphor-power, phosphor-fan-presence.
- Any application that has the PIM service name hardcoded.
- bmcweb uses the object mapper and should be immune to this change.

### Organizational

- Does this repository require a new repository?  No
- Who will be the initial maintainer(s) of this repository? NA
- Which repositories are expected to be modified to execute this design?
  openpower-vpd-parser, entity-manager, phosphor-led-manager, pldm, phal/guard
- Make a list, and add listed repository maintainers to the gerrit review. TBD

## Testing

TBD

## Footnotes

[vpd-design]:
    https://github.com/openbmc/docs/blob/master/designs/vpd-collection.md
[em-associations]:
    https://gerrit.openbmc.org/c/openbmc/docs/+/54205
[assoc]:
    https://github.com/openbmc/phosphor-inventory-manager#creating-associations
