Author: Lakshmi Yadlapati < lakshmi-y! >
Other contributors: Adriana Kobylak < anoo! >
Created: 2022-10-18

## Problem Description

Currently the FW update process updates only the primary u-boot image and the
alternate u-boot image is treated as a golden image. The latest ASPEED BMCs have
a software enabled ECC (Error Correction Code). Upgrading the FW from a build
with ECC disabled to a build ECC enabled will result in inconsistent u-boot
images, primary u-boot image with ECC enabled and an alternate u-boot image with
ECC disabled.

If secure boot is enabled, ASPEED BMCs always attempt to boot from the primary
u-boot image and if it fails, then it falls back to the alternate image. Failing
over from ECC enabled u-boot image to u-boot with ECC disabled causes memory
corruption.

In addition, if the secure boot keys need to be updated then u-boot mirroring
will ensure that both copies of u-boot image will have updated keys.

## Background and References

U-Boot is a boot loader for Embedded boards based on PowerPC, ARM, MIPS and
several other processors, which can be installed in a boot ROM and used to
initialize and test the hardware or to download and run application code. The
 ASPEED BMCs have redundant u-boot images, the latest AST2600 stores the u-boot
image in eMMC. Primary u-boot is stored in eMMC partition mmcblk0boot0 and
alternative u-boot is stored in eMMC partition mmcblk0boot1. If the primary
u-boot image is corrupted, then BMC will fail over to the alternative u-boot
image. We already have support for u-boot mirroring in UBI layout. This
document is to explain the reasoning for having u-boot mirroring add support
for u-boot mirroring in the eMMC layout.
https://github.ibm.com/openbmc/openbmc/wiki/eMMC-Flash-and-Code-Update


## Requirements

• The FW update process to always update both primary and alternate u-boot image
 if the system supports alternative u-boot image.
• As part of the boot process always check and update the alternative
u-boot image if it is not the same as the primary u-boot image.


## Proposed Design

 Create new systemd service “obmc-flash-mmc-mirroruboot” in the phosphor code
management repository. This obmc-flash-mmc-mirroruboot service runs only once
as part of the BMC boot process. obmc-flash-mmc-mirroruboot compares the
checksum of the current u-boot image that BMC booted from with the alternative
u-boot image and copies the primary u-boot image to the alternative u-boot if
they are different.

FW update process first updates the primary u-boot image. BMC will boot from
the primary u-boot and as part of the boot process, obmc-flash-mmc-mirroruboot
service copies the primary u-boot image to the alternative u-boot image.


## Alternatives Considered

None


## Impacts

None


## Organizational
Does this repository require a new repository? No - all changes will go in
existing repositories.


## Testing

Test Scenarios:

• After FW update, verify checksum of primary and alternate u-boot image.
• Boot with corrupted alternate u-boot image, verify checksum of primary and
alternate u-boot image after reboot.
• Enable full secure boot. Verify BMC boots successfully from alternative u-boot
image if primary u-boot is corrupted. Verify checksum of primary and alternate
u-boot image after reboot.
• Enable full secure boot. BMC boots successfully with corrupted alternative
u-boot image. Verify checksum of primary and alternate u-boot image after
reboot.
