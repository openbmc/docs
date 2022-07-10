# Physical LED Design Support

Author: Velumani T (Velu), [velumanit@hcl](mailto:velumanit@hcl.com) Jayashree D
(Jayashree), [jayashree-d@hcl](mailto:jayashree-d@hcl.com)

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
bicolor blue-yellow LED per host. A total of 4 × LEDs will be placed along the
front edge of the board in a grid. The grid will be 2 × rows of 2 × LEDs to
match the layout of the card slots.

Depending on the status of each host, blue or yellow LED needs to be Blink, OFF
or ON and other LEDs needs to be in OFF state. Therefore, bi-color LED needs to
be paired as a group and exposed in the userspace.

Based on the current design in phosphor-led-sysfs application, pairing groups
will be difficult, since it exposes one service per LED. To abstract this method
and also to create LEDs under a single service, a new application is proposed.

## Background and References

The below diagram represents the overview for current physical LED design.

```

      KERNEL                 META-PHOSPHOR           PHOSPHOR-LED-SYSFS SERVICE

   +------------+  Event 1   +----------+  LED 1  +-----------------------------+
   |            |----------->|          |-------->|                             |
   |    LED 1   |  Event 2   |          |  LED 2  | Service 1 :                 |
   |            |----------->|   UDEV   |-------->|   xyz.openbmc_project.      |
   |    LED 2   |   .....    |  Events  |  ....   |        Led.Controller.LED1  |
   |            |   .....    |          |  ....   | Service 2 :                 |
   |    LED 3   |  Event N   |          |  LED N  |   xyz.openbmc_project.      |
   |            |----------->|          |-------->|        Led.Controller.LED2  |
   |    LED 4   |            +----------+         |          .......            |
   |            |                                 |          .......            |
   |  .......   |                                 | Service N :                 |
   |  .......   |                                 |   xyz.openbmc_project.      |
   |  .......   |                                 |        Led.Controller.LEDN  |
   |            |                                 +-----------------------------+
   |    LED N   |                                             |
   |            |                                             |
   +------------+                                             V
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
triggers the necessary action to generate the dbus object path and interface for
that specified service. It exposes one service per LED.

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

- To support the systemd service to trigger script.

- Add DBUS API method for phosphor-led-sysfs daemon.

## Proposed Design

The below diagram represents the overview for proposed physical LED design.

```

      KERNEL                               PHOSPHOR-LED-SYSFS DAEMON

   +------------+  Event 1   +--------+      +---------+      +--------------+
   |            |----------> |        |      |         |      |              |
   |   LED 1    |  Event 2   |  UDEV  |      |  SYSFS  |      |  EXECUTABLE  |
   |            |----------> | Events |----->| SERVICE |----->| (DBUS Method |
   |   LED 2    |  .....     |        |      |         |      |    call)     |
   |            |  Event N   |        |      |         |      |              |
   |   LED 3    |----------> +--------+      +---------+      +--------------+
   |            |                                                  |
   |            |                                                  V
   |   LED 4    |           +------------------------------------------------------+
   |            |           |  +-----------+   +---------------------------------+ |
   |  .......   |           |  |           |   | Service :                       | |
   |  .......   |           |  |  SYSTEMD  |   |                                 | |
   |  .......   |           |  |  SERVICE  |   |  /xyz/openbmc_project/led/<led1>| |
   |            |           |  |  ( LED.   |-->|  /xyz/openbmc_project/led/<led2>| |
   |   LED N    |           |  |Controller)|   |               .....             | |
   |            |           |  |           |   |               .....             | |
   |            |           |  |           |   |  /xyz/openbmc_project/led/<ledN>| |
   +------------+           |  +-----------+   +---------------------------------+ |
                            +------------------------------------------------------+

```

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

- Dbus API method will be created in the phosphor-led-sysfs repository under
  **_xyz.openbmc_project.Led.Sysfs.Internal_** interface name to add or remove
  the LED, which will be coming from each udev LED event.

```
    NAME                                   TYPE      SIGNATURE  RESULT/VALUE  FLAGS

    xyz.openbmc_project.Led.Sysfs.Internal interface -          -             -
    .AddLED                                method    s          -             -
    .RemoveLED                             method    s          -             -

```

- udev event will be triggered for each LED and service will be invoked to run
  the executable for each LED event.

- Executable will call Dbus API method in the phosphor-led-sysfs repository to
  pass LED name as argument from udev, after the primary service started.

- Phosphor-led-sysfs will have a single systemd service (primary service)
  **_(xyz.openbmc_project.LED.Controller.service)_** running by default at
  system startup.

- Once Dbus API method call invoked, phosphor-led-sysfs daemon will retrieve the
  LED name as parameter and map the name with /sys/class/leds path in the
  hardware. Next, the dbus pbject path and interface will be created under
  single systemd service (xyz.openbmc_project.LED.Controller).

- The schema will be implemented in the phosphor-led-sysfs daemon. A group of
  LEDs is paired for each host and the schema handles color control for that
  group to activate one LED which must be Blink/ON/OFF state and set the
  remaining LEDs in the group as OFF state.

**_Example_**

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

**Approach 1**

When the udev event is initialized for the LED, it will save those LED names in
a file utilizing the script. Phosphor-led-sysfs will monitor the file using
inotify to handle the changes. By reading the file, all the LEDs name will be
retrieved and dbus path will be created for all the LEDs under single systemd
service.

inotify will monitor the file continuously and it needs to communicate between
udev events and phosphor-led-sysfs application. Since, udev events can be
generated before systemd service, inotify does not need to monitor the file
afterwards and also it is not a well used mechanism to communicate.

Executable for Dbus method is a better way to communicate with the application
after the system startup and populate the dbus path for LEDs under single
service.

## Impacts

These changes will have API impact since the dbus objects will exposes multiple
LEDs path in single service. The systemd service will have single name
**_xyz.openbmc_project.LED.Controller_** instead of multiple services.

## Testing

The proposed design is tested in a multiple hosts platform.

**_Manual Test_** - The physical LEDs status will be tested using busctl
commands in the hardware.

**_Robot Test_** - The physical LEDs will be tested under robotframework using
REST API in the system LEDs suite.

**_Unit Test_** - The proposed code can be covered by the unit tests which are
already present.
