# Platform specific EEPROM device type identification

Author:
   Karthikeyan P(karthik), [p_karthikeya@hcl.com](mailto:p_karthikeya@hcl.com)
   Kumar Thangavel(kumar), [thangavel.k@hcl.com](mailto:thangavel.k@hcl.com)
   Velumani T(velu),  [velumanit@hcl](mailto:velumanit@hcl.com)

other contributors:

created:
    Jan 31, 2022

## Problem Description

The current implementation in entity-manager FRU device application
supports EEPROM single byte or two byte identification. But, this
implementation does not work in all the machine. Also there is a chance
of data corruption in the logic when it does EEPROM write.

We have an FRU that contain types of oem, one type of oem supports
one byte EEPROM another type of oem supports two byte EEPROM,
so we need to identify which type of EEPROM byte type is present
in yosemitev2 machine.

## Background and References

Existing FRU device implementation of device byte type identification:

The current entity-manager FRU device application identifies single byte or
two byte EEPROM type using byte read and write logic.

https://github.com/openbmc/entity-manager/commit/
2d681f6fdec02b2f56d9155117c4830703026cb0#diff-
c0c1c17cf32614c7fb0d6241a3aad7977352e148b481d91936a8bd43b42837ecR66

Our yosemitev2 machine supports two types of NIC card, Broadcom NIC has
FRU details in single byte EEPROM and Mellanox NIC has FRU details
stored in two byte EEPROM.

Below is the link to OCP NIC specification,
https://www.opencompute.org/documents/facebook-ocp-mezzanine-20-specification
http://files.opencompute.org/oc/public.php?service=files&t=3c8f57684f959c5b7abe2eb3ee0705b4

## Requirements

* Need to identify single byte or two byte EEPROM (Broadcom or Mellanox).

* Entity-manager should read this EEPROM type from NIC card. This
  identifiaction is generic to all. So it is handled in the phosphor-networkd
  repo. It is not machine layer specific.

## Proposed Design

This document proposes a new design engaging the FRU device to find the EEPROM
device byte type, as single byte or two byte from the ncsid(phosphor-networkd)
and read this device byte type via D-bus.

The implementation flow of EEPROM byte type identification:

1) Created a new interface and property for eeprom byte type identification
   in phosphor-dbus-interface. This interface is created in phosphor-networkd
   repo.

2) In phosphor-networkd, creating a new D-bus for identifying the eeprom byte
   type.

3) Using ncsi-netlink command "ncsi-netlink --info -x 2 -p 0 -c 0", Parses
   the NIC manufacturer information like Broadcom or Mellanox from NIC
   information and get the EEPROM device byte type can be identified as
   single byte (Broadcom) or two byte(Mellanox). and update the output
   in D-bus under the interface of xyz.openbmc_project.Network.NCSI in
   phosphor-networkd.

4) Based on D-bus(phosphor-networkd), the output will be updated in json's
   probe field of eeprom in entity-manager.

5) Finally the FRU device will read and use the byte type(single byte
   or two byte)from the D-bus of phosphor-networkd.

This byte type identification logic in entity-manager should be generic
for all the platforms.

Following modules will be updated for this implementation
* phosphor-dbus-interface
* phosphor-networkd
* Entity-manager

## Alternatives Considered

1) An approach has been tried with identifying the EEPROM byte type from the
   platform specific service and added probe field with compatible string
   in entity-manager platform specific config file. But, In entity-manager
   dbus properties doesn't have write permission. So EEPROM byte type cannot
   be updated in dbus property.

2) The flag has been created to enable the EEPROM device byte identification
   service in entity-manager config file. If the flag is set in entity-manager
   configuration, the particular machine specific service will be be executed.
   If not set the existing logic will be executed.

   The platform specific service shall execute the script which parses the NIC
   manufacturer information like Broadcom or Mellanox from NIC information
   and get the EEPROM device byte type can be identified as single byte
   (Broadcom) or two byte(Mellanox).

   This service will return the output (single byte or two byte) to the
   service initiated from FruDevice and get the byte type using sdbusplus
   call and updated those byte type information in FruDevice D-bus.
   Finally the FRU device will read and use the byte type
   (single byte or two byte)from the D-bus.

This new generic function creation approach in entity-manager can be more
suitable to handle EEPROM device byte type identification.

## Impacts

This is generic way to detect the model ID so less impact.

## Testing

Testing with yosemitev2 platform.

