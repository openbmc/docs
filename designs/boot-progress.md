____
# System Boot Progress on OpenBMC

Author: Andrew Geissler (geissonator)

Primary assignee: Andrew Geissler (geissonator)

Other contributors: None

Created: May 27, 2020

## Problem Description
The boot progress of an OpenBMC managed system is complex. There are a mix
of phosphor D-Bus properties, IPMI sensors, PLDM sensors, and Redfish
properties to represent it. The goal of this design document is to highlight
these different entities and provide a base for system implementors on what
to implement.

## Background and References
[phosphor-state-manager][1] implements D-Bus properties which track the state
of the BMC, Chassis, and Host. This design is primarily focused on the status
of the Host since it is the most complex.

[HostState][2] provides basic `Off`/`Running` type information along
with an error state (`Quiesced`) and debug state (`DiagnosticMode`). The focus
of this document is the `Running` state. This simply indicates whether something
is running on the host processors. The goal with this design document is to
define more detailed information on what is running and what has it
accomplished.

phosphor-state-manager implements some other D-Bus properties that represent
the host boot progress:

- [xyz.openbmc_project.State.Boot.Progress][3]
- [xyz.openbmc_project.State.OperatingSystem.Status][4]

These two D-Bus properties are very IPMI-centric. They were defined based on
two IPMI sensors which are set by the host firmware as it boots the system.

PLDM also has a boot progress sensor. Search for "Boot Progress" in this
[doc][5]. A subset of this maps fairly well to the IPMI sensors above. This
PLDM sensor is not implemented yet in OpenBMC code.

PLDM currently has a [story][6] to implement host progress codes, but it's not
clear if this is actually implementing the "Boot Progress" sensor or something
else related to BIOS boot codes.

Redfish represents system state in a variety of ways. The BMC, Chassis,
and System all implement a [Status][7] object. This provides a list of generic
`State` options which are applicable to Redfish objects. OpenBMC has the
following mapping for phosphor-state-manager to the Redfish System
[Status][State]:
- `xyz.openbmc_project.State.Host.HostState.Running` : `Enabled`
- `xyz.openbmc_project.State.Host.HostState.Quiesced` : `Quiesced`
- `xyz.openbmc_project.State.Host.HostState.DiagnosticMode` : `InTest`

There is a proposal working its way through DMTF/Redfish to create a new
`BootProgress` property which will be associated with the System. This new
property would track the following:

- None
- PrimaryProcessorInitialization
- ComputerSystemInitialization
- MemoryInitialization
- SecondaryProcessorInitialization
- PCIResourceConfig
- SystemFirmwareComplete
- OSStart
- OSBooting
- Completed

There is still a lot of active [discussion][8] surrounding this so don't take
anything as written in stone yet. It could be that a time stamp is associated
with this new `BootProgress` property so a user could determine if the boot
is hung or looping.

In the end, the goal is to be able to map either the IPMI or PLDM boot sensors
into this new Redfish property.

## Requirements
- Enhance the existing [BootProgress][3] D-Bus property to cover all supported
  PLDM boot progress codes
- Ensure the `BootProgress` property is updated appropriately in both IPMI
  and PLDM based stacks
  - It is the responsibility of the IPMI or PLDM implementation to update
    the property on D-Bus.
- Ensure the new Redfish `BootProgress` interface has the appropriate mappings
  to the `BootProgress` D-Bus property

Different OpenBMC systems could support different boot progress codes, and
support them in different orders. This document does not try to set any
requirements on this other then that they must all be defined within
the `BootProgress` D-Bus property.

## Proposed Design
[BootProgress][3] will be enhanced to support a superset of what is provided
by IPMI and PLDM.

IPMI code already sets the `BootProgress` D-Bus property based on a config
file. For example, look for `fw_boot_sensor` in this [file][9] for an example
of how a particular IPMI sensor is mapped to this D-Bus property.

PLDM software will do something similar.

bmcweb will then use this `BootProgress` D-Bus sensor to respond appropriately
to external interfaces, such as Redfish.

## Alternatives Considered
A lot of system BIOS's provided some form of a detailed boot progress codes.
UEFI has POST codes, POWER has istep progress codes. If more fine grained
details were needed, we could look into using these. Currently though the
need is just high level boot progress.

## Impacts
Each system will need to document which `BootProgress` codes they support
and the expected order of them when a system is booting.

## Testing
Ensure an IPMI and PLDM based system boot and update the `BootProgress` D-Bus
property as expected.


[1]: https://github.com/openbmc/phosphor-state-manager#state-tracking-and-control
[2]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/State/Host.interface.yaml
[3]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/State/Boot/Progress.interface.yaml
[4]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/State/OperatingSystem/Status.interface.yaml
[5]: https://github.com/ibm-openbmc/dev/issues/1920
[6]: https://github.com/ibm-openbmc/dev/issues/1920
[7]: http://redfish.dmtf.org/schemas/v1/Resource.json#/definitions/Status
[8]: https://github.com/DMTF/Redfish/issues/3834
[9]: https://github.com/openbmc/meta-ibm/blob/master/recipes-phosphor/configuration/acx22-yaml-config/acx22-ipmi-sensors-mrw.yaml
