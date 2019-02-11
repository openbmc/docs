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
- Use Redfish APIs to backup specified BMC configuration settings, which
would create a file that can later be used as input to restore the
configuration settings back.

## Proposed Design
- Incorporate the Backup and Restore actions into the Redfish schema (via
extension, OEM, etc).
- Initially target the following settings to be backed up:
  -  Network
  -  Rsyslog
  -  User and authentication, including certificates
- Create an application that gathers the required D-Bus properties and their
values into JSON format. For example, backing up the network configuration
would store the values for the ip property, gateway, etc.
- In addition to D-Bus properties, the application can gather file names and
their content, such as the shadow file for user configuration. Sensitive
information such as passwords are assumed to not be stored in clear text
in OpenBMC, and therefore are acceptable for the file data to be retrieved
out of the BMC.
- If multiple settings are selected, the data would be concatenated into
a single JSON file.
- The same application would accept the JSON data to set the property values
for the restore implementation.
- Include the BMC level in the backup file(s), so that the user can be warned
about a minimum BMC level that the system should be running. But if the
operation is performed on an older level that is missing some of the properties,
the application would just error out when it fails to restore.
- The OpenBMC Web UI would provide the option to choose multiple
configuration settings to backup / restore at once.
- A BMC reboot would be issued after a restore operation.

## Alternatives Considered
The backup file can be written in any format, but having a standard format
like JSON that is supported by Redfish and does not require additional
parsers in OpenBMC seems like an optimal choice.

## Impacts
Development would be required to incorporate the new APIs into Redfish and
implement them in OpenBMC.

## Testing
Tests can be added to the current test infrastructure since there are
existing frameworks that perform factory reset of the BMC and restore
certain settings.
