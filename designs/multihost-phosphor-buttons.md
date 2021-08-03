# multihost front panel phosphor buttons interface

Author:
  Velumani T(velu),  [velumanit@hcl](mailto:velumanit@hcl.com)
  Naveen Moses S(naveen.moses), [naveen.mosess@hcl.com](mailto:naveen.mosess@hcl.com)
Primary assignee:

Other contributors:

Created:
  August 3, 2021

## Problem Description
Phosphor buttons module has currently option to monitor gpio events of power
and reset buttons and trigger power event handlers.

phosphor-buttons currently only support push type buttons.support for
different form factor inputs such as switches/MUX other than push type
buttons are needed for some hardwares. It will be helpful if we could
add option to support additional input types in phosphor button interfaces
and event handling.

Currently handler events are only based on monitoring gpio events
as input (power and reset).There may be cases where whe need to create
button interface which monitors non gpio events
and triggers button actions for example events based on dbus property changes.

## Requirements
This feature is needed to support additonal phosphor button interfaces
corresponding to platform specific hardware buttons/MUX/Switches which are
available in the front panel apart from existing power and reset button
interfaces.

## Background and References
The front panel of bmc has buttons like power button, reset button
in general the code for monitoring these buttons and triggering actions are
supported.

'''
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
'''


platform specific front panel may contain additional hardware buttons or
mux switch

example for a multihost platform yosemite-V2 it has host selector switch mux
as additional button in front panel.

'''
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

'''

There are two additional hardware buttons/switch available in the
frontpanel for yv2.

**1.Host selector switch (selector switch) :**
This host selector switch/selector switch has 4 gpio connected as input and
based on the gpio state values the host selector switch value is derived.
The possible switch values can vary from 0 to 10.
The specific mapping between gpio values and host selector values
can be stored as part of gpio json config.

yosemiteV2 platform has 1 bmc and 4 hosts and only one power button and
reset button.

For example, when the power button is pressed,
The value of the host selector switch position(1 to 10) decides whether the
power button action is needs to be triggered for bmc or any one of the
hosts.

This option is to use same power button and reset button for multiple hosts.

The power/reset button press events are triggered based on the value of
host selector switch position.

**example :**
host selector position value = 0,bmc is mapped to power and reset buttons
host selector position value = 1,host 1 is mapped to power and reset buttons
host selector position value = 2,host 2 is mapped to power and reset buttons
host selector position value = 3,host 3 is mapped to power and reset buttons
host selector position value = 4,host 4 is mapped to power and reset buttons

**2.Serial console MUX switch:**

There is also uart console for each host and bmc which is connected via a
MUX switch.

At a time any one of the device (either bmc or any one host) can be accessed
via the console which is based on the UART MUX switch configuration.

Based on the position of the host selector switch, a separate handler interface
can configure the respective serial console device is selected.

**3.Debug card extenstion:**

An OCP debug card can be connected to yosemitev2 via usb for debugging purpose.

This is a extension which has three buttons :
1.Power button
2.reset button
3.host selector button

Power and reset buttons are mapped to the same gpio of frontpanel so no
separate event handling is required.

The host selector button purpose is same as that of the front panel
host selector switch in yosemitev2 front panel. The debug card host selector button
(push button) has different gpio lines than that of front panel host selector
switch gpios. Whenever the debug host selector button is pressed the host
selector switch position value should be increased up to 10.if the host
selector switch value is more than 10 then it should be reset to zero.

## Proposed Design
Two high level changes are required in the phosphor button interface class

1.Add support for adding button interfaces which are monitoring more than
 one gpio event and process with single event handler.

2.Add support to monitor button / switch interface event  based on dbus
property changes (needed for serial console MUX).

There should be option enable/disable the supported additional buttons as a
platform specific feature by adding a machine layer config.

**The host selector switch button interface :**

The host selector switch has 4 gpios assosiated with it. host selector
switch button interface monitors these 4 gpio io events as sd-event
based event loop and a single event handler function is called for
all 4 gpio events.

 Based on the 4 gpio state values, the host selector switch position value
 is dervied and stored as a dbus property.

**serial console MUX button interface :**
This button interface monitors for change in host selector switch position dbus
property and based on the dbus property value change, corresponding serial
console is selected.mux value can be  set as configuring 4 gpio lines dedicated
for the serial console mux.

0 - bmc serial console
1 - host 1 serial console
2 - host 2 serial console
3 - host 3 serial console
4 - host 4 serial console

**Change in power button and reset button interface event handler :**
If selector button instance is available,then the power button and reset
button pressed events must route to appropriate multihost dbus service name

**Debug card  host selector switch button interface :**
A separate interface for debug card host selector switch is created.
This  monitors its corresponding gpio lines for io event change
via sd-event based loop. when the event handler callback is called
then the host selector position dbus property value is increased by 1.
The host selector switch dbus property value is rollover to zero after 10.

## Impacts
 The change impacts are under phosphor buttons repo,the existing power and
 reset button handlers are calling dbus calls for single host power events.
 This calls should be modified to adapt corresponding multi host power events
 and the respective host dbus object is based on host selector position.

## Testing
The proposed design can be tested in a platform in which the multiple hosts
are connected.