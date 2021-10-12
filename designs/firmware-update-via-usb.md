# In-Band Update of BMC Firmware using USB

Author: George Liu <liuxiwei!>

Primary assignee: George Liu

Created: 2021-10-12

## Problem Description

When Redfish or scp cannot be used, BMC needs a new mechanism for images to get
into the machine through the USB driver of BMC to verify and complete the
firmware update.

## Background and References

The openbmc project currently has a [phosphor-software-manager][1] repository.
In order to perform an update, need first to bring the image into the BMC
directory (/tmp/images). However, only TFTP and HTTP are currently supported,
and USB is not yet supported.

The intent of this new application design is to enable the USB driver of BMC to
enter the new image into BMC.

## Requirements

Enable the USB driver and detect that the USB key has been inserted in a way,
and then immediately trigger systemd service.

This service will traverse and obtain the first file with a `.tar` extension
according to the incoming USB path (for example: `run/media/usb/sda1`) and copy
it to `/tmp/images/`, and the [phosphor-software-manager][1] will start to
extract the image and verify this file.

Once the verification is successful, the process needs to do the following:
1. Get the property value of Activation whether it meets the conditions for
   upgrading.
2. If so, We should set the ApplyTime property to OnReset so that the proposed
   usb code update app does not reboot the BMC after activation, this gives the
   user an opportunity to remove the key then reset the BMC. This is to avoid
   unintentional automatic code updates if the bmc were to be rebooted with the
   usb key still plugged in.
3. Set the RequestedActivation property to Active to start updating the firmware.
4. Exit the phosphor-usb-code-update daemon.

## Proposed Design

The new code would be part of the phosphor-software-manager repository.
The design process is as follows:
 - Define a macro switch (`usb-code-update`) in [phosphor-software-manager][1]
repository to identify whether to enable the USB Code Update function,
which is _enabled_ by default.
 - If `usb-code-update` enabled, install the udev rules file to
`/lib/udev/rules.d` during compilation.
 - Once the udev rules are met, the systemd service is directly triggered.
 - This service verifies the `/run/media/usb/sda1` directory and copies
the first `.tar` file in the directory to `/tmp/images` and starts verification.
 - Set ApplyTime to OnReset so that the proposed usb code update app does not reboot
the BMC after activation.
 - Set RequestedActivation to Active, follow the updated status, start to update
the firmware, and restart the BMC after completion.

## Pseudocode

The udev rules files for example:
```
SUBSYSTEM=="block", ACTION=="add", KERNEL=="sda1", ENV{SYSTEMD_WANTS}="usb-code-update@sda1", TAG+="systemd"
SUBSYSTEM=="block", ACTION=="add", KERNEL=="sdb1", ENV{SYSTEMD_WANTS}="usb-code-update@sdb1", TAG+="systemd"
```

## Security

 - We can use the `openbmc-build-scripts` tool to build & analyze & fix vulnerabilities
in the code layer.
 - Assuming that there are security vulnerabilities in the USB driver (such as memory
overflow, etc.), a mechanism is needed to disable the USB driver. After the vulnerability
is fixed, enable this USB driver again and restore the function.

## Testing

 - When the USB code update is disabled, the service will return directly without any update.
 - Manually insert the USB key with the firmware upgrade package, and check whether
the upgrade file is correct through the log.
 - Verify that the ApplyTime attribute value is set to OnRest.
 - Verify that the RequestedActivation property value is set to Active.
 - Verify that the firmware update was successful.

[1]: https://github.com/openbmc/phosphor-bmc-code-mgmt
