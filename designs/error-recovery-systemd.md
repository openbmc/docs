# Error Recovery on OpenBMC

Author: Andrew Geissler
  < geissonator >

Primary assignee: Andrew Geissler

Created: June 7, 2019

## Problem Description

As OpenBMC becomes more mature, its critical to have defined infrastructure
and best practices for how error situations are handled. For example, the
expected behavior when a server power on/off fails or when a service detects
a critical failure in which they need to perform some sort of system-wide
recovery. The goal is to define the requirements on OpenBMC infrastructure
and the requirements on OpenBMC applications that use this infrastructure.


## Background and References

OpenBMC uses systemd to manage itself. For example, systemd is used to
start all of OpenBMC's initial services, to power on and off the servers it
manages, and to perform operations like firmware updates.

[openbmc-systemd.md][1] has a good summary of systemd and the basics of how
it is used within OpenBMC.

There is a section on best practices for Error Handling. This document looks
to build on that.

Certain companies will have their own requirements on what happens during
different error scenarios. For example, some may wish to display a specific
error to the op-panel, some may have different services which need to run to
recover. The goal is to have common systemd error targets for the common
OpenBMC systemd targets in which companies can put their own custom services
as needed.

In general, an application should simply return non-zero to systemd and let the
defined recovery and error targets be run when an error is encountered. However
there are situations where an application may detect an error outside of a
systemd target or needs to run a more specific error path. In these cases the
application will use a defined OpenBMC systemd error target template. This
ensures all error paths follow a standard path and allows companies to put
their own hooks as needed for these error paths.

Applications that are calling error targets which will cause the host to stop
running must use extreme caution. Causing the host to stop running can have
large implications to users of OpenBMC if it's for example taking down a server
which is running a critical workload. All efforts should be taken to avoid
impacting the host state in error conditions.

A common complaint on OpenBMC's usage of systemd has been the difficulty
in visualizing the interactions of all the different targets within OpenBMC.
Before adding any more complexity to OpenBMC, we must find a solution to this.

There are complexities users need to be aware of if they are doing non-standard
systemd services. i.e. the `Type=oneshot` and `RemainAfterExit=no`. If not
using these then in a lot of cases, the OpenBMC systemd error target templated
should be used.

## Requirements

- Must provide an easy mechanism to visualize and review the OpenBMC targets
- Must have common OpenBMC based error recovery systemd target for all OpenBMC
  common targets
    - Error recovery target must follow a standard naming convention:
    ```
      obmc-chassis-poweron@.target -> obmc-chassis-poweron-error@.target
      obmc-host-start@.target -> obmc-host-start-error@.target
    ```
    - An error must be logged to phosphor-logging when any OpenBMC \*-error
      target is run
    - obmc-error-handle@%.target must be called with the phosphor-logging error
- Must be possible to put company specific applications in any arbitrary error
  target
- Applications that require doing their own error paths which cause OpenBMC
  state changes must adhere the following rules:
    - Must log an error to phosphor-logging indicating the reason for the error
    - Must utilize and call a templated target, obmc-error-handle@%.target
       - The templated argument will be the descriptive string used to create
         the log in phosphor-logging
    - If the host is running, the target must eventually enter host quiesce
      state to ensure system recovery attributes are processed
    - The target must conflict with the appropriate targets and services to
      ensure they are stopped
- If an application can provide better isolation of an error then the more
  general one provided by the \*-error.targets then they must utilize the
  obmc-error-handle@%.target to create and report the more detailed error


## Proposed Design

### obmc-chassis-poweron@.target Failure

A power on failure does not follow the normal recovery process's. If a fail
occurs then a phosphor-logging event is created and the system is powered off.
The system is powered off by calling `obmc-chassis-poweroff@.target` in the
`obmc-chassis-poweron-error@.target`.


### obmc-host-start@.target and obmc-host-startmin@.target Failure

`obmc-host-start@.target` does some basic initial boot services and then calls
into `obmc-host-startmin@.target` which does most of the work on starting the
host. Both of these targets will call a common `obmc-host-start-error@.target`

This target must call `obmc-host-quiesce@.target` so appropriate recovery
actions can be executed.

### obmc-host-stop@.target Failure

TBD: Log error, call obmc-chassis-poweroff

### obmc-chassis-poweroff@.target Failure

TBD: Log error, leave system in weird state or do something drastic?


## Alternatives Considered

A nice feature would be to have the \*-error targets query systemd to determine
which service caused the failure and put that in the phosphor-logging event.
This will be considered as a future feature.

Getting a generic "failed to power on" error is not going to make people happy
so in general, when an error can be isolated, it should be created and used
for the failure. Is there a better way to do this other then requiring each
error have its own obmc-error-handle@%.target then we should discuss.

It would be simpler to just say users must always utilize the default
\*-error.target by exiting non-zero back to the target but we don't get
enough error detail that way unless we can log a generic error and tell
the user to look for more detailed error logs.

## Impacts

Error recovery paths

## Testing

All OpenBMC defined systemd \*-error@.target must be tested.

[1]: https://github.com/openbmc/docs/blob/master/openbmc-systemd.md
