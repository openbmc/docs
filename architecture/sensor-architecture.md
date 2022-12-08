# Sensor Support for OpenBMC using phosphor-hwmon

This document describes sensors provided by [phosphor-hwmon][15]. An alternate
method is to use the suite of applications provided by [dbus-sensors][16]. While
the configuration details between the two methods differ, the D-Bus
representation remains mostly the same.

OpenBMC makes it easy to add sensors for your hardware and is compliant with the
traditional Linux HWMon sensor format. The architecture of OpenBMC sensors is to
map sensors to [D-Bus][1] objects. The D-Bus object will broadcast the
`PropertiesChanged` signal when either the sensor or threshold value changes. It
is the responsibility of other applications to determine the effect of the
signal on the system.

## D-Bus

```
Service     xyz.openbmc_project.Hwmon-<hash>.Hwmon1
Path        /xyz/openbmc_project/sensors/<type>/<label>
Interfaces  xyz.openbmc_project.Sensor.[*], others (see below)

Signals: All properties for an interface will broadcast signal changed
```

**Path definitions**

- **<type\>** : The [HWMon class][2] name in lower case.

  - Examples include `temperature, fan_tach, voltage`.

- **<label\>** : User defined name of the sensor.
  - Examples include `ambient, cpu0, fan5`

**Note**: The label shall comply with "Valid Object Paths" of [D-Bus Spec][3],
that shall only contain the ASCII characters "[A-Z][a-z][0-9]\_".

**Hash definition**

The hash value in the service name is used to give the service a unique and
stable name. It is a decimal number that is obtained by hashing characteristics
of the device it is monitoring using std::hash().

## Redfish

The [BMCWeb Redfish][10] support returns information about sensors. The support
depends on two types of [ObjectMapper associations][11] to find the necessary
sensor information on D-Bus.

### Association Type #1: Linking a chassis to all sensors within the chassis

Sensors are grouped by chassis in Redfish. An ObjectMapper association is used
to link a chassis to all the sensors within the chassis. This includes the
sensors for all hardware that is considered to be within the chassis. For
example, a chassis might contain two fan sensors, an ambient temperature sensor,
and a VRM output voltage sensor.

#### D-Bus object paths

The association links the following D-Bus object paths together:

- Chassis inventory item object path
- List of sensor object paths for sensors within the chassis

#### Association names

- "all_sensors"
  - Contains the list of all sensors for this chassis
- "chassis"
  - Contains the chassis associated with this sensor

#### Example associations

- /xyz/openbmc_project/inventory/system/chassis/all_sensors
  - "endpoints" property contains
    - /xyz/openbmc_project/sensors/fan_tach/fan0_0
    - /xyz/openbmc_project/sensors/fan_tach/fan0_1
    - /xyz/openbmc_project/sensors/temperature/ambient
    - /xyz/openbmc_project/sensors/voltage/p0_vdn_voltage
- /xyz/openbmc_project/sensors/fan_tach/fan0_0/chassis
  - "endpoints" property contains
    - /xyz/openbmc_project/inventory/system/chassis

### Association Type #2: Linking a low-level hardware item to its sensors

A sensor is usually related to a low-level hardware item, such as a fan, power
supply, VRM, or CPU. The Redfish sensor support can obtain the following
information from the related hardware item:

- Presence ([Inventory.Item interface][12])
- Functional state ([OperationalStatus interface][13])
- Manufacturer, Model, PartNumber, SerialNumber ([Decorator.Asset
  interface][14])

For this reason, an ObjectMapper association is used to link a low-level
hardware item to its sensors. For example, a processor VRM could have
temperature and output voltage sensors, or a dual-rotor fan could have two tach
sensors.

#### D-Bus object paths

The association links the following D-Bus object paths together:

- Low-level hardware inventory item object path
- List of sensor object paths for sensors related to that hardware item

#### Association names

- "sensors"
  - Contains the list of sensors for this low-level hardware item
- "inventory"
  - Contains the low-level hardware inventory item for this sensor

#### Example associations

- /xyz/openbmc_project/inventory/system/chassis/motherboard/fan0/sensors
  - "endpoints" property contains
    - /xyz/openbmc_project/sensors/fan_tach/fan0_0
    - /xyz/openbmc_project/sensors/fan_tach/fan0_1
- /xyz/openbmc_project/sensors/fan_tach/fan0_0/inventory
  - "endpoints" property contains
    - /xyz/openbmc_project/inventory/system/chassis/motherboard/fan0

## Development Details

Sensor properties are standardized based on the type of sensor. A Threshold
sensor contains specific properties associated with the rise and fall of a
sensor value. The [Sensor Interfaces][4] are described in their respective YAML
files. The path location in the source tree is identical to the interface being
described below the [phosphor-dbus-interfaces][5] parent directory.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;example:
[openbmc/phosphor-dbus-interfaces/xyz/openbmc_project/Sensor/Threshold/Warning.yaml][6]

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Maps to D-Bus interface
`xyz.openbmc_project.Sensor.Threshold.Warning`

Each 'name' property in the YAML file maps directly to D-Bus properties.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;example:

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Warning.interface.yaml][6]

```
properties:
    - name: WarningHigh
      type: int64
    - name: WarningLow
      type: int64
    - name: WarningAlarmHigh
      type: boolean
    - name: WarningAlarmLow
      type: boolean
```

Maps to

```
busctl --system introspect xyz.openbmc_project.Hwmon-3301914901.Hwmon1 \
 /xyz/openbmc_project/Sensors/temperature/ambient \
 xyz.openbmc_project.Sensor.Threshold.Warning | grep property

.WarningAlarmHigh                            property  b         false        emits-change writable
.WarningAlarmLow                             property  b         false        emits-change writable
.WarningHigh                                 property  x         40000        emits-change writable
.WarningLow                                  property  x         10000        emits-change writable

```

### REST

```
"/xyz/openbmc_project/Sensors/temperature/ambient": {
      "Scale": -3,
      "Unit": "xyz.openbmc_project.Sensor.Value.Unit.DegreesC",
      "Value": 30125,
      "WarningAlarmHigh": 0,
      "WarningAlarmLow": 0,
      "WarningHigh": 40000,
      "WarningLow": 10000
}
```

### Other Interfaces

Aside from the `xyz.openbmc_project.Sensor` interfaces, the sensor D-Bus objects
may also expose the following interfaces:

1.  `xyz.openbmc_project.Control.FanSpeed`
    - Provides a Target property to set a fan RPM value
2.  `xyz.openbmc_project.Control.FanPwm`
    - Provides a Target property to set a fan PWM value
3.  `xyz.openbmc_project.State.Decorator.OperationalStatus`
    - Provides a Functional property that tracks the state of any fault files

### Signals

Any property value change broadcasts a signal on D-Bus. When a value trips past
a threshold, an additional D-Bus signal is sent.

Example, if the value of WarningLow is 5...

| From | To  | propertyChanged Signals                                                                                                  |
| ---- | --- | ------------------------------------------------------------------------------------------------------------------------ |
| 1    | 5   | "xyz.openbmc_project.Sensor.Value" : value = 5                                                                           |
| 1    | 6   | "xyz.openbmc_project.Sensor.Value" : value = 6 ,<br>"xyz.openbmc_project.Sensor.Threshold.Warning" : WarningAlarmLow = 0 |
| 5    | 6   | "xyz.openbmc_project.Sensor.Value" : value = 6                                                                           |
| 6    | 1   | "xyz.openbmc_project.Sensor.Value" : value = 1 ,<br>"xyz.openbmc_project.Sensor.Threshold.Warning" : WarningAlarmLow = 1 |

### System Configuration

On the BMC each sensor's configuration is located in a file. These files can be
found as a child of the `/etc/default/obmc/hwmon` path.

## Creating a Sensor

HWMon sensors are defined in the `recipes-phosphor/sensor/phosphor-hwmon%` path
within the [machine configuration][7]. The children of the `obmc/hwmon`
directory should follow the path of either:

1.  The children of the `devicetree/base` directory path on the system, as
    defined by the kernel. The code obtains this from the OF_FULLNAME udev
    environment variable.

2.  If the device isn't in the device tree, then the device path can be used.

As an example, the Palmetto [configuration][8] file for the ambient temperature
sensor.

```
recipes-phosphor/sensors/phosphor-hwmon/obmc/hwmon/ahb/apb/bus@1e78a000/i2c-bus@c0/tmp423@4c.conf
```

which maps to this specific sensor and conf file on the system...

```
/sys/firmware/devicetree/base/ahb/apb/bus@1e78a000/i2c-bus@c0/tmp423@4c
/etc/default/obmc/hwmon/ahb/apb/bus@1e78a000/i2c@c0/tmp423@4c.conf
```

This next example shows using the device path as opposed to the devicetree path
for the OCC device on an OpenPOWER system. Note how a '--' replaces a ':' in the
directory names for the conf file.

```
recipes-phosphor/sensors/phosphor-hwmon%/obmc/hwmon/devices/platform/gpio-fsi/fsi0/slave@00--00/00--00--00--06/sbefifo1-dev0/occ-hwmon.1.conf
```

which maps to this specific sensor and conf file on the system...

```
/sys/devices/platform/gpio-fsi/fsi0/slave@00:00/00:00:00:06/sbefifo1-dev0/occ-hwmon.1
/etc/default/obmc/hwmon/devices/platform/gpio-fsi/fsi0/slave@00--00/00--00--00--06/sbefifo1-dev0/occ-hwmon.1.conf
```

In order for the sensor to be exposed to D-Bus, the configuration file must
describe the sensor attributes. Attributes follow a format.

```
xxx_yyy#=value

xxx = Attribute
#   = Association number (i.e. 1-n)
yyy = HWMon sensor type (i.e. temp, pwm)
```

| Attribute      | Interfaces Added                       |
| -------------- | -------------------------------------- |
| LABEL          | xyz.openbmc_project.Sensor.Value       |
| WARNHI, WARNLO | xyz.openbmc_project.Threshold.Warning  |
| CRITHI, CRITLO | xyz.openbmc_project.Threshold.Critical |

The HWMon sensor type

| [HWMon sensor type][2] | type                         |
| ---------------------- | ---------------------------- |
| temp                   | temperature                  |
| in                     | voltage                      |
| \*                     | All other names map directly |

See the [HWMon interface][2] definitions for more definitions and keyword
details

In this conf example the tmp423 chip is wired to two temperature sensors. The
values must be described in 10<sup>-3</sup> degrees Celsius.

```
LABEL_temp1=ambient
WARNLO_temp1=10000
WARNHI_temp1=40000

LABEL_temp2=cpu
WARNLO_temp2=10000
WARNHI_temp2=80000
```

#### Additional Config File Entries

The phosphor-hwmon code supports these additional config file entries:

**INTERVAL**

The interval, in microseconds, at which sensors should be read. If not specified
the interval is 1000000us (1 second).

```
INTERVAL=1000000
```

**GAIN**, **OFFSET**

Used to support scaled sensor readings, where value = (raw sensor reading) \*
gain + offset

```
GAIN_in3 = 5.0  #GAIN is a double
OFFSET_in3 = 6  #OFFSET is an integer
```

**MINVALUE**, **MAXVALUE**

If found, will be used to set the MinValue/MaxValue properties on the
`xyz.openbmc_project.Sensor.Value` interface.

```
MINVALUE_temp1 = 1
```

**MODE**

Needed for certain device drivers, specifically the OpenPOWER OCC driver, where
the instance number (the N in tempN_input) is dynamic and instead another file
contains the known ID.

For example

```
temp26_input:29000
temp26_label:171
```

Where the 26 is just what hwmon assigns, but the 171 corresponds to something
like an IPMI sensor value for a DIMM temperature.

The config file would then have:

```
MODE_temp26 = "label"  #Tells the code to look in temp26_label
LABEL_temp171 = "dimm3_temp" #Says that temp26_input holds dimm3_temp
```

**REMOVERCS**

Contains a list of device driver errno values where if these are obtained when
reading the hardware, the corresponding sensor object will be removed from D-Bus
until it is successfully read again.

```
REMOVERCS = "5,6"  #If any sensor on the device returns a 5 or 6, remove it.
REMOVERCS_temp1 = "42"  #If reading temp1_input returns a 42, remove it.
```

**TARGET_MODE**

Allows one to choose the fan target mode, either RPM or PWM, if the device
driver exposes both methods.

```
TARGET_MODE = "RPM"
```

**PWM_TARGET**

For fans that are PWM controlled, can be used to map the pwmN file to a fan M.

```
PWM_TARGET_fan0 = 1 #Use the pwm1 file to control fan 0
```

**ENABLE**

Will write a value to a pwmN_enable file on startup if present.

```
ENABLE_fan1 = 2 #Write a 2 to pwm1_enable
```

### Defining sensors in an IPMI YAML configuration file

For an example of how sensors entries are defined, consult the
[example YAML](https://github.com/openbmc/phosphor-host-ipmid/blob/master/scripts/sensor-example.yaml)

#### How to best choose coefficients

Sensor reading, according to IPMI spec, is calculated as:

```none
y = L[(Mx + B * 10^(bExp)) * 10^(rExp)]
```

- y: the 'final value' as reported by IPMItool
- x: 8 bits, unsigned, reading data encoded in IPMI response packet
- M: 10 bits, signed integer multiplier, `multiplierM` in YAML
- B: 10 bits, signed additive offset, `offsetB` in YAML
- bExp: 4 bits, signed, `bExp` in YAML
- rExp: 4 bits, signed, `rExp` in YAML

In addition, phosphor-ipmi-host configuration also supports `scale` property,
which applies for analog sensors, meaning the value read over DBus should be
scaled by 10^S.

As you can tell, one should choose the coefficients based on possible sensor
reading range and desired resolution. Commonly, B=0, we would have

    Supported range: [0, 255 * M * 10^(scale - rExp)]
    Resolution: M * 10^(scale - rExp)

For a concrete example, let's say a voltage sensor reports between 0 to 5.0V.
hwmon sysfs scales the value by 1000, so the sensor value read over DBus is
between 0 and 5000. A possible configuration for this is:

```none
multiplierM: 20
offsetB: 0
bExp: 0
rExp: -3
scale: -3
```

so for a DBus sensor value of 4986 meaning 4.986V, phosphor-ipmi-host would
encode it as

    x: 4986 / 20 = 249
    M: 20
    rExp: -3

When ipmitool sensor list is called, the tool fetches sensor factors and
computes value as:

```none
y = 20 * 249 * 10^-3 = 4.98 (V)
```

## Additional Reading

Mailing List [Comments on Sensor design][9]

[1]: https://dbus.freedesktop.org/doc/dbus-tutorial.html
[2]: https://www.kernel.org/doc/Documentation/hwmon/sysfs-interface
[3]: https://dbus.freedesktop.org/doc/dbus-specification.html#basic-types
[4]:
  https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/yaml/xyz/openbmc_project/Sensor
[5]: https://github.com/openbmc/phosphor-dbus-interfaces
[6]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Sensor/Threshold/Warning.interface.yaml
[7]: https://github.com/openbmc/openbmc/tree/master/meta-openbmc-machines
[8]:
  https://github.com/openbmc/openbmc/blob/master/meta-ibm/meta-palmetto/recipes-phosphor/sensors/phosphor-hwmon/obmc/hwmon/ahb/apb/bus@1e78a000/i2c-bus@c0/tmp423@4c.conf
[9]: https://lists.ozlabs.org/pipermail/openbmc/2016-November/005309.html
[10]: https://github.com/openbmc/bmcweb/blob/master/DEVELOPING.md#redfish
[11]:
  https://github.com/openbmc/docs/blob/master/architecture/object-mapper.md#associations
[12]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Inventory/Item.interface.yaml
[13]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/Decorator/OperationalStatus.interface.yaml
[14]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Inventory/Decorator/Asset.interface.yaml
[15]: https://github.com/openbmc/phosphor-hwmon
[16]: https://github.com/openbmc/dbus-sensors
