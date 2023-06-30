# OpenBMC Flash Layout and Filesystem Documentation

This file is focused on providing information about the flash setup that the
code update application requires to be supported out-of-the-box, which includes
how the Linux filesystem is setup, filesystem layouts, overlays, boot options,
and how code update updates the flash modules and boots the new image. See
[code-update.md](code-update.md) for details about code update interfaces.

## Design considerations

### Boot loading and init

For system initialization and bootstrap, [Das U-Boot][] was selected as the
bootloader.

After basic initialization of the system, the bootloader may present a prompt
and/or start automatic boot. The commands and/or data to select the boot image
are stored in the bootloader environment. The bootloader copies the compressed
kernel, initrd image, and device tree into memory, and then transfers control to
the kernel. The kernel initializes itself and the system using the information
passed in the device tree, including the flash partitions and the kernel command
line embedded in the tree.

### Runtime management

For runtime management, the [systemd][] system and service manager was chosen
for its configuration, dependency, and triggered action support, as well as its
robust recovery.

Before starting execution, systemd requires the root filesystem and all binaries
to be mounted. The filesystems for /dev, /sys, and /proc may be mounted before
starting systemd. Reference the [systemd File Hierarchy Requirements][].

### Root filesystem

For storage of the root filesystem, a read-only volume was selected. This allows
the majority of the filesystem content, including all executables and static
data files, to be stored in a read-only filesystem image. Replacing read-only
filesystem images allows the space used by the content to be confirmed at build
time and allows the selection of compressed filesystems that do not support
mutations.

An effort has been made to adhere to the Filesystem Hierarchy Standard [FHS][].
Specifically data ephemeral to the current boot is stored in /run and most
application data is stored under /var. Some information continues to be stored
in the system configuration data directory /etc; this is mostly traditionally
configuration such as network addresses, user identification, and ssh host keys.

To conserve flash space, squashfs with xz compression was selected to store the
read-only filesystem content. This applies to systems with limited attached
flash storage (see the JFFS2 and UBI options below), not eMMC.

To load the root filesystem, the initramfs locates and mounts the squashfs and
writable filesystems, merges them with overlayfs, performs a chroot into the
result and starts systemd. Alternatively, information to find the active image
for the BMC can be stored in the U-Boot environment, and an init script can
mount the images then start systemd. This choice depends on the platform
implementation, and details are located in the Supported Filesystem Choices
section below.

## Supported Filesystem Choices

OpenBMC supports code update for the following types of filesystems. Additional
information is given for each filesystem such as how the filesystem is stored on
flash, how the filesystem is instantiated during system boot/init, and how code
update handles the filesystem.

### Writable Filesystem Options

#### JFFS2 on MTD partition

The majority of the filesystem is stored in a read-only squashfs in an MTD
partition using the block emulation driver (mtdblock). A second MTD partition is
mounted read-write using the JFFS2 filesystem. This read-write filesystem is
mounted over the entire filesystem space allowing all files and directories to
be written.

This filesystem stack requires the mounts to be performed from an initramfs. The
initramfs is composed of a basic system based on busybox and three custom
scripts (init, shutdown, and update) that locate the MTD partitions by name.
These scripts are installed by [obmc-phosphor-initfs][].

In code update mode, the squashfs image and white-listed files from the
read-write filesystem are copied into RAM by the initramfs and used to assemble
the root overlayfs instance, leaving the flash free to be modified by installing
images at runtime. An orderly shutdown writes remaining images to like-named raw
MTD partitions and white listed files to the writable overlay filesystem.
Alternatively, if code update mode was not selected, the image updates must be
delayed until the partitions are unmounted during an orderly shutdown.

This is the default filesystem in OpenBMC. It is used in several BMC systems
based around the AST2400 and AST2500 system-on-chip controllers from Aspeed
Technology. These SOCs support 1 and 2 GB of DDR RAM, while the attached flash
storage is typically in the 10s of MB, so staging the filesystem to RAM is not
an issue.

#### UBI on MTD partition

The majority of the filesystem is stored in a read-only squashfs in a static UBI
volume using the UBI block emulation driver (ubiblock). To store updates to
files, a UBIFS volume is used for /var and mounted over the /etc and /home
directories using overlayfs. These mounts are performed by the `init` script
installed by the [preinit-mounts][] package before `systemd` is started.
Selecting UBI allows the writes to the read-write overlay to be distributed over
the full UBI area instead of just the read-write MTD partition.

The environment for Das U-boot continues to be stored at fixed sectors in the
flash. The Das U-boot environment contains enough MTD partition definition to
read UBI volumes in a UBI device in the same flash. The bootcmd script loads a
kernel from a FIT image and pass it to bootargs to locate and mount the squashfs
in the paired UBI volume.

This option is enabled via the `obmc-ubi-fs` OpenBMC distro feature. Used in the
same BMC subsystems as the JFFS2 ones, but targeted for configurations that have
enough flash storage to store at least 2 copies of the filesystem. This can be
accomplished with dual flash storage. Some controllers, such as those in the
AST2500, allow booting from an alternate flash on failure and this UBI option
supports this feature. For this support, a copy of each kernel is stored on each
flash and the U-Boot environment selects which kernel to use.

##### With root as initramfs

Alternatively to squashfs, the root filesystem can be stored in a cpio format
and placed as the initramfs for the kernel's FIT image so that the BMC is
entirely running from a ramdisk. This option is enabled via the
`obmc-static-norootfs` OpenBMC distro feature.

#### ext4 on eMMC

See the [eMMC Design Document][] for details.

This option is enabled via the `phosphor-mmc` OpenBMC distro feature.

### Auxiliary Filesystems

A tmpfs is used for /tmp, /run, and similar, while /dev, /proc, and /sys are
supported by their normal kernel special filesystems, as specified by the FHS.

## Other

Additional Bitbake layer configurations exist for Raspberry Pi and x86 QEMU
machines, but are provided primarily for code development and exploration. Code
update for these environments is not supported.

[das u-boot]: https://www.denx.de/wiki/U-Boot
[systemd]:
  https://github.com/openbmc/docs/blob/master/architecture/openbmc-systemd.md
[systemd file hierarchy requirements]:
  https://www.freedesktop.org/wiki/Software/systemd/FileHierarchy/
[fhs]: https://refspecs.linuxfoundation.org/fhs.shtml
[obmc-phosphor-initfs]:
  https://github.com/openbmc/openbmc/blob/master/meta-phosphor/recipes-phosphor/initrdscripts/obmc-phosphor-initfs.bb
[preinit-mounts]:
  https://github.com/openbmc/openbmc/tree/master/meta-phosphor/recipes-phosphor/preinit-mounts
[emmc design document]:
  https://github.com/openbmc/docs/blob/master/architecture/code-update/emmc-storage-design.md
