# Control.ThermalMode dbus interface with Supported and Current properties

Author: Matthew Barth !msbarth

Other contributors: None

Created: 2019-02-06

## Problem Description

An issue was discovered where the exhaust heat from the system GPUs causes
overtemp warnings on optical cables on certain system configurations. The issue
can be resolved by altering the fan control application's floor table,
effectively raising the floor when these optical cables exist but an interface
is needed to do so. Since the issue revolves around the optical cables
themselves, where no current mechanism exists to detect the presence of the
optical cables plugged into a card downwind from the GPUs' exhaust, an end-user
must be presented with an ability to enable this raised floor speed table.

## Background and References

The witherspoon system supports pci cards that could have optical cables plugged
in place of copper cables. These optical cables can report overtemp warnings to
the OS when high GPU utilization workloads exist. When this occurs with low
enough CPU utilization, the fans could be kept at a given floor speed that
sufficiently cools the components within the chassis, but not the optical cables
with the slow moving hot exhaust.

Without an available exhaust temp sensor, there's no direct way to determine the
exhaust temp and include that within the fan control algorithm. A similar issue
exists on other system where mathematical calculations are done based on the
overall power dissipation.

Mathematical calculations to logically estimate exit air temps:
https://github.com/openbmc/dbus-sensors/blob/master/src/ExitAirTempSensor.cpp

## Requirements

Create the ability for an end-user to enable the use of a thermal control mode
other than the default. In this use-case, the mode is specific to an
undetectable configuration that alters the fan floor speeds unrelated to
standardized profile/modes such "Acoustic" and "Performance". Once the end-user
selects a documented mode for the platform, the thermal control application
alters its control algorithm according to the defined mode, which is
implementation specific to that instance of the application on that platform.

## Proposed Design

Create a Control.ThermalMode dbus interface containing a supported list of
available thermal control modes along with what current mode is in use.
Initially the current mode would be set to "Default" and the implementation of
the interface would populate the supported list of modes.

As one implementation, phosphor-fan-presence/control would be updated to extend
this dbus interface object which would fill in the list of supported modes from
its fan control configuration for the platform. Once the fan control application
starts, the interface would be added on the zone object and available to be
queried for supported modes or update the current mode. An end-user may set the
current mode to any of those supported modes and the current mode would be
persisted each time it is updated. This is to ensure each time the fan control
application zone objects are started, the last set control mode is used.

## Alternatives Considered

Mathematical calculation to create a virtual exhaust temp sensor value based on
overall power dissipation. However, in the witherspoon situation, using this
technique would not be reliable in adjusting the floor speeds for only
configurations using optical cables. This would instead present the possibility
of raising floor speeds for configurations where its unnecessary.

## Impacts

The thermal control application used must be configured to provide what thermal
control modes are supported/available on the interface as well as perform the
associated control changes when a mode is set.

## Testing

Trigger the use of an alternative fan floor table based on the thermal control
mode selected on a witherspoon system.
