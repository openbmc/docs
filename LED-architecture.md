# LED Support for OpenBMC #

The architecture around LED management is thoroughly described in the [dbus interface](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Led/README.md)
document.  This end to end design documents how to add LED support for your
platform is based upon that design.

## D-Bus ##

```
Service     xyx.openbmc_project.LED.GroupManager
Path        /xyz/openbmc_project/led/groups/<label>
Interfaces  xyz.openbmc_project.Led.Group

Signals: none
Attribute: Asserted (binary)
```

## REST ##

```
PUT /xyz/openbmc_project/led/groups/\<label\>/attr/Asserted
```

The LED group state can be changed by setting the Asserted value to 0 or 1.
In the following example the lamp_test group is being asserted...
```
 curl -b cjar -k -X PUT -H "Content-Type: application/json" -d '{"data":  1}' \
  https://bmc/xyz/openbmc_project/led/groups/lamp_test/attr/Asserted
```


## Development Details ##
The LED Manager connects the physical LEDs defined in `/sys/class/led/` to the
labels aka groups in the machines led.yaml file.  The LED Manager then exposes
a DBus/REST interface for those groups to be acted upon.

### Defining the physical LED ###

Physical LED wiring is defined in the `leds` section of the systems [device tree](https://github.com/openbmc/linux/tree/dev-4.10/arch/arm/boot/dts).
See the [Palmetto dts](https://github.com/openbmc/linux/blob/dev-4.10/arch/arm/boot/dts/aspeed-bmc-opp-palmetto.dts#L39)
as an example.

_Add a fault LED to the device tree with a corresponding gpio pin..._
```
  leds {
    compatible = "gpio-leds";

    fault {
      gpios = <&gpio ASPEED_GPIO(N, 2) GPIO_ACTIVE_LOW>;
    };
  }
```

_The [phosphor-led-sysfs](https://github.com/openbmc/phosphor-led-sysfs) would
then create..._

```
 ls -l /sys/class/leds/fault/
total 0
-rw-r--r--    1 root     root          4096 Jun 21 20:04 brightness
lrwxrwxrwx    1 root     root             0 Jun 21 20:29 device -> ../../../leds
-r--r--r--    1 root     root          4096 Jun 21 20:29 max_brightness
drwxr-xr-x    2 root     root             0 Jun 21 20:29 power
lrwxrwxrwx    1 root     root             0 Jun 21 20:04 subsystem -> ../../../../../class/leds
-rw-r--r--    1 root     root          4096 Jun 21 20:04 trigger
-rw-r--r--    1 root     root          4096 Jun 21 20:04 uevent
```

### Defining Groups ###
Groups of LEDs (which can simply contain a single LED) are defined in the systems
[led.yaml](https://github.com/openbmc/phosphor-led-manager/blob/master/led.yaml).
The default one will likely need to be tailored to your machines layout.  If you
are using an MRW for your LEDs this is done automatically and is beyond the scope
of this document.  Customized yaml files are placed into the machines specific
Yocto location.  As an example....

```
meta-openbmc-machines/meta-openpower/meta-ibm/meta-palmetto/recipes-phosphor/leds/palmetto-led-manager-config/led.yaml
```

The parent properties in the yaml file will be created below `/xyz/openbmc_project/led/groups/`
The children properties need to map to an LED name in `/sys/class/leds`.

In the example below two URIs would be created `/xyz/openbmc_project/led/groups/EnclosureFault`
and `/xyz/openbmc_project/led/groups/lamp_test`.  Both act on the same LED `fault`
but do so differently.  The lamp_test would also drive a blink signal to the
physical `power` led if one was created.


```
EnclosureFault:
    fault:
        Action: 'On'
        DutyOn: 50
        Period: 0
lamp_test:
    fault:
        Action: 'Blink'
        DutyOn: 20
        Period: 100
    power:
        Action: 'Blink'
        DutyOn: 20
        Period: 100

```

### Require Groups ###
OpenBMC Architecture requires specific LED Groups to be created and are documented
in the [dbus interface](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Led/README.md)


## Yocto packaging ##
1.  Create a tailored LED manager file

`meta-openbmc-machines/meta-openpower/meta-ibm/meta-romulus/recipes-phosphor/leds/romulus-led-manager-config-native.bb`
```
SUMMARY = "Phosphor LED Group Management for Romulus"
PR = "r1"

inherit native
inherit obmc-phosphor-utils
inherit obmc-phosphor-license

PROVIDES += "virtual/phosphor-led-manager-config-native"

SRC_URI += "file://led.yaml"
S = "${WORKDIR}"

# Overwrites the default led.yaml
do_install() {
    SRC=${S}
    DEST=${D}${datadir}/phosphor-led-manager
    install -D ${SRC}/led.yaml ${DEST}/led.yaml
}
```
2. Change your systems peferred provider for the led-manager in the conf file

_i.e. meta-openbmc-machines/meta-openpower/meta-ibm/meta-romulus/conf/machine/romulus.conf_

`PREFERRED_PROVIDER_virtual/phosphor-led-manager-config-native = "romulus-led-manager-config-native"`



## Additional Reading ##
DBus Interfaces [Managing LED groups](https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/xyz/openbmc_project/Led/README.md)
