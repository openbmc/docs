# State Transition Audit Log

Author: [Ramya Sivakumar](mailto:sramya@ami.com)

Created: August 11, 2026

## Problem Description

OpenBMC does not provide a unified, structured history of BMC, chassis, and host
power-state changes. An operator cannot reliably answer who powered off a system
at a particular time. Diagnosis instead requires manually correlating journald
records emitted by several services.

This proposal adds a persistent State Transition Audit Log that records every
observed BMC, chassis, and host state transition together with its time,
initiator when available and origin. The history will be available to Redfish
and IPMI consumers. This proposal does not change state-transition semantics,
replace the system event log, or audit configuration changes that are unrelated
to BMC, chassis, or host state.

## Background and References

`phosphor-state-manager` owns the D-Bus state objects for the BMC, chassis, and
host. Their current state is represented by `CurrentBMCState`,
`CurrentPowerState`, and `CurrentHostState`; requested actions are represented
by the associated requested-transition properties. `phosphor-state-manager` also
coordinates transitions through systemd targets.

An observed D-Bus state-property update alone does not contain the authenticated
user or the external protocol that requested it. Conversely, Redfish and IPMI
can identify an authenticated caller but do not observe every resulting state
update, including ones caused by hardware, watchdogs, or power restore policy.
The design therefore requires a trusted request context at the protocol boundary
and a common observer in `phosphor-state-manager`.

References:

- [phosphor-state-manager documentation][psm-readme]
- [OpenBMC systemd architecture][openbmc-systemd]
- [DMTF Redfish LogService schema][redfish-log-service]
- [DMTF Redfish LogEntry schema][redfish-log-entry]
- [IPMI v2.0 Specification, System Event Log][ipmi-sel]

[psm-readme]:
  https://github.com/openbmc/phosphor-state-manager/blob/master/README.md
[openbmc-systemd]:
  https://github.com/openbmc/docs/blob/master/architecture/openbmc-systemd.md
[redfish-log-service]: https://redfish.dmtf.org/schemas/LogService.json
[redfish-log-entry]: https://redfish.dmtf.org/schemas/LogEntry.json
[ipmi-sel]:
  https://www.intel.com/content/dam/www/public/us/en/documents/product-specifications/ipmi-second-gen-interface-spec-v2-rev1-1.pdf

## Requirements

The feature shall record every change to the current state of each
`phosphor-state-manager`-managed BMC, chassis, and host instance. This includes
intermediate states, such as `TransitioningToOff`, so an audit trail represents
what actually occurred rather than only the final requested action.

The log records state changes observable by `phosphor-state-manager`. A power
loss that stops the BMC cannot be recorded at the instant it occurs; normal
state discovery records the recovered state when the BMC starts again.

Each entry shall include a persistent, monotonic entry ID; UTC epoch time in
milliseconds; a boot-relative monotonic timestamp; component and instance;
previous and new state; requested transition when applicable; trigger; actor;
and request correlation ID. Valid triggers include `Redfish`, `IPMI`,
`PhysicalButton`, `Watchdog`, `Policy`, `Service`, `PowerLoss`, and `Unknown`.
Actor data shall contain a user name, IPMI user and channel, trusted service
name, or `Unknown`; it shall never contain credentials or session tokens.

The audit log shall survive `phosphor-state-manager` and BMC restarts, use
bounded storage, preserve chronological retrieval order, and support a
platform-configurable retention count. Hardware, policy, watchdog, and direct
D-Bus-originated changes shall remain auditable even where no authenticated
actor exists. The implementation must not infer an actor it cannot establish.

Read access must be exposed to Redfish and IPMI. Clearing the log shall require
the appropriate administrative privilege. Standard IPMI SEL integration is not
required: SEL record size and data shape cannot carry all required audit fields.
The feature shall be optional at build time so platforms that do not need
persistent transition history can exclude it.

## Proposed Design

`phosphor-state-manager` shall contain a State Transition Audit Manager. It
observes state changes committed by the existing BMC, chassis, and host state
managers. It does not initiate, delay, retry, or otherwise alter a power
operation. Capturing committed changes makes the audit history authoritative for
the resulting state while preserving current state-machine behavior.

```text
 Redfish client       IPMI client       Internal / physical source
       |                  |                       |
     bmcweb          phosphor-ipmi-host          state manager / service
       |                  |                       |
       +---- Audit request-context D-Bus API -----+
                              |
                    phosphor-state-manager
               +--------------+---------------+
               | Existing state managers      |
               | BMC / Chassis / Host         |
               +--------------+---------------+
                              | observed state changes
                    State Transition Audit Manager
                              |
                 persistent bounded audit store
                    |                       |
              Redfish LogService      IPMI OEM retrieval
```

Before it requests a power transition, bmcweb or the IPMI command handler shall
register a short-lived request context with the Audit Manager. The context
contains an opaque request ID, target component and instance, requested
transition, authenticated actor, protocol, and source channel when available.
The normal state-transition request then proceeds without modification.

The Audit Manager shall correlate a context to observed state changes by
component, instance, requested transition, and a bounded validity period. It
shall create one audit entry for each current-state change. A chassis
`PowerCycle`, for example, can produce entries for the transitions toward off,
off, toward on, and on; the common request ID links these entries. If no context
matches, the manager shall create an `Observed` entry with a known internal
trigger or `Unknown`.

`phosphor-dbus-interfaces` shall define an interface tentatively named
`xyz.openbmc_project.State.TransitionAudit`. The interface shall expose a
manager object and read-only entry objects under:

```text
/xyz/openbmc_project/state/audit
/xyz/openbmc_project/state/audit/entry/<id>
```

Trusted clients shall use a request-context registration method on the manager
object. The manager shall expose retention capacity, entry count, oldest entry
ID, and an authorized clear operation. Entry objects shall expose the audit
fields specified in the requirements.

D-Bus policy shall permit request-context registration only from bmcweb,
`phosphor-ipmi-host`, and explicitly authorized internal services. This avoids
allowing an untrusted D-Bus caller to claim a different user identity. Clients
that use existing state interfaces without a registered context remain
supported; their entries contain an `Unknown` actor and appropriate trigger.

`phosphor-state-manager` shall persist entries in its durable state location
with a versioned format and atomic replacement. The log shall be a fixed-count
ring buffer, with a default retention count of 1,024 entries. A corrupt or
incompatible persisted file shall be preserved for diagnosis and reported
through normal logging; `phosphor-state-manager` shall start an empty audit
store instead of interpreting invalid data.

The BMC reboot request shall be recorded when it is accepted. Once the BMC has
restarted, normal `phosphor-state-manager` state discovery will record
subsequent observed states. This preserves both the initiating action and the
post-restart state without requiring a process to observe events while it is not
running.

## Alternatives Considered

Using only journald was rejected. Journald is useful for diagnostics, but the
relevant records are distributed across services, retention is not specific to
this feature, and it provides no consistent API for actor correlation or
Redfish/IPMI retrieval.

Using IPMI SEL or phosphor-logging as the primary audit store was rejected. SEL
capacity and record format are insufficient for all state transitions, actor
identity and correlation IDs. phosphor-logging can complement diagnostic
reporting but does not provide a bounded, queryable state-transition history. A
platform may later emit a concise SEL summary, but the Audit Manager remains the
authoritative record.

Recording events separately in bmcweb and IPMI was rejected because it cannot
cover physical buttons, hardware loss and restore, watchdog events, policy
actions, direct D-Bus callers, and `phosphor-state-manager` state discovery.
`phosphor-state-manager` is the shared observer for all managed state changes.

## Dependency Module Functionality

- `phosphor-state-manager`: Owns the Audit Manager; observes BMC, chassis, and
  host state changes; correlates contexts; persists entries; and publishes audit
  D-Bus objects.
- `phosphor-dbus-interfaces`: Defines the Audit Manager and Audit Entry D-Bus
  interfaces, enums, methods, properties, errors, and object paths.
- bmcweb: Registers authenticated Redfish request context before transition
  requests and maps D-Bus entries to Redfish LogService and LogEntry resources.
- `phosphor-ipmi-host`: Registers authenticated IPMI user and channel context,
  and implements audited OEM retrieval and clear commands.
- `sdbusplus`: Provides D-Bus interface generation, property-change signals,
  object-server support, and request-context communication.
- systemd: Continues executing existing `phosphor-state-manager` power targets.
  No new transition targets are required.
- bmcweb session authentication: Supplies authenticated Redfish identity and
  authorization privileges.
- IPMI user/session management: Supplies authenticated IPMI username, channel,
  and privilege information.

## Testing

- Verify an entry is created for every BMC, chassis, and host state change that
  `phosphor-state-manager` can observe, including intermediate transition
  states.
- Verify each entry contains the component, states, timestamp, trigger, actor,
  requested transition, and request ID.
- Verify Redfish and IPMI operations record the authenticated actor, while
  hardware, policy, and watchdog events use the correct non-user trigger.
- Verify all state changes from one request, including `PowerCycle`, share one
  request ID, and unmatched changes create an `Observed` entry.
- Verify entries persist through `phosphor-state-manager` restart and BMC
  reboot, and retention removes only the oldest entries at capacity.
- Verify only trusted D-Bus services register request context, and only
  authorized users can clear the audit log.
- Verify Redfish and IPMI retrieve entries in chronological order with the
  required audit fields.
