# Platform specific EEPROM device type identification

Author:
   Karthikeyan P(karthik), [pkarthikey@hcl.com](mailto:pkarthikey@hcl.com)
   Kumar Thangavel(kumar), [thangavel.k@hcl.com](mailto:thangavel.k@hcl.com)
   Velumani T(velu),  [velumanit@hcl](mailto:velumanit@hcl.com)

other contributors:

created:
    Dec 15, 2021

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

https://github.com/openbmc/entity-manager/blob/master/src/FruDevice.cpp#L197

Our yosemitev2 machine supports two types of NIC card, Broadcom NIC has
FRU details in single byte EEPROM and Mellanox NIC has FRU details
stored in two byte EEPROM.

Below is the link to OCP NIC specification,
https://www.opencompute.org/documents/facebook-ocp-mezzanine-20-specification
http://files.opencompute.org/oc/public.php?service=files&t=3c8f57684f959c5b7abe2eb3ee0705b4

## Requirements

* Need to identify single byte or two byte EEPROM (Broadcom or Mellanox).
* To get the EEPROM type single byte or two byte at platform level.
* Entity-manager should read this EEPROM type from machine layer.
* This byte type identification logic in entity-manager should be generic
  for all the platforms.

## Proposed Design

This document proposes a new design engaging the FRU device to find the EEPROM
device byte type, as single byte or two byte from the machine layer and read
this device byte type via D-bus.

The implementation flow of EEPROM byte type identification:

1) The flag has been created to enable the EEPROM device byte identification
   service in entity-manager config file.

2) If the flag is set in entity-manager configuration, the particular machine
   specific service will be be executed. If not set the existing
   logic will be executed.

3) The platform specific service shall execute the script which parses the NIC
   manufacturer information like Broadcom or Mellanox from NIC information
   and get the EEPROM device byte type can be identified as single byte
   (Broadcom) or two byte(Mellanox).

4) This service will return the output (single byte or two byte) to the service
   initiated from FruDevice and get the byte type using sdbusplus call and
   updated those byte type information in FruDevice D-bus.

5) Finally the FRU device will read and use the byte type(single byte
   or two byte)from the D-bus.

This byte type identification logic in entity-manager should be generic
for all the platforms.

Following modules will be updated for this implementation
* Entity-manager
* Machine layer

## Alternatives Considered

An approach has been tried with identifying the EEPROM byte type from the
platform specific service and added probe field with compatible string
in entity-manager platform specific config file. But, In entity-manager
dbus properties doesn't have write permission. So EEPROM byte type cannot
be updated in dbus property.

This new generic function creation approach in entity-manager can be more
suitable to handle EEPROM device byte type identification.

## Impacts

This is for machine specific so less impact.

## Testing

Testing with yosemitev2 platform.
