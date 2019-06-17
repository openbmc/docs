# UBI on MTD partition

The rootfs content is stored in a squashfs using xz compression. This content is
stored in a static UBI volume. To store updates to files, a UBIFS volume is used
for /var and mounted over the /etc and /home directories using overlayfs. Note
that the remaining of the squashfs files are read-only.

During code update, a new static UBI volume is created and updated with the new
squashfs image. Then the U-Boot environment variables are updated to point to
the newly created volume.
