# Dual flash design

Author: Lei YU <yulei.sh@bytedance.com> <LeiYU!>

Other contributors: John Wang <wangzhiqiang.bj@bytedance.com> <JohnWang!>,
                    Lotus Xu <xuxiaohan@bytedance.com> <lotus007!>

Created: 2020-09-10


## Problem Description

This doc is to provide a design to support the dual flash where the secondary
flash is treated as the golden image.


## Background and References

Some systems use two flash chips for BMC, one is the primary flash, the other
is the "golden" image that is flashed in the manufacture and never updated in
the field.

OpenBMC does not support the above case.


## Requirements

- Simplicity: The design is easy to understand and the management is easy to
  maintain.
- Golden image: The secondary flash is expected to never be updated, and it
  shall be able to program the primary flash.
- Re-use: Use existing software and try not to add too many differences.
- [Optional] Persistent network: The network related settings (IP, gateway)
  could be preserved when BMC is running in the golden image. This makes sure
  the BMC could be reached.


## Proposed Design

There is an assumption that the golden image has either:
* The default username/password from production;
* The synced username/password from primary flash;
So that the system administrator could login and operate when the BMC is
running on the golden image.

Note that the proposed design applies to the static flash layout only. The UBI
flash layout uses the two flashes differently.

* In Linux kernel dts, the device related to switching to golden image shall
  be enabled. (E.g, it is wdt2 in aspeed.)
* In Linux kernel dts, the secondary flash shall be described, e.g. `flash@1`,
  and the label shall indicate it's an alternative flash. Typically the lable
  is set to `alt-bmc`.
  [Optional] The secondary flash rwfs partition could be exposed as mtd device to
  sync the settings from primary flash, e.g. the networking settings, the
  username/password, etc.
* In phosphor-dbus-interfaces, an interface or property is needed to indicate
  which flash is it running.
  It is possible to re-use the `RedundancyPriority` property:
  * Make it read-only when golden image design is enabled.
  * When BMC is running on the primary flash, set it to 0.
  * When BMC is running on the golden image, set it to 1 (or non zero).
* `phosphor-bmc-code-mgmt` shall check the boot flash status and set the above
  property accordingly.
   * If BMC is running on the primary flash:
      * The code update is the same as before, that it updates the primary
        flash.
      * [Optional] In runtime, it could mount the golden image's rwfs and sync
        the settings from the primary flash. So that when the BMC boots into
        the golden image, it does not lose settings.
   * If BMC is running on the golden image (when primary flash fails to boot,
     or could be triggered by code in case primary kernel/rofs panics):
      * The code update shall use the full image to program the whole primary
        flash, typically by the system admin manually.
      * [Optional] On startup, it shall not mount the primary flash's rwfs,
        because:
        * The settings are already synced.
        * The primary flash's rwfs could be broken.
* The code update when BMC is running in the golden image is:
   1. Upload a tarball with the full image;
   2. `phosphor-bmc-code-mgmt` shall check the image, and fail the process if
      the image is not the full image or the signature is not valid.
   3. On activating, the full image is flashed into the primary flash.
   4. [Optional] After the update, it could mount the primary flash's rwfs and
      sync the settings, and umount it when the sync is done. So that when the
      BMC is recovered and boot back to the primary flash, it does not lose
      settings.
   5. After the above is done, the BMC shall be rebooted from the primary flash.
   6. If everything is OK, the BMC is running well on the primary flash, the
      recovery is done.
   7. In case any of the above steps 1~4 fail, the BMC shall keep running in
      the golden image and requires a manual fix.


## Alternatives Considered

With two BMC flash chips, some could use them as side A/B, which has its pros
and cons.
But this doc is to propose a design with the "golden image" concept.


## Impacts

For the systems with a single BMC flash, this design adds a minor change that
it puts the "CurrentFlash" property on D-Bus and it's always the primary flash.
No other impacts.

For the systems with two BMC chips that would want the "golden image" concept,
this design adds the new functions in `phosphor-bmc-code-mgmt` to handle the
networking settings sync and the code update in golden image.

The changes related to this design involve the below components:

- `openbmc/linux`: dts changes to support booting from golden flash.
- `openbmc/phosphor-dbus-interfaces`: Update the description of
   RedundancyPriority interface to describe that it could indicate the running
   flash status.
- `openbmc/phosphor-bmc-code-mgmt`: Changes to support the above functions.

Potentially, there could be some other changes to report the situation, e.g.
via Redfish event or IPMI sel log.

## Testing

This requires the manual test to:
* Manually trigger or make the robot (in openbmc-test-automation) to trigger
  the watchdog to make the hardware boot from the golden image.
* [Optional] Verify BMC networking settings or username/password in golden
  image.
* Verify the BMC code update functions in the golden image.
* Reboot the BMC to verify the updated image is working on the primary flash.
* Verify the event log to make sure the above event is logged.

