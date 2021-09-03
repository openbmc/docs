# Hardware Fault Monitor

Author:
  Claire Weinan (cweinan@google.com), daylight22)

Primary assignee:
  Claire Weinan (cweinan@google.com, daylight22),
  Heinz Boehmer Fiehn (heinzboehmer@google.com)

Other contributors:
  Drew Walton (acwalton@google.com)

Created:
  Aug 5, 2021

## Problem Description
The goal is to create a new hardware fault monitor which will provide a
framework for collecting various fault and sensor information and making it
available externally via Redfish for data center monitoring and management
purposes. The information logged would include a wide variety of chipset
registers and data from manageability hardware. In addition to collecting
information through BMC interfaces, the module will also receive information
via Redfish from the associated host kernel (specifically for cases in which
the desired information cannot be collected directly by the BMC, for example
when accessing registers that are read and cleared by the host kernel).

Future expansion of this module would include adding the means to locally
analyze fault and sensor information and then based on specified criteria
trigger repair actions in the host BIOS or kernel. In addition, the module
could receive repair action requests via Redfish from external data center
monitoring software.


## Background and References
The following are a few related existing OpenBMC modules:

- Host Error Monitor logs CPU error information such as CATERR details and
  takes appropriate actions such as performing resets and collecting
  crashdumps: https://github.com/openbmc/host-error-monitor

- SEL logger logs to the IPMI and Redfish system event logs when certain events
  happen, such as sensor readings going beyond their thresholds:
  https://github.com/openbmc/phosphor-sel-logger

- FRU fault manager controls the blinking of LEDs when faults occur:
  https://github.com/openbmc/phosphor-led-manager/blob/master/fault-monitor/fru-fault-monitor.hpp

- bmcweb implements a Redfish webserver for openbmc:
  https://github.com/openbmc/bmcweb. The Redfish LogService schema is available
  for logging purposes and the EventService schema is available for a Redfish
  server to send event notifications to clients.

- Dbus-sensors reads and saves sensor values and makes them available to other
  modules via D-Bus: https://github.com/openbmc/dbus-sensors

- Phosphor Debug Collector (phosphor-debug-collector) collects various debug
  dumps and saves them into files:
  https://github.com/openbmc/phosphor-debug-collector

- Guard On BMC records and manages a list of faulty components for isolation.
  (Both the host and the BMC may identify faulty components and create guard
  records for them):
  https://github.com/openbmc/docs/blob/9c79837a8a20dc8e131cc8f046d1ceb4a731391a/designs/guard-on-bmc.md

There is an OpenCompute Fault Management Infrastructure proposal that also
recommends delivering error logs from the BMC:
https://drive.google.com/file/d/1CtRZVLGaDBCoiuP0mk6wjOFZNY_SweNn/


## Requirements
- The users of this solution are Redfish clients in data center software. The
  goal of the fault monitor is to enable rich error logging (OEM and CPU vendor
  specific) for data center tools to monitor servers, manage repairs, predict
  crashes, etc.

- The fault monitor must be able to handle receiving fault information that is
  polled periodically as well as fault information that may come in
  sporadically based on fault incidents (e.g. crash dumps).

- The fault monitor should allow for logging of a variety of sizes of fault
  information entries (on the order of bytes to megabytes). In general, more
  severe errors which require more fault information to be collected tend to
  occur less frequently, while less severe errors such as correctable errors
  require less logging but may happen more frequently.

- Fault information must be added to a Redfish LogService in a timely manner
  (within a few seconds of the original event) to be available to external data
  center monitoring software.

- The fault monitor must allow for custom overwrite rules for its log entries
  (e.g. on overflow, save first errors and more severe errors), or guarantee
  that enough space is available in its log such that all data from the most
  recent couple of hours is always kept intact. The log does not have to be
  stored persistently (though it can be).


## Proposed Design
A generic fault monitor will be created to collect fault information. First we
discuss a few example use cases:

- On CATERR, the Host Error Monitor requests a crash dump (this is an existing
  capability). The crash dump includes chipset registers but doesn’t include
  platform-specific system-level data. The fault monitor would therefore
  additionally collect system-level data such as clock, thermal, and power
  information. This information would be bundled, logged, and associated with
  the crash dump so that it could be post-processed by data center monitoring
  tools without having to join multiple data sources.

- The fault monitor would monitor link level retries and link retrainings of
  high speed serial links such as UPI links. This isn’t typically monitored by
  the host kernel at runtime and the host kernel isn’t able to log it during a
  crash. The fault monitor in the BMC could check link level retries and link
  retrainings during runtime by polling over PECI. If a MCERR or IERR occurred,
  the fault monitor could then add additional information such as high speed
  serial link statistics to error logs.

- In order to monitor memory on a bare metal machine, a system could be
  configured to give the BMC exclusive access to memory error logging registers
  (to prevent the host kernel from being able to access and clear the registers
  before the BMC could collect the register data). For corrected memory errors,
  the fault monitor could log error registers either through polling or
  interrupts. Data center monitoring tools would use the logs to determine
  whether memory should be swapped or a machine should be removed from usage.

The fault monitor will be incorporated as a new monitor in the existing Host
Error Monitor module. Based on the needs of the OEM, the fault monitor will
register to be notified of D-Bus signals of interest in order to be alerted
when fault events occur. The fault monitor will also poll registers of interest
and log their values to the fault log. In addition, the host kernel will be
able to write fault information to the fault log (via a POST (Create) request
to its corresponding Redfish log resource collection). When the fault monitor
becomes aware of a new fault event through any of these ways, it may add fault
information to the fault log (if it isn’t already logged to the main Redfish
system event log by another module). The fault monitor may also gather relevant
sensor data (read via D-Bus from the dbus-sensors services) and add it to the
fault log, with a reference to the original event information. The EventGroupID
in a Redfish LogEntry could potentially be used to associate multiple log
entries related to the same fault event.

The fault log for storing relevant fault information (and exposing it to
external data center monitoring software) will be a new Redfish LogService
(/redfish/v1/Systems/system/LogServices/FaultLog) with
`OverwritePolicy=unknown`, in order to implement custom overwrite rules such as
prioritizing retaining first and/or more severe faults. The back end
implementation of the fault log including saving and managing log files will be
added into the existing Phosphor Debug Collector module with an associated
D-bus object (e.g. xyz/openbmc_project/dump/faultlog) whose interface will
include methods for writing new data into the log, retrieving data from the
log, and clearing the log.

bmcweb will be used as the associated Redfish webserver for external entities
to read and write the fault log. When delivering a Redfish fault log entry to a
Redfish client, large-sized fault information (e.g. crashdumps) can be
specified as an attachment sub-resource instead of being inlined. Redfish
events (EventService schema) will be used to send external notifications, such
as when the fault monitor needs to notify external data center monitoring
software of new fault information being available. Redfish events may also be
used to notify the host kernel and/or BIOS of any repair actions that need to
be triggered based on the latest fault information.


## Alternatives Considered
We considered adding the fault management logging into the main system event
log (/redfish/v1/Systems/system/LogServices/EventLog) or other logs already
existing in bmcweb (e.g. /redfish/v1/Systems/system/LogServices/Dump,
/redfish/v1/Managers/bmc/LogServices/Dump), but we would like to implement a
separate custom overwrite policy to ensure the most important information (such
as first errors and most severe errors) is retained for local analysis.


## Impacts
There may be situations where external consumers of fault monitor logs (e.g.
data center monitoring tools) are running software that is newer or older than
the version matching the BMC software running on a machine. In such cases,
consumers can ignore any types of fault information provided by the fault
monitor that they are not prepared to handle.

Errors are expected to happen infrequently, or to be throttled, so we expect
little to no performance impact.

## Testing
Error injection mechanisms or simulations may be used to artificially create
error conditions that will be logged by the fault monitor module.

There is no significant impact expected with regards to CI testing, but we do
intend to add unit testing for the fault monitor.
