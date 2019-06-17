# OpenBMC Flash Layout and Filesystem Documentation
This file is focused on providing information about the flash setup that the
code update application requires to be supported out-of-the-box, which includes
how the Linux filesystem is setup, filesystem layouts, overlays, boot options,
and how code update updates the flash modules and boots the new image. See
[code-update.md](code-update.md) for details about code update interfaces.

## Boot loading and init
For system initialization and bootstrap, [Das U-Boot][] was selected as the
bootloader. Based on the contents of the environment, a prompt is presented and
automatic boot is started following the script in the environment. The boot
script copies the compressed kernel, initrd image, and device tree into memory.

The device tree describes the flash partitioning to the Linux kernel; it is
configured and control is transferred to the kernel wrapper. The kernel wrapper
decompresses the kernel and transfers control to it. The kernel initializes
itself based on the passed device tree (including its embedded command line), as
well as unpacking any initramfs and copying and decompressing any initrd.

For runtime management, the [systemd][] system and service manager was chosen
for its configuration, dependency, and triggered action support, as well as its
robust recovery. Before starting execution, systemd requires the root filesystem
and all binaries to be mounted. The filesystems for /dev, /sys, and /proc may be
mounted before starting systemd.
Reference the [systemd File Hierarchy Requirements][].

The initramfs locates and mounts the squashfs and writable systems, creates and
mounts the overlayfs, performs chroot and executes the systemd. Alternatively,
information to find each activated image for the BMC can be stored in the U-Boot
environment, and have an init script and systemd create the mount points. This
choice depends on the platform implementation, and details are located in the
filesystem section below.

## Filesystem
### Filesystem layout
An effort has been made to adhere to the Filesystem Hierarchy Standard [FHS][].
Specifically data ephemeral to the current boot is stored in /run and most
application data is stored under /var. Some information continues to be stored
in the system configuration data directory /etc; this is mostly traditionally
configuration such as network addresses, user identification, and ssh host keys.

A tmpfs is used for /tmp, /run, and similar, while /dev, /proc, and
/sys are supported by their normal kernel special filesystems, as specified by
the FHS.

OpenBMC supports code update for the following types of filesystems. Additional
information is given for each filesystem such as how the filesystem is stored on
flash, how the filesystem is instantiated during system boot/init, and how code
update handles the filesystem.

### Read-Only Filesystem
The root filesystem is stored in a read-only volume. To conserve flash space,
the majority of the filesystem content, including all executables and static
data files, is stored in a read-only squashfs filesystem using xz compression.

Each filesystem update image must be a stand-alone squashfs image since no
merging of delta content is supported.

### Writable Filesystem Options
- JFFS2 on MTD partition

  The read-only squashfs filesystem content is normally stored in a MTD
  partition using the block emulation driver (mtdblock). A second MTD partition
  is mounted read-write using the JFFS2 filesystem. This read-write filesystem
  is mounted over the entire filesystem space allowing all files and directories
  to be written.

  This filesystem stack requires the mounts to be performed from an initramfs.
  The initramfs is composed of a basic system based on busybox and three custom
  scripts (init, shutdown, and update) that locate the MTD partitions by name.
  These scripts are installed by [obmc-phosphor-initfs][].

  In code update mode, the squashfs image and white-listed files from the
  read-write filesystem are copied into RAM by the initramfs and used to
  assemble the root overlayfs instance, leaving the flash free to be modified by
  installing images at runtime. An orderly shutdown writes remaining images to
  like-named raw MTD partitions and white listed files to the writable overlay
  filesystem. Alternatively, if code update mode was not selected, the image
  updates must be delayed until the partitions are unmounted during an orderly
  shutdown.

  This is the default filesystem in OpenBMC. Used in BMC subsystems such as the
  ones based around the AST2400 and AST2500 system-on-chip controllers from
  Aspeed Technology. These SOCs support 1 and 2 GB of DDR RAM, while the
  attached flash storage is typically in the 10s of MB.

- UBI on MTD partition

  The squashfs content is stored in a static UBI volume using the UBI block
  emulation driver (ubiblock). To store updates to files, a UBIFS volume is used
  for /var and mounted over the /etc and /home directories using overlayfs.
  These mounts are performed by the `init` script installed by the
  [preinit-mounts][] package before `systemd` is started.

  The environment for Das U-boot will continue to be stored at fixed sectors in
  the flash. The Das U-boot environment will contain enough MTD partition
  definition to read UBI volumes in a UBI device in the same flash. The bootcmd
  script will load a kernel from a FIT image and pass it to bootargs to locate
  and mount the squashfs in the paired UBI volume.

  This option is enabled via the `obmc-ubi-fs` OpenBMC distro feature. Used in
  the same BMC subsystems as the JFFS2 ones, but targeted for configurations
  with dual flash storage.

- ext4 on eMMC

  This is a work in progress. See the [eMMC Design Document][].

## Other
Additional Bitbake layer configurations exist for Raspberry Pi and x86 QEMU
machines, but are provided primarily for code development and exploration. Code
update for these environments is not supported.

[Das U-Boot]: https://www.denx.de/wiki/U-Boot
[systemd]: https://github.com/openbmc/docs/blob/master/openbmc-systemd.md
[systemd File Hierarchy Requirements]: https://www.freedesktop.org/wiki/Software/systemd/FileHierarchy/
[FHS]: https://refspecs.linuxfoundation.org/fhs.shtml
[obmc-phosphor-initfs]: https://github.com/openbmc/openbmc/blob/master/meta-phosphor/recipes-phosphor/initrdscripts/obmc-phosphor-initfs.bb
[preinit-mounts]: https://github.com/openbmc/openbmc/tree/master/meta-phosphor/recipes-phosphor/preinit-mounts
[eMMC Design Document]: https://github.com/openbmc/docs/blob/master/designs/emmc-storage-design.md
