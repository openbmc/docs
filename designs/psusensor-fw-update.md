# Firmware upgrades via PSUSensor

Author: Justin Ledford (justinledford@google.com)

Primary assignee: Justin Ledford (justinledford@google.com)

Other contributors: Jason Ling (jasonling@google.com)

Created: Mar 18, 2021

## Problem Description

Firmware updates and telemetry for PMBus devices are currently supported by
separate applications that are lacking in synchronization. This leads to race
conditions and bus contention errors.

## Background and References

Typically in a system with PMBus devices that require both firmware updates and
telemetry, dbus-sensors is used for telemetry reporting, and other software is
used for firmware updates.

Upgrading firmware on certain devices such as power sequencers and PWM
controllers with telemetry polling concurrently can result in bugs caused
by the interleaving of competing i2c transactions to these devices.

In the worst case, interfering telemetry can disrupt firmware upgrades and
brick machines.

## Requirements

-   Telemetry from PMBus devices should not interfere with firmware upgrades on
    those same devices (and vice versa)
-   Access to a single PMBus device should be mediated by a single owner

## Proposed Design

An existing telemetry solution such as dbus-sensors should be modified to
support firmware upgrades to avoid rewriting sensor gathering code, as well as
to avoid fragmentation within OpenBMC.

PSUSensor can be modified to implement the ItemUpdater design defined in the
`xyz.openbmc_project.Software` interface. PSUSensor will monitor for new
`Software.Version` objects created by the ImageManager to identify firmware that
is applicable to devices that telemetry is reported for. PSUSensor will then
create a `Software.Activation` object which can trigger the firmware upgrade
process by another process updating the `RequestedActivation` property.
Telemetry collection should be temporarily paused by PSUSensor by the
`RequestedActivation` handler until the firmware upgrade process is completed.
While telemetry is paused, the sensor should be marked unavailable with
the Availability interface, and the Sensor value should return NaN.

The ImageManager can be any software that implements the design defined in the
Software interface, including an existing implementation provided by
phopshor-bmc-code-mgmt's `phosphor-version-software-manager`.

The `Software.Version` object should be matched against a device using the
`xyz.openbmc_project.Inventory.Decorator.Compatible` interface that provides
compatibility strings. These strings are reported with the inventory item and
should be known to PSUSensor ahead of time to identify applicable firmware. The
`Software.Activation` objects should then implement a handler on the
`RequestedActivation` property that will trigger the firmware upgrade process by
starting a systemd unit. A mapping should be supplied to PSUSensor to map
compatibility strings to systemd units.

Note that this implementation is similar to phosphor-bmc-code-mgmt's
implementation of the ItemUpdater in `phosphor-image-updater` which triggers a
systemd unit to write BIOS firmware.

PSUSensor should also create `Software.Version` objects during start up to
report the firmware version currently installed on a device. The process to
retrieve the version will also be initiated by a systemd unit defined in the
mapping.

An example of a mapping of compatibility strings to systemd units supplied to
PSUSensor:

```
{
  "foo.software.element.mainsequencer.type.ADM1266": {
    "update": "adm1266-update@mainsequencer.service",
    "version": "adm1266-version@mainsequencer.service",
  },
  "foo.software.element.subsequencer0.type.ADM1266": {
    "update": "adm1266-update@subsequencer0.service",
    "version": "adm1266-version@subsequencer0.service",
  },
  ...
}
```

Users may also want to pass information such as bus and address of the device to
the systemd units through the unit's instance name specifier (%i). The mapping
file can declare variables in the unit names that are defined by PSUSensor, e.g.
`adm1266-update@${BUS}-${ADDRESS}.service`.

## Alternatives Considered

An alternative to starting systemd units to trigger the writing process is to
integrate the firmware writing code directly into PSUSensor with the
compatibility strings being used to select an implementation matching the
device. This approach may not be preferable, as code from CLIs or other daemons
would need to be copied out into PSUSensor, duplicating work. Additionally, some
devices may be programmed with an executable provided by vendors without
supplied code, or code that would be easily integrated into PSUSensor (e.g.
written in a different language than C++ or with a vastly different coding style
).

## Impacts

Updating a device risks the chance of bricking the system if the wrong image is
written. The update handler for a device should include a verification step to
validate a signed image before writing the image.

## Testing

This will be tested with integration tests to verify that the expected DBus
objects are created, and that the expected systemd units are started.
