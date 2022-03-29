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

HealthRollup contains the combined health for all components below it. Main use
cases are:

- Health of individual sensors applying to health of chassis
- Health of chassis applying to health of bmc
- Health of host applying to system health

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
- Managers/BMC
- System

Currently operational status only has pass / fail options. Where Redfish health
is tri-state:
https://github.com/openbmc/bmcweb/blob/master/static/redfish/v1/schema/Resource_v1.xml#L197

Threshold interface (Currently unused by some vendors):
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Sensor/Threshold.errors.yaml

Associations In Sensors:
https://lists.ozlabs.org/pipermail/openbmc/2019-February/015188.html

Association Definition Interface:
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Association/Definitions.interface.yaml

Operational Status Interface:
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/Decorator/OperationalStatus.interface.yaml

Redfish schema guide:
https://www.dmtf.org/sites/default/files/standards/documents/DSP2046_2018.1_0.pdf

BMC inventory interface:
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Inventory/Item/Bmc.interface.yaml

System inventory interface:
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Inventory/Item/System.interface.yaml

Item interface:
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Inventory/Item.interface.yaml

## Requirements
- Must minimize D-Bus calls needed to get health rollup
- Must minimize new D-Bus objects being created
- Should try to utilize existing D-Bus interfaces

## Proposed Design

### Individual Sensor Health

Map critical thresholds to health critical, and warning thresholds to health
warning. If thresholds do not exist or do not indicate a problem, map
OperationalStatus failed to critical.

### Chassis / System Health

Chassis have individual sensors. Cross reference the individual sensors with
the global health. If any of the sensors of the chassis are in the
global health association for warning, the chassis rollup is warning.
Likewise if any inventory for the chassis is in the global health critical, the
chassis is critical. The global inventory item will be
xyz.openbmc_project.Inventory.Item.Global.

### Chassis / System Health Rollup

Any other associations ending in "critical" or "warning" are combined, and
searched for inventory. The worst inventory item in the Chassis is the rollup
for the Chassis. System is treated the same.

### Fan example

A fan will be marked critical if its threshold has crossed critical, or its
operational state is marked false. This fan may then be placed in the global
health warning association if the system determines this failure should
contribute as such. The chassis that the fan is in will then be marked as
warning as the inventory that it contains is warning at a global level.

## Alternatives Considered

A new daemon to track global health state. Although this would be difficult
to reuse to track individual component health.

### Health Rollup With Extra Dbus Support

The current health rollup implementation is hard to be applied universally cross
different modules like sensor and chassis. Each module has its own way to
determine the health status and bmcweb does not buffer these health statuses
during runtime in each module implmentation. Also, relationships between objects
in different modules cannot be easily figured out through existing associations
and ObjectMapper tree provided by dbus. In order to collect health statuses of
all descendants, it is required to not only collect children from multiple
associations but also traverse the association tree since the associations of an
object only cover direct children. The latter action needs extra efforts to be
implemented as bmcweb consists of asynchronous operations and some parents might
have shared children. For ObjectMapper, it is also hardly to get all descendants
because they might not be subtrees of the ancestor object like ancestor chassis
and its descendant sensors.

Therefore, an alternative solution is directly providing both a status interface
,`xyz.openbmc_project.State.Decorator.Status`, and an assocation of descendants,
`health_rollup`, on dbus.

`xyz.openbmc_project.State.Decorator.Status` has a `Health` property with three
possible values which are just "OK", "Warning" and "Critical". Users can update
the `Health` property based on their own health checking logics. It should be
noticed that this health rollup alternative itself is not reponsible for health
updates.

`health_rollup` includes all descendant object paths but not just those of
direct children. The rollup health can be easily figured out by checking all
`Health` properties of the object and its descendants. It is the responsibility
of association registrators to unflatten the device tree in order to create the
`health_rollup` association. If a device is a leaf device which means it has no
descendant, it will not have the Redfish `Status.HealthRollup` field but only
`Status.Health` since health rollup does not apply to it.

With this alternative, users can indeed achieve health rollup on device trees
and flexibly update device health without being limited by existing OpenBMC
interfaces and properties.

## Impacts


## Testing

Testing will be performed using sensor values and the status LED.
