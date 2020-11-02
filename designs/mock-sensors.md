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
allows us to specify custom sensor values and allows us to inject certain
errors, this tool would allow for a wider range of errors to be injected.

This tool is meant to serve as a lightweight and portable way of testing all
userspace code above the hwmon driver without requiring the use of hardware
(ovens/cooling chambers) or physically destroying the machine to replicate.
This allows the user to simulate all driver/userspace interactions without
having to rely on the behavior of the underlying hardware.

## Background and References

phosphor-hwmon[1] - the static sensor stack
dbus-sensors[2] - the dynamic sensor stack
Error cases we may want to inject at the kernel level read syscall:[3]

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

The goal of this design is to build a lightweight tool that can allow for
custom value and error injection into the sensor stack at the kernel level
to mock out faulty sensors to test various corner cases within the BMC.

This implementation should provide a user the ability to choose existing
sensors and mock out their sensor values or error values through the static
and dynamic sensor stack.


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

The design focuses on physical sensors that utilize sysfs.

Each sensor interacts with a linux driver that reads values from the physical
sensor and populates a file within sysfs[4] with the value read in. When the
sensor stack wants to retrieve these values, they're passed in a direct path
to the file within sysfs and a long value is read out of that file a read()
syscall. Once the value is retrieved, we can use ptrace to either skip the
read() syscall and return a custom value, or to override the value returned
from the read() syscall and the buffer associated with it.

## Using Ptrace

The mock sensor tool will not require any special build/build changes, making
the tool very lightweight and portable (35KB). For someone using the tool to
mock out values/errors in the static stack, the mock sensor tool will attach
itself to phosphor-hwmon using its PID (similar to strace) and begin overloading
user-specified sensor values. This method of tracing by PID only works if the
correct permissions are allowed (kernel.yama.ptrace_scope in
/etc/sysctl.d/10-ptrace.conf must be set to 0), however the correct permission
should be on by default.

Users can dynamically choose which sensors to mock and specify the values and
errors they want to inject into that specific sensor. Users will also be able
to specify a latency value which adds a delay between the polling from the
sensor stack and the response from the userspace.

For now, users will run the tool and enter the PID of the phosphor-hwmon
instance they want to overload and the injection will persist throughout the
lifetime of the instance of the mock sensor tool. Further research will be
required to see how the tool would be used with dbus-sensors, but ideally
would be identical to how the tool is used with the static stack.


## Alternatives Considered

There were two other design alternatives considered for this project:

## New Linux Driver using the Hwmon Framework

This idea is to build a custom debug linux driver that the user can control to
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
the sensors and would require kernel changes, which would require build changes.
We want to maintain the smallest configuration delta possible to the actual
platform firmware, and the ptrace approach does a better job of that than this
approach.


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

While this approach would provide us the most accurate mock sensors and we could
inject sensor values through the use of the monitor framework, there doesn't
seem to be any way to inject user-specified error values at this level.
Furthermore, this approach would only work for BMC instances specifically
emulated on top of QEMU, and we want this tool to be as portable as possible to
serve the most use-cases possible.

## Additional Design Considerations

There may be opportunity to blend the approaches above, specifically the main
ptrace approach to mock out existing sensors, and the custom linux drivers
to mock out completely new sensors. More research would have to be done in
how sensors are added to the dynamic sensor stack however.

## Impacts

***Security Impact***
There may be some security concerns in a malicious entity hijacking the
mock sensor tool to inject erroneous values, but this tool is meant to be used
for testing and development purposes and probably shouldn't be built into any
production environments otherwise. In the case that the tool is to be built in
to some environment, it can be disabled by disabling PTRACE-ATTACH permissions
in /proc/sys/kernel/yama/ptrace_scope.

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
