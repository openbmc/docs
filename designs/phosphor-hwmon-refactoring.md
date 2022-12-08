# Phosphor-hwmon refactoring

Author: Kun Yi <kunyi!>

Created: 2019/09/05

## Problem Description

Convoluted code in phosphor-hwmon is imparing our ability to add features and
identify/fix bugs.

## Background and References

A few cases of smelly code or bad design decisions throughout the code:

- Limited unit testing coverage. Currently the unit tests in the repo [1] mostly
  test that the sensor can be constructed correctly, but there is no testing of
  correct behavior on various sensor reading or failure conditions.
- Bloated mainloop. mainloop::read() sits at 146 lines [2] with complicated
  conditions, macros, and try/catch blocks.
- Lack of abstraction and dependency injection. This makes the code hard to test
  and hard to read. For example, the conditional at [3] can be replaced by
  implementing different sensor::readValue() method for different sensor types.

[1](https://github.com/openbmc/phosphor-hwmon/tree/master/test)
[2](https://github.com/openbmc/phosphor-hwmon/blob/94a04c4e9162800af7b2823cd52292e3aa189dc3/mainloop.cpp#L390)
[3](https://github.com/openbmc/phosphor-hwmon/blob/94a04c4e9162800af7b2823cd52292e3aa189dc3/mainloop.cpp#L408)

## Goals

1. Improve unit test coverage
2. Improve overall design using OOD principles
3. Improve error handling and resiliency
4. Improve configurability
   - By providing a common configuration class, it will be feasible to add
     alternative configuration methods, e.g. JSON, while maintaining backward
     compatibility with existing env files approach.

## Proposed Design

Rough breakdown of refactoring steps:

1. Sensor configuration
   - Add a SensorConfig struct that does all std::getenv() calls in one place
   - DI: make the sensor struct take SensorConfig as dependency
   - Unit tests for SensorConfig
   - Add unit tests for sensor creation with various SensorConfigs
2. Abstract sensors
   - Refine the sensor object interface and make it abstract
   - Define sensor types that inherit from the common interface
   - Add unit tests for sensor interface
   - DI: make the sensor map take sensor interface as dependency
   - Add a fake sensor that can exhibit controllable behavior
3. Refactor sensor management logic
   - Refactor sensor map to allow easier lookup/insertion
   - Add unit tests for sensor map
   - DI: make all other functions take sensor map as dependency
4. Abstract error handling
   - Add overridable error handler to sensor interface
   - Define different error handlers
   - Add unit tests for error handlers using the fake sensor

## Alternatives Considered

N/A.

## Impacts

The refactoring should have no external API impact. Performance impact should be
profiled.

## Testing

One of the goals is to have better unit test coverage. Overall functionality to
be tested through functional and regression tests.
