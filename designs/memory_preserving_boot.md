# Memory Preserving Boot sequence - BMC support.

Author: Jayanth Othayoth <ojayanth@gmail.com> <ojayanth_>

Primary assignee:
  Jayanth Othayoth

Other contributors:
  yes please

Created:
  02/28/2019

## Problem Description
POWER Server supports a special feature called firmware-assisted dump to collect
Host specific debug data for any system failure due to host issues. To enable
this feature a special boot sequence called, memory preserving initial program
load (MPIPL) should support by BMC to capture OPAL and host kernel core after
system failure.  This is valid only when system is running (at least OPAL
is loaded). Either user can induce a memory preserving IPL or one will occur
with certain error scenarios. In this boot sequence BMC should help to trigger
the special boot seuence based on the host request and inform the impacted
BMC/Host software about this.

## Background and References
The goal of firmware-assisted dump is to enable the dump of a crashed system,
and to do so from a fully-reset system, and to minimize the total elapsed time
until the system is back in production use. As compared to kdump or other
strategies, firmware-assisted dump offers several strong, practical advantages.

Reference Links:
https://www.ibm.com/support/knowledgecenter/en/ssw_aix_71/com.ibm.aix.kernextc/sysdumpfac.htm
https://lists.ozlabs.org/pipermail/linuxppc-dev/2008-February/051556.html

## Requirements
* Provide a mechanism to watch Interrupts from the Host related to MPIPL request.
* Reset VPNOR and States.
    * Provide a support to reset VPNOR.
* Halt OCC Communication.
* Indicate User that system is not running and MPIPL is in progress.
    * Need to introduce new state called MPIPL and provide mechanism to reset
      the state after IPL completion.
* Watchdog timer management.
    * Need to start watchdog timer, same as in the normal boot.
* Failure Handling
    * Any failure should trigger a normal host reboot.

## Proposed Design
Introduce a new infrastructure to watch host special hardware based requests or 
IPMI requests to trigger MPLIPL sequence from BMC. BMC State manager should
introduce new state called MPIPL, which will help the application to handle
the special function need to enable boot support. which includes,
* Halting POWRER Processor Onchip Chip controller Traffics
* Resetting Host VPNOR.
* Enable watchdog timer
* Handle Failures in special way.

## Alternatives Considered

## Impacts
Design and development needs to involve potential host firmware implementations.

## Testing
For system level functional validation, enable tests in system level functional
verification CI.
