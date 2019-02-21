# IBM Enterprise Event Logging on OpenBMC

Author:
  Matt Spinler <mspinler!>

Primary assignee:
  Matt Spinler

Other contributors:

Created:
  2/21/2019

## Problem Description
This document is the requirements document for IBM enterprise event logging,
which is being done before the design in order to guide the design.

IBM's enterprise systems have specific event logging and reporting requirements
that need to be implemented by the OpenBMC code that runs on them.  At the
highest level this includes being able to:
1. Generate IBM's very detailed Platform Event Log (PEL) format for events
   created by the OpenBMC code.
2. Generate Redfish OEM logs for BMC events with some of the same details that
   would be in a PEL.
3. Generate IPMI SEL events from the BMC events.
4. Process the PELs sent down from the host firmware in order to present them
   alongside BMC generated events.

As OpenBMC's native event logging format isn't PEL or Redfish, conversions need
to be done from the native format.  Because PELs contain additional details
that aren't in the native logs, either the native event format need to be
enhanced, all additional functionality needs to be in IBM specific code, or
some combination of the two may need to be done.

## Background and References
PELs have been tuned by IBM for many years to aid in failure resolution on
IBM's enterprise systems, and extensive frameworks have been put in place
across the company to deal with them. Adding PEL support to OpenBMC allows
OpenBMC systems to integrate with this framework.

To date, the PEL specification has not been externally published, though the
open source hostboot code (https://github.com/open-power/hostboot), does
create them and there is a parser at https://github.com/open-power/errl. A
specification can hopefully be published as part of this work.

A PEL entry consists of sections, some required and some optional, that can
describe several types of events - error, customer attention, user action, OS
notification, and informational. For example, there is a FRU call-out section
that contains which pieces of hardware need to be replaced to resolve the
error.

PELs have the ability to contain FFDC (first failure data capture) data for the
event, which is useful in helping:
1. Determine root cause of the event.
1. Prevent the need for problem recreates in test environments.
2. Allow data warehousing tools to pull in FFDC fields for reports and analysis.

A PEL is designed to be stored and transferred in its flattened state - i.e
as a raw array of bytes, and then can be parsed when necessary by finding
sections via eyecatchers, offsets, and size fields. The maximum size of PEL
is 16KB, though some subsystems may support less.

Further details on the contents of a PEL are beyond the scope of this
document and will be left to the specification.  This includes the definitions
of the various fields, such as 'callhome', 'severity', 'subsystem', etc.

Aside from just reporting events via REST requests from users and the web UI,
the BMC also reports its events in PEL format to the host (via PowerVM), and to
IBM's HMC (hardware management console), possibly in PEL format or in Redfish.
These cases introduce tracking requirements such as that logs shouldn't be sent
to these entities more than once, and logs can't be pruned until they are sent.

## Requirements

* Event logs on the BMC must be persisted across loss of AC and reboots.

* Performance must be a factor considered when designing the storage and
  management of logs.  It is likely that D-Bus is too slow to hold everything.

* OpenBMC event logs must be converted into PEL logs, either on demand or ahead
  of time.
  * PELs will be sent to the host via PLDM.
  * PELs may be sent off-BMC via Redfish OEM commands that contain the PEL as
    ASCII hex data.

* OpenBMC event logs, including those created from host PELs, must be converted
  into Redfish OEM logs.

* OpenBMC event logs must contain still contain the same `Resolved` field that
  exists in the current OpenbMC event logs.
  * The timestamp of when the Resolved field is set to true, either by an
    external REST call or by the code itself, must saved in the log.

* PELs have fields with predefined set of values that must be filled in when
  a BMC event is converted to a PEL or returned in a Redfish log.
  Examples are:
  * Severity value
  * If serviceable
  * If callhome
  * The affected subsystem
  * If hidden from user
  * A unique system reference code (SRC in PEL terminology)
  * Report externally

  This implies that either the error log creator must fill these in, or they
  get filled in at the time of conversion or creation from a metadata table off
  to the side.

  Notes on SRCs which are relavent to a design:
    * An SRC is 8 4B words.
    * In the past, the SRCs were the keys into the documentation:
      * For hardware errors, an SRC must be unique enough to point to the
        hardware system and a small subset of hardware errors.
      * For firmware errors, the SRC must be unique enough to point to the
        firmware subsystem and the component that detected the error.
    * SRCs contain the last known boot progress code.
    * SRCs contain a bit field of system status.

* PELs require data that needs to be snapshotted at event creation time so
  it can be included in the PEL on conversion.
  * Code level
  * Machine type/model.
  * For FRU callouts, the PN/SN of the FRU
  * The code file that the error log was created in.
  * etc

* PELs have a concept of a Platform Log ID which is a unique identifier for a
  single system event.  If there are multiple log entries generated for a
  single event, and some code knows this, it can modify the PLID field of other
  logs to match the first one.  This implies a requirement that BMC event logs
  also support a method of linking events together so that at PEL creation time
  the PLIDs can be linked.

* An event log must be able to be passed up the call stack and be able to have
  its fields modified after it has been created but before it has been
  committed.  For example, code futher up the call stack should be able to
  change the severity of a log to informational.

* Events must be able to have multiple callouts, where a callout can be:
  * A hardware FRU (a location code)
  * A symbolic FRU number
  * An isolation procedure number

  Callouts also have an associated priority.  The PEL has specific sections and
  formats in which to store callouts, as does the Redfish OEM log.
  * The number and type of callouts may not be known until the code runs.
  * If a callout is for hardware, the serial and part numbers of the FRU must be
    added to the callout at the time of creation by the error logging code, as
    is the IBM location code for that part.
  * MRU IDs may also be in a callout. A MRU is a manufacturing replaceable unit
    and is usually a module on a field replaceable unit.

* The event creator and also handlers of the event up the call stack must be
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
  * The BMC needs to track the host and HMC acks separately.

* IBM has specific requirements on the event log retention when it comes to
  deleting logs to free up space for new ones:
  * If a log resulted in a GARD record,the log must not be pruned as long as
    that GARD record is still present.
  * If a log has a hardware callout such that it is picked up by the nag
    function (see below), even if there is no GARD record, it must not be
    pruned.
  * Whether a log has been reported to both the HMC and host also affects if it
    can be pruned.
  * There are additional detailed polices on the order in which to prune logs
    in the case that space is becoming constrained.  These will be handled in a
    separate design.
  * Event logs do not 'expire' otherwise and will not be pruned just because
    they are old.

* PELs that get sent to the BMC must be saved on the BMC.
  * There must be an OpenBMC event log associated with the PEL, which means
    certain fields must be pulled from the PEL for use in the log:
    * Callouts, Severity, call-home, serviceable, etc.
  * The incoming PEL needs to be modified when it is received: (Is this right?)
    * The MTM needs to be added to it.
    * The callouts may need to be modified.
  * As PELs can be up to 16KB, they must not be stored on D-Bus.

* If a system goes to a quiesced state, then the SRC From the error log that
  caused the failure must be posted on the panel.  This drives the requirement
  that the event log that caused the failure must be known.
  * Manufacturing test has an additional requirement that systems generate no
    errors logs through their tests, which means any single error should
    force the system to quiesce.

* OpenBMC event logs stemming from a host PEL must be returned alongside event
  logs created on the BMC, and PELs obtained from BMC created event logs must
  be returned alongside host PELs in requests for PELs.
  * There may be a specific type of PEL sent down from the host that is not to
    be sent back up again.  The type is <TBD>.

* Power Line Disturbance checks must be done when when a PEL is received
  from the host, or when an event log is created on the BMC that stems from
  a hardware issue.
  * If there was a PLD, the previous log should be made informational and
    its callouts should be removed.
  * The PLD event log(s) should be committed.

* IBM systems display progress codes, which indicate the state of the machine,
  on the panel (either real or virtual).  These progress codes are SRCs.  There
  must be an interface for a caller to send a progress code SRC to the panel.

* The OEM Redfish log entry for an event hasn't been completely defined yet,
  but may contain information like:
  * A text description of the event.  This must be translatable.
  * The severity
  * The SRC that would be in the PEL
  * The FRU callouts
  * The subsystem
  * If call home
  * If serviceable
  * If resolved, and when
  * The event type

  There may be different formats of OEM logs for different types of events,
  such as other formats for audit logs and maintenance logs.  These logs
  would not need to be returned as PELs.

* Event logs, including logs created from PELs, must converted into IPMI
  SEL entries upon request.

* The web UI is required to show event logs that aren't in PEL format.  Its
  interface to the BMC is Redfish.  This implies it will display data
  from the Redfish OEM log calls.

* If a D-Bus method call fails, it must be able to return an event log that
  the caller may either change or add to, and then commit it or abort it.

* Periodically, a check needs to be done for any event logs have callouts with
  deconfigured hardware in them.  If any are found, a new event log needs to be
  created with a special procedure callout telling the customer that service is
  still pending on this system. (Commonly referred to as the nag function.)
  * This drives a requirement that event logs must know if their hardware
    callouts were deconfigured.  This is also a flag in the PEL SRC.
  * The web UI may also need to display if there are any logs with this
    condition.
  * The remaining details on the nag function are outside the scope of this
    document, as are any requirements on which application it is implemented
    in.

Nice to haves:
  * It has been requested for user data to be returned in the Redfish log format
    in a parsed format so IBM's event analysis repositories don't have to
    run parsers for each log.

  * Redfish supports a message registry, which is a JSON file on the BMC that
    contains additional data about events.  If this could add value to what is
    already returned in the OEM logs, it may be good to support.

Not covered by this design:
  * LED activation based on error log callouts.  This is covered by the LED
    design.
