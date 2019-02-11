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
- Ability to provide the API user with a backup file containing the BMC
configuration settings.
- Upon restore, the BMC configuration parameters are properly set to the values
specified by the provided backup file.
- BMC configuration settings must be able to be applied across application
versions. Application versions may include changes to Redfish schemas and
available parameters.
- When "old" configurations are applied to a newer firmware revision that
contains new parameters, new parameters will take on their default value.
- The backup and restore functionality must be accessible over multiple
interfaces such as Redfish and IPMI.
- The new Backup and Restore API requires that the Redfish session is
running under an admin role in order to ensure the data being retrieved
or uploaded is coming from a secure source.

## Proposed Design
- Propose the addition of a Backup and Restore function to the Redfish schema.
The proposed schema would make use of GET/PUT to backup/restore items by
using existing Redfish interfaces to get the JSON data. Ex:

```
https://${bmc}/redfish/v1/BackupRestoreService/
{
  "@odata.context": "/redfish/v1/$metadata#BackupCollection.BackupCollection",
  "@odata.id": "/redfish/v1/BackupRestoreService",
  "@odata.type": "#BackupCollection.BackupCollection",
  "Members": [
    {
      "@odata.id": "/redfish/v1/Managers/bmc/EthernetInterfaces/"
    },
    {
      "@odata.id": "/redfish/v1/Managers/1/NetworkProtocol/HTTPS/Certificates"
    },
  ],
  "Members@odata.count": 2,
  "Name": "Backup Service Collection"
```
A GET on the first member (EthernetInterfaces) would return the JSON file with
all the properties these interface supports. A PUT would then set those
properties to the values provided by the backup JSON file.

- Areas to propose to the DMTF include network, certificates, LDAP, rsyslog,
power management, boot settings, user accounts, etc.
- There is no explicit protection for sensitive information such as passwords,
the user is responsible for safeguarding the backup data.
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
certain settings. Minimum test scenarios:

1. Basic use case: configure settings, backup, factory reset, then restore
the BMC configurations, and validate the settings are correct.
2. Upgrade: Release-to-release migration: starting from a system configured
on release N, backup, install release N+1, and restore, then validate
settings are expected (given that the APIs may have changed between
releases).
3. Downgrade: Same as scenario 2), but going backwards: N -> N-1.
