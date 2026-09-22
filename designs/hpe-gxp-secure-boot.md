# HPE GXP Secure Boot

Author: Tan Siewert <tan.siewert@9elements.com>

Created: September 22, 2026

## Summary

The HPE GXP SoC requires a prebuilt, HPE-signed bootloader binary (called the
bootblock) for bring-up, which then chainloads the self-built U-Boot:

```text
+---------------------------+
| HPE GXP SoC --> bootblock |  --> U-Boot --> Linux --> OpenBMC
|       (this code)         |
+---------------------------+
```

OpenBMC pulls in the bootblock, appends it to the image, and signs U-Boot with
the customer key.

### Bootblock Binary Location

The bootblock binaries are stored in the gxp-bootblock repository
(gxp2-bootblock branch):

- <https://github.com/HewlettPackard/gxp-bootblock/tree/gxp2-bootblock>

While HPE publishes these binaries on GitHub, their contents are not documented
and no source is available. HPE licenses them as closed source, so the recipes
consuming them are marked `LICENSE = "CLOSED"`.

### Bootblock Binary Summary

There are multiple files in the repository. Although they have the same
functionality, they differ in the ASIC revision they target and the key ID they
were signed with. The file name schema is
`GXP2loader-<asic_revision>-sgn<keyid>.bin`.

### Bootblock Binary Mapping

Each image is built against one bootblock variant:

| Image                                 | Target systems        |
| --------------------------------------| --------------------- |
| `GXP2loader-t26x-sgn00.bin`           | RL300 systems         |
| `GXP2loader-t277-t280-t285-sgn00.bin` | DL32x - DL38x systems |
| `GXP2loader-t282-t288-sgn00.bin`      | DL32x - DL38x systems |

The DL32x - DL38x systems is covered by two binaries because those platforms
are shipped with different ASIC revisions. The revision of the installed SoC,
not the platform name, decides which of the two images is the correct one.

Information on how to determine the ASIC revision can be, according to HPE,
requested via their technical contact.

### Bootblock Binary Description

These bootblocks are for use with servers that have undergone the Transfer of
Ownership process, a secure method of storing customer signing keys and enabling
customer firmware to run securely on HPE ProLiant servers.

Two keys are therefore involved:

- The HPE signing key, whose hash is fused into the SoC. It is used for the
  bootblock itself.
- The customer signing key, provisioned into the secure element by the original
  HPE firmware. It is used for U-Boot, and must be present before customer
  firmware can boot.

This gives the following chain of trust. The SoC verifies the hash of an
immutable block of initial code (the bootblock) before executing it. This block
is therefore matched to a specific SoC. This code then verifies the HPE digital
signature of the second stage of code, which is also contained within the
gxp2-bootblock. The second stage reads a dedicated `u-boot-sig` section of the
SPI flash that holds the signature of U-Boot, verifies that signature against
the customer public key, and then jumps into U-Boot.

The chain of trust enforced by the bootblock therefore ends at U-Boot. Verifying
anything beyond that point is up to U-Boot itself, so the rest of the OpenBMC
image may be signed with a different key if desired.

The bootblock itself cannot be rebuilt from source, because only HPE holds the
source code and the matching private key. It is therefore consumed as a prebuilt
binary.

### Flash Layout

Two partitions of the 32 MiB flash are relevant to secure boot. Their offsets
and sizes are fixed by the SoC and must not be changed:

| Offset   | Size   | Partition       |
| -------- | ------ | --------------- |
| 32192 KB | 64 KB  | `u-boot-sig`    |
| 32256 KB | 256 KB | `gxp-bootblock` |

`u-boot-sig` is the last partition of the BMC region, which ends at 32256 KB,
and `gxp-bootblock` is the first partition of the region owned by the GXP
hardware. Note that `u-boot-sig` therefore sits at the far end of the flash
rather than next to the `u-boot` partition it describes.

The bootblock is appended to the rendered `static.mtd` image, but is
deliberately not part of any update tarball. Nothing prevents the partition from
being written at runtime, so it is treated as read-only by convention.
Overwriting it with anything other than a valid, correctly matched bootblock
leaves the system unbootable, with no recovery path from the BMC itself.

### U-Boot Signature Section

The `u-boot-sig` partition is generated at build time. It is laid out as a
20 byte header, followed by the raw signature over `u-boot.bin`, with the
remainder padded to the full 64 KB. Offsets below are relative to the start of
the partition:

```text
+--------------------------------+ 0x0000
| Header (20 bytes)              |
+--------------------------------+ 0x0014
| Signature over u-boot.bin      |
+--------------------------------+
| 0xFF padding                   |
+--------------------------------+ 0x10000
```

The header is identical for every build:

```text
00000000  24 73 74 61 72 74 24 00  24 62 6f 6f 74 31 24 00  |$start$.$boot1$.|
00000010  00 02 00 00                                       |....|
```

The signature itself is produced with the customer private key, using SHA-384 by
default. Because it covers `u-boot.bin` as a whole, the partition has to be
regenerated whenever U-Boot is rebuilt.
