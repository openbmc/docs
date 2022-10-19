Author: Lakshmi Yadlapati < lakshmi-y! >
Other contributors: Adriana Kobylak < anoo! >
Created: 2022-10-18

## Problem Description

Currently the FW update process updates only the primary u-boot image and the alternate u-boot image is treated as a golden image. The latest ASPEED BMCs have a software enabled ECC (Error Correction Code). Upgrading the FW from a build with ECC disabled to a build ECC enabled will result in inconsistent u-boot images, primary u-boot image with ECC enabled and an alternate u-boot image with ECC disabled. If secure boot is enabled, ASPEED BMCs always attempt to boot from the primary image and if it fails, then it falls back to the alternate image. Failing over from ECC enabled u-boot image to u-boot with ECC disabled causes memory corruption.


## Background and References

None


## Requirements

• The FW update process to always update both primary and alternate u-boot image.
• As part of the boot process always check and update the alternative u-boot image if it is inconsistent or corrupted.


## Proposed Design

Add new systemd service “obmc-flash-mmc-mirroruboot”. This obmc-flash-mmc-mirroruboot service runs only once as part of the boot process. obmc-flash-mmc-mirroruboot compares the checksum of current u-boot image with alternative u-boot image and updates the alternative u-boot if they are different.


## Alternatives Considered

None


## Impacts

None


## Organizational
Does this repository require a new repository? No - all changes will go in existing repositories.


## Testing

Test Scenarios:

• After FW update, verify checksum of primary and alternate u-boot image.
• Boot with corrupted alternate u-boot image, verify checksum of primary and alternate u-boot image after reboot.
• Enable full secure boot. Verify BMC boots successfully from alternative u-boot image if primary u-boot is corrupted. Verify checksum of primary and alternate u-boot image after reboot.
• Enable full secure boot. BMC boots successfully with corrupted alternative u-boot image. Verify checksum of primary and alternate u-boot image after reboot.
