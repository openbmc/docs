# Fail Boot on Hardware Errors

Author: Andrew Geissler (geissonator)

Other contributors:

Created: Feb 20, 2020 Updated: Apr 12, 2022

## Problem Description

Some groups, for example a manufacturing team, have a requirement for the BMC
firmware to halt a system if an error log is created which calls out a piece of
hardware. The reason behind this is to ensure a system is not shipped to a
customer if it has any type of hardware issue. It also ensures when an error is
found, it is identified quickly and all activity stops until the issue is fixed.
If the system has a hardware issue once shipped from manufacturing, then the BMC
firmware behavior should be to report the error, but allow the system to
continue to boot and operate.

OpenBMC firmware needs a mechanism to support this use case.

## Background and References

Within IBM, this function has been enabled/disabled by what is called
manufacturing flags. They were bits the user could set in registry variables
which the firmware would then query. These registry variables were only settable
by someone with admin authority to the system. These flags were not used outside
of manufacturing and test.

Extensions within phosphor-logging may process logs that do not always come
through the standard phosphor-logging interfaces (for example logs sent down by
the host). In these cases the system must still halt if those logs contain
hardware callouts.

[This][1] email thread was sent on this topic to the list.

## Requirements

- Provide a mechanism to cause the OpenBMC firmware to halt a system if a
  phosphor-logging log is created with a inventory callout
  - The mechanism to enable/disable this feature does not need to be an external
    API (i.e. Redfish). It can simply be a busctl command one runs in an ssh to
    the BMC
  - The halt must be obvious to the user when it occurs
    - The log which causes the halt must be identifiable
  - The halt must only stop the chassis/host instance that encountered the error
  - The halt must allow the host firmware the opportunity to gracefully shut
    itself down
  - The halt must stop the host (run obmc-host-stop@X.target) associated with
    the error and attempt to leave system in the fail state (i.e. chassis power
    remains on if it is on)
  - The chassis/host instance pair will not be allowed to power on until the log
    that caused the halt is resolved or deleted
    - A BMC reset will clear this power on prevention
- Ensure the mechanism used to halt firmware on inventory callouts can also be
  utilized by phosphor-logging extensions to halt firmware for other causes
  - These causes will be defined within the extensions documentation
- Quiesce the associated host during this failure

**Special Note:** Initially the associated host and chassis will be hard coded
to chassis0 and host0. More work throughout the BMC stack is required to handle
multiple chassis and hosts. This design allows that type of feature to be
enabled at a later time.

## Proposed Design

Create a [phosphor-settingsd][2] setting,
`xyz.openbmc_project.Logging.Settings`. Within this create a boolean property
called QuiesceOnHwError. This property will be hosted under the
xyz.openbmc_project.Settings service.

Define a new D-Bus interface which will indicate an error has been created which
will prevent the boot of a chassis/host instance:
`xyz.openbmc_project.Logging.ErrorBlocksTransition`

This interface will be hosted under a instance based D-Bus object
`/xyz/openbmc_project/logging/blockX` where X is the instance of the
chassis/host pair being blocked.

When an error is created via a phosphor-logging interface, the software will
check to see if the error has a callout, and if so it will check the new
`xyz.openbmc_project.Logging.Settings.QuiesceOnHwError`. If this is true then
phosphor-logging will create a `/xyz/openbmc_project/logging/blockX` D-Bus
object with a `xyz.openbmc_project.Logging.ErrorBlocksTransition` interface
under it. A mapper [association][3] between the log and this new D-Bus object
will be created. The corresponding host instance will be put in quiesce by
phosphor-logging.

The blocked state can be exited by rebooting the BMC or clearing the log
responsible for the blocking. Other system specific policies could be placed in
the appropriate targets (for example if a chassis power off should clear the
block)

See the phosphor-logging [callout][4] design for more information on callouts.

A new `obmc-host-graceful-quiesce@.target` systemd target will be started. This
new target will ensure a graceful shutdown of the host is initated and then
start the `obmc-host-quiesce@.target` which will stop the host and move the host
state to Quiesced.

obmcutil will be enhanced to look for these block interfaces and notify the user
via the `obmcutil state` command if a block is enabled and what log is
associated with it.

The goal is to build upon this concept when future design work is done to allow
developers to associate certain error logs with causing a halt to the system
until a log is handled.

## Host Errors

In certain scenarios, it is desirable to also halt the boot, and prevent it from
rebooting, when the host sends down certain errors to the BMC.

These errors may be of SEL format, or may be OEM specific, such as the [PEL
format][5] used by IBM.

The interfaces provided within phosphor-logging to handle the hardware callout
scenarios can be repurposed for this use case.

## Alternatives Considered

Currently this feature is a part of the base phosphor-logging design. If no one
other then IBM sees value, we could roll this into the PEL-specific portion of
phosphor-logging.

## Impacts

This will require some additional checking on reported logs but should have
minimal overhead.

There will be no changes to system behavior unless a user turns on this new
setting.

## Testing

Unit tests will be run to ensure logic to detect errors with logs and verify
both possible values of the new setting.

Test cases will need to look for this new blocking D-Bus object and handle
appropriately.

[1]: https://lists.ozlabs.org/pipermail/openbmc/2020-February/020575.html
[2]: https://github.com/openbmc/phosphor-settingsd
[3]:
  https://github.com/openbmc/docs/blob/master/architecture/object-mapper.md#associations
[4]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Common/Callout/README.md
[5]:
  https://github.com/openbmc/phosphor-logging/blob/master/extensions/openpower-pels/README.md
