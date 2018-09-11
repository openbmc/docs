### hwmon Alarms in IPMI SEL

Author:
  [Benjamin Fair](mailto:benjaminfair@google.com), benjaminfair
Primary assignee:
  None
Other contributors:
  None
Created:
  10 September, 2018

#### Problem Description
The goal of this proposal is to allow reporting sensor alarms from hwmon devices
as events in the IPMI System Event Log (SEL). Whenever an alarm occurs that the
system is configured to monitor, an event will be generated with the proper
fields and added to the SEL. This proposal does not take into account other
possible types of events that are unrelated to hwmon alarms.

#### Background and References
The IPMI specification includes a particular log format called the System Event
Log[1] which is used to communicate information about events in a standardized
way. These events can be error conditions or other occurances that the system
manager may be interested in.

OpenBMC currently includes limited support for generating SEL entries based on
parsing the D-Bus event log[2]. It does not yet support some features that are
required in order to report sensor alarms.

[1] Intelligent Platform Management Interface Specification v2.0 rev 1.1,
section 31.
[2] https://github.com/openbmc/phosphor-host-ipmid/blob/0b02be925a29357f69abbc9e9a58e7c5aaed2eab/selutility.cpp#L129

#### Requirements
Each time an hwmon device changes the 'alarm' property from '0' to '1' or vice
versa, if a configuration file specifies that this alarm should be monitored, an
entry will be generated and placed in the IPMI SEL. The fields of the entry must
be populated based on the type of sensor (temperature, voltage, etc.), its ID
number, which threshold was crossed (critical or warning), the direction of the
crossing, the value of the sensor when the threshold was crossed, and the value
of the threshold itself. The entry will also contain a timestamp, record type,
and record ID according to the SEL specification.

The user of this data will be anyone who can issue IPMI commands to the BMC,
such as a client program running on the host. It will be used as part of the
monitoring infrastructure in a similar way to the sensor support in IPMI.

#### Proposed Design
Most of the new functionality will be in `phosphor-hwmon` and
`phosphor-host-ipmid`. It will be supported by error types in
`phosphor-dbus-interfaces`.

##### `phosphor-hwmon`
A new option will be added to the configuration file. It will look like
`EVENTS_temp1 = "CRITHI,WARNLO"` or `EVENTS_temp2 = "ALL"`. This option will
either list the specific thresholds which will result in events being generated
or contain `"ALL"` to generate events for all active thresholds.

When a threshold is crossed which appears in an `EVENTS` variable,
`phosphor-hwmon` will use the `phosphor-logging` client API to generate an
event. It will populate the event's metadata with the path of the relevant
sensor, the value of the sensor, and the direction of the crossing (assertion or
deassertion).

##### `phosphor-host-ipmid`
The functionality in `phosphor-host-ipmid` which parses log entry objects and
converts them into SEL entries will be extended to populate the rest of the
fields in the SEL entry. This data will come from metadata fields of the event
along with D-Bus properties from the Threshold object exported by
`phosphor-hwmon`.

##### `phosphor-dbus-interfaces`
The error types for `xyz.openbmc_project.Sensor.Threshold.CriticalHigh` and
`CriticalLow` will be extended to include the path of the sensor which generated
the error in addition to the value of the sensor. The direction of the threshold
crossing will also be added as metadata. Matching types for `WarningHigh`,
`WarningLow`, `CatastrophicHigh`, and `CatastrophicLow` events will be created
with the same set of metadata for non-critical and non-recoverable states
respectively.

#### Alternatives Considered
Instead of adding this functionality to `phosphor-hwmon`, an entirely new daemon
could be created. This idea was rejected because all of the relevant information
is already known by `phosphor-hwmon`, so communicating it to another daemon
would be redundant. The approach of adding the functionality to
`phosphor-dbus-monitor` was rejected for similar reasons.

#### Impacts
There will be no impact to the runtime API, since the IPMI SEL commands are
already implemented. The configuration format and documentation for
`phosphor-hwmon` will have to be updated to reflect the new `EVENTS` option.
Since event log objects are being created, enabling this feature will have a
small performance impact on the rest of the system, but if it is left disabled
there will be no effect.

#### Testing
Unfortunately, this feature is difficult to perform end-to-end tests on, since
it depends on external hardware reporting a potentially dangerous situation such
as over-temperature or over-voltage. This can be worked around by setting the
thresholds very close to the normal values for a sensor and waiting for them to
be crossed. The functionality within `phosphor-hwmon` and `phosphor-host-ipmid`
can be simply tested using unit testing.
