# BMC Reset with Host Booted

Author: Andrew Geissler (geissonator)

Other contributors:

Created: June 23rd, 2020

## Problem Description

BMCs can reboot for a lot of different reasons. User request, firmware update,
and a variety of different error scenarios. When the BMC is rebooted while the
host is up and running, there needs to be a process by which the two synchronize
with each other and the BMC gets itself into a state that matches with the host.

## Background and References

A good portion of this is explained in the phosphor-state-manager [README][1].

This design doc is written to formalize the design and add some more details on
dealing with both IPMI and PLDM communication to the host as well as desired
behavior when unable to talk with the host.

The high level flow is that OpenBMC software first checks for pgood, and if set,
it then checks the host to see if it is up and alive. If the power is on and the
host is running, then files are created in the filesystem to indicate this:

- /run/openbmc/chassis@0-on
- /run/openbmc/host@0-on

It should be noted that although full support is not in place for multi-chassis
and multi-host systems, the framework is there to build on.
`op-reset-chassis-running@.service` is a templated service, checking pgood in
its instances power domain. It creates a file in the filesystem,
/run/openbmc/chassis@%i-on, to indicate power is on for that instance. Similar
implementation is done for the host via `phosphor-reset-host-check@.service` and
the file /run/openbmc/host@%i-on.

If chassis power is on and the host is up, then `obmc-chassis-poweron@.target`
and `obmc-host-start@.target` are started.

The /run/ files are used by OpenBMC services to determine if they need to run or
not when the chassis and host targets are started. For example, if the chassis
is already powered on and the host is running, there is no need to actually turn
power on, or start the host. The behavior wanted is that these services "start"
but do nothing. That is commonly done within a systemd service file via the
following:

- `ConditionPathExists=!/run/openbmc/chassis@%i-on`
- `ConditionPathExists=!/run/openbmc/host@%i-on`

This allows the targets to start and for the BMC to get in synch with the state
of the chassis and host without any special software checks.

Different systems have different requirements on what the behavior should be
when the chassis power is on, but the host is unreachable. This design needs to
allow this customization. For example, some systems would prefer to just leave
the system in whatever state it is in and let the user correct things. Some
systems want to recover automatically (i.e. reboot the host) for the user. Some
systems have a hybrid approach where depending on where the host was in its
boot, the BMC may leave it or recover.

## Requirements

- Support both IPMI and PLDM as mechanisms to determine if the host is running.
  - Allow either or both to be enabled
- Support custom behavior when chassis power is on but the BMC is unable to
  communicate with the host
- Both IPMI and PLDM stacks will give the host a set amount of time to respond.
  Lack of response within this time limit will result in the BMC potentially
  taking recovery actions.
  - This time limit must be configurable at build time
- IPMI and PLDM will implement a phosphor-dbus-interface interface,
  `xyz.openbmc_project.Condition.HostFirmware`, which will have a
  `CurrentFirmwareCondition` property which other applications can read to
  determine if the host is running.

### IPMI Detailed Requirements

- IPMI will continue to utilize the SMS_ATN command to indicate to the host that
  a "Get Message Flags Command" is requested. Upon the host issuing that
  command, it will be considered up and running

### PLDM Detailed Requirements

- PLDM will utilize a GetTID command to the host to determine if it is running
- Where applicable, PLDM will provide a mechanism to distinguish between
  different host firmware stacks
  - For example, on IBM systems there is a difference between the hostboot (host
    initialization) firmware and Hypervisor firmware stacks. Both are host
    firmware and talking PLDM but the BMC recovery paths will differ based on
    which is running. The `CurrentFirmwareCondition` property should not return
    "Running" unless the Hypervisor firmware is running.

## Proposed Design

High Level Flow:

- Check pgood
- Call mapper for all implementations of
  `xyz.openbmc_project.Condition.HostFirmware` PDI interface
- Read `CurrentFirmwareCondition` property of all interface. If any call returns
  that a host is running then create file and start host target.
- Otherwise, check host via any custom mechanisms
- Execute automated recovery of host if desired

IPMI and PLDM software will be started as applicable. A combination of systemd
services and applications within phosphor-state-manager will coordinate the
checking of pgood, and if set, request the IPMI and PLDM applications to
discover if the host is running. Based on the response from these queries the
software in phosphor-state-manager will take the appropriate action of creating
the /run files and starting the chassis and host targets or entering into
recovery of the host.

The systemd targets responsible for this and any common services will be hosted
within phosphor-state-manager. Any system or company specific services can be
installed in the common targets:

- obmc-chassis-powerreset@.target.require
- obmc-host-reset@.target.requires

### Automated Recovery when host does not respond

A separate service and application will be created within phosphor-state-manager
to execute the following logic in situations where chassis power is on but the
host has failed to respond to any of the different mechanisms to communicate
with it:

- If chassis power on (/run/openbmc/chassis@%i-on)
- And host is off (!ConditionPathExists=!/run/openbmc/host@%i-on)
- And restored BootProgress is not None
- Then (assume host was booting before BMC reboot)
  - Log error indicating situation
  - Move host to Quiesce and allow automated recovery to kick in

### Note on custom mechanism for IBM systems

IBM systems will utilize a processor CFAM register. The specific register is
**Mailbox scratch register 12**.

If the chassis power is on but the BMC is unable to communicate with the host
via IPMI or PLDM, then the BMC will read this processor CFAM register.

The Host code will write `0xA5000001` to this register to indicate when it has
reached a state in which it can boot an operating system without needing the
BMC. If the BMC sees this value written in the CFAM register, then it will leave
the host as-is, even if it is unable to communicate with the host over IPMI or
PLDM. It will log an error indicating it was unable to communicate with the host
but it will also show the host state as `Running`.

If the register is not `0xA5000001`, then the BMC will follow whatever recovery
mechanisms are defined for when the host has a failure (most likely a reboot of
the host).

It is the responsibility of the host firmware to set this register as applicable
during the boot of the system. Host firmware will clear this register in
shutdown scenarios. To handle different host crash scenarios, the register will
also be cleared by the BMC firmware on power off's, system checkstops, and
during Memory Preserving reboots.

## Alternatives Considered

One thought was to avoid IPMI/PLDM all together and only use a "lowest common
denominator" hardware register of some sort. The problem with that is you start
creating your own protocol, and before you know it you have something like IPMI
or PLDM anyway, except you created something custom. So 99% of the time the IPMI
or PLDM path will be fine, and as a backup option, system owners can put their
own custom host-detection applications in.

## Impacts

None

## Testing

The normal path of IPMI and PLDM will be simple to test. Boot your system and
reboot the BMC. Verify the BMC chassis and host states are correct and verify
the host continued to run without issue throughout the BMC reset.

The more complicated tests will be error paths:

- Reboot the BMC while the host is booting, but before it's in a state where it
  can continue to run without the BMC. Verify the BMC detects this error
  scenario once it comes back from its reboot and takes the proper recovery
  actions for the host.
- Reboot the BMC when the host is up and running but disable the IPMI/PLDM stack
  on the host so it does not respond to the BMC when it comes back from its
  reboot. Ensure the BMC follows the defined recovery for the system in this
  situation.

[1]:
  https://github.com/openbmc/phosphor-state-manager#bmc-reset-with-host-andor-chassis-on
