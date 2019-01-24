- [Porting Guide](#porting-guide)
  * [Porting to a new machine](#porting-to-a-new-machine)
    + [Add machine layer](#add-machine-layer)
    + [Kernel changes](#kernel-changes)
    + [Workbook](#workbook)
    + [Misc](#misc)
      - [Hwmon Sensors](#hwmon-sensors)
      - [LEDs](#leds)
      - [Inventories and other sensors](#inventories-and-other-sensors)
      - [Fans](#fans)
        * [Fan presence](#fan-presence)
        * [Fan monitor](#fan-monitor)
        * [Fan control](#fan-control)
      - [GPIOs](#gpios)
        * [GPIOs in device tree](#gpios-in-device-tree)
        * [GPIO Presence](#gpio-presence)
        * [GPIO monitor](#gpio-monitor)

---

# Porting Guide

In this article, it introduces the guide of how to port OpenBMC to a new
machine, including changes to openbmc layers, linux kernel, and several
components related to hwmon sensor, LED, inventory, etc.


## Porting to a new machine

To port OpenBMC to a new machine, usually it includes:
1. Add new machine's layer in meta-openbmc
2. Add new machine's kernel changes, e.g. configs, dts
3. Add new machine's Workbook
4. Make the required changes that are specific to the new machine, e.g. hwmon
   sensor, LED, etc.


**Note**: The above 1~3 could be referenced from [add-new-system.md][25] for
step-by-step tutorial.


### Add machine layer

Let's take an example of adding machine `machine-name-2` with manufacture
`manufacturer-1`.

1. To create the layer, you can either reuse the existing repository
   `meta-manufacturer-1`, or create a new repository.
2. Create `meta-machine-name-2` under `meta_manufacturer-1` as a directory for
   the machine
3. Use machine name `machine-name-2` instead of `manufacturer-1` in config
   files
4. Create a conf dir in `meta_manufacturer-1`, following `meta-ibm/conf`
5. So the final directory tree looks like below:
   ```
   meta-manufacturer-1/
   ├── conf
   │   └── layer.conf
   └── meta-machine-name-2
       ├── conf
       │   ├── bblayers.conf.sample
       │   ├── conf-notes.txt
       │   ├── layer.conf
       │   ├── local.conf.sample
       │   └── machine
       │       └── machine-name-2.conf
       ├── recipes-kernel
       │   └── linux
       │       ├── linux-obmc
       │       │   └── machine-name-2.cfg  # Machine specific kernel configs
       │       └── linux-obmc_%.bbappend
       └── recipes-phosphor
           ...
           ├── images
           │   └── obmc-phosphor-image.bbappend  # Machine specfic apps/services to include
           └── workbook
               ├── machine-name-2-config
               │   └── Machine-name-2.py  # Machine specific workbook (see below)
               └── machine-name-2-config.bb
   ```

The above directory tree creates a new layer in the machine, so you can build
with:
```
TEMPLATECONF=meta-manufacturer-1/meta-machine-name-2/conf . oe-init-build-env
bitbake obmc-phosphor-image
```


### Kernel changes

This section describes how you can make changes to the kernel to port OpenBMC
to a new machine.
The device tree is in https://github.com/openbmc/linux/tree/dev-4.13/arch/arm/boot/dts.
For examples, see [aspeed-bmc-opp-romulus.dts][1] or a similar machine.
Complete the following steps to make kernel changes:

1. Add the new machine device tree:
   * Describe the GPIOs, e.g. LED, FSI, gpio-keys, etc. You should get such
     info from schematic.
   * Describe the i2c buses and devices, which usually include various hwmon
     sensors.
   * Describe the other devices, e.g. uarts, mac.
   * Usually the flash layout does not need change, just include
     `openbmc-flash-layout.dtsi` is OK.
2. Modify Makefile to build the device tree.

Note:
* In `dev-4.10`, there is common and machine-specific initialization code in
  `arch/arm/mach-aspeed/aspeed.c` which is used to do common initializations
  and perform specific settings in each machine.
* From `dev-4.13`, there is no such code anymore, most of the inits are done
  with the upstream clock and reset driver.
* So if the machine really needs specific settings (e.g. uart routing), please
  send mail to [the mailing list][2] for discussion.


### Workbook

In legacy OpenBMC, there is a "workbook" to describe the machine's services,
sensors, FRUs, etc.
It is a python config and is used by other services in [skeleton][3].
In the latest OpenBMC, they are mostly replaced by phosphor-xxx services and
thus skeleton is deprecated.

But workbook is still needed for now to make the build.

An example is [meta-quanta][4], that defines its own config in OpenBMC tree,
so that it does not rely on skeleton repo, although it is kind of dummy.

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

After that, the GPIO related configs are removed from the workbook, and
replaced by `gpio_defs.json`, e.g. [2a80da2][27] introduces the GPIO json
config for Romulus.

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

### Misc

Different machines have different devices, e.g. hwmon sensors, leds, fans.

OpenBMC is designed to be configurable, so you could describe such devices in
different config files in the machine layer.


#### Hwmon Sensors

Hwmon sensors include sensors on board (e.g. temperature sensors, fans) and
OCC sensors.
The config files path and name shall match the devices in device tree.

There is detailed document in openbmc [doc/sensor-architecture][6].

Here let's take Romulus as an example.
The config files are in [meta-romulus/recipes-phosphor/sensors][7] which
includes sensors on board and sensors of OCC, where on board sensors are via
i2c and occ sensors are via FSI.

* [w83773g@4c.conf][8] defines the `w83773` temperature sensor containing 3
temperatures:
   ```
   LABEL_temp1 = "outlet"
   ...
   LABEL_temp2 = "inlet_cpu"
   ...
   LABEL_temp3 = "inlet_io"
   ```
   This device is defined in its device tree as [w83773g@4c][9].
   When BMC starts, the udev rule will start `phosphor-hwmon` and it will create
   temperature sensors on below DBus objects based on its sysfs attributes.
   ```
   /xyz/openbmc_project/sensors/temperature/outlet
   /xyz/openbmc_project/sensors/temperature/inlet_cpu
   /xyz/openbmc_project/sensors/temperature/inlet_io
   ```
* [pwm-tacho-controller@1e786000.conf][10] defines the fans and the config is
   similar as above, the difference is that it creates `fan_tach` sensors.
* [occ-hwmon.1.conf][11] defines the occ hwmon sensor for master CPU.
   This config is a bit different, that it shall tell `phosphor-hwmon` to read
   the label instead of directly getting the index of the sensor, because CPU
   cores and DIMMs could be dynamic, e.g. CPU cores could be disabled, DIMMs
   could be pulled out.
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
   * The `MODE_temp* = "label"` tells that if it sees `tempX`, it shall read
      the label which is the sensor id.
   * And `LABEL_temp* = "xxx"` tells the sensor name for the corresponding
      sensor id.
   * For example, if `temp1_input` is 37000 and `temp1_label` is 91 in sysfs,
      `phosphor-hwmon` knows `temp1_input` is for sensor id 91, which is
      `p0_core0_temp`, so it creates
      `/xyz/openbmc_project/sensors/temperature/p0_core0_temp` with sensor
      value 37000.
   * For Romulus, the power sensors do not need to read label since all powers
      are available on a system.
   * For Witherspoon, the power sensors are similar to temperature sensors,
      that it shall tell hwmon to read the `function_id` instead of directly
      getting the index of the sensor.


#### LEDs

Several parts are involved for LED.

1. In kernel dts, LEDs shall be described, e.g. [romulus dts][12] describes
   3 LEDs, `fault`, `identify` and `power`.
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
   * Get identify LED state:
      ```
      curl -b cjar -k https://$bmc/xyz/openbmc_project/led/physical/identify
      ```
   * Set identify LED to blink:
      ```
      curl -b cjar -k -X PUT -H "Content-Type: application/json" -d '{"data": "xyz.openbmc_project.Led.Physical.Action.Blink" }' https://$bmc/xyz/openbmc_project/led/physical/identify/attr/State
      ```
5. When an error related to a FRU occurs, an event log is created in logging
   with a CALLOUT path. [phosphor-fru-fault-monitor][29] monitors the logs:
   * Assert the related fault LED group when a log with the CALLOUT path is
      generated;
   * De-assert the related fault LED group when the log is marked as
      "Resolved" or deleted.

**Note**: This yaml config can be automatically generated by
[phosphor-mrw-tools][13] from its MRW, see [Witherspoon example][14].


#### Inventories and other sensors

Inventories, other sensors (e.g. CPU/DIMM temperature), and FRUs are defined
in ipmi's yaml config files.

E.g. [meta-romulus/recipes-phosphor/ipmi][15]
* `romulus-ipmi-inventory-map` defines regular inventories, e.g. CPU, memory,
   motherboard.
* `phosphor-ipmi-fru-properties` defines extra properties of the inventories.
* `phosphor-ipmi-sensor-inventory` defines the sensors from IPMI.
* `romulus-ipmi-inventory-sel` defines inventories used for IPMI SEL.

For inventory map and fru-properties, they are similar between different
systems, you can refer to this example and make one for your system.

For ipmi-sensor-inventory, the sensors from IPMI are different between
systems, so you need to define your own sensors, e.g.
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
defined in MRW.
You should follow the system's MRW to define the above config.

**Note**: The yaml configs can be automatically generated by
[phosphor-mrw-tools][13] from its MRW, see [Witherspoon example][14].


#### Fans
[phosphor-fan-presence][16] manages all the services about fan:
* `phosphor-fan-presence` checks if a fan is present, creates the fan DBus
   objects in inventory and update the `Present` property.
* `phosphor-fan-monitor` checks if a fan is functional, and update the
   `Functional` property of the fan Dbus object.
* `phosphor-fan-control` controls the fan speed by setting the fan speed target
   based on conditions, e.g. temperatures.
* `phosphor-cooling-type` checks and sets if the system is air-cooled or
   water-cooled by setting properties of
   `/xyz/openbmc_project/inventory/system/chassis` object.

All the above services are configurable, e.g. by yaml config.
So the machine specific configs shall be written when porting OpenBMC to a new
machine.

Taking Romulus as an example, it is air-cooled and has 3 fans without GPIO
presence detection.

##### Fan presence
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
* It shall create `/system/chassis/motherboard/fan0` object in inventory.
* It shall check fan0 tach sensor (`/sensors/fan_tach/fan0`) to set `Present`
   property on the fan0 object.

##### Fan monitor
Romulus fans use pwm to control the fan speed, where pwm ranges from 0 to 255,
and the fan speed ranges from 0 to about 7000.
So it needs a factor and offset to mapping the pwm to fan speed:
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
1. It shall use `FanPwm` as target interface of the tach sensor;
2. It shall calculate the expected fan speed as `target * 21 + 1600`
3. The deviation is `15%`, so if the fan speed is out of the expected range
   for more than 30 seconds, fan0 shall be set as non-functional.

##### Fan control
The fan control service requires 4 yaml configuration files:
* `zone-condition` defines the cooling zone conditions. Romulus is always
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
* `zone-config` defines the cooling zones. Romulus has only one zone:
   ```
  zones:
    - zone: 0
      full_speed: 255
      default_floor: 195
      increase_delay: 5
      decrease_interval: 30
   ```
   It defines that the zone full speed and default floor speed for the fans,
   so the fan pwm will be set to 255 if it is in full speed, and set to 195 if
   fans are in default floor speed.
* `fan-config` defines which fans are controlled in which zone and which target
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
* `events-config` defines the various events and its handlers, e.g. which fan
   targets shall be set in which temperature.
   This config is a bit complicated, the [exmaple event yaml][17] provides
   documents and examples.
   Romulus example:
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
   1. When the temperature is lower than 27 degreesC, the floor speed (pwm)
      shall be set to 85.
   2. When the temperature is between 27 and 32 degrees C, the floor speed
      (pwm) shall be set to 112, etc.

With above configs, phosphor-fan will run the fan presence/monitor/control
logic as configured specifically for the machine.

**Note**: Romulus fans are simple. For a more complicated example, refer to
[Witherspoon fan configurations][18]. The following are the additional
functions of Witherspoon fan configuration:

* It checks GPIO for fan presence.
* It checks GPIO to determine if the system is air or water cooled.
* It has more sensors and more events in fan control.


#### GPIOs
This section mainly focuses on the GPIOs in device tree that shall be
monitored.
E.g.:
* A GPIO may represent a signal of host checkstop;
* A GPIO may represent a button press;
* A GPIO may represent if a device is attached or not.

They are categorized as `phosphor-gpio-presence` for checking presences of a
device, and `phosphor-gpio-monitor` for monitoring a GPIO.

##### GPIOs in device tree
All the GPIOs to be monitored shall be described in the device tree.
E.g.
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

##### GPIO Presence
Witherspoon and Zaius have examples for gpio presence.

* [Witherspoon][19]:
   ```
   INVENTORY=/system/chassis/motherboard/powersupply0
   DEVPATH=/dev/input/by-path/platform-gpio-keys-event
   KEY=104
   NAME=powersupply0
   DRIVERS=/sys/bus/i2c/drivers/ibm-cffps,3-0069
   ```
   It checks GPIO key 104 for `powersupply0`'s presence, creates the inventory
   object and bind or unbind the driver.
* [Zaius][20]:
   ```
   INVENTORY=/system/chassis/pcie_card_e2b
   DEVPATH=/dev/input/by-path/platform-gpio-keys-event
   KEY=39
   NAME=pcie_card_e2b
   ```
   It checks GPIO key 39 for `pcie_card_e2b`'s presence, and creates the
   inventory object.

##### GPIO monitor
Typical usage of GPIO monitor is to monitor the checkstop event from the host,
or button presses.

* [checkstop monitor][21] is a common service for OpenPOWER machines.
   ```
   DEVPATH=/dev/input/by-path/platform-gpio-keys-event
   KEY=74
   POLARITY=1
   TARGET=obmc-host-crash@0.target
   ```
   By default it monitors GPIO key 74, and if it is triggered, it tells
   systemd to start `obmc-host-crash@0.target`.
   For systems using a different GPIO pin for checkstop, it simply overrides
   the default one by specifying its own config file in meta-machine layer.
   E.g. [Zaius's checkstop config][22].
   **Note**: when the key is pressed, `phosphor-gpio-monitor` starts the target
   unit and exits.
* [id-button monitor][23] is an example service on Romulus to monitor ID
   button press.
   ```
   DEVPATH=/dev/input/by-path/platform-gpio-keys-event
   KEY=135
   POLARITY=1
   TARGET=id-button-pressed.service
   EXTRA_ARGS=--continue
   ```
   It monitors GPIO key 135 for the button press and starts
   `id-button-pressed.service`, that handles the event by setting the identify
   LED group's `Assert` property.
   **Note**: It has an extra argument, `--continue`, that tells
   `phosphor-gpio-monitor` to not exit and continue running when the key is
   pressed.


[1]: https://github.com/openbmc/linux/blob/dev-4.13/arch/arm/boot/dts/aspeed-bmc-opp-romulus.dts
[2]: https://lists.ozlabs.org/listinfo/openbmc
[3]: https://github.com/openbmc/skeleton
[4]: https://github.com/openbmc/openbmc/tree/master/meta-quanta/meta-q71l/recipes-phosphor/workbook
[5]: https://github.com/openbmc/skeleton/blob/master/configs/Romulus.py
[6]: https://github.com/openbmc/docs/blob/master/sensor-architecture.md
[7]: https://github.com/openbmc/openbmc/tree/master/meta-ibm/meta-romulus/recipes-phosphor/sensors
[8]: https://github.com/openbmc/openbmc/blob/298c4328fd20fcd7645da1565c143b1b668ef541/meta-ibm/meta-romulus/recipes-phosphor/sensors/phosphor-hwmon/obmc/hwmon/ahb/apb/i2c%401e78a000/i2c-bus%40440/w83773g%404c.conf
[9]: https://github.com/openbmc/linux/blob/aca92be80c008bceeb6fb62fd1d450b5be5d0a4f/arch/arm/boot/dts/aspeed-bmc-opp-romulus.dts#L208
[10]: https://github.com/openbmc/openbmc/blob/298c4328fd20fcd7645da1565c143b1b668ef541/meta-ibm/meta-romulus/recipes-phosphor/sensors/phosphor-hwmon/obmc/hwmon/ahb/apb/pwm-tacho-controller%401e786000.conf
[11]: https://github.com/openbmc/openbmc/blob/298c4328fd20fcd7645da1565c143b1b668ef541/meta-ibm/meta-romulus/recipes-phosphor/sensors/phosphor-hwmon/obmc/hwmon/devices/platform/gpio-fsi/fsi0/slave%4000--00/00--00--00--06/sbefifo1-dev0/occ-hwmon.1.conf
[12]: https://github.com/openbmc/linux/blob/aca92be80c008bceeb6fb62fd1d450b5be5d0a4f/arch/arm/boot/dts/aspeed-bmc-opp-romulus.dts#L42
[13]: https://github.com/openbmc/phosphor-mrw-tools
[14]: https://github.com/openbmc/openbmc/blob/764b88f4056cc98082e233216704e94613499e64/meta-ibm/meta-witherspoon/conf/distro/openbmc-witherspoon.conf#L4
[15]: https://github.com/openbmc/openbmc/tree/master/meta-ibm/meta-romulus/recipes-phosphor/ipmi
[16]: https://github.com/openbmc/phosphor-fan-presence
[17]: https://github.com/openbmc/phosphor-fan-presence/blob/master/control/example/events.yaml
[18]: https://github.com/openbmc/openbmc/tree/master/meta-ibm/meta-witherspoon/recipes-phosphor/fans
[19]: https://github.com/openbmc/openbmc/blob/master/meta-ibm/meta-witherspoon/recipes-phosphor/gpio/phosphor-gpio-monitor/obmc/gpio/phosphor-power-supply-0.conf
[20]: https://github.com/openbmc/openbmc/blob/master/meta-ingrasys/meta-zaius/recipes-phosphor/gpio/phosphor-gpio-monitor/obmc/gpio/phosphor-pcie-card-e2b.conf
[21]: https://github.com/openbmc/openbmc/blob/master/meta-openpower/recipes-phosphor/host/checkstop-monitor.bb
[22]: https://github.com/openbmc/openbmc/blob/master/meta-ingrasys/meta-zaius/recipes-phosphor/host/checkstop-monitor/obmc/gpio/checkstop
[23]: https://github.com/openbmc/openbmc/tree/master/meta-ibm/meta-romulus/recipes-phosphor/gpio
[24]: https://github.com/openbmc/linux/blob/dev-4.13/include/dt-bindings/gpio/aspeed-gpio.h
[25]: https://github.com/openbmc/docs/blob/master/development/add-new-system.md
[26]: https://github.com/openbmc/openbmc/commit/e0e69beab7c268e4ad98972016c78b0d7d5769ac
[27]: https://github.com/openbmc/openbmc/commit/2a80da2262bf13aa1ddb589cf3f2b672d26b0975
[28]: https://github.com/openbmc/openbmc/blob/3cce45a96f0416b4c3d8f2b698cb830662a29227/meta-ibm/meta-romulus/recipes-phosphor/leds/romulus-led-manager-config/led.yaml
[29]: https://github.com/openbmc/phosphor-led-manager/tree/master/fault-monitor
