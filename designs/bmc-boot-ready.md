# BMC Boot Ready

Author: Andrew Geissler (geissonator)

Other contributors:

Created: May 12, 2022

## Problem Description
There are services which run on the BMC which are required for the BIOS (host
firmware) to power on and boot the system. The goal of this design is to
provide a flexible design for system owners to easily define those requirements
and ensure a system power on and boot do not occur until the needed BMC
services have started.

For example, on some system, you can not power on the chassis until the VPD
has been collected from the VRM's by the BMC to determine their characterstics.
On other systems, the BIOS service is needed so the host firmware can look
for any overrides.

Currently, OpenBMC has a undefined behavior in this area. If a particular
BMC has a large time gap between when the webserver is available and when all
BMC services have completed running, there is a window there that a user could
request a power on via the webserver when not all needed services are running.

## Background and References

The mailing list discussion can be found [here][1]. The BMC currently has
three major [state][2] management objects in a system. The BMC, Chassis, and
Host. Within each state object, the current state and requested state are
tracked.

The [BMC][3] state object is considered `Ready` once the systemd
`multi-user.target` has successfully started all if it's services. A simple
solution to this problem would be to say you can't power on the Chassis or
boot the Host firmware until the BMC is `Ready`. The issue with this is that
there are a lot of services that run in the multi-user.target which are not
needed to power on or boot. This would be wasted time where the BMC and Host
firmware could be booting in parallel.

[1]: https://lists.ozlabs.org/pipermail/openbmc/2022-April/030175.html
[2]: https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/yaml/xyz/openbmc_project/State
[3]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/BMC.interface.yaml

## Requirements

- Provide a mechanism for a system owner to define specific BMC services
  required to power on the chassis and start the firmware boot
  - Reject power on and boot requests until needed BMC services have succesfully
    started
- Provide a specific error code to the caller when rejecting a request due
  to above requirements
- If new mechanism is not utilized then default to waiting for BMC `Ready` to
  allow the power on and boot of a system

## Proposed Design

Define a new BMC state, `BootReady` which indicates all needed servics have
started and system can be powered on and booted. This would be a state that's
between `NotReady` and `Ready`.

A new synchronization taret, obmc-bmc-boot-ready.target, would be defined and
services required to power on and boot would have this:
```
Requires=obmc-bmc-boot-ready.target
Before=obmc-bmc-boot-ready.target
```

If there is common agreement that a phosphor service is needed for `BootReady`
then the dependency can be added directly, otherwise an override can be placed
in the systems meta-* layer.

The BMC state object would monitor for this target to complete and set its
state to `BootReady` once it sees the target complete. As usual, the BMC would
set its state to `Ready` once it sees multi-user.target complete.

If a power on or boot request is made to the Chassis or Host state objects and
the BMC is not at `BootReady` or `Ready` then an error will be returned and
the request rejected.

## Alternatives Considered
Instead of an error being returned, just queue up the request and process it
once the command can be issued. This introduces a lot of complexity on what
to do if the BMC never reaches `BootReady` or `Ready` and what to do with
other requests that come in and need to be queued as well.

Another option is to code the dependencies directly into the services. For
example, if the power on service requires the vrm vpd collection service,
encode that dependency in the systemd files. Having a level of absraction
via a systemd target and clear errors returned when not in the correct state
seems more developer and user friendly.

## Impacts

Callers of the D-Bus API to power on and boot the system will potentially
need to handle a new error code. This includes the BMC webserver and the
Redfish clients utilizing it. The error returned to Redfish will be a
503 (Service Unavailable).

### Organizational
This function will be implemented within the existing phosphor-state-manager
repository. x86-power-control, an alternative to phosphor-state-manager, can
could also implement this logic.

## Testing
- Ensure the proper error is returned when the BMC is `NotReady` and a power on
  or boot operation is requested.
  - Ensure the Redfish API returnes a 503.
- Ensure system boots as expected in BMC `BootReady` or `Ready` states
