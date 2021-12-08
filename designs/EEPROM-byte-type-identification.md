# Platform specific EEPROM device type identification

Author:
   Karthikeyan P(Karthik), [p_karthikeya@hcl.com](mailto:p_karthikeya@hcl.com)
   Kumar Thangavel(kumar), [thangavel.k@hcl.com](mailto:thangavel.k@hcl.com)
   Velumani T(velu),  [velumanit@hcl](mailto:velumanit@hcl.com)

other contributors:

created:
    Dec 8, 2021

## Problem Description

The current implementation in entity-manager, FRU device application
supports EEPROM single-byte or two-byte identification. But, this
implementation does not work in all the machines. Also, there is a chance
of data corruption in the logic when it does EEPROM write.

We have a NIC that is from (multiple vendors) two different OEMs,
one of the OEM has single-byte EEPROM support and other OEM has support
two-byte EEPROM, which has the details of ipmi fru data. So we need
to identify which type of EEPROM size is present.

## Background and References

Existing FRU device implementation of device size identification:

The current entity-manager FRU device application identifies single-byte or
two-byte EEPROM size using byte read and write logic.

https://github.com/openbmc/entity-manager/commit/
2d681f6fdec02b2f56d9155117c4830703026cb0#diff-
c0c1c17cf32614c7fb0d6241a3aad7977352e148b481d91936a8bd43b42837ecR66

The yosemitev2 machine supports two types of NIC cards, Broadcom NIC has
FRU details in Atmel-24c02 EEPROM (single-byte) and Mellanox NIC has FRU details
stored in Atmel-24c64 byte EEPROM (two-byte).

Below is the link to the OCP NIC specification,
https://www.opencompute.org/documents/facebook-ocp-mezzanine-20-specification
http://files.opencompute.org/oc/public.php?service=files&t=3c8f57684f959c5b7abe2eb3ee0705b4

## Requirements

* The requirement is to identify single byte(atmel-24c02) or two byte(atmel-24c64)
  EEPROM (Broadcom or Mellanox).

* Each vendor has a different EEPROM size, from that networkd identifies the
  manufacturer details, Based on the manufacturer details we will enable the
  configuration from entity-manager.

## Proposed Design

This document proposes a design engaging the FRU device to find the EEPROM
device size, as single-byte(Atmel-24c02) or two-byte(Atmel-24c64) from the
ncsid(phosphor-networkd) and read this device size via D-bus.

The implementation flow of EEPROM byte size identification:

1) Created a new interface and property in Inventory.Decorator.I2CDevice
   to describe the EEPROM size identification in phosphor-dbus-interface.
   This interface is used in phosphor-networkd repo.

2) In phosphor-networkd, Implemented a separate systemd service for identifying
   EEPROM size, which initiates the ncsi-info daemon and this daemon updates
   the D-bus interface with the details of what is the size of EEPROM.

3) Using getInfo API function from phosphor-networkd, which is used to parses
   the NIC manufacturer information, Based on the manufacturer information
   we will update the EEPROM size as a single-byte (Broadcom Atmel-24c02) or
   two-byte (Mellanox atmel-24c64).

4) The ncsi-info daemon will update the EEPROM size as a single byte or two byte
   and entity-manager configuration will be read from it using probe field in
   config file.

5) In FRU device identification, while parsing the EEPROM it will try to get the
   size information from the entity-manager D-bus configuration, So if the
   configuration is present it will take that configuration and it will use
   single-byte read or two-byte read. if no configuration is found the FRU
   identification will be a default.

This byte size identification logic in entity-manager should be generic
for all the platforms.

The following modules will be updated for this implementation
* phosphor-dbus-interface
* phosphor-networkd
* Entity-manager

## Alternatives Considered

1) An approach has been tried with identifying the EEPROM byte size from the
   platform-specific service and added probe field with compatible string
   in entity-manager platform specific config file. But, In entity-manager
   dbus properties don't have write permission. So EEPROM byte size cannot
   be updated in dbus property.

2) The flag has been created to enable the EEPROM device byte identification
   service in an entity-manager config file. If the flag is set in entity-manager
   configuration, the particular machine-specific service will be executed,
   If not set the existing logic will be executed.

   The platform-specific service shall execute the script which parses the NIC
   manufacturer information like Broadcom or Mellanox from NIC information
   and get the EEPROM device byte size can be identified as a single byte
   (Broadcom) or two-byte(Mellanox).

   This service will return the output (single-byte or two-byte) to the
   service initiated from FruDevice and get the byte size using sdbusplus
   call and updated those byte size information in FruDevice D-bus.
   Finally, the FRU device will read and use the byte size
   (single-byte or two-byte)from the D-bus.

This new generic function creation approach in entity-manager can be more
suitable to handle EEPROM device byte size identification.

## Impacts

1) By default this particular implementation will not be enabled, only the
   platform which are going to be use this EEPROM identification feature they
   will only be using it others will not have any impact.

2) This feature gives the user flexibility to adopted in any system because
   it is implemented through D-bus can anyone configure easily to their
   partiular machine. So there is no impact to others.

## Testing

This feature will be tested with yosemitev2 platform for both mellanox and
broadcom NIC cards.

