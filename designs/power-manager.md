# OpenBMC Power Management

Author: Thang Tran

Other contributors:

Created: June 2nd, 2023

## Problem Description

Currently, OpenBmc does not have any application to monitor and control the
total power consumption without hardware dependence, it can make the system
broken when the total power consumption exceeds the system limit. To protect the
system in this case, this document proposes a design of an application which
monitors and controls total power consumption to protect the system.

## Background and References

The [openpower-occ-control][occ-repo] application is used to monitor and control
the CPU's power. But this application requests [OCC hardware][ooc-hw] which is
unavailable on the non-IBM system. Fortunately, [DCMI][dcmi-spec] specification
provided a solution for this issue.

In chapter 6.6 of [DCMI][dcmi-spec] specification, it defined `Power Management`
application. This application has 2 main features: `Power Monitoring` and
`Power Limit`.

### Power Monitoring

The `Power Monitoring` is used to get System Power Statistics: current, maximum,
minimum and the average of the total power consumption. It supports 2 modes:

- System Power Statistics: The timeframe in this mode is configurable in JSON
  file. Each system only has 1 option for _System Power Statistics_ mode. The
  unit of timeframe in this option is milliseconds.
- Enhanced System Power Statistics: The timeframe in this mode is configurable
  in JSON file. The system can have multiple options for _Enhanced System Power
  Statistics_ mode. The unit of timeframe in this option can be
  seconds/minutes/hours/days.

### Power Limit

The `Power Limit` is used to define a threshold which, if the total power
consumption exceeded for a configurable amount of time, BMC shall trigger a
system power-off action and/or event logging action, based on configured
exception action.

The properties of the `Power Limit` feature are defined in [Power
Cap][power-cap] interface.

## Requirements

As defined in chapter 3.2.1 and 6.6 of [DCMI][dcmi-spec] specification, Power
Management application has to follow the below requirements:

- Platform hardware shall provide power monitoring sensors for input power.
- Power monitoring sensors shall be updated at an average rate of at least once
  per second.
- Power limiting shall perform corrective action if the power limiting control
  fails to lower the power consumption as requested in the form of exception
  actions.
- Power limiting shall provide a configuration option for setting the maximum
  time expected for power limiting, in multiples of power monitor sampling time.
- Platform shall provide the power management controller discovery information,
  if the power management controller is a satellite controller.
- The power limit setting should be persistent across AC and DC cycles.
- The power limit sampling interval is the same as that for the Power Monitoring
  feature.

## Proposed Design

### Architecture Design

The high level architectural design:

```
    +------------------+                   +------------------------+
    |  Dbus-sensors/   |                   | Phosphor-state-manager |
    |  Virtual-sensors |<---+         +--->|                        |
    +------------------+    |         |    +------------------------+
                            |         |
                            |         |
                        +-----------------+
                        |  Power-Manager  |
                        |                 |
                        +-----------------+
                            ^         ^
                            |         |
    +------------------+    |         |    +---------------------+
    |       IPMID      |----+         +----|       RedFish       |
    |                  |                   |                     |
    +------------------+                   +---------------------+
```

The Dbus-sensors reads sensor information to calculate the total power
consumption of the system. The sampling period of this action has to be at least
once per second.

The total power consumption of the system can be indicated by Dbus-sensors or
virtual-sensors application, the `Power Management` has to query the value of
this sensor to update statistics and check the health of the system. When the
total power consumption exceeds the system's limit, the `Power Management` can
request to turn off the HOST via the Phosphor-state-manager application.

IPMID and Redfish are used to transfer the user's configuration for Power Limit
feature.

### D-bus service

Create D-bus service "xyz.openbmc_project.PowerManager" which includes object
paths to `Power Limit` and `Power Monitoring` feature. The structure of this
service like:

```
-/xyz
  └─/xyz/openbmc_project
    └─/xyz/openbmc_project/power_manager
      └─/xyz/openbmc_project/power_manager/power_limit
      └─/xyz/openbmc_project/power_manager/power_monitor
        └─/xyz/openbmc_project/power_manager/power_monitor/standard
        └─/xyz/openbmc_project/power_manager/power_monitor/enhanced_01
        └─/xyz/openbmc_project/power_manager/power_monitor/enhanced_02
        └─/xyz/openbmc_project/power_manager/power_monitor/enhanced_03
```

#### Power Limit

The `/xyz/openbmc_project/power_manager/power_limit` object path includes
`xyz.openbmc_project.Control.Power.Cap` interface which implement `Power Limit`
feature. The properties of this interface are described at below:

| Property        | Type   | Description                                                                                                       |
| --------------- | ------ | ----------------------------------------------------------------------------------------------------------------- |
| CorrectionTime  | uint64 | Minimum amount of time to wait before the Exception Action will be taken                                          |
| ExceptionAction | string | Exception Actions, taken if the Power Limit is exceeded and cannot be controlled within the Correction time limit |
| PowerCap        | uint32 | Power cap (system's limit) value                                                                                  |
| PowerCapEnable  | bool   | Enable/disable Power Limit feature                                                                                |
| SamplingPeriod  | uint64 | Statistics Sampling period                                                                                        |

_Note_: refer [Power Cap][power-cap] interface for more information.

#### Power Monitoring

The `/xyz/openbmc_project/power_manager/power_monitor/<standard | enhanced_XX>`
object paths include `xyz.openbmc_project.Control.Power.Monitor` interface which
implement `Power Monitoring` feature. In the standard mode, it has 1 option and
the units has to be **milliseconds**. But in the enhanced mode, it have 0 or
multiple options and the units can be **seconds/minutes/hours/days**.

The properties of `xyz.openbmc_project.Control.Power.Monitor` interface are
described at below:

| Property | Type                             | Description                                       |
| -------- | -------------------------------- | ------------------------------------------------- |
| Duration | uint64                           | Statistics duration                               |
| Mode     | string                           | Standard or Enhanced System Power Statistics mode |
| Units    | string                           | Statistics duration units                         |
| Value    | [double, double, double, double] | Current, max, min and average values              |

BMC starts `Power Monitoring` feature when host is turned on. When host is
turned off, BMC stops `Power Monitoring` feature and restart in the next power
on.

_Note_: refer to chapter 6.6.1 of [DCMI][dcmi-spec] specification for more
information of `xyz.openbmc_project.Control.Power.Monitor` interface.

### Configuration

#### Initialization

There is a JSON configuration file for sensor's name and statistics properties

```json
{
  "Desc": "The configuration for Power Management",
  "sensor_path": "/xyz/openbmc_project/sensors/power/total_power",
  "power_monitor": {
    "standard": {
      "duration": 100000
    },
    "enhanced": [
      {
        "units": "days",
        "duration": 7
      },
      {
        "units": "hours",
        "duration": 24
      },
      {
        "units": "hours",
        "duration": 1
      }
    ]
  }
}
```

- "sensor_path": The object path of the sensor which includes total power
  consumption. It can be a dbus sensor or virtual sensor.
- "power_monitor": include the information of monitoring.
- "standard": include the duration in milliseconds of the statistics in _System
  Power Statistics_ mode.
- "enhanced": include options of _Enhanced System Power Statistics_ mode.
  - "units": time duration units, it can be "seconds/minutes/hours/days".
  - "duration": time duration.

_Note_: "enhanced" is an optional configuration.

#### Run-time

The [Power Cap][power-cap] interface defines properties which can be configured
by users during run-time. It includes information about the system's limit,
exception action, correction time and sampling period.

### Sampling behavior

Both Power-limit and Power-monitor have the same "sampling interval", this is a
fixed value (at least 1s). 2 features use "sampling interval" as below:

- Power-limit: Read the new system's power after "sampling interval" then
  compare to the configured power limit value.
- Power-monitor: Read the new system's power after "sampling interval" then
  store in the buffer. Power-monitor calculates the Statistics base on the
  buffer and the "Statistics Sampling period", it means that Power-monitor does
  not get all of elements in the buffer, it only gets element each "Statistics
  Sampling period".

`Example`:

- "sampling interval" = 0,5s
- "Statistics Sampling period" = 2s
- Standard mode has timeframe is 100000ms (10s).
- Enhanced mode has timeframe is 60s.

Power-limit shall read the system's power each 0,5s then compare it with the
configured power limit.

Power-monitor shall read the system's power each 0,5s then store to the buffer:

- When users request to read Statistics in standard mode, Power-monitor shall
  get the 5 samples from current time in the buffer (the gap between of samples
  is 2s not 0,5s). Power-monitor calculate Statistics via those 5 samples.
- When users request to read Statistics in Enhanced mode, Power-monitor shall
  get the 30 samples from current time in the buffer (the gap between of samples
  is 2s not 0,5s). Power-monitor calculate Statistics via those 30 samples.

### Logging

If the exception action is _Hard Power Off_ or _Log Event Only_, the
`Power Management` application shall log event in below cases:

- When the total power consumption exceeds the power limit after waiting the
  specified correction time, the `Power Management` application shall log an
  event to inform users that System's power is greater than the limit.
- When the total power consumption drops below the power limit, the
  `Power Management` application shall log an event to inform users that
  System's power is less than the limit.

## Alternatives Considered

The [Telemetry][telemetry-repo] which follow [Redfish Telemetry White
Paper][Redfish Telemetry] specification provides a solution for monitoring
Max/Min/Avg values of sensors. To monitor sensor's values, the
[Telemetry][telemetry-repo] has to create buffer to store reading values for
each metric report. If users want to monitor Max/Min/Avg of a sensor in a time
frame (E.g: 1 hour), users have to create 3 different metric reports, and the
Telemetry has to create 3 buffers to store reading values. In case user want to
monitor Max/Min/Avg total power consumption in multiple time frames (E.g: 1
hour, 5 hours, 1 day ...), the [Telemetry][telemetry-repo] has to create so much
buffer to store reading values for each time frame. On the other side,
power-manager only uses 1 buffer to store reading values of total power
consumption.

The [Redfish Telemetry White Paper][Redfish Telemetry] is using
`CollectionDuration` property to indicate the Interval. And this property is
follow [ISO 8601][ISO-8601], the smallest time units is `seconds`. Therefore the
minimum of Interval is 1000ms, but in Power-monitoring, we expect the maximum of
Interval is 1000ms.

The [Telemetry][telemetry-repo] is design to monitor sensors value, not control
the power, therefore it seems updating Telemetry to control power is
out-of-scope. Using Telemetry to implement power-monitor feature wastes so much
resource to create buffers to store reading values. Therefore, we should keep
current implementation of Telemetry and define new applcation to control and
monitor total power consumption.

## Impacts

In the IPMID and Redfish, they implemented power-limit feature, but it is not
enough. Therefore, we have to update ipmid and Redfish to fully support power
limit feature. Please refer to below link to get more information about
implementation of IPMID and Redfish.

- IPMID:
  https://github.com/openbmc/phosphor-host-ipmid/blob/master/dcmihandler.cpp#L338
- Redfish:
  https://github.com/openbmc/bmcweb/blob/522377dcb85082da598403e104a44d621b4c2bb4/redfish-core/lib/power.hpp#L120

### Organizational

New repository is requested to implement Power Management application:

- phosphor-power-manager:
  - Initial maintainer:
    - Thang Tran
      - Email: thuutran@amperecomputing.com
      - Discord: ThangTran
  - The [Phosphor-host-ipmid][ipmid-repo], [bmcweb][bmcweb-repo] and
    [phosphor-dbus-interfaces][dbus-repo] repositories have to be updated.
  - The list of Gerrit reviewers:
    - Thang Tran
      - Email: thuutran@amperecomputing.com
      - Discord: ThangTran
    - Thang Nguyen
      - Email: thang@os.amperecomputing.com
      - Discord: thangqn

## Testing

### Power Limit

Configure properties of [Power Cap][power-cap] interface then check the behavior
of BMC when total power consumption exceeds the power limit. The BMC's behavior
depended on exception action property:

- No Action: Do nothing.
- Hard Power Off: Turn Off the HOST and add the event logs.
- Log Event Only: Add the event log.
- OEM: Call the OEM action (as defined by users).

When the total power consumption drops below the power limit and the exception
action is "Hard Power Off"/"Log Event Only" then BMC logs an event to inform
users that the system out of critical condition.

### Power Monitoring

Check the current, maximum, minimum and the average of System Power in modes.

[occ-repo]: https://github.com/openbmc/openpower-occ-control
[ooc-hw]: https://github.com/open-power/docs/tree/P10/occ
[dcmi-spec]:
  https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/dcmi-v1-5-rev-spec.pdf
[power-cap]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Control/Power/Cap.interface.yaml
[telemetry-repo]: https://github.com/openbmc/telemetry
[dbus-repo]: https://github.com/openbmc/phosphor-dbus-interfaces
[ipmid-repo]: https://github.com/openbmc/phosphor-host-ipmid
[bmcweb-repo]: https://github.com/openbmc/bmcweb
[Redfish Telemetry]:
  https://www.dmtf.org/sites/default/files/standards/documents/DSP2051_1.0.1.pdf
[ISO-8601]: https://en.wikipedia.org/wiki/ISO_8601#Durations
