
# D-Bus Mocking for Integration Testing

Author:
  Ramin Izadpanah (iramin @google)

Primary assignee:

Other contributors:

Created:
 October 13, 2020

## Problem Description
Currently, in the OpenBMC project, integration tests performed
against a full daemon is limited.
Most of these daemons depend on services that expose their implemented
interfaces using D-Bus.
Mock services are needed to facilitate the testing of OpenBMC daemons,
especially in integration tests.

This is a proposal to facilitate integration testing by providing the
capabilities for placing mock objects with specific expectations on D-Bus and
making calls to a daemon under test.

## Background and References
One of the challenges in [integration tests](
https://martinfowler.com/articles/practical-test-pyramid.html#IntegrationTests)
for OpenBMC is the dependency of daemons to other services that communicate over
[D-Bus](https://www.freedesktop.org/wiki/Software/dbus/). It is not always
feasible to have all of the required services available during the testing of a
single OpenBMC daemon or make them behave in a certain way. Examples include
services on devices and various types of sensors that are used by daemons
such as [swampd](https://github.com/openbmc/phosphor-pid-control) and
[ipmid](https://github.com/openbmc/phosphor-host-ipmid).

A framework that provides the capabilities for mocking the OpenBMC services over
D-Bus, enables OpenBMC daemon developers to write tests for their daemons that
interact with other services using D-Bus.
Instead of real services, mock services pre-programmed with expectations will be
run that allow for performing integration testing of daemons that communicate
with external services over D-Bus. This decouples the testing process of a
daemon from external services and devices.


### Goals

The goal of an "integration test" is to test a full OpenBMC daemon with no
modifications to its source in an environment with real IPC on an actual
D-Bus instance.

To clarify the goals of this proposal, we include two examples with figures.
In each example, we have an OpenBMC daemon under test.
We mock services that the daemon under test needs to function properly.
The expected behavior of the daemon can be verified using the mock services.

In both figures, we have D-Bus in the middle and a (real) [mapperx](
https://github.com/openbmc/phosphor-objmgr) daemon is running
(seen connected to the D-Bus top). The daemon under test is connected to the
right side of D-Bus. The mock services that implement D-Bus interfaces are
connected to the left side of D-Bus.


#### Example 1

In this example, the daemon under test is swampd, and we evaluate its capability
in maintaining the temperature within a zone.
swampd needs to interact with temperature and fan sensors using D-Bus to
function properly.
In the test program, two mock services will be run:
1) MockTemperatureSensor implements the [Sensor.Value](
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Sensor/Value.interface.yaml) interface.

2) MockFanSensor implements [Sensor.Value](
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Sensor/Value.interface.yaml)
 and [Control.FanPwm](
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Control/FanPwm.interface.yaml) interfaces.


                                              +---------+
                                              | mapperx |
                                              +----+----+
                                                   |
                                                   |
                                               +---+---+
          +-------------------------+          |       |
          |  MockTemperatureSensor  +----------+       |
          +-------------------------+          |       |     +--------+
                                               | D-Bus +-----+ swampd |
               +---------------+               |       |     +--------+
               | MockFanSensor +---------------+       |
               +---------------+               |       |
                                               +-------+

The behavior of mock sensors can be customized for the specific test and
the swampd response will be evaluated.


#### Example 2

In this example, the daemon under test is ipmid, and we evaluate its capability
in network configuration.
ipmid needs to interact with networkd using D-Bus to function properly.

                         +---------+
                         | mapperx |
                         +----+----+
                              |
                              |
                          +---+---+
     +--------------+     |       |     +-------+
     | MockNetworkd +-----+ D-Bus +-----+ ipmid |
     +--------------+     |       |     +-------+
                          +-------+




### Non-goals

In this proposal, our goal is not to mock the D-Bus for unit tests that need to
bypass D-Bus for testing a single unit/function (not a whole daemon).



## Requirements
Users of this solution are the developers who run integration tests for a
single OpenBMC daemon.

* Users should have the options to enable/disable integration tests.

* The solution should be run in an environment where a dbus-broker is available.

* Users should be able to run parallel tests without interference with
each other.

* Users should be able to run mock services on D-Bus without the need for root
privileges or possible interference with running services on a system.

* Users should be able to customize the expected behavior of the methods or
values of the properties exposed by selected interfaces of the mock services.

* Users should be able to test an OpenBMC daemon by specifying the expected
behaviors/values for only a selection of OpenBMC D-Bus interfaces and services
that they need.

* Users should be provided with examples of running integration tests of OpenBMC
daemons using mock D-Bus services so they can follow the examples and/or
customize them for their purposes.

## Proposed Design

Given the requirements,
the [sdbusplus](https://github.com/openbmc/sdbusplus) library is
selected for implementing mock D-Bus services in OpenBMC.
Common convenience functions for mocking D-Bus interfaces will be added to
[sdbusplus](https://github.com/openbmc/sdbusplus).
Parts of these functions include running and management of a private bus
per test that allows for running parallel tests and avoiding interfering with
system services.
[sdbus++](https://github.com/openbmc/sdbusplus#how-to-use-toolssdbus) will be
extended to generate default [gMock](
https://github.com/google/googletest/tree/master/googlemock)
headers for interfaces.
Common functionalities for mocking OpenBMC D-Bus services and running
integration tests will be added to
[dbus-interfaces](https://github.com/openbmc/phosphor-dbus-interfaces/).
The required configuration will be added to
[openbmc-build-scripts](https://github.com/openbmc/openbmc-build-scripts)
to run the integration tests.



Examples of integration tests using the proposed solution will be added to
repositories hosting OpenBMC daemons source code. In-repo integration tests,
if enabled, will be run after the unit tests of the repository.

Users can use gMock to define arbitrary behavior for the methods and expected
values for properties in the mock interfaces. The arbitrary behavior can be a
simple return value or a complex C++ method that works based on the input
arguments and the service state.

## Alternatives Considered
In addition to sdbusplus, some other libraries and modules were also
considered:

* [python-dbusmock](https://github.com/martinpitt/python-dbusmock) is a library
for mocking dbus services that is implemented in python.
This library was not selected to have the ability to run tests on systems with
no python support.

* A perl
[module](
https://gitlab.com/berrange/perl-net-dbus/-/blob/master/lib/Net/DBus/Test/MockObject.pm)
exists in the perl-net-dbus repository that is challenging to be used for
running tests with C++ or Python and there is no
perl module maintenance in the OpenBMC repository.

* [Bendy bus](https://github.com/pwithnall/bendy-bus)
is mostly suitable for unit testing, but it is not convenient for integration
testing that requires interaction with other services to observe an expected
behavior.

* [GTestDBus](https://developer.gnome.org/gio/stable/GTestDBus.html)
is a part of the GDBus library and it is
[known](https://lists.freedesktop.org/archives/dbus/2018-February/017413.html)
to have some design issues.

## Impacts

This solution impacts the testing system by adding new required dependencies and
increasing the amount of time required to run tests. To mitigate these impacts,
running integration tests will become optional, and the required environment
will be provided when users want to execute integration tests using the docker
scripts in
[openbmc-build-scripts](https://github.com/openbmc/openbmc-build-scripts).

Examples of integration tests using this solution will be provided for
developers.
All examples will be based on existing OpenBMC daemons and D-Bus interfaces.

## Testing

While this solution is used in the testing system, appropriate unit tests will
be developed to test its functionalities.
However, these unit tests will not be a part of CI testing.
