# Power Managment / OCC

Author:
  Chris Cain (cjcain)

Other contributors:
  Martha Broyles (None)

Created:
  September 6, 2022

## Problem Description
On systems with multiple BMCs, there is a set of power management configuration
data that will apply to the composed system as a whole. This data includes:
- Power Mode
- Idle Power Saver Parameters
- Power Cap

This data needs to be available to all BMCs in that composed system. When that
data is changed it also needs to be relayed to those BMCs.

## Background and References
With the current design the power management configuration data is maintained by
the single BMC associated with a system. The data is configured via the BMC GUI
and/or Redfish interface for that system.

When a system boots, there is a single Hostboot image that starts and configures
the OCCs for the system. Once all OCCs are active, the BMC is notified via PLDM.
The BMC will then begin communication with all OCCs to collect temperatures and
power data from the OCCs.

[System Power Mode and Idle Power Saver Support design document][powermode]

## Requirements
### SCA - System Coordinator
- The SCA must provide an interface for the end user to set/read power
  management configuration data (Redfish and any other supported interface)
- The SCA must provide a notification message when any changes are made to the
  power management configuration data that will be used by all BMCs in the
  composed system.
### BMC
- Each BMC must be able to read the power management configuration data from an
  SCA.
- Each BMC must monitor for changes to the power management configuration data.
- Each BMC will maintain a copy of the power management configuration data
### Hostboot
- A single Hostboot instance will be running for each composed system.  This
  Hostboot code will be responsible for loading and starting all OCCs for all
  processors in the system.
- After the OCCs are activated, a PLDM message will be relayed to each BMC in
  the system. The message will contain the master OCC instance.
- When there is a failure requiring an OCC/PM Complex reset, all BMCs in the
  composed system must be notified that they should stop communication with
  the OCCs, prior to the reset. Another PLDM notification will be sent when the
  OCCs go active again.
### PLDM (BMC & Hostboot)
- Provide a synchronized communication method between Hostboot and the BMCs
- Provide a way to determine if the other side is busy or if it was stopped
- Provide a queue to allow multiple requests to be outstanding (prevent
  messages and responses from getting overwritten)

## Proposed Design
### Startup
A single SCA will host/provide access to the power management configuration
data. The SCA will be responsible for notifying and/or sending this data to each
BMC in the composed system anytime requested or when any of the data changes.

When the occ-control app in the BMC starts, it will request the power management
configuration data from the SCA. It will continue to save this data locally. The
occ-control app must then monitor the SCA for any changes to that configuration
data. As in the current design, the occ-control app must continue to monitor for
PLDM messages from Hostboot that indicate when the OCCs go active, or when the
OCC communication needs to stop.

### OCC Activation
Hostboot continues to be responsible for loading and starting all OCCs in a
composed system.

When the OCCs reach active state, Hostboot will send a single synchronous PLDM
message containing the master OCC instance to each BMC in the composed system.
This message should be retried once if delivery could not be confirmed.  If the
retry fails, a PEL should be created indicating that fault.

Since Hostboot has already determined which OCCs are functional, occ-control can
use the poll response data from the master to know which other OCCs are
available and then begin polling all OCCs.

### BMC Detected Problem
When BMC detects an issue with an OCC, communication with all OCCs will be
stopped and a PEL will be created indicating the type of failure.  A
synchronized PLDM message will then be sent to Hostboot to request an OCC/PM
Complex reset.  That message will also indicate the OCC triggering by the issue.
The message should be retried once if delivery could not be confirmed.

When Hostboot receives the reset request, it will send a synchronous PLDM
message to all BMCs in the composed system, indicating that communications with
the OCCs needs to be stopped. These messages should be retried once on failure.
After all BMCs were notified, Hostboot will reset the PM Complex. If there are
still recovery attempts available, Hostboot will attempt to load/start the OCCs
again and if successful, will send the synchronous PLDM messages to each BMC
indicating that OCCs are again active.

### OCC/Hostboot Detected Problem
When the OCC or Hostboot detects problems, Hostboot will first notify all BMCs
in the composed system that communications with the OCCs needs to be stopped.
This notification will be a synchronous PLDM message with a disabled status.
These messages should be retried once on failure.  Hostboot will then reset all
PM Complexes in the system.  If the reset/recovery is successful, Hostboot will
again send a synchronous PLDM notification to all BMCs in the composed system.

## Alternatives Considered
- Keeping PLDM interface non-synchronous: The current interface allows other
  applications to overwrite the command buffer and/or re-use MTCP instance IDs
  that are used by other apps. This leads to confusion and unexpected results.
  There is no watchdog or other health monitoring done on either BMC or Hostboot
  to know if the other side has gone away or is just busy doing other work.
  There is no way to queue up multiple requests.
- Eliminate SCA: That would require the power management config data to be
  managed by one of the BMCs in the system. That BMC would then need to relay
  that data to all other BMCs in that system. That BMC would need to know about
  all other BMCs.

## Impacts
- Limiting the PLDM messages to a single message between Hostboot and each BMC
  will help limit the PLDM traffic
- Using the existing OCCs present data from the master poll response, will
  eliminate extra code on BMC to detect which OCCs were present (reduce
  duplicate code)
- Having a single interface (via SCA) for the power management configuration
  will simplify the configuration

### Organizational
- Does this repository require a new repository?
  No
- Which repositories are expected to be modified to execute this design?
  openpower-occ-control, [open-power/hostboot][hostbootrepo]

[hostbootrepo]:
https://github.com/open-power/hostboot
[powermode]:
https://github.com/openbmc/docs/blob/master/designs/ibm/system-power-mode.md

## Testing
Since the OCCs are started on every boot, the main code paths will be verified
every time a system is started. There are commands that can be injected on the
OCCs to force error conditions to verify some of the error paths. Additional
code changes can be done to force some of the more complex error conditions.
