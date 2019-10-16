# Secure/verified and trusted boot for Aspeed BMC

Author: Chris Engel

Primary assignee: Chris or Joel?
  < Name and/or IRC nic or None >

Other contributors: Joel Stanley, Joseph Reynolds

Created: 2019-08-08

## Problem Description
OpenBMC doesn't currently support secure/verified boot for Aspeed BMC HW.  This
includes the lack of support for existing U-boot verified boot.

The goal of this design document is to describe the changes required to
support secure/verified boot using Aspeed 2600 as the CRT (Core Root of Trust)
and extending into U-boot and the Linux kernel using U-boot verified boot.

An additional optional goal of this design is to allow changing the secure boot
public key in a secure environment.  This allows the system owner to transition
control of as much of the FW stack as possible away from the system vendor if
they desire.  This feature also enables a 'development' secure boot key which
is a well known key pair used during firmware development.  However when this
feature is enabled it is critical that the system owner can detect what keys
were used to validate the FW stack in a secure fashion.  That is achieved through
measuring the secure boot key into the TPM which can then be verified through
attestation.


## Background and References
U-boot supports verified boot which can perform cryptographic validation of
the kernel [1].  The Aspeed AST2600 BMC also adds support for a HW ROM based
verification.  The combination of these two features allows us to create a
chain of trust that extends from immutable HW into the Linux kernel.

Facebook added verified boot to their OpenBMC flavor which some information
can be seen here [2].

[1] Verified U-Boot : https://lwn.net/Articles/571031/
[2] https://github.com/facebook/openbmc/tree/helium/tests/verified-boot


## Requirements

The implementation should:
- Provide a HW based ROT (Root Of Trust) with a chain of trust that extends
  into the Linux kernel and initrd.
- Provide TPM support for remote attestation with a CRTM (Core Root of Trust
  for Measurement) that includes U-Boot SPL.
- (Optional) Provide the ability for the system owner to transition control
  of the FW stack to a different set of secure boot keys.
   - Physical presence required to transition control.
   - TPM support required when this feature is enabled to allow verification
     of public secure boot keys used through TPM attestation.
- (Optional) Provide the ability to disable secure boot of some portions of the
  FW stack for debug and recovery through assertion of physical presence.

## Proposed Design
To support this design it requires the following HW implementation at a minimum:
- Aspeed AST2600:
  - First layer U-Boot SPL verification public keys in OTP (One Time
    Programmable) memory.
  - Secure boot enabled in OTP memory.
  - OTP configuration and secure regions write protected.
  - NOR/eMMC flash attached for FW stack.
  - (Optional) TPM attached (either SPI or i2c).
  - (Optional) External security jumper tied to BMC GPIO.
    - Board wiring must enforce GPIO as input to not allow BMC to drive
      false physical presence.
  - (Optional) EEProm attached for second layer U-Boot/Linux public key storage.
    - EEProm write enable pin tied to security jumper with default state write
      disabled.

The basic design flow without the optional key transition support is the
following :
- AST2600 load U-Boot SPL into internal SRAM and verifies against keys in OTP
  memory and starts execution.
- (Optional) U-Boot SPL initializes TPM.
- (Optional) U-Boot SPL measures itself and extends into TPM.
- U-Boot SPL has second layer public key built in which is used for verification
  of the remainder of the stack
- U-Boot SPL loads full U-boot and verifies, measures and starts execution.
- Full U-Boot loads Linux kernel/initrd and verifies, measures and starts
  execution.
  - Optionally U-boot could decide to skip verification based on security
    jumper.

The basic design flow with key transition support is the following :
- AST2600 load U-Boot SPL into internal SRAM and verifies against keys in OTP
  memory and starts execution.
- U-Boot SPL initializes TPM.
- U-Boot SPL measures itself and extends into TPM.
- U-Boot SPL loads second layer public key from EEProm, measures into TPM.
- U-Boot SPL extends state of security jumper sensed via GPIO into TPM.
- U-Boot SPL loads full U-boot and verifies, measures and starts execution.
  - Optionally SPL could decide to skip verification based on security jumper.
- Full U-Boot loads Linux kernel/initrd and verifies, measures and starts
  execution.
  - Optionally U-boot could decide to skip verification based on security
    jumper.


## Alternatives Considered
None

## Impacts
- Minimal boot time performance impacts expected for signature validation and
  interaction with the TPM.
- Development teams will need to plan to allow developers to build and sign
  development images using non-production "development" keys.
- Bringup HW must be provisioned with both development and production
  verification keys.
- System manufacturing must add processes to provision the BMC with production
  and development keys.


## Testing

Test scenarios:
- Full secure boot enabled and BMC boots successfully
- Invalid U-Boot SPL signature to verify AST2600 failover to backup image
- Invalid U-Boot Full signature to verify failover to backup image
  - When security jumper is set to disable security BMC must boot successfully
- Invalid Kernel signature to verify failover to backup image
