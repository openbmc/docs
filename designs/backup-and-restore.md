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

- AMI: [Megarac Backup and Restore Configuration]
(ftp://ftp.tyan.com/doc/Habanero_BMC_Configuration_Guide_v1.0_for_Channel.pdf)
- HP: [iLO Backup and Restore]
(https://github.com/HewlettPackard/ilo-rest-api-docs/blob/master/source/includes/_ilo5_backupandrestore.md)

## Requirements
- Ability to backup BMC configuration settings.
- Upon restore, the BMC configuration parameters are properly set to the values
specified by the provided backup file.
- BMC configuration settings must be able to be applied across application 
versions. Application versions may include changes to Redfish schemas and 
available parameters.
- When "old" configurations are applied to a newer firmware revision that 
contains new parameters, new parameters will take on their default value.
- The backup and restore functionality ust be accessible over multiple
interfaces such as Redfish and IPMI.
- BMC configuration must ensure that passwords and private keys are encrypted 
with per-BMC encryption to avoid replication.

## Proposed Design
- Incorporate the Backup and Restore actions into the Redfish schema (via
extension, OEM, etc). The proposed schema could look like:

    Backup Manager:

         |_ BackupAll action (Call all the below actions)
         |_ BackupA action (specific to the implementation, like network)
         |_ BackupB action
         |_ BackupN action

- Initially target the following settings to be backed up:
  -  Network
  -  Rsyslog
- Each target would implement a backup function that gathers the persistent
files where the configuration settings are stored into a tarball.
- Each target would implement a restore function that would take a tarball
with the persistent files that store the configuration settings, and would
either restart any necessary services for the changes to take effect, or
return to the caller a notification (such as a variable) that a BMC reboot
is needed to apply the settings.
- If multiple settings are selected, the files would be packaged into a
single tarball file.
- Include the BMC level in the backup file(s), so that the user can be warned
about a minimum BMC level that the system should be running. But if the
operation is performed on an older level that is missing some of the properties,
the application would just error out when it fails to restore.
- The OpenBMC Web UI would provide the option to choose multiple
configuration settings to backup / restore at once.

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
