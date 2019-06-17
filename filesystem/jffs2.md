# JFFS2 on MTD partition

The rootfs content is stored in a squashfs using xz compression. This content is
normally stored in a MTD partition using the block emulation driver (mtdblock).
To store updates to any files, a JFFS2 MTD partition is mounted over the
squashfs using overlayfs.

The file system hierarchy is created by an initramfs composed of a basic system
based on busybox and three custom scripts (init, shutdown, and update). These
scripts are installed by [obmc-phosphor-initfs].

In code update mode, the squashfs image and white-listed files from the
read-write file system are copied into RAM and used to assemble the root
overlayfs instance, leaving the flash free to be modified. An orderly shutdown
writes remaining images to like-named raw MTD partitions and white listed files
to the writable overlay filesystem.

[obmc-phosphor-initfs]: https://github.com/openbmc/openbmc/blob/master/meta-phosphor/recipes-phosphor/initrdscripts/obmc-phosphor-initfs.bb
