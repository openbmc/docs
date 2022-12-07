
#  phosphor-metric-collector

Author: Willy Tu <wltu@google.com>, Sui Chen <suichen@google.com>, Edward Lee <edwarddl@google.com>

Created: Dec 7, 2022

##  Problem Description

How do we collect software-based metrics in a method that is lightweight, fast, and easily exportable?

Software-based metrics would allow much more information to monitor the overall health of the BMC and allow much more actionable measure to be taken if a BMC is failing. More information would allow problems to be potentially pinpointed much faster.

##  Background and References

References: [bmc-health-monitor.md](./bmc-health-monitor.md)

While the design doc linked above outlines the architecture for phosphor-health-monitor, as the code-base has evolved there are conventions and limitations that have been introduced into this daemon that have not been explicitly stated in the design document.

Restrictions of phosphor-health-monitor:
1. Metrics must be IPMI compliant, which restricts them to be 8 bit values that are percentages.
2. Metrics are based off of sensor readings, which represent physical hardware measurements.

##  Requirements

To solve the problem described above, it would not make sense to export complex software-based metrics via phosphor-health-monitor due to the restrictions above. This new daemon proposed would have the following requirements:

1. The daemon will periodically scrape metrics from the system and make them accessible anytime.
2. The daemon will provide a DBus service and interface to read the any specific requested metric.
3. The DBus interface shall not limit types of metrics to be exported. (Such as how phosphor-health-monitor models the DBus interface after IPMI sensors, limiting their capability)
4. The daemon will provide the user with the requested metric on-demand via DBus calls. (Does not mean the metrics are collected on-demand)

This metric collection daemon's purpose is essentially a more lightweight phosphor-health-monitor that is aimed towards exposing software-based metrics via DBus.

##  Proposed Design

There are two main parts of the daemon design: Metric Collection, Metric Service/Interface

### Metric Collection:
Periodically, this daemon will have built-in implementations of scraping metrics from the system itself. These metrics will be stored in a data structure in memory for the daemon and ready to be exported at any time. Any time the system scrapes metrics, the old metrics shall be discarded and updated with the new ones. This implementation and design philosophy is similar to phosphor-health-monitor.

### Metric Service/Interface:
A new dbus service and interface will be created: "xyz.openbmc_project.Metric".

The interface will have all the metrics as properties, and they can be obtained as such. 

Any users can poll object path /xyz/openbmc_project/{bmc_id}/Metric with this interface to get the metrics they wish to obtain.

Sample dbus command:
```
busctl call xyz.openbmc_project.Metric  /xyz/openbmc_project/{bmc_id}/Metric \
org.freedesktop.DBus.Properties GetAll string:xyz.openbmc_project.Metric 
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


##  Alternatives Considered

We have also tried doing the metric collection task by running an external binary as well as a shell script. It turns out running shell script is too slow, while running an external program might have security concerns (in that the 3rd party program will need to be verified to be safe).

Collectd was an alternative that was seriously considered as well. However, collectd's memory and high I/O usage would cause this daemon to be a better choice.

##  Impacts

Most of what the Metric Collecting Daemon does is to do scrap metrics from the system and update DBus objects. The impacts of the daemon itself should be small.

###  Organizational

- Does this repository require a new repository?
Yes, phosphor-metric-collector

- Who will be the initial maintainer(s) of this repository? 
???

- Which repositories are expected to be modified to execute this design?
None except the new repository created. (phosphor-metric-collector)

- Make a list, and add listed repository maintainers to the gerrit review.

##  Testing

Unit tests will ensure correct logic of the entire code base. Jenkins will be used as the primary CI.