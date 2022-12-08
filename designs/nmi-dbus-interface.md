# Design proposal for issuing NMI on servers that use OpenBMC

Author: Lakshminarayana Kammath

Other contributors: Jayanth Othayoth

Created: 2019-05-21

## Problem Description

Currently, servers that use OpenBMC cannot have the ability to capture relevant
debug data when the host is unresponsive or hung. These systems need the ability
to diagnose the root cause of hang and perform recovery along with debugging
data collected.

## Background and References

There is a situation at customer places/lab where the host goes unresponsive
causing system hang(https://github.com/ibm-openbmc/dev/issues/457). This means
there is no way to figure out what went wrong with the host in a hung state. One
has to recover the system with no relevant debug data captured.

Whenever the host is unresponsive/running, Admin needs to trigger an NMI event
which, in turn, triggers an architecture-dependent procedure that fires an
interrupt on all the available processors on the system.

## Proposed Design for servers that use OpenBMC

This proposal aims to trigger NMI, which in turn will invoke an
architecture-specific procedure that enables data collection followed by
recovery of the Host. This will enable Host/OS development teams to analyze and
fix any issues where they see host hang and unresponsive system.

### D-Bus

Introducing new D-Bus interface in the control.host namespace
(/openbmc/phosphor-dbus-interfaces/xyz/openbmc_project/Control/Host/
NMI.interface.yaml) and implement the new D-Bus back-end for respective
processor specific targets.

### BMC Support

Enable NMI D-Bus phosphor interface and support this via Redfish

### Redfish Schema used

- Reference: DSP2046 2018.3,
- ComputerSystem 1.6.0 schema provides an action called #ComputerSystem.Reset,
  This action is used to reset the system. The ResetType parameter is used for
  indicating the type of reset needs to be performed. In this case, we can use
  An NMI type
  - Nmi: Generate a Diagnostic Interrupt (usually an NMI on x86 systems) to
    cease normal operations, perform diagnostic actions and typically halt the
    system.

## High-Level Flow

1. Host/OS is hung or unresponsive or one need to take kernel dump to debug some
   error conditions.
2. Admin/User can use the Redfish URI ComputerSystem.Reset that allows POST
   operations and change the Action and ResetType properties to
   {"Action":"ComputerSystem.Reset","ResetType":"Nmi"} to trigger NMI.
3. Redfish URI will invoke a D-Bus NMI back-end call which will use an arch
   specific back-end implementation of xyz.openbmc_project.Control.Host.NMI to
   trigger an NMI on all the processors on the system.
4. On receiving the NMI, the host will automatically invoke Architecture
   specific actions. One such action could be; invoking the kdump followed by
   the reboot.

- Note: NMI can be sent to the host in any state, not just at an unresponsive
  state.

## Alternatives Considered

Extending the existing D-Bus interface state.Host namespace
(/openbmc/phosphor-dbus-interfaces/xyz/openbmc_project/State/Host.interface.yaml)
to support new RequestedHostTransition property called Nmi. D-Bus back-end can
internally invoke processor-specific target to invoke NMI and do associated
actions.

There were strong reasons to move away from the above approach.
phosphor-state-manager has always been focused on the states of the BMC,
Chassis, and Host. NMI will be more of action against the host than a state.

## Impacts

This implementation only needs to make some changes to the system state when NMI
is initiated irrespective of what host OS state is in, so it has minimal impact
on the rest of the system.

## Testing

Depending on the platform hardware design, this test requires a host OS kernel
module driver to create hard lockup/hang and then check the scenario is good.
Also, one can invoke NMI to get the crash dump and confirm HOST received NMI via
console logs.
