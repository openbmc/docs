# Platform init

Author: Ed Tanous (edtanous)

Created: 7-21-25

## Problem Description

There are a number of platforms that require handling of state that requires
interacting with hardware in a consolidated way. Some examples of this are:

1. Resetting an i2c driver that initially is without power.
2. Bringing up a power rail under the control of the BMC.
3. Initing a power rail dependent on the state of a plug detect pin.

## Background and References

And any number of other cases. Some concrete examples of this kind of init of
these things exists here. [1]

Very routinely, significant bash scripts are used to do some of this init. Those
bash scripts suffer some minor performance penalty on startup, and are generally
checked into meta layers, reducing the reuse between platforms.

## Requirements

1. OpenBMC must be able to init a system with devices on power rails that it
   controls.
2. During the control of aformentioned power rails, the OpenBMC must be able to
   monitor GPIO signals.
3. OpenBMC must be able to reset i2c lines that might not be available.
4. (secondary) Must be able to init multiple platforms from a single binary
   (dc-scm)

## Proposed Design

Initially, a new repository would be created to house the platform-init for
gb200nvl platform, and get static analysis autobumps, and recipes reused between
platforms. Later, this repository might be extended to include bash
platform-init scripts.

Platforms will be separated by folder to ensure that platform owners can review
and create their platform configurations as required, and identical to the
meta-layer process that exists today.

Top level structure will be

platform_init.cpp platform1/init.cpp platform2/init.cpp

Initially a meson option will be used to compile the appropriate paths for a
given platform. At some point in the future, some level of detection _may_ be
added that allows detecting platforms at runtime. Design for that is to be
determined based on future requirements.

Guard rails will need to evolve over time, but initially this repository will
not interact with DBus or the the OpenBMC model. As defined (at this time) it is
purely for initial bringup of hardware.

## Alternatives Considered

Status quo, continue building init scripts in bash. This leads to a lack of code
reuse, and while some layers have done an excelent job keeping the code clean,
it's still very difficult to refactor/consolidate.

## Impacts

1. Will require a new repository, recipe, and CI build.
2. A new service will be launched for platforms opting into platform-init.

### Organizational

- Does this proposal require a new repository? Yes Request for the repository is
  available[2]
- Who will be the initial maintainer(s) of this repository? Ed Tanous
- Which repositories are expected to be modified to execute this design?
  openbmc/openbmc New repository

## Testing

Testing section omitted. Platform boot will be tested using individual
platforms, with no change.

[1]:
  https://github.com/openbmc/openbmc/blob/master/meta-nvidia/meta-gb200nvl-obmc/recipes-nvidia/platform-init/files/platform_init.cpp
[2]: https://github.com/openbmc/technical-oversight-forum/issues/51
