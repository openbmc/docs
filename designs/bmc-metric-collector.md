# phosphor - metric - collector

Author: Edward Lee <edwarddl@google.com>, Sui Chen <suichen@google.com>, Willy
Tu <wltu@google.com>

Created: Dec 7, 2022

## Problem Description

How do we collect BMC software-based health metrics in a method that is
lightweight, fast, and easily exportable?

Software-based metrics would allow more information to monitor the overall
health of the BMC software and allow more actionable measures to be taken if a
BMC software is failing or regressing. More information would allow problems to
be pinpointed much faster.

As a user, I want to be able to obtain the metrics I need in order to monitor my
BMC without actually having to scrape my system for the metric itself. For
example, I do not want to manually go through the `/proc` filesystem to
determine the memory usage of a specific process. This example can extend to
process file descriptors, restart counts, and other software-based metrics.

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

This design highly models after phosphor-health-monitor with some slight
differences.

Characteristics of phosphor-health-monitor:

1. Currently everything is considered a Sensor

As the first non-physical sensor, Sui has added utilization sensors and this has
come with some apprehension with the maintainers of phosphor-health-monitor. The
maintainers want to avoid the trap of where 'everything is a sensor', and this
means that any metrics reported as a sensor should ideally be a physical
measurement. Realistically, utilization does not belong as a sensor and other
metrics of similar category need a place to reside.
[Source](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/51779)

2. Metrics are scraped on query

Currently whenever a property is queried from phosphor-health-monitor, the
metrics are scraped as the program receives the query. From the user's
standpoint, when they query for a metric from phosphor-health-monitor, they have
to wait for: the daemon to receive the query, the daemon to scrape the metric,
and the response to travel back from the daemon. Now this could be optimized
with caching the metrics will be further discussed below.

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
Using this example, telemetry states that its flexibility and modularity allows
other daemons to be used in cooperation with it. This example is essentially the
design proposed by this document, with collectd replaced with this proposed
metric-collector daemon.

Their architecture shows collectd in conjunction with telemetry because they
source different types of metrics. Telemetry was created to be flexible and
modular and its intention was to expose telemetry data of the host through DBus.

## Requirements

To solve the problem described above, it would not make sense to export complex
software-based metrics via the current state of phosphor-health-monitor.

With this, I propose a new addition to phosphor-health-monitor that could either
go in as a new daemon or a separate scanning mechanism that collects and scrapes
software-based metrics from the system using an entire separate DBus interface
to represent different models of software based metrics on the BMC. The DBus
interface addition is linked further below.

This new feature proposed would have the following requirements:

1. The feature will periodically scrape metrics from the system and make them
   accessible anytime. The period is further discussed below.
2. The feature will provide a DBus service and interface(s) to read the any
   specific requested metric.
3. The feature will provide the user with the requested metric on-demand via
   DBus calls. (Does not mean the metrics are collected on-demand). Essentially,
   metrics will be cached and returned instead of being scraped on query.
4. All metrics that are exported must be defined in xyz.openbmc_project.Metrics.

This metric collection feature has the same goal as phosphor-health-monitor
program but is aimed towards exposing cached software-based metrics via DBus for
lower query latency.

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

There are two main parts of the design: Metric Collection and Metric
Service/Interface

### Metric Collection

Periodically, the program will scrape metrics from the system itself. These
metrics will be stored in a data structure in memory for the daemon and ready to
be exported at any time. Any time the system scrapes metrics, the old metrics
shall be discarded and updated with the new ones. This stages the metrics for
exporting via dbus and allows them to always be available without blocking the
user making the dbus call.

How often a metric should be scraped is a decision that may differ based on the
type of metric. Certain metrics such as boot count or crash count are not as
high priority as CPU or memory usage of a process, for example. This design will
use JSON config files that detail which metrics will be collected with variable
refresh times to both optimize useless collection of low priority metrics and
allow users to obtain the most up-to-date information on high priority metrics.

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
BMCWeb                                                       metric-collector
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

### What is implemented here vs what is already there?

This design proposes that any software-based metric that is not a physical
reading be exported. To maintain the concept of physical sensors in
phosphor-health-monitor, this design wishes to move over the Utilization sensor
to this new proposed interface and any new non-physical readings should be
exported by this service.

Some examples: Process/Service stats, System Boot Info, System uptime.

## Alternatives Considered

There are two options for this proposed feature above. This can be incorporated
into a separate service running in the current phosphor-health-monitor program
or can be a separate program in the same repo. This design pushes for this
feature to be added to the same daemon, but this would require some
refactorization of the current code. This is an implementation decision and does
not affect the overall design of the feature itself.

## Impacts

Most of what the metric collection does is to do scrap metrics from the system
and update DBus objects.

There will be impacts to phosphor-dbus-interfaces. An initial proposal of the
interface is
[here](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/59504)

### Organizational

- Does this repository require a new repository? No, it will go in
  phosphor-health-monitor

- Which repositories are expected to be modified to execute this design? Changes
  to phosphor-health-monitor, and bmcweb to query and expose the metrics.

## Testing

Unit tests will ensure correct logic of the entire code base. Jenkins will be
used as the primary CI.
