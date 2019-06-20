# eMMC Storage Design

Author: Adriana Kobylak < anoo! >

Primary assignee: Adriana Kobylak

Other contributors: Joel Stanley < shenki! >

Created: 2019-06-20

## Problem Description
Proposal to define the storage design for an eMMC device. This includes
filesystem type, partitioning, volume management, boot options and
initialization, etc.

## Background and References
OpenBMC currently supports raw flash such as the SPI NOR found in the systems
based on AST2400 and AST2500, but there is no design for managed NAND.

## Requirements
- Security: Ability to enforce read-only, verfication of official/signed
  images for production.

- Updatable: Ensure that the filesystem design allows for an effective and
  simple update mechanism to be implemented.

- Simplicity: Make the system easy to understand, so that it is easy to
  develop, test, use, and recover.

- Code reuse: Try to use something that already exists instead of re-inventing
  the wheel.

## Proposed Design
- Store U-Boot and the Linux kernel in a separate SPI NOR flash device, since
  SOCs such as the AST2500 do not support executing U-Boot from an eMMC. In
  addition, having the Linux kernel on the NOR saves from requiring U-Boot
  support for the eMMC. The U-Boot and kernel are less than 10MB in size, so a
  fairly small chip such as a 32MB one would suffice. A second NOR device could
  be added as redundancy in case U-Boot or the kernel failed to run.

- Filesystem: ext4. This is a stable and widely used filesystem for eMMC. See
  the Alternatives section below for additional options.

- Volume management: LVM. This allows for dynamic partition/removal, similar to
  the current UBI implementation. LVM support increases the size of the kernel
  by ~100kB, but the increase in size is worth the ability of being able to
  resize the partition if needed. In addition, UBI volume management works in a
  similar way, so it would not be complex to implement LVM management in the
  code update application.

- Partitioning: Model the full eMMC as a single device containing logical
  volumes, instead of fixed-size partitions. This provides flexibility for cases
  where the contents of a partition outgrow its size. This also means that other
  firmware images, such as BIOS and PSU, would be stored in volume in the single
  eMMC device.

- Initramfs: Use an initramfs, which is the default in OpenBMC, to boot the
  rootfs from a logical volume. An initramfs allows for flexibility if
  additional boot actions are needed, such as mounting overlays.

- Mount points: For firmware images such as BIOS that currently reside in
  separate SPI NOR modules, the logical volume in the eMMC would be mounted in
  the same paths as to prevent changes to the applications that rely on that
  data.

- Code update: Support multiple versions on flash, default to two like the
  current UBI implementation.

## Alternatives Considered
- Filesystem: f2fs (Flash-Friendly File System). The ext4 filesystem may be seen
  as more mature and stable, but it is unknown how it would perform against
  other filesystems in an OpenBMC environment. Plans are still in place to try
  it out to compare it against ext4 for OpenBMC.

- No initramfs: It may be possible to boot the rootfs by passing the UUID of the
  logical volume to the kernel, although a pre-init script[1] will likely still
  be needed. Therefore having an initramfs would offer a more standard
  implementation for initialization.

- Static partitioning: This would avoid the kernel memory overhead to search for
  the LVM volume where the rootfs resides, but this is probably not significant.
  In addition, having static partitioning requires to commit to a fixed size,
  without the ability to be able to resize in the future if more space is needed
  for that partition.

## Impacts
This design would impact the OpenBMC build process and code update
internal implementations, but should not affect the external interfaces.

## Testing
Verify OpenBMC functionality in a system containing an eMMC. This system could
be added to the CI pool.

[1]: https://github.com/openbmc/openbmc/blob/master/meta-phosphor/recipes-phosphor/preinit-mounts/preinit-mounts/init
