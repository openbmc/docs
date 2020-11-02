# Mock Sensors

Author:
  Jun Ho Park <pjunho@google.com>

Primary assignee:
  Jun Ho Park <pjunho@google.com>

Other contributors:
  Brandon Kim <brandonkim@google.com>; <brandonk>
  Nancy Yuen <yuenn@google.com>; <yuennancy>

Created:
  Oct 28, 2020

## Problem Description

Currently phosphor-hwmon does not allow custom values and error cases
to be injected into the static sensor stack. Furthermore, while dbus-sensors
allows us to specify custom sensor values, it also does not allow error
cases to be injected into the dynamic sensor stack. The goal of this design
is to build a lightweight tool that can allow for custom value and error
injection into the sensor stack at the kernel level to mock out faulty sensors
to test various corner cases within the BMC.


## Background and References

phosphor-hwmon[1] - the static sensor stack
dbus-sensors[2] - the dynamic sensor stack
Error cases we may want to inject at the kernel level read syscall: [3]

High level diagram of the sensor stack:

```
               Interface Layer
+--------------------------------------------+
|                                            |
|   +------------+         +-------------+   |
|   |            |         |             |   |
|   |  Redfish   |         |     IPMI    |   |
|   |            |         |             |   |
|   +-----+------+         +-------+-----+   |
|         ^                        ^         |
+---------|------------------------|---------+
          |                        |
          v                        v
+---------+------------------------+---------+
|                  DBus                      |
|                                            |
+---------^------------------------^---------+
          |                        |
  +-------v-------+       +--------v-------+
  |               |       |                |
  |phosphor-hwmon |       |  dbus-sensors  |
  |               |       |                |
  +-------^-------+       +--------^-------+
          |                        |
 +--------v------------------------v--------+
 |               linux kernel               |
 |                                          |
 +----------^---------------------^---------+
            |                     |
       +----v-----+         +-----v----+
       |          |         |          |
       |i2c sensor|         |i2c sensor|
       |          |         |          |
       +----------+         +----------+

```

## Requirements

This implementation should:

1. Provide a user the ability to choose existing sensors and mock out their 
sensor values or error values through the static and dynamic sensor stack.

2. Allow the user to mock out a custom virtual sensor with user-specified
values to mock out non-existing hardware.

## Proposed Design

In order to most effectively mock sensors, the ptrace framework can be used
to intercept read() calls to the kernel from phosphor-hwmon and dbus-sensors 
and once intercepted, the user can decide what value to put into the buffer or
what error value they'd want to return back to the sensor stack.

```
               Interface Layer
+--------------------------------------------+
|                                            |
|   +------------+         +-------------+   |
|   |            |         |             |   |
|   |  Redfish   |         |     IPMI    |   |
|   |            |         |             |   |
|   +-----+------+         +-------+-----+   |
|         ^                        ^         |
+---------|------------------------|---------+
          |                        |
          v                        v
+---------+------------------------+---------+
|                  DBus                      |
|                                            |
+---------^------------------------^---------+
          |                        |
  +-------v-------+       +--------v-------+
  |               |       |                |
  |phosphor-hwmon |       |  dbus-sensors  |
  |               |       |                |
  +-------^-------+       +--------^-------+
          | <--------------------- | <------ Intercepting values at this level
 +--------v------------------------v--------+
 |               linux kernel               |
 |                                          |
 +----------^---------------------^---------+
            |                     |
       +----v-----+         +-----v----+
       |          |         |          |
       |i2c sensor|         |i2c sensor|
       |          |         |          |
       +----------+         +----------+
```

## How Sensor Values are Retrieved

Each sensor interacts with a linux driver that reads values from the physical
sensor and populates a file within sysfs[4] with the value read in. When the
sensor stack wants to retrieve these values, they're passed in a direct path
to the file within sysfs and a long value is read out of that file using the
STL's ifstream library. Since ifstream uses a read() syscall to retrieve the
value from the file in sysfs, we can use ptrace to either skip the read()
syscall and return a custom value, or to override the value returned from the
read() syscall and the buffer associated with it.

## Enabling ptrace

In order to use ptrace, the tracee program must be a child process of the
tracer program. For our ptrace tool, we must spawn phosphor-hwmon or
dbus-sensors as a child process using of the ptrace tool through the use of
execve (or execvp)[5]. This would require some build changes - for phosphor-
hwmon, this can be accomplished by using a drop-in[6] within ptrace-tool's
bitbake file to overload the ExecStart argument in phosphor-hwmon's .service
file[7]. A similar approach for dbus-sensors could probably be applied, but
more research would have to be done.

## User Config Tool

We want users to be able to dynamically change what sensors and sensor values
to mock. We believe the best way to enable this would be through the use of
a config file that ptrace-tool would periodically read from to determine its
arguments. The user would interact with a different tool that allows the user
to modify this config file without having to worry about formatting errors.
The config file could live in the /tmp directory of Linux[8].


## Alternatives Considered

There were two other design alternatives considered for this project:

## New Linux Driver using the Hwmon Framework

This idea is to build a custom linux driver that the user can control to 
mock the existence of a sensor. Values would be mocked at the following level:

```
  +-------v-------+       +--------v-------+
  |               |       |                |
  |phosphor-hwmon |       |  dbus-sensors  |
  |               |       |                |
  +-------^-------+       +--------^-------+
          |                        |
 +--------v------------------------v--------+
 |               linux kernel               |
 |                                          | <---- driver written in kernel
 +----------^---------------------^---------+
            |                     |
       +----v-----+         +-----v----+
       |          |         |          |
       |i2c sensor|         |i2c sensor|
       |          |         |          |
       +----------+         +----------+

```

This approach is fairly easy to implement and doesn't require modifying any
existing functionality within phosphor-hwmon or dbus-sensors and also doesn't
require changing the build order using drop-in. However, to overload values of
existing sensors, a custom driver or driver patch would have to be built for
each sensor and this solution would not scale very well.


## Adding Emulated Sensor Devices to QEMU using their i2c Framework

Using QEMU's i2c framework, we can "plug in" new devices to QEMU directly. We
can treat this new sensor exactly as we would any other sensor, and we could
expect it to behave like any other sensor once we emulate the BMC on top of
QEMU. The values would be mocked at the following level:

```
  +-------v-------+       +--------v-------+
  |               |       |                |
  |phosphor-hwmon |       |  dbus-sensors  |
  |               |       |                |
  +-------^-------+       +--------^-------+
          |                        |
 +--------v------------------------v--------+
 |               linux kernel               |
 |                                          |
 +----------^---------------------^---------+
            |                     |
       +----v-----+         +-----v----+
       |          |         |          |
       |i2c sensor|         |i2c sensor| <--- mock sensor emulated here
       |          |         |          |
       +----------+         +----------+

```

While this approach would provide us the most accurate mock sensor, we would
still need to build a compatible linux driver for each mocked sensor leading
to the same scaling issues above. Furthermore, this approach would only work
for BMC instances specifically emulated on top of QEMU.

## Additional Design Considerations

There may be opportunity to blend the approaches above, specifically the main
ptrace approach to mock out existing sensors, and the custom linux kernel
to mock out completely new sensors. More research would have to be done in
how sensors are added to the sensor stack however.

## Impacts

***Security Impact***
There may be some security concerns in a malicious entity hijacking the
configuration file that feeds arguments into the ptrace tool, however this
tool is meant to be used strictly in a test environment and not in any
production environment. With this in mind, there shouldn't be any security
issues.

***Performance Impact***
Using the ptrace tool will introduce some overhead - every time there's a
system call, the program will jump to the tracer to process the syscall and
perform extra work if necessary (encountered a read() syscall). However, this
performance loss should be fairly minor (most syscalls will return
immediately).

## Testing
Unit tests will be written with mock sensor files and a mock sensor stack that
reads from these sensor files.

[1]: (https://github.com/openbmc/phosphor-hwmon)
[2]: (https://github.com/openbmc/dbus-sensors)
[3]: (https://man7.org/linux/man-pages/man2/read.2.html)
[4]: (https://man7.org/linux/man-pages/man5/sysfs.5.html)
[5]: (https://man7.org/linux/man-pages/man2/execve.2.html)
[6]: (https://coreos.com/os/docs/latest/using-systemd-drop-in-units.html)
[7]: (https://github.com/openbmc/meta-phosphor/blob/master/recipes-phosphor/sensors/phosphor-hwmon/xyz.openbmc_project.Hwmon%40.service)
[8]: (https://tldp.org/LDP/Linux-Filesystem-Hierarchy/html/tmp.html)
