Design proposal for issuing NMI on POWER Systems

Author: Lakshminarayana Kammath

Primary assignee: Lakshminarayana Kammath

Other contributors: Jayanth Othayoth

Created: 2019-05-21


## Problem Description
Currently, POWER Systems do not have the ability to capture relevant debug data,
when the OPAL/HOST is unresponsive or hung. POWER Systems needs ability to
diagnose the root cause of hang and perform recovery along with debug data
collected.


## Background and References
There is a situation at customer places/lab where OPAL/Linux goes unresponsive
causing system hang(https://github.com/ibm-openbmc/dev/issues/457).
This means there is no way to figure out what went wrong with Linux/OPAL in
hung state. One has to recover the system with no relevent debug data captured.

Whenever host is unresponsive or even if everything is up and running, Admin
has to trigger a NMI event which, in turn triggers a SBE SRESET chipop
on all the available processors on the system.


## Proposed Design for POWER systems
The aim of this proposal is to trigger a dump on host Linux through a NMI.
Get all Host CPUs in reset vector and Linux then, has a mechanism to patch it
into panic-kdump path to trigger dump. This will enable Linux team to analyze
and fix any issues where they see OPAL/Linux hang and unresponsive system.

* ### D-Bus
Introducing new D-Bus interface in the control.host namespace
(/openbmc/phosphor-dbus-interfaces/xyz/openbmc_project/Control/Host/
NMI.interface.yaml)
and implement the new D-Bus back-end for respective processor specific targets.

* ### BMC Support
Enable pdbg based SRESET D-Bus interface and enable this via Redfish

* ### Redfish Schema used
* Reference: DSP2046 2018.3,
* ComputerSystem 1.6.0 schema provides an action called #ComputerSystem.Reset,
  This action is used to reset the system. ResetType parameter is used for
  indicating type of reset need to be performed. In this use case we can use
  Nmi type
    * Nmi: Generate a Diagnostic Interrupt (usually a NMI on x86 systems)
     to cease normal operations, perform diagnostic actions and typically
     halt the system.

## High Level Flow
1. OPAL/Host OS is hung or unresponsive or one need to take kernel dump
   to debug some error condition in host.
2. Admin can use the Redfish URI ComputerSystem.Reset
   /redfish/v1/Systems/1/Actions/ComputerSystem.Reset, "ResetType": "Nmi"
   to trigger NMI reset
3. Redfish URI will invoke a D-Bus NMI backend call which will use pdbg/libpdbg
   (https://github.com/open-power/pdbg)
   to trigger a stop instruction followed by reset on all the processors on the
   system (pdbg -a stop; pdbg -a sreset)
4. SRESET should trigger a NMI signal to OPAL/host OS to invoke dump, followed
   by reboot and recover the system from unresponsive state.

* Note: Steps #2 to #4 should also work in normal case as well. i.e. even if
host is not hung

## Alternatives Considered
Extending  the existing  D-Bus interface state.Host namespace
(/openbmc/phosphor-dbus-interfaces/xyz/openbmc_project/State/Host.interface.yaml)
to support new RequestedHostTransition property called Nmi.
D-Bus backend can internally invoke processor specific target to do Sreset
(equivalent to x86 NMI) and do associated actions.

## Impacts:
This implementation only needs to make some changes to the system state
when OPAL/Host is unresponsive, so it has minimal impact on rest of the system.

## Testing
Depending on the platform hardware design, this test requires an host OS kernel
module driver to create hard lockup/hang and then check the scenario is good.
Also, one can invoke NMI to get the crash dump and confirm HOST recieved NMI
via console logs.
