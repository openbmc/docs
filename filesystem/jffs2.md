# JFFS2 on MTD partition

The rootfs content is stored in a squashfs using xz compression. This content is
normally stored in a MTD partition using the block emulation driver. To store
updates to any files, a JFFS2 MTD partition is mounted over the squashfs using
overlayfs. A tmpfs is used for /tmp, /run, and similar, while /dev, /proc, and
/sys are supported by their normal kernel special file systems.

The file system heirachy is created by an initramfs composed of a basic system
based on busybox and three custom scripts (init, shutdown, and update).

In code udpate mode, the squashfs image and white-listed files from the
read-write file system are copied into ram and used to assemble the root
overlayfs instance, leaving the flash free to be modified. An orderly shutdown
writes remaining images to like named raw MTD partitions and white listed files
to the writable overlay filesystem.
