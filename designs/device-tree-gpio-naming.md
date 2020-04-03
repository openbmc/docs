# Device Tree GPIO Naming in OpenBMC

Author: Andrew Geissler (geissonator)

Primary assignee: Andrew Geissler (geissonator)

Other contributors:
  < Name and/or IRC nic or None >

Created: April 3, 2020

## Problem Description
The Linux kernel has deprecated the use of sysfs to interact with the GPIO
subsystem. The replacement is a "descriptor-based" character device interface.

[libgpiod][1] is a c and c++ suite of tools and library which provides an
abstraction to this new character device gpio interface.

libgpiod provides a feature where you can access gpios by a name given to
them in the kernel device tree files. The problem is there are no naming
conventions for these GPIO names and if you want userspace code to be able
to be consistent across different machines, these names would need to be
consistent.

## Background and References
The kernel [documentation][2] has a good summary of the GPIO subsystem. The
specific field used to name the GPIO's in the DTS is `gpio-line-names`.
This [patch][3] shows an example of naming the GPIO's for a system.

There does not appear to be any consistent naming conventions yet in the
open source world.

## Requirements
- Ensure common function GPIO's within OpenBMC use the same naming convention

## Proposed Design
Below are the standard categories. The first bullet in each section describes
the naming convention and then lists some common GPIO names to be used (when
available on an OpenBMC system)

### LEDs
- led-*
- led-fault, led-identify, led-power, led-sys-boot-status, led-attention
  led-hdd-fault, led-rear-fault, led-rear-power, led-rear-id
### Power
- power-*
- power-button
### Buttons
- \*-button
- power-button
### Presence
- presence-*
- presence-ps0, presence-ps1
### FSI
- fsi-mux, fsi-enable, fsi-trans, fsi-clock, fsi-data, fsi-routing
### Special
- cfam-reset
- checkstop
- air-water

## Alternatives Considered
Continue to hard code a config file per system type that has the
gpio bank and pin number. This removes a dependency on the device tree to
have consistent names but adds overhead in supporting each new system.

## Impacts
Need to ensure OpenBMC device trees conform to the above naming conventions.

## Testing
Userspace utilization of the GPIO names will provide some testing coverage
during CI.

[1]: https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/about/
[2]: https://www.kernel.org/doc/html/latest/driver-api/gpio/index.html
[3]: https://lists.ozlabs.org/pipermail/openbmc/2020-March/020879.html
