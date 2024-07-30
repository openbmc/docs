# Liquid Leak Detection

Author: Jagpal Singh Gill <paligill@gmail.com>

Created: July 26, 2024

## Problem Description

Liquid cooling is becoming a promising alternative to traditional air flow
cooling for high performance computing in data center environments. However,
this cooling technique comes with its own challenges as liquids can be sensitive
to electronic components. Therefore, there is an intense need for any system
using this cooling technique to detect liquid leaks and report them for
remediation actions.

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
4. Able to provision configuration for detectors on hot pluggable devices/cables
   and raise alerts if later is missing.
5. Able to produce an interface complaint with
   [Redfish Leak Detection](https://redfish.dmtf.org/schemas/v1/LeakDetection.v1_0_1.json).

## Proposed Design

### Dbus interfaces

The DBus Interface for leak detection will consist of following -

| Interface Name                                                                                                                                                                |                 Existing/New                 |    Purpose    |
| :---------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | :------------------------------------------: | :-----------: |
| [xyz.openbmc_project.LeakDetection.Zones](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/73151/1/yaml/xyz/openbmc_project/LeakDetection/Zone.interface.yaml) |             Leak Detector Zones              | New Interface |
| [xyz.openbmc_project.Object.Enable](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/73151/1/yaml/xyz/openbmc_project/Object/Enable.interface.yaml)            |     Enable/Disable Leak Detector Status      |   Existing    |
| [xyz.openbmc_project.Inventory.Item](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/73151/1/yaml/xyz/openbmc_project/Inventory/Item.interface.yaml)          | Related inventory item for the Leak Detector |   Existing    |

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

The configuration enumerates cables which are expected to be connected and needs
monitoring.

- GpioName : This is the gpio name for cable from kernel device tree.

#### cable-monitor

The cable monitor watches for cable.conf file and parses it. Based on expected
cable connectivity and gpio status it creates the cable inventory item,
otherwise raises appropriate alerts. This fulfills requirement# 3 and #4.

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

The configuration defines different leak zones alongwith detectors which form
that zone.

- Detectors : The list of detectors for the enclosing zone.
- GpioName : The gpio name for the detector status from kernel device tree.
- Depends : This field is optional. If specified, Leak Manager will only create
  this zone if specific inventory item interface exists. Otherwise, zone will be
  created by default. This fulfills the requirement# 3.
- Target : The corresponding systemd target to run if the detector is enabled.

#### Config Parser

This parses the json configuration file, performs validation and creates
configuration objects for Leak Manager input.

#### Leak Manager

Leak Manager consumes the input configuration objects from config parser. In
case of `Depends` action, it watches for interfaces added/removed signals for
specific inventory items to create/delete the specific zones and detectors D-Bus
interfaces. Additionally, it also monitors detector gpios every 1 sec and runs
specified Target and raise alerts in case any detector fora zone becomes
enabled. This fulfills requirement #1 and #2.

## Alternatives Considered

### Dbus-sensors

Refer to Background section.

### phosphor-gpio-monitor

Refer to Background section.

## Impacts

The proposed D-Bus interface adheres to the Leak Detection Redfish Schema and
would need new implementation for this service in BMCWeb.

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
