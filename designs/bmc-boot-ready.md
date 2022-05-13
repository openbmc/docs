# BMC Boot Ready

Author: Andrew Geissler (geissonator)

Other contributors:

Created: May 12, 2022

## Problem Description
There are services which run on the BMC which are required for the BIOS (host
firmware) to power on and boot the system. The goal of this design is to
define a mechanism to ensure these dependencies are met before a power on
or boot is started.

For example, on some system, you can not power on the chassis until the VPD
has been collected from the VRM's by the BMC to determine their characteristics.
On other systems, the BIOS service is needed so the host firmware can look
for any overrides.

Currently, OpenBMC has an undefined behavior in this area. If a particular
BMC has a large time gap between when the webserver is available and when all
BMC services have completed running, there is a window there that a user could
request a power on via the webserver when not all needed services are running.

## Background and References

The mailing list discussion can be found [here][1]. The BMC currently has
three major [state][2] management interfaces in a system. The BMC, Chassis, and
Host. Within each state interface, the current state and requested state are
tracked.

The [BMC][3] state object is considered `Ready` once the systemd
`multi-user.target` has successfully started all if its services.

There are three options that have been discussed to solve this issue:
1. D-Bus objects don't exist until the backend is prepared to handle them.
2. If a user tries something that system is not in proper state to handle then
   return an error code.
3. If a user tries something that system is not in proper state to handle then
   queue it up.

Option 1 is challenging because D-Bus interfaces provided by OpenBMC state
applications have a mix of read-only properties (like current state) and
writeable properties that are used to request state changes. Not showing any
until everything is available could have unknown consequences. This also has
similar issues to option 2 in that applications and clients must have proper
code to handle missing interfaces.

Option 2 is challenging because Redfish clients and internal applications like
the op-panel code now need to properly handle error codes like this. You can
argue that they already should, but that is definitely not the case with a lot
of OpenBMC applications and clients.

Option 3 is the most user friendly option. No client or OpenBMC application
changes would be needed. One concern is that having a system somewhat randomly
power on at some later point in time could be unexpected. The general consensus
in this review though has been that this is the most preferred option.

[1]: https://lists.ozlabs.org/pipermail/openbmc/2022-April/030175.html
[2]: https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/yaml/xyz/openbmc_project/State
[3]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/BMC.interface.yaml

## Requirements

- Queue up chassis and host requested state changes until the BMC is in the
  proper state to allow the request
  - What the "proper state" is will be implementation specific but by default
    phosphor-state-manager will queue all requests until the BMC state has
    reached `Ready`

## Proposed Design

If a power on or boot request is made to the Chassis or Host state objects and
the BMC is not at `Ready` then the request will be queued and the state
management code will begin monitoring for BMC `Ready`. Once reached, the
requested operation will be automatically executed.

## Alternatives Considered
The "Background and References" section covered some alternative options
and the complexity behind them.

Another option is to code the dependencies directly into the services. For
example, if the power on service requires the vrm vpd collection service,
encode that dependency in the systemd files. This is easy to say but in practice
has been challenging. Some OpenBMC services have built in assumptions that
the multi-user.target and all of it's dependent services have completed prior
to a power on being started. The general consensus within IBM was that it's
much easier and safer to just have a global wait-for-bmc-ready function as
proposed in this design.

## Impacts

Users will need to understand that their request to power on the system may
be delayed by an undefined amount of time. In general, a BMC gets to Ready state
within a couple of minutes.

### Organizational
This function will be implemented within the existing phosphor-state-manager
repository. x86-power-control, an alternative to phosphor-state-manager, could
also implement this logic.

## Testing
- Ensure a power on request is properly queued and executed when it is made
  prior to the BMC being `Ready`.
