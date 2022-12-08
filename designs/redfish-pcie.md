# Redfish PCIe Resources

Author: Jason Bills, jmbills

Other contributors: Ed Tanous

Created: April 3, 2019

## Problem Description

Redfish has resources that describe PCIe devices and functions available on a
system. It would be useful to provide these resources to users out-of-band over
Redfish from OpenBMC.

## Background and References

The Redfish PCIe resources are here:

[PCIeSlots](https://redfish.dmtf.org/schemas/PCIeSlots_v1.xml)

[PCIeDevice](https://redfish.dmtf.org/schemas/PCIeDevice_v1.xml)

[PCIeFunction](https://redfish.dmtf.org/schemas/PCIeFunction_v1.xml)

## Requirements

This feature is intended to meet the Redfish requirements for the PCIe resources
above to provide useful system configuration information to system
administrators and operators.

## Proposed Design

The proposed implementation will follow the standard D-Bus producer-consumer
model used in OpenBMC. The producer will provide the required PCIe values read
from hardware. The consumer will retrieve and parse the D-Bus data to provide
the Redfish PCIe resources.

The proposed D-Bus interface can be found here:
https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/19768

The proposed producer will be a new D-Bus daemon that will be responsible for
gathering and caching PCIe hardware data and maintaining the D-Bus interfaces
and properties. The actual hardware mechanism that is used to gather the PCIe
hardware data will vary.

For example, on systems that the BMC has access to the host PCI configuration
space, it can directly read the required registers. On systems without access to
the host PCI configuration space, an entity such as the BIOS or OS can gather
the required data and send it to the PCIe daemon through IPMI, etc.

When reading hardware directly, the PCIe daemon must be aware of power state
changes and any BIOS timing requirements, so it can check for hardware changes,
update its cache, and make the necessary changes to the D-Bus properties. This
will allow a user to retrieve the latest PCIe resource data as of the last
system boot even if it is powered off.

bmcweb will be the consumer. It will be responsible for retrieving the Redfish
PCIe resource data from the D-Bus properties and providing it to the user.

## Alternatives Considered

None.

## Impacts

Possible performance impact on the hardware-scanning and D-Bus updates. The
piece that implements hardware scanning should use mechanisms, such as caching
of the hardware configuration, to minimize the scanning time and updates to
D-Bus properties.

## Testing

This can be tested using the Redfish Service Validator.
