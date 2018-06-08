# Sensor Support for OpenBMC #
OpenBMC makes it easy to add sensors for your hardware and is compliant with
the traditional Linux HWMon sensor format.  The architecture of OpenBMC
sensors is to map sensors to [D-Bus][1]
objects.  The D-Bus object will broadcast the `PropertiesChanged` signal when either
the sensor or threshold value changes. It is the responsibility of other
applications to determine the effect of the signal on the system.

## D-Bus ##

```
Service     xyz.openbmc_project.Hwmon-<hash>.Hwmon1
Path        /xyz/openbmc_project/sensors/<type>/<label>
Interfaces  xyz.openbmc_project.Sensor.[*], others (see below)

Signals: All properties for an interface will broadcast signal changed
```

**Path definitions**

* **<type\>** : The [HWMon class][2] name in lower case.
    - Examples include `temperature, fan_tach, voltage`.

* **<label\>** : User defined name of the sensor.
    - Examples include `ambient, cpu0, fan5`

**Note**: The label shall comply with "Valid Object Paths" of [D-Bus Spec][3],
that shall only contain the ASCII characters "[A-Z][a-z][0-9]_".

**Hash definition**

The hash value in the service name is used to give the service a unique
and stable name.  It is a decimal number that is obtained by hashing
characteristics of the device it is monitoring using std::hash().

## Development Details ##

Sensor properties are standardized based on the type of sensor.  A Threshold
sensor contains specific properties associated with the rise and fall of a
sensor value.  The [Sensor Interfaces][4]
are described in their respective YAML files.  The path location in the source
tree is identical to the interface being described below the
[phosphor-dbus-interfaces][5] parent directory.

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

### REST ###

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

### Other Interfaces ###

Aside from the `xyz.openbmc_project.Sensor` interfaces, the sensor D-Bus objects
may also expose the following interfaces:

1.  `xyz.openbmc_project.Control.FanSpeed`
    - Provides a Target property to set a fan RPM value
2.  `xyz.openbmc_project.Control.FanPwm`
    - Provides a Target property to set a fan PWM value
3.  `xyz.openbmc_project.State.Decorator.OperationalStatus`
    - Provides a Functional property that tracks the state of any fault files

### Signals ###

Any property value change broadcasts a signal on D-Bus.  When a value trips
past a threshold, an additional D-Bus signal is sent.

Example, if the value of WarningLow is 5...

| From | To | propertyChanged Signals |
|---|---|---|
| 1 | 5 | "xyz.openbmc_project.Sensor.Value" : value = 5 |
| 1 | 6 | "xyz.openbmc\_project.Sensor.Value" : value = 6 ,<br>"xyz.openbmc\_project.Sensor.Threshold.Warning" : WarningAlarmLow = 0    |
| 5 | 6 | "xyz.openbmc\_project.Sensor.Value" : value = 6  |
| 6 | 1 | "xyz.openbmc\_project.Sensor.Value" : value = 1 ,<br>"xyz.openbmc\_project.Sensor.Threshold.Warning" : WarningAlarmLow = 1 |


### System Configuration ###

On the BMC each sensor's configuration is located in a file.  These files
can be found as a child of the `/etc/default/obmc/hwmon` path.


## Creating a Sensor ##

There are two techniques to add a sensor to your system and which to use
depends on if your system defines sensors via an MRW (Machine Readable
Workbook) or not.


### My sensors are not defined in an MRW ###

HWMon sensors are defined in the `recipes-phosphor/sensor/phosphor-hwmon%`
path within the [machine configuration][7].
The children of the `obmc/hwmon` directory should follow the path of either:

1.  The children of the `devicetree/base` directory path on the system,
as defined by the kernel.  The code obtains this from the OF_FULLNAME udev
environment variable.

2.  If the device isn't in the device tree, then the device path can be used.

As an example, the Palmetto [configuration][8]
file for the ambient temperature sensor.

```
recipes-phosphor/sensors/phosphor-hwmon%/obmc/hwmon/ahb/apb/i2c@1e78a000/i2c-bus@c0/tmp423@4c.conf
```

which maps to this specific sensor and conf file on the system...

```
/sys/firmware/devicetree/base/ahb/apb/i2c@1e78a000/i2c-bus@c0/tmp423@4c
/etc/default/obmc/hwmon/ahb/apb/i2c@1e78a000/i2c-bus@c0/tmp423@4c.conf
```

This next example shows using the device path as opposed to the devicetree
path for the OCC device on an OpenPOWER system.
Note how a '--' replaces a ':' in the directory names for the conf file.

```
recipes-phosphor/sensors/phosphor-hwmon%/obmc/hwmon/devices/platform/gpio-fsi/fsi0/slave@00--00/00--00--00--06/sbefifo1-dev0/occ-hwmon.1.conf
```

which maps to this specific sensor and conf file on the system...

```
/sys/devices/platform/gpio-fsi/fsi0/slave@00:00/00:00:00:06/sbefifo1-dev0/occ-hwmon.1
/etc/default/obmc/hwmon/devices/platform/gpio-fsi/fsi0/slave@00--00/00--00--00--06/sbefifo1-dev0/occ-hwmon.1.conf
```

In order for the sensor to be exposed to D-Bus, the configuration file must
describe the sensor attributes.  Attributes follow a format.

```
xxx_yyy#=value

xxx = Attribute
#   = Association number (i.e. 1-n)
yyy = HWMon sensor type (i.e. temp, pwm)
```

| Attribute | Interfaces Added |
|---|---|
|LABEL | xyz.openbmc\_project.Sensor.Value |
| WARNHI, WARNLO | xyz.openbmc\_project.Threshold.Warning |
| CRITHI, CRITLO | xyz.openbmc\_project.Threshold.Critical |


The HWMon sensor type

| [HWMon sensor type][2] | type |
|---|---|
| temp | temperature |
| in | voltage |
| * | All other names map directly |



See the [HWMon interface][2]
definitions for more definitions and keyword details



In this conf example the tmp423 chip is wired to two temperature sensors.
The values must be described in 10<sup>-3</sup> degrees Celsius.


```
LABEL_temp1=ambient
WARNLO_temp1=10000
WARNHI_temp1=40000

LABEL_temp2=cpu
WARNLO_temp2=10000
WARNHI_temp2=80000
```

#### Additional Config File Entries ####
The phosphor-hwmon code supports these additional config file entries:

**INTERVAL**

The interval, in microseconds, at which sensors should be read.  If
not specified the interval is 1000000us (1 second).
```
INTERVAL=1000000
```

**GAIN**, **OFFSET**

Used to support scaled sensor readings, where
value = (raw sensor reading) * gain + offset
```
GAIN_in3 = 5.0  #GAIN is a double
OFFSET_in3 = 6  #OFFSET is an integer
```

**MINVALUE**, **MAXVALUE**

If found, will be used to set the MinValue/MaxValue properties
on the `xyz.openbmc_project.Sensor.Value` interface.
```
MINVALUE_temp1 = 1
```

**MODE**

Needed for certain device drivers, specifically the OpenPOWER OCC driver,
where the instance number (the N in tempN_input) is dynamic and instead
another file contains the known ID.

For example
```
temp26_input:29000
temp26_label:171
```
Where the 26 is just what hwmon assigns, but the 171 corresponds
to something like an IPMI sensor value for a DIMM temperature.

The config file would then have:
```
MODE_temp26 = "label"  #Tells the code to look in temp26_label
LABEL_temp171 = "dimm3_temp" #Says that temp26_input holds dimm3_temp
```

**REMOVERCS**

Contains a list of device driver errno values where if these are obtained
when reading the hardware, the corresponding sensor object will be removed
from D-Bus until it is successfully read again.

```
REMOVERCS = "5,6"  #If any sensor on the device returns a 5 or 6, remove it.
REMOVERCS_temp1 = "42"  #If reading temp1_input returns a 42, remove it.
```

**TARGET\_MODE**

Allows one to choose the fan target mode, either RPM or PWM,
if the device driver exposes both methods.
```
TARGET_MODE = "RPM"
```

**PWM\_TARGET**

For fans that are PWM controlled, can be used to map the pwmN file to a fan M.
```
PWM_TARGET_fan0 = 1 #Use the pwm1 file to control fan 0
```

**ENABLE**

Will write a value to a pwmN\_enable file on startup if present.
```
ENABLE_fan1 = 2 #Write a 2 to pwm1_enable
```

### My sensors are defined in an MRW ###

Setting up sensor support with an MRW is done by adding a unit-hwmon-feature
unit, for each hwmon feature needing to be monitored and then filling in the
HWMON_FEATURE attribute.  The XML field is required however a D-Bus interface
will only be generated when the value property is not null. Values for
Thresholds follow the HWMon format of no decimals.  Temperature values must
be described in 10<sup>-3</sup> degrees Celsius. The HWMON\_NAME will be used
to derive the \<type> while the DESCRIPTIVE\_NAME creates the \<label> for the
instance path.

| Field id | Value Required | Interfaces Added |
|---|:---:|---|
| HWMON\_NAME | Y | xyz.org.openbmc\_project.Sensor.Value |
| WARN\_LOW, WARN\_HIGH | N | xyz.openbmc\_project.Threshold.Warning |
| CRIT\_LOW, CRIT\_HIGH | N | xyz.openbmc\_project.Threshold.Critical |


Here is an example of a Fan sensor.  If the RPMs go above 80000 or below 1000
addition signals will be sent over D-Bus.  Note that neither CRIT\_LOW or
CRIT\_HIGH is set so `xyz.org.openbmc_project.Threshold.Critical` will not be
added.  The instance path will be `/xyz/openbmc_project/Sensors/fan/fan0`.

```
<targetInstance>
	<id>MAX31785.hwmon2</id>
	<type>unit-hwmon-feature</type>
	...
	<attribute>
		<id>HWMON_FEATURE</id>
		<default>
				<field><id>HWMON_NAME</id><value>fan1</value></field>
				<field><id>DESCRIPTIVE_NAME</id><value>fan0</value></field>
				<field><id>WARN_LOW</id><value>1000</value></field>
				<field><id>WARN_HIGH</id><value>80000</value></field>
				<field><id>CRIT_LOW</id><value></value></field>
				<field><id>CRIT_HIGH</id><value></value></field>
		</default>
	</attribute>
```


## Additional Reading ##
Mailing List [Comments on Sensor design][9]


[1]: https://dbus.freedesktop.org/doc/dbus-tutorial.html
[2]: https://www.kernel.org/doc/Documentation/hwmon/sysfs-interface
[3]: https://dbus.freedesktop.org/doc/dbus-specification.html#basic-types
[4]: https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/xyz/openbmc_project/Sensor
[5]: https://github.com/openbmc/phosphor-dbus-interfaces
[6]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Sensor/Threshold/Warning.interface.yaml
[7]: https://github.com/openbmc/openbmc/tree/master/meta-openbmc-machines
[8]: https://github.com/openbmc/openbmc/blob/master/meta-openbmc-machines/meta-openpower/meta-ibm/meta-palmetto/recipes-phosphor/sensors/phosphor-hwmon%25/obmc/hwmon/ahb/apb/i2c%401e78a000/i2c-bus%40c0/tmp423%404c.conf
[9]: https://lists.ozlabs.org/pipermail/openbmc/2016-November/005309.html
