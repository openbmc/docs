# eMMC Storage Design

Author: Adriana Kobylak < anoo! >

Other contributors: Joel Stanley < shenki! >, Milton Miller

Created: 2019-06-20

## Problem Description

Proposal to define an initial storage design for an eMMC device. This includes
filesystem type, partitioning, volume management, boot options and
initialization, etc.

## Background and References

OpenBMC currently supports raw flash such as the SPI NOR found in the systems
based on AST2400 and AST2500, but there is no design for managed NAND.

## Requirements

- Security: Ability to enforce read-only, verification of official/signed images
  for production.

- Updatable: Ensure that the filesystem design allows for an effective and
  simple update mechanism to be implemented.

- Simplicity: Make the system easy to understand, so that it is easy to develop,
  test, use, and recover.

- Code reuse: Try to use something that already exists instead of re-inventing
  the wheel.

## Proposed Design

- The eMMC image layout and characteristics are specified in a meta layer. This
  allows OpenBMC to support different layouts and configurations. The tarball to
  perform a code update is still built by image_types_phosphor, so a separate
  IMAGE_TYPES would need to be created to support a different filesystem type.

- Code update: Support two versions on flash. This allows a known good image to
  be retained and a new image to be validated.

- GPT partitioning for the eMMC User Data Area: This is chosen over dynamic
  partitioning due to the lack of offline tools to build an LVM image (see
  Logical Volumes in the Alternatives section below).

- Initramfs: An initramfs is needed to run sgdisk on first boot to move the
  secondary GPT to the end of the device where it belongs, since the yocto wic
  tool does not currently support building an image of a specified size and
  therefore the generated image may not be exactly the size of the device that
  is flashed into.

- Read-only and read-write filesystem: ext4. This is a stable and widely used
  filesystem for eMMC.

- Filesystem layout: The root filesystem is hosted in a read-only volume. The
  /var directory is mounted in a read-write volume that persists through code
  updates. The /home directory needs to be writable to store user data such as
  ssh keys, so it is a bind mount to a directory in the read-write volume. A
  bind mount is more reliable than an overlay, and has been around longer. Since
  there are no contents delivered by the image in the /home directory, a bind
  mount can be used. On the other hand, the /etc directory has content delivered
  by the image, so it is an overlayfs to have the ability to restore its
  configuration content on a factory reset.

        +------------------+ +-----------------------------+
        | Read-only volume | | Read-write volume           |
        |------------------| |-----------------------------|
        |                  | |                             |
        | / (rootfs)       | | /var                        |
        |                  | |                             |
        | /etc  +------------->/var/etc-work/  (overlayfs) |
        |                  | |                             |
        | /home +------------->/var/home-work/ (bind mount)|
        |                  | |                             |
        |                  | |                             |
        +------------------+ +-----------------------------+

- Provisioning: OpenBMC will produce as a build artifact a flashable eMMC image
  as it currently does for NOR chips.

## Alternatives Considered

- Store U-Boot and the Linux kernel in a separate SPI NOR flash device, since
  SOCs such as the AST2500 do not support executing U-Boot from an eMMC. In
  addition, having the Linux kernel on the NOR saves from requiring U-Boot
  support for the eMMC. The U-Boot and kernel are less than 10MB in size, so a
  fairly small chip such as a 32MB one would suffice. Therefore, in order to
  support two firmware versions, the kernel for each version would need to be
  stored in the NOR. A second NOR device could be added as redundancy in case
  U-Boot or the kernel failed to run.

  Format the NOR as it is currently done for a system that supports UBI: a fixed
  MTD partition for U-Boot, one for its environment, and a UBI volume spanning
  the remaining of the flash. Store the dual kernel volumes in the UBI
  partition. This approach allows the re-use of the existing code update
  interfaces, since the static approach does not currently support storing two
  kernel images. Selection of the desired kernel image would be done with the
  existing U-Boot environment approach.

  Static MTD partitions could be created to store the kernel images, but
  additional work would be required to introduce a new method to select the
  desired kernel image, because the static layout does not currently have dual
  image support.

  The AST2600 supports executing U-Boot from the eMMC, so that provides the
  flexibility of just having the eMMC chip on a system, or still have U-Boot in
  a separate chip for recovery in cases where the eMMC goes bad.

- Filesystem: f2fs (Flash-Friendly File System). The f2fs is an up-and-coming
  filesystem, and therefore it may be seen as less mature and stable than the
  ext4 filesystem, although it is unknown how any of the two would perform in an
  OpenBMC environment.

  A suitable alternative would be btrfs, which has checksums for both metadata
  and data in the filesystem, and therefore provides stronger guarantees on the
  data integrity.

- All Code update artifacts combined into a single image.

  This provides simple code maintenance where an image is intact or not, and
  works or not, with no additional fragments lying around. U-Boot has one choice
  to make - which image to load, and one piece of information to forward to the
  kernel.

  To reduce boot time by limiting IO reading unneeded sectors into memory, a
  small FS is placed at the beginning of the partition to contain any artifacts
  that must be accessed by U-Boot.

  This file system will be selected from ext2, FAT12, and cramfs, as these are
  all supported in both the Linux kernel and U-Boot. (If we desire the U-Boot
  environment to be per-side, then choose one of ext2 or FAT12 (squashfs support
  has not been merged, it was last updated in 2018 -- two years ago).

- No initramfs: It may be possible to boot the rootfs by passing the UUID of the
  logical volume to the kernel, although a [pre-init script][] will likely still
  be needed. Therefore, having an initramfs would offer a more standard
  implementation for initialization.

- FAT MBR partitioning: FAT is a simple and well understood partition table
  format. There is space for 4 independent partitions. Alternatively one slot
  can be chained into extended partitions, but each partition in the chan
  depends on the prior partition. Four partitions may be sufficient to meet the
  initial demand for a shared (single) boot filesystem design (boot, rofs-a,
  rofs-b, and read-write). Additional partitions would be needed for a dual boot
  volume design.

  If common space is needed for the U-Boot environment, is is redundantly stored
  as file in partition 1. The U-Boot SPL will be located here. If this is not
  needed, partition 1 can remain unallocated.

  The two code sides are created in slots 2 and 3.

  The read-write filesystem occupies partition 4.

  If in the future there is demand for additional partitions, partition can be
  moved into an extended partition in a future code update.

- Device Mapper: The eMMC is divided using the device-mapper linear target,
  which allows for the expansion of devices if necessary without having to
  physically repartition since the device-mapper devices expose logical blocks.
  This is achieved by changing the device-mapper configuration table entries
  provided to the kernel to append unused physical blocks.

- Logical Volumes:

  - Volume management: LVM. This allows for dynamic partition/removal, similar
    to the current UBI implementation. LVM support increases the size of the
    kernel by ~100kB, but the increase in size is worth the ability of being
    able to resize the partition if needed. In addition, UBI volume management
    works in a similar way, so it would not be complex to implement LVM
    management in the code update application.

  - Partitioning: If the eMMC is used to store the boot loader, a ext4 (or vfat)
    partition would hold the FIT image containing the kernel, initrd and device
    tree. This volume would be mounted as /boot. This allows U-Boot to load the
    kernel since it doesn't have support for LVM. After the boot partition,
    assign the remaining eMMC flash as a single physical volume containing
    logical volumes, instead of fixed-size partitions. This provides flexibility
    for cases where the contents of a partition outgrow a fixed size. This also
    means that other firmware images, such as BIOS and PSU, can be stored in
    volumes in the single eMMC device.

  - Initramfs: Use an initramfs, which is the default in OpenBMC, to boot the
    rootfs from a logical volume. An initramfs allows for flexibility if
    additional boot actions are needed, such as mounting overlays. It also
    provides a point of departure (environment) to provision and format the eMMC
    volume(s). To boot the rootfs, the initramfs would search for the desired
    rootfs volume to be mounted, instead of using the U-Boot environments.

  - Mount points: For firmware images such as BIOS that currently reside in
    separate SPI NOR modules, the logical volume in the eMMC would be mounted in
    the same paths as to prevent changes to the applications that rely on the
    location of that data.

  - Provisioning: Since the LVM userspace tools don't offer an offline mode,
    it's not straightforward to assemble an LVM disk image from a bitbake task.
    Therefore, have the initramfs create the LVM volume and fetch the rootfs
    file into tmpfs from an external source to flash the volume. The rootfs file
    can be fetched using DHCP, UART, USB key, etc. An alternative option include
    to build the image from QEMU, this would require booting QEMU as part of the
    build process to setup the LVM volume and create the image file.

## Impacts

This design would impact the OpenBMC build process and code update internal
implementations but should not affect the external interfaces.

- openbmc/linux: Kernel changes to support the eMMC chip and its filesystem.
- openbmc/openbmc: Changes to create an eMMC image.
- openbmc/openpower-pnor-code-mgmt: Changes to support updating the new
  filesystem.
- openbmc/phosphor-bmc-code-mgmt: Changes to support updating the new
  filesystem.

## Testing

Verify OpenBMC functionality in a system containing an eMMC. This system could
be added to the CI pool.

[pre-init script]:
  https://github.com/openbmc/openbmc/blob/master/meta-phosphor/recipes-phosphor/preinit-mounts/preinit-mounts/init
