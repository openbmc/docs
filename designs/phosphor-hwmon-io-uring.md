# Using io_uring in BMCs for asynchronous sensor reads

Author: Jerry Zhu ([jerryzhu@google.com](mailto:jerryzhu@google.com))

Other contributors:

- Brandon Kim ([brandonkim@google.com](mailto:brandonkim@google.com), brandonk)
- William A. Kennington III ([wak@google.com](mailto:wak@google.com), wak)

Created: June 9, 2021

## Problem Description

Currently, OpenBMC has code that performs I2C reads for sensors that may take
longer than desired. These IO operations are currently synchronous, and
therefore may block other functions such as IPMI. This project will involve
going through OpenBMC repositories (specifically
[phosphor-hwmon](https://github.com/openbmc/phosphor-hwmon)) that may have this
drawback currently, and adding an asynchronous interface using the new io_uring
library.

## Background and References

io_uring is a new asynchronous framework for Linux I/O interface (added to 5.1
Linux kernel, 5.10 is preferred). It is an upgrade from the previous
asynchronous IO called AIO, which had its limitations in context of its usage in
sensor reads for OpenBMC.

[brandonkim@google.com](mailto:brandonkim@google.com) has previously created a
method for preventing sensors from blocking all other sensor reads and D-Bus if
they do not report failures quickly enough in the phosphor-hwmon repository
([link to change](https://gerrit.openbmc.org/c/openbmc/phosphor-hwmon/+/24337)).
Internal Google BMC efforts have also focused on introducing the io_uring
library to its code.

## Requirements

By using io_uring, the asynchronous sensor reads will need to maintain the same
accuracy as the current, synchronous reads in each of the daemons. Potential
OpenBMC repositories that will benefit from this library include:

- [phosphor-hwmon](https://github.com/openbmc/phosphor-hwmon)
- [phosphor-nvme](https://github.com/openbmc/phosphor-nvme)
- [dbus-sensors](https://github.com/openbmc/dbus-sensors)
- any other appropriate repository

The focus of this project is to add asynchronous sensor reads to the
phosphor-hwmon repository, which is easier to implement than adding asynchronous
sensor reads into dbus-sensors.

Users will need the ability to choose whether they want to utilize this new
asynchronous method of reading sensors, or remain with the traditional,
synchronous method. In addition, the performance improvement from using the new
io_uring library will need to be calculated for each daemon.

## Proposed Design

In the phosphor-hwmon repository, the primary files that will require
modification are
[sensor.cpp](https://github.com/openbmc/phosphor-hwmon/blob/master/sensor.cpp)/[.hpp](https://github.com/openbmc/phosphor-hwmon/blob/master/sensor.hpp)
and
[mainloop.cpp](https://github.com/openbmc/phosphor-hwmon/blob/master/mainloop.cpp)/[.hpp](https://github.com/openbmc/phosphor-hwmon/blob/master/mainloop.hpp),
as well the addition of a caching layer for the results from the sensor reads.

In mainloop.cpp currently, the `read()` function, which reads hwmon sysfs
entries, iterates through all sensors and calls `_ioAccess->read(...)` for each
one; this operation is potentially blocking.

The refactor will maintain this loop over all sensors, but instead make the read
operation non-blocking by using an io_uring wrapper. A caching layer will be
used to store the read results, which will be the main access point for
obtaining sensor reads in mainloop.cpp.

```
               Interface Layer
+--------------------------------------------+
|                                            |
|   +------------+         +-------------+   |
|   |            |         |             |   |
|   |   Redfish  |         |     IPMI    |   |
|   |            |         |             |   |
|   +-----+------+         +-------+-----+   |
|         ^                        ^         |
+---------|------------------------|---------+
          |                        |
          v                        v
+---------+------------------------+---------+
|                                            |
|                  DBus                      |
|                                            |
+---------^------------------------^---------+
          |                        |
  +-------v-------+       +--------v-------+
  |               |       |                |
  |phosphor-hwmon |       |  dbus-sensors  |
  |               |       |                |
  +-------^-------+       +--------^-------+
          | <--------------------- | <------- caching layer at this level
 +--------v------------------------v--------+
 |                                          |
 |               Linux kernel               |
 |                                          |
 +----------^---------------------^---------+
            |                     |
       +----v-----+         +-----v----+
       |          |         |          |
       |i2c sensor|         |i2c sensor|
       |          |         |          |
       +----------+         +----------+

```

Using a flag variable (most likely to be placed in the .conf files of each hwmon
sensor), users will be able to determine whether or not to utilize this new
io_uring implementation for compatibility reasons.

## Detailed Design

The read cache is implemented using an `unordered_map` of {sensor hwmon path:
read result}. The read result is a struct that keeps track of any necessary
information for processing the read values and handling errors. Such information
includes open file descriptor from the `open()` system call, number of retries
remaining for reading this sensor when errors occur, etc.

Each call to access the read value of a particular sesnor in the read cache will
not only return the cached value but will also submit a SQE (submission queue
event) to io_uring for that sensor; this SQE acts as a read request that will be
sent to the kernel. The implementation maintains a set of sensors that keeps
track of any pre-existing submissions so that multiple SQEs for the same sensor
do not get submitted and overlap; the set entries will be cleared upon
successful return of the read result using the CQE (completion queue event). The
CQE will then be processed, and its information will update the cache map.

The asynchronous nature of this implementation comes from sending all possible
SQE requests, a non-blocking operation, at once instead of being blocked by slow
sensor reads in the synchronous implementation. The kernel will process these
requests, and before the next iteration of sensor reads the cache will attempt
to process any returned CQEs, a non-blocking operation as well.

Simply put, an access to some "Sensor A" in the read cache will create an
underlying read request that makes a best effort to update the value of "Sensor
A" before the next time the sensor read loop (currently 1 s by default) gets the
value of "Sensor A" through the cache.

## Alternatives Considered

Linux does have a native asynchronous IO interface, simply dubbed AIO; however,
there are a number of limitations. The biggest limitation of AIO is that it only
supports true asynchronous IO for un-buffered reads. Furthermore, there are a
number of ways that the IO submission can end up blocking - for example, if
metadata is required to perform IO. Additionally, the memory costs of AIO are
more expensive than those of io_uring.

For these primary reasons, the native AIO library will not be considered for
this implementation of asynchronous reads.

## Impacts

This project would impact all OpenBMC developers of openbmc/phosphor-hwmon
initially. It has improved the latency performance of phosphor-hwmon; throughput
has also been shown to increase (note that throughput profiling was more
arbitrary than latency profiling). These performance changes will have to be
calculated in further detail across different machines.

There will be no security impact.

## Testing

The change will utilize the gTest framework to ensure the original functionality
of the code in the repository modified stays the same.
