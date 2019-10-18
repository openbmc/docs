# Monitoring and Logging OpenBMC Systemd Target Failures

Author: Andrew Geissler
  < geissonator >

  Primary assignee: Andrew Geissler

  Created: June 18, 2019

## Problem Description

OpenBMC uses systemd to coordinate the starting and stopping of its
applications. For example, systemd is used to start all of OpenBMC's initial
services, to power on and off the servers it manages, and to perform operations
like firmware updates.

[openbmc-systemd.md][1] has a good summary of systemd and the basics of how
it is used within OpenBMC.

There are situations where OpenBMC systemd targets fail. They fail due to a
target or service within them hitting some sort of error condition. In some
cases, the service which caused the failure will log an error to
phosphor-logging but in some situations (segfault, unhandled exception,
unhandled error path, ...), there will be no indication to the end user that
something failed. It is critical that if a systemd target fails within OpenBMC,
the user be notified.

## Background and References

See the [phosphor-state-manager][2] repository for background information on
state management and systemd within OpenBMC.

systemd provides signals when targets complete and provides status on that
completed target. See the JobNew()/JobRemoved() section in the [systemd dbus
API][3]. The six different results are:
```
done, canceled, timeout, failed, dependency, skipped
```

phosphor-state-manager code already monitors for these signals but only looks
for a status of `done` to know when to mark the corresponding dbus object
as ready(bmc)/on(chassis)/running(host).

The proposal within this document is to monitor for these other results and
log an appropriate error to phosphor-logging.

## Requirements

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
```
    xyz.openbmc_project.State.BMC.Error.MultiUserTargetFailure
    STATUS=timeout
```
- By default, enable this monitor and logging service for all critical OpenBMC
  targets
  - Critical systemd targets are ones used by phosphor-state-manager
    - BMC: multi-user.target
    - Chassis: obmc-chassis-poweron@0.target, obmc-chassis-poweroff@0.target
    - Host: obmc-host-start@0.target, obmc-host-startmin@0.target,
      obmc-host-shutdown@0.target, obmc-host-stop@0.target,
      obmc-host-reboot@0.target
  - The errors for these must be defined in phosphor-dbus-interfaces
- Limitations:
    - Fully qualified target name must be input (i.e. no templated / wild card
      target support)

## Proposed Design

Create a new standalone application in phosphor-state-manager which will load
json file(s) on startup. The json file(s) would have the following format:
```
{
    "targets" : [
        {
            "name": "multi-user.target",
            "errorsToMonitor": ["default"],
            "errorToLog": "xyz.openbmc_project.State.BMC.Error.MultiUserTargetFailure",
        },
        {
            "name": "obmc-chassis-poweron@0.target",
            "errorsToMonitor": ["timeout", "failed"],
            "errorToLog": "xyz.openbmc_project.State.Chassis.Error.PowerOnTargetFailure",
        }
      ]
}
```

On startup, all input json files will be loaded and monitoring will be setup.

This application will not register any interfaces on D-Bus but will subscribe
to systemd dbus signals for JobRemoved. sdeventplus will be used for all
D-Bus communication.

For additional debug, the errors may be registered with the BMC Dump function to
ensure the cause of the failure can be determined. This requires the errors
logged by this service be put into `phosphor-debug-errors/errors_watch.yaml`.

## Alternatives Considered

Targets have an OnError directive. Could put the error logging logic within that
path but it introduces more complexity to OpenBMC systemd usage which is already
quite complicated.

Could implement this within the existing state manager applications since they
are already monitoring some of these targets. The standalone application and
generic capability to monitor any target was chosen as a better option.

## Impacts

A phosphor-logging event will be logged when one of the targets listed above
fails. This will be viewable by owners of the system. There may be situations
where two logs are generated for the same issue. For example, if the power
application detects an issue during power on and logs it to phosphor-logging and
then returns non zero to systemd, the target that service is a part of will also
fail and the code logic defined in this design will also log an error.

The thought is that two errors are better than none. Also, there is a plugin
architecture being defined for phosphor-logging. The user could monitor for
that first error within the phosphor-logging plugin architecture and as a
defined action, cancel the systemd target. A target status of `canceled` will
not result in phosphor-state-manager generating an error.

## Testing
Need to cause all targets mentioned within this design to fail. They should fail
for each of the reasons defined within this design and the error generated for
each scenario should be verified.

[1]: https://github.com/openbmc/docs/blob/master/architecture/openbmc-systemd.md
[2]: https://github.com/openbmc/phosphor-state-manager
[3]: https://www.freedesktop.org/wiki/Software/systemd/dbus/
