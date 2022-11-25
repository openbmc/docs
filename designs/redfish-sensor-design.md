# RedfishSensor Design

Author: Josh Lehan[^1] (krellan@[^2])

Created: November 21, 2022

## Introduction

This is another sensor-reading program, to be added to the suite of
already-existing sensor-reading daemons in the `dbus-sensors`[^3] package.
Unlike the existing daemons, this new program is capable of reading sensor data
from a Redfish server.

## Problem Description

There already exists a good software stack for handling sensor data within
OpenBMC, using D-Bus as an interprocess communications mechanism. Both the
producers (the `dbus-sensors` suite of daemons) and consumers (the thermal PID
loop) use this method and it works well. However, it works only for local
sensors, reachable by I2C or similar direct hardware connection, or for remote
data passively pushed into a local `ExternalSensor` container. There is no way
to actively pull in data from remote sensors, from somewhere else on the
network, reachable by the Redfish[^4] protocol. We want to have a thermal PID
loop that takes both local and remote sensors into account. To do this, we need
to be able to read a Redfish-published sensor from another device, and add it to
a locally-running PID loop.

## Background

The `dbus-sensors` package is a suite of daemons, each daemon working with a
particular class of hardware, typically a variety of sensor chips communicating
over the I2C bus. Each of these daemons publishes the sensors that it knows how
to read from, in the same way, so that they all form one large usable set of
sensors on the BMC.

The Redfish protocol is layered over the familiar HTTP protocol used with Web
servers. In other words, a Redfish server is a Web server with the special
purpose of providing Redfish service. In Redfish, all responses are JSON[^5],
and the JSON format is highly structured and rigorously made consistent, like a
database, so that it can be standardized. This allows it to work across BMC
vendors, and all Redfish servers should ideally look and feel the same, unlike
traditional management Web pages which were originally designed to be
human-readable and interactive, making them difficult to automate, and
inconsistent between vendors. By standardizing the interface using JSON, this
allows Redfish software to be written in a generic manner, to work with any
hardware supported by Redfish, to provide system management that can be
automated and scale up to an arbitrary number of devices. In this way, it
replaces and upgrades older local management protocols like IPMI, and network
management protocols like SNMP.

## Requirements

To make use of the new `RedfishSensor` daemon, a Redfish server is required.
This can either run locally on the BMC, such as the `bmcweb`[^6] daemon which is
the OpenBMC reference implementation of a Redfish server, or remotely. Providing
network reachability to the desired Redfish server is beyond the scope of
`RedfishSensor`, as it is assumed other packages on the BMC will take care of
correctly setting up the network configuration.

The `RedfishSensor` daemon should be responsive to local queries, returning
sensor data through D-Bus in a timely manner that is indistinguishable from
querying local hardware. As far as the network connection goes, `RedfishSensor`
will do the best it can. As is standard practice with `dbus-sensors` daemons,
the program will be written to use asynchronous I/O[^7], so the program will
itself remain responsive for local queries, even when remote network connections
are blocked waiting for network I/O.

`RedfishSensor` will make a single outbound TCP connection to each server. This
connection will be used to read all sensors configured to use that server.
Between polling intervals, this connection will be held open (via HTTP
keep-alive, aka persistent connections) if possible, to avoid having to reopen
it each time. Using more than one TCP connection at a time, for potential faster
performance, might be the subject of future work.

It is considered good practice for a Redfish client, such as `RedfishSensor`, to
not depend on exact URL paths within the Redfish server, as they can vary over
time, as components are added or removed. Although using exact URL paths would
simplify the implementation, sensors for use with `RedfishSensor` will be
configured indirectly, to avoid having to hardcode an exact URL path into the
configuration. Attributes of desired Redfish `Sensor` objects, as well as the
Redfish `Chassis` object that each sensor resides in (Redfish uses this
two-layer addressing scheme for sensors)[^8], will be specified as desired
attribute values. The desired sensor will then be found indirectly. This is
similar to a "magnet link"[^9] that might be familiar from peer-to-peer file
sharing services. The Redfish server will be queried at startup, to find the
sensors that best match these attributes. The resulting URL of these sensors
will then be used, for the remainder of the uptime of the `RedfishSensor`
daemon.

The bandwidth requirement will vary, based on the amount of data the Redfish
server returns for each query. As Redfish is a text-based format, the response
sizes vary, depending on length of strings, addition of optional fields, and
such. The sensor queries will be repeated on a regular basis, with the design
goal being one-second intervals for all sensors. There will also be a larger
amount of data consumed at program startup, as `RedfishSensor` must enumerate
all `Chassis` and `Sensor` objects available on the Redfish server, to find
objects that best match the desired sensors from the configuration.

## Proposed Design

Design and implement another daemon to be added to the existing suite of
`dbus-sensors` daemons, namely, `RedfishSensor`. This follows the naming
convention of the existing sensor daemons. It will behave similarly to them,
publishing one or more sensors onto D-Bus, following the existing interfaces and
examples of usage thereof. It will update the values of these sensors at regular
intervals. However, instead of reading from I2C hardware, however, it will be a
Redfish client, reading from arbitrary Redfish servers on the network.

Unlike most of the programs already in the `dbus-sensors` suite, `RedfishSensor`
does not read from hardware. Instead, it creates a D-Bus object which is a
virtual sensor, and populates it with data that comes in from elsewhere. In this
way, it is similar to the already-existing `ExternalSensor`[^10] program.
However, the intended operation of these programs are quite different.

`ExternalSensor` operates on a "push" design. `ExternalSensor` creates a virtual
sensor, following the model of existing sensors on D-Bus, then passively awaits
for data to be pushed in from an arbitrary external source. Another service
running on the BMC, typically IPMI or Redfish, receives external requests and
issues the appropriate D-Bus sensor write messages. `ExternalSensor` expects
this data to arrive on a regular basis, and validates the liveliness of the data
by marking it as stale if a new update has not arrived in time.

On the other hand, `RedfishSensor` operates on a "pull" design. `RedfishSensor`
creates a virtual sensor, then actively goes out and connects to another source
on the network, a Redfish server. `RedfishSensor` performs a Redfish query,
pulling in data from this server, and then placing that data into a D-Bus sensor
object. `RedfishSensor` repeats this on a regular basis, to prevent the sensor
data from becoming stale. The `Reading`[^11] field, of the JSON reply from the
Redfish server, is mirrored to the `Value` field of the D-Bus interface.

Like `ExternalSensor`, this new `RedfishSensor` daemon will ensure that its
created virtual sensors look and feel similar to all the other sensors, so the
existing D-Bus software stack will continue to process them normally, allowing
features such as PID control to work unmodified. Both `ExternalSensor` and
`RedfishSensor` serve a similar purpose, that is, to accept and integrate
externally-provided data into the existing framework of D-Bus objects
representing sensors on the BMC. This allows that data to be used by other
services on the BMC which make use of D-Bus objects, such as
`phosphor-pid-control`[^12] for thermal management.

## Alternatives Considered

*   Instead of writing a new program, why not integrate with the existing
    `bmcweb` Redfish server, which is the standard way of providing Redfish
    service within OpenBMC?

Reason is, `RedfishSensor` is to be a Redfish client, not a server. Although
there is some Redfish client code within `bmcweb` already, which would allow for
some commonalities and code reuse, the client and server work differently enough
that they would essentially need to be two different programs anyway. Among
other things, the server passively waits for incoming connections and services
them, but the client uses a timing loop to unilaterally initiate new outgoing
connections on a regularly repeating basis. There is currently no timing loop
within `bmcweb`, as it is designed to passively wait for incoming network
requests, servicing them on a per-request basis, without unilaterally initiating
new requests on its own. Also, the `bmcweb` program is quite large already, and
this would add further complexity to it.

*   Instead of fetching Redfish data and incorporating it into a D-Bus sensor,
    why not extend D-Bus over the network, so that remote sensors could be
    addressed similarly to local sensors?

Reason is, this would be a great deal of additional work, and introduce security
issues. OpenBMC itself used to expose an interface to allow D-Bus to be directly
accessed from the network, before realizing that it opened up a lot of security
holes and needed to be closed off. In other words, D-Bus is a trusted interface.

*   Instead of writing a new sensor program, why not add Redfish client support
    to existing consumers of `dbus-sensors` sensor data, such as
    `phosphor-pid-control` for fan control?

Reason is, it would take just as much work, and it would only solve the problem
for one particular `dbus-sensors` consumer, instead of exposing a general
solution that could be used for much more than fan control if desired.

*   Instead of writing a new sensor program, why not just use an existing
    Redfish aggregator, to bring the desired remote Redfish sensors into the
    local Redfish server?

Reason is, this still doesn't solve the problem of how to get the sensor data
from Redfish into the D-Bus sensor processing pipeline, as used by
`phosphor-pid-control` and other programs that make use of D-Bus sensor values.
It would then become necessary to extend those programs to speak the Redfish
protocol, as per the previous question.

*   Instead of adding a new `dbus-sensors` sensor daemon, why not extend
    `bmcweb` to perform Redfish aggregation, that is, embed a remote Redfish
    server into an existing local Redfish server, appending the resources
    published by that remote server onto the list of local resources already
    being published locally? Then, wouldn't the the end user be able to access
    and query remote sensors just as easily as if they were local sensors?

Reason is, yes, this would work for the end user, but in this case, the end user
is not a Redfish client. Instead, the end user is `phosphor-pid-control` and
other programs that are specially coded to read from D-Bus sensors. There still
needs to be a way to get this remote Redfish data onto D-Bus, otherwise the
underlying problem is still not yet solved.

*   What about retry and failure? Network sockets fail in different ways than
    local kernel sysfs files.

Although this is an implementation detail, perhaps it should be addressed here.
If there is a network connection failure, it will be retried during the next
sensor reading interval. If the network connection works but returns unexpected
data, or the data is not parseable as expected, that will also be treated the
same way as if it were a network connection failure. If the network connection
blocks for a long period of time, there will be a timeout period, again before
treating it as a failure. If the network is slow or blocked, and it takes longer
than one sensor reading interval to read all the sensors, the nonresponsive
sensors will be skipped. These events will be noted in metrics maintained
internally by `RedfishSensor` (publishing them is a future feature), as well as
noting the time taken for successful sensor readings. Successful sensor readings
will be internally timestamped, and if enough time has elapsed since the last
known good sensor reading, the sensor value will be forcefully replaced with
`NaN`, to avoid wrongly serving stale data, similar to how `ExternalSensor`
currently handles timeouts.

## Impacts

If successfully configured and started, the `RedfishSensor` daemon will publish
additional D-Bus sensors, visible over D-Bus just like any other sensors in
OpenBMC. If IPMI is in use, these additional sensors will also appear in the
IPMI SDR. However, these additional sensors will not be published by the
`bmcweb` Redfish server. This is made possible by the fact that the sensors
published by RedfishSensor will intentionally have no Chassis assigned to them.
Since `bmcweb` groups sensors by Chassis before serving them, and does not
publish sensors unless they correspond to a known Chassis, this works. This
effectively hides the RedfishSensor sensors from `bmcweb`. This is intentional,
to avoid the possibility of an infinite loop of recursive sensor reading. Also,
if it is desired to mirror remote Redfish sensors onto the local Redfish server
for local serving, a Redfish aggregator is a better solution.

RedfishSensor will make use of the existing API of these services. There should
be no change to the API of D-Bus, IPMI, or Redfish. If successful, the
`RedfishSensor` daemon will look and feel just like other `dbus-sensors`
daemons. Configuration will be done similar to the existing `dbus-sensors`
daemons, namely, using D-Bus inventory objects maintained by Entity Manager to
know which sensors to look for and use. The `Type` field, of each of these
objects, serves to route them to the correct daemon. `RedfishSensor` claims
three new values for the `Type` field: `RedfishSensor`, `RedfishChassis`, and
`RedfishServer`.

Security impact should be equal to that of a Redfish client running on the BMC,
since `RedfishSensor` is a Redfish client. It is possible a malicious Redfish
server could attempt a buffer overflow by replying with deliberately malformed
data, as would be expected of an attacker. Sanity checking string lengths and
reply sizes should mitigate this somewhat, as well as using good programming
practice and C++ strings throughout, never C strings.

Setting up the configuration is non-trivial, as there are three different object
types that must be configured, as mentioned above. A `RedfishServer` can contain
one or more `RedfishChassis` objects. A `RedfishChassis` can contain one or more
`RedfishSensor` objects. A companion document is being prepared, to teach how to
prepare the configuration. By using different types of objects, instead of
redundantly requiring the chassis and server to be fully included within the
configuration for each sensor, it is possible to set up a relation between these
objects, saving a lot of data entry if there are many sensors.

As for performance, a remote Redfish server is expected to respond slower than a
local sensor chip over a hardware bus such as I2C. Given this, the rate at which
a sensor can be updated is limited by the Redfish server responsiveness, and by
overall network performance. To mitigate this, making use of alternative Redfish
query types, instead of reading each sensor individually, is a future project.
Some Redfish servers support the `expand`[^13] query parameter, which can expand
related sensors into a single large query and reply. Some Redfish servers
support an alternative listing of sensors in a single reply, using the
`Telemetry`[^14] or `Metrics` features of Redfish, which is also an option. This
will be the subject of future work, as there are many ways to do this and it
does not seem to be fully consistent within practice yet.

As for developer impact, it will require no additional burden. The
`dbus-sensors` package will continue to compile and install, in the usual way.
As for upgradability impact, `RedfishSensor` will follow Redfish standards for
iterating through the server replies and querying the resulting information, so
as long as the Redfish server continues to obey the standards, it should be fine
to upgrade the Redfish server.

### Organizational

No new repository is required. No new package is required, as this simply
extends the `dbus-sensors` package.

## Testing

`RedfishSensor` can be tested using D-Bus calls to verify correct operation of
the sensor interfaces, same as any other `dbus-sensors` sensor daemon. Unlike
`ExternalSensor`, but like a typical hardware-based sensor, external writes are
not accepted, so there is no need to test external writability.

Because `RedfishSensor` reads from a Redfish server, the existing Redfish mockup
server[^15] can be used. This can make `RedfishSensor` actually easier to test
than hardware-based sensors, as the reported sensor values will be easy to
control from the Redfish server. The Redfish server does not have to be a remote
machine, it can run locally as `localhost`.

## Footnotes

[^1]: https://gerrit.openbmc.org/q/owner:krellan%2540google.com
[^2]: https://discord.com/users/776585252750753793
[^3]: https://github.com/openbmc/dbus-sensors
[^4]: https://www.dmtf.org/standards/redfish
[^5]: https://www.json.org/json-en.html
[^6]: https://github.com/openbmc/bmcweb
[^7]: https://www.boost.org/doc/libs/1_80_0/doc/html/boost_asio.html
[^8]: https://redfish.dmtf.org/redfish/schema_index
[^9]: https://en.wikipedia.org/wiki/Magnet_URI_scheme
[^10]: https://github.com/openbmc/docs/blob/master/designs/external-sensor.md
[^11]: https://redfish.dmtf.org/schemas/v1/Sensor.v1_6_0.json
[^12]: https://github.com/openbmc/phosphor-pid-control
[^13]: https://gerrit.openbmc.org/c/openbmc/bmcweb/+/52418
[^14]: https://github.com/openbmc/telemetry
[^15]: https://github.com/DMTF/Redfish-Mockup-Server
