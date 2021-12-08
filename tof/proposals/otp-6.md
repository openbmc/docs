OTP: 6
Title: OpenBMC Inventory architecture
Author: Andrei Kartashev <a.kartashev@yadro.com>
Create: 2021-12-07
Type: Process
Requires: N/A
Replaces: N/A
Superseded-By: N/A

---
# OTP-6: OpenBMC Inventory architecture

## Background

Originally the inventory in OpenBMC was individually handled by Phosphor
Inventory Manager (PIM). The idea behind the PIM that different services can
detect Inventory Items and send information regarding them to the PIM which
store and expose it independently. Actual PIM implementation is limited to only
expose information came with `Notify` call. Once inventory object exposed it
can't be removed - only modified. As result, all the managed inventory objects
was exposed by single service under same hierarchy.

This ideas was actually never documented, which opens some kind of flexibility
in inventing own ways to expose Inventory Items.

Entity Manager (EM) bring to OpenBMC new way of inventory organization: now
Inventory Items exposed not only by single manager (EM this case) but also by
number of other services like smbios-mdrv2,  CPU Info, peci-pcie, pldm,
dbus-sensors and so on. Each of this services has it's own understanding about
which interfaces should be implemented by items and how exactly. This make it
difficult to get full inventory - one need to know how exactly to get items of
each types. 

Additionally, EM primary function is to be hardware-based configuration system.
Because of this, EM can only have plain organization of items itself and most
of the items have children configuration nodes which are not Inventory Items.

Nowadays inventory mainly used by bmcweb, where each Inventory Item type
handled by its own code, no any generic code there (bmcweb is Redfish driven
and Redfish have no Inventory concept), and thus no one cares about being
uniform. Any other software have to do same if it want to obtain Inventory
list.

## Proposal

In order to get consistency in Inventory system implementation we should first
bring architecture documentation for inventory which will describe strict rules
on how OpenBMC works with inventory information in dbus. Once we agreed on
inventory architecture we can start to fix/refactor corresponding daemons where
required.

The final result of this proposal should be architectural document in
openbmc/docs repo.

## Definitions

* Inventory - OpenBMC subsystem aimed to represent composition of system
  hardware
* Inventory Item - particular piece of hardware that supposed to be shown in
  inventory

## Functional requirements

* Enumerate hardware in the system (dynamically add/remove items)
* Expose Inventory Items information to dbus:
 * name
 * type
 * presence/absence
 * asset (vendor/model/serial/etc)
 * type-specific information
* Provide associations between system sensors and Inventory Items
* Provide a way to get Inventory Item health status
* Provide a way to determine IPMI SDR attributes for Inventory Item and
  associated sensors (EntityId, EntityInstance, FRU locator)
* Add log entries on Inventory Item add/remove
* Ability to model Inventory Item hierarchy
* Merge information from different sources

## Usecases

Here is the list of Inventory usecases which are broken or implemented in weird
way:

### Retrieve full Inventory Items list 

There is no way to get all items with single dbus call since items spread over
several daemons (one can't call GetManagedObjects for PIM to get full
inventory) with different interfaces set (e.g. peci-pcie doesn't implement any
of `xyz.openbmc_project.Inventory.XXX` interfaces bu only undocumented
`xyz.openbmc_project.PCIe.Device`).

### Inconsistent logging of Inventory Item insertion/removal

EM sends log message when it detect or lose new Item but no other daemons do.
As result we have Inventory-related entries in the Redfish log only for some
Item types, but not for other.

### Merge data from several sources

Different inventory information about same item may come from different daemons
(e.g. CPU can me expose by both CPU Info and smbios-mdrv2 daemons) and there is
no any simple and common ways to merge this information.

### Hierarchical connections between Items

EM can't model hierarchical relation between Inventory Items since it have
plain structure and almost zero knowledge on Items relation. There is also no
any strict universal rules on now to describe connectors, which can be used by
software to represent to end user information on where the Inventory Item
physically located.

### Child Items, based on top-level

EM can't create derived Inventory Items like fans or cables, which causes
inventing weird solutions like upcoming gpiopresencesensor in dbus-sensors.

### IPMI SDR representation

When using static SDR layout in the `phosphor-ipmi-host`  EntityId and
EntityInstance for all Inventory Items and related sensors are defined in YAML,
but there is no good way to do so when Dynamic SDR is used. There was several
attempts to invent a way to do this, but problem still actual.

## Actions

1.  Make a decision on hierarchy representation way
    (https://gerrit.openbmc-project.xyz/c/openbmc/docs/+/41468)
2.  Define dbus objects paths structure for all kind of Inventory Items.

Any object should start with prefix `/xyz/openbmc_project/inventory`, rest of
the path should be generated based on the rules, we define lately, after
decision on (1) is made.
3.  Define set of interfaces, mandatory for any Inventory Types:
 * xyz.openbmc_project.Inventory.Item
 * xyz.openbmc_project.Inventory.Decorator.Asset (should it?)

Describe how all properties of this interfaces should be filled and
interpreted.
4. Define recommended interfaces for Inventory Types:
 * xyz.openbmc_project.State.Decorator.OperationalStatus

Describe how this interfaces should be used and when they might be not
implemented.
5. Define special interfaces for each of particular item types (CPU, DIMM,
   Storage Drive, etc).
[TBD]
6. Define how inventory-related operations should be logged by different
   daemons 
[TBD]: this kind of complicated question when we come to data duplication over
several services. BTW information about detected items should be cached to get
rid of annoying "XXX was installed" on every BMC boot
7. Define recommendations on how we should process situations when information
   about same Item came from several sources.
Most cases, there are "primary" source, which expose main information and some
secondary service which can provide some additional information like actual
health status, presence (if main service can't detect it by itself), extra
details and so on. The ideal case would be to get entire information by single
call.
8. Decide what to do with Child Items in EM (and if we need to do something
   with them).
9. Decide on a way how do we configure EntityId and EntityInstance for IPMI
   Dynamic SDR (likely, there should be some special interface for that).

## Optional proposal

Since EM's main goal is to provide hardware-based software configuration,
providing inventory is kind of secondary function. We can delegate inventory
function back to PIM, while EM will keep configuration function. Required
changes to implement this is to refactor EM to not add
"xyz.openbmc_project.Inventory.XXX" interfaces to exposed objects but push
Inventory Item information to PIM instead. Some modifications to PIM would also
be required (e.g. logging, items removal, graceful merging information from
different surces and so on).

Other inventory daemons should also be refactored in such way, but that should
not be a big deal.

This approach require lot of work, but will help to solve most of the issues,
described above.

