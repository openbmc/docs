OpenBMC & Host Code Update
===================

After building OpenBMC, you will end up with a set of image files in
`tmp/deploy/images/<platform>/`. The `image-*` symlinks correspond to components
that can be updated on the BMC:

 * `image-bmc` → `flash-<platform>-<timestamp>`

    The whole-of-flash image for the BMC

 * `image-initramfs` → `core-image-minimal-initramfs-palmetto.cpio.lzma.u-boot`

    The small initramfs image; used for early init and flash management

 * `image-kernel` → `cuImage`

    The OpenBMC kernel cuImage (combined kernel and device tree)

 * `image-rofs` → `obmc-phosphor-image-palmetto.squashfs-xz`

    The read-only OpenBMC filesystem

 * `image-rwfs` → `rwfs.jffs2`

    The read-write filesystem for persistent changes to the OpenBMC filesystem

 * `image-u-boot` → `u-boot.bin`

    The OpenBMC bootloader

Update from the OpenBMC shell
-----------------------------

To update from the OpenBMC shell, copy one or more of these `image-*` files to
the directory:

    /run/initramfs/

(preserving the filename), then reboot. Before the system reboots, the new image
will be copied to the appropriate flash partition.


Update via REST
---------------

An OpenBMC system can download an update image from a TFTP server, and apply
updates, controlled via REST. The general procedure is:

 1. Configure update settings
 2. Initiate update
 3. Check flash status

### Configure update settings

There are a few settings available to control the update process:

 * `preserve_network_settings`: Preserve network settings, only needed if updating uboot
 * `restore_application_defaults`: update read-only file system
 * `update_kernel_and_apps_only`: update kernel and initramfs
 * `clear_persistent_files`: Erase persistent files

To configure the update settings, perform a REST PUT to
`/control/flash/bmc/attr/<setting>`. For example:

    curl -b cjar -k -H "Content-Type: application/json" -X PUT \
        -d '{"data": 1}' \
        https://bmc/org/openbmc/control/flash/bmc/attr/preserve_network_settings

### Initiate update

Perform a POST to invoke the `updateViaTftp` method of the `/flash/bmc` object:

    curl -b cjar -k -H "Content-Type: application/json" -X POST \
        -d '{"data": ["<TFTP server IP address>", "<filename>"]}' \
        https://bmc/org/openbmc/control/flash/bmc/action/updateViaTftp

### Check flash status

You can query the progress of the flash with a simple GET request:

    curl -b cjar -k https://bmc/org/openbmc/control/flash/bmc

Host code update
================

Using a similar method, we can update the host firmware (or "BIOS"), by
performing a POST request to call the `updateViaTftp` method of
`/control/flash/bios` (instead of `/control/flash/bmc` used above). To initiate
the update:

    curl -b cjar -k -H "Content-Type: application/json" -X POST \
        -d '{"data": ["<TFTP server IP address>", "<filename>"]}' \
        https://bmc/org/openbmc/control/flash/bios/action/updateViaTftp

And to check the flash status:

    curl -b cjar -k https://bmc/org/openbmc/control/flash/bios
