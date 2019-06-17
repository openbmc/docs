# OpenBMC Filesystem Documentation
This directory is focused on providing information about the filesystems
supported in OpenBMC, including filesystem layouts, overlays, and boot options.

- [jffs2 on mtd partition](jffs2.md)

   Default filesystem in OpenBMC. Used in BMC subsystems such as the ones based
   around the ast2400 and ast2500 system-on-chip controllers from Aspeed
   Technology. These SOCs support 1 and 2 GB of DDR RAM, while the attached
   flash storage is typically in the 10s of MB.

- [ubi on mtd partition](ubi.md)

   Enabled via the `obmc-ubi-fs` OpenBMC distro feature. Used in the same BMC
   subsystems as the jffs2 ones, but targeted for configurations with dual flash
   storage.
