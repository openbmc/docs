# CCI over MCTP stack

Author: Carter Chen ([carter_chen@wiwynn.com](mailto:carter_chen@wiwynn.com))

Contributors:
- Marshall Zhan ([marshall_zhan@wiwynn.com](mailto:marshall_zhan@wiwynn.com))
- Eric Yang ([eric_yang@wiwynn.com](mailto:eric_yang@wiwynn.com))
- Unive Tien ([unive_tien@wiwynn.com](mailto:unive_tien@wiwynn.com))

Created: Aug 4, 2026

## Problem Description

Today most out-of-band device management on OpenBMC is done with **PLDM over
MCTP**. However, some devices do not implement PLDM — CXL Type 3 devices in
particular expose their management surface through the CXL **Component Command
Interface (CCI)**, not PLDM. For those devices, PLDM-based tooling cannot reach
them at all; we need a path that speaks CCI, so the BMC can monitor these
devices — their events, identity, and logs — and surface that alongside
everything else it manages.

CCI for CXL Type 3 devices is transported over MCTP, as defined by the CXL
Type 3 Device CCI over MCTP Binding Specification (DSP0281). But today there is
no OpenBMC service that constructs CCI commands and conveys them to CXL Type 3
devices over MCTP.

That service should be a single daemon, not a library each caller links
directly, because:

- Several BMC services may want to talk to CXL devices. A single daemon gives
  one coordinated owner of the shared transport, instead of several
  uncoordinated library instances interfering with each other.
- We also need to handle the CCI command transmission in the CCI daemon while
  doing a CXL device firmware update over CCI, to prevent unexpected
  transmission during firmware updating.

So this design employs a single daemon, **`ccid`**, to send and receive CXL
Type 3 Device CCI messages over MCTP on behalf of every other BMC service.

This document scopes the daemon to the read/telemetry-oriented command
categories — **Events, Information and Status, Timestamp, and Logs** (see
Requirements). Firmware update and the other categories are deliberately left
for later.

## Background and References

Compute Express Link is a dynamic multi-protocol technology designed to support
accelerators and memory devices. CXL provides a rich set of protocols that
include I/O semantics similar to PCIe (CXL.io), caching protocol semantics
(CXL.cache), and memory access semantics (CXL.mem) over a discrete or
on-package link.

CXL Type 3 devices are devices that support CXL.io and CXL.mem protocols. These
devices can be managed during runtime via a Component Command Interface (CCI)
that represents a command target for management and configuration commands.
The CCI and the commands supported via the CCI are defined by the CXL
Consortium.

CCI commands are sent to CXL Type 3 devices over MCTP, which is already
supported via the kernel-native `AF_MCTP` stack.

Each MCTP message carries a Message Type as the first byte of the message
data, defined in the MCTP IDs and Codes Specification (DSP0239). This value
indicates the protocol of the message contents — e.g. `0x01` for PLDM, `0x04`
for NVMe-MI. For CXL Type 3 CCI over MCTP messages, the assigned type value is
**`0x08`**.

DSP0281 defines the MCTP transport binding; the CCI commands and event record
formats come from the CXL specification. This design targets **CXL
Specification Revision 3.1** for the CCI/event content.

References:

- DMTF DSP0281 — [CXL Type 3 Device CCI over MCTP Binding Specification 1.0.0](https://www.dmtf.org/sites/default/files/standards/documents/DSP0281_1.0.0_0.pdf)
- DMTF DSP0236 — [MCTP Base Specification 1.3](https://www.dmtf.org/sites/default/files/standards/documents/DSP0236_1.3.0.pdf)
- DMTF DSP0239 — [MCTP IDs and Codes 1.9](https://www.dmtf.org/sites/default/files/standards/documents/DSP0239_1.9.pdf)
- CXL Consortium — [Compute Express Link Specification, Revision 3.1](https://computeexpresslink.org/wp-content/uploads/2024/02/CXL-3.1-Specification.pdf)

## Requirements

**Scope.** The daemon implements four Generic Component Command categories
from CXL r3.1 Table 8-37 — the telemetry/status set needed to observe and
identify a CXL Type 3 device from the BMC:

**Events (Command Set `01h`)**

| Opcode | Command |
|---|---|
| `0100h` | Get Event Records |
| `0101h` | Clear Event Records |
| `0104h` | Get MCTP Event Interrupt Policy |
| `0105h` | Set MCTP Event Interrupt Policy |
| `0106h` | Event Notification |

**Information and Status (Command Set `00h`)**

| Opcode | Command |
|---|---|
| `0001h` | Identify |
| `0002h` | Background Operation Status |
| `0003h` | Get Response Message Limit |
| `0004h` | Set Response Message Limit |
| `0005h` | Request Abort Background Operation |

**Timestamp (Command Set `03h`)**

| Opcode | Command |
|---|---|
| `0300h` | Get Timestamp |
| `0301h` | Set Timestamp |

**Logs (Command Set `04h`)**

| Opcode | Command |
|---|---|
| `0400h` | Get Supported Logs |
| `0401h` | Get Log |
| `0402h` | Get Log Capabilities |
| `0403h` | Clear Log |
| `0404h` | Populate Log |
| `0405h` | Get Supported Logs Sub-List |

Functional:

- **Events**: detect when a device raises an Event Notification, retrieve the
  records via Get Event Records, surface each as a standard BMC log entry (see
  D-Bus surface), and clear them via Clear Event Records once consumed. Allow
  configuring the device's notification behaviour via Get/Set (MCTP) Event
  Interrupt Policy, as daemon-internal configuration.
- **Information and Status**: identify each CXL Type 3 device (vendor/device
  IDs, serial number, component type) and expose it on the BMC; track
  background-operation status and support aborting one.
- **Timestamp**: read and set the device timestamp.
- **Logs**: enumerate the device's supported logs and read them (e.g. the
  Command Effects Log), with clear/populate as needed.

Out of scope: Firmware Update, Features, Maintenance, and PBR Components
command categories.

## Proposed Design

Single `ccid` daemon, event-driven, serving both requester and responder
roles — it issues CCI commands and handles incoming responses and
notifications.

We implement the CCI encode/decode **ourselves, as part of `ccid`**.

Layering inside the BMC. The left column is the per-message data path; the
right column is the control/config plane (all D-Bus, set up before any CCI
traffic flows):

```text
  Legend:   |  MCTP data path (per message)        :  D-Bus link

  +------------------------------------+       +---------------------------+
  | Redfish / bmcweb · CLI             |       | entity-manager            |
  | (event log + CXL device view)      |       | (MCTP / CXL device config)|
  +----+-------------------------+-----+       +-------------+-------------+
       : reads Logging.Entry     : reads CXL device          : consumed by
       :                         : object (read-only)        :
  +----+-------------+           :                       +----v------------+
  | phosphor-logging |           :                       | MCTPReactor     |
  +----+-------------+           :                       +----+------------+
       : lg2::commit()           :                            : drives
  +----+-------------------------+-----+                  +----v------------+
  | ccid                               | :·· discovery ···| mctpd           |
  |   event logic (fetch on notify,    |                  | (EID resolution)|
  |   translate to log, manage policy) |                  +-----------------+
  |   CCI codec + owns AF_MCTP socket  |
  |   publishes read-only CXL device   |
  |   object (identity, eff. policy)   |
  +-----------------+------------------+
                    | AF_MCTP (type 0x08)
  +-----------------+------------------+
  | kernel MCTP stack                  |
  +-----------------+------------------+
                    | i2c / i3c / PCIe VDM driver
                    v
          +----------------------+
          | CXL Type 3 device    |
          +----------------------+

  Every ':' link is D-Bus. entity-manager → MCTPReactor → mctpd sets up the
  MCTP endpoint/EID; ccid then reads endpoint discovery from mctpd and its
  per-device event-interrupt policy from entity-manager's CXLCCIDevice config
  (see Configuration) — both over D-Bus.
```

Everything on the right — entity-manager, `MCTPReactor`, `mctpd` — and the
kernel MCTP stack already exist; `ccid` consumes them (reads endpoint
discovery from `mctpd` over D-Bus, never touches EID assignment itself). What
this design actually builds is `ccid` and a small amount of shared plumbing:

- **`ccid`** — a new daemon (new repository), event-driven.
- **CCI codec** — encode/decode for the four command categories
  (Events, Information and Status, Timestamp, Logs); `ccid` owns a kernel
  `AF_MCTP` socket on Message Type `0x08`.
- **Event-handling logic** — detect a notification and fetch records,
  translate each record to a `Logging.Entry`, and manage the event interrupt
  policy.
- **Device discovery** — read CXL endpoints from `mctpd` over D-Bus; no
  hardcoded EIDs.
- **New phosphor-logging error definitions** in `phosphor-dbus-interfaces`,
  so decoded events surface as standard `Logging.Entry` objects.
- **Semantic D-Bus surface** — events via the standard `Logging.Entry`, plus
  a `ccid`-defined read-only CXL device interface (identity, effective policy,
  timestamp); no raw CCI is exposed.
- **Configuration** — read the event interrupt policy (see Configuration) and
  apply it during initialization.

### Command categories at a glance

How each in-scope category is used by the daemon:

| Category | When | Role in `ccid` |
|---|---|---|
| Information and Status (`00h`) | init | Identify the device (vendor/serial/type) → inventory; negotiate response message limit; track/abort background operations |
| Timestamp (`03h`) | init | Sync the device clock to BMC time and verify it, so event timestamps are meaningful |
| Logs (`04h`) | init / on-demand | Enumerate the device's supported logs and read them (e.g. Command Effects Log) |
| Events (`01h`) | runtime | Receive notifications, fetch and clear event records, surface them as `Logging.Entry` |

### CCI encode/decode

The CCI encode/decode is written as part of `ccid`: given a CCI
opcode and its data fields it builds the CCI-over-MCTP message, and it decodes
the responses. It covers the four command categories in scope (Events,
Information and Status, Timestamp, Logs) against the DSP0281 wire format and
CXL r3.1 command definitions.

### Event handling

Event handling has three responsibilities inside `ccid`:

- react to an incoming Event Notification (`0106h`) and trigger a Get Event
  Records call;
- decode each CXL event record and commit it as a phosphor-logging event via
  `lg2::commit`, producing the `Logging.Entry` (see D-Bus surface below);
- own the (MCTP) Event Interrupt Policy state — configuring this is what makes
  the device raise notifications in the first place.

The Information-and-Status, Timestamp and Logs commands are issued on demand
and exposed via D-Bus (see D-Bus surface).

Each CXL event record carries a severity (CXL r3.1 Common Event Record, Event
Record Flags bits[1:0]: Informational / Warning / Failure / Fatal). `ccid`
maps this to a phosphor-logging `Entry.Level` (defined in
phosphor-dbus-interfaces); the mapping below is ccid's choice, not
spec-mandated. `bmcweb` then derives the Redfish severity (OK / Warning /
Critical) from that level.

| CXL event severity | `Entry.Level` (ccid's mapping) |
| ------------------ | ------------------------------ |
| Informational      | Informational                  |
| Warning            | Warning                        |
| Failure            | Error                          |
| Fatal              | Critical                       |

### Transport layer

`ccid` opens a kernel `AF_MCTP` socket bound to Message Type `0x08` and uses
it for both sending requests and receiving responses/notifications.
Multi-packet segmentation and reassembly are handled entirely by the kernel
MCTP stack, so `ccid` only ever deals with whole CCI messages.

Every CCI request is bounded by a timeout so an unresponsive device cannot
stall the daemon. On timeout or transport error `ccid` abandons that request
and logs the failure; because it is single-in-flight per device and
event-driven, other devices continue to be served. A device that stays
unreachable is reflected through its `OperationalStatus` (Functional = false)
on D-Bus so the condition is visible in Redfish, and `ccid` re-attempts on the
next notification or discovery event rather than busy-retrying.

### Device discovery

`ccid` does not hardcode EIDs. On startup and on D-Bus `InterfacesAdded`, it
queries Code Construct's `mctpd` (bus name `au.com.codeconstruct.MCTP1`) for
endpoint objects exposing `xyz.openbmc_project.MCTP.Endpoint` whose supported
message types include `0x08`; those endpoints are populated from
entity-manager's CXL device configuration by OpenBMC's `MCTPReactor`. This
gives `ccid` a live EID → device map without its own topology logic.

On `InterfacesRemoved`, `ccid` drops that EID and releases its per-device
state, leaving no stale entry.

### Configuration

The proposal is to configure the event interrupt policy **per device**
through entity-manager, as a dedicated `CXLCCIDevice` config entry. The main
reason for entity-manager:
with **multiple CXL devices**, it already associates each config with its
physical device and MCTP endpoint, so `ccid` knows which policy belongs to
which device without inventing its own matching.

The concrete config/schema shape is left as an implementation detail — for
example, adding a small `CXLCCIDevice` schema entry in entity-manager — with
the exact field names and layout decided during implementation.

### Initialization flow (example)

When a CXL Type 3 device is discovered (EID from `mctpd`), `ccid` brings it to
a known state before steady-state event handling. The steps below are an
**illustrative example** of that startup, not a prescriptive sequence:

1. **Identify** (`0001h`) — read vendor/device IDs, serial, component type;
   record them on the device's inventory object.
2. **Set Timestamp** (`0301h`) then **Get Timestamp** (`0300h`) — push BMC
   time to the device, read it back, and confirm it matches within a
   tolerance. If it does not, log a warning and continue (event timestamps
   would otherwise be meaningless).
3. Read this device's `CXLCCIDevice` config from entity-manager and apply it
   via **Set MCTP Event Interrupt Policy** (`0105h`).
4. **Get MCTP Event Interrupt Policy** (`0104h`) — read the applied policy
   back, confirm it matches what was set, and publish it as **read-only D-Bus
   properties** on the device object (so operators can see the effective
   policy, and mismatches are visible).
5. Drain any events already pending from before `ccid` started, using the
   same **Get → commit each as `Logging.Entry` → Clear** pipeline as the
   runtime flow. (A plain Get only reads the records; it is **Clear Event
   Records** that removes them.)
6. Enter steady state — wait for Event Notifications.

```text
device discovered (EID from mctpd)
        │
        ▼
  Identify (0001h) ─────────────▶ record vendor / serial / type in inventory
        │
        ▼
  Set Timestamp (0301h) ── BMC time ──▶ device
        │
        ▼
  Get Timestamp (0300h) ◀── device time
        │
        ├── within tolerance? ── no ──▶ log warning (continue)
        ▼ yes
  Set MCTP Event Interrupt Policy (0105h)  ◀── EM config (per device)
        │
        ▼
  Get MCTP Event Interrupt Policy (0104h)  ◀── read back
        │
        ├── matches what was set? ── no ──▶ log warning
        ▼ yes
  publish applied policy as read-only D-Bus properties
        │
        ▼
  drain pending events (see step 5)
        │
        ▼
  steady state: wait for Event Notification (0106h)
```

Persistence across a BMC reboot follows from reusing phosphor-logging: events
already committed are stored by the logging service and remain visible in
Redfish after a restart. `ccid` keeps no durable state of its own — on startup
it re-runs initialization for each discovered device (re-applies the interrupt
policy, then drains any events that accumulated while it was down), so a
restart neither loses events nor leaves the device un-armed.

### Request/response model — Event Log flow

Events are **notification-driven**: when a device has new events it raises an
Event Notification (`0106h`); `ccid` reacts and fetches the records. This
requires the device's event interrupt policy to have been configured.

The flow: on the notification, `ccid` first **acknowledges it**, then issues a
**Get Event Records** to that EID. `ccid` decodes the response and commits
each record as a `Logging.Entry`; then it issues **Clear Event Records** for
that batch.

Because the notification only fires on an **empty → non-empty** transition,
after clearing `ccid` issues **Get Event Records** again to confirm the log is
empty and thereby **re-arm** the trigger. If records remain (new events
arrived while it was processing), it repeats the process/clear until the log
is empty — otherwise those residual events would never re-trigger a
notification. (Trigger conditions and the confirm-empty re-arm step are
defined in CXL r3.1 §8.2.9.2.8, Event Notification, Opcode 0106h.)

```text
+-----------+      +-----------+      +-----------------------------+      +----------------+
|CXL Type 3 |      |kernel     |      |ccid                         |      |phosphor-logging|
|device     |      |AF_MCTP    |      |(CCI codec + event logic)    |      | / Redfish      |
+-----+-----+      +-----+-----+      +--------------+--------------+      +--------+-------+
      |                  |                           |                             |
      | Event Notif.     |                           |                             |
      | (0106h)          |                           |                             |
      +----------------->+-------------------------->+ decode notification         |
      |          Success (ack)                       |                             |
      +<-----------------+<--------------------------+                             |
      |                  | Get Event Records (0100h) |                             |
      |                  +<--------------------------+ encode + send               |
      +<-----------------+                           |                             |
      | event records    |                           |                             |
      +----------------->+-------------------------->+ decode + translate to log    |
      |                  |                           +---------------------------->+
      |                  |                           | lg2::commit() -> Logging.Entry
      |                  | Clear Event Records (0101h)|                            |
      |                  +<--------------------------+                             |
      +<-----------------+                           |                             |
      |                  | Get Event Records (0100h) | confirm empty -> re-arm     |
      |                  +<--------------------------+                             |
      +<-----------------+                           |                             |
      | records: 0 = done, >0 = repeat process+clear |                             |
      +----------------->+-------------------------->+                             |
      |                  |                           |                             |
```

### D-Bus surface

`ccid` exposes only semantic objects, never raw CCI. Two things are
published; both are **read-only** reflections of device state (the input
policy stays in config — see Configuration; runtime-writable policy over
D-Bus is deferred).

**1. CXL events → the BMC event log.** Each event surfaces as a standard
`xyz.openbmc_project.Logging.Entry`, so Redfish (`bmcweb`) and CLI read CXL
events with no CCI/MCTP knowledge. This needs new CXL event error definitions
in `phosphor-dbus-interfaces`, one per event record type (General Media, DRAM,
Memory Module, Memory Sparing, Physical Switch, Virtual Switch, MLD Port,
Dynamic Capacity) plus an `Unknown` fallback.

Because events are committed as standard `Logging.Entry` objects, they surface
through the existing phosphor-logging → `bmcweb` path as Redfish `LogEntry`
resources under the system event log; no ccid-specific Redfish code is added.
Whether these CXL events warrant dedicated Redfish message-registry entries
(structured messages) rather than generic log entries is left to follow-up
with the `bmcweb` maintainers.

**2. CXL device object** (one per device). `ccid` defines its **own** D-Bus
interface for this, with read-only properties:

- identity — vendor / serial / component type (from Identify);
- effective event interrupt policy (read back at init);
- device timestamp (last synced at init).

## Alternatives Considered

- **Use `libcxlmi` as the CCI codec instead of writing our own.** `libcxlmi`
  ([github.com/computexpresslink/libcxlmi](https://github.com/computexpresslink/libcxlmi))
  already implements CCI encode/decode for these command sets, and already
  supports this platform's Code Construct `mctpd` via a build option, so it
  would save implementation effort. Trade-offs of taking it as a dependency: it is
  **LGPL-2.1-or-later** while OpenBMC is Apache-2.0, so it would have to be
  dynamically linked and kept at arm's length to avoid pulling LGPL
  obligations into the codebase; and it adds an external dependency whose
  release cadence and API we do not control.
- **CCI as a shared library that each caller links and sends CCI itself, with
  no daemon in front.** This is exactly what the Problem Description
  argues against — there is no central point to serialize traffic during a
  firmware-update session, CCI-level tag collisions become every caller's
  problem, and raw CCI protocol details leak into the rest of the BMC.
- **Add CCI handling to the existing MCTP daemon (`mctpd`).** `mctpd` is
  deliberately protocol-agnostic — it implements MCTP control (EID assignment,
  routing) only. Folding an application-layer protocol into it grows its scope
  and couples its release cadence to unrelated feature work.
- **Add CCI handling to `pldmd`.** Not every CXL device supports both
  PLDM and CCI — some expose only CCI — so tying CCI handling to the PLDM
  daemon would leave CCI-only devices unserved and couple two independent
  protocols in one process. `pldmd` remains a good *architectural* reference
  (event loop, handler dispatch, D-Bus patterns); this is about not sharing
  its process.
- **Add CCI encode/decode to `libpldm`.** `libpldm` is scoped and named
  for PLDM specifically; CXL CCI codec logic does not belong there.

## Impacts

### Organizational

- Does this repository require a new repository? Yes.
- Initial maintainer(s): TBD — a TOF-eligible member will need to act as
  proxy maintainer initially.
- Repositories expected to be modified: `phosphor-dbus-interfaces` (new CXL
  event error/metadata YAML); `entity-manager` (e.g., a new `CXLCCIDevice`
  schema under `schemas/`, plus the per-board config entry).

## Testing

### Unit tests (googletest)

Run in CI, no hardware needed:

- CCI codec: encode/decode round-trips for the in-scope opcodes against
  known-good byte vectors from the CXL spec.
- Record translation: feed known-good event-record byte vectors (one per
  record type — General Media, DRAM, Memory Module, …) and assert the
  resulting `Logging.Entry` has the expected severity and metadata fields.
- Notification handling: assert an incoming Event Notification triggers
  exactly one Get Event Records to the right EID.
- Interrupt policy: assert Get/Set policy encode/decode round-trips correctly.
- Timeout path: assert an unanswered CCI request times out cleanly instead of
  hanging.

### End-to-end test (openbmc-test-automation)

Trigger a CXL event on a CXL Type 3 device, confirm it appears as a Redfish
LogEntry, and confirm it persists across a BMC reboot.
