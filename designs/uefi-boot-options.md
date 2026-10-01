# UEFI boot options and boot order over the Redfish host interface

Author: Dhruv Goyal <dhgoyal@nvidia.com>, Prithvi Pai <ppai@nvidia.com>

Other contributors: None

Created: 2026-09-30

## Problem Description

Operators want to view a host's UEFI boot options and change its boot order out
of band. OpenBMC has no D-Bus model for boot options and no Redfish `BootOption`
resource. The host firmware alone knows which `Boot####` load options exist; the
BMC can only store what the host reports and stage a request for the next boot.

## Background and References

- UEFI 2.10 section 3.1.1: load options are `Boot####` variables, `####` a
  hexadecimal number 0000-FFFF; `BootOrder` lists them. The set is dynamic.
- Redfish `BootOption` v1.0.6 and `ComputerSystem.Boot.BootOrder`.
- DSP0270, Redfish Host Interface.
- `designs/remote-bios-configuration.md`: publish on boot N, stage on the BMC,
  apply on boot N+1. This design uses the same pattern.
- `designs/bootstrap-account-access-restriction.md`: how bmcweb identifies a
  request from the host firmware.

## Requirements

- Host publishes its boot options and current order on every boot.
- Clients read them and stage a new order or enable/disable an option.
- Any published option can be staged, including disabled ones.
- Only the host can change what the host reported.
- The BMC stores references as reported and does not interpret them.

## Proposed Design

### D-Bus model (phosphor-dbus-interfaces, hosted by bios-settings-mgr)

```
/xyz/openbmc_project/bios_config/boot_order
    xyz.openbmc_project.Control.Boot.BootOrder
        BootOrder         array[string]  host-written
        PendingBootOrder  array[string]  client-written
        SetBootOptions(a(ssbs))          host-called: reference, display
                                         name, enabled, device path

/xyz/openbmc_project/bios_config/boot_option/<reference>   (one per option)
    xyz.openbmc_project.Control.Boot.BootOption
        Enabled, Description, DisplayName, UEFIDevicePath   host-written
        PendingEnabled                                      client-written
```

One object per option because each option has its own metadata and pending
state, maps one to one onto a Redfish `BootOption` resource, and is removable
with `Object.Delete`.

`SetBootOptions` replaces the whole set: create new, update existing, delete
unreported. Nothing else creates a `BootOption` object.

### Validation

References are firmware-defined strings, not an enum: the set is open and
firmware has been seen to append a description ("Boot0014: Ubuntu"). Host
reports are stored as given.

Client writes are checked against the published set:

- Each `PendingBootOrder` entry must name an existing `BootOption` object and
  appear once; otherwise `InvalidArgument`.
- The current `BootOrder` is not the reference set, so a disabled or omitted
  option can be re-added.
- `PendingBootOrder` cannot be set while no options are published.

### Who may write what

D-Bus has no caller identity, so bmcweb enforces the split. A request from a
bootstrap account on the host-interface socket is a host report and goes to the
reported properties; any other request goes to the pending properties. A client
PATCH of a reported property returns `PropertyNotWritable`.

### Redfish mapping

| Redfish (under `Systems/<id>`)          | Writer | D-Bus              |
| --------------------------------------- | ------ | ------------------ |
| `Boot/BootOrder`, GET                   | -      | `BootOrder`        |
| `Boot/BootOrder`, PATCH                 | host   | `BootOrder`        |
| `Boot/BootOrder`, PATCH                 | client | `PendingBootOrder` |
| `BootOptions`, PUT                      | host   | `SetBootOptions`   |
| `BootOptions/<ref>`, GET                | -      | `BootOption`       |
| `BootOptions/<ref>` `BootOptionEnabled` | client | `PendingEnabled`   |

### Flow

```
 Host (RHI)                    BMC                          Client
 boot N:   PUT BootOptions --> SetBootOptions
           PATCH BootOrder --> BootOrder
                               PendingBootOrder  <-- PATCH Boot/BootOrder
                               PendingEnabled    <-- PATCH BootOptions/<ref>
 boot N+1: GET pending     <-- returned
           apply, then
           PUT/PATCH report --> reported updated, pending cleared
```

Pending values persist until the host reports a new state.

## Alternatives Considered

- One array-of-struct property: no per-option pending state, no Redfish
  collection mapping.
- Boot order as a `BaseBIOSTable` attribute: cannot carry per-option metadata;
  Redfish already defines `BootOption`.
- Enum references: the set is open (UEFI 2.10 section 3.1.1).
- Validate pending order against current `BootOrder`: disabled options could
  never be re-added.

## Impacts

- phosphor-dbus-interfaces: 78214 (`BootOption`), 81554 (`BootOrder`).
- bios-settings-mgr: 83749 hosts the objects and persists state.
- bmcweb: `BootOptions` collection and host/client split; depends on the
  bootstrap account series.
- Security: reported state is writable only over the host interface.
- No existing API changes.

### Organizational

No new repository. Reviewers: maintainers of the three repositories above.

## Testing

- bios-settings-mgr unit tests: `SetBootOptions` create/update/delete;
  `PendingBootOrder` rejects unknown and duplicate references.
- Redfish Service Validator on `BootOptions` and `ComputerSystem.Boot`.
- On a UEFI host: firmware publishes options; client stages a disabled network
  option first; next boot reports the new order and clears pending; client PATCH
  of `Boot/BootOrder` returns `PropertyNotWritable`; unknown reference
  returns 400.
