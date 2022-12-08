# Converting to OpenBMC from AMI firmware

Often a number of steps are required to convert from the AMI firmware to
OpenBMC. These steps are documented for various machines below.

## Palmetto

1. Upgrade the firmware flash chip to 256Mb (32MB) part (MX25L25635E or
   equivalent)
2. [Build](cheatsheet.md#building-for-palmetto) and flash the `image-bmc` output
   to the 256Mb flash chip
3. Replace existing flash chip on the motherboard (cradle closest to the
   battery)
4. Manufacture a cable to connect to the COM2 header for the BMC console. COM2
   is wired for a DB-9-style pin-out.
5. Reconfigure both `BMC_DEBUG1` (TX) and `BMC_DEBUG2` (RX) jumpers to short
   pins 2-3.
6. Boot OpenBMC on your Palmetto. The console runs at 115200baud.
