# In-Band Update of BMC Firmware using USB

Author: George Liu <liuxiwei!>

Primary assignee: George Liu

Created: 2021-10-12

## Problem Description

Currently OpenBMC does not support verifying firmware updates via USB.
Now BMC needs this mechanism to update the firmware, There's a desire to have
firmware updates via USB authenticated like the out-of-band updates are.

## Background and References

None.

## Requirements

When the user updates the firmware via USB in-band, the `USBCodeUpdate` service
will listen to and trigger the `hotplug` event, get a temporary mount
directory(`/media/usb`), copy the first file found in the key with `.tar` extension
to `/tmp/images/`, and the [phosphor-software-manager](https://github.com/openbmc/phosphor-bmc-code-mgmt)
will start decompressing the file, verify and update the firmware.

## Proposed Design

The new code would be part of the phosphor-bmc-code-mgmt repository. The new code
flow is as follows:
 - The premise is that there is a monitoring `hotplug` service. If it detects
 that the USB key is inserted, it will immediately trigger and send a `hotplug`
 event and generate a temporary mount directory. In addition, if security
 vulnerability is found in the USB driver itself, we'll want a function to unload
 the USB driver, and not merely stop the code update service.
 - Define a macro switch in the phosphor-bmc-code-mgmt repository to indicate
 whether to enable the USB Code Update function.
 - If `USBCodeUpdate` enabled, need to subscribe to the `hostplug` event and
 be triggered by this signal.
 - Copy the first `.tar` file in the `/media/usb` directory to `/tmp/images` and
 start to verify and update the firmware.

## Testing

Manually insert the USB key with the upgrade package, and check whether the
firmware upgrade is successful through the log.
