# Redfish self test

Author:
  edtanous <ed@tanous.net>

Created:
10/1/2021

## Problem Description
The Redfish subsystem within OpenBMC has a number of sharp edges[1] that tend to
trip up developers patchsets.  A number of these issues can be relatively easily
tested within the data structures within bmcweb, but can't be tested on every
startup because of startup CPU costs, and because the tests might be invasive,
or force crashes on previously good code, therefore, it's desirable to be able
to selectively enable or disable these tests.  While this idea could be
generalized to all daemons, this design doc intentionally limits itself to a set
of Redfish tests.

## Background and References
Redfish the specification [2] defines behavior for a restful CRUD API.
As an example of a "self test" that's easily run from a bmc, consider this

Common error in 404 handling. [1]  Because it relies on the behavior of other
system resources (ie the mapper, and other dbus daemons) to produce the correct
result, there is no way to verify statically or in a unit test whether or not
the code is correct without a full system running.

Previous design doc with similar ideas [3], but attempting to run them out of
band off-bmc.


## Requirements
BMC should be capable of executing code on the BMC from within the webserver
application.
The self test application should be capable of running coverage directed fuzzing
(libfuzzer, AFLfuzzer, ect) at greater than line rate.
The self tested application should have access to internal webserver data
structures (ie route lists, ect) to allow it to develop testing plans.
The self tested application shall have a mechanism to trigger and run the self
test from within the BMC.
The self test code shall be capable of being disabled at compile time to avoid
binary size impact should the tests or dependencies get large.
While not always possible, the self test should attempt to do its best to reset
all state back to previous values when the test is completed.


## Proposed Design
This doc proposes the addition of a new dbus interface to bmcweb,
xyz.openbmc_project.SelfTest.Run().  This interface and method would trigger the
running of all self tests.  This interface is intentionally not scoped to the
webserver such that it could be extended to other applications in the future,
and possibly be the basis for populating the IPMI/Redfish self test results,
although this is outside the scope of this document.
bmcweb would add a compile option called to compile in this self test.  This
test would link against libfuzzer, and implement fuzzing tests using boosts test
socket implementation [5] to simulate a connected client, and ensure that we can
run socket tests at full fuzzing rate.
bmcweb will also implement a "404" checker, that dumps the available roots, and
tries injecting known incorrect IDs into the URI, and ensure that 404 is
returned properly.
Once there is an infrastructure for testing, more tests can be added at will as
common errors arise.

## Alternatives Considered
Running tests from off-system.  While we already do this via
Redfish-service-validator[4], this tends to have limitations in the amount of
things that we can test, given that it doesn't have access to internal data
structures (namely the route map), and can't run greater than line rate tests.
The difficulty in setting up the test also seems to have impacts on how many
developers actually run them, and have been a reason for people to not run them
in the past.

Status quo/null hypothesis, continue without this form of testing.  Given that
security matters, and the number of incorrect patchsets to Redfish keeps
increasing, this doesn't seem like a sustainable option, for either the codebase
or for the maintainers therein.

Running tests in a separate application that could be sideloaded.  This is a
viable option, and will need to be constantly evaluated as the feature is built,
but considering the large link surface of bmcweb, and the fact that this would
require a developer to take more steps to actually run the test, this is
explicitly discounted.  Developers already know how to build debug/logging
binaries, which is largely the same operation as adding a self-test compile
flag.

## Impacts
Developer impact would likely be that now we would require that developers run
this method on their system before checking in their code.  This would be
similar to the service validator.  Ideally we could run a service-validator
level test in the future, and combine all our "on system testing" in to one
package.
While there will be a binary size impact when this feature is enabled, that
should only effect developer builds hoping to use this feature.

## Testing
This is itself a test, so testing will be done by running the test and observing
coverage statistics, as well as measuring how quickly we can identify incorrect
pathsets.

[1] https://github.com/openbmc/bmcweb/blob/master/COMMON_ERRORS.md#11-not-responding-to-404
[2] https://www.dmtf.org/sites/default/files/standards/documents/DSP0266_1.14.0.pdf
[3] https://gerrit.openbmc-project.xyz/c/openbmc/docs/+/36984
[4] https://github.com/DMTF/Redfish-Service-Validator
[5] https://www.boost.org/doc/libs/develop/libs/beast/doc/html/beast/ref/boost__beast__test__stream.html
