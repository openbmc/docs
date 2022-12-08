# System Power Mode and Idle Power Saver Support

Author: Chris Cain

Other contributors: Martha Broyles

Created: 12/03/2020

## Problem Description

Power management on POWER platforms needs assistance from the BMC for managing
the system power mode and idle power save modes. We need the ability to set and
query the system power mode and also the ability to control and set idle power
save parameters. This design is only applicable to POWER processors.

## Background and References

Each POWER processor contains an embedded PowerPC 405 processor that is referred
to as the On Chip Controller or OCC. The OCC provides real time power and
thermal monitoring and control. When a system is powered on, the OCCs will go to
an Active state. Anytime the OCC state changes to active, the BMC will need to
send a mode change and idle power saver (IPS) settings to the OCC. It will also
need to send the commands if the user changes the mode or idle power save
parameters.

## Requirements

When a system is booted, the OCC will move to an ACTIVE state. In the ACTIVE
state, the OCC is managing the processor frequency, power consumption, and
monitoring the systems thermal sensors. For certain error conditions it may be
necessary to reset the OCC. When this happens, the OCC will move out of ACTIVE
state. After recovery, the OCC will be put back into the ACTIVE state. Anytime
the OCC state changes to ACTIVE or the customer updates these new parameters at
runtime, the BMC will need to send the mode and the idle power saver settings to
the OCC.

PowerMode was added to version 2021.1 Redfish Schema Supplement:
https://www.dmtf.org/sites/default/files/standards/documents/DSP0268_2021.1.pdf

Current Customer Settable System Power Modes that will be sent to the OCCs:

- Static
- Power Saver
- Maximum Performance

A proposal for adding Idle Power Saving parameters was submitted to the Redfish
committee and will be used if/when approved.

Customer Settable Idle Power Save Parameters:

- Enabled / Disabled
- Enter Delay Time (in seconds)
- Enter Utilization threshold (percentage)
- Exit Delay Time (in seconds)
- Exit Utilization threshold (percentage)

Defaults will need to be configurable by the system owner (via JSON file)

## Proposed Design

The new code would be part of the openpower-occ-control repository. New code
will be triggered by the following:

- OCC poll response data showing a new state of Active (0x03)
- OCC Active sensor is enabled (may be covered in above bullet)
- Customer updates system power mode user interface or Redfish interface
- Customer updates idle power save setting or Redfish interface

Code will need to trigger off of OCC state changes. The kernel currently sends a
POLL command to the OCC periodically (every second). The OCC state is available
in the OCC poll response data.

When initiated, the new code will send a SET_MODE_AND_STATE command (0x20) to
the OCC and a SET_CONFIG_DATA (0x21) command with the Idle Power Saver
parameters. These commands are defined in the OCC Interface Spec:
https://github.com/open-power/docs/blob/master/occ/OCC_P9_FW_Interfaces.pdf

Default values will also be defined for Power Mode and Idle Power Saver
parameters for the system. If the customer has not yet set any of these
parameters, these default values will be used. If/when the customer does set any
of these, that new customer parameter will become current and the default value
will no longer be used.

The customer requested PowerMode and Idle Power Saver parameters will be stored
as D-Bus object in the phosphor-dbus-interfaces repository:
xyz/openbmc_project/Control/Power/Mode.interface.yaml Once set, these values
will be retained across power cycles, AC loss, code updates, etc.

## Alternatives Considered

- Using hardcoded power mode and Idle Power Save parameters (no flexibility to
  control system power usage)

## Impacts

New interfaces that were described in the requirements section will be
implemented. Parameters should be able to be set via user interface or via
Redfish. API impact - Add Redfish support for new parameters as well as new user
interface to allow customer to set power mode and idle power saver settings
Security impact - update of these parameters should be able to be restricted to
specific users/groups (may not want any user updating these parameters)
Documentation impact - need to document new parameters Performance impact -
minimal, new code will only execute on OCC state change which should normally
happen once at boot time or when user changes parameters. The new code is only
sending 2 additional commands which should complete within a few seconds.
Developer impact - code to be written by OCC team with guidance from OpenBMC
power management team Upgradability impact - None

## Testing

This will be able to be tested by directly updating the power mode and idle
power saver setting. This testing will be automated
