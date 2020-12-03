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

Power management on POWER platforms needs assistance from the BMC for
managing the system power mode and idle power save modes.
We need the ability to set and query the system power mode and also
the ability to control and set idle power save parameters.
This design is only applicable to POWER processors.

## Background and References

Each POWER processor contains an embedded PowerPC 405 processor that is
referred to as the On Chip Controller or OCC.  The OCC provides real time
power and thermal monitoring and control.
When a system is powered on, the OCCs will go to an Active state.
Anytime the OCC state changes to active, the BMC will need to send a
mode change and idle power saver (IPS) settings to the OCC.  It will also
need to send the commands if the user changes the mode or idle power save
parameters.

## Requirements

When a system is booted, the OCC will move to an ACTIVE state.  In the
ACTIVE state, the OCC is managing the processor frequency, power consumption,
and monitoring the systems thermal sensors.  For certain error conditions
it may be necessary to reset the OCC.  When this happens, the OCC will move
out of ACTIVE state.  After recovery, the OCC will be put back into the
ACTIVE state.
Anytime the OCC state changes to ACTIVE or the customer updates these new
parameters at runtime, the BMC will need to send the mode and the idle
power saver settings to the OCC.

The Redfish committee accepted the Power Modes as a new property on the
ComputerSystem schema (https://github.com/DMTF/Redfish/issues/4385).

Current Customer Settable System Power Modes that will be sent to the OCCs:
 - Disabled (0x01)
 - Static Power Save (0x05)
 - Dynamic Performance (0x0A)
 - Maximum Performance (0x0C)

There are also a couple of modes that are settable for only for lab usage:
 - Static Frequency Point (0x03) - requires 2 byte frequency point
 - Maximum Frequency (0x09)
 - Fixed Frequency Override (0x0B) - requires 2 byte frequency

Two of the above modes (Static Frequency Point and Fixed Frequency Override)
need an additional 2 byte parameter which defines the specific frequency
that the processors should be set to in that mode.

The Redfish committee is still investigating the addition of Idle Power
Save parameters (https://github.com/DMTF/Redfish/issues/4386)
If not approved they would need be created as OEM parameters instead.

Customer Settable Idle Power Save Parameters:
 - Enabled / Disabled
 - Enter Delay Time (in seconds)
 - Enter Utilization threshold (percentage)
 - Exit Delay Time (in seconds)
 - Exit Utilization threshold (percentage)

Defaults will need to be configurable by the system owner (via JSON file)

## Proposed Design
The new code would be part of the openpower-occ-control repository.
New code will be triggered by the following:
 - OCC poll response data showing a new state of Active (0x03)
 - OCC Active sensor is enabled (may be covered in above bullet)
 - Customer updates system power mode user interface or Redfish interface
 - Customer updates idle power save setting or Redfish interface

Code will need to trigger off of OCC state changes.  The BMC currently
sends a POLL command to the OCC periodically (every second).  The OCC state
will show up in the OCC poll response data.

When initiated, the new code will send a SET_MODE_AND_STATE command (0x20)
to the OCC and/or a SET_CONFIG_DATA (0x21) command with the Idle Power
Saver parameters.
These commands are defined in the OCC Interface Spec:
https://github.com/open-power/docs/blob/master/occ/OCC_P9_FW_Interfaces.pdf

## Alternatives Considered
- Using hardcoded power mode and Idle Power Save parameters (no flexibility
to control system power usage)

## Impacts
New interfaces that were described in the requirements section will be
implemented.  Parameters should be able to be set via user interface or
via Redfish.
API impact - Add Redfish support fo new parameters as well as new user
interface to allow customer to set power mode and idle power saver settings
Security impact - update of these parameters should be able to be restricted
to specific users/groups (may not want any user updating these parameters)
Documentation impact - need to document new parameters
Performance impact - minimal, new code will only execute on OCC state change
which should normally happen once at boot time or when user changes parameters.
The new code is only sending 2 additional commands which should complete
within a few seconds.
Developer impact - code to be written by OCC/HTMGT team with guidance from
OpenBMC power managmenet team
Upgradability impact - None

## Testing
This will be able to be tested by directly updating the power mode and
idle power saver setting.
This testing will be automated
