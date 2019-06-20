# AST2600 Storage Design

Author: Adriana Kobylak < anoo! >

Primary assignee: Adriana Kobylak

Other contributors: Joel Stanley < shenki! >

Created: 2019-06-20

## Problem Description
Need to determine the storage design for the AST2600-based platforms with an
eMMC device.

## Background and References
None

## Requirements
- Security: Ability to enforce read-only, verfication of official/signed
images for production.
- Updatable: Ensure that the filesystem design allows for an effective and
simple update mechanism to be implemented.
- Simplicity: Make the system easy to understand, so that it is easy to
develop, test, and use.
- Code reuse: Try to use something that already exists instead of re-inventing
the wheel.

## Proposed Design
- Filesystem: ext4 on LVM volumes. This allows for dynamic partition/removal,
similar to the current UBI implementation.

## Alternatives Considered
- Filesystem: f2fs. Work would be needed to compare it against ext4 for OpenBMC.

## Impacts
This design would impact the OpenBMC build process and code update
internal implementations, but should not affect the external interfaces.

## Testing
A system would be brought up. It could be added to the CI pool.
