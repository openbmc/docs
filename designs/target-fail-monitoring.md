# Monitoring and Logging OpenBMC Systemd Target Failures

Author: Andrew Geissler
  < geissonator >

  Primary assignee: Andrew Geissler

  Created: June 18, 2019

## Problem Description

OpenBMC uses systemd to manage itself. For example, systemd is used to
start all of OpenBMC's initial services, to power on and off the servers it
manages, and to perform operations like firmware updates.

[openbmc-systemd.md][1] has a good summary of systemd and the basics of how
it is used within OpenBMC.

There are situations where OpenBMC systemd targets fail. They fail due to a
target or service within them hitting some sort of error condition. In some
cases the service which caused the failure will log an error to phosphor-logging
but in some situations (segfault, unhandled exception, unhandled error path,
...), there will be no indication to the end user that something failed. It is
critical that if a systemd target fails within OpenBMC, the user be notified.

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

## Requirements

- Must log error if critical OpenBMC systemd target fails
  - Critical systemd targets are ones used by phosphor-state-manager
    - BMC: multi-user.target
    - Chassis: obmc-chassis-poweron@.target, obmc-chassis-poweroff@.target
    - Host: obmc-host-start@.target, obmc-host-startmin@.target,
      obmc-host-stop@.target
  - Error must be specific to the target that failed and the reason it failed
- Error must trigger BMC Dump to ensure cause of failure can be determined

## Proposed Design

phosphor-state-manager currently has three applications already monitoring for
the JobRemoved signal and looking for their corresponding target(s). The
proposal here is to enhance that code to also look for a result of timeout,
failed, and dependency. If it finds one of these errors it will log the
appropriate error to phosphor-logging.

## Alternatives Considered

Targets have an OnError directive. Could put the error logging logic within that
path but it introduces more complexity to OpenBMC systemd usage which is already
quite complicated.

Could create a standalone application within phosphor-state-manager that does
all monitoring and error logging. This seems redundant since we already have
applications monitoring for the signals and putting the error creation within
them seems more architecturally sound.

Could make the enablement of this error logging compile based (i.e. company can
choose if they want this enabled or not). Will see during the review of this
document if anyone would like to opt out.

## Impacts

A phosphor-logging event will be logged when one of the targets listed above
fails. This will be viewable by owners of the system. There may be situations
where two logs are generated for the same issue. For example, if the power
application detects an issue during power on and logs it to phosphor-logging and
then returns non zero to systemd, the target that service is a part of will also
fail and the code logic defined in this design will also log an error.

The thought is that two errors are better then none. Also, there is a plugin
architecture being defined for phosphor-logging. The user could monitor for
that first error within the phosphor-logging plugin architecture and as a
defined action, cancel the systemd target. A target status of `canceled` will
not result in phosphor-state-manager generating an error.

## Testing
Need to cause all targets mentioned within this design to fail. They should fail
for each of the reasons defined within this design and the error generated for
each scenario should be verified.

[1]: https://github.com/openbmc/docs/blob/master/openbmc-systemd.md
[2]: https://github.com/openbmc/phosphor-state-manager
[3]: https://www.freedesktop.org/wiki/Software/systemd/dbus/
