# Device Tree GPIO Naming in OpenBMC

Author: Andrew Geissler (geissonator)

Primary assignee: Andrew Geissler (geissonator)

Other contributors:
  < None >

Created: April 3, 2020

## Problem Description
The Linux kernel has deprecated the use of sysfs to interact with the GPIO
subsystem. The replacement is a "descriptor-based" character device interface.

[libgpiod][1] is a suite of tools and library implemented in C and C++ which
provides an abstraction to this new character device gpio interface.

libgpiod provides a feature where you can access gpios by a name given to
them in the kernel device tree files. The problem is there are no naming
conventions for these GPIO names and if you want userspace code to be able
to be consistent across different machines, these names would need to be
consistent.

## Background and References
The kernel [documentation][2] has a good summary of the GPIO subsystem. The
specific field used to name the GPIOs in the DTS is `gpio-line-names`.
This [patch][3] shows an example of naming the GPIOs for a system.

GPIOs are used for arbitrary things. It's pretty hard to have a coherent naming
scheme in the face of a universe of potential use-cases.

Scoping the problem down to just the vastness of OpenBMC narrows the
possibilities quite a bit and allows the possibility of a naming scheme to
emerge.

## Requirements
- Ensure common function GPIOs within OpenBMC use the same naming convention

## Proposed Design
Below are the standard categories. The "Pattern" in each section describes the
naming convention and then the sub bullets list the common GPIO names to
be used (when available on an OpenBMC system). This naming convention must be
followed for all common GPIOs.

This list below includes all common GPIOs within OpenBMC. Any OpenBMC
system which provides one of the below GPIOs must name it as listed in
this document. This document must be updated as new common GPIOs are added.

### BMC Properties

#### bmc-platform-management
Indicates whether the management interfaces required by the platform are
provided by the BMC. These interfaces may include any of Redfish, IPMI, SSH or
other appropriate protocols. Asserting the GPIO does not indicate that the BMC
or platform as a whole is healthy, rather, if it is unhealthy the problems can
now be inspected via the management interfaces. Conversely if it is deasserted
it is not necessarily an indication that the BMC or platform is unhealthy,
rather that the management interfaces are not reachable, for example as a result
of rebooting the BMC.

#### bmc-secure-boot
Input pin that indicates that the BMC is configured to boot with security
protections enforced.

Pulled up by default (secure). Placing the jumper will pull the pin down
(bypass security).

### Buttons
Pattern: `*-button`

#### power-button

### LEDs
Pattern: `led-*`

#### led-fault
#### led-identify
#### led-power
#### led-sys-boot-status
#### led-attention
#### led-hdd-fault
#### led-rear-fault
#### led-rear-power
#### led-rear-id

### Power and Regulators
Pattern: `power-*`, `regulator-*`

#### power-chassis-control
Set to initiate power-on or power-off of the chassis.

#### power-chassis-good
Indicates the power good state of the chassis.

#### power-config-full-load
Output GPIO set by the power managing application that indicates to the hardware
the threshold of power supplies that are expected to be present and working for
this type of system for the case where a system has a model that supports two
different number of power supplies (example 2 or 4). Default is the lowest
number of power supplies expected of the two models. In the case when the number
of power supplies that are present are less than the ones indicated by this
GPIO, the hardware can then take actions such as reducing the system's
performance so that it can maintain its powered on state.

#### regulator-standby-faulted
This GPIO value represents the status of standby power regulator fault detection
logic. This GPIO is an input only. The status will reflect a regulator
non-faulted condition after AC power cycle when no standby power regulator fault
condition is present. The status will reflect a standby regulator power faulted
condition when an unexpected drop in standby power is detected.

### Presence
Pattern: `presence-*`

#### presence-ps0, presence-ps1, ..., presence-ps\<N>

### Special
These are special case and/or grandfathered in pin names.

#### air-water
Indicates whether system is air or water cooled

#### factory-reset-toggle
The software records the state of this GPIO and checks upon reboot if the state
has changed since the last reboot. If it has, it indicates that a factory reset
should be performed.

### POWER Specific GPIOs
Below are GPIO names specific to the POWER processor based servers.

#### Special
These are special case and/or grandfathered in pin names.

##### cfam-reset
Utilized to issue a processor logic reset to a IBM POWER processor.

##### checkstop
Utilized to indicate a IBM POWER processor has entered an unrecoverable error
state.

## Alternatives Considered
- Continue to hard code a config file per system type that has the
gpio bank and pin number. This removes a dependency on the device tree to
have consistent names but adds overhead in supporting each new system.

- Have the device tree GPIO names match the hardware schematics and then
have another userspace config file that maps between the schematic names
and logical pin names. This makes the GPIO to schematic mapping easy but
adds an additional layer of work with the userspace config.

## Impacts
Need to ensure OpenBMC device trees conform to the above naming conventions.

## Testing
Userspace utilization of the GPIO names will provide some testing coverage
during CI.

[1]: https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/about/
[2]: https://www.kernel.org/doc/html/latest/driver-api/gpio/index.html
[3]: https://lore.kernel.org/linux-arm-kernel/20200306170218.79698-1-geissonator@yahoo.com/
