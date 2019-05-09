# Redfish Health and Status

Author: James Feist  !jfei

Primary assignee: James Feist !jfei

Other contributors: None

Created: 2019-04-17

## Problem Description Redfish Status has 3 main properties: Health,
HealthRollup, and State. We need ways to be able to determine from a high level
the health of contained components. We also need to be able to determine the
health of individual components.

## Background and References

HealthRollup contains the combined health for all components below it. Main use
cases are:

- Health of individual sensors applying to health of chassis
- Health of chassis applying to health of service root
- Health of bmc events applying to health of bmc

The more difficult examples that we need to cover are:

https://<bmc-addr>/redfish/v1/Systems/system/Memory/dimm0, where we need to roll
the health of the dimm up multiple levels to the system health.

https://<bmc-addr>/redfish/v1/Managers/bmc, where we need to get the health of
the bmc with no direct sensors.

https://<bmc-addr>/redfish/v1/Chassis/1Ux16_Riser_1/<sensor-type>, where
multiple sensor types roll up to the chassis health.

https://<bmc-addr>/redfish/v1/Managers/bmc/EthernetInterfaces/eth1, where the
ethernet interface failing needs to contribute to bmc health.

Some examples of health with different use cases are:
- Individual Sensors
- Chassis containing sensors
- Mangers/BMC
- System
- Service Root

Currently operational status only has pass / fail options. Where Redfish health
is tri-state:
https://github.com/openbmc/bmcweb/blob/master/static/redfish/v1/schema/Resource_v1.xml#L197

Threshold interface (Currently unused by some vendors):
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Sensor/Threshold.errors.yaml

Associations In Sensors:
https://lists.ozlabs.org/pipermail/openbmc/2019-February/015188.html

Association Definition Interface:
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Association/Definitions.interface.yaml

Operational Status Interface:
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/State/Decorator/OperationalStatus.interface.yaml

Redfish schema guide:
https://www.dmtf.org/sites/default/files/standards/documents/DSP2046_2018.1_0.pdf

BMC inventory interface:
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Inventory/Item/Bmc.interface.yaml

System inventory interface:
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Inventory/Item/System.interface.yaml

Item interface:
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Inventory/Item.interface.yaml

Callback Manager:
https://github.com/Intel-BMC/provingground/blob/master/callback-manager/src/callback_manager.cpp

## Requirements
- Must minimize D-Bus calls needed to get health rollup
- Must minimize new D-Bus objects being created
- Should try to utilize existing D-Bus interfaces

## Proposed Design

### Individual Sensor Health

Map critical thresholds to health critical, and warning thresholds to health
warning.  If thresholds do not exist, map OperationStatus failed to critical.

### System Health Rollup

Create an association to the System inventory interface for critical and
warning.

Ex:

["", "warning", "\<system-inventory-item\>"]

In this way a individual sensors health might be critical, but to the system it
only contributes to warning.

### BMC Health Rollup

Create an association to the BMC inventory interface for critical and warning.

Ex:

["", "critical", "\<bmc-inventory-item\>"]

Things like watchdogs then may add themselves at the correct level to BMC
health.

### Service Root Health Rollup

Create a new inventory item, global, to create associations to in the same way
as the above.

### Chassis Health Rollup

Chassis have individual inventory. Cross reference the individual inventory with
the service root health. If any of the inventory of the chassis is in the
service root health association for warning, the chassis rollup is warning.
Likewise if any inventory for the chassis is in the service root critical, the
chassis is critical.

### Fan example

A fan will be marked critical if its threshold has crossed critical, or its
operational state is marked false. This fan may then be placed in the service
root warning association if the system determines this failure should contribute
as such. The chassis that the fan is in will then be marked as warning as the
inventory that it contains is warning at a service root level. The service root
will also be marked as warning.

## Alternatives Considered

Using callback manager, although this only partially solves the problem as it
only tracks the equivalent of the service root state. It would be better to have
a consistent approach for all levels of the Redfish tree.

## Impacts


## Testing

Testing will be performed using sensor values and the status LED.
