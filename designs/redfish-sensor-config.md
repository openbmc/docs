# RedfishSensor Configuration

Author: Josh Lehan[^1] (krellan@[^2])

Created: November 21, 2022

## Introduction

This is a companion document to the `RedfishSensor` design document[^3]. More
elaboration on the design is given here, but the main focus of this document is
practical information as to how to configure `RedfishSensor` to read desired
sensors from a desired Redfish server.

## Identifying Sensors

In the Redfish community, it is often frowned upon to encode a dependency upon
an exact URL path when writing an external tool that accesses a Redfish service.
Therefore, sensors on a Redfish server will be referenced in a roundabout way,
by specifying desired characteristics of the sensor to be used, instead of
simply giving the exact URL of the sensor. This works similarly to a "magnet
link"[^4] that might be familiar from using peer-to-peer file sharing services.

What's more, in Redfish, an individual sensor is not a uniquely addressable
object. Instead, the addressable Redfish object is a `Chassis`[^5], and a
`Chassis` object can contain multiple `Sensor`[^6] objects. Therefore, the
chassis must also be specified, in order to reach a desired sensor. The chassis
must also be referenced in a roundabout way, by specifying desired
characteristics of it.

The supported characteristics of `Sensor` objects and `Chassis` objects are
listed below, under Configuration. To provide dependable references that will
not change, the only supported fields are those that are guaranteed by the
Redfish schema to be read-only strings.

All of these characteristics are optional, although at least one of them must be
given, otherwise it would be impossible to know what sensor or chassis were
desired. If a particular characteristic is not given, then the string is ignored
during comparison (treated as a "don't care" field).

During comparison, all given strings will be matched against those available
from the Redfish server. The first complete match of all given strings, in the
order returned by the Redfish server, will be accepted as the desired `Sensor`
object to be used. This ordering is arbitrary, and is dependent upon the
implementation of each Redfish server. So, in order to avoid matching the wrong
`Sensor` object, it is recommended that enough characteristics be given to
uniquely identify the desired sensor.

## Configuration

The configuration of `RedfishSensor` is done similarly to the other daemons in
the `dbus-sensors`[^7] suite. The configuration data comes from
`entity-manager`[^8] and ultimately from JSON files. Each of these JSON files
contains one JSON object. In that object, a field named `Exposes`[^9] has a
value consisting of an array of JSON objects. Each of those objects represents a
configuration. In a given configuration, the `Type` field determines which of
the daemons, in the `dbus-sensors` suite, will respond to it.

As `RedfishSensor` is one of these daemons, it responds to the `Type` field, and
claims three values: `RedfishSensor`, `RedfishChassis`, and `RedfishServer`. The
reason for three different types is because of hierarchy. As the JSON
configuration array is flat[^10], each object will indicate who its parents are,
in lieu of being able to organize the object directly under its parents. This
also avoids having to give many duplicate copies of the same configuration
values.

*   `RedfishSensor`: This `Type` represents a Redfish `Sensor` object. This is
    the primary object maintained by the `RedfishSensor` daemon, hence its name.
    If the configuration is correct, each Redfish sensor object will correspond
    1:1 with an appropriate D-Bus sensor object. The local D-Bus sensor object
    will be created and maintained by the `RedfishSensor` daemon, in an effort
    to match the corresponding remote Redfish sensor object.

*   `RedfishChassis`: This `Type` represents a Redfish `Chassis` object. In
    Redfish, a chassis can contain one or more `Sensor` objects.

*   `RedfishServer`: This `Type` represents a Redfish server. As a Redfish
    server is a specialized Web server, identifying a desired Redfish server is
    done the same way as one would do this for a Web server, namely, by
    specifying the desired URL. In Redfish, the server can provide one or more
    `Chassis` objects.

### RedfishSensor

These are the fields supported by a `RedfishSensor` object:

*   `Name`: String. Mandatory. The name of this object, which will be how the
    rest of the system refers to it. As with all objects exposed by Entity
    Manager, the value of `Name` must be unique systemwide. It becomes the last
    component of the D-Bus object path.

*   `Type`: String. Mandatory. Must be exactly `RedfishSensor`, case sensitive.
    It is used by Entity Manager to route this configuration to the appropriate
    daemon to handle it, namely `RedfishSensor`.

*   `RedfishName`: String. Optional. This is used to help match up the correct
    `Sensor` object on the Redfish server. All the `Sensor` objects, on the
    selected `Chassis` object of the Redfish server, will be compared. If the
    `Name` field of a `Sensor` object matches the value of this string, that
    sensor will be selected for use.

*   `RedfishId`: String. Optional. This is used to help match up the correct
    `Sensor` object on the Redfish server. It is similar to `RedfishName`, but
    works on the `Id` field instead. If both `RedfishName` and `RedfishId` are
    present, both must match, in order for the `Sensor` object to be selected
    for use.

The `RedfishName` and `RedfishId` fields represent characteristics of the
desired sensor, and they work as filters. At least one of these fields must be
given, as they are used to filter the results from the Redfish server when
searching for the desired `Sensor` object.

*   `Chassis`: String. Mandatory. The name of the corresponding `RedfishChassis`
    object to be used with this sensor.

*   `Server`: String. Mandatory. The name of the corresponding `RedfishServer`
    object to be used with this sensor.

*   `PowerState`: String. Optional. Not used by `RedfishSensor` itself, but some
    underlying common functionality of the `dbus-sensors` suite can make use of
    it.

When defining a `RedfishSensor` configuration, the corresponding
`RedfishChassis` and `RedfishServer` configurations must also be within the same
`Exposes` array. In other words, the corresponding chassis and server must be
instantiated at the same time the sensor is instantiated. This avoids problems
when trying to do the necessary cross-referencing during initialization.

#### Differences from ExternalSensor

This subsection is informational only. If you have used `ExternalSensor`, here
are the differences between that and `RedfishSensor`:

*   `MinValue`: This parameter is not supported. The information, about the
    bounds, the minimum possible value that this sensor will report as a sensor
    reading, will be pulled automatically from the corresponding JSON field
    `ReadingRangeMin`[^11] provided by the Redfish server.

*   `MaxValue`: Same as `MinValue`, except for the other extreme of the bounds,
    the maximum. The corresponding Redfish field is `ReadingRangeMax`.

*   `Timeout`: This parameter is not supported. See "Limitations" below.

*   `Units`: This parameter is not supported. The information, about the correct
    unit of measurement to use, will be pulled automatically from the
    corresponding JSON field `ReadingUnits` provided by the Redfish server.

*   `Thresholds`: This parameter is not supported. The information, about
    various thresholds, will be pulled automatically from the corresponding JSON
    object `Thresholds` provided by the Redfish server. Also, see "Limitations"
    below.

### RedfishChassis

*   `Name`: String. Mandatory. Identifies this chassis. As with all objects
    exposed by Entity Manager, the name must be unique systemwide. To correctly
    link a `RedfishSensor` sensor up with this chassis, the value of the
    `Chassis` field, within that sensor, must be equal to the value of this
    `Name` field here.

*   `Type`: String. Mandatory. Must be exactly `RedfishChassis`, case sensitive.

*   `RedfishName`: String. Optional. Same as with `RedfishSensor`, this matches
    up against the corresponding `Name` field on the Redfish server. If present,
    it is used to help identify the correct `Chassis` object to match up with.

*   `RedfishId`: String. Optional. Same as with `RedfishSensor`, this matches up
    against the corresponding `Id` field on the Redfish server.

The following fields all work the same. String. Optional. If present, they are
used to match up against the corresponding field on the Redfish server, to help
narrow down the search, when searching for the desired `Chassis` object to match
up with.

*   `Manufacturer`
*   `Model`
*   `PartNumber`
*   `SKU`
*   `SerialNumber`
*   `SparePartNumber`
*   `Version`

The names of these fields are exactly the same as on the Redfish server. For
example, `Manufacturer` is used for comparison against the `Manufacturer`
read-only string field within the `Chassis` objects returned by the Redfish
server.

All of the optional fields above represent characteristics of the desired
chassis, and they work as filters. At least one of these fields must be given,
as they are used to filter the results from the Redfish server when searching
for the desired `Chassis` object.

### RedfishServer

A Redfish server is simply a Web server that complies with the Redfish protocol.
Therefore, a Redfish server can be identified in the same way, as a familiar
URL, http://example.com/ for example.

*   `Name`: String. Mandatory. Identifies this Redfish server. As with all
    objects exposed by Entity Manager, the name must be unique systemwide. To
    correctly link a `RedfishSensor` sensor up with this Redfish server, the
    value of the `Server` field, within that sensor, must be equal to the value
    of this `Name` field here.

*   `Type`: String. Mandatory. Must be exactly `RedfishServer`, case sensitive.

The following configuration fields are used to build up a URL.

*   `Host`: String. Mandatory. The hostname (or IP address) of the Redfish
    server. IPv4 addresses can be given in familiar dotted-quad notation. IPv6
    addresses, however, must be surrounded by square brackets, as is convention,
    to disambiguate them from hostnames.

*   `Protocol`: String. Optional. Must be either `http` or `https`, case
    sensitive. If left out, `http` is assumed. See "Limitations" below.

*   `User`: String. Optional. This will be inserted into the resulting URL. If
    left out, no user name is used.

*   `Password`: String. Optional. If left out, the user is assumed to have no
    password. If `User` is not specified, this field will have no effect.

*   `Port`: Numeric. Optional. If left out, 80 for HTTP, 443 for HTTPS, as is
    convention.

If more than one sensor is configured to use the same Redfish server, they will
be internally consolidated together, for efficient sensor reading.

## Limitations

There is currently no way to configure the polling rate, either globally, or on
a per-sensor basis. Each sensor will have its reading taken, as fast as
possible, or at a one-second interval, whichever is slower.

There is currently no way to configure a timeout, either globally, or on a
per-sensor basis. The timeout value is outside of the control of
`RedfishSensor`, and is up to the underlying network stack, and the target
Redfish server. If there is a network error, the existing sensor data will be
deemed stale, just as if a timeout had occurred.

There is currently no way to configure the number of simultaneous TCP
connections that will be made. Each server will have only one connection made to
it at a time. Similarly, there is no way to configure the number of threads that
will be used. For simplicity, the first implementation of `RedfishSensor` will
be single-threaded.

Thresholds are not supported yet. The definition of these in Redfish is slightly
different, but still mostly compatible, with these in the D-Bus interface. A
mapping will have to be designed, the subject of future work.

Although there is a way to request that HTTPS be used, instead of HTTP, HTTPS is
not supported yet. Among other things, there needs to be a way to specify which
certificates to use and to accept.

### Future Directions

There needs to be a faster way of reading sensors, than having a single Redfish
query per sensor. If multiple sensors exist on the same chassis, it should be
possible to use the Redfish `expand`[^12] query parameter to fetch multiple
sensors at the same time. This is dependent on the individual Redfish server
software in use, though, and appears poorly supported. This feature is very new
to `bmcweb`[^13], so not many Redfish servers in the field support this new
feature.

Another faster way of reading sensors might be to use the Redfish
`Telemetry`[^14] or `Metrics` schemas. Not all Redfish servers support these,
though, and there is no guarantee that individual sensors will be 1:1 mappable
to these results, even when they are present. It also appears that many of the
Redfish field names have changed, and some mandatory fields are now optional, so
some software adaptation will be necessary.

## Example

This example is a snippet of JSON, from within the `Exposes` array, of the JSON
object represented by a JSON file to be used with Entity Manager.

```json
{
  "Name": "IntakeTemp",
  "Type": "RedfishSensor",
  "RedfishName": "Intake Temperature",
  "Chassis": "ExampleCase",
  "Server": "ExampleCom"
},
{
  "Name": "ExhaustTemp",
  "Type": "RedfishSensor",
  "RedfishName": "Exhaust Temperature",
  "Chassis": "ExampleCase",
  "Server": "ExampleCom"
},
{
  "Name": "PowerIn",
  "Type": "RedfishSensor",
  "RedfishName": "Input Power",
  "Chassis": "ExamplePowerSupply",
  "Server": "ExampleCom"
},
{
  "Name": "ExampleCase",
  "Type": "RedfishChassis",
  "Manufacturer": "Acme",
  "Model": "ISM-9000"
},
{
  "Name": "ExamplePowerSupply",
  "Type": "RedfishChassis",
  "Manufacturer": "Consolidated Fuzz",
  "SKU": "1234007"
},
{
  "Name": "ExampleCom",
  "Type": "RedfishServer",
  "Host": "www.example.com"
}
```

In this example, a system has 3 sensors, and two Chassis objects. Naming
differences are taken into account, by providing appropriate strings to be
matched upon. There is one Redfish server. The `ExampleCase` chassis has 2
sensors on it, and the `ExamplePowerSupply` chassis has one sensor on it.

## Footnotes

[^1]: https://gerrit.openbmc.org/q/owner:krellan%2540google.com
[^2]: https://discord.com/users/776585252750753793
[^3]: https://gerrit.openbmc.org/c/openbmc/docs/+/58954
[^4]: https://en.wikipedia.org/wiki/Magnet_URI_scheme
[^5]: https://redfish.dmtf.org/schemas/v1/Chassis.v1_21_0.json
[^6]: https://redfish.dmtf.org/schemas/v1/Sensor.v1_6_0.json
[^7]: https://github.com/openbmc/dbus-sensors
[^8]: https://github.com/openbmc/entity-manager
[^9]: https://github.com/openbmc/entity-manager/blob/master/docs/my_first_sensors.md
[^10]: https://github.com/openbmc/entity-manager/blob/master/docs/entity_manager_dbus_api.md
[^11]: https://redfish.dmtf.org/redfish/mockups/v1/1206
[^12]: http://redfish.dmtf.org/schemas/DSP0266_1.7.0.html#query-parameters
[^13]: https://gerrit.openbmc.org/c/openbmc/bmcweb/+/52272
[^14]: https://github.com/openbmc/telemetry
