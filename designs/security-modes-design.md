# Security modes - OpenBMC

Author: Richard Marian Thomaiyar (rthomaiy123)

Primary assignee: Richard Marian Thomaiyar

Created: May 6th, 2019

## Problem Description
Host interfaces in BMC are all unauthenticated and by default trusts the Host
executed commands. This raises security risks as operating system doesn't
restrict kernel / driver access to the physical host interface. E.g. System
interfaces like KCS, BT, SMIC are session-less and can execute all the
administrative privilege command. Apart from this, OpenBMC has features which
can be executed only in special cases. E.g. Manufacturing commands
or validation commands, must be executed in certain environments and
not in-field.

## Background and References
Attacking BMC from host system, and executing functions, which are restricted
for manufacturing or validation cases are one of the security issues, which
BMC faces. There have been several recommendations to block / minimize the
commands executed through unauthenticated Host interfaces (KCS, SMIC, BT etc.)
Ref - https://docs.oracle.com/cd/E24707_01/html/E37451/z4002aeb1391741.html
and to reduce the attack vector as general security guideline.

## Requirements
Security of BMC over unauthenticated Host interface must be improved. The best
approach for the same is to block the unauthenticated Host interfaces, which is
not practical in nature (as there are scenarios where Host OS agent requires
access to BMC). Hence options must be provided in such a way that
it can be configured as desired and depending on the user needs.
1. Provide options such that OpenBMC Host interface communication can be
restricted as per the need of the end-user.
2. Provide options such that intrusive features are executed in certain
scenarios and not in field (like Manufacturing mode commands, validation
based commands etc.)

## Proposed Design
### Restriction Mode - To limit commands executed through Host interfaces.
`RestrictionMode` property in interface
`xyz.openbmc_project.Control.Security.RestrictionMode` will be used to store
the security operational mode of the BMC. `Whitelist` & `Blacklist` modes are
already defined in the D-Bus interface. Following enumerations as defined will
be added to the interface
1. `Provisioning` - All commands are accepted through KCS interface in both pre
and post BIOS boot. This also indicates that user has not configured any
restriction and BMC are still in Deployment state (Configuration state)
(Note: `None` mode already defined in this D-Bus interface is not removed. It
can be used to differentiate between vendors who implement these modes or don't
implement the same at all.)
2. `ProvisionedHostWhitelist` - (Note: `Whitelist` mode already used is
different from these as it controls differently between Pre and Post BIOS
boot). All commands are executed till BIOS Post complete, after which commands
which are in whitelist configuration will only be executed.
3. `ProvisionedHostDisabled` - All Host interface commands are blocked after
BIOS POST complete.
All the above modes are persistent. Recommend storing the same in
u-boot environmental variable, so that these modes are preserved even when
reset to default action is performed. Recommend having IPMI OEM Command
through which `RestrictionMode` property can be updated to `Provisioning`,
`ProvisionedKCSWhitelist` and `ProvisionedKCSDisabled`.
There are further restrictions to the command which will be applied (like
Provisioning to `ProvisionedKCSWhitelist` can be updated through Host interface
whereas, it can't go back to `Provisioning` mode through Host interface
directly. These restrictions are not applied for LAN sessions).
### Special Mode - To limit feature execution on certain conditions
A new property `SpecialMode` in interface
`xyz.openbmc_project.Control.Security.SpecialMode` will be used to represent
special modes which must be non-persistent and restricted generally.
Following values are defined for the same.
1. `None` - System is not under any special mode. It is working in normal
condition as per the RestrictionMode only
2. `Manufacturing` - To indicate that BMC is in special mode, and can execute
manufacturing related commands. Implementer must make sure that this can be
set only on proper secure condition. (Current recommendation is to allow
the same only when `RestrictionMode` property is in `Provisioning` and with
further security condition as per need)
2. `ValidationUnsecure` - Special mode in which commands needed for validation
only will be executed (like Sensor override etc.). Implementer must make sure
this can be set only on proper secure condition. (Current recommendation is
to allow the same using Linux user id 0 `root` through SSH only using `busctl`
command.)

## Alternatives Considered
Currently there are no viable alternatives. There can be implementation
which can fully block host interface or never allow any special features
in the BMC image.

## Impacts
1. Requires defining new D-Bus interface as stated above.
2. Need to implement filtering logic mechanism which will make use of
these properties. Daemon to maintain these values also needs to be implemented.

## Testing
Testing can be done by setting the mode and executing the command accordingly
for both positive and negative cases. If required, we can have CI system which
does the same.
