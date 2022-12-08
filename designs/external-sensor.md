# ExternalSensor in dbus-sensors

Author: Josh Lehan[^1]

Other contributors: Ed Tanous, Peter Lundgren, Alex Qiu

Created: March 19, 2021

## Introduction

In OpenBMC, the _dbus-sensors_[^2] package contains a suite of sensor daemons.
Each daemon monitors a particular type of sensor. This document provides
rationale and motivation for adding _ExternalSensor_, another sensor daemon, and
gives some example usages of it.

## Motivation

There are 10 existing sensor daemons in _dbus-sensors_. Why add another sensor
daemon?

- Most of the existing sensor daemons are tied to one particular physical
  quantity they are measuring, such as temperature, and are hardcoded as such.
  An externally-updated sensor has no such limitation, and should be flexible
  enough to measure any physical quantity currently supported by OpenBMC.

- Essentially all of the existing sensor daemons obtain the sensor values they
  publish to D-Bus by reading from local hardware (typically by reading from
  virtual files provided by the _hwmon_[^3] subsystem of the Linux kernel). None
  of the daemons are currently designed with the intention of accepting values
  pushed in from an external source. Although there is some debugging
  functionality to add this feature to other sensor daemons[^25], it is not the
  primary purpose for which they were designed.

- Even if the debugging functionality of an existing daemon were to be used, the
  daemon would still need a valid configuration tied to recognized hardware, as
  detected by _entity-manager_[^4], in order for the daemon to properly
  initialize itself and participate in the OpenBMC software stack.

- For the same reason it is desirable for existing sensor daemons to detect and
  properly indicate failures of their underlying hardware, it is desirable for
  _ExternalSensor_ to detect and properly indicate loss of timely sensor updates
  from their external source. This is a new feature, and does not cleanly fit
  into the architecture of any existing sensor daemon, thus a new daemon is the
  correct choice for this behavior.

For these reasons, _ExternalSensor_ has been added[^5], as the eleventh sensor
daemon in _dbus-sensors_.

## Design

After some discussion, a proof-of-concept _HostSensor_[^6] was published. This
was a stub, but it revealed the minimal implementation that would still be
capable of fully initializing and participating in the OpenBMC software stack.
_ExternalSensor_ was formed by using this example _HostSensor_, and also one of
the simplest existing sensor daemons, _HwmonTempSensor_[^7], as references to
build upon.

As written, after validating parameters during initialization, there is
essentially no work for _ExternalSensor_ to do. The main loop is mostly idle,
remaining blocked in the Boost ASIO[^8] library, handling D-Bus requests as they
come in. This utilizes the functionality in the underlying _Sensor_[^9] class,
which already contains the D-Bus hooks necessary to receive values from the
external source.

An example external source is the IPMI service[^10], receiving values from the
host via the IPMI "Set Sensor Reading" command[^11]. _ExternalSensor_ is
intended to be source-agnostic, so it does not matter if this is IPMI or
Redfish[^12] or something else in the future, as long as they are received
similarly over D-Bus.

### Timeout

The timeout feature is the primary feature which distinguishes _ExternalSensor_
from other sensor daemons. Once an external source starts providing updates, the
external source is expected to continue to provide timely updates. Each update
will be properly published onto D-Bus, in the usual way done by all sensor
daemons, as a floating-point value.

A timer is used, the same Boost ASIO[^13] timer mechanism used by other sensor
daemons to poll their hardware, but in this case, is used to manage how long it
has been since the last known good external update. When the timer expires, the
sensor value will be deemed stale, and will be replaced with floating-point
quiet _NaN_[^14].

### NaN

The advantage of floating-point _NaN_ is that it is a drop-in replacement for
the valid floating-point value of the sensor. A subtle difference of the earlier
OpenBMC sensor "Value" schema change, from integer to floating-point, is that
the field is essentially now nullable. Instead of having to arbitrarily choose
an arbitrary integer value to indicate "not valid", such as -1 or 9999 or
whatever, floating-point explicitly has _NaN_ to indicate this. So, there is no
possibility of confusion that this will be mistaken for a valid sensor value, as
_NaN_ is literally _not a number_, and thus can not be misparsed as a valid
sensor reading. It thus saves having to add a second field to reliably indicate
validity, which would break the existing schema[^15].

An alternative to using _NaN_ for staleness indication would have been to use a
timestamp, which would introduce the complication of having to parse and compare
timestamps within OpenBMC, and all the subtle difficulties thereof[^16]. What's
more, adding a second field might require a second D-Bus message to update, and
D-Bus messages are computationally expensive[^17] and should be used sparingly.
Periodic things like sensors, which send out regular updates, could easily lead
to frequent D-Bus traffic and thus should be kept as minimal as practical. And
finally, changing the Value schema would cause a large blast radius, both in
design and in code, necessitating a large refactoring effort well beyond the
scope of what is needed for _ExternalSensor_.

### Configuration

Configuring a sensor for use with _ExternalSensor_ should be done in the usual
way[^18] that is done for use with other sensor daemons, namely, a JSON
dictionary that is an element of the "Exposes" array within a JSON configuration
file to be read by _entity-manager_. In that JSON dictionary, the valid names
are listed below. All of these are mandatory parameters, unless mentioned as
optional. For fields listed as "Numeric" below, this means that it can be either
integer or valid floating-point.

- "Name": String. The sensor name, which this sensor will be known as. A
  mandatory component of the `entity-manager` configuration, and the resulting
  D-Bus object path.

- "Units": String. This parameter is unique to _ExternalSensor_. As
  _ExternalSensor_ is not tied to any particular physical hardware, it can
  measure any physical quantity supported by OpenBMC. This string will be
  translated to another string via a lookup table[^19], and forms another
  mandatory component of the D-Bus object path.

- "MinValue": Numeric. The minimum valid value for this sensor. Although not
  used by _ExternalSensor_ directly, it is a valuable hint for services such as
  IPMI, which need to know the minimum and maximum valid sensor values in order
  to scale their reporting range accurately. As _ExternalSensor_ is not tied to
  one particular physical quantity, there is no suitable default value for
  minimum and maximum. Thus, unlike other sensor daemons where this parameter is
  optional, in _ExternalSensor_ it is mandatory.

- "MaxValue": Numeric. The maximum valid value for this sensor. It is treated
  similarly to "MinValue".

- "Timeout": Numeric. This parameter is unique to _ExternalSensor_. It is the
  timeout value, in seconds. If this amount of time elapses with no new updates
  received over D-Bus from the external source, this sensor will be deemed
  stale. The value of this sensor will be replaced with floating-point _NaN_, as
  described above. This field is optional. If not given, the timeout feature
  will be disabled for this sensor (so it will never be deemed stale).

- "Type": String. Must be exactly "ExternalSensor". This string is used by
  _ExternalSensor_ to obtain configuration information from _entity-manager_
  during initialization. This string is what differentiates JSON stanzas
  intended for _ExternalSensor_ versus JSON stanzas intended for other
  _dbus-sensors_ sensor daemons.

- "Thresholds": JSON dictionary. This field is optional. It is passed through to
  the main _Sensor_ class during initialization, similar to other sensor
  daemons. Other than that, it is not used by _ExternalSensor_.

- "PowerState": String. This field is optional. Similarly to "Thresholds", it is
  passed through to the main _Sensor_ class during initialization.

Here is an example. The sensor created by this stanza will form this object
path: /xyz/openbmc_project/sensors/temperature/HostDevTemp

```
        {
            "Name": "HostDevTemp",
            "Units": "DegreesC",
            "MinValue": -16.0,
            "MaxValue": 111.5,
            "Timeout": 4.0,
            "Type": "ExternalSensor"
        },
```

There can be multiple _ExternalSensor_ sensors in the configuration. There is no
set limit on the number of sensors, except what is supported by a service such
as IPMI.

## Implementation

As it stands now, _ExternalSensor_ is up and running[^20]. However, the timeout
feature was originally implemented at the IPMI layer. Upon further
investigation, it was found that IPMI was the wrong place for this feature, and
that it should be moved within _ExternalSensor_ itself[^21]. It was originally
thought that the timeout feature would be a useful enhancement available to all
IPMI sensors, however, expected usage of almost all external sensor updates is a
one-shot adjustment (for example, somebody wishes to change a voltage regulator
setting, or fan speed setting). In this case, the timeout feature would not only
not be necessary, it would get in the way and require additional coding[^22] to
compensate for the unexpected _NaN_ value. Only sensors intended for use with
_ExternalSensor_ are expected to receive continuous periodic updates from an
external source, so it makes sense to move this timeout feature into
_ExternalSensor_. This change also has the advantage of making _ExternalSensor_
not dependent on IPMI as the only source of external updates.

A challenge of generalizing the timeout feature into _ExternalSensor_, however,
was that the existing _Sensor_ base class did not currently allow its existing
D-Bus setter hook to be customized. This feature was straightforward to
add[^23]. One limitation was that the existing _Sensor_ class, by design,
dropped updates that duplicated the existing sensor value. For use with
_ExternalSensor_, we want to recognize all updates received, even duplicates, as
they are important to pet the watchdog, to avoid inadvertently triggering the
timeout feature. However, it is still important to avoid needlessly sending the
D-Bus _PropertiesChanged_ event for duplicate readings.

The timeout value was originally a compiled-in constant. If _ExternalSensor_ is
to succeed as a general-purpose tool, this must be configurable. It was
straightforward to add another configurable parameter[^24] to accept this
timeout value, as shown in "Parameters" above.

The hardest task of all, however, was getting it accepted upstream. If you are
reading this, then most likely, it was successful!

## Footnotes

[^1]: https://gerrit.openbmc.org/q/owner:krellan%2540google.com
[^2]: https://github.com/openbmc/dbus-sensors/blob/master/README.md
[^3]: https://www.kernel.org/doc/html/latest/hwmon/index.html
[^4]: https://github.com/openbmc/entity-manager/blob/master/README.md
[^5]: https://gerrit.openbmc.org/c/openbmc/dbus-sensors/+/36206
[^6]: https://gerrit.openbmc.org/c/openbmc/dbus-sensors/+/35476
[^7]: https://github.com/openbmc/dbus-sensors/blob/master/src/HwmonTempMain.cpp
[^8]: https://think-async.com/Asio/
[^9]: https://github.com/openbmc/dbus-sensors/blob/master/include/sensor.hpp
[^10]:
    https://github.com/openbmc/docs/blob/master/architecture/ipmi-architecture.md

[^11]:
    https://www.intel.com/content/www/us/en/servers/ipmi/ipmi-intelligent-platform-mgt-interface-spec-2nd-gen-v2-0-spec-update.html

[^12]: https://www.dmtf.org/standards/redfish
[^13]:
    https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio/overview/timers.html

[^14]: https://anniecherkaev.com/the-secret-life-of-nan
[^15]:
    https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Sensor/Value.interface.yaml

[^16]: https://cr.yp.to/proto/utctai.html
[^17]: https://github.com/openbmc/openbmc/issues/1892
[^18]:
    https://github.com/openbmc/entity-manager/blob/master/docs/my_first_sensors.md

[^19]: https://github.com/openbmc/dbus-sensors/blob/master/src/SensorPaths.cpp
[^20]: https://gerrit.openbmc.org/c/openbmc/dbus-sensors/+/36206
[^21]: https://gerrit.openbmc.org/c/openbmc/dbus-sensors/+/41398
[^22]: https://gerrit.openbmc.org/c/openbmc/dbus-sensors/+/39294
[^23]: https://gerrit.openbmc.org/c/openbmc/dbus-sensors/+/41394
[^24]: https://gerrit.openbmc.org/c/openbmc/entity-manager/+/41397
[^25]: https://gerrit.openbmc.org/c/openbmc/dbus-sensors/+/16177
