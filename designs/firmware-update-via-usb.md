# In-Band Update of BMC Firmware using USB

Author: George Liu <liuxiwei!>

Primary assignee: George Liu

Created: 2021-10-12

## Problem Description

The firmware update cannot be completed when the OS(operating system) does not
boot or the system does not power on because of a bug, or perhaps the network
is failing so cannt do updates throught redfish.
Therefore, BMC needs this mechanism to update the firmware, There's a desire to
have firmware updates via USB authenticated like the out-of-band updates are.
so service support personnel can access the machine locally and update
the system to a working firmware level.

## Background and References

None.

## Requirements

There is a udev rules file to monitor the USB `hotplug` status. When the user
uses the `in-band` method to update the firmware through the USB key, and once
the udev rules are met, it will call a shell script and trigger the systemd
service (eg: USBCodeUpdate@${ID_FS_LABEL}.service).

The `USBCodeUpdate` service will traverse and obtain the first file with a `.tar`
extension according to the incoming USB path (ID_FS_LABEL) and copy it to
`/tmp/images/`, and the [phosphor-software-manager](https://github.com/openbmc/phosphor-bmc-code-mgmt) will start to unzip
and verify this file.

Once the verification is successful, set the RequestedActivation property to Active
to start updating the firmware.

## Proposed Design

The new code would be part of the phosphor-software-manager repository.
The design process is as follows:
 - Define a macro switch (fw-update-via-usb) in phosphor-software-manager
repository to identify whether to enable the USB Code Update function,
which is disabled by default.
 - If `fw-update-via-usb` enabled, install the udev rules file to
`/lib/udev/rules.d` during compilation.
 - Once the udev rules are met, call the shell script and trigger the systemd service.
 - This service verifies the `/media/${ID_FS_LABEL}` directory and copies
the first `.tar` file in the directory to `/tmp/images` and starts verification.
 - Set RequestedActivation to Active, follow the updated status, start to update
the firmware, and restart the BMC after completion.

## Disable USB Code Update

There is a requirement: When the USB code update is disabled through redfish,
the process cannot do any processing, and it returns directly and records the log.
When the process is started, first obtain the `Enabled` attribute of the corresponding
`xyz.openbmc_project.Control.Service.Attributes` interface under the [service-config-manager](https://github.com/openbmc/service-config-manager)
repo, if it is true, record the log and return directly, otherwise the process continues.

## Pseudocode

The udev rules files for example:
```
SUBSYSTEM=="block", KERNEL=="sda1", RUN+="/usr/bin/usb_update.sh %E{ID_FS_LABEL}"
SUBSYSTEM=="block", KERNEL=="sdb1", RUN+="/usr/bin/usb_update.sh %E{ID_FS_LABEL}"
```

## Testing

 - When the USB code update is disabled, the service will return directly without any update.
 - Manually insert the USB key with the firemware upgrade package, and check whether
the upgrade file is correct through the log.
 - Set the RequestedActivation of D-Bus to Active to verify whether the firmware
update is successful.
