# In-Band Update of BMC Firmware using USB

Author: George Liu <liuxiwei!>

Created: 2021-10-12

## Problem Description

When Redfish or scp cannot be used, BMC needs a new mechanism for images to get
into the machine.

## Background and References

The openbmc project currently has a [phosphor-software-manager][1] repository.
In order to perform an update, need first to bring the image into the BMC
directory (/tmp/images). However, only TFTP and HTTP are currently supported,
and USB is not yet supported.

The intent of this new application design is to enable the USB driver of BMC to
enter the new image into BMC.

## Requirements

The following statements are reflective of the initial requirements.

- Monitor whether the USB key is inserted.
- The first tar file found in the sorted list of files on the USB device is
  copied to `/tmp/images`.
- Manually trigger firmware upgrade.
- Disable automatic reboot the BMC firmware after upgrade is complete to prevent
  a potential loop in the event of a key inserted.
- This mechanism attempts to maintain security, for example this feature is
  disabled by default or can be enabled or disabled via Redfish.

## Proposed Design

The new code would be part of the phosphor-software-manager repository(eg:
phosphor-usb-code-update). The design process is as follows:

- Define a macro switch (`usb-code-update`) in [phosphor-software-manager][1]
  repository to identify whether to enable the USB Code Update function, which
  is _enabled_ by default.
- If `usb-code-update` enabled, install the udev rules file to
  `/lib/udev/rules.d` during compilation.
- Once the udev rules are met, the systemd service is directly triggered and
  start the phosphor-usb-code-update daemon.
- This daemon verifies the `/run/media/usb/sda1` directory and copies the first
  `.tar` file in the directory to `/tmp/images` and starts verification.
- Set ApplyTime to OnReset so that the proposed usb code update app does not
  reboot the BMC after activation.
- Set RequestedActivation to Active, follow the updated status, start to update
  the firmware, and restart the BMC after completion.
- Exit the phosphor-usb-code-update daemon.

## Pseudocode

The udev rules files for example:

```
SUBSYSTEM=="block", ACTION=="add", ENV{ID_USB_DRIVER}=="usb-storage", ENV{DEVTYPE}=="partition", ENV{SYSTEMD_WANTS}="usb-code-update@%k", TAG+="systemd"
```

## Security

- It is recommended to run a local CI run and analyze & avoid potential
  vulnerabilities via cppcheck.
- Assuming that the USB drive has a physical security vulnerability (such as
  memory overflow, etc.), should disable "USB code update" via Redfish. After
  the vulnerability is fixed, enable "USB code update" again via Redfish.

## Alternatives Considered

If the OS fails to boot due to an error, so the firmware update cannot be done
through the OS, or the network fails, and the update cannot be done through
Redfish or scp, the server support staff can only uninstall the flash chip and
re-flashing, this is not Reasonably, service support should have local access to
the machine and update the system to a working firmware level.

## Impacts

This impacts security because it can copy files to the BMC via an external USB
key. There is no expected performance impact since the process just copies files
during runtime and exits automatically after completion.

## Testing

- When the USB code update is disabled, the service will return directly without
  any update.
- Manually insert the USB key with the firmware upgrade package, and check
  whether the upgrade file is correct through the log.
- Simulate `dev/sda1` on qemu with some test scripts and start the service(eg:
  `systemcl start usb-code-update@sda1.service`)
- Verify that the ApplyTime attribute value is set to OnRest.
- Verify that the RequestedActivation property value is set to Active.
- Verify that the firmware update was successful.

[1]: https://github.com/openbmc/phosphor-bmc-code-mgmt
