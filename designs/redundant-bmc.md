# Redundant BMCs

Author: Matt Spinler (mspinler)

Other contributors: Andrew Geissler

Created: March 22, 2024

## Problem Description

IBM has an upcoming multi-chassis system architecture where two of the chassis
will have their own BMCs. These will be used in a redundant fashion, where only
one BMC at a time will access chassis hardware and be the primary point of
contact with the host and any system management consoles. If the active BMC
fails, the passive one needs to take over its duties.

This leads to several issues that this document will propose solutions for:

- How the active BMC can be chosen.
- How one BMC communicates with the other.
- How a failover is requested and performed.
- How other entities know the role of each BMC.
- What is the reduced function of the passive BMC.

As used above, the terminology used here is:

- `Active BMC` for the fully functioning BMC.
- `Passive BMC` for the other BMC.

This direction was first proposed on the mailing list [here][1].

## Background and References

A block diagram of the first two chassis of the system is shown below.

```text

   Chassis 0                       Chassis 1
   ┌────────────────────┐          ┌────────────────────┐
   │                    │          │                    │
   │   BMC Card         │          │   BMC Card         │
   │   ┌────────────┐   │          │   ┌────────────┐   │
   │   │            │   │          │   │            │   │
   │   │  ┌──────┐  │   │ Network  │   │  ┌──────┐  │   │
   │   │  │ BMC  │  │   │ Cable    │   │  │ BMC  │  │   │
   │   │  │      ├──┼───┼──────────┼───┼──┤      │  │   │
   │   │  │      │  │   │          │   │  │      │  │   │
   │   │  └─┬───┬┘  │   │          │   │  └─┬──┬─┘  │   │
   │   │    │   │   │   │FSI Cables│   │    │  │    │   │
   │   │    │   └───┼───┼───────┐  │   │    │  │    │   │
   │   │    │       │   │  ┌────┼──┼───┼────┘  │    │   │
   │   │    │       │   │  │    │  │   │       │    │   │
   │   │  ┌─┴────┐  │   │  │    │  │   │  ┌────┴─┐  │   │
   │   │  │Comm  │  │   │  │    │  │   │  │Comm  │  │   │
   │   │  │Device├──┼───┼──┘    └──┼───┼──┤Device│  │   │
   │   │  │      │  │   │          │   │  │      │  │   │
   │   │  └──┬───┘  │   │          │   │  └──┬───┘  │   │
   │   │     │      │   │          │   │     │      │   │
   │   └─────┼──────┘   │          │   └─────┼──────┘   │
   │         │I2C       │          │         │I2C       │
   │       ┌─┴─┐        │          │       ┌─┴─┐        │
   │       │   │        │          │       │   │        │
   │       └───┘        │          │       └───┘        │
   │                    │          │                    │
   └────────────────────┘          └────────────────────┘
```

There are two connections between the BMCs.

1. A network cable.
2. FSI cables. FSI stands for `FRU Support Interface` and is a point to point
   two wire bus which the AST2600 and AST2700 have masters for. FSI is also used
   by the BMC to communicate with IBM's Power processors.

The `Comm Device` is an IBM device that contains a data area that both BMCs can
access. It is also the source of all I2C connections to the chassis, and has a
switch that allows a BMC to claim the connection to the chassis's devices,
preventing the other one from doing so.

Each BMC also has:

- A network connection to the hardware management console, which is IBM's system
  management application running on a separate server that communicates via
  Redfish.
- A PCIe link to the host for communication.
- A GPIO to the reset pin of its sibling.
- An indication of its chassis position.

When a new BMC card is first plugged in, it will need to undergo a provisioning
process, externally started, to install any certificates and keys necessary for
it to communicate with other entities. The details of this process are outside
the scope of this document.

## Requirements

The high level requirements this design must meet are:

- There must be a way for the BMCs to agree on which will be active, and which
  will be passive. A single active BMC must still be chosen even if the inter
  BMC network cable isn't plugged in or working.

- As only a single BMC may access chassis hardware (EEPROMs, sensors, etc) at a
  time in IBM's configuration, there must be a way for the passive BMC to not
  run any services which would try to access this hardware.

- The BMCs must report via Redfish and PLDM the BMC's role and whether
  redundancy is enabled, and if a failover is in progress.

- There must be an API that can be called from both Redfish and the PCIe link to
  the host to start a failover. This must be provided on the passive BMC in case
  the active BMC is dead at the time.

- On the failover, the original passive BMC must be able to take over the active
  role without requiring a reboot. There is no such requirement for when the
  active becomes passive.

- There are certain requirements that need to be met for failovers to be
  enabled. This includes things like:

  - Code levels must match.
  - The passive BMC must be at the BMC ready state.

- There must be a way to mirror files, directories, certificates, accounts,
  possibly time, etc, from the active BMC to the passive so they are available
  to the passive BMC if a failover is required.

  - This requirement will get its own design document. Most likely it would
    require a separate sync daemon.

- Service files must not be directly modified to get them to behave differently
  in active vs passive situations, though drop-in config files can be used.

- There is no requirement that a failover be done after every reboot of the
  active BMC. For example, if a BMC reboots due to an out-of-memory condition,
  it can remain active when it comes back.

- There is no requirement for the passive BMC itself to detect a dead active BMC
  and initiate a failover.

## Proposed Design

### Active and Passive BMC Description

The active BMC is the fully functioning BMC. It will have all of its services
running and be the main point of contact for any external entities, providing
them a full view of the system.

On the other hand the passive BMC will be running only the bare minimum
services. It will still provide a Redfish model and have a web UI, but it will
be limited as it does not have a full view of the chassis hardware. Failovers
will go through this BMC, as the active BMC may be dead when a failover is
requested.

As only one BMC can access the chassis hardware at a time, any service that
touches that hardware will not run while the BMC is passive.

### systemd Unit File Design

As not all services can run on the passive BMC, there needs to be a way to have
services not start by default, and then only start at some point in the future
when the BMC assumes the active role.

This will be done with new systemd targets, links, and drop in config files.
These do not require modifying existing unit files, and will be installed only
for the target image.

To start with, a new target will be created: `obmc-bmc-active.target`.

This target will not be started as part of multi-user.target/BMC ready. This
means that initially neither BMC will be active, until something later tells one
to be.

These targets look like:

```sh
$ cat obmc-bmc-active.target
[Unit]
Description=Active BMC Target
BindsTo=start-stop-bmc-active.service
```

So that the desired services don't start when they usually would as part of
multi-user.target, it can use ConditionPathExists along with the following
helper services to create a file when the active target is alive:

```sh
$ cat start-stop-bmc-active.service
[Unit]
Description=Start/Stop the active target
After=obmc-bmc-active.target
PartOf=obmc-bmc-active.target

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStartPre=mkdir -p /run/openbmc/
ExecStart=/bin/touch /run/openbmc/active-bmc
ExecStop=/bin/rm -f /run/openbmc/active-bmc
```

This would be linked into the obmc-bmc-active.target.wants.

Now, an `active-only.conf` file can be installed for each service that isn't
required for the passive role:

```sh
$ cat active-only.conf
[Unit]
ConditionPathExists=/run/openbmc/active-bmc
After=start-stop-bmc-active.service
PartOf=obmc-bmc-active.target
```

The `PartOf` directive ensures the service stops when the active target stops,
though this isn't technically necessary as the BMC will just be rebooted when
going from active to passive.

So that the service starts as part of the active target, the service also needs
to be linked into obmc-bmc-active.target.wants.

As for exactly which services this applies to, that may depend on the specific
system as different systems can install different services. This implies the
linking and override files should be installed from a bitbake layer that has
that information. The .conf files may reside in a different common location as
they are common across systems.

### Application Design

This design proposes a new application, tentatively named `rbmc-state-manager`,
that runs on each BMC to handle determining roles, redundancy D-Bus APIs, etc.

#### Inter-BMC Communication

The application requires a method to exchange a minimal amount of information
with the other BMC. This design proposes abstracting this into D-Bus properties
hosted by a separate BMC interface application.

This application, however it obtains the data, will have D-Bus properties for
each item, with objects for each BMC. The current BMC would make data about
itself visible by writing the properties on a path that represents itself, and
would obtain information from the sibling by reading properties from an object
path that represents that BMC.

If the BMC needs to write something to the other BMC, it would write a property
or call a method on that BMC's path.

IBM's implementation will make use of the FSI connected device which is
accessible to both BMCs. It's not using the network path so that communication
can still be done even before a BMC is provisioned or if there is an issue with
the network cable. Another implementation could do something different, as long
as it uses the same D-Bus interface.

The proposed object paths for these are
`/xyz/openbmc_project/state/redundancy_info/this_bmc` and
`/xyz/openbmc_project/state/redundancy_info/sibling_bmc`.

The proposed D-Bus interface name is
`xyz.openbmc_project.State.Redundancy.BMC.Status`.

The starting list of properties on the interface is:

| Property          | Type   | Description                      |
| ----------------- | ------ | -------------------------------- |
| Version           | int    | Version of this data             |
| Position          | int    | Detected Chassis Pos             |
| Previous Role     | enum   | Previous Role                    |
| Role              | enum   | Determined Role                  |
| RedundancyEnabled | bool   | Redundancy enabled (active only) |
| Provisioned       | bool   | If already provisioned           |
| BMCState          | enum   | The BMC State                    |
| FwVersion         | uint32 | Firmware version hash            |
| Heartbeat         | uint32 | Heartbeat status                 |

The `Heartbeat` property would be periodically incremented by this application
and when monitored on the `sibling_bmc` path provides an indication that the
sibling is alive.

#### Startup and Discovery

When this new application starts up on each BMC, it will gather information
about itself and update the `this_bmc` D-Bus object so it's ready for the
sibling to read via the `sibling_bmc` object. Then it will start its heartbeat
so the sibling knows its alive and the contents are valid.

It may need to wait for the `sibling_bmc` object to show up on D-Bus, and for
its state to get to BMC ready, as the BMCs may not come up at exactly the same
time. There will be a timeout value as the sibling BMC may not even be present
or some hardware problem may prevent it from showing up.

Using the information about themselves and their sibling, the BMCs can then
determine their roles. The position zero BMC will choose the active role the
first time through if there is nothing preventing that from occurring, such as
the BMC being at quiesce. On subsequent reboots the BMCs would just maintain
their previous roles if nothing prevented it.

If the BMC hasn't been provisioned yet, it will not claim the active role and
will wait in the passive role for provisioning to occur. This could lead to both
BMCs being passive for a time for a new system.

If only one BMC was doing the POR, the sibling's determined role is available to
it so it claim the opposite.

After the active and passive roles have been established, there are other checks
that need to be done to see if redundancy can be enabled, like checking that the
code levels match and that the inter-BMC network is healthy. If redundancy isn't
enabled the system can still boot but a failover cannot be done.

#### Becoming Active

After the roles have been determined, the active BMC can activate all of its
services. It will do this by doing a full file sync and enabling background
syncing (separate design), and then starting the obmc-bmc-active.target to start
up all of the active services. It will also set the 'RedundancyEnabled` property
(see below) to indicate it is ready for failovers, assuming there isn't
something preventing redundancy from being enabled.

#### D-Bus Representation

The application needs to host a few of its own D-Bus properties for use by
others.

The proposed object path is `/xyz/openbmc_project/state/redundancy/bmc`.

| Name                  | Prop or Method | BMC     | Interface                                               |
| --------------------- | -------------- | ------- | ------------------------------------------------------- |
| Role                  | Property       | Both    | xyz.openbmc_project.State.Decorator.Redundancy.BMC      |
| RedundancyEnabled     | Property       | Both    | xyz.openbmc_project.State.Decorator.Redundancy.BMC      |
| FailoverInProgress    | Property       | Both    | xyz.openbmc_project.State.Decorator.Redundancy.BMC      |
| Provisioned           | Property       | Both    | xyz.openbmc_project.State.Decorator.Redundancy.BMC      |
| ManualFailoverDisable | Property       | Active  | xyz.openbmc_project.State.Decorator.Redundancy.Disable  |
| Failover(force)       | Method         | Passive | xyz.openbmc_project.State.Decorator.Redundancy.Failover |

The `Role` property reflects the current role and will also be available via
Redfish.

The `RedundancyEnabled` property is reported via Redfish to let external users
(the web UI and management console) know that the system has redundancy
operational. The passive BMC obtains this from the active BMC.

The `FailoverInProgress` property indicates a failover is currently in progress.
It only gets set to true on the original passive BMC, and stays true as the
passive becomes active until it completes.

The `Provisioned` property will be set to true by the provisioning code after
the provisioning process.

The `ManualFailoverDisable` property can be used by manufacturing test or the
lab to disable failovers for testing. It is persistent and remains until
disabled.

The `Failover` method takes a boolean force parameter and is used to trigger a
failover. It is only available on the passive BMC, and is available on Redfish.
It's only on the passive BMC because the active BMC may be dead at the time. The
force parameter can be used to have the passive BMC still failover in cases
where it usually wouldn't.

#### Failovers

The flow of a failover looks like:

1. Set `RedundancyEnabled` to false.
1. Set `FailoverInProgress` to true.
1. Block BMC reboots.
1. Put the previous active BMC in reset.
1. Update the new role on D-Bus.
1. Start obmc-bmc-active.target to start all active services.
1. Release the reset to let the BMC reboot and come back up as passive.(+)
1. Clear `FailoverInProgress`.
1. Unblock BMC reboots.
1. Wait for the BMC to reach BMC ready and start its heartbeat.
1. Do a full file sync and start background syncing. (separate design doc)
1. Set `RedundancyEnabled` to true.

(+) Forcing the BMC to come back up as the passive requires a new property on
the inter-BMC communication object such as `PassiveRequested`. This would be the
one writeable property so far that the BMC would write on the `sibling_bmc`
object. When it comes up and sees that it will assume the passive role and then
clear the property.

The case where the BMC never comes fully back would need to be handled
appropriately. In that case, redundancy would stay disabled.

#### Failover Causes

For the most part, this design leaves triggering failovers to a Redfish client
or the host through its interface. If there is a case where the BMC needs to
trigger a failover, it would use the Failover method on the passive BMC.

#### Redfish and the Host Interface

The following items need to be exposed on Redfish:

- Role
- RedundancyEnabled
- FailoverInProgress
- Start Failover

The [Manager schema][2] currently only contains a `ForceFailover(NewManager)`
action. We will need to work with the DMTF to get our needs added into the
schema.

The host interface is similar. IBM plans to use PLDM on the active BMC but
something closer to the KCS layer on the passive.

## Alternatives Considered

### Corosync/Pacemaker

This was investigated, but is more geared toward clustering scenarios,
especially in our use case where the BMCs don't automatically failover
themselves. It would also take a lot of custom code to do things like determine
which BMC should be active.

## Impacts

There is no impact to non-redundant BMC systems as none of this will be
installed.

On redundant BMC systems, this introduces a new BMC behavior of a BMC running in
passive mode, with a reduced set of services running.

There are new redundancy related Redfish and host interfaces available that
those clients will need to use if they are involved.

Code updates will have to follow a specific flow, where first the passive is
updated and rebooted, and then the active is updated and rebooted, and then
redundancy can be enabled again after the code levels match.

Any impacts relating to file syncing will be covered in that design.

### Organizational

This design proposes the new RBMC management daemon reside in the
phosphor-state-manager repository as it is related to state management. It can
be wrapped in a compile flag so it is only installed when desired.

The systemd service override files and the new active target files can be stored
along with this code. The list of services needing the overrides and any linking
of service files into the new targets may need to be done from a machine layer
as not all systems use every possible service.

The inter-BMC communication daemon will be in a repository specific to the
transport mechanism. In IBM's case it's proposed to be a new daemon in the
openpower-proc-control repo as it will be interfacing with IBM specific devices.

## Testing

Unit tests will be written to test the business logic of the new application,
using mocks to handle mimicking the sibling BMC and any other interfaces in
various situations.

Robot tests will be written to run against real and simulated hardware for
testing the behavior of code running on both BMCs at the same time.

[1]:
  https://lore.kernel.org/openbmc/E5183DEB-8B54-45AF-BE0F-6D470937B73D@gmail.com/
[2]: https://redfish.dmtf.org/schemas/v1/Manager.v1_19_0.json
