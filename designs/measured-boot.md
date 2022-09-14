# Measured boot of BMC

Author: Sandhya Koteshwara <skoteshwara#2780>

Other contributors: Gheorghe Almasi, Angelo Ruocco, Ruud Haring

Created: September 12, 2022

## Problem Description
OpenBMC does not currently support measured boot using a Trusted Platform
Module (TPM).

The goal of this design document is to describe the changes required to support
measured boot, in compliance with the [TCG Server Management Domain Firmware
Profile Specification][]. This includes:
1. Initialization of the TPM device during system startup.
2. Creation of a boot log that includes the entire booted software stack from
   the moment of the TPM initialization.
3. Extending measurements to the Platform Configuration Registers (PCR) of the
   TPM during every stage of the boot process.
4. Making said boot log available to the OpenBMC Linux kernel at the end of
   the boot process.

[TCG Server Management Domain Firmware Profile Specification]: https://trustedcomputinggroup.org/wp-content/uploads/TCG_ServerManagementDomainFirmwareProfile_v1p00_11aug2020.pdf

In a separate design document we will describe [Remote Attestation][]: a
process that is performed after a measured boot, to retrieve and verify boot
logs from the BMC and quotes from the TPM.

[Remote Attestation]: https://link-to-gerrit-design. <--- TODO after upload

## Background and References
The [Host Measured Boot][] process is well known and extensively documented. It
covers the CPU boot from the first UEFI/BIOS stage through the booting of the
hypervisor / operating system, creating boot logs and storing measurements
(hashes) into a Trusted Platform Module ([TPM 2.0][]) attached to the CPU. The
logs and measurements are made available to the booted kernel at the end of the
boot process and can be retrieved by user space processes for the purpose of
Remote Attestation, which serves to ensure to stakeholders that the CPU booted
exactly the specified software (and nothing else or different).

Under auspices of the Trusted Computing Group, the industry is extending this
same mechanism to booting the [Server Management Domain][], i.e. the BMC and
associated chips/firmware, with measurements stored in a [TPM 2.0][] attached
to the BMC.

[Host Measured boot]: https://trustedcomputinggroup.org/resource/pc-client-specific-platform-firmware-profile-specification/
[Server Management Domain]: https://trustedcomputinggroup.org/wp-content/uploads/TCG_ServerManagementDomainFirmwareProfile_v1p00_11aug2020.pdf
[TPM 2.0]: https://trustedcomputinggroup.org/resource/tpm-library-specification/

Measured Boot is distinct from, and orthogonal to, Secure Boot. Secure Boot is
active: it measures firmware images for provenance (encrypted signature of
originator) and integrity (hash checking) before the image is loaded. If it
fails, it invokes a recovery procedure. Secure boot for AST2600 is outlined in
a separate [Secure Boot design document][].

[Secure Boot design document]: https://gerrit.openbmc.org/c/openbmc/docs/+/26169

Measured Boot is passive: measurements and logs are taken while the software
goes through the boot stages, but no determination is made during boot of
whether the measurements are "right". The measurements are stored in the boot
log for later access, and the digests of the measurements are stored in the
TPM for purposes of authenticating the boot log.

Measured Boot is also more expansive than secure boot: measurements cover not
only the firmware image itself, but can also cover configuration parameters,
peripherals, etc. The logs tell a verbose story of the boot process.

The salient features of Measured Boot are:
- "root of trust" -- the TPM device must be initialized by secure, preferably
  immutable, code early in the boot process. This provides the first entry in
  the boot log.
- "chain of trust" -- each booted software component is _measured_ before it
  runs. Since the individual measurements are authenticated by the TPM device,
  this ensures that no software can distort measurements of itself
  retroactively.

## Requirements
The chief implementation constraint for OpenBMC/Measured Boot is the location
of the "root of trust" (RoT), i.e. the component that initializes the TPM
device and provides the first measurement called the "Static Core Root of
Trust for Measurement" (S-CRTM). Security considerations dictate that this
process happens as early in the boot process as possible (as tampering of
software before the RoT would be undetected).

The implementation should:
- Provide a hardware-based RoT which can initialize the TPM as early as
  possible in the boot process;
- Provide boot logs at various stages of the firmware including the bootloader
  and kernel;
- Extension of measurement digests to PCRs of the TPM, according to the [TCG
  Server Management Domain Firmware Profile Specification][].

Measured boot is typically implemented with later processing in mind. This
processing is usually remote -- meaning that a (user space) agent on the
booted node retrieves the (authenticated) TPM PCR registers and relevant logs
(including the boot log) and ships them for remote processing. This is "remote
attestation" (mentioned earlier); it makes it possible to ascertain the exact
software running on the booted node without having to trust said software.
Remote attestation imposes some processing and bandwidth requirements. On the
agent side processing overhead is minimal, and communication overhead is in
the 15-25 kBytes range per attestation event.

## Proposed Design
Currently, the BMC firmware boot process is made up of four stages:
1. Immutable code;
2. uboot SPL (secondary program loader);
3. uboot;
4. BMC Linux.

This design assumes the availability of the following hardware implementation
at a minimum:
- Secure boot is enabled starting from the immutable code and
  extended to include u-boot-spl, u-boot and Linux kernel
- TPM attached to BMC (either SPI or i2c)

Constraints:
- The immutable code (for example, ROM in AST2600) is closed
  source or vendor dependent, and inaccessible to OpenBMC developers.
- SPL has a 64k code size limit that greatly limits our ability to write a
  driver for TPM devices and the functions to start the measured boot process.

Hence, the earliest we can currently create a RoT for measured boot (including
TPM initialization and S-CRTM) is stage 3: uboot. Mainline uboot already has
the TPM driver and the measured boot process in it, so it's just a matter of
making use of it in OpenBMC.

The necessary patches to yocto to achieve this are:
- Fast-forward the uboot branch in OpenBMC so that it uses tag v2022.07 or
  newer, either modifying u-boot poky recipe's SRCREV
  (meta/recipes-bsp/u-boot/u-boot-common.inc) or having it being overridden
  by a .bbappend in openbmc (meta-phosphor/recipes-bsp/u-boot);
- Edit the yocto kernel fit image bbclass (meta/classes/kernel-fitimage) to
  allow for fit images to boot EFI Operating Systems;
- Add a new .bbappend recipe for kernel config to enable EFI and SECUREFS
  support (meta-phosphor/recipes-kernel); following the instructions from
  [UEFI on u-boot][].
- Add a new "distro feature", namely `measured_boot`, that will trigger all
  necessary changes (EFI OS for u-boot and bbappend file for kernel config);
- (Optional) Change the machine description we want to use measured boot on,
  to use the `measured_boot` distro feature.

[UEFI on u-boot]: https://u-boot.readthedocs.io/en/latest/develop/uefi/uefi.html

## Alternatives Considered
For UEFI, an end to end implementation of measured boot and attestation has
already been implemented. Most of the UEFI design can be mirrored to the BMC
side.

An implementation of measured boot exists in [ARM Trusted Firmware][]. However,
the support for a particular BMC vendor may or may not be available in [TF-A][].
Even if support were to be included, it is not clear if the vendor proprietary
firmware can be swapped.

[ARM Trusted Firmware]: https://review.trustedfirmware.org/q/measured-boot.
[TF-A]: https://github.com/ARM-software/arm-trusted-firmware/tree/master/plat

## Impacts
- API impact? N/A

- Security impact? Measured Boot will assure that the end user is running on a
  machine that has booted the exact code that they have specified -- i.e.
  there has been no tampering.

- Documentation impact? The above quoted documents should suffice. If any
  OpenBMC-specific documents need to be written, the team will provide.

- Performance impact? Minimal boot time performance impacts expected for
  signature validation and interaction with the TPM, and only when adding
  measured_boot as a distro feature. None if disabled.

- Developer impact? N/A

- Upgradability impact? N/A

### Organizational
Does this repository require a new repository? No
Who will be the initial maintainer(s) of this repository? N/A

## Testing
Test scenarios:
- No loss of function: Measured boot enabled and BMC boots successfully
  (measured boot does not impact BMC functionality)
- Basic functional test I ("boot log exists"): a measured boot log exists, and
  can be retrieved, on the booted system. The measured boot log *only* exists
  if the system is booted with measured boot enabled.
- Basic functional test II ("boot log is not garbage"): the measured boot log
  is syntactically compliant with the TCG standard. (tested with the
  tpm2-tools, like "tpm2_eventlog <log>")
- Basic functional test III ("boot log is standards compliant"): the measured
  boot log contains entries for firmware, operating system, secure boot status
  etc. as defined by the TCG standard.
- Basic functional test IV ("boot log contains expected values"): the measured
  boot log entries correspond to expected firmware, O/S etc. states. The
  measured boot log *changes* appropriately if the system is not secure booted,
  or is booted with different components.
- [Optional] Integration test: running a remote attestation agent on the BMC,
  and enabling measured boot attestation, results in a pass if the correct
  entries are supplied to verifier, and conversely a failure if incorrect
  entries are supplied.

We expect to be providing CI testing for these scenarios (excluding the
integration test, since that implies work that is in another proposal).
We are running such testing on UEFI based systems today.
