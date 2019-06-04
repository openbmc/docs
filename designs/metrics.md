# OpenBMC Metrics Collector Daemon ("Metricsd")

Author: Kun Yi <kunyi@>

Primary assignee: Kun Yi <kunyi@>

Created: June 4, 2019

## Problem Description

Design a configurable BMC telemetry and health monitoring backend framework for
OpenBMC platforms. Specifically, this doc proposes

*   a general format for BMC metrics
*   how to integrate collectd/librrd to scrape and listen to metrics
*   how other components connect to the metrics data pipeline

The doc tries **not** to cover:

*   how metrics are logged
    *   A daemon may log metrics/events using one of the available logging
        mechanisms, but it should be optional instead of part of metrics.
*   how metrics are acted upon
    *   The metrics collection framework tries to be agnostic to the meaning of
        the metrics data. Instead, the system 'owner' can decide to have
        additional hooks that act based on collected metrics on a case-by-case
        basis.
*   which metrics should be reported
    *   Again, this is going to be highly variable between platforms. The whole
        point of the collection mechanism is to make it customizable.

## Background and References

Property                      | Log                                              | Metric                                                                                 | Event
----------------------------- | ------------------------------------------------ | -------------------------------------------------------------------------------------- | -----
Generally numeric             | No                                               | Yes                                                                                    | Maybe
Time sensitivity              | No                                               | Somewhat                                                                               | Yes
Urgency                       | No                                               | No                                                                                     | Maybe
Target                        | Human                                            | Automation                                                                             | Human/Automation
Impact of losing a data point | Medium                                           | Low                                                                                    | High
Examples                      | dmesg/kmesg<br>systemd journal<br>rsyslog<br>... | CPU load<br>memory usage<br>disk usage<br>daemon restart count<br>system uptime<br>... | catastrophic GPIO
Existing OpenBMC collection   | journal<br>rsyslog<br>Redfish logging            | ?                                                                                      | IPMI SEL<br>Redfish event logs

## Assumptions and Requirements

### Assumptions on the BMC system

The design is specifically targeting an OpenBMC system, which is an embedded
Linux distro with Systemd/D-Bus available. On the hardware side, we assume the
BMC system is an embedded system that is memory, storage, compute and I/O bound.

The design also assumes the BMC userspace is operating (or at least recoverable
to a normal state) when the metrics are collected and reported. How to recover
BMC in catastrophic situations is beyond the scope of this doc.

### Assumptions on metrics/events to be collected

We assume the metrics to be collected are interesting and meaningful to the
external observer, which is not necessarily human.

We also assume a finite number of metrics to be collected not exceeding 10^2
data points per second. Beyond this number, BMC may not have the bandwidth and
storage to keep the data.

### Requirements

Given the assumptions we layout requirements for the framework as the following:

*   Simple
    *   We keep features at a minimum with the mindset that the framework will
        be used as a building block, not the do-it-all infrastructure
    *   The baseline is that monitoring should **not** impact BMC stability and
        reliability
*   Both push/poll models are supported for collection
    *   Daemons and short-lived processes can push metrics to the collector
    *   Collector spawns a separate thread that can scrape metrics from
        kernel/userspace interfaces
*   Optional
    *   We aim to provide compile-time and run-time knobs such that when
        monitoring is not needed, it can be turned off with minimal performance
        overhead on the system
*   Best-of-effort perseverance of critical metrics
    *   Given the embedded nature of BMCs, it is nonsensical for it to hold an
        extremely long time worth of data points, or carry extensive computation
        to compress/aggregate the data
        *   Corollary: there must be an entity polling data from BMC constantly
            to guarantee there is no data loss
    *   However, poller unresponsiveness is still very common (think about a
        host reboot for updating) and BMC should persist critical data
        accounting for the time it takes to resume polling
        *   As a hypothetical scenario, for events happening at a 1/minute
            frequency, storing one day's worth of data points requires 1440 data
            points, which is not unreasonable

## Integrating Collectd/librrd

Collectd is a daemon which collects system and application performance metrics
periodically and provides mechanisms to store the values in a variety of ways,
for example in RRD files. [1]

Round Robin Database (RRD) is a format specially designed to handle timeseries
data, and collectd includes tools and libraries around it such as librrd,
rrdtool and rrdcached.

There are several advantages of using the collectd/librrd framework:

*   There are many plugins available for input/output already [2]
*   Collectd and most plugins are implemented in C, which reduces memory usage
    and binary size
*   RRD files are designed to be constant size over time, which suits BMC
    systems with limited storage
*   librrd can be used to implement a push model where different daemons can
    create RRD files either in memory or on the persistent storage
*   With collectd and RRD it is easy to implement basic data aggregation such as
    taking average or maximum of values over time

[1] https://collectd.org/ [2]
https://collectd.org/wiki/index.php/Table_of_Plugins

## Proposed Design

#### Collecting via collectd

For metrics that already able to be collected via collectd, almost nothing needs
to be done in addition to enable and configure the collectd plugin.

#### Collecting via librrd

OpenBMC processes can create arbitrary metrics through librrd. There will be
several helper methods created in phosphor-logging to allow creating metrics
objects, and calls to send them to filesystem via librrd "rrd-create" interface.

If we need to differentiate between metrics and only selectively persist them,
it can be done by creating separate calls to create files in different
filesystem partitions (for example, rrd_create() will send files to /tmp and
rrd_create_rw() will send them to /var/lib).

#### Querying metrics via Redfish

There are some relevant Redfish Schema available and can be implemented.
See[1][2][3] [1] https://redfish.dmtf.org/schemas/v1/MetricDefinition.json [2]
https://redfish.dmtf.org/schemas/v1/MetricReportCollection.json [3]
https://redfish.dmtf.org/schemas/v1/MetricReportDefinitionCollection.json

#### Export metrics to another collection endpoint

Sending metrics to another collectd network socket is natively supported via
collectd "network" plugin.

For transporting metrics to other collection frameworks, minimal effort is
needed if either: 1. Collectd has a plugin to export to this framework 2. This
framework has a plugin to scrape from a collectd network socket

#### Querying metrics via inband IPMI

In some cases, there is no network connection to the BMC. We propose an
interface on top of the phosphor-ipmi-blobs interface
(https://github.com/openbmc/phosphor-ipmi-blobs) to query the metrics:

BmcBlobEnumerate BmcBlobOpen BmcBlobRead BmcBlobWrite BmcBlobCommit BmcBlobClose
BmcBlobDelete

Host would transfer the metrics query protobuf to BMC, and BMC IPMI blob handler
will parse the protobuf, query the underlying RRD files, and prepare a protobuf
response.

### Example System Diagram

![diagram](designs/obmc-metrics.png "Metrics System Diagram")

## Alternatives Considered

Run existing infra like Prometheus on the BMC Although this is feasible, we feel
like it is not the best option since:

1.  Systems like Prometheus assumes at least ample storage for timeseries data,
    which obligates the use of external storage like eMMCs/SD cards for a BMC
    chip. Many BMC systems only have an SPI EEPROM with a few MBs of available
    space to start with.
2.  Performance cost is too high when each of the metric to be collected must
    exist as an HTTP endpoint to be scraped.

Use structured logs based on phosphor-logging or systemd journal Although it's
possible (in fact, the free-form nature of logging means any metrics can be
encoded as logs), we feel like there are two important requirements that are
hard to achieve:

1.  Guarantee quality of service for each individual metric. With this design, a
    metric event being thrashed will not prevent other metrics being logged,
    which is extremely important since BMC is limited in memory and storage.

2.  Configure persistence on a per-metric basis. Persistence for logging is
    usually configured on a severity or source level basis. In this case we have
    many metrics coming from the same source, but each of them may need
    different persistent level. For example, it makes sense to persist BMC boot
    count across reboots, but maybe not about BMC uptime.

## Impacts

TODO(kunyi)

## Testing

TODO(kunyi)
