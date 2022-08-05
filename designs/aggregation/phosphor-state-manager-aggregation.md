# Impact of Aggregation on phosphor-state-manager

Author: Author: Andrew Geissler (geissonator)

Other contributors:

Created: August 4th, 2022

## Problem Description
The impacts of [BMC aggregation][aggr-design] on OpenBMC software will have
significant impacts to some areas of the code base. The goal of this document
is to highlight the challenges (and solutions) that phosphor-state-manager (PSM)
will have.

Please review the referenced aggregation design for a general overview of the
software design and terms used by this document.

Issues with the Aggregator failing and potential fail over scenarios are outside
the scope of this document as there will need to be some larger system
architecture in this area.

Here's a quick summary of the PSM functions which will be affected by BMC
aggregation:

### Overall owner of BMC, Chassis, and Host states
[PSM][psm-overview] provides the following D-Bus objects:

- /xyz/openbmc_project/state/bmc`X`
- /xyz/openbmc_project/state/chassis`X`
- /xyz/openbmc_project/state/host`X`
- /xyz/openbmc_project/state/hypervisor`X`

Where `X` represents the instance of the object. For legacy reasons, an instance
of 0 will always represent the entire system so a request to reboot bmc0 or to
power on chassis0/host0 will need to result in that request going to all sleds.

How does state management software present a consistent state to external
clients when the overall system state is a combination of states from each
satellite BMC? What happens when a sled has a BMC or Host in the Quiesced state
but other sleds are working?

What about when a non-Aggregator sled needs to trigger a system wide power off
or reboot of the system?

### Power Restore
The [Power Restore][power-restore] logic is run when a BMC boots to BMC Ready.
It has logic to automatically power the system back on depending on user
settings.

Will need to determine if the power outage was just for the sled the BMC
is in, or if it affected other sleds within the system. The system design may
or may not be able to handle a sled losing power. Depending on what the system
can support, proper power restore logic may need to be coordinated by the
Aggregator.

Another function is this area provided by PSM is the monitoring of a
Uninterruptible Power Supply (UPS) if present. The UPS status could come into
any sled depending on which one the user plugs its status line to, so there must
be a mechanism for any sled to report the status of the UPS to the Aggregator.

### BMC reset when host is booted
The [BMC reset when host is booted][reset-booted] design handles reconnecting
with the host firmware after a BMC reboot. For a period of time, the sled
being rebooted will be unavailable. Some thought will be need to be put into
what type of "system state" is returned to clients in this scenario. Similar
concerns occur when a sled just becomes unavailable for whatever reason.

Also need to confirm that non-Aggregator BMC's can reconnect with the host
firmware in the same way it does on the Aggregator.

### Host boot progress
The [Boot Progress][boot-progress] issues are similar to the PSM state
questions, how is host BootProgress represented when there are multiple host
firmwares booting across the sleds within the same system? BootProgress is
under /redfish/v1/Systems/system/BootProgress which means it is an overall
system representation.

### Hypervisor state tracking
Within a system there is only one hypervisor running. Does only the Aggregator
represent its state and allow control or does each sled need to have a
representation of the hypervisor state?

### Scheduled host transitions
The scheduled host transition function provides interfaces for users to
schedule a PSM operation in the future. For example, someone could schedule
a power off  of their system over night and a power on in the morning.

This function would be on a system basis, so only implemented/supported by
the Aggregator? What's the impact if all BMC sleds were scheduled to power on?
Could all sleds run it but the aggregator ensure they all have the same
schedule? Schedule is based on BMC time so all BMC's will need to be time
synchronized.

### obmcutil
`obmcutil` has quite a few different commands within it. In general the
expectation is that the commands within it operate on a system basis. So if
a user does a `obmcutil poweron` the expectation is that all sleds within that
system will be powered on and booted. But other commands like `showlog` or
`deletelogs` could be sled specific.

### Error Paths
When a boot fails, there are usually automatic reboots that occur. In a multi
sled system, the boot will need to be restarted on all sleds when one fails (or
the failed sled will need to be removed from the system). How does the
Aggregator know a boot failed, how does it properly halt the working sleds and
coordinate a reboot. The host reboot count of a working sled should not be
affected by a reboot due to another sleds boot failure. What happens when the
issue is the cable between the sleds? How does a sled get its reboot count
reset when a sled is removed due to a bad cable?



[aggr-design]: https://gerrit.openbmc.org/c/openbmc/docs/+/44547/2/designs/redfish-aggregation.md
[psm-overview]: https://github.com/openbmc/docs/blob/master/designs/state-management-and-external-interfaces.md
[power-restore]: https://github.com/openbmc/docs/blob/master/designs/power-recovery.md
[reset-booted]: https://github.com/openbmc/docs/blob/master/designs/bmc-reset-with-host-up.md
[boot-progress]: https://github.com/openbmc/docs/blob/master/designs/boot-progress.md

## Background and References
None other than what was put in previous section.

## Requirements

- Provide an aggregated state of all supported objects as required by the
  Redfish specification
  - Some states, such as host will need to be aggregated, but other such
    as chassis and BMC have their own in individual states within Redfish
  - In general the "least" state of the aggregate will be returned
  - This includes the host `BootProgress`
- Provide system level control
  (`redfish/v1/Systems/system/Actions/ComputerSystem.Reset`) support for all
  sleds within the aggregate
- Provide the ability for non-Aggregator sleds to issue composed system
  operations
- Do not allow a composed system to power on if a UPS has run out of battery
  power and/or a brown out condition is present
- Provide automated recovery when bmc, chassis, or host fails in a sled
- Ensure power restore and scheduled host transition logic is only run on a
  system basis
- No host POST code aggregation will be done
  - Host firmware can include a sled identifier in their POST code if
    if differentiation is required
- Update `obmcutil` to support an individual sled and an aggregate system

## Proposed Design
Move the system view and control of state into the Aggregator. The Aggregator
will initially just be the bmcweb instance running on the sled which has been
selected as the Aggregator. It is still to-be-determined on whether this
function will be broken out into a separate application.

This new function will aggregate state across all sleds and ensure commands
to change state are issued to all sleds. It will create D-Bus objects that
represent instance 0 of all state objects. It will handle failures of sleds
and recovery of the system. It will interact with the local PSM object to gather
and control state on its sled, as well as using Redfish to communicate with the
other sleds and their corresponding PSM instances. The PSM applications will
create objects with instance 1 on systems that support aggregation and that will
represent the state of the sled it is running on. On non-aggregating systems,
PSM will continue to default to creating instance 0 of all state objects.

On sleds that are not the Aggregator, bmcweb will still implement instance 0
of all PSM objects. The sled bmcweb will send the requested operation back
to the Aggregator using Redfish. The sled will also send the operation to the
local PSM object to ensure operations like emergency power off's due to thermal
or power issues are issued quickly.

The power restore function and scheduled host transitions will only be run
within the Aggregator.

obmcutil will add an option to target the entire system, or just the sled
it is executed on. The default will be the system.

## Alternatives Considered
One alternative is to keep all of the logic within a PSM application. This
application would need the Aggregator to provide D-Bus interfaces to read and
write to other sleds. The benefit of the D-Bus abstraction and separation of
code logic is not enough of a benefit to adding all of the overhead required
to have the D-Bus abstraction. It would need to handle propagating events like
state changes and boot errors of all other sleds on D-Bus.

Another alternative is for a PSM application to do all of the communication to
other sleds directly over Redfish. This would require each individual
application within OpenBMC that requires this type of communication to replicate
the http stack of bmcweb, along with all the needed authentication and handling
of sled failures. This would cause a lot of redundant code.

## Impacts
The Aggregator application will need to have some concept of state. For example,
when it initiates a power on operation, it will need to setup Redfish events
for host state changes on all sleds that are a part of the system so that it
can monitor for failures and initiate appropriate recovery when needed.

### Organizational
- Does this repository require a new repository?
  No
- Which repositories are expected to be modified to execute this design?
  bmcweb, phosphor-state-manager

## Testing
Need at least 2 BMC's in a system. Verify state is returned correctly from
an Aggregate perspective. Power system on, off, reboot and ensure both sleds
operate in parallel and synchronize as expected.
