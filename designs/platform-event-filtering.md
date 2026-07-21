# Platform Event Filtering (PEF) for OpenBMC

Author: Anirudh Tiwari (Discord: <add your Discord nickname>)

Other contributors: None

Created: June 16, 2026

## Problem Description

IPMI 2.0 Platform Event Filtering (PEF) lets a management client install a
table of event filters in the BMC, so the BMC takes a configured action
(alert, power control, diagnostic interrupt, or an OEM action) when an event
it logs matches a filter. OpenBMC implements none of this: the Sensor/Event
commands Get PEF Capabilities and Get/Set PEF Configuration Parameters are
not serviced, and nothing evaluates logged events against a filter table. The
goal is to add PEF so a host can install filters and receive the OEM
action on critical events (for example a memory uncorrectable
error). Implementing external alert transports (SNMP PET) is a non-goal; the
design only leaves room for them.

## Background and References

PEF and its commands are defined by the "Intelligent Platform Management
Interface Specification, v2.0": section 17 (Platform Event Filtering) covers
the filter table and matching, and section 30 covers the PEF commands. The
20-byte event filter entry is defined in section 17.7.

Related OpenBMC work: phosphor-sel-logger owns SEL logging and is the natural
home for event matching; phosphor-dbus-interfaces defines the D-Bus contract
conventions; the IPMI command to D-Bus split is described in
https://github.com/openbmc/docs/blob/master/architecture/ipmi-architecture.md.

## Requirements

A management client must be able to read PEF capabilities and read/write the
PEF global configuration and the numbered event filter table over the standard
IPMI Sensor/Event commands (section 30). The filter table and global
configuration must persist across BMC reboots.

The BMC must evaluate every event it logs to the SEL against the enabled
filters and execute the matching filter's action. At minimum the OEM action
must be delivered end to end; the alert and power-control actions must be
expressible without a wire or interface change. Get PEF Capabilities must
advertise only the actions actually delivered on a given build, so a client is
never told an action is supported when no component implements it.

The table is small (on the order of tens of filters) and matching runs once
per logged event, so per-event cost is not a concern. PEF configuration is an
optional, build-time feature (see architecture/optionality.md); builds that do
not enable it are unaffected, and the IPMI commands report the provider as
unavailable. Setting PEF configuration requires Admin privilege; reading
configuration requires Operator and reading capabilities requires User,
per IPMI 2.0 Appendix G.

## Proposed Design

PEF is split into three cooperating components, each in its natural
repository:

```
   IPMI client (e.g. ipmitool)
        |  NetFn Sensor/Event: Get PEF Capabilities + Get/Set Config Params
        v
  +-------------------------------+
  | phosphor-host-ipmid           |  this change (pefhandler)
  | PEF IPMI <-> D-Bus translator |  stateless; validates params,
  +-------------------------------+  forwards over D-Bus
        |  xyz.openbmc_project.PefManager
        |    SetFilter() / GetFilter() + global config properties
        v
  +-------------------------------+
  | PEF provider (new service)    |  owns + persists the filter table
  | hosts Manager + Filter        |  and global config; publishes one
  +-------------------------------+  Filter object per populated slot
        |  ObjectManager: InterfacesAdded / Removed,
        |  PropertiesChanged on xyz.openbmc_project.PefFilter
        v
  +-------------------------------+
  | phosphor-sel-logger           |  PEF matcher (pef_filter_match)
  | matches events, runs action   |  compares each logged event vs
  +-------------------------------+  enabled filters; runs the action
        |  OEM action -> platform-specific handler / NMI / power op
        v
   host firmware / platform
```

The translator (this change) is stateless. It validates the IPMI parameter
selectors, reserved bits and filter numbers, then forwards to the provider. It
carries the 20-byte event filter entry (IPMI 2.0 section 17.7) verbatim and
never interprets entry bytes other than byte 0 for the Event Filter Data 1
read-modify-write.

The provider (new) owns the authoritative filter table and the PEF global
controls, persists them, and republishes them on boot. It exposes
xyz.openbmc_project.PefManager for management and one
xyz.openbmc_project.PefFilter object per populated slot, and hosts an
ObjectManager so the matcher learns of slot add/remove. The global PEF
controls are typed D-Bus properties; the per-filter entry is exposed as a
fixed 20-byte array because it is a fixed spec-defined structure and the
matcher needs the exact bytes. The interface definitions are submitted as a
companion phosphor-dbus-interfaces change.

## Alternatives Considered

Implementing all of PEF inside phosphor-host-ipmid was rejected: ipmid is a
protocol translator and should not own persisted platform state or run an
event-matching loop, and SEL logging already lives in phosphor-sel-logger.
Implementing PEF entirely inside phosphor-sel-logger and poking it directly
from ipmid was rejected because it couples the IPMI parameter model into the
logger and offers no clean D-Bus surface for other front-ends (Redfish,
in-band tooling).

Modelling every filter sub-field as an individual D-Bus property, instead of a
raw 20-byte entry, was considered and set aside: the entry is a fixed spec
structure, so per-field properties add churn and a lossy reassembly step with
no consumer benefit. Storing filters in phosphor-settings was rejected because
PEF needs a live object per slot plus ObjectManager add/remove semantics to
drive the matcher, which a flat settings store does not provide.

## Impacts

API impact: two new D-Bus interfaces (xyz.openbmc_project.PefManager and
.PefFilter) in phosphor-dbus-interfaces; new IPMI commands serviced; no change
to existing interfaces. Security impact: writing PEF configuration requires
Admin privilege while reads require Operator (IPMI 2.0 Appendix G); actions
that affect platform state are gated
by the advertised action-support mask, which lists only actions a consumer
implements. Documentation impact: this design doc plus the interface
documentation in the YAML. Performance impact: one O(enabled-filters) scan per
logged event over a small table; negligible. Developer impact: an optional
provider service; builds without PEF are unaffected and the commands return
Destination Unavailable when no provider is present. Upgradability impact: the
persisted filter format is owned and versioned by the provider.

### Organizational

- Does this proposal require a new repository? Possibly. The PEF provider may
  be a new repository or may live within an existing platform service; this is
  open for maintainer input.
- Who will be the initial maintainer(s) of this repository? The submitting
  contributor will provide and maintain the initial implementation;
  co-maintainership is welcome.
- Which repositories are expected to be modified to execute this design?
  - phosphor-host-ipmid (the pefhandler translator, this change)
  - phosphor-dbus-interfaces (the PefManager and PefFilter interfaces)
  - phosphor-sel-logger (the pef_filter_match event matcher)
  - the PEF provider service (new, or an existing platform repository)
- Maintainers of the listed repositories will be added to the Gerrit reviews.

## Testing

Unit tests in phosphor-host-ipmid cover the parameter selectors, reserved-bit
rejection on the selector byte, PEF-control truncation to implemented bits,
action-global-control rejecting un-advertised bits, the event filter table
20-byte round-trip, the Event Filter Data 1 read-modify-write preserving the
other 19 bytes, and completion-code mapping (Destination Unavailable when the
provider is absent; never CC 0x00 with a stale or zeroed payload).

Integration testing uses ipmitool to set then get the filter table and global
controls. End-to-end testing installs a memory-uncorrectable OEM-action filter,
delivers a matching platform event, and confirms the OEM action
fires while a non-matching event does not. CI impact: the new unit tests run in
the phosphor-host-ipmid test suite; no new CI infrastructure is required.
