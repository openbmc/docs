# Dual flash design

Author: Lei YU <yulei.sh@bytedance.com> <LeiYU!>

Other contributors: John Wang <wangzhiqiang.bj@bytedance.com> <JohnWang!>,
                    Lotus Xu <xuxiaohan@bytedance.com> <lotus007!>

Created: 2020-09-10


## Problem Description

This doc is to provide a design to support the dual flash for the systems
with static layout.


## Background and References

Some systems use two flash chips for BMC, one is the primary flash, the other
is the backup image. It is up to the system administrator to decide which
image(s) to flash during code update.

OpenBMC does not support the above case.


## Requirements

- Simplicity: The design is easy to understand and the management is easy to
  maintain.
- Dual image update: The BMC shall be able to update the primary and secondary
  flash as requested.
- Re-use: Use existing software and try not to add too many differences.
- Persistent settings: The settings like network/user/password could be
  preserved when BMC is running in the secondary flash. This makes sure the BMC
  could be reached.


## Proposed Design

Note that the proposed design applies to the static flash layout only. The UBI
flash layout uses the two flashes differently.

* In Linux kernel dts, the device related to switching to secondary image shall
  be enabled. (E.g, it is wdt2 in aspeed.)
* In Linux kernel dts, the secondary flash shall be described, e.g. `flash@1`,
  and the label shall indicate it's an alternative flash. Typically the label
  is set to `alt-bmc`.
  The secondary flash rwfs partition could be exposed as mtd device to sync
  the settings from primary flash, e.g. the networking settings, the
  username/password, etc.
  The settings to sync could be specified by `synclist`, which could be
  overridden by the machine layer as needed.
* In phosphor-dbus-interfaces, an interface or property is needed to indicate
  which flash it is running on.
  It is possible to re-use the `RedundancyPriority` property:
  * For the image that is on the primary flash, set it to 0.
  * For the image that is on the secondary flash, set it to 1 (or non zero).
* In phosphor-dbus-interfaces, an interface is needed to indicate the uploaded
  image's `UpdateTarget`, it could be specified as:
  * Primary: the image shall be programmed to the primary flash;
  * Secondary: the image shall be programmed to the secondary flash;
  * Both: the image shall be programmed to both flashes.
* The code update shall follow as below:
   * If the BMC is running on the primary flash:
      1. If the `UpdateTarget` is set to `Primary`, the image is flashed to
         the primary flash by `/run/shutdown`.
      2. If the `UpdateTarget` is set to `Secondary`, the image is flashed to
         the secondary flash at runtime by a systemd service.
      3. If the `UpdateTarget` is set to `Both`, the image is flashed to the
         secondary flash at runtime, and then flashed to the primary flash by
         `/run/shutdown`.
   * If the BMC is running on the secondary flash:
      1. If the `UpdateTarget` is set to `Primary`, the image is flashed to
         the primary flash at runtime by a systemd service.
      2. If the `UpdateTarget` is set to `Secondary`, the image is flashed to
         the secondary flash by `/run/shutdown`.
      3. If the `UpdateTarget` is set to `Both`, the image is flashed to the
         primary flash at runtime, and then flashed to the secondary flash by
         `/run/shutdown`.


## Alternatives Considered

With two BMC flash chips, some could use them as side A/B, which has its pros
and cons.
But this doc is to propose a design with the "primary" and "secondary" concept
for the dual images, where the primary is considered the functional image in
normal case and the secondary is considered the backup image for rescue
purpose.


## Impacts

For the systems with a single BMC flash, this design has no impact.

For the systems with two BMC chips that would want the primary and secondary
concept, this design adds the new functions in `phosphor-bmc-code-mgmt` to
handle code update for the two images and the sync of settings.

The changes related to this design involve the below components:

- `openbmc/linux`: dts changes to support booting from secondary flash.
- `openbmc/phosphor-dbus-interfaces`: Update the description of
   RedundancyPriority interface to describe that it could indicate the running
   flash status, add a new interface for the setting of the `UpdateTarget`.
- `openbmc/phosphor-bmc-code-mgmt`: Changes to support the above functions.

Potentially, there could be some other changes to report the situation, e.g.
via Redfish event or IPMI sel log.

## Testing

This requires the manual test to:
* Manually trigger or make the robot (in openbmc-test-automation) to trigger
  the watchdog to make the hardware boot from the secondary image.
* Verify that BMC settings in the secondary image are synced with the primary
  one.
* Verify the BMC code update functions in both primary and secondary image.
* Reboot the BMC to verify the updated image is working on the primary flash.
* Verify the event log to make sure the above event is logged.
