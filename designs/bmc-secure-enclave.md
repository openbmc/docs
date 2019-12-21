# Intel BMC Secure Enclave

Author: Vernon Mauery  !vmauery

Primary assignee: Vernon Mauery  !vmauery

Other contributors: Nilan Naido, James Mihm, Terry Duncan

Created: 2019-12-09

## Problem Description

High level design and scoping of Secure Enclave for openBMC

## Background and References

[Secure Enclave Requirements](https://cosmos8.intel.com/review.req#/r:REV-24030)
> The BMC shall enable the ARM TrustZone hardware security extension techonology
> on the BMC SoC , which aims to provide secure execution environment by
> splitting computer resources between two execution worlds, namely normal world
> and secure world.

[TrustZone Information](http://infocenter.arm.com/help/topic/com.arm.doc.ddi0464f/DDI0464F_cortex_a7_mpcore_r0p5_trm.pdf) (Details in the Cortex-A7 Technical Reference Manual)
[OP-TEE Documentation](https://optee.readthedocs.io/en/latest/general/index.html)
[ARM Trusted Firmware Documentation](https://trustedfirmware-a.readthedocs.io/en/latest/)

[OP-TEE Code](https://github.com/OP-TEE)
[ARM Trusted Firmware Code](https://github.com/ARM-software/arm-trusted-firmware)

### Components

Secure Boot
Trusted Firmware A
Trusted Execution Environment (TEE) (OP-TEE)
Trusted Monitor
Rich Execution Environment (REE) (Linux Runtime)
Trusted Applications (apps that run in the TEE context on behalf of the REE)

## Requirements
Provide a mechanism to secure BMC and end-customer secrets.

## Proposed Design

### Background Info
Execution modes (ARM core execution contexts/privileges)
```
EL0 - userspace
EL1 - kernel
EL2 - hypervisor
EL3 - monitor
```

secure boot on arm:
```
  #================#         +-----------------------+
  # ROM Bootloader #         |  ARM Trusted Firmware |
  # PFR or SoC     ---------->  TF-A                 |
  # Root of Trust  #         |  BL1              EL3 |
  #================#         +-----------|-----------+
                                         |
                                         |
                             +-----------v-----------+
                             | ARM Trusted Firmware  |
            +----------------- TF-A                  ----------------+
            |                | BL2               EL3 |               |
            |                +-----------|-----------+               |
            |                            |                           |
            |                            |                           |
+-----------v-----------+    +-----------v-----------+    +----------v---------+
|  ARM Trusted Firmware |    |  Trusted Execution    |    |  Bootloader        |
|  Monitor              |    |  Environment (TEE)    |    |  U-Boot            |
|  BL21             EL3 |    |  OP-TEE               |    |  BL23          EL1 |
+-----------------------+    |  BL22             EL2 |    +----------|---------+
                             +-----------------------+               |
                                         |                           |
                                         .                +----------v---------+
                                         |                |  Linux Kernel      |
                                         .                |                EL1 |
                                         |                +----------|---------+
                                         .                           |             +------------------+
                                         |                           |             |  Linux Userspace |
                             +-----------v-----------+               |             |                  |
                             |  Trusted Application  |               +------------->                  |
                             |  (Custom work)        |                             |                  |
                             |                   EL2 |<- - - - - - - - - - - - - - -              EL0 |
                             +-----------------------+                             +------------------+
```
1. ROM boot code stage 1 (PFR or AST2600 secure boot ROM) authenticate BL1
2. BL1 authenticates BL2 and boots it
3. BL2 authenticates and loads trusted world kernel (BL32),
   trusted world monitor (BL31), and REE bootloader (BL33) (e.g., U-boot)
4. U-Boot authenticates and loads Linux Kernel (and runtime)
5. Linux Userspace makes calls (via the monitor) to TEE for services provided
   by Trusted Applications


NOTES:
* During investigation, it was discovered that not all ARM processors that
  support TrustZone can securely run a TEE. It was discovered that the
  Raspberry Pi 3B was incapable of being secure, despite having TrustZone in
  its ARM core.  More work needs to be done to determine of AST2600 will be
  secure.
* Part of the BMC security features will need to be derived from the AST2600
  features that are not fully tested yet


### Ideas for what the 'secure enclave' might provide via Trusted Applications:
* secure storage
  * secure storage of HMAC keys (for IPMI passwords/RMCP+ HMAC keys)
  * secure storage of secret keys (AES encryption/decryption)
  * secure storage of private keys (RSA/ECDSA for web, ...)
* HSM-like operations via CHIL API (OpenSSL HW engine support)
  * AES encryption/decryption
  * RSA/ECDSA encryption/decryption/signing/verification
  * random number generation
  * HMAC operations (for RMCP+)
* Attestation of BMC FW? (because the TEE is smaller, more trusted, and harder to attack)
* firmware updates
  * secures flash access (updating BMC FW)
  * secures i2c access (updating satellite controller FW)
* OAuth (via HOTP/TOTP)

## Alternatives Considered
* Custom TEE (not really an option; don't write your own security primitives)
* Trusty TEE (not a strong contender because we are not running android OS)
* Custom non-TEE less-secure 'enclave' (not a contender at all)

## Impacts and Dependencies
* Requires strong guarantees on secure boot or it makes no sense
  * PFR booting directly to BL2 would work
  * Aspeed AST2600 secure boot to BL1 would work
* Some impact on boot times because of the BL1->BL2->BL31/BL32/BL23->BL33 chain
* Some impact on performance (likely very low overhead during runtime)


## Estimation of work
* Initial secure boot with TEE/REE - Effort 16 Weeks
* Testing and Debug - Effort 4 Weeks (for initial secure boot)
* Implementation of Trusted Applications - Effort 2-4 Weeks per application, depending on complexity
* Testing and Debug - Effort 2 Weeks (per trusted application)
