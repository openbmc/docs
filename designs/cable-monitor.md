# Cable Monitor

Author: Jagpal Singh Gill <paligill@gmail.com>

Created: July 26, 2024

## Problem Description

Cables are components that can be intentionally or unintentionally inserted or
removed while a system is running. Additionally, the specific configuration of
cables connected for a particular hardware setup may need to be determined at
runtime based on datacenter deployments. Currently, there is no service in
openBMC that can be configured at runtime to monitor connected cables and
generate corresponding Redfish events.

## Background and References

openBMC provides a GPIO monitoring framework through
[phosphor-gpio-monitor](https://github.com/openbmc/phosphor-gpio-monitor), but
this framework is too generic in its design that allows for watching a GPIO and
running a related systemd target. For more information, please refer to the
Alternative section.

## Requirements

1. Able to support runtime configuration of cable connectivity.
2. Able to monitor inventory interface events related to cables and generate
   corresponding Redfish events.

## Proposed Design

### Cable Monitor

```
                xyz.openbmc_project.Inventory.Item.Cable
                           inventory updates
                                   |
                                   |
                                   v
    +--------------+      +-----------------+
    |              |      |                 |
    |  cable.conf  |----->|  cable-monitor  |-----> Cable Events
    |              |      |                 |
    +--------------+      +-----------------+
```

#### cable.conf

The format of the runtime configuration is as under -

```json
{
  "cable0": {
    "Inventory": "/system/chassis/motherboard/cable0"
  }
}
```

The configuration lists the cables that should be connected and monitored.

- Inventory: The inventory path to check for the presence of
  xyz.openbmc_project.Inventory.Item.Cable for this cable.

This fulfills requirement #1.

#### cable-monitor

The cable monitor is responsible for processing the cable.conf file and parsing
its contents. It uses this information to raise appropriate alerts based on
expected cable connectivity and inventory item status. Additionally, it
continuously monitors any changes in the cable inventory item status and raises
or resolves alerts accordingly. This fulfills requirement #2.

### Error Reporting

A definition for
[missing cable](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/74397)
event is proposed for Redfish, which is based on
[event logging design](https://github.com/openbmc/docs/blob/master/designs/event-logging.md).

### Future Enhancements

The cable monitor can be extended to support runtime cable configurations
through the Redfish
[Cable](https://redfish.dmtf.org/schemas/v1/Cable.v1_2_3.json) schema by using
the CableStatus property.

## Alternatives Considered

### phosphor-gpio-monitor

The phosphor-gpio-monitor does not currently support monitoring inventory items
such as cable inventory items, although it could be extended to do so. However,
this would not align with its intended purpose and nomenclature. Additionally,
there is no support for runtime configuration in phosphor-gpio-monitor. Any
change to add these features would result in a monolithic daemon that would need
to be extended for every other inventory item type.

## Impacts

### API Impacts

- A cable connect event api is being suggested as the source of Redfish event.

### Organizational

- Does this repository require a new repository? No.
- Which repositories are expected to be modified to execute this design?
  `dbus-senosrs`
- Make a list, and add listed repository maintainers to the gerrit review.

## Testing

### Unit Testing

All the functional testing of the reference implementation will be performed
using GTest.

### Integration Testing

Integration testing for generating Redfish events will be carried out using the
inventory item interface by creating and deleting cable inventory.
