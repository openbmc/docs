# System Boot Progress on OpenBMC

Author: Andrew Geissler (geissonator)

Other contributors: None

Created: May 27, 2020

## Problem Description

The boot progress of an OpenBMC managed system is complex. There are a mix of
phosphor D-Bus properties, IPMI sensors, PLDM sensors, and Redfish properties to
represent it. The goal of this design document is to highlight these different
entities and provide a base for system implementors on what to implement.

## Background and References

[phosphor-state-manager][1] implements D-Bus properties which track the state of
the BMC, Chassis, and Host. This design is primarily focused on the status of
the Host since it is the most complex. Specifically the goal is to provide
detailed information on what the host is doing during a boot.

[HostState][2] provides basic `Off`/`Running` type information along with an
error state (`Quiesced`) and debug state (`DiagnosticMode`). The focus of this
document is the `Running` state. This simply indicates whether something is
running on the host processors.

phosphor-state-manager implements some other D-Bus properties that represent the
host boot progress:

- [xyz.openbmc_project.State.Boot.Progress][3]
- [xyz.openbmc_project.State.OperatingSystem.Status][4]

These two D-Bus properties are very IPMI-centric. They were defined based on two
IPMI sensors which are set by the host firmware as it boots the system.

PLDM also has a boot progress sensor. Search for "Boot Progress" in this
[doc][5]. A subset of this maps fairly well to the IPMI sensors above. This PLDM
sensor is not implemented yet in OpenBMC code.

Redfish represents system state in a variety of ways. The BMC, Chassis, and
System all implement a [Status][6] object. This provides a list of generic
`State` options which are applicable to Redfish objects. OpenBMC has the
following mapping for phosphor-state-manager to the Redfish System
[Status][state]:

- `xyz.openbmc_project.State.Host.HostState.Running` : `Enabled`
- `xyz.openbmc_project.State.Host.HostState.Quiesced` : `Quiesced`
- `xyz.openbmc_project.State.Host.HostState.DiagnosticMode` : `InTest`

As of [ComputerSystem.v1_13_0][7], the `BootProgress` object is officially in
the Redfish ComputerSystem schema.

To summarize, the `LastState` property under this new `BootProgress` object
tracks the following boot states of the system:

- None
- PrimaryProcessorInitializationStarted
- BusInitializationStarted
- MemoryInitializationStarted
- SecondaryProcessorInitializationStarted
- PCIResourceConfigStarted
- SystemHardwareInitializationComplete
- OSBootStarted
- OSRunning
- OEM

There is also a `LastStateTime` associated with this new BootProgress object
which should be updated with the date and time the `LastState` was last updated.

In the end, the goal is to be able to map either the IPMI or PLDM boot sensors
into all or some of the values in this new Redfish property.

Note that this design does not include multi-host computer system concepts but
it also does not preclude them. The `BootProgress` D-Bus property is associated
with a `/xyz/openbmc_project/state/host<X>` instance where <X> is the host
instance. Similarly, the Redfish system object is also an instance based object.

## Requirements

- Enhance the existing [BootProgress][3] D-Bus property to cover all supported
  PLDM boot progress codes
- Create a new `BootProgressLastUpdate` D-Bus property that will hold the date
  and time of the last update to `BootProgress`
- Ensure the `BootProgress` and `BootProgressLastUpdate` properties are updated
  appropriately in both IPMI and PLDM based stacks
  - It is the responsibility of the IPMI or PLDM implementation to update the
    `BootProgress` property on D-Bus
  - It is the responsibility the phosphor-state-manager to update the
    `BootProgressLastUpdate` property on D-Bus when it sees `BootProgress`
    updated
- Ensure the new Redfish `LastState` and `LastStateTime` properties have the
  appropriate mappings to the `BootProgress` and `BootProgressLastUpdate D-Bus
  properties

Different OpenBMC systems could support different boot progress codes, and
support them in different orders. This document does not try to set any
requirements on this other than that they must all be defined within the
`BootProgress` D-Bus property and the ones which have a mapping to the Redfish
BootProgress object will be shown via the Redfish interface.

## Proposed Design

[BootProgress][3] will be enhanced to support a superset of what is provided by
IPMI and PLDM.

IPMI code already sets the `BootProgress` D-Bus property based on a config file.
Look for `fw_boot_sensor` in this [file][8] for an example of how a particular
IPMI sensor is mapped to this D-Bus property.

PLDM software will do something similar.

bmcweb will then use this `BootProgress` D-Bus interface to respond
appropriately to Redfish requests.

## Alternatives Considered

A lot of system BIOS's provided some form of a detailed boot progress codes.
UEFI has POST codes, POWER has istep progress codes. If more fine grained
details were needed, we could look into using these. Currently though the need
is just high level boot progress. Note that these POST/istep codes could be
mapped to the appropriate BootProgress value if desired (or for cases where the
host does not support an IPMI or PLDM stack that has BootProgress in it).

## Impacts

Each system will need to document which `BootProgress` codes they support and
the expected order of them when a system is booting.

## Testing

Ensure an IPMI and PLDM based system boot and update the `BootProgress` D-Bus
property as expected.

[1]:
  https://github.com/openbmc/phosphor-state-manager#state-tracking-and-control
[2]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/Host.interface.yaml
[3]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/Boot/Progress.interface.yaml
[4]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/OperatingSystem/Status.interface.yaml
[5]:
  https://www.dmtf.org/sites/default/files/standards/documents/DSP0249_1.0.0.pdf
[6]: http://redfish.dmtf.org/schemas/v1/Resource.json#/definitions/Status
[7]: https://redfish.dmtf.org/schemas/v1/ComputerSystem.v1_13_0.json
[8]:
  https://github.com/openbmc/meta-ibm/blob/master/recipes-phosphor/configuration/acx22-yaml-config/acx22-ipmi-sensors-mrw.yaml
