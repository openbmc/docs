# Separating nvmesensor from openbmc/dbus-sensors

Author: Andrew Jeffery <andrew@codeconstruct.com.au>

Created: October 11 2023

## Problem Description

The [NVMe Management Interface specification][nvme-mi] (NVMe MI, or just MI)
deals with many aspects of NVMe drives beyond exposing sensor information from
an NVMe device or enclosure. The `nvmesensor` daemon in D-Bus sensors [has been
augmented][nvmesensor-rework] to expose many of these management capabilities.
While this work is yet to be merged, however, once it is, the results are that:

[nvme-mi]: https://nvmexpress.org/wp-content/uploads/NVM-Express-Management-Interface-Specification-1.2c-2022.10.06-Ratified.pdf
[nvmesensor-rework]: https://gerrit.openbmc.org/q/owner:%2522Hao+Jiang%2522+repo:openbmc/dbus-sensors

1. `nvmesensor` becomes much more than a sensor daemon
2. `nvmesensor` becomes a significant portion of the dbus-sensors codebase

## Background and References

Like a number of other elements of the ecosystem, OpenBMC has evolved multiple
solutions for managing NVMe devices:

1. [phosphor-nvme][]
2. `nvmesensor` from [dbus-sensors][]

[phosphor-nvme]: https://github.com/openbmc/phosphor-nvme/
[dbus-sensors]: https://github.com/openbmc/dbus-sensors/

`phosphor-nvme` is well-suited to OpenBMC platforms which utilise a fixed
firmware configuration (i.e. platform-specific firmware). It consumes the
configuration file at `/etc/nvme/nvme_config.json` which defines drive location
in terms of the platform I2C topology and GPIOs for sensing. It hosts D-Bus
objects representing the available drives, which implement the usual sensor and
status interfaces. Sensor and status information is populated by issuing the
[NVMe MI Basic Management Command][nvme-mi-basic-technical-note] to the drives.
The inventory information for the drives is pushed into
[phosphor-inventory-manager][].

[nvme-mi-basic-technical-note]: https://nvmexpress.org/wp-content/uploads/NVMe_Management_-_Technical_Note_on_Basic_Management_Command_1.0a.pdf
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
could continue to upstream and be enabled by default without requiring forks of
`libmctp` and the kernel. [The outstanding patches in Gerrit][nvmesensor-rework]
return the MCTP functionality to `nvmesensor`, though this time in terms of
Linux's [AF\_MCTP][linux-mctp] support via [libnvme][] now that these options
are available.

[entity-manager]: https://github.com/openbmc/entity-manager/
[libmctp]: https://github.com/openbmc/libmctp
[dmtf-pmci-dsp0237]: https://www.dmtf.org/sites/default/files/standards/documents/DSP0237_1.2.0.pdf
[i2c-slave-mqueue]: https://github.com/AspeedTech-BMC/linux/commit/25ffc39c2330a2ddbdea164ef9ada0233650fdc8
[linux-mctp]: https://docs.kernel.org/networking/mctp.html
[libnvme]: https://github.com/linux-nvme/libnvme

The work to integrate support for MI-over-MCTP via `libnvme` was not always
destined for `nvmesensor`. The significant shift in functionality prompted an
earlier proposal to [create a third NVMe stack in
OpenBMC][design-proposal-nvmed]. Consensus from the reviews was that there was
no reason to avoid building on `nvmesensor`, and a preference away from a third
stack.

[design-proposal-nvmed]: https://gerrit.openbmc.org/c/openbmc/docs/+/53809

Over the course of these events the OpenBMC community has reached a preference
for the dynamic inventory management approach of `entity-manager` over the more
static firmware configurations associated with `phosphor-inventory-manager`.
As an example, some evidence for this [can be found in the Discord
conversation][discord-openbmc-inventory-prefer-em]
between [Brad Bishop (`radsquirrel`)][brad-bishop], [Matt Spinler
(`mspinler`)][matt-spinler] and `Xavier`. 

[discord-openbmc-inventory-prefer-em]: https://discord.com/channels/775381525260664832/775381525260664836/1085595133279469609
[brad-bishop]: https://github.com/orgs/openbmc/teams?query=%40bradbishop
[matt-spinler]: https://github.com/orgs/openbmc/teams?query=%40spinler

To reiterate, this proposal does not take the position that we need a third NVMe
stack, rather that we migrate the existing `nvmesensor` implementation out of
`dbus-sensors` to its own repository.

## Requirements

That `nvmesensor` is decoupled from the `dbus-sensors` codebase to allow the
implementation of the remaining `dbus-sensors` applications to progress
independently of the development of `nvmesensor`, and vice versa.

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

- Does this repository require a new repository?
  - Yes

- Who will be the initial maintainer(s) of this repository?
  - Andrew Jeffery <andrew@codeconstruct.com.au>

- Which repositories are expected to be modified to execute this design?
  - `dbus-sensors`

## Testing

Any tests present in `dbus-sensors` will be migrated with the implementation.
