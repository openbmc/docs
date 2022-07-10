# Physical LED Design Support

Author:
  Velumani T,  [velumanit@hcl](mailto:velumanit@hcl.com)
  Jayashree D, [jayashree-d@hcl](mailto:jayashree-d@hcl.com)

Other contributors: None

Created: July 10, 2022

## Problem Description

The existing phosphor-led-sysfs design exposes one service per LED configuration
in device tree. Hence, multiple services will be created for multiple GPIO pins
configured for LEDs.

There are cases where multiple LEDs were configured in the device tree for each
host and needs to be paired as a group of LEDs for the specified host in the
multi host platform.

For example, Power LED and System Identification LED combines into a single
bicolor blue-yellow LED per host. A total of 4 × LEDs will be placed along
the front edge of the board in a grid. The grid will be 2 × rows of 2 × LEDs
to match the layout of the card slots.

Depending on the status of each host, blue or yellow LED needs to be blink, OFF
or ON and other LEDs needs to be in OFF state. Therefore, bi-color led needs to
be paired as a group and exposed in the userspace.

Based on the current design in phopshor-led-sysfs application, pairing groups
will be difficult, since it exposes one service per LED. To abstract this
method and also to create LEDs under a single service, a new design is
proposed.

## Background and References

The below diagram represents the overview for current physical LED design.

```


    DEVICE TREE                  BMC                PHOSPHOR-LED-SYSFS SERVICE

   +------------+   Pin 1   +----------+  LED 1  +-----------------------------+
   |            |---------->|          |-------->|                             |
   | GPIO PIN 1 |   Pin 2   |          |  LED 2  | Service 1 :                 |
   |            |---------->|   UDEV   |-------->|   xyz.openbmc_project.      |
   | GPIO PIN 2 |   .....   |  Events  |  ....   |        Led.Controller.LED1  |
   |            |   .....   |          |  ....   | Service 2 :                 |
   | GPIO PIN 3 |   Pin N   |          |  LED N  |   xyz.openbmc_project.      |
   |            |---------->|          |-------->|        Led.Controller.LED2  |
   | GPIO PIN 4 |           +----------+         |          .......            |
   |            |                                |          .......            |
   |  .......   |                                | Service N :                 |
   |  .......   |                                |   xyz.openbmc_project.      |
   |  .......   |                                |        Led.Controller.LEDN  |
   |            |                                +-----------------------------+
   | GPIO PIN N |                                            |
   |            |                                            |
   +------------+                                            V
                                            +----------------------------------+
                                            |                                  |
                                            | Service 1 :                      |
                                            |    /xyz/openbmc_project/<led1>   |
                                            |                                  |
                                            | Service 2 :                      |
                                            |    /xyz/openbmc_project/<led2>   |
                                            |          .......                 |
                                            |          .......                 |
                                            |                                  |
                                            | Service N :                      |
                                            |    /xyz/openbmc_project/<ledN>   |
                                            |                                  |
                                            +----------------------------------+

                                                 PHOSPHSOR-LED-SYSFS DAEMON
```

The existing design uses sysfs entry as udev events for particular LED and
triggers the necessary action to generate the dbus object path and interface
for that specified service. It exposes one service per LED.

**Example**

```
     busctl tree xyz.openbmc_project.LED.Controller.led1
     `-/xyz
       `-/xyz/openbmc_project
         `-/xyz/openbmc_project/led
           `-/xyz/openbmc_project/led/physical
             `-/xyz/openbmc_project/led/physical/led1

     busctl tree xyz.openbmc_project.LED.Controller.led2
     `-/xyz
       `-/xyz/openbmc_project
         `-/xyz/openbmc_project/led
           `-/xyz/openbmc_project/led/physical
             `-/xyz/openbmc_project/led/physical/led2

```

## Requirements

 - Udev rules to receive LED events.
 - Support for script to save LED events in a file.
 - Read the udev events from the file in phosphor-led-sysfs daemon.
 - Provide a single systemd service for phosphor-led-sysfs application.

## Proposed Design

The below diagram represents the overview for proposed physical LED design.

```

    DEVICE TREE                  BMC                     PHOSPHOR-LED-SYSFS

   +------------+    Pin 1    +----------+              +--------------------+
   |            |------------>|          |              |                    |
   | GPIO PIN 1 |    Pin 2    |   UDEV   |              |  Method to handle  |
   |            |------------>|  Events  |------------->|     udev event     |
   | GPIO PIN 2 |     ...     |          |              |                    |
   |            |     ...     | (To Save |              +--------------------+
   | GPIO PIN 3 |    Pin N    |   LED    |                         |
   |            |------------>|  names)  |                         |
   | GPIO PIN 4 |             +----------+                         V
   |            |                                 +------------------------------+
   |   .....    |                                 |                              |
   |   .....    |      +------------------+       |  Service :                   |
   |   .....    |      |                  |       |                              |
   |            |      |  LED Controller  |       |  /xyz/openbmc_project/<led1> |
   | GPIO PIN N |      |     Service      |       |  /xyz/openbmc_project/<led2> |
   |            |      |      (xyz.       |------>|  /xyz/openbmc_project/<led3> |
   +------------+      |  openbmc_project.|       |         ........             |
                       |  LED.Controller) |       |         ........             |
                       |                  |       |  /xyz/openbmc_project/<ledN> |
                       +------------------+       |                              |
                                                  +------------------------------+

```

Since LEDS cannot pair a group under multiple services, so it is modified to
single service and groups can also be formed as per specified host's LEDs.

This document proposes a new design for physical LED implementation.

 - Physical Leds are defined in the device tree under "leds" section.

 - Corresponding GPIO pin are defined for the physical LEDs.

 - Phosphor-led-sysfs will have a single systemd service
   (xyz.openbmc_project.led.controller.service) running by default at
   system startup.

 - "udev rules" are handled to monitor the physical LEDs events.

 - Once the udev event is initialized for the LED, it will save those LED names
   in a file using the script instead of triggering the systemd service.

 - Phosphor-led-sysfs daemon will read the LED names from the file and create a
   dbus path and interface under single systemd service.

**Example**

```
     busctl tree xyz.openbmc_project.LED.Controller
     `-/xyz
       `-/xyz/openbmc_project
         `-/xyz/openbmc_project/led
           `-/xyz/openbmc_project/led/physical
             `-/xyz/openbmc_project/led/physical/led1
             `-/xyz/openbmc_project/led/physical/led2
                         ............
                         ............
             `-/xyz/openbmc_project/led/physical/ledN
```

Following modules will be updated for this implementation.

 - OpenBMC - meta-phosphor
 - Phosphor-led-sysfs

## OpenBMC - meta-phosphor

udev rules is created for physical LEDs and udev events will be created when
the LED GPIO pins are configured in the device tree.

udev events will receive the LED names which are configured in the device tree
as arguments and save those names to a file using script.

## Phosphor-led-sysfs

Phosphor-led-sysfs will have a single systemd service which will be started
running at the system startup. Once the application started, it will invoke a
method to handle the LED udev events which will be retrieved from a file.

By reading the file, all the LEDs name will be retrieved and dbus path will be
created for all the LEDs under single systemd service.

**D-Bus Objects**

```
   Service        xyz.openbmc_project.LED.Controller

   Object Path    /xyz/openbmc_project/led/physical/ledN

   Interface      xyz.openbmc_project.LED.Physical
```

## Impacts

These changes are under phosphor-led-sysfs design and this displays all the
physical LED dbus path under single systemd service instead of multiple
services.

**Note**
This will not impact the single host physical LED design, since this only
combines the LEDs dbus path under single service.

## Testing

The proposed design will be tested in both single and multiple hosts platform.
