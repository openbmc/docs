# IPMI and Redfish Chassis Control

Author:
  Jason Bills, jmbills

Primary assignee:
  Jason Bills

Other contributors:
  None

Created:
  June 6, 2019

## Problem Description
Redfish and IPMI specify chassis control and reset types that are not
supported with the existing State interfaces.

## Background and References
### Redfish
The Redfish Resource and Schema Guide
(https://www.dmtf.org/sites/default/files/standards/documents/DSP2046_2019.1.pdf)
defines a Reset action with a ResetType parameter as follows:

| string | Description |
| --- | --- |
| ForceOff | Turn the unit off immediately (non-graceful shutdown). |
| ForceOn | Turn the unit on immediately. |
| ForceRestart | Perform an immediate (non-graceful) shutdown, followedby a restart. |
| GracefulRestart | Perform a graceful shutdown followed by a restart of the system. |
| GracefulShutdown | Perform a graceful shutdown and power off. |
| Nmi | Generate a Diagnostic Interrupt (usually an NMI on x86 systems) to cease normal operations, perform diagnostic actions and typically halt the system. |
| On | Turn the unit on. |
| PowerCycle | Perform a power cycle of the unit. |
| PushPowerButton | Simulate the pressing of the physical power button on this unit. |

### IPMI

The IPMI specification defines a Chassis Control Command with a chassis
control parameter as follows:

| option | Description |
| --- | --- |
| power down | Force system into soft off (S4/S45) state. This is for ‘emergency’ management power down actions. The command does not initiate a clean shut-down of the operating system prior to powering down the system. |
| power up |  |
| power cycle | This command provides a power off interval of at least 1 second following the deassertion of the system’s POWERGOOD status from the main power subsystem. |
| hard reset | In some implementations, the BMC may not know whether a reset will cause any particular effect and will pulse the system reset signal regardless of power state. |
| pulse Diagnostic Interrupt | Pulse a version of a diagnostic interrupt that goes directly to the processor(s). This is typically used to cause the operating system to do a diagnostic dump (OS dependent). |
| Initiate a soft-shutdown of OS |  |

## Requirements

The respective Redfish and IPMI chassis control and reset commands should
each respond appropriately and perform the defined action.

The users of the solution will be system users and administrators who will
use these commands for system management.

## Proposed Design

We already have two interfaces that are used for many of the defined actions.

xyz.openbmc_project.State.Host: Used to initiate host state transitions (soft)

Currently supports `Off`, `On`, and `Reboot`.

xyz.openbmc_project.State.Chassis: Used it initiate chassis power state
transitions (hard)

Currently supports `Off` and `On`.

These existing transition options do not cover all the IPMI and Redfish
command options.  The proposal is to add three new transition options:

`Interrupt` under Host to handle software interrupts. This will pulse the
interrupt signal to the host.

`PowerCycle` or `Cycle` under Chassis to support hard power cycling. This
will cycle power by forcing off, waiting the specified time after off, and
powering back on.

`Reset` under Chassis to support hard resets. This will pulse the reset
signal on the chassis.

This will allow supporting the commands as follows:

| Redfish | IPMI | Transition |
| --- | --- | --- |
| ForceOff | power down | Chassis.Off |
| ForceOn | power up | Host.On or Chassis.On |
| ForceRestart | hard reset | Chassis.Reset |
| GracefulRestart |  | Host.Reboot |
| GracefulShutdown | soft off | Host.Off |
| Nmi | pulse interrupt | Host.Interrupt |
| On | power up | Host.On or Chassis.On |
| PowerCycle | power cycle | Chassis.PowerCycle |
| PushPowerButton |  | Plan to use Buttons interface |

## Alternatives Considered

An alternative for power cycling is to handle it in the calling code by
requesting off, waiting for off, starting a timer, then requesting on. This
moves these cycling requirements out to each of the callers which increases
complexity and creates additional points of failure.

An alternative for hard reset and interrupt is to not offer those options.

## Impacts

Possible developer impact to make sure new transition options are (accepted
or rejected) appropriately.

## Testing
Testing will be performed by issuing all commands through Redfish and IPMI
and checking for the correct system response.
