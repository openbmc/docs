# Multi-host front panel phosphor buttons interface

Author: Velumani T(velu), [velumanit@hcl](mailto:velumanit@hcl.com) Naveen Moses
S(naveen.moses), [naveen.mosess@hcl.com](mailto:naveen.mosess@hcl.com)

Other contributors:

Created: August 3, 2021

## Problem Description

Phosphor buttons module has currently option to monitor gpio events of power and
reset buttons and trigger power event handlers.

phosphor-buttons currently only support push type buttons.support for different
form factor inputs such as switches/MUX other than push type buttons are needed
for some hardwares. It will be helpful if we could add option to support
additional input types in phosphor button interfaces and event handling.

Currently handler events are only based on monitoring gpio events as input
(power and reset).There may be cases where we need to create button interface
which monitors non gpio events and triggers button actions for example events
based on dbus property changes.

## Requirements

This feature is needed to support additional phosphor button interfaces
corresponding to platform specific hardware buttons/MUX/Switches which are
available in the front panel apart from existing power and reset button
interfaces.

## Background and References

The front panel of bmc has buttons like power button, reset button in general
the code for monitoring these buttons and triggering actions are supported.

```
   +----------------------------------------------+
   |                                              |
   |       Front panel                            |
   |                                              |
   |   +--------------+      +--------------+     |
   |   |              |      |              |     |
   |   | power button |      | reset button |     |
   |   |              |      |              |     |
   |   +--------------+      +--------------+     |
   |                                              |
   |                                              |
   |                                              |
   |                                              |
   +----------------------------------------------+
```

platform specific front panel may contain additional hardware buttons or mux
switch

example for a multihost platform yosemite-V2 it has host selector switch mux as
additional button in front panel.

```
+---------------------------+                                      +------------+
|                           |                                      |            |
|       yv2 front panel     |           power/           ---------+|  host 1    |
|                           |           reset/serial mux |         |            |
|     +----------------+    |           control          |         +------------+
|     |                |    |                            |
|     |  power button  |-+  |                            |         +------------+
|     |                | |  |                            |         |            |
|     +----------------+ |  |                            |--------+|  host 2    |
|                        |  |                            |         |            |
|                        |  |                            |         +------------+
|                        |  |     +---------------+      |
|     +----------------+ |  |     |    host       |      |         +-------------+
|     |                | |  |     |  selector     |------+         |             |
|     |  reset button  --|--------+   switch      |      |---------+  host 3     |
|     |                |    |     +---------------+      |         |             |
|     +----------------+    |                            |         +-------------+
|                           |                            |
|                           |                            |          +------------+
|                           |                            |          |            |
|     +----------------+    |                            |----------+ host 4     |
|     | serial console +---------------------------------|          |            |
|     |    mux switch  |    |                            |          +------------+
|     |                |    |                            |
|     +----------------+    |                            |           +-----------+
|                           |                            +-----------|   BMC     |
|                           |                                        |           |
+---------------------------+                                        +-----------+

```

There are two additional hardware buttons/switch available in the frontpanel for
yv2.

## 1.Host selector switch (selector switch)

This host selector switch/selector switch has 4 gpio connected as input and
based on the gpio state values the host selector switch value is derived. The
possible switch values can vary from 0 to 10. The specific mapping between gpio
values and host selector values can be stored as part of gpio json config.

yosemiteV2 platform has 1 bmc and 4 hosts and only one power button and reset
button.

For example, when the power button is pressed, The value of the host selector
switch position(1 to 10) decides whether the power button action is needs to be
triggered for bmc or any one of the hosts.

This option is to use same power button and reset button for multiple hosts.

The power/reset button press events are triggered based on the value of host
selector switch position.

### Example :

host selector position value = 0,bmc is mapped to power and reset buttons host
selector position value = 1,host 1 is mapped to power and reset buttons host
selector position value = 2,host 2 is mapped to power and reset buttons host
selector position value = 3,host 3 is mapped to power and reset buttons host
selector position value = 4,host 4 is mapped to power and reset buttons

## 2.Serial console MUX switch

There is also uart console for each host and bmc which is connected via a MUX
switch.

At a time any one of the device (either bmc or any one host) can be accessed via
the console which is based on the UART MUX switch configuration.

Based on the position of the host selector switch, a separate handler interface
can configure the respective serial console device is selected.

## 3.OCP debug card host selector button

An OCP debug card can be connected to yosemitev2 via usb for debugging purpose.

This is a extension which has three buttons : 1.Power button 2.reset button
3.host selector button

Power and reset buttons are mapped to the same gpio of frontpanel so no separate
event handling is required.

The host selector button purpose is same as that of the front panel host
selector switch in yosemitev2 front panel. The debug card host selector button
(push button) has different gpio lines than that of front panel host selector
switch gpios. Whenever the debug host selector button is pressed the host
selector switch position value should be increased up to MaxPosition. If the
host selector switch value is more than MaxPosition then it should be reset to
zero.

# Proposed Design

Three high level changes are required in the phosphor button interface class

1. Add support for adding button interfaces which are monitoring more than one
   gpio event and process with single event handler.

2. Add support to monitor button / switch interface event based on dbus property
   changes (needed for serial console MUX).

3. Each button interface class can be extended with the following overridable
   methods to allow platform specific logic if needed in a derived class.

- init() - This method may have gpio configuration and event handler
  initialization.

- handleEvent() - This method may have custom event handling routines for the
  respective button interface.This should be be called from the main even
  handler function.

## The host selector switch interface

The host selector switch has 4 gpios associated with it. host selector switch
button interface monitors these 4 gpio io events as sd-event based event loop
and a single event handler function is called for all 4 gpio events.

Based on the 4 gpio state values, the host selector switch position value is
derived and stored as a dbus property with name "Position".

### Host selector dbus interface details

1. Position(property) - This is the property which holds the current value of
   the host selector switch based on the GPIO state.

2. MaxPosition(property) - This is the property which keeps the maximum number
   of Position the Position property can hold.(This value is based on the number
   of hosts available in the platform)

## OCP Debug card host selector button interface

A separate interface for debug card host selector button is created.

This button interface monitors the corresponding gpio lines for debug card host
selection button press and release event via sd-event based loop.

### OCP Debug card host selector button dbus interface details :

1. Released(signal) - This is signal is triggered in the ocp debug card event
   handler when the ocp debug card button is pressed and released.

When the event handler is called once the button is released, then the host
selector switch dbus property "Position" is increased by 1. The host selector
switch dbus property value is rollover to zero after Position value exceeds
MaxPosition Value.

This way when power and reset button press events are handled, the Host selector
Position property is referred and based on the Position respective power events
are called.

## serial console MUX interface

This button interface monitors for change in host selector switch Position dbus
property and based on the dbus property value change, corresponding serial
console is selected.mux value can be set as configuring 4 gpio lines dedicated
for the serial console mux.

### Example

0 - bmc serial console 1 - host 1 serial console 2 - host 2 serial console 3 -
host 3 serial console 4 - host 4 serial console

### Change in power button and reset button interface event handler

If selector button instance is available,then the power button and reset button
pressed events must route to appropriate multihost dbus service name

## Impacts

The change impacts are under phosphor buttons repo,the existing power and reset
button handlers are calling dbus calls for single host power events. This calls
should be modified to adapt corresponding multi host power events and the
respective host dbus object is based on host selector position.

## Testing

The proposed design can be tested in a platform in which the multiple hosts are
connected.
