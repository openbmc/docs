# Redfish PCIe Resources

Author:
  Jason Bills, jmbills

Primary assignee:
  Jason Bills

Other contributors:
  Ed Tanous

Created:
  April 3, 2019

## Problem Description
Redfish has resources that describe PCIe devices and functions available
on a system. It would be useful to provide these resources to users
out-of-band over Redfish from OpenBMC.

## Background and References
The Redfish PCIe resources are here:

[PCIeSlots](https://redfish.dmtf.org/schemas/PCIeSlots_v1.xml)

[PCIeDevice](https://redfish.dmtf.org/schemas/PCIeDevice_v1.xml)

[PCIeFunction](https://redfish.dmtf.org/schemas/PCIeFunction_v1.xml)

## Requirements
This feature is intended to meet the Redfish requirements for the PCIe
resources above to provide useful system configuration information to system
administrators and operators.

## Proposed Design
The proposed implementation is to use a D-Bus interface to provide the
PCIe values read from hardware that are required for the Redfish PCIe
resources.

The proposed D-Bus interface can be found here:
https://gerrit.openbmc-project.xyz/c/openbmc/phosphor-dbus-interfaces/+/19768

bmcweb can then just retrieve and parse the D-Bus data to provide the
Redfish PCIe resources.

The hardware-scanning portion to actually build and update the PCIe D-Bus
objects is not part of this design.

## Alternatives Considered
None.

## Impacts
Possible performance impact on the hardware-scanning and D-Bus updates.
The piece that implements hardware scanning should use mechanisms,
such as caching of the hardware configuration, to minimize the scanning time
and updates to D-Bus properties.

## Testing
This can be tested using the Redfish Service Validator.
