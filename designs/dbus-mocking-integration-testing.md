
# D-Bus Mocking for Integration Testing

Author:
  Ramin Izadpanah (iramin @google)

Primary assignee:

Other contributors:

Created:
 October 13, 2020

## Problem Description
Currently, in the OpenBMC project, there is no framework for mocking interfaces implemented by services on D-Bus. Mock services are needed to facilitate testing OpenBMC daemons, especially in integration tests.

This is a proposal to provide the capabilities for placing mock objects with specific expectations on D-Bus and making calls to a daemon under test.

## Background and References
One of the challenges in [integration tests](https://martinfowler.com/articles/practical-test-pyramid.html#IntegrationTests) for OpenBMC is the dependency of daemons to other services that communicate over [D-Bus](https://www.freedesktop.org/wiki/Software/dbus/). It is not always feasible to have all of the required services available during the testing of a single OpenBMC daemon or make them behave in a certain way. Examples include services on devices and various types of sensors that are used by daemons such as [swampd](https://github.com/openbmc/phosphor-pid-control) and [ipmid](https://github.com/openbmc/phosphor-host-ipmid). 

A framework that provides the capabilities for mocking the OpenBMC services over D-Bus, enables OpenBMC daemon developers to write tests for their daemons that interact with other services using D-Bus.  
Instead of real services, mock services pre-programmed with expectations will be run that allow for performing integration testing of daemons that communicate with external services over D-Bus. This decouples the testing process of a daemon from external services and devices.

## Requirements
Users of this solution are the developers who run integration tests for a single OpenBMC daemon.

The solution should facilitate the configuration, building, and running of mock services over D-Bus.

Users should be able to run mock services on D-Bus without the need for root privileges or possible interference with running services on a system.

Users should have the ability to communicate with mock services over D-Bus in the test programs written in Python, C++, or bash scripts.

Users should be able to customize the expected behavior of the methods or values of the properties exposed by selected interfaces of the mock services.

Users should be able to test an OpenBMC daemon by specifying the expected behaviors/values for only a selection of OpenBMC D-Bus interfaces and services that they need.

Users should be provided with examples of running integration tests of OpenBMC daemons using mock D-Bus services so they can follow the examples and/or customize them for their purposes.

## Proposed Design
The proposed solution will enhance [openbmc-build-scripts](https://github.com/openbmc/openbmc-build-scripts) by adding capabilities to configure, build, and run integration tests, similar to the way that unit tests are currently run. The required environment to run the integration tests will be provided in this repository.  
Examples of integration tests using the proposed solution will be added to repositories hosting OpenBMC daemons source code. In-repo integration tests, if enabled, will be run after the unit tests of the repository.  
A new repository will be created to provide common convenience functions for managing integration tests, and mocking services on D-Bus.

Given the requirements, the [python-dbusmock](https://github.com/martinpitt/python-dbusmock) library is selected for implementing mock D-Bbus services in OpenBMC. This library is recommended by the D-Bus community, [here](https://lists.freedesktop.org/archives/dbus/2018-February/017412.html) and [here](https://lists.freedesktop.org/archives/dbus/2018-February/017413.html). Since the mock services can be run as a normal program in a separate process, users can communicate with them over D-Bus from any programming language. This library uses LGPL-3.0 License for copyright that makes it suitable for use in the OpenBMC development. It is well supported by the community and has 38 stars on github and has received support from 23 contributors. Finally, it is an active repository with the last activity of July 2020, at the time of writing.

Users can define arbitrary behavior for the methods and expected values for properties in the mock interfaces. The arbitrary behavior can be a simple return value or a complex python method that works based on the input arguments and the service state.



## Alternatives Considered
In addition to python-dbusmock, some other libraries and modules were also considered.

[A perl module](https://gitlab.com/berrange/perl-net-dbus/-/blob/master/lib/Net/DBus/Test/MockObject.pm) exists in the perl-net-dbus repository that is challenging to be used for running tests with C++ or Python and there is no perl module maintenance in the OpenBMC repository.

[Bendy bus](https://github.com/pwithnall/bendy-bus) is mostly suitable for unit testing, but it is not convenient for integration testing that requires interaction with other services to observe an expected behavior.

[GTestDBus](https://developer.gnome.org/gio/stable/GTestDBus.html) is a part of the GDBus library and [it is known](https://lists.freedesktop.org/archives/dbus/2018-February/017413.html) to have some design issues.

## Impacts

This solution impacts the testing system by adding new required dependencies and increasing the amount of time required to run tests. To mitigate these impacts, running integration tests will become optional, and the required environment will be provided when users want to execute integration tests using the docker scripts in [openbmc-build-scripts](https://github.com/openbmc/openbmc-build-scripts).

Examples of integration tests using this solution will be provided for developers.

## Testing

While this solution is used in the testing system, appropriate unit tests will be developed to test its functionalities. However, these unit tests will not be a part of CI testing.