# Preservation Whitelist Configuration Daemon

Author:
  Ryon Heichelbech <ryonh@ami.com>

Primary assignee:
  Ryon Heichelbech <ryonh@ami.com>

Created:
  September 21, 2021

## Problem Description
A background daemon is needed to manage the list of files to be preserved when
flashing a BMC upgrade image. The file that defines these files is currently
stored in volatile memory, meaning a daemon capable of preserving its state in
nonvolatile memory is required in order to preserve modifications to this file
across BMC reboots. Additionally, it allows us to expose a D-Bus API for IPC,
consistent with the rest of the Phosphor ecosystem.

## Background and References
Currently, firmware upgrade file preservation is handled by the
`obmc-update.sh` script. The files and folders that are to be preserved are
defined in a plaintext file defined in `/run/initramfs/whitelist`. Upon
flashing an update image, the obmc-update script will copy these files to RAM,
write the new image, and then restore these files to the filesystem. As this
file is stored in the initramfs (loaded into RAM), any modifications will not
be preserved across BMC reboots.

## Requirements
The daemon is to provide an abstraction over the list of files preserved in
`/run/initramfs/whitelist`, grouping them into useful configuration settings
(eg `Users`, `LDAP`, or `Network`). It will then expose a D-Bus API to allow
the configuration of these settings while maintaining a persistence file so
that these settings will be preserved across BMC reboots.

## Proposed Design
The daemon will provide a `xyz.openbmc_project.Software.Preserve` service. This
service will expose a `/xyz/openbmc_project/software/preserve` object,
implementing a `xyz.openbmc_project.Software.Preserve` interface, which
containing the configurable items as boolean properties. 

Examples of configuration settings to be implemented and their corresponding
preserved files include:

 - `User`:
   - `/etc/group`
   - `/etc/gshadow`
   - `/etc/passwd`
   - `/etc/shadow`
   - `/var/lib/ipmi`
   - `/etc/ipmi_pass`
 - `Hostname`:
   - `/etc/hostname`
   - `/etc/machine-info`
 - `IPMI`:
   - `/var/lib/phosphor-settings-manager`
 - `LDAP`:
   - `/var/lib/phosphor-ldap-conf`
   - `/var/lib/phosphor-ldap-mapper`
   - `/etc/nslcd`
   - `/etc/nslcd.conf`
 - `Certs`:
   - `/etc/ssl/certs`
 - `Network`:
   - `/etc/systemd/network`
   - `/etc/resolv.conf`
 
The configuration for these settings at any given time would be serialized and
stored under `/var/lib` in order to persist across reboots. When the daemon
starts, it will deserialize the settings and sync its state to the
`/run/initramfs/whitelist` file. When a BMC update is performed, the existing
obmc-update script implementation will handle preserving the files.

Busctl examples:

 - Preserve user info:
`busctl set-property xyz.openbmc_project.Software.Preserve /xyz/openbmc_project/software/preserve xyz.openbmc_project.Software.Preserve User b true`
 - Do not preserve user info:
`busctl set-property xyz.openbmc_project.Software.Preserve /xyz/openbmc_project/software/preserve xyz.openbmc_project.Software.Preserve User b false`

## Impacts
- When new features are added with persistent configurations, this
  implementation may need to be updated to be aware of them.
- Additional care must be taken with unversioned persistence files and when
  bumping the version on versioned persistence files (such as those created
  with cereal) in order to avoid unexpected states after upgrading the BMC.

## Testing
This is tested by issuing busctl calls (as described in proposed design) and
verifying that the `/run/initramfs/whitelist` file is maintained accordingly.
