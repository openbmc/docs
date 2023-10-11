# Separating nvmesensor from openbmc/dbus-sensors

Author: Andrew Jeffery <andrew@codeconstruct.com.au>

Created: October 11 2023

## Problem Description

The [NVMe Management Interface specification][nvme-mi] (NVMe MI, or just MI)
deals with many aspects of NVMe drives beyond exposing sensor information from
an NVMe device or enclosure. The `nvmesensor` daemon in D-Bus sensors [has been
augmented][nvmesensor-rework] to expose many of these management capabilities.
This work is yet to be merged, however, once it is, the results are that:

1. `nvmesensor` becomes much more than a sensor daemon
2. `nvmesensor` becomes a significant portion of the dbus-sensors codebase
3. Maintenance of `nvmesensor` requires significant domain-specific knowledge

[nvme-mi]: https://nvmexpress.org/wp-content/uploads/NVM-Express-Management-Interface-Specification-1.2c-2022.10.06-Ratified.pdf
[nvmesensor-rework]: https://gerrit.openbmc.org/q/owner:%2522Hao+Jiang%2522+repo:openbmc/dbus-sensors

## Background and References

### References

1. [Management Component Transport Protocol (MCTP) Base Specification][dmtf-pmci-dsp0236]
2. [Management Component Transport Protocol (MCTP) SMBus/I2C Transport Binding Specification][dmtf-pmci-dsp0237]
3. [NVM Express Base Specification][nvme-base]
4. [NVM Express Management Interface Specification][nvme-mi]
5. [Technical Note on Basic Management Command][nvme-mi-basic-technical-note]
6. [Redfish Specification][dmtf-redfish-dsp0266]
7. [Swordfish Scalable Storage Management API Specification][snia-swordfish]

[dmtf-pmci-dsp0236]: https://www.dmtf.org/sites/default/files/standards/documents/DSP0236_1.3.1.pdf
[dmtf-pmci-dsp0237]: https://www.dmtf.org/sites/default/files/standards/documents/DSP0237_1.2.0.pdf
[dmtf-redfish-dsp0266]: https://www.dmtf.org/sites/default/files/standards/documents/DSP0266_1.19.0.pdf
[nvme-base]: https://nvmexpress.org/wp-content/uploads/NVM-Express-Base-Specification-2.0c-2022.10.04-Ratified.pdf
[nvme-mi-basic-technical-note]: https://nvmexpress.org/wp-content/uploads/NVMe_Management_-_Technical_Note_on_Basic_Management_Command_1.0a.pdf
[snia-swordfish]: https://www.snia.org/sites/default/files/technical-work/swordfish/release/v1.2.5a/pdf/Swordfish_v1.2.5a_Specification.pdf

### A brief history of NVMe management and inventory stacks in OpenBMC

Like a number of other elements of the ecosystem, OpenBMC has evolved multiple
solutions for managing NVMe devices:

1. [phosphor-nvme][]
2. `nvmesensor` from [dbus-sensors][]

[dbus-sensors]: https://github.com/openbmc/dbus-sensors/
[phosphor-nvme]: https://github.com/openbmc/phosphor-nvme/

`phosphor-nvme` is well-suited to OpenBMC platforms which utilise a fixed
firmware configuration (i.e. platform-specific firmware). It consumes the
configuration file at `/etc/nvme/nvme_config.json` which defines drive location
in terms of the platform I2C topology and GPIOs for sensing. It hosts D-Bus
objects representing the available drives, which implement the usual sensor and
status interfaces. Sensor and status information is populated by issuing the
[NVMe MI Basic Management Command][nvme-mi-basic-technical-note] to the drives.
The inventory information for the drives is pushed into
[phosphor-inventory-manager][].

[phosphor-inventory-manager]: https://github.com/openbmc/phosphor-inventory-manager/

By contrast, `nvmesensor` from `dbus-sensors` is implemented in terms of
[entity-manager][] for inventory- and configuration-management. `nvmesensor`
dynamically responds to changes in system configuration as signaled by
`entity-manager`. Like `phosphor-nvme`, `nvmesensor` hosts the usual sensor
interface. The current upstream implementation populates this using the [NVMe MI
Basic Management Command][nvme-mi-basic-technical-note], however this was not
always the case. The original implementation used the NVMe MI MCTP stack, albeit
with a fork of [libmctp][] implementing the [SMBus MCTP
binding][dmtf-pmci-dsp0237] in userspace, which required a further fork of Linux
to expose the [out-of-tree I2C slave mqueue device][i2c-slave-mqueue].
Conversion to the basic management command was done to achieve something that
could continue to exist upstream and be enabled by default without requiring
forks of `libmctp` and the kernel. [The outstanding patches in
Gerrit][nvmesensor-rework] return the MCTP functionality to `nvmesensor`, though
this time in terms of Linux's [AF\_MCTP][linux-mctp] support via [libnvme][] now
that these options are available.

[entity-manager]: https://github.com/openbmc/entity-manager/
[i2c-slave-mqueue]: https://github.com/AspeedTech-BMC/linux/commit/25ffc39c2330a2ddbdea164ef9ada0233650fdc8
[libmctp]: https://github.com/openbmc/libmctp
[libnvme]: https://github.com/linux-nvme/libnvme
[linux-mctp]: https://docs.kernel.org/networking/mctp.html

The work to integrate support for MI-over-MCTP via `libnvme` and to expose the
resulting capabilities was not always destined for `nvmesensor`. The significant
increase in functionality that results prompted an earlier proposal to [create a
third NVMe stack in OpenBMC][design-proposal-nvmed]. Consensus from the reviews
was that there was no reason to avoid building on `nvmesensor`, and a preference
away from a third stack.

[design-proposal-nvmed]: https://gerrit.openbmc.org/c/openbmc/docs/+/53809

Over the course of these events the OpenBMC community has reached a preference
for the dynamic inventory management approach of `entity-manager` over the more
static firmware configurations associated with `phosphor-inventory-manager`.
As an example, some evidence for this [can be found in the Discord
conversation][discord-openbmc-inventory-prefer-em]
between [Brad Bishop (`radsquirrel`)][brad-bishop], [Matt Spinler
(`mspinler`)][matt-spinler] and `Xavier`.

[brad-bishop]: https://github.com/orgs/openbmc/teams?query=%40bradbishop
[discord-openbmc-inventory-prefer-em]: https://discord.com/channels/775381525260664832/775381525260664836/1085595133279469609
[matt-spinler]: https://github.com/orgs/openbmc/teams?query=%40spinler

### The NVMe MI Basic Management Command

[The basic management command][nvme-mi-basic-technical-note] is a regular SMBus
block read operation that returns six bytes of drive status data in its
payload. The response bytes are laid out as follows:

1. Status flags
2. SMART warnings
3. Composite Temperature
4. Drive Life Used
5. Reserved
6. Reserved

A minor variation on Basic Management Command block read provides access to the
drive's static data - the PCIe vendor ID and drive serial number.

This is the full extent of the functionality exposed by the basic management
command. The NVMe MI Workgroup Chair made the following statement, demonstrating
the basic management command was intended as a crutch to be used until the more
complete NVMe MI specification was finalised:

> The NVMe-MI workgroup is standardizing a comprehensive management
> architecture that builds on top of the Distributed Management Task Force (DMTF)
> Management Component Transport Protocol (MCTP) and allows out-of-band
> management of an NVMe device over SMBus and over PCIe using Vendor Defined
> Messages. The NVMe-MI 1.0 specification is expected to be completed later this
> year and allows a management controller to both monitor the status of an NVMe
> device as well as configure its state. This enables advanced features such as
> out-of-band firmware updates.
>
> Prior to completion and public availability of the NVMe-MI 1.0 specification,
> we have received feedback that a standardized method is needed by the industry
> to poll an NVMe device for basic health status information and which only
> requires SMBus slave support. The intent of the technical note is to publicly
> release a standardized approach for such a capability.

https://nvmexpress.org/nvme-basic-management-command/

### A brief discussion of NVMe MI capabilities

By contrast to the basic management command, [the MI capabilities exposed over
the MCTP interface are extensive][nvme-mi]. The available commands are divided
into three classes:

1. Management Interface Command Set
2. NVM Express Admin Command Set
3. PCIe Command Set

Command sets 1 and 3 are largely uninteresting for the purpose of setting the
scene. Command set 2 on the other hand provides comprehensive management of
namespaces, virtualization, capacity, sanitation and IO queues associated with
the endpoint. Exposure of these capabilities is essential for the related
purpose of implementing support for [SNIA's Swordfish
specification][snia-swordfish] in OpenBMC.

## Requirements

That `nvmesensor` is decoupled from the `dbus-sensors` codebase to allow the
implementation of the remaining `dbus-sensors` applications to progress
independently of the development of `nvmesensor`, and vice versa.

### Expectations

The separation of code will help with the separation of expectations with
respect to knowledge and experience of relevant standards, covering at least:

1. [Management Component Transport Protocol (MCTP) Base Specification][dmtf-pmci-dsp0236]
2. [Management Component Transport Protocol (MCTP) SMBus/I2C Transport Binding Specification][dmtf-pmci-dsp0237]
3. [NVM Express Base Specification][nvme-base]
4. [NVM Express Management Interface Specification][nvme-mi]
5. [Redfish Specification][dmtf-redfish-dsp0266]
6. [Swordfish Scalable Storage Management API Specification][snia-swordfish]

[nvme-base]: https://nvmexpress.org/wp-content/uploads/NVM-Express-Base-Specification-2.0c-2022.10.04-Ratified.pdf
[dmtf-redfish]: https://www.dmtf.org/sites/default/files/standards/documents/DSP0266_1.19.0.pdf

## Proposed Design

`nvmesensor` is moved to its own repository.

Proposed repository names, in order of preference:

1. openbmc/nvmed
2. openbmc/nvmesensor

## Alternatives Considered

The preference of `entity-manager` as the inventory solution over
`phosphor-inventory-manager` suggests there will be increased interest in
`nvmesensor` as the NVMe MI implementation over `phosphor-nvme`.

During initial discussion on Discord the idea of [migrating `nvmesensor` into
the `phosphor-nvme` repository][discord-migrate-nvmesensor] was floated, however
offline discussions with [Brandon Kim][brandon-kim] concluded that we likely
want to grandfather out `phosphor-nvme` with the systems that use it. Merging
the two codebases into the one repository won't prevent us from needing to
continue to build both solutions as there is not much appetite to migrate
existing platforms from `phosphor-nvme` to `nvmesensor`. The presence of both
solutions in the `phosphor-nvme` repository will likely be an ongoing source of
surprise and confusion.

[discord-migrate-nvmesensor]: https://discord.com/channels/775381525260664832/819741065531359263/1159545637352767589
[brandon-kim]: https://github.com/openbmc/phosphor-nvme/blob/master/OWNERS#L46-L48

## Impacts

### API Impact

None: This proposal is about moving the code, not changing the code.

### Security Impact

None: This proposal is about moving the code, not changing the code.

### Documentation Impact

Any documentation in `dbus-sensors` referencing `nvmesensor` will need to be
updated with a forward-reference to the new location.

Any documentation outside of `dbus-sensors` referencing `nvmesensor` with
respect to `dbus-sensors` will need to be updated. This may impact
`openbmc/entity-manager` and `openbmc/docs`, for example.

### Performance Impact

None: This proposal is about moving the code, not changing the code.

### Developer Impact

Developers will need to re-orient themselves with the new location of the code.
The movement will impact the bitbake metadata, likely through a new recipe,
which will require a change in habits with respect to `devtool modify` and
similar.

### Upgradability Impact

None: The same implementation should be installed as it was when it was
developed under `dbus-sensors`.

### Organizational Impact

1. Does this repository require a new repository?
  - Yes

2. Who will be the initial maintainer(s) of this repository?
  - Andrew Jeffery <andrew@codeconstruct.com.au>

3. Which repositories are expected to be modified to execute this design?
  - `dbus-sensors`

## Testing

Any relevant tests present in `dbus-sensors` will be migrated with the
implementation.
