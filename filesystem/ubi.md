# UBI on MTD partition

The rootfs content is stored in a squashfs using xz compression. This content is
stored in a static UBI volume using the block emulation driver (ubiblock). To
store updates to files, a UBIFS volume is used for /var and mounted over the
/etc and /home directories using overlayfs.

The environment for Das U-boot will continue to be stored at fixed sectors in
the flash. The Das U-boot environment will contain enough MTD partition
definition to read UBI volumes in a UBI device in the same flash. The bootcmd
script will load a kernel from a FIT image and pass it to bootargs to locate and
mount the squashfs in the paired UBI volume.
