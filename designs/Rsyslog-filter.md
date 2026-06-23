# Rsyslog Filter Support in OpenBMC

Author: Palaniappan Arunachalam

Other contributors: None

Created: June 23, 2026

## Problem Description

OpenBMC currently forwards all syslog messages (*.*) to a configured remote
server. This causes excessive outbound traffic, poor signal-to-noise ratio,
and potential exposure of low-value or sensitive logs.

The goal of this design is to add standards-aligned remote forwarding filters
using Redfish SyslogFilters, while preserving backward compatibility.

Non-goals:

- Modify local logging behavior (journald or phosphor-logging)
- Introduce non-standard filtering syntax in API contracts
- Replace rsyslog

## Background and References

Redfish defines EventDestination.SyslogFilters as the standard API surface for
remote syslog forwarding filters. This design keeps Redfish as the external
contract, D-Bus as the internal contract, and phosphor-rsyslog-config as the
translation layer that generates rsyslog configuration.

The current OpenBMC behavior forwards all remote logs by default using *.*.
This proposal adds filtering on facility and severity while preserving that
default behavior when no valid filters are configured.

References:

- Redfish EventDestination and SyslogFilters schema
- rsyslog selector syntax and severity/facility model
- OpenBMC phosphor-rsyslog-config implementation and server.conf generation

## Requirements

The design must support filtering by both facility and severity threshold, and
the configuration must be exposed through Redfish. Layering must remain clear:
Redfish for external API, D-Bus for internal contracts, and backend services
for implementation details.

rsyslog syntax must not be exposed outside backend components. The default
forwarding behavior must remain backward compatible (*.* forwarding) when no
valid filters are configured.

Operational requirements:

- Atomic config updates
- No partial write of broken rsyslog config
- Last-known-good behavior on backend failure

## Proposed Design

High-level flow:

Redfish -> bmcweb -> D-Bus -> phosphor-rsyslog-config -> rsyslog

Redfish model example:

{
  "SyslogFilters": [
    {
      "LogFacilities": ["Kern", "User"],
      "LowestSeverity": "Warning"
    }
  ]
}

Severity ordering (most severe to least severe):

Emergency > Alert > Critical > Error > Warning > Notice > Info > Debug

LowestSeverity includes messages equal to or more severe than the provided
value.

D-Bus contract (implementation overlay):

Note: This is an implementation overlay. Exact D-Bus signature and enum typing
are TBD and will be finalized during backend interface definition.

Selectors: array of structs

- facilities[]: enum {kern, user, mail, daemon, auth, syslog, lpr, news,
  uucp, cron, authpriv, ftp}
- severity: enum {emerg, alert, crit, err, warning, notice, info, debug}
- modifier: enum {gte} (default)

Canonical internal format is lowercase.

Mapping example:

auth.warning @<ip>:<port>

## Error handling policy

- Partial accept model: invalid selectors are ignored, valid selectors are
  applied
- Fallback to *.* occurs only when no valid selectors remain after validation
- Availability is prioritized over strict filtering to avoid accidental loss of
  logging

Redfish conformance:

- Enum values must match Redfish schema
- Invalid values return HTTP 400
- Input matching is case-sensitive
- Redfish enum tokens are validated as-is at the API boundary
- Backend uses explicit one-to-one mapping to canonical lowercase internal
  values
- Empty SyslogFilters reverts to default (*.*)

## Alternatives Considered

Forward all logs without filtering:

- Rejected because it does not address traffic volume, signal quality, or
  exposure concerns.

Expose raw rsyslog selectors through Redfish or D-Bus:

- Rejected because it leaks backend-specific syntax and weakens API
  portability and standards alignment.

Reject entire configuration when any selector is invalid:

- Considered but not selected.
- The chosen partial-accept policy is more availability-oriented for remote
  logging continuity.

## Impacts

API impact:

- Adds/uses Redfish SyslogFilters for remote logging destinations.

Security impact:

- Reduces exposure when filters are valid.
- Fallback to *.* can increase exposure and is an explicit availability trade-
  off.

Documentation impact:

- Requires updates to OpenBMC remote logging and Redfish behavior docs.

Performance impact:

- Minimal overhead from config regeneration and rsyslog reload on updates.

Developer impact:

- Requires coordinated handling in bmcweb, D-Bus modeling, and
  phosphor-rsyslog-config translation logic.

Upgradability impact:

- Backward compatible default behavior is preserved.

### Organizational

- Does this proposal require a new repository? No
- Initial maintainers: Existing maintainers of touched repositories
- Repositories expected to change:
  - bmcweb
  - phosphor-rsyslog-config
  - phosphor-dbus-interfaces (if D-Bus interface is formalized)
- Add maintainers from affected repositories to Gerrit review

## Testing

Unit tests:

- Selector validation
- Severity/facility mapping
- Rule generation

Integration tests:

- Redfish -> bmcweb -> D-Bus -> backend -> server.conf -> rsyslog flow

Regression tests:

- Default (*.*) path remains functional
- Partial invalid selectors are ignored while valid selectors apply
- Full invalid selector set triggers fallback behavior
- Duplicate forwarding behavior is as documented
- rsyslog reload failure keeps last known good config

CI impact:

- Add or extend existing test coverage in affected repositories.

## Additional Links

- None
