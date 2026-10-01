# UEFI boot options and boot order over the Redfish host interface

Author: Dhruv Goyal <dhgoyal@nvidia.com>, Prithvi Pai <ppai@nvidia.com>

Other contributors: None

Created: 2026-09-30

## Problem Description

Operators want to see a host's UEFI boot options and change the boot order from
the BMC. OpenBMC has no D-Bus interface for this.

The BMC cannot find the boot options itself. Only the host firmware knows them.
So the host has to report them to the BMC, the BMC stores a requested change,
and the host applies it on its next boot.

## Background and References

- UEFI 2.10 section 3.1.1: each boot option is a `Boot####` variable (`####` is
  a hex number from 0000 to FFFF). `BootOrder` is an array of these numbers
  (`UINT16`). Firmware and the OS add and remove options over time.
- Each option also has a text description. Similar options can repeat, for
  example one network boot option per network port, with descriptions that
  differ only slightly.
- `designs/remote-bios-configuration.md` uses the same report, stage, apply flow
  for BIOS settings.

## Requirements

- The host reports all boot options and the current order on every boot.
- A client can stage a new order, including options that are disabled or not in
  the current order, and can enable or disable an option.
- The BMC rejects invalid entries, and only the host can change reported values.

## Proposed Design

### How options are identified

Since options repeat and their descriptions are not unique, the only reliable
way to identify an option is its `Boot####` number. An enum of types (disk,
network, ...) cannot work either, because the set changes and one type can have
many options. So:

- each reported option becomes one D-Bus object, named by its number;
- the boot order is a list of those numbers.

### D-Bus objects

```text
/xyz/openbmc_project/bios_config/boot_option/<Boot####>   (one per option)
    Control.Boot.BootOption                                (78214)
        Enabled, Description, DisplayName, UEFIDevicePath   read-only
        PendingEnabled                                      writable

/xyz/openbmc_project/bios_config/boot_order
    Control.Boot.BootOrder                                 (81554)
        BootOrder           array[uint16]   read-only
        PendingBootOrder    array[uint16]   writable
        SetBootOptions(options, order)
```

- Each boot option the host reports is stored as its own D-Bus object, for
  example `Boot0014` at `/xyz/openbmc_project/bios_config/boot_option/Boot0014`.
  It holds the values firmware reported for that option:

  | Property         | Value reported by firmware                       |
  | ---------------- | ------------------------------------------------ |
  | `Enabled`        | Whether the option is active                     |
  | `Description`    | The option's text description                    |
  | `DisplayName`    | A short name to show to users                    |
  | `UEFIDevicePath` | The UEFI device path of the device it boots from |

  `PendingEnabled` is set by a client to enable or disable the option on the
  next boot.

- `BootOrder` is the order the host uses now, as a list of option numbers. For
  example `[0x0014, 0x000E]` means try `Boot0014` first, then `Boot000E`. Number
  `0x0014` refers to the object `boot_option/Boot0014`.
- `PendingBootOrder` has the same form and holds the order a client wants for
  the next boot.
- `SetBootOptions` is how the host reports all its options and its current
  order.

Read-only values come only from the host. Clients write only the `Pending`
values. `array[uint16]` is what we propose; see open question 1.

### Flow

```text
 boot N   : host calls SetBootOptions(all options, current order)
            -> bios-settings-mgr creates/updates/deletes BootOption objects
               and sets BootOrder
 any time : client writes PendingBootOrder or PendingEnabled
            -> bios-settings-mgr checks and stores it
 boot N+1 : host reads the pending values, applies them, calls
            SetBootOptions again -> pending values are cleared
```

bmcweb only passes requests from the host, over the Redfish host interface, to
`SetBootOptions`.

### bios-settings-mgr

bios-settings-mgr (83749) hosts both interfaces. It rejects a `PendingBootOrder`
write with `InvalidArgument` if an entry is not a reported option or appears
twice. It checks against all reported options, not only the current order, so a
disabled option can be added back. It drops deleted options from
`PendingBootOrder` and stores everything to a file so it survives a BMC reboot.

### Open question 1: type of BootOrder and PendingBootOrder

- `array[uint16]` (proposed), e.g. `[0x0014, 0x000E]`: the same type UEFI uses
  for `BootOrder`, and only numbers can be written.
- `array[string]`, e.g. `["Boot0014", "Boot000E"]`: the Redfish form, no
  conversion in bmcweb. D-Bus accepts any text, so all checking is left to
  bios-settings-mgr.

### Open question 2: new devices before a reboot

The BMC only knows what the host has reported, so a new drive works like this:

```text
 drive added, no reboot : no Boot#### yet, cannot be ordered
 boot N                 : firmware creates Boot0015 and reports it
 after boot N           : client adds Boot0015 to PendingBootOrder
 boot N+1               : firmware applies the new order
```

So a client can order a new drive only after one boot, and that order takes
effect on the boot after. The same applies to options the OS adds with
`efibootmgr`. Is this acceptable, or should the host also report changes at run
time?

## Alternatives Considered

- Identify an option by an enum of boot types (disk, network, ...) or by its
  description. Several options can have the same type or a similar description,
  for example the network boot options above, so neither can pick out one
  option.
- `array[object_path]`: works, but does not match the UEFI type.
- Checking pending entries against the current `BootOrder`: a disabled option
  could never be added back.

## Impacts

- phosphor-dbus-interfaces: 78214 marks reported properties read-only. 81554
  adds `SetBootOptions` and uses the type from open question 1.
- bios-settings-mgr (83749) hosts and persists the objects.
- bmcweb adds the `BootOptions` resource and `Boot.BootOrder` support.

### Organizational

No new repository.

## Testing

- Unit tests for `SetBootOptions` and `PendingBootOrder` validation.
- On a UEFI system: repeated network options, re-enabling a disabled option, and
  a new NVMe drive after reboot.
