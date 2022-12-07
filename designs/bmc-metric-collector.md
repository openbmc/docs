# phosphor - metric - collector

Author: Edward Lee <edwarddl@google.com>, Sui Chen <suichen@google.com>, Willy
Tu <wltu@google.com>

Created: Dec 7, 2022

## Problem Description

How do we collect BMC software-based health metrics in a method that is
lightweight, fast, and easily exportable at the large scale?

Software-based metrics would allow more information to monitor the overall
health of the BMC software and allow more actionable measures to be taken if a
BMC software is failing or regressing. More information would allow problems to
be pinpointed much faster.

As a user, I want to be able to obtain the metrics I need in order to monitor my
BMC without actually having to scrape my system for the metric itself. For
example, I dont want to manually go through the /proc filesystem to determine
the memory usage of a specific process. This example can extend to process file
descriptors, port bandwidth speeds, and other software-based metrics.

Specifically this proposes:

- A general format for BMC metrics
- How other components connect to the metrics data pipeline

The doc tries **not** to cover:

- How metrics are acted upon

  - The metrics collection framework tries to be agnostic to the meaning of the
    metrics data. Instead, the system 'owner' can decide to monitor and setup
    alerts that trigger certain actions based on thresholds.

- How metrics are implemented

  - The daemon does not cover implementation on metrics. It provides a general
    framework for exporting an implemented metric.

## Background and References

### Definition of Metric used in this Proposal

A metric in this document is defined as any piece of information that can be
used to monitor the health of the system. In this document, I will be implying
that these metrics are software-based.

### Referencing: [bmc-health-monitor.md](./bmc-health-monitor.md)

While the design doc linked above outlines the architecture for
phosphor-health-monitor, as the code-base has evolved there are conventions and
limitations that have been introduced into this daemon that have not been
explicitly stated in the design document.

Restrictions of phosphor-health-monitor:

1. Metrics must be primitive types. Objects such as strings cannot be used.

There is no way currently for a user to obtain the full commandline of a process
that has been executing.

2. Metrics were originally intended to be based off of sensor readings, which
   represent physical hardware measurements. Essentially not all metrics can be
   modeled by sensors.

As the first non-physical sensor, Sui has added utilization sensors and this has
come with some apprehension with the maintainers of phosphor-health-monitor. The
maintainers want to avoid the trap of where 'everything is a sensor', and this
means that any metrics reported in phosphor-health-monitor should ideally be a
physical measurement. Realistically, utilization does not belong in
phosphor-health-monitor and other metrics of similar category need a place to
reside.
[Source](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/51779)

### Referencing: [telemetry.md](./telemetry.md)

The telemetry architecture is somewhat similar to phosphor-health-monitor in the
fact that it is a daemon that scrapes data and utilizes the dbus as an
intermediary to transfer data from collector to user. The major difference is
that telemetry has a specific service inside Redfish, defined in the DMTF
schema, that it is implementing, while phosphor-health-monitor is a generic use
case with open-ended users, not intending to implement any specific service of
any application in particular.

When referencing the alternatives section on the telemetry design document,
telemetry proposes that one possible configuration due to telemetry's
flexibility and modularity is to use collectd in cooperation with telemetry.
Using this example, telemetry states that its flexibility and modularity
allows other daemons to be used in cooperation with it. This example is
essentially the design proposed by this document, with collectd replaced
with this proposed metric-collector daemon.

Their architecture shows collectd in conjunction with telemetry because
they source different types of metrics. Telemetry was created to be flexible
and modular and its intention was to expose telemetry data of the host through
DBus. It wouldnt make sense to clutter non-telemetry data inside their
architecture.

### Referencing: phosphor-debug-collector

Phosphor-debug-collector was a daemon intended to collect log files and
system parameters. This is another exmaple of a daemon that scrapes information
from the BMC system and exposes this information. Any more additions that are
debug logs would be overloading this daemon especially without a design for
a user of this daemon to reference.

## Requirements

To solve the problem described above, it would not make sense to export complex
software-based metrics via phosphor-health-monitor due to the restrictions
above. This new daemon proposed would have the following requirements:

1. The daemon will periodically scrape metrics from the system and make them
   accessible anytime.
2. The daemon will provide a DBus service and interface to read the any specific
   requested metric.
3. The DBus interface shall not limit types of metrics to be exported. (Such as
   how phosphor-health-monitor models the DBus interface after sensors, limiting
   their capability)
4. The daemon will provide the user with the requested metric on-demand via DBus
   calls. (Does not mean the metrics are collected on-demand)
5. All metrics that are exported must be defined in xyz.openbmc_project.Metrics.

This metric collection daemon's purpose is essentially a more lightweight
phosphor-health-monitor that is aimed towards exposing software-based metrics
via DBus.

## Proposed Design

### Architecture

The overall architecture and design flow is below.

```
+--------------+                     +----------------+
|system|       |                     |metric-collector|
+------/       |                     +----------------/
|              +<--metric scraping-->+                |
|              |                     |                |
+--------------+                     +--------^-------+
                                           |
                                           | xyz.openbmc_project.Metric
<------------------------------------------v-----^--DBus---------------------->
                                                 |
                                                 |
+-------------+----------------------------------v----------------------------+
|user program |                                                               |
+-------------/                                                               |
|                                                                             |
+-----------------------------------------------------------------------------+
```

There are two main parts of the daemon design: Metric Collection and Metric
Service/Interface

### Metric Collection

Periodically, this daemon will scrape metrics from the system itself. These
metrics will be stored in a data structure in memory for the daemon and ready to
be exported at any time. Any time the system scrapes metrics, the old metrics
shall be discarded and updated with the new ones. This stages the metrics for
exporting via dbus and allows them to always be available without blocking the
user making the dbus call.

### Metric Service/Interface

A new dbus service and interface will be created: "xyz.openbmc_project.Metric".
(Interface proposal linked below)

The interface will have all the metrics as properties, and they can be obtained
as such.

Any users can poll object path /xyz/openbmc_project/Metric/{bmc_id} with this
interface to get the metrics they wish to obtain.

Sample dbus command:

```
busctl call xyz.openbmc_project.Metric  /xyz/openbmc_project/Metric/BMC \
  org.freedesktop.DBus.Properties GetAll xyz.openbmc_project.Metric
```

Sample workflow:

```
BMCWeb                                                  phosphor-metric-collector
|                                                                  |
|    Get Property "BootCount" from xyz.openbmc_project.Metric      |
| - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -> |
|                                                                  |
|                                                                  |
|                        {"BootCount": 2}                          |
| <- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - |
|                                                                  |
|                                                                  |
```

Note: In the example above, the request must be a property of the interface
xyz.openbmc_project.Metric. If BootCount did not exist in the dbus-interface
schema, nothing would be returned.

### What is implemented here vs phosphor-health-monitor?

This design proposes that any software-based metric that is not a physical
reading be exported. To maintain concept of physical sensors in
phosphor-health-monitor, this design wishes to move over the Utilization sensor
to this new proposed daemon and any new non-physical readings should be exported
by this metric.

Some examples: Process/Service stats, System Boot Info, System uptime.

## Alternatives Considered

Running an external program might have security concerns (in that the 3rd party
program will need to be verified to be safe).

Collectd was an alternative that was seriously considered as well. However,
collectd's memory and high I/O usage would cause this daemon to be a better
choice.

## Impacts

Most of what the Metric Collecting Daemon does is to do scrap metrics from the
system and update DBus objects. The impacts of the daemon itself should be
small.

There will be impacts to phosphor-dbus-interfaces. An initial proposal of the
interface is
[here](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/59504)

### Organizational

- Does this repository require a new repository? Yes, phosphor-metric-collector

- Which repositories are expected to be modified to execute this design? Changes
  to the new repository, phosphor-metric-collector, and also to
  phosphor-dbus-interfaces

## Testing

Unit tests will ensure correct logic of the entire code base. Jenkins will be
used as the primary CI.
