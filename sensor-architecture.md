# Sensor Support for OpenBMC #
OpenBMC makes it easy to add sensors for your hardware and is compliant with
the traditional Linux HWMon sensor format.  The architecture of OpenBMC
sensors is to map sensors to [D-Bus](https://dbus.freedesktop.org/doc/dbus-tutorial.html)
objects.  The D-Bus object will broadcast the `PropertiesChanged` signal when either
the sensor or threshold value changes. It is the responsibility of other
applications to determine the effect of the signal on the system.

## D-Bus ##

```
Service     xyz.openbmc_project.Hwmon.Hwmon[x]
Path        /xyz/openbmc_project/Sensors/<type>/<label>
Interfaces  xyz.openbmc_project.Sensor.[*]

Signals: All properties for an interface will broadcast signal changed
```

**Path definitions**

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**\<type>** : The [HWMon class]
(https://www.kernel.org/doc/Documentation/hwmon/sysfs-interface) name in lower
case. Examples include `temperature, fan, voltage`.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**\<label>** : User defined
name of the sensor.  Examples include `ambient, cpu0, fan5`


## Development Details ##

Sensor properties are standardized based on the type of sensor.  A Threshold
sensor contains specific properties associated with the rise and fall of a
sensor value.  The [Sensor Interfaces](https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/xyz/openbmc_project/Sensor)
are described in their respective YAML files.  The path location in the source
tree is identical to the interface being described below the
[phosphor-dbus-interfaces](https://github.com/openbmc/phosphor-dbus-interfaces)
parent directory.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;example:
[openbmc/phosphor-dbus-interfaces/xyz/openbmc_project/Sensor/Threshold/Warning.yaml]
(https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Sensor/Threshold/Warning.interface.yaml)

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Maps to D-Bus interface
`xyz.openbmc_project.Sensor.Threshold.Warning`

Each 'name' property in the YAML file maps directly to D-Bus properties.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;example:

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Warning.interface.yaml]
(https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Sensor/Threshold/Warning.interface.yaml)

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
busctl --system introspect xyz.openbmc_project.Hwmon.hwmon1 \
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

### Signals ###

Any property value change broadcasts a signal on D-Bus.  When a value trips
past a threshold, an additional D-Bus signal is sent.

Example, if the value of WarningLow is 5...

| From | To | propertyChanged Signals |
|---|---|---|
| 1 | 5 | "xyz.org.openbmc_project.Sensor.Value" : value = 5 |
| 1 | 6 | "xyz.org.openbmc\_project.Sensor.Value" : value = 6 ,<br>"xyz.org.openbmc\_project.Sensor.Threshold.Warning" : WarningAlarmLow = 0    |
| 5 | 6 | "xyz.org.openbmc\_project.Sensor.Value" : value = 6  |
| 6 | 1 | "xyz.org.openbmc\_project.Sensor.Value" : value = 1 ,<br>"xyz.org.openbmc\_project.Sensor.Threshold.Warning" : WarningAlarmLow = 1 |


### System Configuration ###

On the BMC each sensor's configuration is located in a file.  These files
can be found as a child of the `/etc/default/obmc/hwmon` path.


## Creating a Sensor ##

There are two techniques to add a sensor to your system and which to use
depends on if your system defines sensors via an MRW (Machine Readable
Workbook) or not.


### My sensors are not defined in an MRW ###

HWMon sensors are defined in the `recipes-phosphor/sensor/phosphor-hwmon%`
path within the [machine configuration](https://github.com/openbmc/openbmc/tree/master/meta-openbmc-machines).
The children of the `obmc/hwmon` directory should follow the children of the
`devicetree/base` directory path on the system as defined by the kernel.

As an example, the Palmetto [configuration](https://github.com/openbmc/openbmc/blob/master/meta-openbmc-machines/meta-openpower/meta-ibm/meta-palmetto/recipes-phosphor/sensors/phosphor-hwmon%25/obmc/hwmon/ahb/apb/i2c%401e78a000/i2c-bus%40c0/tmp423%404c.conf)
file for the ambient temperature sensor.

```
recipes-phosphor/sensors/phosphor-hwmon%/obmc/hwmon/ahb/apb/i2c@1e78a000/i2c-bus@c0/tmp423@4c.conf
```

which maps to this specific sensor and conf file on the system...

```
/sys/firmware/devicetree/base/ahb/apb/i2c@1e78a000/i2c-bus@c0/tmp423@4c
/etc/default/obmc/hwmon/ahb/apb/i2c@1e78a000/i2c-bus@c0/tmp423@4c.conf
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
|LABEL | xyz.org.openbmc_project.Sensor.Value |
| WARNHI, WARNLO | xyz.org.openbmc_project.Threshold.Warning |
| CRITHI, CRITLO | xyz.org.openbmc_project.Threshold.Critical |


The HWMon sensor type

| [HWMon sensor type](https://www.kernel.org/doc/Documentation/hwmon/sysfs-interface) | type |
|---|---|
| temp | temperature |
| in | voltage |
| * | All other names map directly |



See the [HWMon interface](https://www.kernel.org/doc/Documentation/hwmon/sysfs-interface)
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

With that level of system information, the sensor infrastructure code can
provide all needed D-Bus properties.

Optionally you can provide an interval value in microseconds for a sensor configuration file:

```
INTERVAL=1000000
```

This configures how often the sensors listed in this configuration should be read.

#### My sensors are defined in an MRW ####

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
| WARN\_LOW, WARN\_HIGH | N | xyz.org.openbmc\_project.Threshold.Warning |
| CRIT\_LOW, CRIT\_HIGH | N | xyz.org.openbmc\_project.Threshold.Critical |


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
Mailing List [Comments on Sensor design](https://lists.ozlabs.org/pipermail/openbmc/2016-November/005309.html)
