Author: Lakshmi Yadlapati < lakshmi-y! >

Other contributors: Adriana Kobylak < anoo! >

Created: 2022-10-18

## Problem Description
Currently FW update process updates only primary u-boot image and the alternate u-boot image is 
treated as a golden image. Proposal is to always check and update the alternative u-boot image as 
part of boot process.

## Requirements
• ECC is enabled in latest ASPEED BMCs. Upgrading the FW from the build with ECC disabled to the 
  build with ECC enabled will result in primary u-boot image with ECC enabled and alternate u-boot 
  image with ECC disabled. 
• If secure boot is enabled, ASPEED BMCs always attempt to boot from the primary image and if it 
  fails, then falls back to the alternate image. Upgrading the FW from the build with ECC disabled 
  to the build with ECC enabled will result in inconsistent u-boot images.
 
## Proposed Design
• New systemd service “obmc-flash-mmc-mirroruboot” will be created. 
• obmc-flash-mmc-mirroruboot service runs once as part of the boot process.
• obmc-flash-mmc-mirroruboot service compares the checksum of current u-boot image with alternative 
  u-boot image and updates the alternative u-boot if they are different.

## Testing
Test scenarios:
• After FW update, verify checksum of primary and alternate u-boot image.
• Boot with corrupted alternate u-boot image, verify checksum of primary and alternate u-boot image 
  after reboot.
• Enable full secure boot. Verify BMC boots successfully from alternative u-boot image if primary 
  u-boot is corrupted. Verify checksum of primary and alternate u-boot image after reboot.
• Enable full secure boot. BMC boots successfully with corrupted alternative u-boot image. Verify 
  checksum of primary and alternate u-boot image after reboot.

