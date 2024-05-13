# Hardware Vendor Repository Policy

Author: Andrew Geissler @geissonator

Other contributors:

Created: May 13, 2024

## Background and References

As OpenBMC has grown, so has the need for software function that is tied to
specific hardware families. For example, [OpenPOWER][openpower] was one of the
first users of OpenBMC and a search for "openpower" in the openbmc github
organization will show many repositories with that name in it. You will get a
similar result if you search for "intel".

OpenBMC in general is to be agnostic of the hardware it is managing, but that is
not always possible due to specific features or functions only provided by
certain hardware.

For example, OpenPOWER systems have a separate power management device within
the processor called an OCC. This OCC has its own protocol and functions. This
is why OpenBMC has a openpower-occ-control repository.

Another example, Intel has a communication interface called PECI, it provides
useful information (like temperatures) but only on Intel based systems. This is
why OpenBMC has a peci-pcie repository.

The goal of this document is to clearly state the requirements and expectations
for hosting a hardware/vendor specific repository within the openbmc github
organization. This document aims to answer the following questions:

1. Where does the OpenBMC community draw the line between what goes into the
   OpenBMC organization and what should not?

2. If a repository doesn't belong in the OpenBMC organization, what determines
   if a bitbake recipe can be added to OpenBMC to point to a vendors repository
   outside of the OpenBMC org?

This document is based on a TOF meeting on Jan 9th, 2024.

## Philosophy

The OpenBMC project wants to support as many machines as possible but also wants
to be agnostic of the hardware as much as possible. Being agnostic means more
people use and contribute to the code, resulting in a better software stack.

But the more systems OpenBMC supports, even if those systems have some
vendor-specific features, the more reference systems people have to start with
and the more potential for community growth.

Both a better software stack and community growth are goals of OpenBMC.

The OpenBMC project has a fairly extensive CI process, it has many great code
reviewers, and it follows the tip of upstream yocto master. All of these things
ensure a solid code base for others to use, and sets a certain standard for
repositories that are hosted within OpenBMC. Having the source under the OpenBMC
umbrella ensure it gets global updates (like upstream poky/bitbake changes) and
other things like the latest compiler and clang features. It also provides a way
for admins of the OpenBMC project to quickly push through a critical change if a
maintainer is unavailable.

OpenBMC already hosts 100+ repositories, which can be very daunting to someone
new joining the project. New repositories can not just be added without a
certain process in place to vet them. How useful is the feature, how many other
people could make use of it, how active has the person who will maintain the new
repository been in the community? These are just a few of the questions we need
to articulate into a vetting process.

## Requirements

Note that there is an expectation the reader of this document is familiar with
OpenBMC architecture (bitbake, d-bus, phosphor-dbus-interfaces, ...).

A hardware vendor feature is defined as one of the following:

1. A function that is specific to the vendor hardware
   - For example, PECI, FSI, hardware diagnostics (specific to the processor)
2. A device attached to the BMC (MCTP/PLDM/I2C/...) that is specific to the
   vendor which the BMC must interact with
3. A vendor specific data format
   - For example, a dump collection of the host firmware, or a special format of
     SPD/VPD

A hardware vendor feature should remain outside of OpenBMC if any of the
following are true:

1. It exposes D-Bus interfaces but does not consume any OpenBMC D-Bus interfaces
   - There is some wiggle room here, depending on whether it still provides a
     useful feature for users of the hardware vendor
2. It is only a library exposing hardware APIs for applications to utilize
3. It is not compatible with the Apache 2.0 license
4. It requires kernel API's which are not [upstream][upstream]
5. If an existing repository, it has a history of poor maintainership

A hardware vendor feature could be added to the OpenBMC organization if the
following are true:

1. It is a feature that the TOF finds generally useful
   - Gone through the "Create a new repository" TOF [process][tof]
2. It consumes OpenBMC D-Bus API's
3. It has an active member of the OpenBMC community to be its maintainer

A hardware vendor feature could be added to the meta-\<vendor\> layer of
openbmc/openbmc as a bitbake recipe if the following are true:

1. It is utilized by projects outside of OpenBMC
   - If it is only for OpenBMC then it should be hosted within OpenBMC
2. It is a well maintained and active repository
   - No forced pushes or deletion of critical branches
3. Has a maintainer who is active within the OpenBMC community and readily
   available when an issue is found

Note that when a recipe is non-OpenBMC specific, but useful to OpenBMC, it
should be pushed upstream into meta-openembedded.

## Conclusion

This document was written to be a reference for people looking where to put
vendor specific features and also as a reference for the TOF team to review when
assessing a request for a vendor specific repository within OpenBMC.

[openpower]: https://openpowerfoundation.org/
[upstream]: https://github.com/openbmc/docs/blob/master/kernel-development.md
[tof]: https://github.com/openbmc/technical-oversight-forum/issues
