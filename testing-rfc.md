# OpenBMC Testing (for Google) Design

Authors:
Nancy Yuen <[yuenn@google.com](mailto:yuenn@google.com)>,
Brendan Higgins <[brendanhiggins@google.com](mailto:brendanhiggins@google.com)>

[TOC]

## Objective

Define the types of tests, and the related build, testing and automation
infrastructure for testing OpenBMC components and images, and systems running
OpenBMC.  OpenBMC is a full software stack that includes a boot loader,
operating system and applications.  All of it runs on a variety of platforms
(x86, PowerPC).  The OpenBMC test plan incorporates a variety of types of tests
that are run in different scenarios in order to gain confidence in the
stability, and correctness of the full software stack running on supported
platforms.  The OpenBMC test plan also identifies automation of various testing
and related processes.

## Overview

In the long term, there is a lot of infrastructure needed to thoroughly test
OpenBMC.  OpenBMC has many unique elements that have different requirements for
testing.  And OpenBMC is a complex software stack that is meant to be integrated
as part of a larger whole (server) which also translates to a variety of testing
requirements.

However, most of the testing features with components, unit testing, Gerrit
presubmit, and the accompanying infrastructure can be provided in the short
term. Most of the underlying infrastructure for unit testing is already present;
the largest body of work that remains is providing a consistent mechanism for
removing external dependencies. Once unit tests can be run locally, it will not
require much more effort integrate these tests into Jenkins and post the results
to Gerrit automatically. Integration and end-to-end testing are greatly
simplified once those pieces are available.

## Detailed Design

### Types of Tests

#### Unit Testing

Components roughly correspond to a daemon/repo but it doesn't have to.  Unit
tests test individual components.  Unit tests are a critical form of testing
used by developers, helping them iterate more quickly and facilitating code
reviews.  Thus ideally, these tests should be:

*   runnable locally on a developer workstation
*   individual tests should build and run quickly, on the order of tens of
    seconds.

It would also be very helpful to include the %of code coverage achieved by the
unit tests in each component.

Some challenges:

*   How to address issue of dependencies and mocking?

#### End To End Testing

Tests involving multiple components interacting within the BMC.  They verify the
correctness of the interaction between components and provide a sanity check
that the interfaces between components and subsystems are correct.  Individual
tests should build and run on the order of minutes.

These tests should be runnable on **both**:

*   locally on a developer workstation or with qemu instance
*   actual server hardware

#### System Level Testing

Tests the BMC as a black box via external interfaces.  They verify the
correctness of the BMC's response to external interfaces.  This level of testing
is used by the test dev team as part of overall server testing and regression
testing.  It may also be used by release engineering as part of verifying the
correctness of a OpenBMC image.

These tests should be run using real server with BMC via host-BMC link and any
other external interfaces.

#### Performance Testing

Tests the performance of OpenBMC components. These may be unit, end-to-end, or
system level tests.

#### Other Testing

Other types of testing, needs to be flushed out:

*   Security
*   Stress

### When Tests Are Invoked

#### Release Image Build

*   Jenkins build images for supported platforms
*   Release branching from last green cls

#### Release Image Testing

*   Some subset of end-to-end tests can be run on qemuarm or on real server
*   Some subset of system level tests can be run on real server
*   Trigger tests automatically from Jenkins/release process
*   Results posted publicly somewhere

#### Component Build

*   Have maintainers responsible for releasing various components
*   Release a version of the component on some cadence
*   Merge component with OpenBMC with each new OpenBMC tag
*   Cadence depends on how much traffic going on internally and externally
*   Always produce a release version after a merge
*   May need releases more frequently than merges are happening

#### Presubmit/CI Testing

*   Integration with Gerrit
*   Results posted to Gerrit and possibly to some other public location
*   Group unit tests and end-to-end tests into logical groups.
*   Automatically run related groups of tests for each patchset uploaded and on
    submit
*   When a patchset is uploaded:
    *   Unit tests are automatically run, possibly with code coverage
    *   Static analysis is performed
    *   Linter is run
*   When a patchset is submitted:
    *   All of the above
    *   Integration tests are automatically run

#### Developer Testing

*   Run and develop unit tests from workstation when working on a component

### Reference device

*   Reference BMC board that can be incorporated into server designs
*   Ideally, passing tests against reference device demonstrates that changes
    work on all servers
*   Can use for part validation

### Unit Testing

OpenBMC is more than a Linux distribution. It is a full software and firmware
stack for a BMC. It encompases all of the code that is run on a BMC. For the
purposes of testing, OpenBMC is divided into three distinct elements: U-Boot,
Linux, and the OpenBMC userspace. Ideally, every element of the OpenBMC stack
should be unit testable. However each layer is unique and will require a
different approach for testing.

#### U-Boot and Linux Unit Testing

Currently there is no available method for unit testing the kernel. In the
meantime we have autotest and QEMU.

#### OpenBMC Userspace

Fortunately, most of OpenBMC resides in the userspace for which there are
already ways to unit test. OpenBMC has already accepted gtest as their standard
unit testing framework; however, the actual testability of OpenBMC's daemons is
pretty poor.

The biggest problem is that all daemons depend on DBus and a handful of
important daemons; the code as it is written is also not really intended for
testing. Nevertheless, these are really the same problem; the code is not really
written to be tested; it is lacking in abstractions, etc. The solution for this
is simply to create an abstraction for DBus that can be mocked out; all daemons
interface each other through DBus (possibly on sdbusplus on top of DBus), so
getting everything on sdbusplus and providing mocking for sdbusplus will get us
the ability to test most daemons individually.

Most daemons also interact with the OS using the file system; although many of
these have higher level semantics built on top of the file system, it would,
nevertheless, be a huge benefit to provide a mockable file system abstraction.

Once we get the ability to test daemons independently through mocking away
external dependencies, we still need a way to provide a build environment for
building and running the userland daemons and libraries. Fortunately, we already
have this through OpenBMC's Yocto SDK; it is not super well maintained, but does
periodically work. This is something we should maintain so that it could be used
for this purpose.

Once we have all of the above, we will be able to start testing daemons and
libraries independently from each other, locally, quickly, and without a VM. We
will still need to go through and actually write tests for all of the daemons,
which will take some time. Nevertheless, a good way to approach this is to
require that tests are added for any change above some threshold delta. In this
way, new daemons, and daemons with high traffic will get testing first; whereas,
daemons that we don't care about as much, that are not being changed as much, or
are fairly stable will be lower priority.

The last major piece of work for unit testing will be to get unit test running
on submission to Gerrit and as part of the release process. The former should be
pretty straightforward using a Gerrit hook to kick off a build on Jenkins.

### System Level Testing

System level tests treat the BMC as a black box and test via accessible
interfaces.  They test the BMC through the external BMC interfaces like IPMI.
These tests can be run on demand or triggered by CI event or new release
version.

### Reference Device Specification

It might be useful to have a reference device for developing OpenBMC. I have
seen a couple people on the IRC or mailing list (I forget where specifically)
looking for a way to play with OpenBMC. It might be nice to be able to point
them at a reference device that shows off what OpenBMC is supposed to do.

## Planned tasks

### Unit Testing Tasks

*   Make DBus/sdbusplus mockable
    *   Started
*   Provide a mockable file system interface
    *   Not started
*   Ensure OpenBMC SDK is stable and includes mocks for DBus/sdbusplus and the
    file system interface
    *   Started
*   Start incorporating mocks and tests into daemons as changes are made
    *   Not Started
*   Provide ability to build and run tests in Jenkins
    *   Not Started

### System Level Testing Tasks

*   IPMI BMC Uptime
*   Reboot stress test
*   Flash upgrade downgrade
*   NCSI tests
