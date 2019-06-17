# OpenBMC Filesystem Documentation
This directory is focused on providing information about the filesystems
supported in OpenBMC, including filesystem layouts, overlays, and boot options.

For system initialization and bootstrap, Das U-Boot was selected as the
bootloader.

For runtime management the systemd system and service manager was chosen for its
configuration, dependancy, and triggered action support.

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
these environments is not presently supported except by restaring with a new
build product.
