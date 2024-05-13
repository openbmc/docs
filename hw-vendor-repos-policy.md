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

OpenBMC wants to enable as many machines as possible with as many common paths
shared between machines as possible, under the hope of keeping stability in its
implementation between systems.

Within OpenBMC there are two areas of "vendor specific" repositories:

- A Hardware specific feature (implementation for a driver, support for
  hardware, or other things that only exist on that platform).
- Support for a vendor-specific feature. (OEM extensions, debug APIs, Redfish,
  etc).

Yocto differentiates between the above with the "machine" and "distro" concepts.
We need a policy within OpenBMC on how we handle each of these types of
features.

The OpenBMC project has a fairly extensive CI process, it has many great code
reviewers, and it follows the tip of upstream yocto master. All of these things
ensure a solid code base for others to use, and sets a certain standard for
repositories that are hosted within OpenBMC. Having the source under the OpenBMC
umbrella ensure it gets global updates (like upstream poky/bitbake changes) and
other things like the latest compiler.

There are times where big updates come in from yocto that break multiple
repositories utilized by OpenBMC. It is always going to be easier to coordinate
big changes like that on repositories hosted within OpenBMC directly (vs. trying
to coordinate with externally hosted repositories.)

OpenBMC already hosts 100+ repositories, which can be very daunting to someone
new joining the project. New repositories should be added in the case where they
can be maintained over the long term without taking resources away from other
project efforts. Having a set of guidelines for this process helps to speed up
new code development within the project. How useful is the feature, how many
other people could make use of it, how active has the person who will maintain
the new repository been in the community? These are just a few of the questions
we need to articulate into a vetting process.

In a perfect world, only hardware-specific features would be in the meta machine
layers, and vendor specific features should be in distro features. OpenBMC is
not there yet but is something the community should stive towards.

## Requirements

Note that there is an expectation the reader of this document is familiar with
OpenBMC architecture (bitbake, d-bus, phosphor-dbus-interfaces, ...).

Writing hard rules for anything is hard without defeating intent, these are
guidelines to be used by the OpenBMC TOF in their decision to approve or not
approve new recipes or repos.

Both hardware-specific and vendor-specific repositories should meet the
following requirements to be eligible for a repository under OpenBMC:

1. It is compatible with the Apache 2.0 license
2. If an existing repository, it has an active maintainer
3. It has no external dependencies which would fail the requirements defined
   within this document
4. The new repository will not break others (i.e. providing new API's utilized
   by a common repository that are not protected by compile flags, etc)
5. It is a feature that is believed to be generally useful to the OpenBMC
   organization
   - Determination made by the TOF
   - Reference: "Create a new repository" TOF [process][tof]
6. It has an active OpenBMC community member that is willing to be a maintainer

When deciding whether a hardware or vendor-specific repository should be hosted
under OpenBMC or whether a bitbake recipe should be added to point to an
external repository, the following should be considered:

1. It should meet all of the quality-based requirements for hosting a repository
   within OpenBMC
2. If the repository is utilized by other projects outside of OpenBMC then it
   may be better suited outside of OpenBMC and utilized via a bitbake recipe

Note that when a recipe is non-OpenBMC specific, but useful to OpenBMC, it
should be pushed upstream into meta-openembedded.

### Hardware Specific Feature

A hardware vendor feature is defined as one of the following:

1. A function that is specific to the vendor hardware
   - For example, PECI, FSI, hardware diagnostics (specific to the processor)
2. A device attached to the BMC (MCTP/PLDM/I2C/...) that is specific to the
   vendor which the BMC must interact with

To add a hardware specific repository to OpenBMC, it should meet the following
additional criteria:

1. If it requires specific kernel API's, they are available or in progress via
   the [upstream][upstream] guidelines

### Vendor Specific Feature

A vendor-specific feature is defined as one of the following:

1. A vendor-specific data format or protocol
   - For example, a dump collection of the host firmware, or a special format of
     SPD/VPD
2. A function that requires a specific hardware vendor feature

To add a vendor-specific repository to OpenBMC, it should meet the following
additional criteria:

1. It consumes OpenBMC D-Bus API's
   - This indicates a synergy between this feature and OpenBMC

## Conclusion

This document was written to be a reference for people looking where to put
vendor specific features and also as a reference for the TOF team to review when
assessing a request for a vendor specific repository within OpenBMC.

[openpower]: https://openpowerfoundation.org/
[upstream]: https://github.com/openbmc/docs/blob/master/kernel-development.md
[tof]: https://github.com/openbmc/technical-oversight-forum/issues
