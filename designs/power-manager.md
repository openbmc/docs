# OpenBMC Power Management

Author: Thang Tran

Other contributors:

Created: June 2nd, 2023

## Problem Description

Currently, OpenBmc does not have any application to monitor and control the
total power consumption without hardware dependendce, it can make the system has
been broken when the total power consumption exceeds the system limit. To
protect the system in this case, this document proposes a design of an
application which monitors and controls total power consumption to protect the
system.

## Background and References

The [openpower-occ-control][occ-repo] application is used to monitor and control
the CPU's power. But this application requests [OCC hardware][ooc-hw] which is
unavailable on the non-Intel system. Fortunately, [DCMI][dcmi-spec]
specification provided a solution for this issue.

In chapter 6.6 of [DCMI][dcmi-spec] specification, it defined `Power Management`
application. This application has 2 main features: `Power Monitoring` and
`Power Limit`.

### Power Monitoring

The `Power Monitoring` is used to get System Power Statistics: current, maximum,
minimum and the average of the total power consumption. It supports 2 modes:

- System Power Statistics: The timeframe in this mode is configurable. Each
  system only has 1 opption for _System Power Statistics_ mode.
- Enhanced System Power Statistics: The timeframe in this mode is fixed. The
  system can have multiple options for _Enhanced System Power Statistics_ mode.

### Power Limit

The `Power Limit` is used to define a threshold which, if the total power
consumption exceeded for a configurable amount of time, BMC shall trigger a
system power-off action and/or event logging action.

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

    +------------------+
    |     PSU HWMON    |
    |                  |
    +------------------+
              ^
              |
              |
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

The Dbus-sensors reads the PSU HWMON devices to calculate the total power
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
sevice like:

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

| Property        | Type   | Description                                                                                                                          |
| --------------- | ------ | ------------------------------------------------------------------------------------------------------------------------------------ |
| CorrectionTime  | uint64 | Maximum time taken to limit the power after the platform power has reached the power limit before the Exception Action will be taken |
| ExceptionAction | string | Exception Actions, taken if the Power Limit is exceeded and cannot be controlled within the Correction time limit                    |
| PowerCap        | uint32 | Power cap (system's limit) value                                                                                                     |
| PowerCapEnable  | bool   | Enable/disable Power Limit feature                                                                                                   |
| SamplingPeriod  | uint64 | Total power consumption'S sampling period                                                                                            |

_Note_: refer [Power Cap][power-cap] interface for more information.

#### Power Monitoring

The `/xyz/openbmc_project/power_manager/power_monitor/<standard | enhanced_XX>`
object paths include `xyz.openbmc_project.Control.Power.Monitor` interface which
implement `Power Monitoring` feature. The properties of this interface are
described at below:

| Property  | Type                             | Description                                       |
| --------- | -------------------------------- | ------------------------------------------------- |
| Duration  | uint64                           | Sampling duration                                 |
| Mode      | string                           | Standard or Enhanced System Power Statistics mode |
| TimeStamp | uint64                           | IPMI Specification based Time Stamp               |
| Units     | string                           | Sampling duration units                           |
| Value     | [double, double, double, double] | Current, max, min and average values              |

_Note_: refer to chapter 6.6.1 of [DCMI][dcmi-spec] specification for more
information of `xyz.openbmc_project.Control.Power.Monitor` interface.

### Configuration

#### Initialization

There is a JSON configuration file for sensor's name and statistics's properties

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
- "enhanced": inlcude options of _Enhanced System Power Statistics_ mode.
  - "units": time duration units, it can be "seconds/minutes/hours/days".
  - "duration": time duration.

_Note_: "enhanced" is an option configuration.

#### Run-time

The [Power Cap][power-cap] interface defines properties which can be configured
by users during run-time. It includes information about the system's limit,
exception action, correction time and sampling period.

### Logging

When the total power consumption exceeds/drops below the power limit,
`Power Management` application shall log an event if the exception action is
_Hard Power Off_ or _Log Event Only_ to inform users the status of the system's
+power.

## Alternatives Considered

The [Telemetry][telemetry-repo] implements middleware for sensors and metrics
aggregation, it can be used to implement `Power Monitoring` feature.

## Impacts

The IPMID and Redfish are used to transfer user's requests to the power
management application, therefore, they have to be updated to adapt to this
application.

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
of BMC when total power consumption exceeds/drops below the power limit. The
BMC's behavior depended on exception action property:

- No Action: Do nothing.
- Hard Power Off: Turn Off the HOST and add the exceeds/drops below event logs.
- Log Event Only: Add the exceeds/drops below event logs.
- OEM: Call the OEM action (as defined by users).

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
