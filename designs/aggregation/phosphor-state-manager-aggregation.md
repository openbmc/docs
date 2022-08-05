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
software design and terms used by this docuemnt.

Issues with the Aggregator failing and potential fail over scenarios are outside
the scope of this docuemnt as there will need to be some larger system
architecture in this area.

Here's a quick summary of the PSM functions which will be affected by BMC
aggregation:

##### Overall owner of BMC, Chassis, and Host states
[PSM][psm-overview] provides the following D-Bus objects:

- /xyz/openbmc_project/state/bmc`X`
- /xyz/openbmc_project/state/chassis`X`
- /xyz/openbmc_project/state/host`X`
- /xyz/openbmc_project/state/hypervisor`X`

Where `X` represents the instance of the object. For legacy reasons, an
instance of 0 will always represent the entire system (all nodes) so a request
to reboot bmc0 or to power on chassis0/host0 will need to result in that request
going to all nodes.

Will the Aggregator need to implement the 0 interfaces so that requests to
them are properly aggregated to all nodes? Should the default for PSM be to
start at instance 1 for it's objects?

How is overall state represented accross multiple nodes? Is it the "least" or
the "majority" or just whatever the Aggregator PSM has? What happens when a node
has a BMC or Host in the Quiesced state but other nodes are working?

##### Power Restore
The [Power Restore][power-restore] logic is run when a BMC boots to BMC Ready.
It has logic to automatically power the system back on depending on user
settings.

Will need to determine if the power outage was just for the node the BMC
is in, or if it affected other nodes within the system. The system design may
or may not be able to handle a node losing power. Depending on what the system
can support, proper power restore logic may need to be coordinated by the
Aggregator.

##### BMC reset when host is booted
The [BMC reset when host is us booted][reset-booted] design handles reconnecting
with the host firmware after a BMC reboot. For a period of time, the node
being rebooted will be unavailable. Some thought will be need to be put into
what type of "system state" is returned to clients in this scenario. Similar
concerns occur when a node just becomes unavailable for whatever reason.

Also need to confirm that non-Aggregator BMC's can reconnect with the host
firmware in the same way it does on the Aggregator.

##### Host boot progress
The [Boot Progress][boot-progress] issues are similar to the PSM state
questions, how is host BootProgress represented when there are multiple host
firmwares booting accross the nodes within the same system? BootProgress is
under /redfish/v1/Systems/system/BootProgress which means it is an overall
system representation.

##### Hypervisor state tracking
Within a system there is only one hypervisor running. Does only the Aggregator
represent its state and allow control or does each node need to have a
representation of the hypervisor state?

##### Scheduled host transitions
The scheduled host transition function provides interfaces for users to
schedule a PSM operation in the future. For example, someone could schedule
a power off  of their system over night and a power on in the morning.

This function would be on a system basis, so only impelemented/supported by
the Aggregator? What's the impact if all BMC nodes were scheduled to power on?
Could all nodes run it but the aggregator ensure they all have the same
schedule? Schedule is based on BMC time so all BMC's will need to be time
synchronized.

##### obmcutil
obmcutil has quite a few different commands within it. In general the
expectation is that the commands within it operate on a system basis. So if
a user does a `obmcutil poweron` the expectation is that all nodes witin that
system will be powered on and booted. But other commands like `showlog` or
`deletelogs` could be node specific.

[aggr-design]: https://gerrit.openbmc.org/c/openbmc/docs/+/44547/2/designs/redfish-aggregation.md
[psm-overview]: https://github.com/openbmc/docs/blob/master/designs/state-management-and-external-interfaces.md
[power-restore]: https://github.com/openbmc/docs/blob/master/designs/power-recovery.md
[reset-booted]: https://github.com/openbmc/docs/blob/master/designs/bmc-reset-with-host-up.md
[boot-progress]: https://github.com/openbmc/docs/blob/master/designs/boot-progress.md

## Background and References
(1-2 paragraphs) What background context is necessary? You should mention
related work inside and outside of OpenBMC. What other Open Source projects
are trying to solve similar problems? Try to use links or references to
external sources (other docs or Wikipedia), rather than writing your own
explanations. Please include document titles so they can be found when links
go bad.  Include a glossary if necessary. Note: this is background; do not
write about your design, specific requirements details, or ideas to solve
problems here.

## Requirements
(2-5 paragraphs) What are the constraints for the problem you are trying to
solve? Who are the users of this solution? What is required to be produced?
What is the scope of this effort? Your job here is to quickly educate others
about the details you know about the problem space, so they can help review
your implementation. Roughly estimate relevant details. How big is the data?
What are the transaction rates? Bandwidth?

## Proposed Design
(2-5 paragraphs) A short and sweet overview of your implementation ideas. If
you have alternative solutions to a problem, list them concisely in a bullet
list.  This should not contain every detail of your implementation, and do
not include code. Use a diagram when necessary. Cover major structural
elements in a very succinct manner. Which technologies will you use? What
new components will you write? What technologies will you use to write them?

## Alternatives Considered
(2 paragraphs) Include alternate design ideas here which you are leaning away
from. Elaborate on why a design was considered and why the idea was rejected.
Show that you did an extensive survey about the state of the art. Compares
your proposal's features & limitations to existing or similar solutions.

## Impacts
API impact? Security impact? Documentation impact? Performance impact?
Developer impact? Upgradability impact?

### Organizational
- Does this repository require a new repository?  (Yes, No)
- Who will be the initial maintainer(s) of this repository?
- Which repositories are expected to be modified to execute this design?
- Make a list, and add listed repository maintainers to the gerrit review.

## Testing
How will this be tested? How will this feature impact CI testing?
