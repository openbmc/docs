
____

# Power Supply Control Application

Author:
  Brandon Wyman !bjwyman

Primary assignee:
  Brandon Wyman

Other contributors:
  Derek Howard

Created:
  2019-06-17

## Problem Description
This is a proposal to provide a set of a enhancements to the current OpenBMC
power supply application for enterprise class systems. Some enterprise class 
systems may consist of a number of configuration variations including different 
power supply types and numbers. An application capable of communicating with the 
different power supplies is needed in order to validate configurations, prevent 
power on of invalid configurations, detect and report various faults, and create 
and report telemetry data.

## Background and References
The OpenBMC project currently has a [witherspoon-pfault-analysis][1] repository 
that contains a power supply monitor application and a power sequencer monitor
application. The current power supply application is lacking things desired for
an enterprise class server.

The intent of this new application design is to enhance the OpenBMC project 
with a single power supply application that can communicate with one or more 
[PMBus][2] power supplies and provide the enterprise features currently lacking 
in the existing application that has multiple instances talking to a single 
power supply.

## Requirements
1. The power supply application must detect, isolate, and report individual 
input power and power FRU faults, during IPL and at runtime only.  
2. The power supply application must determine power supply presence, 
configuration, and status, and report via external interfaces.  
3. The power supply application must report power supply failures to IPMI and 
Redfish requests (during IPL and at runtime only).  
4. The power supply application must report power supply present/missing changes 
and status to IPMI and Redfish requests, and to the hypervisor.  
5. The power supply application must ensure proper power supply configuration 
and report improper configurations (during IPL and at runtime only).  
6. The power supply application must collect and report power supply Vital 
Product Data (VPD) along with system VPD.  
7. The power supply application must allow power supply hot-plug and concurrent 
maintenance (CM).

The power supply application must be able to detect how many power supplies are 
present in the system, and what type of power supply is present (110 volts, 
220 volts, AC input, DC input, maximum output power, etc.).

The application must be able to recognize if the power supplies present 
consist of a valid configuration. Certain invalid combinations should result in
the application preventing the chassis from being powered on, similar to the 
Minimum Ship Level ([MSL][3]) check.

The application must create error logs for invalid configurations, or for power
supplies experiencing some other faulted condition (no input power, output over
voltage, output over current, etc.).

The application would periodically communicate with the power supplies via the
sysfs file system files currently created and updated by the [ibm-cffps][4] 
device driver that supports the IBM common form factor power supplies that 
communicate using the PMBus specification. Enhancements to that device driver 
are likely needed in order to corrrectly support reading the firmware version 
(fw_version) and multi-page support.

## Proposed Design
The proposal is to create a single new power supply application in some new 
OpenBMC repository, such as `phosphor-psu-monitor`. The application would be 
written in C++17.

Upon startup, the power supply application would be passed a parameter 
consisting of the location of some kind of configuration file, most probably 
some JSON format file. This file would contain information such as the D-Bus 
object name(s), possible power supply types, possible system types that the 
various power supplies are valid to be used in, I2C/PMBus file location data, 
etc. The usage of some kind of JSON parsing library would be needed.

The power supply application would then detect which system type it is running 
on, which supplies are present, what type each supply is, etc. The application
would then try to find a matching valid configuration. If no match is found, 
that configuration would be considered invalid. If the configuration is valid, 
the application should continue to check what if any faults are occurring, and 
if the chassis should be allowed to power on.

If the configuration is valid, the application should allow the chassis to be
powered on, enabling the power supplies to output power to the system. From 
that point on it will continue to monitor the supplies and communicate any 
changes such as removal of input voltage, removal of a power supply, insertion
of a power supply, and any necessary actions to take upon detection of fault 
conditions.

## Alternatives Considered
The current implementation of multiple instances of a power supply monitor was 
considered, essentially similar to the [psu-monitor][5] from the 
[witherspoon-pfault-analysis][1] repository. This design was avoided due to:
 - Length of application parameters to be passed in.
 - Complexity of the various valid and invalid configuration combinations.
 - Power line disturbance communication.
 - Timing/serialization concerns with power supply communication.

## Impacts
API impact? Security impact? Documentation impact? Performance impact?
Developer impact? Upgradability impact?

None?

PLDM?

Similar or lesser performance impact than one application per power supply?

## Testing
How will this be tested? How will this feature impact CI testing?

System configurations needed for CI testing?

[1]: https://github.com/openbmc/witherspoon-pfault-analysis
[2]: https://en.wikipedia.org/wiki/Power_Management_Bus
[3]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Control/README.msl.md
[4]: https://github.com/openbmc/linux/blob/dev-5.1/drivers/hwmon/pmbus/ibm-cffps.c
[5]: https://github.com/openbmc/witherspoon-pfault-analysis/tree/master/power-supply
