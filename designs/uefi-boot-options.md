# UEFI Boot Options and Boot Order

Author: Dhruv Goyal, Prithvi Pai

Created: September 30, 2026

## Problem Description

Operators want to see a host's UEFI boot options and change its boot order from
the BMC, for example to boot from a specific network port on the next boot.
OpenBMC has no D-Bus interface for this.

Only the host firmware knows which boot options exist, and the set changes as
devices are added or removed. So the host must report its options to the BMC,
the BMC must hold a requested change, and the host must apply it on the next
boot. The existing boot source override is not changed.

## Background and References

- [UEFI 2.10](https://uefi.org/specs/UEFI/2.10/03_Boot_Manager.html): each boot
  option is a `Boot####` variable (four hex digits), and `BootOrder` is an array
  of these numbers (`UINT16`).
- [Remote BIOS configuration](https://github.com/openbmc/docs/blob/master/designs/remote-bios-configuration.md)
  uses the same flow: host reports, client stores a pending change, host applies
  it on the next boot.
- The existing boot settings are in the
  [`Control.Boot`](https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/yaml/xyz/openbmc_project/Control/Boot)
  namespace. The boot source override selects a boot type, not a specific
  `Boot####` option.

## Requirements

- The host reports all boot options and its current order on every boot.
- A client can request a new boot order and enable or disable an option for the
  next boot.
- Clients can only request changes. Only the host changes the reported values.
- The BMC rejects an invalid order.
- Values survive a BMC reboot.

## Proposed Design

bios-settings-mgr hosts one D-Bus object per boot option and one boot order
object. The host's report reaches it through a new method, `SetBootOptions`,
which creates, updates and deletes `BootOption` objects to match the report,
sets `BootOrder` and clears pending values. The flow is:

```text
boot N        : host reports its options and current order; the receiving
                daemon calls SetBootOptions(options, order)
between boots : client writes PendingBootOrder or PendingEnabled;
                bios-settings-mgr checks and stores it
boot N+1      : host reads the pending values, applies them and reports
                again through SetBootOptions
```

### D-Bus interfaces

Both interfaces are added to the existing
[`Control.Boot`](https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/yaml/xyz/openbmc_project/Control/Boot)
namespace in phosphor-dbus-interfaces, next to the existing boot settings:
`BootOption` in
[78214](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/78214)
and `BootOrder` in
[81554](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/81554).

```text
/xyz/openbmc_project/control/host0/boot/option/<Boot####>   (one per option)
    xyz.openbmc_project.Control.Boot.BootOption
        Enabled, Description, DisplayName, UEFIDevicePath     read-only
        PendingEnabled                                        writable

/xyz/openbmc_project/control/host0/boot/order
    xyz.openbmc_project.Control.Boot.BootOrder
        BootOrder           array[uint16]   read-only
        PendingBootOrder    array[uint16]   writable
        SetBootOptions(options, order)
```

Each boot option the host reports becomes one `BootOption` object. It holds what
firmware reported for that option: `Enabled` (whether the option is active),
`Description` and `DisplayName` (its text), and `UEFIDevicePath` (the device it
boots from). `BootOrder` lists the option numbers in the order the host tries
them. Clients write only `PendingEnabled` and `PendingBootOrder`.

Options are identified by their `Boot####` number, because several options can
look alike (for example one network option per port). The order is a list of
these numbers, the same as UEFI `BootOrder`.

### Updates from the host

On every boot, the host reports its options and current order over its host
interface (Redfish host interface, PLDM or IPMI OEM). The receiving daemon calls
`SetBootOptions` with every option and the current order. The host reads the
pending values over the same interface. `SetBootOptions` is the only way to
change reported values. On the Redfish host interface, bmcweb accepts the host's
report only from the host's bootstrap account
([DSP0270](https://www.dmtf.org/sites/default/files/standards/documents/DSP0270_1.3.0.pdf)).

### Requests from clients

`PendingBootOrder` must include every entry of the current `BootOrder`, and may
also add reported options that are not in `BootOrder` yet (for example an entry
the OS created without adding it to the order). An order that leaves out a
current option, or contains an unknown or repeated option, is rejected with
`InvalidArgument`.

To stop using an option, a client sets its `PendingEnabled` to false instead of
removing it from the order.

For example, the host reports a disk and two network ports, with port 2
disabled:

```text
option/Boot0001   "disk"                   Enabled = true
option/Boot0004   "network boot, port 1"   Enabled = true
option/Boot0005   "network boot, port 2"   Enabled = false
order             BootOrder        = [0x0001, 0x0004, 0x0005]
                  PendingBootOrder = []
```

To boot from port 2 first, a client requests port 2 enabled and first in the
order:

```text
option/Boot0005   PendingEnabled   = true
order             PendingBootOrder = [0x0005, 0x0001, 0x0004]
```

On the next boot the host applies the request and reports again:

```text
option/Boot0005   Enabled = true
order             BootOrder        = [0x0005, 0x0001, 0x0004]
                  PendingBootOrder = []
```

### New and removed devices

The BMC cannot order a device the host has not reported. A new drive gets its
`Boot####` entry on the next boot, and firmware places it by its own rule (often
at the end). Once it is reported it is part of `BootOrder`, so a client's next
`PendingBootOrder` must include it and can place it anywhere. A removed device's
object is deleted and it is dropped from `PendingBootOrder`.

## Alternatives Considered

- Identify options by boot type or description: not unique.
- `array[string]` order (`["Boot0005", ...]`), as currently handled in
  bios-settings-mgr (83749): each entry needs text validation. `array[uint16]`
  matches UEFI.
- `array[object_path]` order: needs conversion on every report and request.

## Impacts

- New D-Bus interfaces `Control.Boot.BootOption` and `Control.Boot.BootOrder`.
  No existing API changes.
- The bmcweb bootstrap account check depends on
  [bmcweb 82855](https://gerrit.openbmc.org/c/openbmc/bmcweb/+/82855).

### Organizational

- phosphor-dbus-interfaces: 78214 and 81554 (see above), updated to match this
  design (`array[uint16]` order, added options and `SetBootOptions`).
- bios-settings-mgr: hosts and persists the objects
  ([83749](https://gerrit.openbmc.org/c/openbmc/bios-settings-mgr/+/83749)).
- bmcweb: Redfish `BootOptions` and `Boot.BootOrder` for clients (converting
  between Redfish `"Boot0005"` and D-Bus `0x0005`), and calls `SetBootOptions`
  when the host reports over the Redfish host interface.

## Testing

### Unit Test

- `SetBootOptions` creates, updates and deletes options, sets `BootOrder` and
  clears pending values.
- `PendingBootOrder` accepts a valid order and rejects one that leaves out a
  current option, or has an unknown or repeated option.
- Values are restored after bios-settings-mgr restarts.

### Integration Test

- Boot the host and confirm Redfish `BootOptions` and `Boot.BootOrder` match the
  host (`efibootmgr`).
- PATCH a new `BootOrder` and `BootOptionEnabled`, reboot the host, and confirm
  both are applied.
- Add a drive, reboot the host, and confirm it appears in `BootOptions`.
