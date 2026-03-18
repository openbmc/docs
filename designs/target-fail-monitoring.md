# Monitoring and Logging OpenBMC Systemd Target and Service Failures

Author: Andrew Geissler < geissonator >

Created: June 18, 2019 Last Updated: Feb 21, 2022

## Problem Description

OpenBMC uses systemd to coordinate the starting and stopping of its
applications. For example, systemd is used to start all of OpenBMC's initial
services, to power on and off the servers it manages, and to perform operations
like firmware updates.

[openbmc-systemd.md][1] has a good summary of systemd and the basics of how it
is used within OpenBMC.

At a high level, systemd is composed of units. For OpenBMC, the two key unit
types are target and service.

There are situations where OpenBMC systemd targets fail. They fail due to a
target or service within them hitting some sort of error condition. In some
cases, the unit which caused the failure will log an error to phosphor-logging
but in some situations (segfault, unhandled exception, unhandled error path,
...), there will be no indication to the end user that something failed. It is
critical that if a systemd target fails within OpenBMC, the user be notified.

There are also scenarios where a system has successfully started all targets but
a running service within that target fails. Some services are not all that
critical, but something like fan-control or a power monitoring service, could be
very critical. At a minimum, need to ensure the user of the system is informed
that a critical service has failed. The solution proposed in this document does
not preclude service owners from doing other recovery, this solution just
ensures a bare minimum of reporting is done when a failure occurs.

## Background and References

See the [phosphor-state-manager][2] repository for background information on
state management and systemd within OpenBMC.

systemd provides signals when units complete and provides status on that
completed unit. See the JobNew()/JobRemoved() section in the [systemd dbus
API][3]. The six different results are:

```text
done, canceled, timeout, failed, dependency, skipped
```

phosphor-state-manager code already monitors for these signals but only looks
for a status of `done` to know when to mark the corresponding dbus object as
ready(bmc)/on(chassis)/running(host).

The proposal within this document is to monitor for these other results and log
an appropriate error to phosphor-logging.

A systemd unit that is a service will only enter into a `failed` state after all
systemd defined retries have been executed. For OpenBMC systems, that involves 2
restarts within a 30 second window.

## Requirements

### Systemd Target Units

- Must be able to monitor any arbitrary systemd target and log a defined error
  based on the type of failure the target encountered
- Must be configurable
  - Target: Choose any systemd target
  - Status: Choose one or more status's to monitor for
  - Error to log: Specify error to log when error is detected
- Must be able to take multiple configure files as input on startup
- Support a `default` for errors to monitor for
  - This will be `timeout`,`failed`, and `dependency`
- Error will always be the configured one with additional data indicating the
  status that failed (i.e. canceled, timeout, ...)
  - Example:

```text
    xyz.openbmc_project.State.BMC.Error.MultiUserTargetFailure
    STATUS=timeout
```

- By default, enable this monitor and logging service for all critical OpenBMC
  targets
  - Critical systemd targets are ones used by phosphor-state-manager
    - BMC: multi-user.target
    - Chassis: `obmc-chassis-poweron@0.target, obmc-chassis-poweroff@0.target`
    - Host: `obmc-host-start@0.target, obmc-host-startmin@0.target`,
      `obmc-host-shutdown@0.target`, `obmc-host-stop@0.target`,
      `obmc-host-reboot@0.target`
  - The errors for these must be defined in phosphor-dbus-interfaces
- Limitations:
  - Fully qualified target name must be input (i.e. no templated / wild card
    target support)

### Systemd Service Units

- Must be able to monitor any arbitrary systemd service within OpenBMC
  - Service(s) to monitor must be configurable
- Log an error indicating a service has failed (with service name in log)
  - `xyz.openbmc_project.State.Error.CriticalServiceFailure`
- Collect a BMC dump
- Changes BMC state (CurrentBMCState) to indicate a degraded mode of the BMC
- Report changed state externally via Redfish managers/bmc state

### Immediate-Quiesce Service Monitoring

The service monitoring described above relies on the systemd `JobRemoved`
signal and its `result` field. This works well for `Restart=no` services
that exhaust their restart limit, but has two blind spots:

1. **`Restart=always` services** — when such a service crashes, systemd
   immediately schedules a restart. If the restart succeeds, the
   `JobRemoved` result is `done`, so the monitor never fires. However,
   even a brief crash of a critical service (e.g. ObjectMapper) can
   cause dependent D-Bus calls to fail permanently, leaving the system
   in a degraded state with no indication.

2. **`Restart=no` services during normal operation** — if these crash
   outside of a target transition, no new job is created and no
   `JobRemoved` signal is emitted, so the monitor is also blind.

To address this, an optional `immediate_quiesce_services` key can be
added to the same JSON file used for service monitoring (`-s`). For each
service listed under this key, a D-Bus `PropertiesChanged` match is
installed on its systemd unit's `ActiveState` property. When
`ActiveState` becomes `"failed"`, the monitor immediately:

- Collects a BMC dump
- Logs `xyz.openbmc_project.State.Error.CriticalServiceFailure`
- Triggers BMC quiesce (`obmc-bmc-service-quiesce@0.target`)

This fires as soon as systemd marks the unit failed — before any
restart — so the failure is always captured regardless of the unit's
restart policy.

Note that only `"failed"` triggers quiesce. Normal administrative
operations (`systemctl stop`, `systemctl restart`, BMC shutdown)
transition through `"inactive"`, not `"failed"`, so they do not cause
false alarms.

**Important:** BMC quiesce is a significant operation — the BMC enters
a degraded state that typically requires manual intervention to recover.
Only add services to `immediate_quiesce_services` when a single
transient crash truly causes irreparable damage. Most services recover
fine after a systemd restart and should **not** be listed here.

## Proposed Design

Create a new standalone application in phosphor-state-manager which will load
json file(s) on startup.

The json file(s) would have the following format for targets:

```json
{
  "targets": [
    {
      "name": "multi-user.target",
      "errorsToMonitor": ["default"],
      "errorToLog": "xyz.openbmc_project.State.BMC.Error.MultiUserTargetFailure"
    },
    {
      "name": "obmc-chassis-poweron@0.target",
      "errorsToMonitor": ["timeout", "failed"],
      "errorToLog": "xyz.openbmc_project.State.Chassis.Error.PowerOnTargetFailure"
    }
  ]
}
```

The json (files) would have the following format for services:

```json
{
  "services": [
    "xyz.openbmc_project.biosconfig_manager.service",
    "xyz.openbmc_project.Dump.Manager.service"
  ]
}
```

An optional `immediate_quiesce_services` key may be included in the
same json file. Services listed here are monitored via
`PropertiesChanged` on their systemd unit `ActiveState` property and
trigger immediate BMC quiesce on failure:

```json
{
  "services": [
    "xyz.openbmc_project.biosconfig_manager.service",
    "xyz.openbmc_project.Logging.service"
  ],
  "immediate_quiesce_services": [
    "xyz.openbmc_project.ObjectMapper.service",
    "phosphor-multi-gpio-monitor.service"
  ]
}
```

The two lists serve different severity levels and should generally
contain different services:

- `services` — quiesce only after all systemd restart retries are
  exhausted (via JobRemoved). Suitable for services that usually
  recover fine after a restart.
- `immediate_quiesce_services` — quiesce on the **first**
  `ActiveState=failed`, even before systemd attempts a restart.
  Suitable for services where even a brief crash causes irreparable
  damage.

A service in `immediate_quiesce_services` does not need to also appear
in `services` — the immediate mechanism fires first, making the
JobRemoved fallback redundant. Files without the
`immediate_quiesce_services` key are silently skipped for that part
(backward compatible).

On startup, all input json files will be loaded and monitoring will be setup.

This application will not register any interfaces on D-Bus but will subscribe to
systemd dbus signals for JobRemoved (target and service monitoring) and
PropertiesChanged on individual unit objects (immediate-quiesce service
monitoring). sdeventplus will be used for all D-Bus communication.

For additional debug, the errors may be registered with the BMC Dump function to
ensure the cause of the failure can be determined. This requires the errors
logged by this service be put into `phosphor-debug-errors/errors_watch.yaml`.

For service failures, a dump will be collected by default because the BMC will
be moved into a Quisced state.

Note that services which are short running applications responsible for
transitioning the system from one target to another (i.e. chassis power on/off,
starting/stopping the host, ...) are critical services, but they are not the
type of services to be monitored. Their failures will cause systemd targets to
fail which will fall into the target monitoring piece of this design. The
service monitoring is meant for long running services which stay running after a
target has completed.

## Alternatives Considered

Units have an OnError directive. Could put the error logging logic within that
path but it introduces more complexity to OpenBMC systemd usage which is already
quite complicated.

Could implement this within the existing state manager applications since they
are already monitoring some of these units. The standalone application and
generic capability to monitor any unit was chosen as a better option.

## Impacts

A phosphor-logging event will be logged when one of the units listed above
fails. This will be viewable by owners of the system. There may be situations
where two logs are generated for the same issue. For example, if the power
application detects an issue during power on and logs it to phosphor-logging and
then returns non zero to systemd, the target that service is a part of will also
fail and the code logic defined in this design will also log an error.

The thought is that two errors are better than none. Also, there is a plugin
architecture being defined for phosphor-logging. The user could monitor for that
first error within the phosphor-logging plugin architecture and as a defined
action, cancel the systemd target. A target status of `canceled` will not result
in phosphor-state-manager generating an error.

## Testing

Need to cause all units mentioned within this design to fail. They should fail
for each of the reasons defined within this design and the error generated for
each scenario should be verified.

For immediate-quiesce service monitoring, kill a monitored service
(e.g. `systemctl kill -s SIGKILL xyz.openbmc_project.ObjectMapper.service`)
and verify:

- A `CriticalServiceFailure` error is logged with the correct unit
  name
- A BMC dump is collected
- `CurrentBMCState` transitions to `Quiesced`
- The quiesce fires even if the service restarts successfully
  afterwards

[1]: https://github.com/openbmc/docs/blob/master/architecture/openbmc-systemd.md
[2]: https://github.com/openbmc/phosphor-state-manager
[3]: https://www.freedesktop.org/wiki/Software/systemd/dbus/
