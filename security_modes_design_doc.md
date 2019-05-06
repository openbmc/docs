# Security modes - OpenBMC

## Scope
This document covers the interface, and implementation design details needed for
security modes when BMC firmware is in regular operational state

### Design implementation
`RestrictionMode` property in interface
`xyz.openbmc_project.Control.Security.RestrictionMode` will be used to store
the security operational mode of the BMC. `Whitelist` & `Blacklist` modes are
already defined in the D-Bus interface. Few more enumerations as defined will
be added to the interface
1. `Provisioning` - All commands are accepted through KCS interface in both pre
and post BIOS boot. This also indicates that user has not configured any
restriction and BMC is still in Deployment state (Configuration state)
(Note: `None` mode already defined in the interface is not used here, this is
to keep difference between vendors who implement these modes or don't implement
the same at all.)
2. `ProvisionedKCSWhitelist` - (Note: `Whitelist` mode already used is
different from these as it controls differently between Pre and Post BIOS
boot).
3. `ProvisionedKCSDisabled` - All KCS interface commands are blocked after
BIOS POST complete.
All the above modes are persistent. At this point of time, IPMI OEM Command
handler will be provided using which `RestrictionMode` property can be updated
to `Provisioning`, `ProvisionedKCSWhitelist` and `ProvisionedKCSDisabled`.
There are further restrictions to the command which will be applied (like
Provisioning to `ProvisionedKCSWhitelist` can be updated through KCS interface
whereas, it can't go back to `Provisioning` mode through KCS interface directly.
These restrictions are not applied for LAN sessions).

A new property `SpecialMode`  in interface
`xyz.openbmc_project.Control.Security.SpecialMode` will be used to represent
further special modes which must be non-persistent and restricted generally.
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
to allow the same using linux user id 0 `root` through SSH only using `busctl`
command.)
