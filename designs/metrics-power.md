# Metrics.Power.[Average|Maximum] dbus interfaces

Author:
  Matthew Barth !msbarth

Primary assignee:
  Matthew Barth !msbarth

Other contributors:
  None

Created:
  2019-07-12

## Problem Description
There is no known interfaces to enable storing power metrics for a machine
within OpenBMC. This design is to introduce the initial interfaces for a
machine's average power consumption and maximum consumed power over a duration
of time.

## Background and References
An initial IBM power metrics implementation was done for the witherspoon
machine. This was done early in OpenBMC development and therefore was tailored
to IBM and the witherspoon system to provide average and maximum power consumed
data. This was stored within openpower interfaces in a large data array
structure for each of these metrics.

OpenPower average|maximum sensor interfaces:
https://github.com/openbmc/openpower-dbus-interfaces/tree/master/org/open_power/Sensor/Aggregation/History

Power metrics calculated within the `witherspoon-pfault-analysis` application:
https://github.com/openbmc/witherspoon-pfault-analysis

There has also been discussion on the mailing list in regard to platform
telemetry and health monitoring, however there has been no specific designs
proposed upstream. The use of the Redfish metrics was noted in the latest
meeting from the time of writing this design, which is currently the strategic
direction.

OpenBMC platform telemetry and health monitoring workgroup wiki:
https://github.com/openbmc/openbmc/wiki/Platform-telemetry-and-health-monitoring-Work-Group

## Requirements
Produce power metrics available to a user over a given duration of time.
Initially, the necessary power metrics will include an average consumed power
interface and a maximum consumed power interface. The duration and polling
intervals of time in determining these values should be specific to each metric
in order to provide flexibility in calculating them over different machines and
power supplies.

## Proposed Design
Create a Metrics.Power.Average dbus interface containing a property to store
the average power consumed over a configured duration of time. At a given set
interval, the average power consumed would be read and calculated. Initially
the average power consumed would be 0 until after the configured duration of
time has passed. Once this occurs, implementation of the interface would update
the `Watts` property to the calculated average power consumed that was
determined at each interval.

Create a Metrics.Power.Maximum dbus interface containing a property to store
the maximum power consumed over a configured duration of time. At a given set
interval, the maximum power consumed would be determined and stored until the
set duration passes. Initially the maximum power consumed would be 0 until
readings are made over the interval configured and then updated after the set
duration period. Implementation of the interface would update the `Watts`
property to the determined maximum power consumed after each duration period.

As an initial implementation, witherspoon-pfault-analysis would be updated to
extend these dbus interfaces and set the duration and intervals of time to
values given within its configuration. Once the application is started, the
collection of power consumed already occurs, so additional timers will be
enabled to begin calculating the average and maximum power consumed values.
When the duration timer expires for each interface, its power consumed(`Watts`)
property will be updated to the determined value.

## Alternatives Considered
The use of `collectd` does not seem to be easily accessible using the Redfish
protocol. With the current strategic direction of using Redfish within OpenBMC,
this halted investigation of using it.

## Impacts
The application(s) used to host the object(s) containing these interfaces must
be configured with an interval and unit of time which would be used in
calculating the corresponding interfaces power metric value. They must also
enable updating the `Watts` property after a set duration with the
corresponding power metric value that was calculated.

## Testing
Initially within the witherspoon-pfault-analysis, calculate each power metric
on their configured intervals and update the `Watts` property with the
appropriate calculated power metric value every duration.
