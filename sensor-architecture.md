
# Sensor Support for OpenBMC #
OpenBMC makes it easy to add sensors for your hardware and is compliant with 
the traditional Linux HWMon sensor format.  The architecture of OpenBMC 
sensors is to map sensors to dbus objects.  The dbus object will broadcast 
signals when the value of the sensor has changed and when a threshold limit 
has asserted.  It is the responsibility of other applications to determine the
effect of the signal on the system.

## DBus ##

```
Service  xyz.openbmc_project.Hwmon.Hwmon[x]
Path /xyz/openbmc_project/Sensors/<type>/<label>
Interfaces xyz.openbmc_project.Sensor.[*]

Signals: All properties for an interface will broadcast signal changed
```

**Path definitions**

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**\<type>** : The [Hwmon class]
(https://www.kernel.org/doc/Documentation/hwmon/sysfs-interface) name in lower
case. Examples include `temperature, fan, voltage`.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**\<label>** : User defined
name of the sensor.  Examples include `ambient, cpu0, fan5`


## Development Details ##

Sensor properties are standardized based on the type of sensor.  A Threshold
sensor contains specific properties associated with the rise and fall of a
sensor value.  The [Sensor Interfaces](https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/xyz/openbmc_project/Sensor)
are described in their respective yaml files.  The path location in the source
tree is identical to the interface being described below the
[phosphor-dbus-interfaces](https://github.com/openbmc/phosphor-dbus-interfaces)
parent directory.  

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;example:
[openbmc/phosphor-dbus-interfaces/xyz/openbmc_project/Sensor/Threshold/Warning.yaml]
(https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Sensor/Threshold/Warning.interface.yaml)

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Maps to dbus interface 
`xyz.openbmc_project.Sensor.Threshold.Warning`

Each 'name' property in the yaml file map directly to Dbus properties.  

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
 xyz.openbmc_project.Sensor.Threshold.Warning |grep property
 
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

Any property value change broadcasts a signal on dbus.  When a value trips
past a threshold, an additional dbus signal is sent.

Example, if the value of WarningLow is 5...

| From | To | propertyChanged Signals |
|---|---|---|
| 1 | 5 | "xyz.org.openbmc_project.Sensor.Value" : value = 5 |
| 1 | 6 | "xyz.org.openbmc\_project.Sensor.Value" : value = 6 <br>"xyz.org.openbmc\_project.Sensor.Threshold.Warning" : WarningAlarmLow = 0 |
| 5 | 6 | "xyz.org.openbmc\_project.Sensor.Value" : value = 6 |
| 6 | 1 | "xyz.org.openbmc\_project.Sensor.Value" : value = 1 , <br>"xyz.org.openbmc\_project.Sensor.Threshold.Warning" : WarningAlarmLow = 1 |


## Creating a Sensor ##

There are two techniques to add a sensor to your system and which to use
depends on if your system defines sensors via an MRW or not.


### My sensors are not defined in a MRW ###

HWmon sensors are defined in the `recipes-phosphor/sensor/phosphor-hwmon%`
path within the [machine configuration](https://github.com/openbmc/openbmc/tree/master/meta-openbmc-machines).
The children of the `obmc/hwmon` directory should follow the children of the 
`devicetree/base` directory path on the system as defined by the kernel.

As an example, the Palmetto [configuration](https://github.com/openbmc/openbmc/blob/master/meta-openbmc-machines/meta-openpower/meta-ibm/meta-palmetto/recipes-phosphor/sensors/phosphor-hwmon%25/obmc/hwmon/ahb/apb/i2c%401e78a000/i2c-bus%40c0/tmp423%404c.conf)
file for the ambient temperature sensor.

```
recipes-phosphor/sensors/phosphor-hwmon%/obmc/hwmon/ahb/apb/i2c@1e78a000/i2c-bus@c0/tmp423@4c.conf
```

which maps to this specific sensor on the system... 

```
/sys/firmware/devicetree/base/ahb/apb/i2c@1e78a000/i2c-bus@c0/tmp423@4c
```

In order for the sensor to be exposed to dbus, the configuration file must
describe the sensor attributes.  Attributes follow a format.

```
xxx_yyy#=value

xxx = Attribute
#   = Association number (i.e. 1-n)
yyy = HWMon sensor type (i.e. temp, pwm)
```

| Arritbute | Interfaces Added |
|---|---|
|LABEL | xyz.org.openbmc_project.Sensor.Value |
| WARNHI, WARNLO | xyz.org.openbmc_project.Threshold.Warning |
| CRITHI, CRITLO | xyz.org.openbmc_project.Threshold.Critical |


The HWMon sensor type maps to the dbus rest interface directly with
the exception of a couple...

| [HWmon sensor type](https://www.kernel.org/doc/Documentation/hwmon/sysfs-interface) | type |
|---|---|
| temp | temperature |
| in | voltage |
| * | All other names map directly |



See the [HWmon interface](https://www.kernel.org/doc/Documentation/hwmon/sysfs-interface) 
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
provide all needed dbus properties.


#### My sensors are defined in an MRW ####

Setting up sensor support with an MRW is done by adding a HWMON_FEATURE
section to the specified _targetInstance_.  The xml field is required however
a Dbus interface will only be generated when the value property is not null.
Values for Thresholds follow the HWmon format of no decimals.  Temperature
values must be described in 10<sup>-3</sup> degrees Celsius.


| Field id | Value Required | Interfaces Added |
|---|:---:|---|
| HWMON\_NAME | Y | xyz.org.openbmc\_project.Sensor.Value |
| WARN\_LOW, WARN\_HIGH | N | xyz.org.openbmc\_project.Threshold.Warning |
| CRIT\_LOW, CRIT\_HIGH | N | xyz.org.openbmc\_project.Threshold.Critical |



Here is an example of a Fan sensor.  If the RPMs go above 80000 or below 1000
addition signals will be sent over dbus.  Note that neither CRIT\_LOW or
CRIT\_HIGH is set so `xyz.org.openbmc_project.Threshold.Critical` will not be
added.

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