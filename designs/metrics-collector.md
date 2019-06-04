# OpenBMC Metrics Collector Daemon ("Metricsd")

Author: Kun Yi <kunyi@>

Primary assignee: Kun Yi <kunyi@>

Created: June 4, 2019

## Problem Description

Design a configurable BMC telemetry and health monitoring backend framework for
OpenBMC platforms. Specifically, this doc proposes

*   a general format for BMC metrics
*   a collector daemon ("metricsd") that is able to scrape and listen to metrics
*   a serialization strategy for storing metrics
*   how other components connect to the metrics/events data pipeline

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

For simplicity, throughout the doc the term "metrics" is used to refer to what
others may refer to as "metrics and events".

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
*   Optimized for write
    *   A processing writing to the metricsd should spend a minimum amount of
        time blocking for write to complete
*   Optional
    *   We aim to provide compile-time and run-time knobs such that when
        monitoring is not needed, it can be turned off with minimal performance
        overhead on the system
*   Best-of-effort perseverance of critical metrics
    *   Given the embedded nature of BMCs, it is nonsensical for it to hold an
        extremely long time worth of data points, or carry extensive computation
        to compress/aggregate the data
    *   Corollary: there must be an entity polling data from BMC constantly to
        guarantee there is no data loss
    *   However, poller unresponsiveness is still very common (think about a
        host reboot for updating) and BMC should persist critical data
        accounting for the time it takes to resume polling
    *   As a hypothetical scenario, for events happening at a 1/minute
        frequency, storing one day's worth of data points requires 1440 data
        points, which is not unreasonable

## Proposed Design

### High-level Design

OpenBMC processes can create a metric object and send the serialized object(s)
to Metricsd via IPC. Metricsd will store the metrics in a hashtable in memory,
where the hash key is metric name, and value is an array of metric objects,
sorted by timestamp.

Metricsd will spawn a thread to actively scrape kernel + D-Bus interfaces for
critical metrics, and store them in the metrics table.

If one particular metric is defined to be persistent, metricsd will persist the
values to BMC RW filesystem when the metric array is updated, and load the
values from storage when starting.

When the array for a metric exceeds its length limit, the oldest entries are
deleted.

Metricsd will create a D-Bus subtree for each metric it has collected, and
expose a D-Bus interface for querying metrics data from it. Two interfaces will
be provided for each metric: one for getting a "snapshot" of the latest metric
received, and one for "streams" returning an array of most recent metrics until
the specified time.

### Example System Diagram

```none
                                   Consumers

                      +---------+            +----------------+
                      |         |            |                |
                      | Redfish <---+    +--->  Host interface|
                      |         |   |    |   |                |
                      +---------+   |    |   +----------------+
                                    |    |
                                    |    |
                                    |    |
                                    |    |
   +----------------+   report  +---+----+----+ serialize  +-------+
   |    Btbridged   +----------->             +------------>       |
   +----------------+           |             |            |       |
                                |   Metricsd  |            |Storage|
   +----------------+           |             |            |       |
   |     hwmond     +----------->             <------------+       |
   +----------------+           +------^------+ deserialize+-------+
                                       |
   +----------------+           +-------------+
   |     procfs     <-----------+             |
   +----------------+           |             |
                                |   Scraper   |
   +----------------+           |   thread    |
   |     IPMId      <-----------+             |
   +----------------+  scrape   +-------------+
```

### Metricsd Configuration

On start-up, metrics daemon loads a configuration file specifying what are the
metrics to collect and their metadata, in the format of key-value pairs. This
can include parameters global to the entire daemon such as total memory/storage
limit, and also per-metric parameters such as metrics name, maximum metric queue
length, how it should be persisted, and which entity the metric is bounded to
(similar to IPMI sensors -> entity mapping).

### Metrics Object Format

Each metric object is defined as one of {int, double, string}, or a histogram of
them.

```none
SingleBasicMetric:
variant[int64, double, string]
SingleMetricHistogramInt:
array[lower: int64, upper: int64, value: int64]

SingleMetricHistogramDouble:
array[lower: double, upper: double, value: double]

MetricsDataPoint:
struct[timestamp: uint64,
           data: variant[SingleBasicMetric, SingleMetricHistogramInt, SingleMetricHistogramDouble]]

MetricsData:
struct[array[MetricsDataPoint],  tags: array[string]]
Metricsd D-Bus Interface
The metrics daemon has a D-Bus interface for control:

Properties
Enabled: bool, read-write
```

For each metric type: it provides the following interface:

```none
Properties
Name: string, read-only
Tags: array[string], read-only
Snapshot: MetricsDataPoint, read-only

Methods
WriteStreams(MetricsData) -> bool
ReadStreams(timestamp) -> MetricsData
```

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
