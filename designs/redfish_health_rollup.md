# Redfish Health and Status

Author: James Feist  !jfei

Primary assignee: James Feist !jfei

Other contributors: None

Created: 2019-04-17

## Problem Description
Redfish Status has 3 main properties: Health,
HealthRollup, and State. We need ways to be able to determine from a high level
the health of contained components. We also need to be able to determine the
health of individual components.

## Background and References
Associations were the selected way to map a sensor
to a inventory item, using an association of ["inventory", "sensors", \<path>].
Currently Redfish is being architected so that in a common case at maximum will
have the the extension /redfish/v1/Chassis/\<Container>/\<Chassis>/\<Board>. The
difficulty in this problem is to minimize the amount of D-Bus calls and data
referencing to get the health.

Associations In Sensors:
https://lists.ozlabs.org/pipermail/openbmc/2019-February/015188.html

Association Definition Interface:
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Association/Definitions.interface.yaml

Operational Status Interface:
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/State/Decorator/OperationalStatus.interface.yaml

Redfish schema guide:
https://www.dmtf.org/sites/default/files/standards/documents/DSP2046_2018.1_0.pdf

Callback Manager:
https://github.com/Intel-BMC/provingground/blob/master/callback-manager/src/callback_manager.cpp

## Requirements
- Must minimize D-Bus calls needed to get health rollup
- Must minimize new D-Bus objects being created
- Should try to utilize existing D-Bus interfaces

## Proposed Design
The callback manager tracks LED state, and shows the inventory that is currently
affecting the state. Today only sensor thresholds are used to change LED state.
Using the baseboard as an example, Redfish should look up
/xyz/openbmc_project/inventory/system/chassis/WFP_Baseboard/sensors, and
compare the list with the Fatal, Critical, and Warning items in the callback
manager to look for matches. This works for the individual Board level, and for
the sensor/thermal level. For the Chassis level, we don't need to look at the
inventory, because anything in the callback manager's status tracking should be
inside of the chassis. This should at most require 2 D-Bus calls. The individual
Health should be able to be gotten from each component by cross referencing the
callback manager in the same way.

## Alternatives Considered
Inventory should create an association for all paths
of inventory in which they are associatied. For example a temp sensor might on
failure create:

["", "critical", "/xyz/openbmc_project/inventory/system/chassis/WFP_Baseboard"]

Or on warning create: ["", "warning",
"/xyz/openbmc_project/inventory/system/chassis/WFP_Baseboard"]

This way to get the health rollup of the WFP_Baseboard you would need to only
query "/xyz/openbmc_project/inventory/system/chassis/WFP_Baseboard/warning" and
"/xyz/openbmc_project/inventory/system/chassis/WFP_Baseboard/critical" in the
mapper. Redfish would be able to look at the contained boards associations. The
worst case should be a multinode level, which would have to reference the
contained chassis, then reference the contained boards. This should all be able
to be done asynchronously but may add up to near 20 D-Bus calls on the worst
case.

## Impacts
BMCWeb may get issues reported as CallbackManager was never
upstreamed as it was developed recently. It needs to get upstreamed.

## Testing
Testing will be performed using sensor values and the status LED.
