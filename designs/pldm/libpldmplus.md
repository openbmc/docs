# libpldmplus

Author: Alexander Hansen <alexander.hansen@9elements.com>

Other contributors: `None`

Created: September 11th, 2025

## Problem Description

The `pldm` specification and `libpldm` have gained wider adoption in OpenBMC and
other embedded Linux environments. Because of the strict requirements on
`libpldm` (must be written in C, must not allocate, must have a stable ABI,
...), library users have developed abstractions around `libpldm` which make it
more convenient and easier to use from Linux userspaces where allocation and C++
are available.

One such example is the `PackageParser` abstraction which wraps the pldm fw
update package parsing features of `libpldm` and returns a heap object for later
processing.

The observed problem is that the `PackageParser` code is now duplicated between
2 repositories

- [pldm](https://github.com/openbmc/pldm/blob/505542588031c26ce7918ad4d35aacf6bc3b80b9/fw-update/package_parser.hpp)
- [phosphor-bmc-code-mgmt](https://github.com/openbmc/phosphor-bmc-code-mgmt/blob/2e168dba1029d1c5baaf065ad7306150dd3cafc2/common/pldm/package_parser.hpp)

Anyone wanting to use those abstractions from outside OpenBMC would also need to
duplicate it again.

To solve this issue there has been an
[effort](https://gerrit.openbmc.org/c/openbmc/libpldm/+/77095) to develop a C++
binding for libpldm.

However 8 months later, the author of that patch is still failing to meet the
expectation for how such a binding should look like.

## Background and References

- [Patch for libpldm C++ binding](https://gerrit.openbmc.org/c/openbmc/libpldm/+/77095)
- [pldm PackageParser](https://github.com/openbmc/pldm/blob/505542588031c26ce7918ad4d35aacf6bc3b80b9/fw-update/package_parser.hpp)
- [phosphor-bmc-code-mgmt PackageParser](https://github.com/openbmc/phosphor-bmc-code-mgmt/blob/2e168dba1029d1c5baaf065ad7306150dd3cafc2/common/pldm/package_parser.hpp)
- [D-pointer](https://wiki.qt.io/D-Pointer)

## Requirements

The requirement is an API which is capable to parse PLDM fw update package, with
following properties:

- no macros / raw pointers needed after package has been parsed
- easy to use without boilerplate, preferably 1 function call for parsing
- solution should be able to allocate for later processing, and not require all
  processing to be done in a single function.
- covers all use-cases for the currently duplicated `PackageParser`
- extensible to support future changes in package format

### optional-requirements

- ABI stability (in that older clients can use newer versions of the library).
  In the context of OpenBMC this is usually not done, but not too difficult to
  at least attempt it to enhance the solution.

## Proposed Design

Implementation of a new shared library `libpldmplus` in a separate repository.

The implementation scope is any repeated or boilerplate code which occurs when
using libpldm from a C++ context.

The initial use-case is package parsing. fw update package building could be a
future use-case.

The library will depend on `libpldm` . The goal is to make the most use of
anything `libpldm` can provide instead of re-implementing details from
specifications.

For ABI stability, the approach taken by `libpldm` can be applied, by using
`abi-dumper` and `abi-compliance-checker`.

## Alternatives Considered

### 1: continue trying to implement a C++ binding in libpldm

Due to the strict quality requirements on libpldm components, nothing has been
merged in 8 months, and it is not clear if the author will ever be able to come
up with a good solution.

The project needs to come to a solution in a _reasonable_ timeframe, with an
_acceptable_ level of quality, which is provided by the separate repository.

(note: the design author usually tries his best to avoid creating any
unnecessary extra repositories)

### 2: make `PackageParser` users depend on each other

Either make `phosphor-bmc-code-mgmt` depend on `pldm` or the reverse. This
approach is discarded since it could introduce unnecessary dependencies, and
both repos have runtime/daemon components.

## Impacts

API impact

- new library is available. 2 existing users will be migrated.

Security impact

- libpldm APIs must be utilized to handle any raw pointers and any direct
  interaction with the fw package should be through libpldm. This should avoid
  security issues.
- Any fixes done by libpldm will not be delayed through this project since both
  libraries are dynamically linked.

Documentation impact

- usage of `libpldmplus` will be documented

Performance impact

- since the initial use-case (fw update) is infrequent, and the package header
  is small in relation to the actual component images, performance impact will
  be minimal.

### Organizational

- Does this proposal require a new repository? Yes
- Who will be the initial maintainer(s) of this repository? Alexander Hansen
  - open to having someone else maintain it, if there is any interest
- Which repositories are expected to be modified to execute this design?
  - pldm
  - phosphor-bmc-code-mgmt
- Make a list, and add listed repository maintainers to the gerrit review.

## Testing

library APIs will be unit-tested directly as part of the repository. They may be
tested indirectly (integration test) in `phosphor-bmc-code-mgmt`. Unit tests are
already available.
