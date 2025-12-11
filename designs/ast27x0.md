# AST27x0

Author: Vince Chang <vince_chang@aspeedtech.com>

Created: December 10, 2025

## Prebuilt Binary Description

AST27x0 `image-u-boot` includes prebuilt firmware binaries for AST27x0 bring-up.

### Prebuilt Binary Location

The prebuilt binaries are stored in the prebuilt directory of the fmc_imgtool
repository:

- <https://github.com/AspeedTech-BMC/fmc_imgtool/tree/master/prebuilt>

### Prebuilt Binary Summary

| File Name                 | Category       | Description                                                                                 |
| ------------------------- | -------------- | ------------------------------------------------------------------------------------------- |
| ast2700-caliptra-fw\*.bin | Caliptra       | Caliptra binary includes the Firmware Manifest, Caliptra FMC binary, and Caliptra RT binary |
| ddr\*.bin                 | Vendor IP      | PMU training firmware (DDR4 / DDR5)                                                         |
| dp_fw.bin                 | Proprietary IP | DP controller firmware                                                                      |
| uefi_ast2700.bin          | VBIOS          | AST2700 UEFI boot firmware                                                                  |

#### Caliptra Binary Description

Caliptra binary includes the Firmware Manifest, Caliptra FMC binary, and
Caliptra RT binary. The Caliptra FMC and RT binaries were built from the
caliptra-sw project
(<https://github.com/chipsalliance/caliptra-sw/commit/51ff0a89f169bbf8e06acb49b31db555e99fefb6>).
