# AST2600 Storage Design

Author: Adriana Kobylak < anoo! >

Primary assignee: Adriana Kobylak

Other contributors: Joel Stanley < shenki! >

Created: 2019-06-20

## Problem Description
Proposal to define the storage design for the AST2600-based platforms with an
eMMC device. This includes filesystem type, partitioning, volume management,
boot options and initialization, etc.

## Background and References
OpenBMC currently supports raw flash such as the SPI NOR found in the systems
based on AST2400 and AST2500, but there is no design for managed NAND.

## Requirements
- Security: Ability to enforce read-only, verfication of official/signed
images for production.
- Updatable: Ensure that the filesystem design allows for an effective and
simple update mechanism to be implemented.
- Simplicity: Make the system easy to understand, so that it is easy to
develop, test, and use.
- Code reuse: Try to use something that already exists instead of re-inventing
the wheel.

## Proposed Design
- Filesystem: ext4 on LVM volumes. This allows for dynamic partition/removal,
similar to the current UBI implementation.
- Initramfs: Use an initramfs, which is the default in OpenBMC, to boot the
rootfs from a logical volume. An initramfs allows for flexibility if additional
boot actions are needed, such as mounting overlays.
- Partitioning: Model the full eMMC as a single device containing logical
volumes, instead of fixed-size partitions. This provides flexibility for cases
where the contents of a partition outgrow its size. This also means that other
firmware images, such as BIOS and PSU, would be stored in volume in the single
eMMC device.
- Mount points: For firmware images such as BIOS that currently reside in
separate SPI NOR modules, the logical volume in the eMMC would be mounted in the
same paths as to prevent changes to the applications that rely on that data.
- Code update: Support multiple versions on flash, default to two like the
current UBI implementation.

## Alternatives Considered
- Filesystem: f2fs. Work would be needed to compare it against ext4 for OpenBMC.
- No initramfs: It may be possible to boot the rootfs by passing the UUID of the
logical volume to the kernel, although a pre-init script[1] will likely still be
needed.

## Impacts
This design would impact the OpenBMC build process and code update
internal implementations, but should not affect the external interfaces.

## Testing
Verify OpenBMC functionality in a system containing an eMMC. This system could
be added to the CI pool.

[1]: https://github.com/openbmc/openbmc/blob/master/meta-phosphor/recipes-phosphor/preinit-mounts/preinit-mounts/init
