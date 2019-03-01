# System Fatal Error Handling for POWER Systems

Author: Zane Shelley

Primary assignee: Brian Stegmiller

Other contributors: None

Created: 2019-03-01

## Problem Description
In the event of a system fatal error reported by the internal system hardware
(processor chips, memory chips, I/O chips, system, etc.), POWER Systems need
the ability to diagnose the root cause of the failure and perform any service
action needed to avoid repeated system failures.

**NOTE:** The scope of this proposal is specifically for:
1. Detecting the system fatal error event.
2. Initiating diagnostics.
3. Requesting a system reboot, if necessary.
This proposal does **not** describe how diagnostics of the error is done.

## Background and References
Currently on OpenPOWER systems with attached BMCs (all BMCs, not just OpenBMC),
there is a requirement that the BMC must:
1. Listen for a system fatal error event (also known as a system checkstop).
2. Once an event occurs, the BMC must wait a determined amount of time to allow
   a micro-controller on the host processor, called the OCC, to gather
   information regarding the error.
3. After the timer expires, the BMC will reboot the system.

Links regarding the OpenBMC implementation described above:
- https://github.com/openbmc/phosphor-gpio-monitor
- https://github.com/openbmc/openbmc/blob/master/meta-openpower/recipes-phosphor/host/checkstop-monitor.bb

Among other issues, this is an inefficient design because there is no handshake
between the OCC and the BMC. So, it is possible that the BMC may reboot the
machine before all necessary data is collected, but the more likely issue is
that the BMC will wait much longer than required before rebooting.

Part of this proposal is to move the error data collection and analysis of the
data from the host firmware to OpenBMC. High level details regarding how the
data will be collected and analyzed is documented in the following proposal:
- https://gerrit.openbmc-project.xyz/#/c/openbmc/docs/+/18591/

## Requirements
TBD

## Proposed Design
Referencing the current design in the background section, the proposal is to
initiate diagnostics from the BMC instead of using an arbitrary wait.

TBD design considerations:
- Analysis of the fatal error requires support from both the BMC and the host
  firmware. If that support does not exist, we will have to default back to the
  current design.

## Alternatives Considered
TBD

## Impacts
TBD

## Testing
TBD

