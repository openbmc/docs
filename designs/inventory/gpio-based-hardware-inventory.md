# GPIO based hardware inventory

Author: Alexander Hansen <alexander.hansen@9elements.com>

Other contributors: Chu Lin <linchuyuan@google.com> (through their previous
works), Amithash Prasad <amithash@meta.com>

Created: August 26, 2024

## Problem Description

Due to increasing complexity of server designs and different configurations of
the same system being possible, there is a need for a simple way to detect the
presence of cards, cables and other connected entities which may or may not be
plugged into a system. A subset of these entities support presence detection via
gpios. This design focuses on those.

Connected entities detectable via other means are out of scope of this design.

## Background and References

The existing design for the gpio based cable presence is partially implemented
and focuses on ipmi use-case.

[existing design](https://github.com/openbmc/docs/blob/879601d92becfa1dbc082f487abfb5e0151a5091/designs/gpio-based-cable-presence.md)

Currently the way to do gpio based presence detection is via
phosphor-multi-gpio-presence and phosphor-inventor-manager.

The static inventory is declared and the inventory items are exposed at runtime
by phosphor-inventory-manager.

The presence daemon then toggles the 'Present' property on dbus interface
xyz.openbmc_project.Inventory.Item.

Additional item-specific properties are statically declared in the
phosphor-inventory-manager configuration.

An example of how this is currently done:

[presence daemon config](https://github.com/openbmc/openbmc/blob/1d438f68277cdb37e8062ae298402e9685882acb/meta-ibm/meta-sbp1/recipes-phosphor/gpio/phosphor-gpio-monitor/phosphor-multi-gpio-presence.json)

[phosphor-inventory-manager config](https://github.com/openbmc/openbmc/blob/1d438f68277cdb37e8062ae298402e9685882acb/meta-ibm/meta-sbp1/recipes-phosphor/inventory/static-inventory/static-inventory.yaml)

In the example we have inventory item **dimm_c0a1**

which has following phosphor-multi-gpio-presence configuration

```json
{ "Name": "DIMM_C0A1", "LineName": "PLUG_DETECT_DIMM_C0A1", "ActiveLow": true, "Bias": "PULL_UP", "Inventory": "/system/chassis/motherboard/dimm_c0a1" },
```

and phosphor-inventory-manager configuration

```yaml
- name: Add DIMMs
  description: >
    Add the DIMM inventory path.
  type: startup
  actions:
    - name: createObjects
      objs:
        /system/chassis/motherboard/dimm_c0a1:
          xyz.openbmc_project.Inventory.Decorator.Replaceable:
            FieldReplaceable:
              value: true
              type: boolean
          xyz.openbmc_project.State.Decorator.OperationalStatus:
            Functional:
              value: true
              type: boolean
          xyz.openbmc_project.Inventory.Item:
            PrettyName:
              value: "DIMM C0A1"
              type: string
            Present:
              value: false
              type: boolean
          xyz.openbmc_project.Inventory.Item.Dimm:
          xyz.openbmc_project.Inventory.Decorator.LocationCode:
            LocationCode:
              value: "CPU0_DIMM_A1"
              type: string
```

## Requirements

- Support the gpio based detection of inventory items without static
  configuration

- Allow configuration of the detectable inventory items through entity-manager
  configuration

  - e.g. cable
  - e.g. fan board
  - e.g. daughter board
  - entity-manager should expose all required Inventory interfaces
  - the properties for these inventory interfaces can be obtained through the
    exposes record in the json configuration

- A presence daemon needs to be able to pick up configuration from
  entity-manager and expose some information on dbus when it detects the
  hardware.

- (possible extension): support for re-use of presence information in PROBE
  statements
  - detecting one entity as 'Present' should enable detecting other entities
    based on that info

## Proposed Design

The proposed design is to create a new daemon in the dbus-sensors repository,
which is 'gpio-presence-sensor'.

It can be inspired by implementations found in downstream forks such as the
[NVIDIA gpio presence sensor implementation](https://github.com/NVIDIA/dbus-sensors/blob/889abc1c9bfae9395690d1562f3e08453dfa12ba/src/GPIOPresenceSensorMain.cpp)

### Proposed DBus Interfaces

'gpio-presence-sensor' should consume configuration via dbus interface

`xyz.openbmc_project.Configuration.GPIOHWDetect`

```
description: >
    Information to enable a daemon to probe hw based on gpio value
properties:
    - name: Id
      type: string
      description: >
          Used by entity-manager to identify which hw was detected.
          For internal use by entity-manager.
    - name: PresenceGpios
      type: array[string]
      description: >
          Names of the gpio lines.
    - name: Polarities
      type: array[string]
      description: >
          "High" or "Low" depending on which level means the hw is present.
```

and then expose `xyz.openbmc_project.Configuration.GPIOHWDetected` dbus
interface of its own if it detects the hardware.

```
description: >
    Information for a daemon to expose if hardware has been detected based on
    xyz.openbmc_project.Configuration.GPIOHWDetect interface
properties:
    - name: Id
      type: string
      description: >
          Used by entity-manager to identify which hw was detected.
          For internal use by entity-manager.
```

entity-manager can then consider the hardware as present and expose the
inventory interfaces for it.

### Handling removal of the device

In case the gpio state changes, 'gpio-presence-sensor' can remove the
`xyz.openbmc_project.Configuration.GPIOHWDetected` interface and entity-manager
can have a dbus matcher for that, to then remove the respective inventory items
and any inventory items detected below it aswell.

### Proposed changes in entity-manager

entity-manager needs to be extended to be able to expose additional inventory
items besides Board, Chassis, PSU and the small number of others. These
inventory items will be bespoke implementations with each of them having custom
code in entity-manager. Because they may have custom logic and inventory item
specific properties.

This has to be done in code since otherwise it will be like the
phosphor-inventory-manager implementation and expose anything that's configured.

Optionally, (if required) extend entity-manager to be able to probe on the
already present inventory items, to expose additional entites found in other
configuration files.

### Example EM Config Fragments

Below is an incomplete example of how such a config could look like.

Not all of the item types correspond to actual items, they may also be there to
make a point.

```json
{
  Exposes:
  [
    {
      "Name": "cable0",
      "PresenceGpio": "presence-cable0",
      "CableLength": 0.3,
      "CableTypeDescription": "optical",
      "Type": "Cable"
    },
    {
      "Name": "fanboard0",
      "PresenceGpio": "presence-fanboard0",
      "Type": "FanBoard"
    },
    {
      "Name": "fanboard1",
      "PresenceGpio": "presence-fanboard1",
      "Type": "FanBoard"
    },
    ...
  ],
  ...
  Name: "Chassis",
}
```

## Alternatives Considered

- The existing approach with phosphor-inventory-manager and static configuration
  Leaning away from that because it cannot support multiple different chassis
  configurations in one fw image.

- Presence detection integrated into entity-manager. There already exists a
  presence daemon, and it's an explicit non-goal of EM to implement any presence
  detection.

- Another daemon which would expose inventory items based on EM configuration.
  This would mean EM does not need to expose the item-specific inventory
  interfaces and properties.

- Exposing the item-specific interfaces and properties in a generic way. This
  means EM would lose any semantic knowledge of the entities it exposes and
  become more like phosphor-inventory-manager

## Impacts

### Organizational

- Does this repository require a new repository? No
- Who will be the initial maintainer(s) of this repository?
- Which repositories are expected to be modified to execute this design?
  - entity-manager
  - dbus-sensors
- Make a list, and add listed repository maintainers to the gerrit review.

## Testing

How will this be tested? How will this feature impact CI testing?

The feature can be tested in the entity-manager repository and dbus-sensors
repository. Both can use dbus-run-session and create the dbus interfaces
expected from the other daemon from within the test, to create the expected
environment.
