# OpenBMC Flash Layout and Filesystem Documentation
This directory is focused on providing information about the filesystems
supported in OpenBMC, including filesystem layouts, overlays, boot options, and
code update with focus on updating and booting from a new image. See
[code-update] for details about code update interfaces.

## Boot loading and init
The boot loading and init is common across the filesystems supported in OpenBMC.

For system initialization and bootstrap, [Das U-Boot] was selected as the
bootloader. In the pre-boot stages, [Das U-boot] initializes the memory if it
was not already setup. After locating its environment via compile options, it
copies itself into the top of memory. Additional peripherals are reset, and the
command loop is started. Based on the contents of the environment, a prompt is
presented and automatic boot is started following the script in the environment.

For runtime management, the [systemd] system and service manager was chosen for
its configuration, dependency, and triggered action support, as well as its
robust recovery. Before starting execution, systemd requires the root filesystem
and all binaries to be mounted. The filesystems for /dev, /sys, and /proc may be
mounted before starting systemd.
Reference the [systemd File Hierarchy Requirements].

Mount points are created by systemd, an initramfs, or a combination or both,
depending on the platform implementation. Details are located in the filesystem
section below.

## Filesystem
### Filesystem layout
An effort has been made to restrict runtime modification of the filesystem to
the Filesystem Hierarchy Standard [FHS]. Specifically data ephemeral to the
current boot is stored in /run and most application data is stored under /var.
Some information continues to be stored in the system configuration data
directory /etc; this is mostly traditionally configuration such as network
addresses, user identification, and ssh host keys.

A tmpfs is used for /tmp, /run, and similar, while /dev, /proc, and
/sys are supported by their normal kernel special filesystems, as specified by
the [FHS].

### Filesystem types
The following filesystems are supported in OpenBMC. Additional information is
given for each filesystem such as how the filesystem is stored on flash, how the
filesystem is instantiated during system boot/init, and how code update handles
the filesystem.

- [JFFS2 on MTD partition](jffs2.md)

   Default filesystem in OpenBMC. Used in BMC subsystems such as the ones based
   around the AST2400 and AST2500 system-on-chip controllers from Aspeed
   Technology. These SOCs support 1 and 2 GB of DDR RAM, while the attached
   flash storage is typically in the 10s of MB.

- [UBI on MTD partition](ubi.md)

   Enabled via the `obmc-ubi-fs` OpenBMC distro feature. Used in the same BMC
   subsystems as the JFFS2 ones, but targeted for configurations with dual flash
   storage.

## Other
Additional Bitbake layer configurations exist for Raspberry Pi and x86 QEMU
machines, but primarily for code development and exploration. Code update for
these environments is not presently supported except by restarting with a new
build product.

[code-update]: https://github.com/openbmc/docs/tree/master/code-update
[Das U-Boot]: https://www.denx.de/wiki/U-Boot
[systemd]: https://github.com/openbmc/docs/blob/master/openbmc-systemd.md
[systemd File Hierarchy Requirements]: https://www.freedesktop.org/wiki/Software/systemd/FileHierarchy/
[FHS]: https://refspecs.linuxfoundation.org/fhs.shtml
