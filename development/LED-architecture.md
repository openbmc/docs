# LED Support for OpenBMC

This document describes how to add LED support for your machine based upon the
OpenBMC [LED Architecture][LED D-Bus README] document. LED group management is
done automatically for machines that support the use of the MRW and is beyond
the scope of this document.

## D-Bus

```
Service     xyx.openbmc_project.LED.GroupManager
Path        /xyz/openbmc_project/led/groups/<label>
Interfaces  xyz.openbmc_project.Led.Group

Signals: none
Attribute: Asserted (boolean)
```

## REST

```
PUT /xyz/openbmc_project/led/groups/<group>/attr/Asserted
```

The LED group state can be changed by setting the Asserted value to boolean 0 or 1.
In the following example, the lamp_test group is being asserted...
```
 curl -b cjar -k -X PUT -H "Content-Type: application/json" -d '{"data":  1}' \
  https://${bmc}/xyz/openbmc_project/led/groups/lamp_test/attr/Asserted
```


## Development Details
There are two significant layers for LED operations.  The physical and the
logical.  The LED Group Manager communicates with the physical LED Manager to
drive the physical LEDs.  The logical groups are defined in the machine's
led.yaml file.  LED Group manager consumes this and creates D-Bus/REST
interfaces for the individual LEDs that are part of the group.

### Defining the physical LED

Physical LED wiring is defined in the `leds` section of the machine's
[device tree][Kernel ARM DTS]. See the [Palmetto DTS][Palmetto DTS LED]
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

_The kernel will then create..._

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

### Defining Groups
An LED Group can contain zero or more LEDs and is defined in the machines
[led.yaml][LED YAML]. The default one will likely need to be tailored to your
machines layout. Customized yaml files are placed into the machines specific
Yocto location. As an example:

```
meta-ibm/meta-palmetto/recipes-phosphor/leds/palmetto-led-manager-config/led.yaml
```

The parent properties in the yaml file will be created below `/xyz/openbmc_project/led/groups/`.
The children properties need to map to an LED name in `/sys/class/leds`.

In the example, below two URIs would be created:
`/xyz/openbmc_project/led/groups/enclosure_fault` and
`/xyz/openbmc_project/led/groups/lamp_test`.  Both act on the same physical
LED `fault` but do so differently.  The lamp_test would also drive a blink
signal to the physical `power` LED if one was created.


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

### Required Groups
OpenBMC Architecture requires specific LED Groups to be created and are
documented in the [D-Bus interface][LED D-Bus README].


## Yocto packaging
1.  Create a tailored LED manager file

    E.g. `meta-ibm/meta-romulus/recipes-phosphor/leds/romulus-led-manager-config-native.bb`
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
2. Change your machine's preferred provider for the led-manager in the conf file

    E.g. `meta-ibm/meta-romulus/conf/machine/romulus.conf`

    ```PREFERRED_PROVIDER_virtual/phosphor-led-manager-config-native = "romulus-led-manager-config-native"```


[LED D-Bus README]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Led/README.md
[LED YAML]: https://github.com/openbmc/phosphor-led-manager/blob/master/led.yaml
[Kernel ARM DTS]: https://github.com/openbmc/linux/tree/dev-4.19/arch/arm/boot/dts
[Palmetto DTS LED]: https://github.com/openbmc/linux/blob/dev-4.19/arch/arm/boot/dts/aspeed-bmc-opp-palmetto.dts#L45
