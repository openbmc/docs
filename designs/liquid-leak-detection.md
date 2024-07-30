# Liquid Leak Detection

Author: Jagpal Singh Gill <paligill@gmail.com>

Created: July 26, 2024

## Problem Description

Liquid cooling is becoming a promising alternative to traditional air flow
cooling for high performance computing in data center environments. However,
this technique presents its own set of challenges, as liquids can be harmful to
electronic components. Therefore, it is crucial for any system that uses liquid
cooling to have a mechanism for detecting and reporting leaks so that
remediation actions can be taken. Currently, there is no service available in
openBMC to handle this task.

## Background and References

In this document, a leak is considered to be an entity with digital (present or
not-present) value. Currently, openBMC has a framework for sensors through
[DBus-sensors](https://github.com/openbmc/dbus-sensors), but it is primarily
designed for numerical readings rather than detectors with digital values. The
[phosphor-gpio-monitor](https://github.com/openbmc/phosphor-gpio-monitor) is too
generic in its design and does not meet most of the requirements. The design of
the hardware inventory based on GPIO is beyond the scope of this document. For
more information on that topic, please refer to
[design document](https://gerrit.openbmc.org/c/openbmc/docs/+/74022).

## Requirements

1. Able to detect a leak presence and raise alerts.

- Leak detectors can be connected to the BMC or installed on a hot-pluggable
  device/cable.

2. Able to classify leaks based on severity levels and take appropriate
   remediation actions.
3. Able to identify the presence of a hot-pluggable device/cable specified in
   runtime configurations and generate alerts if any are missing.
4. Able to produce an interface complaint with
   [Redfish Leak Detection](https://redfish.dmtf.org/schemas/v1/LeakDetection.v1_0_1.json).

## Proposed Design

### Dbus interfaces

The DBus Interface for leak detection will consist of following -

| Interface Name                                                                                                                                                                    |       Existing/New        |                     Purpose                     |
| :-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | :-----------------------: | :---------------------------------------------: |
| [xyz.openbmc_project.Cooling.Zone](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/73151/4/yaml/xyz/openbmc_project/Cooling/Zone.interface.yaml)                  |            New            |          Implement Leak Detector Zones          |
| [xyz.openbmc_project.Cooling.LeakDetector](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/73151/4/yaml/xyz/openbmc_project/Cooling/LeakDetector.interface.yamll) |            New            |         Implement Leak Detector Status          |
| [xyz.openbmc_project.Inventory.Item](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/73151/4/yaml/xyz/openbmc_project/Inventory/Item.interface.yaml)              | Existing (with additions) | Implement inventory that contains Leak Detector |

### Cable Monitor

```
                        xyz.openbmc_project.Inventory.Item.Cable
                                     inventory updates
                                             |
                                             |
                                             v
            +--------------+       +------------------+
            |              |       |                  |
            |  cable.conf  |------>|  cable-monitor   |----------> alerts
            |              |       |                  |
            +--------------+       +------------------+
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

#### cable-monitor

The cable monitor is responsible for processing the cable.conf file and parsing
its contents. It uses this information to raise appropriate alerts based on
expected cable connectivity and inventory item status. Additionally, it
continuously monitors any changes in the cable inventory item status and raises
or resolves alerts accordingly. This fulfills requirement #3.

### Liquid Leak Detector

```
    +-----------------------+
    |                       |
    | Entity Manager Config |
    |     (via D-Bus)       |
    |                       |
    +-----------------------+
                |
                v                                                       +--------------------------------------------------+
      +------------------+             +---------------------+          |    Create Interface                              |
      |                  |   creates   |                     |          |    xyz.openbmc_project.LeakDetection.Zones       |
      |   Leak Manager   |------------>| Leak Detector Zones |--------->|    at                                            |
      |                  |             |                     |          |    /xyz/openbmc_project/leak_detection/<ZoneX>   |
      +------------------+             +---------------------+          |                                                  |
                                                  |                     +--------------------------------------------------+
                                                  |
                                                  v
                                         +------------------+           +--------------------------------------------------+
                                        +|-----------------+|           |  Create Interface                                |
                                       +------------------+||           |  xyz.openbmc_project.Object.Enable               |
                                       |||                |||---------->|  at                                              |
                                       ||| Leak Detectors |||           |  /xyz/openbmc_project/detectors/leak/<DetectorY> |
                                       ||+----------------|-+           |                                                  |
                                       |+-----------------|+            +--------------------------------------------------+
                                       +------------------+
```

#### Entity Manager Configuration

The format of the entity manager configuration for leak detector is as under -

```json
"Exposes": [
  {
    "Name": "ManifoldFrontDetector",
    "Type": "LiquidLeakDetector",
    "PinName": "DETECTOR1_GPIO",
    "Polarity": "Low",
    "Zone": "Rack",
    "Action": "RackPowerOff"
  }
]
```

#### Leak Manager

The Leak Manager is responsible for processing input configuration objects from
the Entity Manager via DBus. Based on related interface signals, it creates or
deletes the corresponding zones and detectors D-Bus interfaces. Additionally, it
periodically checks the status of detector GPIOs every second and executes the
specified Target if any detector for a zone becomes enabled. It also raises or
resolves the appropriate leak-related alerts. This fulfills requirements #1 and
#2.

### Error Reporting

A
[leak detection](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/73707)
and
[missing cable](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/74397)
definition is proposed for events through Redfish based on
[event logging design](https://github.com/openbmc/docs/blob/master/designs/event-logging.md).

#### Leak Severity

The recipient of leak detection alerts can use the cooling zone to determine the
severity of the leak. Alternatively, a cooling zone can be assigned a severity
level for leaks, which can be classified as minor, major, or critical. This
severity level can then be included in the error message when reporting a leak.

## Alternatives Considered

### phosphor-gpio-monitor

The current design of the phosphor-gpio-monitor is too generic and does not meet
most of the requirements, particularly Requirement #2, #3, and #4.

## Impacts

The proposed D-Bus interface adheres to the
[Leak Detection Redfish Schema](https://redfish.dmtf.org/schemas/v1/LeakDetection.v1_0_1.json)
and would need new implementation for this service in BMCWeb.

### Organizational

- Does this repository require a new repository? Yes.
- Who will be the initial maintainer(s) of this repository? `Jagpal Singh Gill`
  & `Patrick Williams`
- Which repositories are expected to be modified to execute this design? BMCWeb
- Make a list, and add listed repository maintainers to the gerrit review.

## Testing

### Unit Testing

All the functional testing of the reference implementation will be performed
using GTest.

### Integration Testing

The end to end integration testing involving Servers (for example BMCWeb) will
be covered using openbmc-test-automation.
