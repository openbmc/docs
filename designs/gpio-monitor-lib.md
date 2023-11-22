# GPIO monitoring library

Author: Kyle Nieman, @Kyle_N

Other contributors:

Created: Nov 22, 2023

## Problem Description

GPIO monitoring is handled in phosphor-gpio-monitor. However, GPIO monitoring
isn't unique to phosphor-gpio-monitor and is applicable elsewhere. This
leads to duplicate code which could be reduced.

## Background and References

Linux has [libgpiod][1] which is used to interact with the kernel GPIO
interface. This library provides the means to implement two types of
monitoring:
* Polling
* Event-driven (which is more efficient)

The universal steps of a GPIO monitoring application is as follows:
1. Configure the GPIOs
2. Take action after initial set up
3. Start monitoring
4. Detect GPIO state change occurs
5. Take action on GPIO state change/event
(EX. create log, start services, set properties, stop/continue monitoring)

Of these, only steps 2 and 5 are unique to individual applications.

[Phosphor-gpio-monitor][2] package has several services that are used to take
an action on GPIO events. The phosphor-gpio-monitor service will start a
systemd service on the configured GPIO events. The phosphor-gpio-presence
service will set a present property on configured GPIO events. The package has
several implementations that slightly deviate from each other with the steps
seen above. Phosphor-gpio and phosphor-multi-gpio services differ in the
amount of GPIOs are monitored and the format of setting the configuration. The
difference in the phosphor-gpio-monitor and phosphor-gpio-presence services
are the actions taken on state change and initial action taken.

## Requirements

* Provide an easy to use, efficient means of GPIO monitoring
* Ensure that implementation is flexible enough for most usages

The following are some packages that monitor GPIOs:
* [phosphor-gpio-monitor][2]
* [phosphor-power][3]
* [skeleton][4]

## Proposed Design

Phosphor-multi-gpio-monitor/presence will be refactored so that the same
library is used for all its implementations. This event-driven monitoring
library will be exposed so that other packages can use it.

The library will provide general implementations for the following:
1. Setting up the GPIOs based on the configuration
2. Monitoring GPIO events
3. Handling GPIO events

The library will also provide a means to configure the following:
1. Action taken after setting up the GPIOs (initial state)
2. Action taken on a GPIO event

## Alternatives Considered

None

## Impacts

* Improves performance for previous GPIO polling implementations
* Reduces the repeated code and improves maintainability
* Adds an additional API that would have to be learned by developers

### Organizational

* This will not require a new repo
* No need for initial maintainers
* Phosphor-gpio-monitor is required to be modified. Phosphor-power, and
  skeleton could be modified with these changes (not within this scope of the
  proposal).

## Testing

Ensure phosphor-multi-gpio-monitor and phosphor-multi-gpio-presence services
successfully monitor GPIOs.

[1]: https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/tree/README
[2]: https://github.com/openbmc/phosphor-gpio-monitor
[3]: https://github.com/openbmc/phosphor-power/tree/master
[4]: https://github.com/openbmc/skeleton
