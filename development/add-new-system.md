# Add a New System to OpenBMC

**Document Purpose:** How to add a new system to the OpenBMC distribution

**Audience:** Programmer familiar with OpenBMC

**Prerequisites:** Completed Development Environment Setup [Document][32]

## Overview

**Please note:** This document is no longer officially supported by the OpenBMC
team. It still contains a lot of useful information so it has been left here.
Ideally this guide would become a standalone document (outside of the
development tree) and would cover all of the different areas that must be
updated to add a new system.

This document will describe the following:

- Review background about Yocto and BitBake
- Creating a new system layer
- Populating this new layer
- Building the new system and testing in QEMU
- Adding configs for sensors, LEDs, inventories, etc.

## Background

The OpenBMC distribution is based on [Yocto](https://www.yoctoproject.org/).
Yocto is a project that allows developers to create custom Linux distributions.
OpenBMC uses Yocto to create their embedded Linux distribution to run on a
variety of devices.

Yocto has a concept of hierarchical layers. When you build a Yocto-based
distribution, you define a set of layers for that distribution. OpenBMC uses
many common layers from Yocto as well as some of its own layers. The layers
defined within OpenBMC can be found with the meta-\* directories in OpenBMC
[GitHub](https://github.com/openbmc/openbmc).

Yocto layers are a combination of different files that define packages to
incorporate in that layer. One of the key file types used in these layers is
[BitBake](https://github.com/openembedded/bitbake/blob/master/README) recipes.

BitBake is a fully functional language in itself. For this lesson, we will focus
on only the aspects of BitBake required to understand the process of adding a
new system.

## Start the Initial BitBake

For this work, you will need to have allocated at least 100GB of space to your
development environment and as much memory and CPU as possible. The initial
build of an OpenBMC distribution can take hours. Once that first build is done
though, future builds will use cached data from the first build, speeding the
process up by orders of magnitude.

So before we do anything else, let's get that first build going.

Follow the direction on the OpenBMC GitHub
[page](https://github.com/openbmc/openbmc/blob/master/README.md#2-download-the-source)
for the Romulus system (steps 2-4).

## Create a New System

While the BitBake operation is going above, let's start creating our new system.
Similar to previous lessons, we'll be using Romulus as our reference. Our new
system will be called romulus-prime.

From your openbmc repository you cloned above, the Romulus layer is defined
within `meta-ibm/meta-romulus/`. The Romulus layer is defined within the `conf`
subdirectory. Under `conf` you will see a layout like this:

```
meta-ibm/meta-romulus/conf/
├── bblayers.conf.sample
├── conf-notes.txt
├── layer.conf
├── local.conf.sample
└── machine
    └── romulus.conf
```

To create our new romulus-prime system we are going to start out by copying our
romulus layer.

```
cp -R meta-ibm/meta-romulus meta-ibm/meta-romulus-prime
```

Let's review and modify each file needed in our new layer

1. meta-ibm/meta-romulus-prime/conf/bblayers.conf.sample

   This file defines the layers to pull into the meta-romulus-prime
   distribution. You can see in it a variety of Yocto layers (meta, meta-poky,
   meta-openembedded/meta-oe, ...). It also has OpenBMC layers like
   meta-phosphor, meta-openpower, meta-ibm, and meta-ibm/meta-romulus.

   The only change you need in this file is to change the two instances of
   meta-romulus to meta-romulus-prime. This will ensure your new layer is used
   when building your new system.

2. meta-ibm/meta-romulus-prime/conf/conf-notes.txt

   This file simply states the default target the user will build when working
   with your new layer. This remains the same as it is common for all OpenBMC
   systems.

3. meta-ibm/meta-romulus-prime/conf/layer.conf

   The main purpose of this file is to tell BitBake where to look for recipes
   (\*.bb files). Recipe files end with a `.bb` extension and are what contain
   all of the packaging logic for the different layers. `.bbappend` files are
   also recipe files but provide a way to append onto `.bb` files. `.bbappend`
   files are commonly used to add or remove something from a corresponding `.bb`
   file in a different layer.

   The only change you need in here is to find/replace the "romulus-layer" to
   "romulus-prime-layer"

4. meta-ibm/meta-romulus-prime/conf/local.conf.sample

   This file is where all local configuration settings go for your layer. The
   documentation in it is well done and it's worth a read.

   The only change required in here is to change the `MACHINE` to
   `romulus-prime`.

5. meta-ibm/meta-romulus-prime/conf/machine/romulus.conf

   This file describes the specifics for your machine. You define the kernel
   device tree to use, any overrides to specific features you will be pulling
   in, and other system specific pointers. This file is a good reference for the
   different things you need to change when creating a new system (kernel device
   tree, MRW, LED settings, inventory access, ...)

   The first thing you need to do is rename the file to `romulus-prime.conf`.

   **Note** If our new system really was just a variant of Romulus, with the
   same hardware configuration, then we could have just created a new machine in
   the Romulus layer. Any customizations for that system could be included in
   the corresponding .conf file for that new machine. For the purposes of this
   exercise we are assuming our romulus-prime system has at least a few hardware
   changes requiring us to create this new layer.

## Build New System

This will not initially compile but it's good to verify a few things from the
initial setup are done correctly.

Do not start this step until the build we started at the beginning of this
lesson has completed.

1. Modify the conf for your current build

   Within the shell you did the initial "bitbake" operation you need to reset
   the conf file for your build. You can manually copy in the new files or just
   remove it and let BitBake do it for you:

   ```
   cd ..
   rm -r ./build/conf
   . setup romulus-prime
   ```

   Run your "bitbake" command.

2. Nothing RPROVIDES 'romulus-prime-config'

   This will be your first error after running "bitbake obmc-phosphor-image"
   against your new system.

   The openbmc/skeleton repository was used for initial prototyping of OpenBMC.
   Within this repository is a
   [configs](https://github.com/openbmc/skeleton/tree/master/configs) directory.

   The majority of this config data is no longer used but until it is all
   completely removed, you need to provide it.

   Since this repository and file are on there way out, we'll simply do a quick
   workaround for this issue.

   Create a config files as follows:

   ```
   cp meta-ibm/meta-romulus-prime/recipes-phosphor/workbook/romulus-config.bb meta-ibm/meta-romulus-prime/recipes-phosphor/workbook/romulus-prime-config.bb

   vi meta-ibm/meta-romulus-prime/recipes-phosphor/workbook/romulus-prime-config.bb

   SUMMARY = "Romulus board wiring"
   DESCRIPTION = "Board wiring information for the Romulus OpenPOWER system."
   PR = "r1"

   inherit config-in-skeleton

   #Use Romulus config
   do_make_setup() {
           cp ${S}/Romulus.py \
                   ${S}/obmc_system_config.py
           cat <<EOF > ${S}/setup.py
   from distutils.core import setup

   setup(name='${BPN}',
       version='${PR}',
       py_modules=['obmc_system_config'],
       )
   EOF
   }

   ```

   Re-run your "bitbake" command.

3. Fetcher failure for URL: file://romulus.cfg

   This is the config file required by the kernel. It's where you can put some
   additional kernel config parameters. For our purposes here, just modify
   romulus-prime to use the romulus.cfg file. We just need to add the `-prime`
   to the prepend path.

   ```
   vi ./meta-ibm/meta-romulus-prime/recipes-kernel/linux/linux-aspeed_%.bbappend

   FILESEXTRAPATHS_prepend_romulus-prime := "${THISDIR}/${PN}:"
   SRC_URI += "file://romulus.cfg"
   ```

   Re-run your "bitbake" command.

4. No rule to make target arch/arm/boot/dts/aspeed-bmc-opp-romulus-prime.dtb

   The .dtb file is a device tree blob file. It is generated during the Linux
   kernel build based on its corresponding .dts file. When you introduce a new
   OpenBMC system, you need to send these kernel updates upstream. The linked
   email
   [thread](https://lists.ozlabs.org/pipermail/openbmc/2018-September/013260.html)
   is an example of this process. Upstreaming to the kernel is a lesson in
   itself. For this lesson, we will simply use the Romulus kernel config files.

   ```
   vi ./meta-ibm/meta-romulus-prime/conf/machine/romulus-prime.conf
   # Replace the ${MACHINE} variable in the KERNEL_DEVICETREE

   # Use romulus device tree
   KERNEL_DEVICETREE = "${KMACHINE}-bmc-opp-romulus.dtb"
   ```

   Re-run your "bitbake" command.

## Boot New System

And you've finally built your new system's image! There are more customizations
to be done but let's first verify what you have boots.

Your new image will be in the following location from where you ran your
"bitbake" command:

```
./tmp/deploy/images/romulus-prime/obmc-phosphor-image-romulus-prime.static.mtd
```

Copy this image to where you've set up your QEMU session and re-run the command
to start QEMU (`qemu-system-arm` command from [dev-environment.md][32]), giving
your new file as input.

Once booted, you should see the following for the login:

```
romulus-prime login:
```

There you go! You've done the basics of creating, booting, and building a new
system. This is by no means a complete system but you now have the base for the
customizations you'll need to do for your new system.

## Further Customizations

There are a lot of other areas to customize when creating a new system.

### Kernel changes

This section describes how you can make changes to the kernel to port OpenBMC to
a new machine. The device tree is in
https://github.com/openbmc/linux/tree/dev-4.13/arch/arm/boot/dts. For examples,
see [aspeed-bmc-opp-romulus.dts][1] or a similar machine. Complete the following
steps to make kernel changes:

1. Add the new machine device tree:
   - Describe the GPIOs, e.g. LED, FSI, gpio-keys, etc. You should get such info
     from schematic.
   - Describe the i2c buses and devices, which usually include various hwmon
     sensors.
   - Describe the other devices, e.g. uarts, mac.
   - Usually the flash layout does not need to change. Just include
     `openbmc-flash-layout.dtsi`.
2. Modify Makefile to build the device tree.
3. Reference to [openbmc kernel doc][31] on submitting patches to mailing list.

Note:

- In `dev-4.10`, there is common and machine-specific initialization code in
  `arch/arm/mach-aspeed/aspeed.c` which is used to do common initializations and
  perform specific settings in each machine. Starting in branch `dev-4.13`,
  there is no such initialization code. Most of the inits are done with the
  upstream clock and reset driver.
- If the machine needs specific settings (e.g. uart routing), please send mail
  to [the mailing list][2] for discussion.

### Workbook

In legacy OpenBMC, there is a "workbook" to describe the machine's services,
sensors, FRUs, etc. This workbook is a python configuration file and it is used
by other services in [skeleton][3]. In the latest OpenBMC, the skeleton services
are mostly replaced by phosphor-xxx services and thus skeleton is deprecated.
But the workbook is still needed for now to make the build.

[meta-quanta][4] is an example that defines its own config in OpenBMC tree, so
that it does not rely on skeleton repo, although it is kind of dummy.

Before [e0e69be][26], or before v2.4 tag, OpenPOWER machines use several
configurations related to GPIO. For example, in [Romulus.py][5], the
configuration details are as follows:

```python
GPIO_CONFIG['BMC_POWER_UP'] = \
        {'gpio_pin': 'D1', 'direction': 'out'}
GPIO_CONFIG['SYS_PWROK_BUFF'] = \
        {'gpio_pin': 'D2', 'direction': 'in'}

GPIO_CONFIGS = {
    'power_config' : {
        'power_good_in' : 'SYS_PWROK_BUFF',
        'power_up_outs' : [
            ('BMC_POWER_UP', True),
        ],
        'reset_outs' : [
        ],
    },
}
```

The PowerUp and PowerOK GPIOs are needed for the build to power on the chassis
and check the power state.

After that, the GPIO related configs are removed from the workbook, and replaced
by `gpio_defs.json`, e.g. [2a80da2][27] introduces the GPIO json config for
Romulus.

```json
{
    "gpio_configs": {
         "power_config": {
            "power_good_in": "SYS_PWROK_BUFF",
            "power_up_outs": [
                { "name": "SOFTWARE_PGOOD", "polarity": true},
                { "name": "BMC_POWER_UP", "polarity": true}
            ],
            "reset_outs": [
            ]
        }
    },

     "gpio_definitions": [
        {
            "name": "SOFTWARE_PGOOD",
            "pin": "R1",
            "direction": "out"
        },
        {
            "name": "BMC_POWER_UP",
            "pin": "D1",
            "direction": "out"
        },
    ...
}
```

Each machine shall define the similar json config to describe the GPIO
configurations.

### Hwmon Sensors

Hwmon sensors include sensors on board (e.g. temperature sensors, fans) and OCC
sensors. The config files path and name shall match the devices in device tree.

There is detailed document in openbmc [doc/architecture/sensor-architecture][6].

Here let's take Romulus as an example. The config files are in
[meta-romulus/recipes-phosphor/sensors][7] which includes sensors on board and
sensors of OCC, where on board sensors are via i2c and occ sensors are via FSI.

- [w83773g@4c.conf][8] defines the `w83773` temperature sensor containing 3
  temperatures:
  ```
  LABEL_temp1 = "outlet"
  ...
  LABEL_temp2 = "inlet_cpu"
  ...
  LABEL_temp3 = "inlet_io"
  ```
  This device is defined in its device tree as [w83773g@4c][9]. When BMC starts,
  the udev rule will start `phosphor-hwmon` and it will create temperature
  sensors on below DBus objects based on its sysfs attributes.
  ```
  /xyz/openbmc_project/sensors/temperature/outlet
  /xyz/openbmc_project/sensors/temperature/inlet_cpu
  /xyz/openbmc_project/sensors/temperature/inlet_io
  ```
- [pwm-tacho-controller@1e786000.conf][10] defines the fans and the config is
  similar as above, the difference is that it creates `fan_tach` sensors.
- [occ-hwmon.1.conf][11] defines the occ hwmon sensor for master CPU. This
  config is a bit different, that it shall tell `phosphor-hwmon` to read the
  label instead of directly getting the index of the sensor, because CPU cores
  and DIMMs could be dynamic, e.g. CPU cores could be disabled, DIMMs could be
  pulled out.
  ```
  MODE_temp1 = "label"
  MODE_temp2 = "label"
  ...
  MODE_temp31 = "label"
  MODE_temp32 = "label"
  LABEL_temp91 = "p0_core0_temp"
  LABEL_temp92 = "p0_core1_temp"
  ...
  LABEL_temp33 = "dimm6_temp"
  LABEL_temp34 = "dimm7_temp"
  LABEL_power2 = "p0_power"
  ...
  ```
  - The `MODE_temp* = "label"` tells that if it sees `tempX`, it shall read the
    label which is the sensor id.
  - And `LABEL_temp* = "xxx"` tells the sensor name for the corresponding sensor
    id.
  - For example, if `temp1_input` is 37000 and `temp1_label` is 91 in sysfs,
    `phosphor-hwmon` knows `temp1_input` is for sensor id 91, which is
    `p0_core0_temp`, so it creates
    `/xyz/openbmc_project/sensors/temperature/p0_core0_temp` with sensor
    value 37000.
  - For Romulus, the power sensors do not need to read label since all powers
    are available on a system.
  - For Witherspoon, the power sensors are similar to temperature sensors, that
    it shall tell hwmon to read the `function_id` instead of directly getting
    the index of the sensor.

### LEDs

Several parts are involved for LED.

1. In kernel dts, LEDs shall be described, e.g. [romulus dts][12] describes 3
   LEDs, `fault`, `identify` and `power`.

   ```
     leds {
       compatible = "gpio-leds";

       fault {
         gpios = <&gpio ASPEED_GPIO(N, 2) GPIO_ACTIVE_LOW>;
       };

       identify {
         gpios = <&gpio ASPEED_GPIO(N, 4) GPIO_ACTIVE_HIGH>;
       };

       power {
         gpios = <&gpio ASPEED_GPIO(R, 5) GPIO_ACTIVE_LOW>;
       };
     };
   ```

2. In machine layer, LEDs shall be configured via yaml to describe how it
   functions, e.g. [Romulus led yaml][28]:
   ```
   bmc_booted:
       power:
           Action: 'Blink'
           DutyOn: 50
           Period: 1000
           Priority: 'On'
   power_on:
       power:
           Action: 'On'
           DutyOn: 50
           Period: 0
           Priority: 'On'
   ...
   ```
   It tells the LED manager to set the `power` LED to blink when BMC is ready
   and booted, and set it on when host is powered on.
3. At runtime, LED manager automatically set LEDs on/off/blink based on the
   above yaml config.
4. LED can be accessed manually via /xyz/openbmc_project/led/, e.g.
   - Get identify LED state:
     ```
     curl -b cjar -k https://$bmc/xyz/openbmc_project/led/physical/identify
     ```
   - Set identify LED to blink:
     ```
     curl -b cjar -k -X PUT -H "Content-Type: application/json" -d '{"data": "xyz.openbmc_project.Led.Physical.Action.Blink" }' https://$bmc/xyz/openbmc_project/led/physical/identify/attr/State
     ```
5. When an error related to a FRU occurs, an event log is created in logging
   with a CALLOUT path. [phosphor-fru-fault-monitor][29] monitors the logs:
   - Assert the related fault LED group when a log with the CALLOUT path is
     generated;
   - De-assert the related fault LED group when the log is marked as "Resolved"
     or deleted.

**Note**: This yaml config can be automatically generated by
[phosphor-mrw-tools][13] from its MRW, see [Witherspoon example][14].

### Inventories and other sensors

Inventories, other sensors (e.g. CPU/DIMM temperature), and FRUs are defined in
ipmi's yaml config files.

E.g. [meta-romulus/recipes-phosphor/ipmi][15]

- `romulus-ipmi-inventory-map` defines regular inventories, e.g. CPU, memory,
  motherboard.
- `phosphor-ipmi-fru-properties` defines extra properties of the inventories.
- `phosphor-ipmi-sensor-inventory` defines the sensors from IPMI.
- `romulus-ipmi-inventory-sel` defines inventories used for IPMI SEL.

For inventory map and fru-properties, they are similar between different
systems, you can refer to this example and make one for your system.

For ipmi-sensor-inventory, the sensors from IPMI are different between systems,
so you need to define your own sensors, e.g.

```
0x08:
  sensorType: 0x07
  path: /org/open_power/control/occ0
  ...
0x1e:
  sensorType: 0x0C
  path: /system/chassis/motherboard/dimm0
  ...
0x22:
  sensorType: 0x07
  path: /system/chassis/motherboard/cpu0/core0
```

The first value `0x08`, `0x1e` and `0x22` are the sensor id of IPMI, which is
defined in MRW. You should follow the system's MRW to define the above config.

**Note**: The yaml configs can be automatically generated by
[phosphor-mrw-tools][13] from its MRW, see [Witherspoon example][14].

### Fans

[phosphor-fan-presence][16] manages all the services about fan:

- `phosphor-fan-presence` checks if a fan is present, creates the fan DBus
  objects in inventory and update the `Present` property.
- `phosphor-fan-monitor` checks if a fan is functional, and update the
  `Functional` property of the fan Dbus object.
- `phosphor-fan-control` controls the fan speed by setting the fan speed target
  based on conditions, e.g. temperatures.
- `phosphor-cooling-type` checks and sets if the system is air-cooled or
  water-cooled by setting properties of
  `/xyz/openbmc_project/inventory/system/chassis` object.

All the above services are configurable, e.g. by yaml config. So the machine
specific configs shall be written when porting OpenBMC to a new machine.

Taking Romulus as an example, it is air-cooled and has 3 fans without GPIO
presence detection.

#### Fan presence

Romulus has no GPIO detection for fans, so it checks fan tach sensor:

```
- name: fan0
  path: /system/chassis/motherboard/fan0
  methods:
    - type: tach
      sensors:
        - fan0
```

The yaml config tells that

- It shall create `/system/chassis/motherboard/fan0` object in inventory.
- It shall check fan0 tach sensor (`/sensors/fan_tach/fan0`) to set `Present`
  property on the fan0 object.

#### Fan monitor

Romulus fans use pwm to control the fan speed, where pwm ranges from 0 to 255,
and the fan speed ranges from 0 to about 7000. So it needs a factor and offset
to mapping the pwm to fan speed:

```
  - inventory: /system/chassis/motherboard/fan0
    allowed_out_of_range_time: 30
    deviation: 15
    num_sensors_nonfunc_for_fan_nonfunc: 1
    sensors:
      - name: fan0
        has_target: true
        target_interface: xyz.openbmc_project.Control.FanPwm
        factor: 21
        offset: 1600
```

The yaml config tells that:

1. It shall use `FanPwm` as target interface of the tach sensor.
2. It shall calculate the expected fan speed as `target * 21 + 1600`.
3. The deviation is `15%`, so if the fan speed is out of the expected range for
   more than 30 seconds, fan0 shall be set as non-functional.

#### Fan control

The fan control service requires 4 yaml configuration files:

- `zone-condition` defines the cooling zone conditions. Romulus is always
  air-cooled, so this config is as simple as defining an `air_cooled_chassis`
  condition based on the cooling type property.
  ```
  - name: air_cooled_chassis
   type: getProperty
   properties:
     - property: WaterCooled
       interface: xyz.openbmc_project.Inventory.Decorator.CoolingType
       path: /xyz/openbmc_project/inventory/system/chassis
       type: bool
       value: false
  ```
- `zone-config` defines the cooling zones. Romulus has only one zone:
  ```
  zones:
   - zone: 0
     full_speed: 255
     default_floor: 195
     increase_delay: 5
     decrease_interval: 30
  ```
  It defines that the zone full speed and default floor speed for the fans, so
  the fan pwm will be set to 255 if it is in full speed, and set to 195 if fans
  are in default floor speed.
- `fan-config` defines which fans are controlled in which zone and which target
  interface shall be used, e.g. below yaml config defines fan0 shall be
  controlled in zone0 and it shall use `FanPwm` interface.
  ```
  - inventory: /system/chassis/motherboard/fan0
   cooling_zone: 0
   sensors:
     - fan0
   target_interface: xyz.openbmc_project.Control.FanPwm
   ...
  ```
- `events-config` defines the various events and its handlers, e.g. which fan
  targets shall be set in which temperature. This config is a bit complicated,
  the [example event yaml][17] provides documents and examples. Romulus example:
  ```
   - name: set_air_cooled_speed_boundaries_based_on_ambient
     groups:
         - name: zone0_ambient
           interface: xyz.openbmc_project.Sensor.Value
           property:
               name: Value
               type: int64_t
     matches:
         - name: propertiesChanged
     actions:
         - name: set_floor_from_average_sensor_value
           map:
               value:
                   - 27000: 85
                   - 32000: 112
                   - 37000: 126
                   - 40000: 141
               type: std::map<int64_t, uint64_t>
         - name: set_ceiling_from_average_sensor_value
           map:
               value:
                   - 25000: 175
                   - 27000: 255
               type: std::map<int64_t, uint64_t>
  ```
  The above yaml config defines the fan floor and ceiling speed in
  `zone0_ambient`'s different temperatures. E.g.
  1.  When the temperature is lower than 27 degreesC, the floor speed (pwm)
      shall be set to 85.
  2.  When the temperature is between 27 and 32 degrees C, the floor speed (pwm)
      shall be set to 112, etc.

With above configs, phosphor-fan will run the fan presence/monitor/control logic
as configured specifically for the machine.

**Note**: Romulus fans are simple. For a more complicated example, refer to
[Witherspoon fan configurations][18]. The following are the additional functions
of Witherspoon fan configuration:

- It checks GPIO for fan presence.
- It checks GPIO to determine if the system is air or water cooled.
- It has more sensors and more events in fan control.

### GPIOs

This section mainly focuses on the GPIOs in device tree that shall be monitored.
E.g.:

- A GPIO may represent a signal of host checkstop.
- A GPIO may represent a button press.
- A GPIO may represent if a device is attached or not.

They are categorized as `phosphor-gpio-presence` for checking presences of a
device, and `phosphor-gpio-monitor` for monitoring a GPIO.

#### GPIOs in device tree

All the GPIOs to be monitored shall be described in the device tree. E.g.

```
  gpio-keys {
    compatible = "gpio-keys";
    checkstop {
      label = "checkstop";
      gpios = <&gpio ASPEED_GPIO(J, 2) GPIO_ACTIVE_LOW>;
      linux,code = <ASPEED_GPIO(J, 2)>;
    };
    id-button {
      label = "id-button";
      gpios = <&gpio ASPEED_GPIO(Q, 7) GPIO_ACTIVE_LOW>;
      linux,code = <ASPEED_GPIO(Q, 7)>;
    };
  };
```

The following code describes two GPIO keys, one for `checkstop` and the other
for `id-button`, where the key code is calculated from [aspeed-gpio.h][24]:

```
#define ASPEED_GPIO_PORT_A 0
#define ASPEED_GPIO_PORT_B 1
...
#define ASPEED_GPIO_PORT_Y 24
#define ASPEED_GPIO_PORT_Z 25
#define ASPEED_GPIO_PORT_AA 26
...

#define ASPEED_GPIO(port, offset) \
  ((ASPEED_GPIO_PORT_##port * 8) + offset)
```

#### GPIO Presence

Witherspoon and Zaius have examples for gpio presence.

- [Witherspoon][19]:
  ```
  INVENTORY=/system/chassis/motherboard/powersupply0
  DEVPATH=/dev/input/by-path/platform-gpio-keys-event
  KEY=104
  NAME=powersupply0
  DRIVERS=/sys/bus/i2c/drivers/ibm-cffps,3-0069
  ```
  It checks GPIO key 104 for `powersupply0`'s presence, creates the inventory
  object and bind or unbind the driver.
- [Zaius][20]:
  ```
  INVENTORY=/system/chassis/pcie_card_e2b
  DEVPATH=/dev/input/by-path/platform-gpio-keys-event
  KEY=39
  NAME=pcie_card_e2b
  ```
  It checks GPIO key 39 for `pcie_card_e2b`'s presence, and creates the
  inventory object.

#### GPIO monitor

Typical usage of GPIO monitor is to monitor the checkstop event from the host,
or button presses.

- [checkstop monitor][21] is a common service for OpenPOWER machines.
  ```
  DEVPATH=/dev/input/by-path/platform-gpio-keys-event
  KEY=74
  POLARITY=1
  TARGET=obmc-host-crash@0.target
  ```
  By default it monitors GPIO key 74, and if it is triggered, it tells systemd
  to start `obmc-host-crash@0.target`. For systems using a different GPIO pin
  for checkstop, it simply overrides the default one by specifying its own
  config file in meta-machine layer. E.g. [Zaius's checkstop config][22].
  **Note**: when the key is pressed, `phosphor-gpio-monitor` starts the target
  unit and exits.
- [id-button monitor][23] is an example service on Romulus to monitor ID button
  press.
  ```
  DEVPATH=/dev/input/by-path/platform-gpio-keys-event
  KEY=135
  POLARITY=1
  TARGET=id-button-pressed.service
  EXTRA_ARGS=--continue
  ```
  It monitors GPIO key 135 for the button press and starts
  `id-button-pressed.service`, that handles the event by setting the identify
  LED group's `Assert` property. **Note**: It has an extra argument,
  `--continue`, that tells `phosphor-gpio-monitor` to not exit and continue
  running when the key is pressed.

[1]:
  https://github.com/openbmc/linux/blob/dev-4.13/arch/arm/boot/dts/aspeed-bmc-opp-romulus.dts
[2]: https://lists.ozlabs.org/listinfo/openbmc
[3]: https://github.com/openbmc/skeleton
[4]:
  https://github.com/openbmc/openbmc/tree/master/meta-quanta/meta-q71l/recipes-phosphor/workbook
[5]: https://github.com/openbmc/skeleton/blob/master/configs/Romulus.py
[6]:
  https://github.com/openbmc/docs/blob/master/architecture/sensor-architecture.md
[7]:
  https://github.com/openbmc/openbmc/tree/master/meta-ibm/meta-romulus/recipes-phosphor/sensors
[8]:
  https://github.com/openbmc/openbmc/blob/298c4328fd20fcd7645da1565c143b1b668ef541/meta-ibm/meta-romulus/recipes-phosphor/sensors/phosphor-hwmon/obmc/hwmon/ahb/apb/i2c%401e78a000/i2c-bus%40440/w83773g%404c.conf
[9]:
  https://github.com/openbmc/linux/blob/aca92be80c008bceeb6fb62fd1d450b5be5d0a4f/arch/arm/boot/dts/aspeed-bmc-opp-romulus.dts#L208
[10]:
  https://github.com/openbmc/openbmc/blob/298c4328fd20fcd7645da1565c143b1b668ef541/meta-ibm/meta-romulus/recipes-phosphor/sensors/phosphor-hwmon/obmc/hwmon/ahb/apb/pwm-tacho-controller%401e786000.conf
[11]:
  https://github.com/openbmc/openbmc/blob/298c4328fd20fcd7645da1565c143b1b668ef541/meta-ibm/meta-romulus/recipes-phosphor/sensors/phosphor-hwmon/obmc/hwmon/devices/platform/gpio-fsi/fsi0/slave%4000--00/00--00--00--06/sbefifo1-dev0/occ-hwmon.1.conf
[12]:
  https://github.com/openbmc/linux/blob/aca92be80c008bceeb6fb62fd1d450b5be5d0a4f/arch/arm/boot/dts/aspeed-bmc-opp-romulus.dts#L42
[13]: https://github.com/openbmc/phosphor-mrw-tools
[14]:
  https://github.com/openbmc/openbmc/blob/764b88f4056cc98082e233216704e94613499e64/meta-ibm/meta-witherspoon/conf/distro/openbmc-witherspoon.conf#L4
[15]:
  https://github.com/openbmc/openbmc/tree/master/meta-ibm/meta-romulus/recipes-phosphor/ipmi
[16]: https://github.com/openbmc/phosphor-fan-presence
[17]:
  https://github.com/openbmc/phosphor-fan-presence/blob/master/control/example/events.yaml
[18]:
  https://github.com/openbmc/openbmc/tree/master/meta-ibm/meta-witherspoon/recipes-phosphor/fans
[19]:
  https://github.com/openbmc/openbmc/blob/master/meta-ibm/meta-witherspoon/recipes-phosphor/gpio/phosphor-gpio-monitor/obmc/gpio/phosphor-power-supply-0.conf
[20]:
  https://github.com/openbmc/openbmc/blob/master/meta-ingrasys/meta-zaius/recipes-phosphor/gpio/phosphor-gpio-monitor/obmc/gpio/phosphor-pcie-card-e2b.conf
[21]:
  https://github.com/openbmc/openbmc/blob/master/meta-openpower/recipes-phosphor/host/checkstop-monitor.bb
[22]:
  https://github.com/openbmc/openbmc/blob/master/meta-ingrasys/meta-zaius/recipes-phosphor/host/checkstop-monitor/obmc/gpio/checkstop
[23]:
  https://github.com/openbmc/openbmc/tree/master/meta-ibm/meta-romulus/recipes-phosphor/gpio
[24]:
  https://github.com/openbmc/linux/blob/dev-4.13/include/dt-bindings/gpio/aspeed-gpio.h
[25]: https://github.com/openbmc/docs/blob/master/development/add-new-system.md
[26]:
  https://github.com/openbmc/openbmc/commit/e0e69beab7c268e4ad98972016c78b0d7d5769ac
[27]:
  https://github.com/openbmc/openbmc/commit/2a80da2262bf13aa1ddb589cf3f2b672d26b0975
[28]:
  https://github.com/openbmc/openbmc/blob/3cce45a96f0416b4c3d8f2b698cb830662a29227/meta-ibm/meta-romulus/recipes-phosphor/leds/romulus-led-manager-config/led.yaml
[29]: https://github.com/openbmc/phosphor-led-manager/tree/master/fault-monitor
[30]: https://github.com/openbmc/docs/blob/master/development/dev-environment.md
[31]: https://github.com/openbmc/docs/blob/master/kernel-development.md
[32]: https://github.com/openbmc/docs/blob/master/development/dev-environment.md
