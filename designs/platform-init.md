# Platform init

Author: Ed Tanous (edtanous)

Other contributors: Oliver Brewka (olek)

Created: 7-21-25 Updated: 6-15-26

## Problem Description

There are a number of platforms that require handling of state that requires
interacting with hardware in a consolidated way. Some examples of this are:

1. Resetting an i2c driver that initially is without power.
2. Bringing up a power rail under the control of the BMC.
3. Initing a power rail dependent on the state of a plug detect pin.

## Background and References

Very routinely, significant bash scripts are used to do some of this init. Those
bash scripts suffer some minor performance penalty on startup, and are generally
checked into meta layers, reducing the reuse between platforms.

## Requirements

1. OpenBMC must be able to init a system with devices on power rails that it
   controls.
2. During control of the aforementioned power rails, OpenBMC must be able to
   monitor GPIO signals.
3. OpenBMC must be able to reset i2c lines that might not be available.
4. (secondary) Must be able to init multiple platforms from a single binary
   (dc-scm)

## Proposed Design

Initially, a new repository would be created to house the platform-init for
gb200nvl platform, and get static analysis autobumps, and recipes reused between
platforms.

Platforms will be separated by folder to ensure that platform owners can review
and create their platform configurations as required, and identical to the
meta-layer process that exists today.

Top level structure will be

platform_init.cpp platform1/init.cpp platform2/init.cpp

Functionality of the platform binary may be expanded through cli subcommands.

A subcommand may be added if it checks following criteria:

1. The subcommand executes a common platform functionality that solves a problem
   as described above
2. The subcommand requires similar project and code structure. Creating a
   separate repository would duplicate a lot of what this repository already
   provides.
3. The subcommand is designed to be common code
4. The subcommand is designed to expect a single function specific to the
   platform.

Having these subcommand requirements should help to maintain a uniform software
architecture and allow for future enhancements to the build process.

Platform hardware initialization is executed through the init subcommand.

Initially a cli argument will be used to determine the correct platform function
at runtime.

At some point in the future, some level of detection _may_ be added that allows
detecting platforms at runtime. Design for that is to be determined based on
future requirements.

Further guard rails will need to evolve over time.

## Alternatives Considered

Status quo, continue building init scripts in bash. This leads to a lack of code
reuse, and while some layers have done an excellent job keeping the code clean,
it's still very difficult to refactor/consolidate.

## Impacts

1. Will require a new repository, recipe, and CI build.
2. A new service will be launched for platforms opting into platform-init.

### Organizational

- Does this proposal require a new repository? Yes Request for the repository is
  available[1]
- Who will be the initial maintainer(s) of this repository? Ed Tanous and
  Patrick Williams
- Which repositories are expected to be modified to execute this design?
  openbmc/openbmc New repository

## Testing

Testing section omitted. Platform boot will be tested using individual
platforms, with no change.

[1]: https://github.com/openbmc/technical-oversight-forum/issues/51
