# Extended Host Software State

Author:
  Jason Bills, jmbills

Primary assignee:
  Jason Bills

Other contributors:
  None

Created:
  June 10, 2019

## Problem Description

The current Host States cover the basic Off and Running states; however,
there are cases where some additional information either between those
states or beyond them into operating system state would be useful.

For example, the Redfish PowerState definition includes a "PoweringOn" and
"PoweringOff" state that would provide useful information to a user.

## Background and References

The Host State enum has the following states:

- Off
- Running
- Quiesced

https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/State/Host.interface.yaml

Redfish ComputerSystem has a PowerState definition with the following states:

- On
- Off
- PoweringOn
- PoweringOff

https://redfish.dmtf.org/schemas/ComputerSystem_v1.xml

## Requirements

We're looking for information on operating system state. The current usage of
Host State treats Running as the final state such that if the system is not
"Running", it is "Off". This doesn't allow for information on whether the OS
has loaded or if the system has completely booted.

Additional boot state information would be useful to a system administrator or
user for informational purposes, but it could also be used by BMC subsystems,
such as sensors, that may execute differently in a pre-OS environment.

## Proposed Design

The proposal is to add new states to Host State enum to provide more granular
info on what state the host is in.

The Host State enum could be extended to:

- Off
- Started
- Running
- Quiesced

This would provide additional detail on when the host has started up, but has
not reached a full Running state.  For example, when power is applied and
host firmware starts executing, it can be in "Started", and when host
firmware has completed and either the system is fully booted or has started
the operating system, it can move to "Running".

It may also be useful to have a "Stopping" or "PoweringOff" state to cover
cases where powering-off transition information is needed.

## Alternatives Considered

An alternative would be to leave the Host State as it is and add a new OS
State interface. The OS State can then show the state of the operating system
independent of the Host State.

## Impacts
All current usages of Host State would need to comprehend the extended states.
There are fewer than 20 results for "HostState.Running" in GitHub.

## Testing
The extended states would need to be checked for correct behavior everywhere
Host State is currently used.
