# Backup and Restore Configuration

Author: Adriana Kobylak < anoo! >

Primary assignee: Adriana Kobylak

Other contributors:

Created: 2019-02-11

## Problem Description
The ability to restore BMC configuration settings can be useful in cases such
as after a factory reset or BMC replacement, by making it faster to get the
system back to a previously known setting.

## Background and References
Other BMC firmware stacks provide this functionality, such as:

- AMI: [Megarac Backup and Restore Configuration](ftp://ftp.tyan.com/doc/Habanero_BMC_Configuration_Guide_v1.0_for_Channel.pdf)
- HP: [iLO Backup and Restore](https://github.com/HewlettPackard/ilo-rest-api-docs/blob/master/source/includes/_ilo5_backupandrestore.md)

## Requirements
- Ability to backup BMC configuration settings.
- Upon restore, the BMC configuration parameters are properly set to the values
specified by the provided backup file.
- BMC configuration settings must be able to be applied across application
versions. Application versions may include changes to Redfish schemas and
available parameters.
- When "old" configurations are applied to a newer firmware revision that
contains new parameters, new parameters will take on their default value.
- The backup and restore functionality must be accessible over multiple
interfaces such as Redfish and IPMI.
- BMC configuration must ensure that passwords and private keys are encrypted
with per-BMC encryption to avoid replication.

## Proposed Design
- Incorporate the Backup and Restore actions into the Redfish schema (via
extension, OEM, etc). The proposed schema could look like:

    Backup and Restore Manager:

         |_ Backup Action
            |_ Attributes:
              |_ Network : Supported
              |_ Rsyslog : Not supported
         |_ Restore Action

- The attributes property could list the backup items that are supported in
that system.
- Potential areas to propose to the DMTF include network, certificates, LDAP,
rsyslog, power management, boot settings, user accounts, etc.
- The backup function would gather the persistent files where the configuration
settings are stored and package it into a tarball.
- There is no explicit protection for sensitive information such as passwords,
the user is responsible for safeguarding the backup data.
- The restore function would take the tarball with the persistent files that
store the configuration settings, and would either restart any necessary services
for the changes to take effect.
- Include the BMC level in the backup file, so that the user can be warned
about a minimum BMC level that the system should be running. But if the
operation is performed on an older level that is missing some of the properties,
the application would just error out when it fails to restore.

## Alternatives Considered
N/A

## Impacts
Development would be required to incorporate the new APIs into Redfish and
implement them in OpenBMC.
DMTF will need to be engaged to standardize the Redfish interfaces.

## Testing
Tests can be added to the current test infrastructure since there are
existing frameworks that perform factory reset of the BMC and restore
certain settings.
