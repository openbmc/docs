# LED Management

Author:
  Jinu Joy Thomas (jthomas7)

Other contributors:
  Santosh Puranik (sanpuran)

Created:
  September 9, 2022

## Problem Description
IBM has an upcoming system architecture that consists of more than 1 sled that
can be SMP cabled together into 1 or more composed systems.  There will be a
coordinator application, or group of applications, that may float between sleds
that is responsible for doing operations at a composed system level.  It will
communicate to all non-coordinator entities using Redfish, including
non-coordinator applications on the same BMC.


On such composed systems with multiple BMCs, there is a set of LEDs that will
apply to the composed system as a whole.
This LED types includes physical LEDs such as:
- Power LED
- Identify LED
- Fault LED

These LEDS though are physically available within all the SLEDs individually,
we may need to turn them ON or OFF together based on the number of SLEDs on a
composed system. We will also need to read the states of the LED from all the
SLEDs under a composed system to be given to the end user as needed.

If an end user identifies a composed system , then we need to relay this
information to all the BMCs on the SLEDs which are a part of that composed system.

## Background and References
With the current design the led management is maintained by the single BMC
associated with a system. The LEDs are controlled externally via the BMC GUI
and/or Redfish interface for that system.

The LEDs are also controlled by the hypervisor by sending commands via the PLDM
interface to the led manager.


## Requirements
### SCA - System Coordinator
- The SCA must have the ability to manage multiple composed systems.
- The SCA must provide an interface for the end user to set/read LED state
  data (Redfish and any other supported interface).
- The SCA must provide a notification message when any changes are made to the
  LED state data that will be used by all BMCs in the composed system.
### BMC
- Each BMC must be able to set the LED state data from an SCA.
- Each BMC must be able to return the LED state data to an SCA.
- Each BMC will maintain a saved Group state data of its own.


## Proposed Design
The SCA will have the ability to divert the required calls to the corresponding
satellite BMC for which it is intended.
The SCA will have the ability to inverse multiplex a single LED command for a
composed system to the various satellite BMCs that are part of the composed
system.
The SCA will have the ability to multiplex the state data of each LED of the
satellite BMC in the composed system to display a single state for the
composed system to the end user.

The current LED manager working in the BMC will remain to do so.

## Alternatives Considered
One alternative is to make the user to have knowledge of the composed systems
and identify all of them instead of a single point.

### SCA Detected Problem - Open issues
 - what needs to be done when we see that all states read from the satellite
   BMCs are not same.
 - what needs to be done if we encounter one or more faults in an inverse
   multiplexed flow?


## Other changes
The association of LEDs to inventory will be changed to be used via Entity Manager.
more details on it in the inventory document.

## Impacts
- Limiting the changes to SCA side.
- Using the existing Led manager.
- Having a single interface (via SCA) for the composed system.
- Need to fix the current association, due to move to Entity Manager from PIM.

### Organizational
- Does this repository require a new repository?
  No
- Which repositories are expected to be modified to execute this design?
  phosphor-led-manager, [sca-repo TBD]


## Testing
TBD based on SCA.

