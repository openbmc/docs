# GPIO Initialization Service

Author: < Delphine_CC_Chiu@wiwynn.com >

Other contributors: None

Created: 2022-10-06


## Problem Description

There are some GPIOs that are used in application layer(BMC services).

These GPIOs should be initialized in advance.
 
## Background and References

BMC sets GPIO states(direction and value) following the hardware design.

The GPIO should be set if its direction is output and not open-drain pin.

## Requirement

None

## Proposed Design
A service called gpio-init.service initializes all the gpio values from configuration file in JSON format.

    Repository Name: gpio-init
    Service: gpio-init.service
    Project JSON file: gpio-init.json

### Common Layer
There is a oneshot service "gpio-init.service" and this service starts at sysinit.target.

The gpio-init binary is called by the service to execute GPIO initialization.

The GPIO information is parsed from project JSON file and then BMC sets GPIOs through libgpiod library.

### Project Layer
There is a JSON file "gpio-init.json" that defines details of GPIOs which is required to be initialized.

This file can be replaced with a platform specific configuration file via bbappend.

The example JSON format is
```
    [
        {
            "LineName": "reset-cause-emmc",
            "GpioNum": 21,
            "ChipId": "0",
            "Direction" : "Output",
            "Value" : 0
        }
    ]
```
 - LineName: this is the line name defined in device tree for specific gpio
 - GpioNum: GPIO offset, this field is optional if LineName is defined.
 - ChipId: This is device name either offset ("0") or complete gpio device ("gpiochip0"). This field is not required if LineName is defined.
 - Direction: The GPIO direction for specific gpio
 - Value: The default value to set for specific gpio
 
