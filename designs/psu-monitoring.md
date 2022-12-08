# Power Supply Monitoring Application

Author: Brandon Wyman !bjwyman

Other contributors: Derek Howard

Created: 2019-06-17

## Problem Description

This is a proposal to provide a set of enhancements to the current OpenBMC power
supply application for enterprise class systems. Some enterprise class systems
may consist of a number of configuration variations including different power
supply types and numbers. An application capable of communicating with the
different power supplies is needed in order to initialize the power supplies,
validate configurations, report invalid configurations, detect and report
various faults, and report vital product data (VPD). Some of the function will
be configurable to be included or excluded for use on different platforms.

## Background and References

The OpenBMC project currently has a [witherspoon-pfault-analysis][1] repository
that contains a power supply monitor application and a power sequencer monitor
application. The current power supply application is lacking things desired for
an enterprise class server.

The intent of this new application design is to enhance the OpenBMC project with
a single power supply application that can communicate with one or more
[PMBus][2] power supplies and provide the enterprise features currently lacking
in the existing application that has multiple instances talking to a single
power supply.

## Requirements

Some of these requirements may be deemed as business specific logic, and thus
could be configurable options as appropriate.

1. The power supply application must detect, isolate, and report individual
   input power and power FRU faults, during boot and at runtime only.
2. The power supply application must determine power supply presence,
   configuration, and status, and report via external interfaces.
3. The power supply application must report power supply failures to IPMI and
   Redfish requests (during boot and at runtime only).
4. The power supply application must report power supply present/missing changes
   and status to IPMI and Redfish requests, and to the hypervisor. Recipes and
   code for presence state monitoring and event log creation may need to be
   moved from the `phosphor-dbus-monitor` to this application, depending on if
   such function was already written or ported forward from a previously similar
   system.
5. The power supply application must ensure proper power supply configuration
   and report improper configurations (during boot and at runtime only).
6. The power supply application must collect and report power supply VPD (unless
   that VPD is collected and reported via another application reading an EEPROM
   device).
7. The power supply application must allow power supply hot-plug and concurrent
   maintenance (CM).
8. The power supply application should create and update average and maximum
   power consumption metric interfaces for telemetry data.
9. The power supply application must be able to detect how many power supplies
   are present in the system, what type of power supply is present (maximum
   output power such as 900W, 1400W, 2200W, etc.), and what type of input power
   is being supplied (AC input, DC input, input voltage, etc.).
10. The application must be able to recognize if the power supplies present
    consist of a valid configuration. Certain invalid combinations may result in
    the application updating properties for a Minimum Ship Level ([MSL][3])
    check.
11. The application must create error logs for invalid configurations, or for
    power supplies experiencing some other faulted condition (no input power,
    output over voltage, output over current, etc.).
12. The application would periodically communicate with the power supplies via
    the sysfs file system files updated via a PMBus device driver (currently
    only known to be created and updated by the [ibm-cffps][4] device driver).
    Certain device driver updates may be necessary to support some power
    supplies or power supply features. Any power supply that communicates using
    the PMBus specification should be able to be supported, some manufacturing
    specific code paths may be required for commands in the "User Data and
    Configuration" (USER_DATA_00 through USER_DATA_15) and the "Manufacturer
    Specific Commands" (MFR_SPECIFIC_00 through MFR_SPECIFIC_45), as well as bit
    definitions for STATUS_MFR_SPECIFIC and any other "MFR" command.

## Proposed Design

The proposal is to create a single new power supply application in the OpenBMC
[phosphor-power][6] repository. The application would be written in C++17.

Upon startup, the power supply application would be passed a parameter
consisting of the location of some kind of configuration file, some JSON format
file. This file would contain information such as the D-Bus object name(s),
possible power supply types, possible system types that the various power
supplies are valid to be used in, I2C/PMBus file location data, read retries,
deglitch counts, etc.

The power supply application would then detect which system type it is running
on, which supplies are present, if the power supply is ready for reading VPD
information, what type each supply is, etc. The application would then try to
find a matching valid configuration. If no match is found, that configuration
would be considered invalid. The application should continue to check what if
any faults are occurring, logging errors as appropriate.

When the system is powered on, the power supplies should start outputting power
to the system. At that point the application will start to and continue to
monitor the supplies and communicate any changes such as removal of input
voltage, removal of a power supply, insertion of a power supply, and take any
necessary actions to take upon detection of fault conditions.

The proposed power supply application would not control any fans internal to the
power supply, that function would be left to other userspace application(s).

## Alternatives Considered

The current implementation of multiple instances of a power supply monitor was
considered, essentially similar to the [psu-monitor][5] from the
[witherspoon-pfault-analysis][1] repository. This design was avoided due to:

- Complexity of the various valid and invalid configuration combinations.
- Power line disturbance communication.
- Timing/serialization concerns with power supply communication.

## Impacts

The application is expected to have some impact on the PLDM API, due to the
various DBus properties it may be updating.

No security impacts are anticipated.

The main documentation impact should be this design document. Future
enhancements or clarifications may be required for this document.

The application is expected to have a similar or lesser performance impact than
the one application per power supply.

## Testing

Testing can be accomplished via automated or manual testing to verify that:

- Configuration not listed as valid results in appropriate behavior.
- Application detects and logs faults for power supply faults including input
  faults, output faults, shorts, current share faults, communication failures,
  etc.
- Power supply VPD data reported for present power supplies.
- Power supply removal and insertion, on a system supporting concurrent
  maintenance, does not result in power loss to powered on system.
- System operates through power supply faults and power line disturbances as
  appropriate.

CI testing could be impacted if a system being used for testing is in an
unsupported or faulted configuration.

[1]: https://github.com/openbmc/witherspoon-pfault-analysis
[2]: https://en.wikipedia.org/wiki/Power_Management_Bus
[3]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Control/README.msl.md
[4]:
  https://github.com/openbmc/linux/blob/dev-5.3/drivers/hwmon/pmbus/ibm-cffps.c
[5]:
  https://github.com/openbmc/witherspoon-pfault-analysis/tree/master/power-supply
[6]: https://github.com/openbmc/phosphor-power/
