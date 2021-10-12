# In-Band Update of BMC Firmware using USB

Author: George Liu <liuxiwei!>

Primary assignee: George Liu

Created: 2021-10-12

## Problem Description

The firmware update cannot be completed when the OS(operating system) does not
boot or the system does not power on because of a bug, or perhaps the network
is failing so cannt do updates through redfish.
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
service (eg: USBCodeUpdate@sda1.service).

The `USBCodeUpdate` service will traverse and obtain the first file with a `.tar`
extension according to the incoming USB path (/run/media/usb/sda1) and copy it to
`/tmp/images/`, and the [phosphor-software-manager](https://github.com/openbmc/phosphor-bmc-code-mgmt)
will start to extract the image and verify this file.

Once the verification is successful, the process needs to do the following:
1. Get the property value of Activation whether it meets the conditions for upgrading.
2. If so, We should set the ApplyTime property to OnReset so that the proposed usb code update
   app does not reboot the BMC after activation, this gives the user an opportunity to remove
   the key then reset the BMC. This is to avoid unintentional automatic code updates if
   the bmc were to be rebooted with the usb key still plugged in.
3. Set the RequestedActivation property to Active to start updating the firmware.
4. Exit the phosphor-usb-code-update daemon.

## Proposed Design

The new code would be part of the phosphor-software-manager repository.
The design process is as follows:
 - Define a macro switch (`usb-code-update`) in phosphor-software-manager
repository to identify whether to enable the USB Code Update function,
which is disabled by default.
 - If `usb-code-update` enabled, install the udev rules file to
`/lib/udev/rules.d` during compilation.
 - Once the udev rules are met, call the shell script and trigger the systemd service.
 - This service verifies the `/run/media/usb/sda1` directory and copies
the first `.tar` file in the directory to `/tmp/images` and starts verification.
 - Set ApplyTime to OnReset so that the proposed usb code update app does not reboot
the BMC after activation.
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
SUBSYSTEM=="block", KERNEL=="sda1", RUN+="/usr/bin/usb_update.sh sda1"
SUBSYSTEM=="block", KERNEL=="sdb1", RUN+="/usr/bin/usb_update.sh sdb1"
```

## Testing

 - When the USB code update is disabled, the service will return directly without any update.
 - Manually insert the USB key with the firmware upgrade package, and check whether
the upgrade file is correct through the log.
 - Verify that the ApplyTime attribute value is set to OnRest.
 - Verify that the RequestedActivation property value is set to Active.
 - Verify that the firmware update was successful.
