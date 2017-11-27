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

Additionally, there are two tarballs created that can be deployed and unpacked by REST:

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

By default, an orderly `reboot` will stop all applications and unmount
the root filesystem, and the images copied into the `/run/initramfs`
directory will be applied at that point before restarting.  This also
applied to the `shutdown` and `halt` commands -- they will write the
flash before stopping.

As an alternative, an option can be parsed by the `init` script in
the initramfs to copy the required contents of these filesystems into
RAM so the images can be applied while the rest of the application stack
is running and progress can be monitored over the network.  The
`update` script can then be called to write the images while the
system is operational and its progress output monitored.

Update from the OpenBMC shell
-----------------------------

To update from the OpenBMC shell, follow the steps in this section.

It is recommended that the BMC be prepared for update (note that the
environment variable needs to be set twice for initramfs be able to
read it due to the U-Boot redundant environments):

    fw_setenv openbmconce copy-files-to-ram copy-base-filesystem-to-ram
    fw_setenv openbmconce copy-files-to-ram copy-base-filesystem-to-ram
    reboot

Copy one or more of these `image-*` files to the directory:

    /run/initramfs/

(preserving the filename), then run the `update` script to apply the images:

    /run/initramfs/update

then reboot to finish applying:

    reboot

During the reboot process, the `update` script will be invoked after
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
        https://${bmc}/org/openbmc/control/flash/bmc/action/prepareForUpdate

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
        https://${bmc}/org/openbmc/control/flash/bmc/attr/preserve_network_settings

### Initiate update

Perform a POST to invoke the `updateViaTftp` method of the `/flash/bmc` object:

    curl -b cjar -k -H "Content-Type: application/json" -X POST \
        -d '{"data": ["<TFTP server IP address>", "<filename>"]}' \
        https://${bmc}/org/openbmc/control/flash/bmc/action/updateViaTftp

Note the `<filename>` shall be a tarball.

### Check flash status

You can query the progress of the download and image verification with
a simple GET request:

    curl -b cjar -k https://${bmc}/org/openbmc/control/flash/bmc

Or perform a POST to invoke the `GetUpdateProgress` method of the `/flash/bmc` object:

    curl -b cjar -k -H "Content-Type: application/json" -X POST \
        -d '{"data": []}' \
        https://${bmc}/org/openbmc/control/flash/bmc/action/GetUpdateProgress

Note:

 * During downloading the tarball, the progress status is `Downloading`
 * After the tarball is downloaded and verified, the progress status becomes `Image ready to apply`.

### Apply update
If the status is `Image ready to apply.` then you can either initiate
a reboot or call the Apply method to start the process of writing the
flash:

    curl -b cjar -k -H "Content-Type: application/json" -X POST \
        -d '{"data": []}' \
        https://${bmc}/org/openbmc/control/flash/bmc/action/Apply

Now the image is being flashed, you can check the progress with above step’s command as well.

* During flashing the image, the status becomes `Writing images to flash`
* After it’s flashed and verified, the status becomes `Apply Complete. Reboot to take effect.`

### Reboot the BMC

To start using the new images, reboot the BMC using the warmReset method
of the BMC control object:

    curl -b cjar -k -H "Content-Type: application/json" -X POST \
        -d '{"data": []}' \
        https://${bmc}/org/openbmc/control/bmc0/action/warmReset


Host code update
================

Reference:
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Software

Following are the steps to update the host firmware (or "BIOS"). This assumes
the host is not accessing its firmware.

1. Get a squashfs image:
  * Build op-build: https://github.com/open-power/op-build
  * After building, the image should be a tarball in the output/images
    directory called <system type>.pnor.squashfs.tar

2. Transfer the generated squashfs image to the BMC via one of the following
methods:
  * Method 1: Via scp: Copy the generated squashfs image to the `/tmp/images/`
    directory on the BMC.
  * Method 2: Via REST Upload:
  https://github.com/openbmc/docs/blob/master/rest-api.md#uploading-images
  * Method 3: Via TFTP: Perform a POST request to call the `DownloadViaTFTP`
    method of `/xyz/openbmc_project/software`.

      ```
      curl -b cjar -k -H "Content-Type: application/json" -X POST \
        -d '{"data": ["<filename>", "<TFTP server IP address"]}' \
        https://${bmc}/xyz/openbmc_project/software/action/DownloadViaTFTP
      ```

3. Note the version id generated for that image file, which is a hash value of 8
hexadecimal numbers generated by the C++ function std::hash using the version
string contained in the image (TODO: the hash algorithm may change to allow users
to calculate it externally openbmc/openbmc#2274) via one of the following methods:

  * Method 1: From the BMC command line, note the most recent directory name
    created under `/tmp/images/`, in this example it'd be `2a1022fe`:

      ```
      # ls -l /tmp/images/
      total 0
      drwx------    2 root     root            80 Aug 22 07:54 2a1022fe
      drwx------    2 root     root            80 Aug 22 07:53 488449a2
      ```

  * Method 2: Using the REST API, note the object that has its Activation
    property set to Ready, in this example it'd be `2a1022fe`:

      ```
      $ curl -b cjar -k https://${bmc}/xyz/openbmc_project/software/enumerate
      {
        "data": {
          "/xyz/openbmc_project/software/2a1022fe": {
            "Activation": "xyz.openbmc_project.Software.Activation.Activations.Ready",
      ```

4. To initiate the update, set the `RequestedActivation` property of the desired
image to `Active`, substitute ``<id>`` with the hash value noted on the previous
step, this will write the contents of the image to a UBI volume in the PNOR chip
via one of the following methods:

  * Method 1: From the BMC command line:

      ```
      busctl set-property org.open_power.Software.Host.Updater \
        /xyz/openbmc_project/software/<id> \
        xyz.openbmc_project.Software.Activation RequestedActivation s \
        xyz.openbmc_project.Software.Activation.RequestedActivations.Active

      ```

  * Method 2: Using the REST API:

      ```
      curl -b cjar -k -H "Content-Type: application/json" -X PUT \
        -d '{"data":
        "xyz.openbmc_project.Software.Activation.RequestedActivations.Active"}' \
        https://${bmc}/xyz/openbmc_project/software/<id>/attr/RequestedActivation
      ```

5. (Optional) Check the flash progress. This interface is only available during
the activation progress and is not present once the activation is completed
via one of the following:

  * Method 1: From the BMC command line:

      ```
      busctl get-property org.open_power.Software.Host.Updater \
        /xyz/openbmc_project/software/<id> \
        xyz.openbmc_project.Software.Activation Progress
      ```

  * Method 2: Using the REST API:

      ```
      curl -b cjar -k https://${bmc}/xyz/openbmc_project/software/<id>/attr/Progress
      ```

6. Check the activation is complete by verifying the Activation property is set
to Active via one of the following methods:

  * Method 1: From the BMC command line:

      ```
      busctl get-property org.open_power.Software.Host.Updater \
        /xyz/openbmc_project/software/<id> \
        xyz.openbmc_project.Software.Activation Activation
      ```

  * Method 2: Using the REST API:

      ```
      curl -b cjar -k https://${bmc}/xyz/openbmc_project/software/<id>
      ```

### Patching the host firmware

Copy the partition binary file to `/usr/local/share/pnor/` on the BMC.

The partition binary file must be named the same as the partition name that
intends to patch, ex: `ATTR_TMP`.
