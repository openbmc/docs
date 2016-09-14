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

Additionally, there are two tar-balls created that can be deloyed and unpacked by REST:

 * `<platform>-<timestamp>.all.tar`

    The complete BMC flash content: A single file wrapped in a tar archive

 * `<platform>-<timestamp>.tar`

    Partitioned BMC flash content: Multiple files wrapped in a tar archive,
    one for each of the u-boot, kernel, initramfs, ro and rw partitions.

Preparing for BMC code Update
-----------------------------

The BMC normally runs with the read-write and read-only file systems
mounted, which means these images may be read (and written, for the
read-write filesystem) at any time.  Because the updates are distributed
as complete file system images, these filesystems have to be unmounted
to replace them with new images.  To unmount these file systems all
applications must be stopped.

By default an orderly `reboot` will stop all applications and unmount
the root filesystem, and the images copied into the `/run/initramfs`
directory will be applied at that point before restarting.  This also
applied to the `shutdown` and `halt` commands -- they will write the
flash before stopping.

As an alternative, the an option can be parsed by the `init` script in
the initramfs to copy the required contents of these filesystems into
RAM so the images can be applied while the rest of the application stack
is running and progress can be monitored over the network.  The
`update` script can then be called to write the images while the
system is operational and its progress output monitored.

Update from the OpenBMC shell
-----------------------------

To update from the OpenBMC shell, follow the steps in this section.

It is recommended that the BMC be prepared for update:

    fw_setenv openbmconce copy-files-to-ram copy-base-filesystem-to-ram
    reboot

Copy one or more of these `image-*` files to the directory:

    /run/initramfs/

(preserving the filename), then run the `update` script to apply the images:

    /run/initramfs/update

then reboot to finish applying:

    reboot

During the reboot process the `update` script will be invoked after
the file systems are unmounted to complete the update process.

Some optional features are available, see the help for more details:

    /run/initramfs/update --help

Update via REST
---------------

An OpenBMC system can download an update image from a TFTP server, and apply
updates, controlled via REST. The general procedure is:

 1. Prepare system for update
 2. Configure update settings
 3. Initiate update
 4. Check flash status
 5. Apply update
 6. Reboot the BMC

### Prepare system for update

Perform a POST to invoke the `PrepareForUpdate` method of the `/flash/bmc` object:

    curl -b cjar -k -H "Content-Type: application/json" -X POST \
        -d '{"data":  []}' \
        https://bmc/org/openbmc/control/flash/bmc/action/prepareForUpdate

This will setup the u-boot environment and reboot the BMC.   If no other
images were pending the BMC should return in about 2 minutes.


### Configure update settings

There are a few settings available to control the update process:

 * `preserve_network_settings`: Preserve network settings, only needed if updating the whole flash
 * `restore_application_defaults`: update (clear) the read-write file system
 * `update_kernel_and_apps`: update kernel and initramfs.
                             If the partitioned tarball will be used for update then this option
                             must be set. Otherwise, if the complete tarball will be used then
                             this option must not be set.
 * `clear_persistent_files`: ignore the persistent file list when resetting applications defaults
 * `auto_apply`: Attempt to write the images by invoking the `Apply` method after the images are unpacked.

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

Note the `<filename>` shall be a tar-ball.

### Check flash status

You can query the progress of the download and image verificaton with
a simple GET request:

    curl -b cjar -k https://bmc/org/openbmc/control/flash/bmc

Or perform a POST to invoke the `GetUpdateProgress` method of the `/flash/bmc` object:

    curl -b cjar -k -H "Content-Type: application/json" -X POST \
        -d '{"data": []}' \
        https://bmc/org/openbmc/control/flash/bmc/action/GetUpdateProgress

Note:

 * During downloading the tar-ball, the progress status is `Downloading`
 * After the tar-ball is downloaded and verified, the progress status becomes `Image ready to apply`.

### Apply update
If the status is `Image ready to apply.` then you can either initiate
a reboot or call the Apply method to start the process of writing the
flash:

    curl -b cjar -k -H "Content-Type: application/json" -X POST \
        -d '{"data": []}' \
        https://bmc/org/openbmc/control/flash/bmc/action/Apply

Now the image is being flashed, you can check the progress with above step’s command as well.

* During flashing the image, the status becomes `Writing images to flash`
* After it’s flashed and verified, the status becomes `Apply Complete. Reboot to take effect.`

### Reboot the BMC

To start using the new images, reboot the BMC using the warmReset method
of the BMC control object:

    curl -b cjar -k -H "Content-Type: application/json" -X POST \
        -d '{"data": []}' \
        https://bmc/org/openbmc/control/bmc0/action/warmReset


Host code update
================

The host firmware (or "BIOS") can be updated in a similar method.  Because
the BMC does not use the host firmware it is updated when the download is
completed.  This assumes the host is not accessing its firmware.

Perform a POST request to call the `updateViaTftp` method of
`/control/flash/bios` (instead of `/control/flash/bmc` used above). To initiate
the update:

    curl -b cjar -k -H "Content-Type: application/json" -X POST \
        -d '{"data": ["<TFTP server IP address>", "<filename>"]}' \
        https://bmc/org/openbmc/control/flash/bios/action/updateViaTftp

And to check the flash status:

    curl -b cjar -k https://bmc/org/openbmc/control/flash/bios
