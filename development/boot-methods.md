# OpenBMC Boot Methods

This document collects common methods developers use to boot OpenBMC images
for testing and debug.

## Overview

Use this guide when you want to boot an OpenBMC build without doing a full
flash cycle each time.

Boot workflows evolve over time across platforms and labs, so this document is
organized as a living reference for commonly used approaches.

There is no single best method for every setup. Choose the approach that
matches your board access, network environment, and test goals.

## Prerequisites

- OpenBMC build artifacts available under
  `build/tmp/deploy/images/<machine>/`
- Access to the BMC U-Boot console (for TFTP/NFS methods)
- BMC and host network configured so the BMC can reach your server

## Boot FIT Image via TFTP

Use this for quick kernel testing.

1. Copy and rename your FIT image on the TFTP server:

   ```bash
   cp build/tmp/deploy/images/<machine>/fitImage-obmc-phosphor-initramfs-<machine>.bin \
      /tftpboot/fitImage
   ```

2. Enter U-Boot and configure network:

   ```bash
   setenv ethaddr <mac:addr>
   setenv ipaddr <bmc-ip>
   setenv serverip <tftp-server-ip>
   ```

3. Fetch and boot:

   ```bash
   tftp 0x83000000 fitImage
   bootm 0x83000000
   ```

## Boot zImage and DTB with NFS Rootfs

In this workflow, U-Boot loads `zImage` and `dtb` from build artifacts
(typically via TFTP), then mounts the root filesystem from an NFS export.

1. Configure an NFS server and export path.

   Helpful references:
   - [Ubuntu NFS server guide](https://ubuntu.com/server/docs/network-file-system-nfs)
   - [Arch Linux NFS wiki](https://wiki.archlinux.org/title/NFS)

2. Extract the OpenBMC rootfs squashfs into the NFS export directory.

   Use the same export directory path that you configured in the NFS server
   setup step above.

   ```bash
   sudo mkdir -p <nfs-export-path>
   sudo unsquashfs -f -d <nfs-export-path> <path-to-rootfs-squashfs-xz>
   ```

3. Ensure the following kernel options are enabled:

   ```text
   CONFIG_NFS_FS=y
   CONFIG_NFS_V3=y

   CONFIG_ROOT_NFS=y

   CONFIG_IP_PNP=y
   CONFIG_IP_PNP_DHCP=y
   CONFIG_IP_PNP_BOOTP=y
   ```

4. Boot from U-Boot:

   ```bash
   ast# setenv serverip <nfs-or-tftp-server-ip>
   ast# setenv ipaddr <bmc-ip-or-use-dhcp>
   ast# tftp 0x80080000 <zimage-file>
   ast# tftp 0x83000000 <dtb-file>
   ast# setenv nfsroot_path <nfs-server-ip>:<nfs-export-path>
   setenv bootargs "console=<console>,<baud> root=/dev/nfs rw nfsroot=${nfsroot_path},v3 ip=dhcp rootwait"
   ast# bootz ${kernel_addr_r} - ${fdt_addr_r}
   ```

## Boot in QEMU

Use QEMU when you want a hardware-independent dev loop.

```bash
qemu-system-arm -m 256 -M <machine>-bmc -nographic \
    -drive file=<path>/obmc-phosphor-image-<machine>.static.mtd,format=raw,if=mtd \
    -net nic \
    -net user,hostfwd=:127.0.0.1:2222-:22,hostfwd=:127.0.0.1:2443-:443,hostname=qemu
```

## Troubleshooting

- Timeout while fetching image:
  verify `ipaddr`, `serverip`, subnet, and firewall rules.
- `Unknown command 'nfs'` in U-Boot:
  your U-Boot build may not enable NFS support.
- `Wrong Image Format` during `bootm`:
  confirm you are loading a valid FIT image for the target machine.
- Boot loops or early panic:
  verify image and device tree match your platform.
