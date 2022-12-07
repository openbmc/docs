
#  phosphor-metric-collector

Author:

< Name and Discord nickname >

Other contributors:

< Name and/or Discord nickname or `None` >

Created: Dec 7, 2022

##  Problem Description

The initial problem to solve is to collect information from the BMC to monitor and measure its health. Certain information such as boot info, network info, and per-daemon metrics would significantly decrease time debugging problems on the BMC and with this information, users would be able to monitor the overall health of the BMC. 

##  Background and References

References: bmc-health-monitor.md

Currently phosphor-health-monitor contains these steps. Each of which, I will touch on in the Requirements Section.

1. Configuration
2. Collection
3. Staging
4. Data Transfer

##  Requirements

phosphor-health-monitor is a crucial reference for this discussion. This proposal essentially proposes a daemon similar to phosphor-health-monitor. There are a few key differences which result in requirements for this daemon.

1. Configuration is not needed in this daemon. Metrics will be uploaded as needed by the community.
2. Collection will be very similar. Any metric needed by the community will be collected here.
3. Staging will be similar to phosphor-health-monitor, but metrics are not required to be staged. They might be obtained on request.
4. Data Transfer will be limited the dbus.

The key requirements are below
1. The daemon will provide the user with the requested metric on demand via dbus.
2. The daemon will provide a dbus interface to read the any specific requested metric.
3. Any metric can be collected and is not limited to size constraints.

This metric collection daemon's purpose is essentially a more lightweight phosphor-health-monitor, with respect to the fact of its simpler purpose, with the ability to collect more complicated metrics to expose to users.

##  Proposed Design

These are similar to steps 2 and 3 of the design of phosphor-health-monitor.

1. Metric Collection
Metric collection will be done within the daemon itself and opposed to running external programs for performance and security concerns.

2. Metric Interface
A new dbus service will be created: "xyz.openbmc_project.Metric". This new dbus service object paths for each metric that is collected

"xyz.openbmc_project.Metric":

```

/xyz/openbmc_project

└─/xyz/openbmc_project/metrics

└─/xyz/openbmc_project/metrics/daemon/

└─/xyz/openbmc_project/sensors/daemon/MemoryUsage

```

Sample user.

```
BMCWeb                                  phosphor-metric-collector
|                                                   |
|   xyz.openbmc_project.Metric.GetAllDaemonMetrics  |
| - - - - - - - - - - - - - - - - - - - - - - - - > | - - - - - - - - - - 
|
|
|

```

##  Alternatives Considered

(2 paragraphs) Include alternate design ideas here which you are leaning away

from. Elaborate on why a design was considered and why the idea was rejected.

Show that you did an extensive survey about the state of the art. Compares

your proposal's features & limitations to existing or similar solutions.

##  Impacts

API impact? Security impact? Documentation impact? Performance impact?

Developer impact? Upgradability impact?

###  Organizational

- Does this repository require a new repository? Yes, phosphor-metric-collector

- Who will be the initial maintainer(s) of this repository? Edward Lee

- Which repositories are expected to be modified to execute this design? None except the new repository created. (phosphor-metric-collector)

- Make a list, and add listed repository maintainers to the gerrit review.

##  Testing

How will this be tested? How will this feature impact CI testing?