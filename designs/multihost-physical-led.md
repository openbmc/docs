# Physical LED Design Support

Author:
  Velumani T (Velu),  [velumanit@hcl](mailto:velumanit@hcl.com)
  Jayashree D (Jayashree), [jayashree-d@hcl](mailto:jayashree-d@hcl.com)

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
method and also to create LEDs under a single service, a new application is
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

 - GPIO pins for LEDs needs to be configured in the device tree.

 - To support the number of LEDs on a specific board, an entity-manager
   configuration file must be developed. In order to create a dbus object in
   the phosphor-led-sysfs daemon, this will confirm the hardware presence of
   the LEDs listed in /sys/class/leds.


## Proposed Design

The below diagram represents the overview for proposed physical LED design.

```

    DEVICE TREE          ENTITY-MANAGER                 PHOSPHOR-LED-SYSFS

   +------------+                              +-----------------------------------+
   |            |                              |    +-------------------------+    |
   | GPIO PIN 1 |                              |    | LED Controller Service  |    |
   |            |                              |    |  (xyz.openbmc_project.  |    |
   | GPIO PIN 2 |      +---------------+       |    |     LED.Controller)     |    |
   |            |      |               |       |    +-------------------------+    |
   | GPIO PIN 3 |      | Configuration |       |                 |                 |
   |            |      |    file to    |       |                 |                 |
   | GPIO PIN 4 |      |  support the  |------>|                 V                 |
   |            |      |   LEDs on a   |       |  +-----------------------------+  |
   |   .....    |      |  given board  |       |  |                             |  |
   |   .....    |      |               |       |  | Service :                   |  |
   |   .....    |      +---------------+       |  |                             |  |
   |            |                              |  | /xyz/openbmc_project/<led1> |  |
   | GPIO PIN N |                              |  | /xyz/openbmc_project/<led2> |  |
   |            |                              |  | /xyz/openbmc_project/<led3> |  |
   +------------+                              |  |        ........             |  |
                                               |  |        ........             |  |
                                               |  | /xyz/openbmc_project/<ledN> |  |
                                               |  |                             |  |
                                               |  +-----------------------------+  |
                                               +-----------------------------------+
```

Following modules will be updated for this implementation.

 - Entity Manager

 - Phosphor-led-sysfs

This document proposes a new design for physical LED implementation.

 - Physical Leds are defined in the device tree under "leds" section and
   corresponding GPIO pins are configured.

```
        leds {
                compatible = "gpio-leds";

                ledName {
                        label = "devicename:color:function";
                        gpios = <&gpio ASPEED_GPIO(M, 0) GPIO_ACTIVE_HIGH>;
                };
        };
```

 - Based on the board's configuration file, entity-manager exposes the LED
   information for a specific board available in dbus objects. ***Class, Name,
   Name1*** represents the label mentioned in device tree configuration. The
   phosphor-led-sysfs application retrieves the dbus objects for LEDs from an
   entity manager.

```
        {
            "Class": "devicename",
            "Name": "function",
            "Name1": "color",
            "Type": "SysfsLed"
        }

```

 - Phosphor-led-sysfs will have a single systemd service
   (xyz.openbmc_project.led.controller.service) running by default at
   system startup.

 - Map the LED names of the entity dbus objects with the path /sys/class/leds
   created in the hardware based on the device tree configuration. Next, the
   dbus object path and the LED interface are created under the single systemd
   service (xyz.openbmc_project.LED.Controller).


***Example***

```

   SERVICE        xyz.openbmc_project.LED.Controller

   INTERFACE      xyz.openbmc_project.LED.Physical

   OBJECT PATH

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

## Alternatives Considered

**First Approach**

"udev rules" are handled to monitor the physical LEDs events. Once the udev
event is initialized for the LED, a dbus call from udev needs to be sent to
the phosphor-led-sysfs daemon to create the dbus path and interface under a
single systemd service.

udev events will be generated based on the kernel configuration. For each LED,
it will generate events and those udev events can be generated before systemd
service, therefore, dbus call may not be supported from udev rules.

So, dbus objects from entity manager is used to establish the communication
between LEDs and the application. The association will be created for LEDs
based on the inventory objects for the given board.

**Second Approach**

When the udev event is initialized for the LED, it will save those LED names in
a file utilizing the script. Phosphor-led-sysfs will monitor the file using
inotify to handle the changes. By reading the file, all the LEDs name will be
retrieved and dbus path will be created for all the LEDs under single systemd
service.

inotify will monitor the file continuously and it needs to communicate between
udev events and phosphor-led-sysfs application. Since, udev events can be
generated before systemd service, inotify does not need to monitor the file
afterwards and also it is not a well used mechanism to communicate.

Dbus objects from entity manager is a better way to communicate with the
application after the system startup and also inventory objects can also be
created for LEDs.

## Impacts

These changes will have API impact since the dbus objects will exposes multiple
LEDs path in single service. The systemd service will have single name
***xyz.openbmc_project.LED.Controller*** instead of multiple services.

Entity Manager configuration needs to be created for the physical LEDs, which
are already existed in other platforms inorder to support this proposed design.

## Testing

The proposed design is tested in a multiple hosts platform.

***Manual Test***  - The physical LEDs status will be tested using busctl
                     commands in the hardware.

***Robot Test***   - The physical LEDs will be tested under robotframework
                     using REST API in the system LEDs suite.

***Unit Test***    - The proposed code can be covered by the unit tests which
                     are already present.
