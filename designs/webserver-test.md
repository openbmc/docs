# webserver test

Author:
   Ed Tanous <edtanous>

Created:
   September 29, 2020

## Problem Description
OpenBMC has a very bad history of breaking specified external web APIs, or
breaking clients that rely on certain behavior.  One of the places where we
break behavior a lot is in Redfish.  This design aims to allow code submitters
and CI to have a way to gain confidence in their changes, as well as for
reviewers and consumers have an easy to run and fast tool for finding
regressions and schema changes.

## Background and References
A number of tools to test Redfish exist:
https://github.com/DMTF/Redfish-Service-Validator
https://github.com/DMTF/Redfish-Protocol-Validator

In addition to the official DMTF tools, openbmc has a test automation framework.
https://github.com/openbmc/openbmc-test-automation

Today, these tools have critical flaws that prevent them from meeting all the
requirements below.

## Requirements
1. Interface test should have a mechanism for users to easily conduct a Redfish
   interface test on a running system, with a less than 5 minute setup time.
2. Interface test should be capable of covering _all_ webserver code, including
   degenerate conditions, load testing, and invalid protocol errors.
3. Interface test defaults should match the Openbmc image features and
   configure option set.
4. Interface test should be able to be run directly from the BMC, for ease of
   some development environments, including qemu.
5. Interface test results should be able to run/wrap the existing generic
   compliance tools to verify they are passing, and reduce scope.  Redfish
   service validator is specifically required.
6. Interface test should be capable of measuring performance of a given
   implementation down to a single number.  Having a single "performance"
   number should be reliable enough to run in a CI infrastructure to gate
   builds with performance problems.
7. Interface test should allow for verification of specific system parameters
   taken from the user, for example, Chassis SerialNumber and UUID parameters
   should be able to be verified to be an exact string match to the expected
   parameters for the system under test.  Warnings should be logged in the
   cases where the test does not have enough information to assert the
   correctness of the parameter for a specific system.
8. Interface test should have a mechanism for exporting the received API
   results such that a reasonable representation of the Redfish tree can be
   built.  Optional: check in the tree for the various CI systems such that we
   can track changes to the redfish tree over time, and developers can answer
   the question: "Does this work on X platform"
9. Interface test should be aware of the OpenBMC image and configure features,
   and provide equivalent options to the user executing the test.
10. Interface test should verify protocol layer (SSL rejection, error
    conditions, socket disconnects) in a way that is as repeatable as is
    possible.
11. Interface test shall test of a typical Redfish system shall take less than
    10 minutes.
12. Interface test shall endeavour to run as many tests in parallel as is
    possible to increase the change of finding race conditions.
13. While non-randomized tests are preferred, test cases that rely on random
    information shall be based on a configurable global seed, to ensure
    repeatability.
14. Interface test shall provide human readable logs that log only warnings
    and failures by default.  For example "The SerialNumber parameter on the
    /redfish/v1/system interface was 'foo' when the test expected it to be
    '12345678'.  Please verify."  This is very different than "FAIL: Redfish
    test.  Arguments:target=System, param=SerialNumber traceback ..."
15. Interface test shall provide for warnings where the test configuration
    did not contain enough information to properly verify all functionality of
    the web interface.  This is to allow gating gerrit checkins in the future
    for new/valid systems.
16. Interface test shall include tests for OpenBMC-specific behavior that might
    be considered optional in Redfish
17. Interface test shall make every attempt to restore the system to its
    previous state at the conclusion of the tests.  In the case of broken code,
    this might not always be possible, but is required to be reliable for
    builds that pass the tests.  This is to prevent "bricking" systems because
    of a test failure.
18. Any tests that rely on specific network topology (ie the BMC connected
    directly to the test host) will be isolated behind a specific, non-default
    config option.
19. Test harness should run in less than 10 minutes for a example dual socket
    OpenBMC system.

## Proposed Design
A compiled test tool built on boost::beast.  Boost beast is a good choice,
because it is crazy fast, and allows fine grained control of things like
intentionally breaking the protocol or modifying SSL parameters without having
to worry about the library getting in your way.  It is also the same framework
bmcweb is based on, so as new features are added to bmcweb, the equivalent
client-side changes can be made to the tool easily.  Because Boost Beast is
c++, it can be compiled to be small, and run on an OpenBMC system itself with
an equivalent yocto recipe.  As an explicit negative, we won't be testing
against multiple implementations, but this could be added with a dependency on
other frameworks, and is also somewhat covered by Redfish-Service-Validator.

The tool will have a folder structure like:

openbmc_test  // the tool itself
test_logs/<machinename>  // the logs specific to a machine from the last run
test_logs/<machinename>/redfish  // folder with the resulting OpenBmc http tree
config/<machinename>/config  // a test file containing the configuration
src/ // the source code for the tool.
docs/

The tool will be invoked with a command like:
openbmc_test <machinename>

// Note: folders should be structured such that they do not conflict with one
another.  They should be named consistently such that the resulting configs and
logs can be checked into git.  In this way, specific machines results can be
compared against one another, as well as being compared against themselves.
This allows comparisons to the external interface to be tracked over time, and
flagged in a simple way when an unexpected change happens.

Redfish-Service-validator will be subtreed into this tools build tree, and
called as part of running this tool.  The --cache option will be used to allow
it to run on the cached tree that this tool will generate in the /redfish
folder, thus mitigating most of its performance constranits, and allowing it to
do its analysis in parallel with other tasks

An example config file might look like
```
Expected Schemas:
AccountService
SessionService
Chassis

Allowed power states:
PowerCycle
On
ForceOff


Expected Chassis names:
baseboard
	SerialNumber: 12345667789
rXX_chassis
	Model: foo
```

And would contain all configuration items needed to verify a full system.  One
key is that it will not rely on the system itself to verify what devices,
resources, or properties exist, so that if properties or resources "disappear"
or don't match what you'd expect, the test tool can flag an error.

## Alternatives Considered
1. Modifying Redfish-Service-Validator to add these specific requirements.
   This option was specifically discounted because the current tool is designed
   to be a generic, all-system redfish validator, and likely would not accept
   changes that verify OpenBMC specific behavior.  Redfish-Service-Validator
   would also be incapable of being extended to test similar features in the
   future.  Redfish-Service-Validator also explicitly does not test any
   actions, or changes.  The Validator also is written in python, which would
   not fit on a BMC
2. Using the existing robot framework test automation as a base.  The existing
   robot framework is based on writing config files, and specific test cases.
   It does not have user-friendly logging, nor does it have the ability to test
   negative conditions at the socket level The logs and schemas are not
   comparable between runs, which is a primary requirement for this tool.  It's
   also unlikely we'd be able to get robot framework to fit on the bmc.

## Impacts
Considering this is a tool that primarily run outside of OpenBMC the impact to
the user should be negligeble unless the test app is compiled in.

## Testing
This is itself a test to be run.  In the future, there are probably
opportunities for this to be integrated with CI, but that will take time.

This tool will slowly evolve.  It should be initially functional against 2
different OpenBMC installations.
