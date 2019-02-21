# IBM Enterprise Event Logging on OpenBMC

Author:
  Matt Spinler <mspinler!>

Primary assignee:
  Matt Spinler

Other contributors:
  yes please

Created:
  2/21/2019

## Problem Description
IBM's enterprise systems have specific event logging and reporting requirements
that need to be implemented by the OpenBMC code that run on them.  These
systems use a proprietary event log format throughout their firmware and
operating system stacks for logging errors and events. It is called a "Platform
Event Log", or PEL. In order for the OpenBMC code to fit in with this, it will
need to be able to convert OpenBMC event logs to a PEL for consumption by the
operating systems and existing service structure. It also needs to be able to
store PELs sent down from the host (hostboot, PowerVM, OPAL, etc).

In addition to PELs, the BMC will need to be able to export its event logs in
Redfish (in an OEM format) and IPMI as a SEL, and this includes the events that
were originally sent to the BMC as a PEL from the host.

IBM expects that a PEL be able to contain enough of a snapshot of the system in
the log for a developer or service person to be able to service the event, and
also aid in the prevention of future occurrences if possible.  For example, the
hardware team may request that a PEL created for a hardware failure contain a
snapshot of chip registers which may aid them in determining if the problem was
caused by a chip bug.

As IBM's OpenBMC stack contains code from other companies, any errors generated
by their code must still be able to be converted to PELs as well.

## Background and References
PELs have been tuned by IBM for over more than ten years to aid in failure
resolution on IBM's enterprise systems, and extensive frameworks have been put
in place across the company to deal with them. Adding PEL support to OpenBMC
allows OpenBMC systems to integrate with this framework.

To date, the PEL specification has not been externally published, though the
open source hostboot code (https://github.com/open-power/hostboot), does
create them and there is a parser at https://github.com/open-power/errl. A
specification can hopefully be published as part of this work.

A PEL entry consists of sections, some required and some optional, that can
describe several types of events - error, customer attention, user action, OS
notification, and informational. For example, there is a FRU call-out section
that contains which pieces of hardware need to be replaced to resolve the
error. Further details on the contents of a PEL are beyond the scope of this
document and will be left to the specification.

A PEL is designed to be stored and transferred in its flattened state - i.e
as a raw array of bytes, and then can be parsed when necessary by finding
sections via eyecatchers, offsets, and size fields. The maximum size of PEL
is 16KB though some subsystems may support less.

As Redfish is becoming more common, generating Redfish logs from OpenBMC
logs as well as host created PELs is also important.

## Requirements

* Event logs on the BMC must be persisted across loss of AC and reboots.

* Performance must be a factor considered when designing the storage and
  management of logs.  It is likely that D-Bus is too slow to hold everything.

* OpenBMC event logs must be converted into PEL logs, either on demand or
  ahead of time.
  * PELs will be sent to the host via PLDM.
  * PELs will be sent off-BMC via Redfish OEM commands in their native
    format.

* OpenBMC event logs, including those created from host PELs, must be converted
  into Redfish OEM logs.

* PELs have fields with predefined set of values that must be filled in when
  a BMC event is converted to a PEL or returned in a Redfish log.
  Examples are:
  * Severity value
  * If serviceable
  * If callhome
  * The affected subsystem
  * A unique reference code (SRC in PEL terminology)

  This implies that either the error log creator must fill these in, or they
  get filled in at the time of conversion or creation from a metadata table off
  to the side.  The metadata method is the tenable solution to allow non IBM
  written code to be converted.

* PELs require data that needs to be snapshotted at event creation time so
  it can be included in the PEL on conversion.
  * Code level
  * Machine type/model.
  * For FRU callouts, the PN/SN of the FRU
  * etc

* Events must be able to have multiple callouts, where a callout can be a
  hardware entity, a service procedure number, or a symbolic hardware entity
  that points to the cause of a problem.  Callouts also have an associated
  priority.  The PEL has specific sections and formats in which to store
  callouts, as does the Redfish OEM log.
  * The number and type of callouts may not be known until the code runs.
  * If a callout is for hardware, the serial and part number of the FRU must be
    added to the callout at the time of creation by the error logging code.

* The event creator and also handlers of the event up the call stack  must be
  able to add any additional debug data they need in order to debug a problem
  to an event for capture in the PEL.  In the PEL, these will be added to
  sections designed for contained custom user data.
  * The creator of this user data must also provide a parser to convert the
    user data into a human readable format if applicable.
  * This data must be saved in the file system, not on D-Bus due to size
    constraints.

* A TBD portion of the journal must be captured upon event creation for
  addition to the PEL upon conversion.  TBD if this has to be done for
  every event, or just a certain class.

* When an event is reported off BMC - both to the HMC (maybe in a Redfish log
  format), or to the host (as a PEL), the receiving entity will acknowledge
  that it received the log, and the BMC needs to persist that information so it
  doesn't resend the same log again, and to aid in its retention algorithm.

* IBM has specific requirements on the event log retention when it comes to
  deleting logs to free up space for new ones:
  * If a log resulted in a GARD record,the log must not be pruned.
  * If a log hasn't been reported to PowerVM and/or(?) the HMC, it cannot
    be pruned.
  * ...Other rules on severity/age and if full to prune old or new logs.
  * Event logs do not 'expire' otherwise.

* PELs that get sent to the BMC must be saved on the BMC.
  * There must be an OpenBMC event log associated with the PEL, which means
    certain fields must be pulled from the PEL for use in the log:
    * Callouts, Severity, call-home, serviceable, etc.

  * The incoming PEL needs to be modified when it is received: (Is this right?)
    * The MTM needs to be added to it.
    * The callouts may need to be modified.

  * As PELs can be up to 16KB, they must not be stored on D-Bus.

* OpenBMC event logs stemming from a host PEL must be returned alongside event
  logs created on the BMC, and PELs obtained from BMC created event logs must
  be returned alongside host PELs in requests for PELs.

* Power Line Disturbance checks must be done when when a PEL is received
  from the host, or when an event log is created on the BMC that stems from
  a hardware issue.
  * If there was a PLD, the previous log should be made informational and
    its callouts should be removed.
  * The PLD event log(s) should be committed.

* If an event log has a hardware callout, the errors need to be periodically
  re-sent to the HMC as a reminder that service is still required.  This is
  commonly referred to as the nag function.

* The Redfish log entry for an event must contain:
  * A text description of the event.
  * The severity
  * The SRC that would be in the PEL
  * The FRU callouts
  * The subsystem
  * If call home
  * If serviceable
  * If resolved
  * The event type
  * A pointer to the Redfish path to the PEL for this event(?)

* Event logs, including logs created from PELs, must converted into IPMI
  SEL entries upon request.

* The web UI is required to show event logs that aren't in PEL format.  Its
  interface to the BMC is Redfish.  This implies it will display data
  from the Redfish OEM log calls.

* If a D-Bus method call fails, it must be able to return an event log that
  the caller may either change or add to, and then commit it or abort it.

Nice to haves:
  * It is desired for user data to be returned in the Redfish log format
    in a parsed format so IBM's event analysis repositories don't have to
    run parsers for each log.

  * Redfish supports a message registry, which is a JSON file on the BMC that
    contains additional data about events.  If this could add value to what is
    already returned in the OEM logs, it may be good to support.

## Proposed Design
TBD

## Alternatives Considered
TBD

## Impacts
TBD

## Testing
TBD
