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

This direction was first proposed on the [mailing list][].

## Background and References

A block diagram of the first two chassis of the system is shown below. This is a
sample IBM reference implementation.

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
   │   │  │ASIC  ├──┼───┼──┘    └──┼───┼──┤ASIC  │  │   │
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
2. FSI cables. FSI stands for `FRU Support Interface` where FRU stands for
   `Field Replaceable Unit`. This is a point to point two wire bus which the
   AST2600 and AST2700 have masters for. FSI is also used by the BMC to
   communicate with IBM's Power processors.

The `Comm ASIC` is an IBM device that contains a data area that both BMCs can
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
it to communicate with other entities, which includes the sibling BMC. The
details of this process are outside the scope of this document.

## Requirements

The high level requirements this design must meet are:

- There must be a way for the BMCs to agree on which will be active, and which
  will be passive.

- An IBM specific requirement is that an active BMC still needs to be chosen and
  the system still needs to be able to boot if there is a problem with the
  network connection between the BMCs. This leads to the FSI connections being
  used for role negotiation. The BMCs will not be redundant in this case. It is
  not possible with the current HW design to have redundant network connections.

- There are conditions that would exclude a BMC from being chosen as the active
  BMC. These are:

  - It hasn't been provisioned yet.
  - It cannot read the system serial number.
  - It can read the system serial number, but the value doesn't match its value
    cached on the BMC card. This means the BMC has been moved from another
    system and will need to be provisioned again.
  - There are plugging, cabling, or other topology errors that would prevent
    communication with the rest of the system. These would need to be resolved
    by service personnel first.

- As only a single BMC may access chassis hardware (EEPROMs, sensors, etc) at a
  time in IBM's configuration, there must be a way for the passive BMC to not
  run any services which would try to access this hardware.

- Both BMCs must report their role via Redfish. The active BMC must also report
  if redundancy is enabled. The BMC driving the failover must also report that a
  failover is in progress while it transitions to becoming active.

- The host must be able to obtain the role from each BMC, and the failover in
  progress indication from the BMC driving the failover.

- There must be an API that can be called from both Redfish and the PCIe link to
  the host to start a failover. This must be provided on the passive BMC in case
  the active BMC is dead at the time.

- On the failover, the original passive BMC must be able to take over the active
  role without requiring a reboot. There is no such requirement for when the
  active becomes passive.

- There are certain requirements that need to be met for failovers to be
  enabled. This includes things like:

  - Code versions must match.
  - The passive BMC must be at the BMC ready state.
  - RedundancyEnabled must be true

- There must be a way to mirror files, directories, certificates, accounts,
  possibly time, etc, from the active BMC to the passive so they are available
  to the passive BMC if a failover is required.

  - The [data sync design doc][] will cover the design of this.

- Service files must not be directly modified to get them to behave differently
  in active vs passive situations, though drop-in config files can be used.

- There is no requirement for the passive BMC itself to detect a dead active BMC
  and initiate a failover, though support for it could be added in the future if
  someone else needs it.

- There is no requirement that a failover be done due to the active BMC
  rebooting. For example, if a BMC reboots due to an out-of-memory condition, it
  can remain active when it comes back. If the active BMC doesn't come back from
  the reboot, this design leaves the decision to do the failover to the host or
  management console.

## Proposed Design

### Active and Passive BMC Description

The active BMC is the fully functioning BMC. It will have all of its services
running and be the main point of contact for any external entities, providing
them a full view of the system. It also owns the hardware connections to the
rest of the system, if the system is wired that way.

On the other hand the passive BMC will not run services whose functionality
isn't required. This includes services that involve:

- Accessing common hardware, as the active BMC owns these paths.
- The full host communication stack. In IBM's case this is PLDM. There will
  still be a minimal host communication stack, which is described below.

When the passive BMC becomes active, these services will be started. The next
section describes how this will be accomplished. There will be a method for
different systems to select their own set of services they don't want to run.

The bmcweb application will still run on the passive. However, as it won't have
the full system hardware view a lot of its Redfish output will be empty. The
active BMC will use aggregation to retrieve the passive BMC's manager data.
There will also be a web UI for the passive BMC, and it too would provide a
minimal set of information about the passive BMC only.

Both BMCs will contain all D-Bus event logs. That way, if one BMC dies or is
replaced, the logs aren't lost.

Failover requests, when made by an external entity, will go to the passive BMC
and it will drive the failover flow.

As mentioned in the requirements section, there will be Redfish and host
interfaces to request failovers, which are described below.

### systemd Unit File Design

As not all services need to run on the passive BMC, there needs to be a way to
have services not start by default, and then only start at some point in the
future when the BMC assumes the active role.

This will be done with a new systemd target and a new drop in systemd config
file. These do not require modifying existing unit files, and will be installed
only for the target image from a single recipe.

To start with, a new target will be created: `obmc-bmc-active.target`.

This target will not be started as part of multi-user.target/BMC ready. This
means that initially neither BMC will be active, until something later tells one
to be.

This target looks like:

```sh
$ cat obmc-bmc-active.target
[Unit]
Description=Active BMC Target
BindsTo=phosphor-set-bmc-active-passive.service
```

So that the desired services don't start when they usually would as part of
multi-user.target, it can use ConditionPathExists along with the following
helper services to create a file when the active target is alive:

```sh
$ cat phosphor-set-bmc-active-passive.service
[Unit]
Description=Start/Stop the active target
PartOf=obmc-bmc-active.target

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStartPre=mkdir -p /run/openbmc/
ExecStart=/bin/touch /run/openbmc/active-bmc
ExecStop=/bin/rm -f /run/openbmc/active-bmc
```

This would be linked into the obmc-bmc-active.target.wants so that it runs when
the target starts and stops.

Now, an `active-bmc-only.conf` file can be installed for each service that isn't
required for the passive role:

```sh
$ cat active-bmc-only.conf
[Unit]
ConditionPathExists=/run/openbmc/active-bmc
After=phosphor-set-bmc-active-passive.service
PartOf=obmc-bmc-active.target
```

The `PartOf` directive ensures the service stops when the active target stops,
though this isn't technically necessary as the BMC will just be rebooted when
going from active to passive.

So that the service starts as part of the active target, the service also needs
to be linked into obmc-bmc-active.target.wants.

For example, the following shows this implemented for the phosphor-regulators
service:

```sh
$ ls obmc-bmc-active.target.wants/phosphor-regulators.service -l
obmc-bmc-active.target.wants/phosphor-regulators.service -> ../phosphor-regulators.service

$ ls phosphor-regulators.service.d/
active-bmc-only.conf
```

To summarize:

1. No service files will have to be modified directly to prevent them from
   running when the BMC is passive.
2. An 'active-bmc-only.conf' drop in file, as shown above, will be used to
   prevent the service file from starting if the obmc-bmc-active target isn't
   started.
3. The service file will also be linked into obmc-bmc-active.target.wants so
   that it will start when the active BMC target is started.
4. The new service and conf files will reside within phosphor-state-manager and
   be installed during a bitbake build if the redundant bmc machine feature is
   enabled.

The commit proposed [here][rbmc commit] accomplishes the above:

1. Provides a redundant-bmc.inc that can be included for a system to enable a
   machine feature to enable this redundant BMC functionality.
1. A recipe or bbappend that provides a service that is only for the active BMC
   would inherit the redundant-bmc bbclass and then specify the service by
   adding it to RBMC_ACTIVE_ONLY_SERVICES:${PN}.
1. When the machine feature is enabled the bbclass will add a post do_install
   function to:
   - Install the active-bmc-only.conf file mentioned above into the
     `<service>.d` subdirectory for that service.
   - Link that service into the obmc-bmc-active.target.wants directory so it
     starts when that target is started.

#### Applications not required to run on the passive BMC.

The following is the list of applications that aren't required on the passive
BMC. Some of them are system specific.

Hardware access related:

- phosphor-host-state-manager, phosphor-chassis-state-manager
  - No HW connection to be able to power on/off or detect power state.
- phosphor-scheduled-host-transition
  - No ability to schedule a power on.
- openpower-hwdiags/occ-control/clock-data-logger/hw-isolation
  - OpenPower apps that deal with hardware
- phosphor-fan-presence/monitor/control
  - No connection to fans.
- phosphor-regulators
  - No connection to regulators.
- phosphor-psu-monitor, phosphor-psu-code-manager
  - No connection to PSUs.
- buttons/button-handler
  - Only active BMC wired to power button.
- phosphor-led-manager
  - Only the active BMC can access all of the LEDs.

Note: The dbus-sensors apps can still run as entity-manager won't be able to
probe hardware that can't be reached.

Host communication related:

- pldmd: The passive BMC won't use the PLDM stack
- biosconfig-manager: Since no host communication, no BIOS mgr necessary.
  - Also, it has persistent data that would be synced over when data is updated
    on the active.
- hyp-network-manager: Used for host interaction, so not necessary.
  - Also, uses PLDM and bios mgr.

Unused functionality:

- telemetry
  - No telemetry from the passive

### Application Design

This design proposes a new application, tentatively named
`phosphor-redundant-bmc`, that runs on each BMC to handle determining roles,
redundancy D-Bus APIs, etc.

#### D-Bus Representation

The application needs to host a few of its own D-Bus properties for use by
others.

The proposed object path is `/xyz/openbmc_project/state/bmc0`, the same one used
by the BMC state daemon from phosphor-state-manager.

| Name                  | Prop or Method | BMC     | Interface                                     |
| --------------------- | -------------- | ------- | --------------------------------------------- |
| Role                  | Property       | Both    | xyz.openbmc_project.State.Redundancy.BMC      |
| RedundancyEnabled     | Property       | Both    | xyz.openbmc_project.State.Redundancy.BMC      |
| FailoverInProgress    | Property       | Both    | xyz.openbmc_project.State.Redundancy.BMC      |
| ManualFailoverDisable | Property       | Active  | xyz.openbmc_project.State.Redundancy.Disable  |
| Failover(force)       | Method         | Passive | xyz.openbmc_project.State.Redundancy.Failover |

The `Role` property reflects the current role and will also be available via
Redfish.

The `RedundancyEnabled` property is reported via Redfish to let external users
(the web UI and management console) know that the system has redundancy
operational. The passive BMC obtains this from the active BMC.

The `FailoverInProgress` property indicates a failover is currently in progress.
It only gets set to true on the original passive BMC, and stays true as the
passive becomes active until it completes.

The `ManualFailoverDisable` property is used to disabled failovers. It is
persistent and remains until disabled.

The `Failover` method takes a boolean force parameter and is used to trigger a
failover. It is only available on the passive BMC, and
[will be available on Redfish](#redfish-and-the-host-interface). It must be on
the passive BMC because the active BMC may be dead at the time. The force
parameter can be used to have the passive BMC still failover in cases where it
usually wouldn't.

#### Inter-BMC Communication

The new application requires a method to exchange information with the other
BMC.

This design proposes using a second application to handle the communication,
abstracting the low level details behind a D-Bus API which the RBMC application
makes use of.

To read from the sibling, the communication daemon will provide the D-Bus
interface `xyz.openbmc_project.State.BMC.Redundancy.Sibling` at
`/xyz/openbmc_project/state/bmc1` with a property for each value.

How the data is acquired is hidden in the daemon, but would involve
communication with the sibling. The RBMC state daemon would then read the
properties to get the sibling values, and under the covers the communication
daemon could return a buffered value, or possibly read the sibling at that time.
PropertiesChanged signals would be emitted to know when something changes.

If the sibling isn't there or doesn't respond, reading the properties would
return errors.

The following table shows the information the BMCs need from each other:

| Property          | Description        | Source Interface and App               |
| ----------------- | ------------------ | -------------------------------------- |
| Role              | Determined Role    | State.Redundancy.BMC (RBMC App)        |
| RedundancyEnabled | Redundancy enabled | State.Redundancy.BMC (RBMC App)        |
| Provisioned       | If provisioned     | TBD (TBD)                              |
| BMCState          | The BMC State      | State.BMC (State Mgr App)              |
| FWVersion         | Firmware version   | Software.Version (BMC Code Update App) |
| Heartbeat         | Heartbeat status   | Signal from State.Red.BMC (RBMC App)   |

The code has the following uses for these fields from the sibling:

- Role: So it can tell if there is already an active or passive BMC when it
  comes up.
- RedundancyEnabled: When the BMC is passive, it would have to reject a failover
  request if redundancy wasn't enabled.
- Provisioned: So it knows why network communication doesn't work.
- BMCState: If the passive BMC is Quiesced, then the active BMC knows that
  redundancy shouldn't be enabled.
- FWVersion: The active BMC will not enable redundancy if the code levels don't
  match.
- Heartbeat: So each BMC knows if the sibling is alive. The heartbeat itself
  could be a periodic D-Bus signal from phosphor-redundant-bmc. If the signal
  were emitted once per second, missing 5 seconds of heartbeat would be
  considered a problem. This number would be tweaked if necessary.

The daemon uses the single `Redundancy.Sibling` interface and not instances of
the source interfaces - `xyz.openbmc_project.State.BMC`,
`xyz.openbmc_project.Software.Version`, and
`xyz.openbmc_project.State.Redundancy.BMC` for the following reasons:

1. These interfaces also have other properties on them that aren't used, such as
   the BMC state interface has another 3 properties on it.
2. Now that there would be two instances of the interfaces, for example a BMC
   state interface for each BMC, there'd need to be a way tell which one is
   which, such as by using associations to link them to the inventory or
   checking for bmc0 vs bmc1 in the object path. Apps currently using those
   interfaces would then have the added complexity of dealing with that even
   though they don't ever care about another BMC. Not using the state interface
   avoids all of that.

IBM's communication implementation will make use of the FSI connected device
which is accessible to both BMCs. It's not using the network path so that
communication can still be done even before a BMC is provisioned (i.e.
authentication configured) or if there is an issue with the network cable.

However, another system could still use the network to communicate if it wanted
to, by creating a communication daemon that does so. As long as it uses the same
D-Bus APIs, there wouldn't be a change to the main RBMC application.

#### Startup and Discovery

On a POR, the RBMC app will first set its role on D-Bus to 'Undetermined',
unless there is a condition where it knows it will have to be passive in which
case it will just set that.

Then, assuming the sibling is detected via HW presence detection, the RBMC
application will use `/xyz/openbmc_project/state/bmc1` to wait for the sibling
BMC state to hit Ready and the heartbeat to start. If this doesn't happen within
5 minutes (using 5 as a starting point here), the BMC will choose itself to be
active and continue on without enabling redundancy.

In the case where the sibling is alive, it can then read the rest of the sibling
information from `/xyz/openbmc_project/state/bmc1` and use that along with its
own knowledge to determine its role. The position zero BMC will choose the
active role the first time through if there is nothing preventing that from
occurring. On subsequent reboots the BMCs would just maintain their previous
roles if nothing has changed.

Since one BMC can read the role of the other, it can just pick the opposite role
if it is already set. This would be the case when one BMC is rebooted, or if a
BMC comes up after the 5 minute timeout.

There could be a case where both BMCs are passive, such as if there are severe
cabling or plugging errors. These situations require outside intervention to
resolve so there needs to be proper event logs created.

After the active and passive roles have been established, there are other checks
that need to be done to see if redundancy can be enabled - i.e. a failover is
possible:

- If the passive BMC were prevented from being active by one of the conditions
  listed in the requirements section.
- The BMCs must have the same code levels.
- The network between the BMCs must be healthy and communication possible.
- The passive BMC cannot be at the Quiesced BMC state.
- [ManualFailoverDisable](#d-bus-representation) hasn't been set.

#### Becoming Active

After the roles have been determined, the active BMC can activate all of its
services. It will do this by starting the obmc-bmc-active.target to start up all
of the active only services, and then doing a full file sync and enabling
background syncing ([data sync design][data sync design doc]). It will also set
the 'RedundancyEnabled` property to indicate it is ready for failovers. If
redundancy couldn't be enabled, then the syncing and RedundancyEnabled assertion
would be skipped.

#### Failovers

The flow of a failover looks like:

1. Set `RedundancyEnabled` to false.
1. Set `FailoverInProgress` to true.
1. Block BMC reboots.
1. Put the previous active BMC in reset.
1. Update the new role on D-Bus.
1. Release the reset to let the BMC reboot and come back up as passive.(+)
1. Unblock BMC reboots.
1. Take control of the system device connections.
1. Probe for devices that are now accessible.
1. Start obmc-bmc-active.target to start all active services.
1. Clear `FailoverInProgress`.
1. Wait for the new passive BMC to reach BMC ready and start its heartbeat.
1. Do a full file sync and start background syncing.
1. Set `RedundancyEnabled` to true.

The case where the BMC never comes fully back would need to be handled
appropriately. In that case, redundancy would stay disabled.

#### Failover Causes

For the most part, this design leaves triggering failovers to a Redfish client
or the host through its interface. One case where the BMCs themselves will need
to start a failover is during a system boot when it can be discovered that the
passive BMC has a better view of the system than the active one. In this case,
the system will need to be powered off, have a failover triggered, and then the
boot can be started again from the new active BMC.

#### Redfish and the Host Interface

The following items need to be exposed on Redfish:

- Role
- RedundancyEnabled
- FailoverInProgress
- Start Failover

The [Manager schema][] currently only contains a `ForceFailover(NewManager)` action.
We will need to work with the DMTF to get our needs added into the schema.

The host interface is similar. IBM plans to use PLDM on the active BMC but
something closer to the KCS layer on the passive. The details on this are still
being worked and may require a standalone design document.

#### Changes to Other Applications

There are a few changes to existing applications required:

- bmcweb

  - Add support for the new redundancy related interfaces (failover/role/etc).

- webui-vue

  - Add redundancy related pages.
  - Possibly not show certain pages on the passive BMC.
  - When a BMC becomes active, throw away any cache of empty collections.
  - Ensure event logs are sorted by timestamp and not by ID so that they aren't
    just grouped by originating BMC. See phosphor-logging section below for
    additional details.

- phosphor-logging

  - Have a unique event log Id property range per BMC, so that there is no
    collision with logs created by the other BMC as the logs will be synced to
    each. This could be done by using the first byte in the uint32_t log ID as
    the BMC identifier. This would be a compile option. Logs are required to be
    synced so that if a BMC dies any logs it had created first can be found on
    the other. Deletes would also need to be synced, in both directions.

  - When logs are synced via their files, phosphor-log-manager will need to
    detect the new (or deleted) files and put them on D-Bus (or remove them) so
    both BMCs can always show all event logs.

- phosphor-inventory-manager

  PIM persists all of its D-Bus data in files under
  /var/lib/phosphor-inventory-manager so it can restore everything to D-Bus when
  it starts. Though these files will all be synced to the passive BMC, the
  passive BMC has no need to show the whole system inventory on D-Bus. Rather,
  it just needs to model its own BMC card and possibly the system object itself,
  which can both be read by the passive BMC.

  This can be accomplished by adding support to the sync daemon to be able to
  sync to a different directory than the path on the source, and then either:

  - Modify the code to use a different directory based on the role, and handle
    the case when the role changes to active during a failover.
  - Use a service file override to link the normal directory to a specific
    active or passive one based on the role before the application starts.
    Something would have to run at the time of the failover to redo the link and
    restart the service.

- phosphor-settings

  This application also stores its D-Bus objects in files that need to be synced
  from the active BMC to the passive while the application is also running on
  the passive. To deal with this, the code can be modified to place inotify
  watches on its files, and when they change it can read them and reload the
  D-Bus objects.

  Alternatively, the ability to restart an application after a sync could be
  added to the sync daemon, though then properties changed signals would be
  lost.

## Alternatives Considered

### Corosync/Pacemaker

This was investigated, but it is found to be more geared toward clustering
scenarios with more than 2 endpoints, and not as useful in the use case where
the BMCs don't automatically failover themselves. It would also take a lot of
new code to do things like determine which BMC should be active based on custom
rules. Other disadvantages are:

- The Pacemaker resource manager expects to create a resource for all required
  systemd services with its dependency services, it does not automatically
  detect and add it as part of the Pacemaker resource configuration.
- The BMC does not need to trigger a failover if any service is failed to start.
  By default, pacemaker just moves the failed service from one node to another
  node. It required a lot of configuration to prevent.

## Impacts

There are a [few](#changes-to-other-applications) applications that need
modification to support the redundancy requirements.

On non-redundant BMC systems, the new applications and systemd targets/overrides
won't be installed.

On redundant BMC systems, this introduces a new BMC behavior of a BMC running in
passive mode, with a reduced set of services running.

There are new redundancy related Redfish and host interfaces available that
those clients will need to use if they are involved.

### Organizational

This design proposes the new RBMC management daemon reside in the
phosphor-state-manager repository as it is related to state management. It will
be wrapped in a compile flag so it is only installed when desired.

The systemd service override files and the new active target files can be stored
along with this code. The list of services needing the overrides and any linking
of service files into the new targets [would be done][rbmc commit] from a
machine layer as not all systems use every possible service.

The inter-BMC communication daemon will be in a repository specific to the
transport mechanism. In IBM's case it's proposed to be a new daemon in the
openpower-proc-control repo as it will be interfacing with IBM specific devices.

## Testing

Unit tests will be written to test the business logic of the new application,
using mocks to handle mimicking the sibling BMC and any other interfaces in
various situations.

Robot tests will be written to run against real and simulated hardware for
testing the behavior of code running on both BMCs at the same time.

[mailing list]:
  https://lore.kernel.org/openbmc/E5183DEB-8B54-45AF-BE0F-6D470937B73D@gmail.com/
[Manager schema]: https://redfish.dmtf.org/schemas/v1/Manager.v1_19_0.json
[data sync design doc]: https://gerrit.openbmc.org/c/openbmc/docs/+/71039
[rbmc commit]: https://gerrit.openbmc.org/c/openbmc/openbmc/+/72556
