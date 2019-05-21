DBUS interface for NMI on POWER Systems

Author: Lakshminarayana Kammath

Primary assignee: Lakshminarayana Kammath

Other contributors: Jayanth Othayoth

Created: 2019-05-21


## Problem Description
In the event of a OPAL/Host OS unresponsive/hung due to CPU lock ups,
POWER Systems needs ability to diagnose the root cause of hang and perform
processor reset action needed to avoid repeated system fails.


## Background and References
There is a situation at customer places/lab where OPAL/Linux goes unresponsive
causing system hang(https://github.com/ibm-openbmc/dev/issues/457).
This means there is no way to figure out what went wrong with Linux/OPAL in
hung state. One has to recover the system with no relevent debug data captured.

The aim of this proposal is to trigger a dump on Linux host through a NMI.
Whenever host is unresponsive, end user has to trigger a NMI event via dbus
which, inturn triggers a SRESET on all the available processors on the system.


## Proposed Design for POWER systems
Get all Host CPUs in reset vector and Linux then, has a mechanism to patch it
into panic-kdump path to trigger dump. This will enable Linux team to analyze
and fix any issues where they see OPAL/Linux hang and unresponsive system.

* ### d-bus
Introducing new d-bus interface in the control.host namespace
(/openbmc/phosphor-dbus-interfaces/xyz/openbmc_project/Control/Host/NMI.interface.yaml)
and implement the new d-bus back-end for respective  processor specific targets.

* ### BMC Support
Enable SBE chip-op based SRESET d-bus interface and enable this via Redfish

* ### Redfish Schema used
* Reference: DSP2046 2018.3,
* ComputerSystem 1.6.0 schema provides an action called #ComputerSystem.Reset,
  This action is used to reset the system. ResetType parameter is used for
  indicating type of reset need to be performed. In this use case we can use Nmi type
    * Nmi: Generate a Diagnostic Interrupt (usually a NMI on x86 systems)
     to cease normal operations, perform diagnostic actions and typically
     halt the system.


## Alternatives Considered
Extending  the existing  d-bus interface state.Host namespace
(/openbmc/phosphor-dbus-interfaces/xyz/openbmc_project/State/Host.interface.yaml)
to support new RequestedHostTransition property called Nmi.
d-bus backend can internally invoke processor specific target to do Sreset
(equivalent to x86 NMI) and do associated actions.

## Impacts:
NA

## Testing
For system level functional validation, enable tests in system level functional
verification CI.
