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

In this document, a leak is considered to be an entity with digital value.
Currently, openBMC has a framework for sensors through
[DBus-sensors](https://github.com/openbmc/dbus-sensors), but it is primarily
designed for numerical readings rather than detectors with digital values. The
[phosphor-gpio-monitor](https://github.com/openbmc/phosphor-gpio-monitor) is too
generic in its design and does not meet most of the requirements.

## Requirements

1. Able to detect a leak presence and raise alerts.

- Leak detectors can be connected to the BMC or installed on a hot-pluggable
  device/cable.

2. Able to classify leaks based on severity levels and take appropriate
   remediation actions.
3. Capable of detecting the presence of a configured hot-pluggable device or
   cable for a leak detector and raising alerts if former is missing.
4. Able to produce an interface complaint with
   [Redfish Leak Detection](https://redfish.dmtf.org/schemas/v1/LeakDetection.v1_0_1.json).

## Proposed Design

### Dbus interfaces

The DBus Interface for leak detection will consist of following -

| Interface Name                                                                                                                                                                |       Existing/New        |                      Purpose                      |
| :---------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | :-----------------------: | :-----------------------------------------------: |
| [xyz.openbmc_project.LeakDetection.Zones](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/73151/1/yaml/xyz/openbmc_project/LeakDetection/Zone.interface.yaml) |            New            |           Implement Leak Detector Zones           |
| [xyz.openbmc_project.Object.Enable](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/73151/1/yaml/xyz/openbmc_project/Object/Enable.interface.yaml)            | Existing (with additions) |          Implement Leak Detector Status           |
| [xyz.openbmc_project.Inventory.Item](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/73151/1/yaml/xyz/openbmc_project/Inventory/Item.interface.yaml)          | Existing (with additions) | Implement inventory item related to Leak Detector |

### Cable Monitor

```
         +--------------+       +------------------+
         |              |       |                  |   creates     +--------------------------------------------------+
         |  cable.conf  |------>|  cable-monitor   |-------------->|    xyz.openbmc_project.Inventory.Item.Cable      |
         |              |       |                  |               +--------------------------------------------------+
         +--------------+       +------------------+
```

#### cable.conf

The format of the configuration is as under -

```json
{
  "cable0": {
    "GpioName": "Cable0_GPIO"
  }
}
```

The configuration lists the cables that should be connected and monitored.

- GpioName : This field specifies the GPIO name for the cable as defined in the
  kernel device tree.

#### cable-monitor

The cable monitor is responsible for monitoring the cable.conf file and parsing
its contents. Based on the expected cable connectivity and GPIO status, it
creates a cable inventory item. If there are any discrepancies between the
expected and actual cable connectivity or GPIO status, the cable monitor raises
appropriate alerts. This fulfills requirement #3.

### Leak Detector

```

           +---------------------+
           | leak-detector.conf  |
           +----------|----------+
                      |
                      |
                      v                                                                                   +--------------------------------------------------+
           +--------------------+       +------------------+             +---------------------+          |    Create Interface                              |
           |                    |       |                  |   creates   |                     |          |    xyz.openbmc_project.LeakDetection.Zones       |
           |    Config Parser   |------>|   Leak Manager   |------------>| Leak Detector Zones |--------->|    at                                            |
           |                    |       |                  |             |                     |          |    /xyz/openbmc_project/leak_detection/<ZoneX>   |
           +--------------------+       +------------------+             +---------------------+          |                                                  |
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

#### leak-detector.conf

The format of the leak detector configuration file is as under -

```json
{
  "SmallLeak": {
    "Detectors": {
      "detector1": {
        "GpioName": "DETECTOR1_GPIO"
      }
    },
    "Depends": {
      "Interface": "xyz.openbmc_project.Inventory.Item.Cable",
      "Inventory": "/system/chassis/motherboard/cable0"
    },
    "Target": "TrayPowerOff.target"
  }
}
```

The configuration specifies various leak zones and their associated detectors.
Each zone is defined by the following properties:

- Detectors: A list of detectors that belong to the enclosing zone.
- GpioName: The name of the GPIO pin used for the detector status from the
  kernel device tree.
- Depends (optional): If specified, Leak Manager will only create this zone if a
  specific inventory item interface exists. Otherwise, the zone will be created
  by default.
- Target: The corresponding systemd target to run when the detector is enabled.

#### Config Parser

This module reads and validates the JSON configuration file, then generates
configuration objects for Leak Manager input.

#### Leak Manager

The Leak Manager is responsible for processing input configuration objects
provided by the config parser. In the case of the `Depends` action, it
continuously monitors for signals indicating the addition or removal of
interfaces for specific inventory items. Based on these signals, the Leak
Manager creates or deletes the corresponding zones and detectors D-Bus
interfaces. Additionally, it periodically checks the status of detector GPIOs
every second and executes the specified Target if any detector for a zone
becomes enabled. This fulfills requirements #1 and #2.

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
