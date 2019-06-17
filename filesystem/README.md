# OpenBMC Filesystem Documentation
This directory is focused on providing information about the filesystems
supported in OpenBMC, including filesystem layouts, overlays, boot options, and
code update.

For system initialization and bootstrap, Das U-Boot was selected as the
bootloader.

For runtime management the systemd system and service manager was chosen for its
configuration, dependancy, and triggered action support.

An effort has been made to restrict runtime modification of the file system to
the Filesystem Hierarchy Standard (FHS). Specifically data ephemeral to the
current boot is stored in /run and most application data is stored under /var.
Some information continues to be stored in the system configuration data
directory /etc; this is mostly traditonally configuration such as network
addresses, user identification, and ssh host keys. There is no current plan to
remove all system unique data traditionally in /etc.

- [JFFS2 on MTD partition](jffs2.md)

   Default filesystem in OpenBMC. Used in BMC subsystems such as the ones based
   around the AST2400 and AST2500 system-on-chip controllers from Aspeed
   Technology. These SOCs support 1 and 2 GB of DDR RAM, while the attached
   flash storage is typically in the 10s of MB.

- [UBI on MTD partition](ubi.md)

   Enabled via the `obmc-ubi-fs` OpenBMC distro feature. Used in the same BMC
   subsystems as the JFFS2 ones, but targeted for configurations with dual flash
   storage.

Additional bitbake layer configurations exist for Raspberry Pi and x86 QEMU
machines, but primarily for code development and exploration. Code update for
these environments is not presently supported except by restarting with a new
build product.
