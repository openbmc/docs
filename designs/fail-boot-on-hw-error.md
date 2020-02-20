# Fail Boot on Hardware Errors

Author: Andrew Geissler (geissonator)

Primary assignee: Andrew Geissler (geissonator)

Other contributors:

Created: Feb 20, 2020

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
which the firmware would then query. These registry variables were only
settable by someone with admin authority to the system. These flags were not
used outside of manufacturing and test.

Extensions within phosphor-logging may process logs that do not always come
through the standard phosphor-logging interfaces (for example logs sent
down by the host). In these cases the system must still halt if those logs
contain hardware callouts.

[This][1] email thread was sent on this topic to the list.

## Requirements
- Provide a mechanism to cause the OpenBMC firmware to halt a system if a
  phosphor-logging log is created with a inventory callout
  - The mechanism to enable/disable this feature does not need to be an
    external API (i.e. Redfish). It can simply be a busctl command one runs
    in an ssh to the BMC
  - The halt must be obvious to the user when it occurs
    - The log which causes the halt must be identifiable
- Provide an interface within phosphor-logging that can be called by extensions
  within phosphor-logging that will cause the halt
  - This interface will require a phosphor-logging ID to associate with the halt
- Create a new systemd target, obmc-bmc-quiesce.target, which will be run to
  put the BMC into the quiesce state when a halt is requested

## Proposed Design
Create a [phosphor-settingsd][2] setting, xyz.openbmc_project.Logging.Settings.
Within this create a boolean property called QuiesceOnHwError. This property
will be hosted under the xyz.openbmc_project.Settings service.

When an error is created via a phosphor-logging interface, the software will
check to see if the error has a callout, and if so it will check the new
xyz.openbmc_project.Logging.Settings.QuiesceOnHwError. If this is true then
phosphor-logging will call systemd to start a new OpenBMC systemd target.

The new systemd target called for this situation will be called
obmc-bmc-quiesce.target. This will move the state of
xyz.openbmc_project.State.BMC.CurrentBMCState to Quiesced. This will be
visible externally by a user by looking at the state of the bmc under the
Managers resource via the Redfish API. This new target will conflict

The BMC Quiesced state can be exited by rebooting the BMC or clearing the log
responsible for the Quiesce state. phosphor-logging will be responsible for
starting and stopping the obmc-bmc-quiesce.target. The BMC state software
will monitor this target and updates its target accordingly:
- Target Start: Quiesced
- Target Stop:  Standby

See the phosphor-logging [callout][3] design for more information on callouts.

The goal is to build upon this new target and Quiesced concept when future
design work is done to allow developers to associate certain error logs with
causing a quiesce state.

## Alternatives Considered
Currently this feature is a part of the base phosphor-logging design. If no
one other then IBM sees value, we could roll this into the PEL-specific
portion of phosphor-logging.

## Impacts
This will require some additional checking on reported logs but should have
minimal overhead.

There will be no changes to system behavior unless a user turns on this new
setting.

## Testing
Unit tests will be run to ensure logic to detect errors with logs and verify
both possible values of the new setting. Will also ensure BMC state manager
software correctly handles the new target entry/exit.

Test cases will need to look for this new Quiesced state of the BMC when
using this new setting.

[1]: https://lists.ozlabs.org/pipermail/openbmc/2020-February/020575.html
[2]: https://github.com/openbmc/phosphor-settingsd
[3]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Common/Callout/README.md
