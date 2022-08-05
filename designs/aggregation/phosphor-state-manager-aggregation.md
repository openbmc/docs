# Impact of Aggregation on phosphor-state-manager

Author: Author: Andrew Geissler (geissonator)

Other contributors:

Created: August 4th, 2022

## Problem Description

The impacts of [BMC aggregation][aggr-design] on OpenBMC software will have
significant impacts to some areas of the code base. The goal of this document is
to highlight the challenges (and solutions) that phosphor-state-manager (PSM)
will have. Note that the linked design document has been abandoned due to
inactivity but still covers a lot of valid design points. There is also a bmcweb
document on [aggregation here][bmcweb-aggr].

Please review the referenced aggregation designs for a general overview of the
software design and terms used by this document.

Throughout this doc, the following terms will be utilized:

- Composed System: A combination of compute nodes representing a single system
- Sled: The compute node within the composed system which has CPU, memory, and a
  BMC
- Aggregator / Rack Manager: The OpenBMC device which responds to external
  Redfish clients and aggregates information from sleds within the composed
  system
- System: All of the compute resources available to create composed systems

Issues with the aggregator failing and potential fail over scenarios are outside
the scope of this document as there will need to be some larger system
architecture in this area.

How composed systems are created within OpenBMC is also outside the scope of
this document. The design for that Redfish support is being tracked out in this
[doc][bmcweb-comp]. This document will assume that composed systemd(s) have been
created and PSM is responsible for tracking their state and supporting the
existing BMC, Chasiss, and Host operations.

Here's a quick summary of the PSM functions which will be affected by BMC
aggregation:

### Overall owner of BMC, Chassis, and Host states

[PSM][psm-overview] provides the following D-Bus objects:

- /xyz/openbmc_project/state/bmc`X`
- /xyz/openbmc_project/state/chassis`X`
- /xyz/openbmc_project/state/host`X`
- /xyz/openbmc_project/state/hypervisor`X`

Where `X` represents the instance of the object. The instance support has
allowed users of OpenBMC lots of flexibility with their designs. The could have
one BMC, multiple chassis, and multiple hosts. Or multiple BMC's, a single
chassis, and multiple hosts.

How does state management software present a consistent state to external
clients when the overall system state is a combination of states from each
satellite BMC? What happens when a sled has a BMC or Host in the Quiesced state
but other sleds are working?

What about when a sled needs to trigger a composed system wide power off or
reboot of the system? How could a redfish client (i.e. the sled) send back such
a request.

### Power Restore

The [Power Restore][power-restore] logic is run when a BMC boots to BMC Ready.
It has logic to automatically power the system back on depending on user
settings.

Will need to determine if the power outage was just for the sled the BMC is in,
or if it affected other sleds within the system. The system design may or may
not be able to handle a sled losing power. Depending on what the system can
support, proper power restore logic may need to be coordinated by the
aggregator.

### BMC reset when host is booted

The [BMC reset when host is booted][reset-booted] design handles reconnecting
with the host firmware after a BMC reboot. For a period of time, the sled being
rebooted will be unavailable. Some thought will be need to be put into what type
of "system state" is returned to clients in this scenario. Similar concerns
occur when a sled just becomes unavailable for whatever reason.

### Host boot progress

The [Boot Progress][boot-progress] issues are similar to the PSM state
questions, how is host BootProgress represented when there are multiple host
firmwares booting across the sleds within the same system? BootProgress is under
/redfish/v1/Systems/system/BootProgress which means it is an overall system
representation.

### Hypervisor state tracking

Within a system there is only one hypervisor running. Does only the aggregator
represent its state and allow control or does each sled need to have a
representation of the hypervisor state?

### Scheduled host transitions

The scheduled host transition function provides interfaces for users to schedule
a PSM operation in the future. For example, someone could schedule a power off
of their system over night and a power on in the morning.

This function would be on a system basis, so only implemented/supported by the
aggregator? What's the impact if all BMC sleds were scheduled to power on? Could
all sleds run it but the aggregator ensure they all have the same schedule?
Schedule is based on BMC time so all BMC's will need to be time synchronized.

### obmcutil

`obmcutil` has quite a few different commands within it. In general the
expectation is that the commands within it operate on a system basis. So if a
user does a `obmcutil poweron` the expectation is that all sleds within that
system will be powered on and booted. But other commands like `showlog` or
`deletelogs` could be sled specific.

### Error Paths

When a boot fails, there are usually automatic reboots that occur. In a multi
sled system, the boot will need to be restarted on all sleds when one fails (or
the failed sled will need to be removed from the system). How does the
aggregator know a boot failed, how does it properly halt the working sleds and
coordinate a reboot. The host reboot count of a working sled should not be
affected by a reboot due to another sleds boot failure. What happens when the
issue is the cable between the sleds? How does a sled get its reboot count reset
when a sled is removed due to a bad cable?

[aggr-design]:
  https://gerrit.openbmc.org/c/openbmc/docs/+/44547/2/designs/redfish-aggregation.md
[bmcweb-aggr]: https://github.com/openbmc/bmcweb/blob/master/AGGREGATION.md
[bmcweb-comp]: https://gerrit.openbmc.org/c/openbmc/docs/+/57985
[psm-overview]:
  https://github.com/openbmc/docs/blob/master/designs/state-management-and-external-interfaces.md
[power-restore]:
  https://github.com/openbmc/docs/blob/master/designs/power-recovery.md
[reset-booted]:
  https://github.com/openbmc/docs/blob/master/designs/bmc-reset-with-host-up.md
[boot-progress]:
  https://github.com/openbmc/docs/blob/master/designs/boot-progress.md

## Background and References

None other than what was put in previous section.

## Requirements

- Provide an aggregated state of all supported objects as required by the
  Redfish specification
  - Some states, such as host will need to be aggregated, but other such as
    chassis and BMC have their own individual states within Redfish
  - In general the "least" state of the aggregate will be returned
  - This includes the host `BootProgress`
- Provide system level control
  (`redfish/v1/Systems/system/Actions/ComputerSystem.Reset`) support for all
  sleds within the aggregate (distribute single command from client to all sleds
  in composed system)
- Provide the ability for sleds to cause composed system operations
- Provide automated recovery of a composed system when bmc, chassis, or host
  fails within the composed system
- Ensure power restore and scheduled host transition logic is only run on a
  composed system basis
- No host POST code aggregation will be done
  - Host firmware can include a sled identifier in their POST code if if
    differentiation is required
- No arbitrary synchronization points needed in host power on phase
  - Must support a synch point after chassis power on to all sleds (i.e. SMP
    training)
- Need to support a peer aggregator (reject request if power on/off issued
  already, invalid operations during code updates)
  - Not a lock but just a rejection of an operation request that is redundant
- Update `obmcutil` to support an individual sled and an aggregate system

## Proposed Design

Move the system view and control of state into the aggregator. The aggregator
will initially just be the bmcweb instance running on the system which has been
selected as the aggregator. Some system configuration will have a dedicated Rack
Manager for this purpose. It is still to-be-determined on whether this function
will be broken out into a separate application within OpenBMC.

This new function will aggregate state across all sleds within a composed system
and ensure commands issued against the composed system are issued to all of the
corresponding sleds. It will monitor for Redfish events in each sled and
propagate required state changes to a composed system d-bus model. It will
handle failures of sleds and recovery of the composed system. It will interact
with the local PSM object to gather and control state on its sled, as well as
using Redfish to communicate with the other sleds and their corresponding PSM
instances. The PSM applications on the sleds will continue to create objects
with instance 0 and will only have knowledge of, and be responsible for the sled
it is running in.

The standard chassis and host systemd targets will be started/stopped on both
the aggregator and all sleds within the composed system as required.

The aggregator will provide composed system d-bus objects:

- /xyz/openbmc_project/state/ComposedSystemBmc`X`
- /xyz/openbmc_project/state/ComposedSystemChassis`X`
- /xyz/openbmc_project/state/ComposedSystemHost`X`
- /xyz/openbmc_project/state/ComposedSystemHypervisor`X`

These d-bus objects can be operated upon by applications in the aggregator to do
global composed system operations. Depending on where the aggregation function
resides, these objects may also be used by bmcweb. These d-bus objects will
provide all of the same properties and operations provided by their
corresponding non-composed system objects. The returned state will be the lowest
state of the object in the composed system.

The aggregator will also provide PSM d-bus objects for each Sled in the system:

- /xyz/openbmc_project/state/bmc`X`
- /xyz/openbmc_project/state/chassis`X`
- /xyz/openbmc_project/state/host`X`

Instance 0 will be for the aggregator and each instance after that will
represent a sled in the system. D-Bus associations will be utilized to map the
PSM instances to the corresponding sled. These objects properties will be
updated as events come in from the corresponding sleds indicating state changes.

### Host power on flow

- External request to power on via `/redfish/v1/Systems/ComposedSystem0`
- bmcweb sets `RequestedHostTransition` on d-bus via
  `/xyz/openbmc_project/state/ComposedSystemHost0` object
- Aggregation function implementing d-bus property processes request
  - Issue requested `RequestedHostTransition` to local
    `/xyz/openbmc_project/state/host0` if present
  - Issue redfish POST to each sled within the composed system to power on
- Each sled in composed system accepts POST to `/redfish/v1/Systems/system` and
  follows existing flow of finding `/xyz/openbmc_project/state/host0` on d-bus
  and issuing `RequestedHostTransition` power on request
  - Each sled begins its chassis power on and host boot targets, updating their
    local d-bus objects (`/xyz/openbmc_project/state/chassis0`, `host0`)
  - The sleds power on each chassis and start host services like hw-diagnostics,
    croserver, and pldmd
- aggregator is subscribed to each sleds CurrentPowerState, CurrentHostState and
  BootProgress changes over Redfish
  - As change events come in, the appropriate properties in
    `/xyz/openbmc_project/state/hostX` are updated
  - When a d-bus properties in `ComposedSystem` is read, it will aggregate the
    data from all of the `/xyz/openbmc_project/state/hostX` within that composed
    system to return the "least" answer
- A systemd service within the host start target on the aggregator will wait for
  all sleds within the composed system to have their chassis power on and host
  started targets complete
- The aggregator will then do any required operations, such as training the SMP
  between the sleds and starting the host firmware
- Any host firmware requested reboots will come through a sled and result in a
  reboot of the composed system

### Hypervisor start flow

- External request to power on via `/redfish/v1/Systems/ComposedHypervisor0`
- Aggregator sends
  `/redfish/v1/Systems/hypervisor/Actions/ComputerSystem.Reset -d '{"ResetType": "On"}'`
  to each sled within the composed system
- This results in the sleds setting `/xyz/openbmc_project/state/hypervisor0` to
  `On` on which triggers the existing flow of pldmd detecting and setting the
  appropriate effecter for the hypervisor
  - The hypervisor will need to handle multiple sleds sending the same command
- The hypervisor sets BootProgress once complete which propagates over to the
  Aggregator and can then be queried by external clients

### Host power off flow

- External request to power off via `/redfish/v1/Systems/ComposedSystem0`
- bmcweb sets `RequestedHostTransition` on d-bus via
  `/xyz/openbmc_project/state/ComposedSystemHost0` object
- Aggregation function implementing d-bus property processes request
  - Issue requested `RequestedHostTransition` to local
    `/xyz/openbmc_project/state/host0` if present
- Aggregator sends
  `/redfish/v1/Systems/system/Actions/ComputerSystem.Reset -d '{"ResetType": "Off"}'`
  to each sled within the composed system
  - The hypervisor may only follow the soft power off flow with one sled
  - pldmd function in each sled will need to utilize etcd to know if soft power
    off has completed in a different node and complete the service
- The power off of the chassis's in each sled can be held until an etcd variable
  indicates to continue (optional feature)
  - This allows any global composed system functions to be run in between the
    host being shut down and the chassis power removed (stopping host
    instructions, pre-poweroff functions, etc)
- The aggregator monitors etcd to know when the soft power off has completed
  within the composed system and then runs any needed pre-poweroff functions
- The aggregator sets an etcd variable indicating sleds can power off their
  chassis's
- Sleds complete chassis power off, Redfish events from sleds indicating power
  off complete are propagated to Aggregator which updates corresponding d-bus
  objects

### Sled initiated fast power off flow

Automated sled-specific events like a thermal error which requires an immediate
power off would have the following high-level flow:

- The sled with the error will immediately power off
- The aggregator will detect the sled powered off and why
- The aggregator will update it's corresponding composed system d-bus objects
- The aggregator will issue a power off to the rest of the sleds in the composed
  system
- The aggregator will execute recovery (either report terminating error and keep
  composed system in failed state or remove the bad sled from the composed
  system and reboot)

### Other Miscellaneous Items

The power restore function and scheduled host transitions will only be run
within the aggregator. As these features can be enabled inband, and sent to any
arbitrary sled within the composed system, etcd will be utilized to ensure there
is a single composed system policy with which the aggregator will be responsible
for executing.

obmcutil will add an option to target an entire composed system, or just the
sled it is executed on. The default will be composed system 0 when it is
present, otherwise just the local PSM objects.

## Open Questions

- Should we have a different object path for remote sleds? Should we include the
  composed system instance somewhere in the path?
  - This would mean a dynamic path as sleds can be moved around to different
    composed systems. What about when a sled is not in a composed system?
  - Being able to look at a d-bus path and know it's a remote sled and which
    composed system it's in could be quite useful
  - i.e. /xyz/openbmc_project/state/ComposedSystem`X`/AggregateBmc`Y`
- How can a sled request a reset of the composed system?
  - Define a Redfish event that the aggregator will look for to cause a reboot
    of the composed system?
- Host Watchdog - How should this function be implemented? If boot is driven via
  Rack Manager but communication from host is to an arbitrary sled, how do we
  reset the watchdog during boot?
- If BootProgress will only be sent to one sled (i.e. single host firmware image
  booting across all sleds), there is no "use the lowest one" concept needed.
- Does phal-export-devtree still need to run? Is the devtree even needed anymore
  on the Sleds?
- Does IBM systems still require a Hypervisor state object? It is driven off of
  BootProgress so could probably just have one per composed system on the
  aggregator a
- hw-diags counts on the stop-instructions service to stop it on power downs,
  that wont be present on sleds so a new design will be needed there (other
  scenarios like this to talk through?)

## Alternatives Considered

One alternative is to keep all of the logic within a PSM application. This
application would need the aggregator to provide D-Bus interfaces to read and
write to other sleds. The benefit of the D-Bus abstraction and separation of
code logic is not enough of a benefit to adding all of the overhead required to
have the D-Bus abstraction. It would need to handle propagating events like
state changes and boot errors of all other sleds on D-Bus.

Another alternative is for a PSM application to do all of the communication to
other sleds directly over Redfish. This would require each individual
application within OpenBMC that requires this type of communication to replicate
the http stack of bmcweb, along with all the needed authentication and handling
of sled failures. This would cause a lot of redundant code.

## Impacts

The aggregator application will need to have some concept of state. For example,
when it initiates a power on operation, it will need to setup Redfish events for
host state changes on all sleds that are a part of the system so that it can
monitor for failures and initiate appropriate recovery when needed.

### Organizational

- Does this repository require a new repository? No
- Which repositories are expected to be modified to execute this design? bmcweb,
  phosphor-state-manager

## Testing

Need at least 2 BMC's in a system. Verify state is returned correctly from an
Aggregate perspective. Power system on, off, reboot and ensure both sleds
operate in parallel and synchronize as expected.
