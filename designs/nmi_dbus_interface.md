DBUS interface for NMI on POWER Systems

Author: Lakshminarayana Kammath

Primary assignee: Lakshminarayana Kammath

Other contributors: Jayanth Othayoth

Created: 2019-05-21


## Problem Description
In the event of a OPAL/Host OS unresponsive/hung due to CPU lock ups,
POWER Systems needs ability to diagnose the root cause of hang and perform
NMI SRESET causes the processor reset, and the reset takes the dump as part
of its recovery.


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

## High Level Flow
1. OPAL/Host OS is hung or unresponsive due to cpu hard lockups.
2. End user or Admin can use the Redfish URI ComputerSystem.Reset
   /redfish/v1/Systems/1/Actions/ComputerSystem.Reset, "ResetType": "Nmi"
   to trigger NMI reset
3. Redfish layer will invoke a dbus NMI call which will use a SBE SRESET
   chip op to trigger a reset on all the processors on the system
4. SRESET chip op will be invoked using openbmc/openpower-sbe-interface
   namespace openpower::sbe::threadcontrol
5. SRESET should trigger a NMI signal on host OS to invoke dump and recover
   the system from unresponsive state

## Alternatives Considered
Extending  the existing  d-bus interface state.Host namespace
(/openbmc/phosphor-dbus-interfaces/xyz/openbmc_project/State/Host.interface.yaml)
to support new RequestedHostTransition property called Nmi.
d-bus backend can internally invoke processor specific target to do Sreset
(equivalent to x86 NMI) and do associated actions.

## Impacts:
This implementation only needs to make some changes to the system state
when OPAL/Host is unresponsive, so it has minimal impact on rest of the system.

## Testing
Depending on the platform hardware design, this test requires an host OS kernel
module driver to create hard lockup and then check the scenario is good.
