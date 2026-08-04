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
devices — their events, identity and time — and surface that alongside
everything else it manages.

CCI for CXL Type 3 devices is transported over MCTP, as defined by the CXL Type
3 Device CCI over MCTP Binding Specification (DSP0281). But today there is no
OpenBMC service that sends CCI commands to CXL Type 3 devices over MCTP.

That service should be a single daemon, not a library each caller links
directly, because:

- Several BMC services may want to talk to CXL devices. A single daemon gives
  one coordinated owner of each device, instead of several uncoordinated
  callers interleaving commands.
- Event records must be read and cleared by exactly one owner, or two readers
  would log the same record twice or clear records the other has not logged.
- A later CXL firmware update over CCI needs one place that can hold off other
  CCI traffic while the update runs.

So this design employs a single daemon, **`ccid`**, to send and receive CXL Type
3 Device CCI messages over MCTP on behalf of every other BMC service.

This document scopes the daemon to device events, identity and time (see
Requirements). Firmware update and the other command categories are left for
later.

## Background and References

Compute Express Link is a dynamic multi-protocol technology designed to support
accelerators and memory devices. CXL provides a rich set of protocols that
include I/O semantics similar to PCIe (CXL.io), caching protocol semantics
(CXL.cache), and memory access semantics (CXL.mem) over a discrete or on-package
link.

CXL Type 3 devices are devices that support CXL.io and CXL.mem protocols. These
devices can be managed during runtime via a Component Command Interface (CCI)
that represents a command target for management and configuration commands. The
CCI and the commands supported via the CCI are defined by the CXL Consortium.

CCI commands are sent to CXL Type 3 devices over MCTP, which is already
supported via the kernel-native `AF_MCTP` stack.

Each MCTP message carries a Message Type as the first byte of the message data,
defined in the MCTP IDs and Codes Specification (DSP0239). This value indicates
the protocol of the message contents — e.g. `0x01` for PLDM, `0x04` for NVMe-MI.
For CXL Type 3 CCI over MCTP messages, the assigned type value is **`0x08`**.

DSP0281 defines the MCTP transport binding; the CCI commands and event record
formats come from the CXL specification. This design targets **CXL Specification
Revision 3.1** for the CCI/event content.

[libcxlmi](https://github.com/computexpresslink/libcxlmi) is the CXL
Consortium's CXL Management Interface library. It encodes CCI commands, sends
them over `AF_MCTP` and checks the responses.

References:

- DMTF DSP0281 —
  [CXL Type 3 Device CCI over MCTP Binding Specification 1.0.0](https://www.dmtf.org/sites/default/files/standards/documents/DSP0281_1.0.0_0.pdf)
- DMTF DSP0236 —
  [MCTP Base Specification 1.3](https://www.dmtf.org/sites/default/files/standards/documents/DSP0236_1.3.0.pdf)
- DMTF DSP0239 —
  [MCTP IDs and Codes 1.9](https://www.dmtf.org/sites/default/files/standards/documents/DSP0239_1.9.pdf)
- CXL Consortium —
  [Compute Express Link Specification, Revision 3.1](https://computeexpresslink.org/wp-content/uploads/2024/02/CXL-3.1-Specification.pdf)

## Requirements

**Scope.** `ccid` uses these Generic Component Commands (CXL r3.1 Table 8-37):

**Events (Command Set `01h`)**

| Opcode  | Command                         |
| ------- | ------------------------------- |
| `0100h` | Get Event Records               |
| `0101h` | Clear Event Records             |
| `0104h` | Get MCTP Event Interrupt Policy |
| `0105h` | Set MCTP Event Interrupt Policy |
| `0106h` | Event Notification              |

**Information and Status (Command Set `00h`)**

| Opcode  | Command  |
| ------- | -------- |
| `0001h` | Identify |

**Timestamp (Command Set `03h`)**

| Opcode  | Command       |
| ------- | ------------- |
| `0300h` | Get Timestamp |
| `0301h` | Set Timestamp |

Functional:

- **Events**: learn about new event records, either from the device's Event
  Notification or by polling, as configured per device. Read the records with
  Get Event Records, commit each as a phosphor-logging event, and only then
  clear them with Clear Event Records, so no record is lost.
- **Identify**: check that the endpoint answers CCI before configuring it.
- **Timestamp**: set the device clock to BMC time and verify it, so event record
  timestamps are meaningful.
- **Recovery**: notice a device that stops answering (reset, power loss) and set
  it up again when it comes back.

A companion command-line tool, `ccitool`, sends single CCI commands for bring-up
and debug without the daemon, including Get Supported Logs (`0400h`), Get Log
(`0401h`) and Get Health Info (`4200h`).

Out of scope: Firmware Update, Features, Maintenance and PBR Components, the
other Information and Status and Logs commands in the daemon, and the Dynamic
Capacity Event Log (see Event handling).

## Proposed Design

A single event-driven `ccid` daemon. It is both a requester (it sends CCI
commands) and a responder (it receives and acknowledges Event Notifications).

Layering inside the BMC:

```text
  Legend:   |  MCTP data path        :  D-Bus

  +------------------------------------+       +---------------------------+
  | Redfish / bmcweb · CLI             |       | entity-manager            |
  | (event log)                        |       | (CCIDevice record)        |
  +----+-------------------------------+       +-------------+-------------+
       : reads Logging.Entry                                 : consumed by
  +----+-------------+                                  +----v------------+
  | phosphor-logging |                                  | MCTPReactor     |
  +----+-------------+                                  | (configured_by) |
       : lg2::commit()                                  +----+------------+
  +----+-------------------------------+                     : drives
  | ccid                               |                +----v------------+
  |   discovery, init, event modes     |:·· endpoints ··| mctpd           |
  |   event record decode              |:·· Recover() ··| (EID, recovery) |
  |   Event Notification receive + ACK |                +-----------------+
  |   libcxlmi (one worker per device) |
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
```

entity-manager, `MCTPReactor`, `mctpd` and the kernel MCTP stack already exist.
`ccid` reads endpoints from `mctpd` over D-Bus and never assigns EIDs itself.
What this design adds:

- **`ccid`** — a new daemon (new repository).
- **libcxlmi** — used as the CCI codec and requester; a recipe is added to
  OpenBMC.
- **Event record decode** — libcxlmi returns event records but does not decode
  them, so `ccid` decodes them.
- **Event Notification receive** — libcxlmi only sends requests, so `ccid` owns
  an `AF_MCTP` socket that receives the device's Event Notifications and
  acknowledges them.
- **phosphor-dbus-interfaces** — CXL CCI events, types and an
  `EventNotification` signal.
- **entity-manager** — a `CCIDevice` exposes record that holds each device's
  event settings.

### Command categories at a glance

| Category                       | When          | Role in `ccid`                                              |
| ------------------------------ | ------------- | ----------------------------------------------------------- |
| Information and Status (`00h`) | init          | Identify: check that the endpoint answers CCI               |
| Timestamp (`03h`)              | init          | Set the device clock to BMC time and verify it              |
| Events (`01h`)                 | init, runtime | Set the interrupt policy; read, log and clear event records |

### CCI requests

`ccid` sends every CCI command with libcxlmi. A libcxlmi call blocks until the
response arrives or its timeout (2 s) expires, so each device has a worker
thread that runs that device's commands in order. The result is handed back to
the event loop, where the waiting coroutine resumes. A slow or unreachable
device delays only its own commands; Event Notifications, other devices and
D-Bus are still served.

libcxlmi is LGPL-2.1-or-later. `ccid` links it dynamically and uses only its
public API.

### Transport layer

libcxlmi opens its own `AF_MCTP` socket per endpoint for the requests it sends.
`ccid` opens one more `AF_MCTP` socket bound to Message Type `0x08` on any
network, which receives the requests devices send — Event Notifications — and
sends their responses with the request's tag. The kernel MCTP stack handles
multi-packet segmentation and reassembly.

The device retransmits an unanswered Event Notification, so `ccid` acknowledges
it at once, before reading any record.

### Device discovery

`ccid` does not hardcode EIDs. It subscribes to `mctpd`'s (bus name
`au.com.codeconstruct.MCTP1`) `InterfacesAdded`, `InterfacesRemoved` and
endpoint `PropertiesChanged` signals first, and then scans the endpoints that
already exist, so an endpoint added during the scan is not missed. An endpoint
is a CCI endpoint when its `xyz.openbmc_project.MCTP.Endpoint`
`SupportedMessageTypes` includes `0x08`.

`MCTPReactor` sets up the endpoints from entity-manager configuration and
associates each one (`configured_by`) with the entity-manager record it came
from. `ccid` follows that association to the device's `CCIDevice` record, so
it knows which settings belong to which device without its own matching. The
inventory object also becomes the `Source` of the device's events.

On `InterfacesRemoved`, `ccid` drops that EID and its state.

### Recovery

A CXL device that resets, or loses power with the host, usually keeps its
`mctpd` endpoint, so no D-Bus signal says that it lost its timestamp and
interrupt policy. `ccid` follows the MCTP recovery flow that `mctpd` and
`MCTPReactor` implement:

- When a CCI command times out, `ccid` calls the endpoint's
  `au.com.codeconstruct.MCTP.Endpoint1.Recover()` (at most once a minute per
  endpoint) and stops polling it. `mctpd` marks the endpoint `Degraded` and
  probes it; it either becomes `Available` again or is removed.
- When `Connectivity` returns to `Available`, `ccid` initialises the device
  again. A removed endpoint is set up again by `MCTPReactor` and found through
  `InterfacesAdded`.

### Configuration

Each device's event settings live in a `CCIDevice` exposes record in
entity-manager, next to the device's MCTP target record:

```json
{
  "EventMode": "Polling",
  "Name": "CXL",
  "PollingIntervalSec": 120,
  "Type": "CCIDevice"
}
```

| Property                   | Used by                              | Meaning                                                                                 |
| -------------------------- | ------------------------------------ | --------------------------------------------------------------------------------------- |
| `EventMode`                | —                                    | `Notification` or `Polling`; not set turns event handling off                           |
| `MCTPEventInterruptPolicy` | `Notification` (required)            | Event Interrupt Settings (CXL r3.1 Table 8-57); bits 0-3 select the logs                |
| `PollingIntervalSec`       | `Polling` (required), `Notification` | Seconds between polls; with `Notification` only the fallback if the policy is not taken |

`ccid` reads the record each time it initialises the device. A record that
misses the property its mode needs, sets a policy in `Polling` mode, or holds an
invalid value (such as a reserved policy bit) is logged and its event handling
is turned off. The schema is added to entity-manager as
[CCIDevice](https://gerrit.openbmc.org/c/openbmc/entity-manager/+/97059).

### Event modes

| Mode                                  | MCTP Event Interrupt Policy | How `ccid` learns about records                                           |
| ------------------------------------- | --------------------------- | ------------------------------------------------------------------------- |
| Not set                               | `0`                         | It does not; the logs are left alone                                      |
| `Notification`                        | `MCTPEventInterruptPolicy`  | Event Notification (`0106h`) from the device                              |
| `Polling`                             | `0`                         | Reads the Informational, Warning, Failure and Fatal logs every interval   |
| `Notification` + `PollingIntervalSec` | as `Notification`           | As `Notification`; polls instead if Set MCTP Event Interrupt Policy fails |

`Polling` serves firmware that does not send Event Notifications yet; an empty
log costs one Get Event Records per log and logs nothing. While event handling
is off, an Event Notification the device sends anyway is still acknowledged but
no log is read.

### Initialization flow

When a CCI endpoint is found, or comes back `Available`, `ccid` brings the
device to a known state:

1. **Identify** (`0001h`) — check that the endpoint answers CCI. A failure is
   logged and initialisation continues.
2. **Set Timestamp** (`0301h`) then **Get Timestamp** (`0300h`) — set the device
   clock to BMC time in nanoseconds and read it back. An offset over 1 s is
   logged as a warning.
3. Read the device's `CCIDevice` record (see Configuration).
4. **Set MCTP Event Interrupt Policy** (`0105h`) — the configured logs in
   `Notification` mode, `0` otherwise. The Dynamic Capacity bit is never set.
5. **Get MCTP Event Interrupt Policy** (`0104h`) — read the applied policy back
   and log a warning if it differs.
6. Unless event handling is off, drain the event logs once, with the same
   pipeline as the runtime flow. Records added while `ccid` was not running
   raised no notification it saw.
7. In `Polling` mode, or after a fallback, start the device's poll timer.

```text
endpoint found / Available again
        │
        ▼
  Identify (0001h)
        │
        ▼
  Set Timestamp (0301h) ─▶ Get Timestamp (0300h) ── off by > 1 s? ──▶ warn
        │
        ▼
  read CCIDevice record ◀── entity-manager (via configured_by)
        │
        ▼
  Set MCTP Event Interrupt Policy (0105h)
        │      └─ fails: Notification + PollingIntervalSec ──▶ fall back to polling
        ▼
  Get MCTP Event Interrupt Policy (0104h) ── differs? ──▶ warn
        │
        ├── event handling off ──▶ done
        ▼
  drain the event logs once
        │
        ▼
  Notification: wait for 0106h      Polling: poll every PollingIntervalSec
```

`ccid` keeps no durable state. Committed events are stored by phosphor-logging
and survive a BMC reboot; after a restart `ccid` initialises each device again
and drains what accumulated while it was down.

### Event handling

A drain of a device reads each requested log (Informational, Warning, Failure,
Fatal):

1. **Get Event Records** (`0100h`).
2. Commit every returned record with `lg2::commit`.
3. **Clear Event Records** (`0101h`) for those records — only after all of them
   are committed, so a failed commit loses nothing.
4. Repeat while the device reports more records, at most 256 reads per log per
   drain, so a device whose Clear does not clear cannot keep the drain going.
5. Emit one `EventNotification` signal for the log if any record was cleared.

The Event Notification fires on an empty-to-non-empty transition (CXL r3.1
§8.2.9.2.8), so reading the log until it is empty also re-arms it.

At most one drain runs per device. A notification or poll that arrives while one
runs only adds its logs to the drain's queue, which runs right after, so two
drains never read and clear the same records.

`ccid` never reads or clears the **Dynamic Capacity Event Log**. Its records are
requests the host must answer (CXL r3.1 Table 8-50), and the BMC clearing them
could break the host's Dynamic Capacity flow.

`ccid` decodes General Media, DRAM, Memory Module, Memory Sparing and Dynamic
Capacity records (CXL r3.1 Tables 8-45 to 8-51) into a one-line summary and
named fields. Other record types are logged with their common header only. The
raw record is always kept.

```text
+-----------+      +-----------+      +-----------------------------+      +----------------+
|CXL Type 3 |      |kernel     |      |ccid                         |      |phosphor-logging|
|device     |      |AF_MCTP    |      |                             |      | / Redfish      |
+-----+-----+      +-----+-----+      +--------------+--------------+      +--------+-------+
      |                  |                           |                             |
      | Event Notif.     |                           |                             |
      | (0106h)          |                           |                             |
      +----------------->+-------------------------->+ ACK at once                 |
      +<-----------------+<--------------------------+                             |
      |                  | Get Event Records (0100h) |                             |
      +<-----------------+<--------------------------+ (libcxlmi, device worker)   |
      | event records    |                           |                             |
      +----------------->+-------------------------->+ decode, lg2::commit()       |
      |                  |                           +---------------------------->+
      |                  | Clear Event Records (0101h)                             |
      +<-----------------+<--------------------------+                             |
      |                  | ... repeat until the log is empty ...                   |
      |                  |                           | EventNotification signal    |
```

### D-Bus surface

`ccid` exposes only semantic D-Bus objects, never raw CCI.

**1. CXL events in the BMC event log.** `phosphor-dbus-interfaces` gains
`xyz.openbmc_project.CXL.CCI` events, one per CXL Event Record Severity:

| CXL event severity | Event                                       | Severity      |
| ------------------ | ------------------------------------------- | ------------- |
| Informational      | `xyz.openbmc_project.CXL.CCI.Informational` | informational |
| Warning            | `xyz.openbmc_project.CXL.CCI.Warning`       | warning       |
| Failure            | `xyz.openbmc_project.CXL.CCI.Failure`       | error         |
| Fatal              | `xyz.openbmc_project.CXL.CCI.Fatal`         | critical      |

Each event carries `Source` (the device's inventory object), `RecordType` and
`Summary` as its metadata, and the decoded fields, the device's Event Record
Timestamp and the raw record as additional data.
`xyz.openbmc_project.CXL.CCI.Types` holds the enumerations (event log, severity,
record type, decoded field values). The events surface through the existing
phosphor-logging → `bmcweb` path as Redfish `LogEntry` resources; no
`ccid`-specific Redfish code is added.

**2. `EventNotification` signal.** `ccid` implements
`xyz.openbmc_project.CXL.CCI.Event` on `/xyz/openbmc_project/cxl/cci`. Its
`EventNotification(EID, EventLog)` signal is emitted once per drained log, so a
service that reacts to CXL events (such as a health monitor) does not have to
poll the event log.

A read-only CXL device object (identity from Identify, effective policy, device
timestamp) is left as follow-up work.

## Alternatives Considered

- **Write the CCI encode/decode in `ccid` instead of using libcxlmi.** It would
  avoid an LGPL-2.1-or-later dependency whose release cadence and API we do not
  control. But libcxlmi already implements the CCI commands, response checks and
  the CodeConstruct `mctpd` integration, and it is the CXL Consortium's library,
  so fixes land in one place shared with other users. `ccid` links it
  dynamically and adds only what it lacks: event record decode and receiving
  Event Notifications.
- **CCI as a shared library that each caller links and sends CCI itself, with no
  daemon in front.** This is what the Problem Description argues against: no
  single owner of a device's event logs, and no central point to hold off
  traffic during a firmware update.
- **Add CCI handling to the existing MCTP daemon (`mctpd`).** `mctpd` is
  deliberately protocol-agnostic — it implements MCTP control (EID assignment,
  routing) only. Folding an application-layer protocol into it grows its scope
  and couples its release cadence to unrelated feature work.
- **Add CCI handling to `pldmd`.** Not every CXL device supports both PLDM and
  CCI — some expose only CCI — so tying CCI handling to the PLDM daemon would
  leave CCI-only devices unserved and couple two independent protocols in one
  process. `pldmd` remains a good _architectural_ reference (event loop,
  requester handler, D-Bus patterns); this is about not sharing its process.
- **Add CCI encode/decode to `libpldm`.** `libpldm` is scoped and named for PLDM
  specifically; CXL CCI codec logic does not belong there.
- **One fixed event mode.** Notification-only leaves devices whose firmware does
  not send Event Notifications without events; polling-only reads every device
  on a timer even when it can notify. A per-device `EventMode` serves both.

## Impacts

### Organizational

- Does this repository require a new repository? Yes, for `ccid` and `ccitool`.
- Initial maintainer(s): TBD — a TOF-eligible member will need to act as proxy
  maintainer initially.
- Repositories expected to be modified:
  - `phosphor-dbus-interfaces`: `xyz.openbmc_project.CXL.CCI` events, `Types`
    and `Event` interfaces.
  - `entity-manager`: the `CCIDevice` schema
    ([97059](https://gerrit.openbmc.org/c/openbmc/entity-manager/+/97059)) and
    the per-board records.
  - `openbmc/openbmc`: a libcxlmi recipe
    ([97251](https://gerrit.openbmc.org/c/openbmc/openbmc/+/97251)).

## Testing

### Unit tests (googletest)

Run in CI with `meson test`, no hardware needed:

- Event record decode: known-good record byte vectors for each decoded record
  type (General Media, DRAM, Memory Module, Memory Sparing, Dynamic Capacity)
  checked against the CXL r3.1 table offsets, the summary and the fields; an
  unknown record type falls back to its header.
- Payload reader: little-endian widths, bounds and failure handling used by the
  decoders.
- libcxlmi glue: event record conversion and return code mapping.

### Hardware test

On a platform with a CXL Type 3 device (verified on Yosemite5):

- `ccitool` checks each command directly: Identify, timestamp, MCTP Event
  Interrupt Policy, Get Event Records, health and logs.
- `journalctl -u ccid` shows discovery, initialisation and the applied policy
  for each event mode.
- A CXL event appears as a Redfish `LogEntry` and persists across a BMC reboot.
- Resetting the device triggers recovery and initialisation again.
