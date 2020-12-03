# System Power Mode and Idle Power Saver Support

Author:
Chris Cain

Primary assignee:
Chris Cain

Other contributors:
Martha Broyles

Created:
12/03/2020

## Problem Description

Adding support for setting a system power mode and idle power saver.

## Background and References

New interface needs to be added that will allow the customer to specify
what power mode the system should be running at.  Also need a new interface
to configure and control Idle Power Saver mode.  The new parameters can be
changed at any time including before the system is powered on.
Each processor contain an embedded 405 processor that is referred to as the
On Chip Controller or OCC.  The OCC provides real time power and thermal
monitoring and control.
When a system is powered on, the OCCs will go to an Active state.
Anytime the OCC state changes to active, the BMC will need to send a
mode change and idle power saver (IPS) settings to the OCC.  It will also
need to send the commands if the user changes the mode or IPS parameters.

OCC Interface Spec: OCC_P10_FW_Interfaces_v1_9.doc
https://ibm.box.com/s/9nxaje8svf7v2x6x7qtas6xgstfr7oxs

## Requirements

Anytime the OCC changes state to active or the customer updates these new
parameters, the code will need to send mode change and/or the idle power
saver settings.
The state change refers to the actual OCC state (not the OCC Active sensor).
(The existing OCC Active sensor just indicates that the BMC can communicate
with the OCCs.)

Current Customer Settable System Power Modes:
 - Disabled (0x01)
 - Static Power Save (0x05)
 - Dynamic Performance (0x0A)
 - Maximum Performance (0x0C)

There are also a couple of modes that are settable for only for lab usage:
 - Static Frequency Point (0x03)
 - Maximum Frequency (0x09)
 - Fixed Frequency Override (0x0B)
Static Frequency Point and Fixed Frequency Override both require an extra
2 byte value for the specific frequency point required.

Customer Settable Idle Power Save Parameters:
 - Enabled / Disabled
 - Enter Delay Time (in seconds)
 - Enter Utilization threshold (percentage)
 - Exit Delay Time (in seconds)
 - Exit Utilization threshold (percentage)

Defaults will need to be configurable by the system owner (via JSON file)

## Proposed Design
New code will be triggered by the following:
 - OCC poll response data showing a new state of Active (0x03)
 - OCC Active sensor is enabled (may be covered in above bullet)
 - Customer updates system power mode interface
 - Customer updates idle power save setting

Code will need to trigger off of OCC state changes.  The OCC state changes
will show up in the OCC poll response data.

When initiated, the new code will send a SET_MODE_AND_STATE command (0x20)
to the OCC and/or a SET_CONFIG_DATA (0x21) command with the Idle Power
Saver parameters.

## Alternatives Considered
None

## Impacts
API impact - need new interface to allow customer to set power mode and
idle power saver settings
Security impact - None
Documentation impact - need to document new parameters
Performance impact - None
Developer impact - None
Upgradability impact - None

## Testing
This will be able to be tested by directly updating the power mode and
idle power saver setting.
