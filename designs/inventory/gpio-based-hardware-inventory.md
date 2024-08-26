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

- All the detectable hardware is 'Present' = false by default

- A presence daemon needs to be able to pick up configuration from
  entity-manager and update 'Present' property

- (optional/unsure): support for re-use of presence information in PROBE
  statements
  - detecting one entity as 'Present' should enable detecting other entities
    based on that info
  - not sure if this is a requirement (TODO: confirm)

## Proposed Design

The proposed design is to extend entity-manager to be able to expose additional
inventory items besides Board, Chassis, PSU and the small number of others.
These inventory items will be bespoke implementations with each of them having
custom code in entity-manager. Because they may have custom logic and inventory
item specific properties.

phosphor-multi-gpio-presence will also have to be extendend to accept
entity-manager configuration via dbus and thus be able to update 'Present'
property.

Optionally, (if required, TBD) extend entity-manager to be able to probe on the
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

- Exposing the item-specific interfaces and properties in a generic way This
  means EM would lose any semantic knowledge of the entities it exposes and
  become more like phosphor-inventory-manager

## Impacts

### Organizational

- Does this repository require a new repository? No
- Who will be the initial maintainer(s) of this repository?
- Which repositories are expected to be modified to execute this design?
  - entity-manager
  - phosphor-multi-gpio-presence
- Make a list, and add listed repository maintainers to the gerrit review.

## Testing

How will this be tested? How will this feature impact CI testing?

Manual testing, since multiple repositories are involved for this feature.
