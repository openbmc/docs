# i2c Device Management Daemon

Author: <justinledford@google.com>

Other contributors: <jasonling@google.com>

## Problem Description

Provide a single daemon to manage i2c devices that require firmware upgrades,
sensor telemetry and device-specific information reporting.

## Background

Existing solutions for supporting firmware upgrades of i2c devices consist
mostly of scripts invoked by phosphor-ipmi-flash. These scripts don’t provide
standard phosphor-dbus-interfaces APIs that can be useful for other applications
to consume. In addition, scripts are more error-prone, errors don’t surface
until run-time and are generally not easy to properly test.

A daemon would be able to provide the standard APIs and stability that is
lacking from shell-script based solutions.

Additionally, the existence of two separate applications interacting with an i2c
device can lead to race conditions and bus contention errors. This single daemon
would encapsulate all access to a device to remove these errors.

## Requirements

-   The daemon should provide a DBus interface to load firmware to devices, and
    retrieve the activated firmware version running on devices using the
    `xyz/openbmc_project/Software` interfaces.
-   The daemon should provide a DBus interface to report sensor telemetry for
    devices following the `xyz/openbmc_project/Sensor` interfaces.
-   The daemon should provide a DBus interface to access various information
    used by clients that traditionally doesn’t fit into sensor reporting
    software, e.g. non-sensor commands such as VOUT_COMMAND.

## Proposed Design

### Firmware Update

The daemon should, at a minimum, implement the ItemUpdater API described by the
`xyz/openbmc_project/Software` interface. The daemon can then monitor for
Software.Version elements from a separate ImageManager service (such as
`image_manager_main` in phosphor-bmc-code-mgmt) to initiate a firmware update on
a device.

The daemon should also handle multi-slot updates (multiple devices of the same
hardware type) but it should also have the ability to handle cases where there
are multiple devices and different firmware images for some of those devices
(e.g. 3 devices of the same type on a system, but 2 require firmware A and 1
requires firmware B).

### Sensor Telemetry

The daemon should implement the `xyz/openbmc_project/Sensor` interface to
provide telemetry. Internally, there should be a mechanism to enable/disable
telemetry during firmware updates and other device access to prevent i2c
collisions.

A device with sensors presented from this daemon shouldn't also have sensors
reported by another daemon. This daemon could include implementations for
reading from underlying hwmon drivers, or in device-specific code contained in
this daemon.

### Device Information

An interface should also be provided to read various information about a device,
such as non-sensor PMBus commands, or device-specific information that isn’t
represented by a standard PMBus command.

An example of a non-sensor PMBus command interface:

```
Interface: xyz/openbmc_project/I2CDevice/PMBus

Property Name: VoutCommand

Description: Output voltage set by PMBus.
```

## Daemon Configuration

The daemon should use the EntityManager configuration to detect a device,
including the device’s name, bus, address and device type.

The device type is a string and should correspond to a specific implementation
for a device existing in the daemon (e.g. adm1266, adp1050, max20730).

An example of a section from the EntityManager configuration would contain the
following:

``` json
{
  "Exposes": [
    {
      "Name": "adm1266_0",
      "Bus": "19",
      "Address": "0x4d"
      "I2CDeviceType": "adm1266",
    },
    {
      "Name": "adm1266_1",
      "Bus": "21",
      "Address": "0x4d"
      "I2CDeviceType": "adm1266",
    },
    {
      "Name": "isl68229",
      "Bus": "17",
      "Address": "0x38"
      "I2CDeviceType": "isl68229",
    },
  ]
}
```
