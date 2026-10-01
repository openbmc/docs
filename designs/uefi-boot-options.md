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

- [UEFI 2.10](https://uefi.org/specs/UEFI/2.10/03_Boot_Manager.html) section
  3.1.1: each boot option is a `Boot####` variable, where `####` is four
  upper-case hex digits (0000 to FFFF). Section 3.3: `BootOrder` is an array of
  these numbers (`UINT16`). Firmware and the OS add and remove options over
  time.
- Each option also has a text description. Similar options can repeat, for
  example one network boot option per network port, with descriptions that
  differ only slightly.
- [Remote BIOS configuration design](https://github.com/openbmc/docs/blob/master/designs/remote-bios-configuration.md):
  the existing OpenBMC design for BIOS settings. The host reports its settings
  to the BMC, a client stores a change on the BMC as pending, and the host
  applies it on its next boot. This design uses the same flow for boot options.

## Requirements

- The host reports all boot options and the current order on every boot.
- A client can stage a new order, including options that are disabled or not in
  the current order, and can enable or disable an option.
- Clients request changes by writing the pending values (`PendingBootOrder`,
  `PendingEnabled`). Only the host changes the reported values (`BootOrder`,
  `Enabled` and the others), when it applies a request on the next boot.
- The BMC rejects invalid entries.

## Proposed Design

### Overview

There are three parts:

- The host firmware owns the boot options. On every boot it sends bmcweb the
  list of its options and its current order.
- bmcweb is the only way in. It checks that the request comes from the host and
  passes the list to bios-settings-mgr. For this we add a new D-Bus method,
  `SetBootOptions`, which takes the whole list of options and the current order
  in one call. Other clients can only send requests for the next boot.
- bios-settings-mgr stores the list as D-Bus objects. The stored values are
  read-only, so `SetBootOptions` is the only way they are updated. It also
  stores and checks the requests from clients.

On the next boot the host reads the stored requests, applies them and sends its
new list, which starts the cycle again.

### How options are identified

Firmware can report several options that look almost the same. For example, a
system with two network ports may report:

```text
Boot0004  network boot, port 1
Boot0005  network boot, port 2
```

- By type, both are "network". The list of types (disk, network, ...) is fixed,
  but one type can have many options, so an enum of types cannot pick out one
  option.
- By description, they differ only slightly, and descriptions are not guaranteed
  to be unique.
- By number, they are different: `Boot0004` and `Boot0005`. Each option has its
  own number.

So the `Boot####` number is used to identify an option:

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

Each boot option the host reports becomes one `BootOption` object. It holds what
firmware reported for that option: `Enabled` (whether the option is active),
`Description` and `DisplayName` (its text), and `UEFIDevicePath` (the device it
boots from). `BootOrder` lists the option numbers in the order the host tries
them.

For example, the host reports a disk and the two network options from above, and
its current order:

```text
boot_option/Boot0001   "disk"                   Enabled = true
boot_option/Boot0004   "network boot, port 1"   Enabled = true
boot_option/Boot0005   "network boot, port 2"   Enabled = false
boot_order             BootOrder        = [0x0001, 0x0004]
                       PendingBootOrder = []   (nothing pending)
```

A client that wants to boot from port 2 first writes:

```text
boot_option/Boot0005   PendingEnabled   = true
boot_order             PendingBootOrder = [0x0005, 0x0001]
```

- `0x0005` is accepted even though it is not in `BootOrder`, because the object
  `Boot0005` exists.
- `Boot0004` (port 1) is not in the new order, so firmware will not try it on
  the next boot (UEFI only tries options listed in `BootOrder`). It is not
  deleted: the host still reports it, so its object stays and a client can add
  it back later, for example `[0x0005, 0x0001, 0x0004]`.
- On the next boot the host applies the change and reports again. `BootOrder`
  becomes `[0x0005, 0x0001]`, `Boot0005` is enabled, and the pending values are
  cleared.

### SetBootOptions (new)

On every boot the host has to tell the BMC which boot options it has and what
its current order is. It cannot simply write `BootOrder` or the `BootOption`
values, because they are read-only. So we add a new D-Bus method for this,
`SetBootOptions`, on `Control.Boot.BootOrder`. It is the only way these
read-only values are updated.

How it is used:

1. The host sends its boot options and current order to bmcweb, as Redfish
   requests over the Redfish host interface.
2. bmcweb checks that the request really comes from the host, then calls
   `SetBootOptions` with every option (number, `Enabled`, `Description`,
   `DisplayName`, `UEFIDevicePath`) and the current order.
3. bios-settings-mgr makes its objects match what the host sent: it creates
   objects for new options, updates existing ones and deletes options that are
   no longer reported. It also sets `BootOrder` and clears the pending values.

### Flow

```text
 boot N        : host reports all options and the current order, and bmcweb
                 calls SetBootOptions(options, order)
                 -> bios-settings-mgr creates/updates/deletes BootOption
                    objects and sets BootOrder
 between boots : client writes PendingBootOrder or PendingEnabled
                 -> bios-settings-mgr checks and stores it; the host is not
                    changed yet
 boot N+1      : host reads the pending values, applies them and reports
                 again; bmcweb calls SetBootOptions -> pending values are
                 cleared
```

The existing
[boot source override](https://github.com/openbmc/docs/blob/master/host-management.md#host-boot-options)
(IPMI, or Redfish `BootSourceOverrideTarget`) is not changed. It selects only a
boot type (disk, network, ...), not a specific `Boot####` option. If it is set,
firmware uses it before `BootOrder`.

A requested change takes effect on the next boot (N+1). Nothing changes on the
host before it reboots.

Only the host may report values. bmcweb recognises the host by the bootstrap
account it uses on the Redfish host interface
([DSP0270](https://www.dmtf.org/sites/default/files/standards/documents/DSP0270_1.3.0.pdf))
and only then calls `SetBootOptions`. Other clients can only write the pending
values.

### bios-settings-mgr

bios-settings-mgr (83749) hosts both interfaces. It rejects a `PendingBootOrder`
write with `InvalidArgument` if an entry is not a reported option or appears
twice. It checks against all reported options, not only the current order, so an
option that is disabled or missing from the current order can be added back. It
drops deleted options from `PendingBootOrder` and stores everything to a file so
it survives a BMC reboot.

### Feedback wanted: type of BootOrder and PendingBootOrder

We propose `array[uint16]`, but we are not sure it is the best choice and would
value reviewers' views on these two options:

- `array[uint16]`, e.g. `[0x0005, 0x0001]`. This is the type UEFI uses for
  `BootOrder`, and D-Bus only accepts numbers. bmcweb turns `0x0005` into
  `"Boot0005"` for Redfish and back.
- `array[string]`, e.g. `["Boot0005", "Boot0001"]`. This is the form Redfish
  uses, so bmcweb passes it through unchanged. D-Bus accepts any text, so
  bios-settings-mgr has to reject anything that is not `Boot` followed by four
  upper-case hex digits.

With either type, bios-settings-mgr also rejects entries that were not reported
by the host or appear twice.

### Feedback wanted: ordering a new device

On every boot, firmware does roughly this:

```text
1. find the hardware
2. create a Boot#### entry for each new device and put it into BootOrder by
   its own rule (often at the end)
3. read the pending request from the BMC and apply it
4. send all options and the current order to bmcweb, which passes them to
   bios-settings-mgr with SetBootOptions
5. boot using BootOrder
```

So when a drive is added:

```text
 drive installed : no Boot#### entry yet, nothing to order
 boot N          : step 2 creates Boot0015; step 3 has no request for it,
                   because the number did not exist when the request was made;
                   step 4 reports Boot0015 to the BMC; it boots as before
 between boots   : client adds Boot0015 to PendingBootOrder
 boot N+1        : step 3 applies the request; it boots in the new order
```

A new drive therefore needs one boot to be reported and a second boot to use a
new order. Options that the OS creates (for example with `efibootmgr`) work the
same way, because the BMC sees them only at the next boot. We think this is
acceptable, but would welcome other views, for example whether the host should
also report changes while it is running.

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
  adds `SetBootOptions` and uses the property type agreed in review.
- bios-settings-mgr (83749) hosts and persists the objects.
- bmcweb adds the `BootOptions` resource and `Boot.BootOrder` support. Its host
  check depends on the bootstrap account changes
  ([bmcweb 82855](https://gerrit.openbmc.org/c/openbmc/bmcweb/+/82855)).

### Organizational

No new repository.

## Testing

Unit tests in bios-settings-mgr will cover `SetBootOptions` and the
`PendingBootOrder` validation.
