# phosphor-state-manager power-control enhancement

Author:
   Karthikeyan P(Karthik), [p_karthikeya@hcl.com](mailto:p_karthikeya@hcl.com)
   Naveen Moses S(naveen.moses), [naveen.mosess@hcl.com](mailto:naveen.mosess@hcl.com)
   Velumani T(velu),  [velumanit@hcl](mailto:velumanit@hcl.com)

other contributors:

created:
    Oct 18, 2022

## Problem Description

The plan is to use phosphor-state-manager for the complete power control of the
machine.

The platform or the machine which is targeted does not have the regular design
of having different GPIO lines for the power control operation.  The
x86-power-control is specifically designed to use the power control using GPIOs
and phosphor-state-manager looks more suitable for this application as it uses
a systemd targets and it could a machine specific operations.  The power of the
host shall be controlled using ipmb interface or I2C with set of oem specific
ipmb commands or I2C commands.

A targeted platform has a sub-chassis or slot, which needs additional things
to control sled power. currently, the phosphor-state-manager doesn't have the
support.

Additionally, need to monitor the power ok(pgood status) for sub-chassis
as well as an individual host. currently, the phosphor-state-manager doesn't
have the support.

* The supported items under phosphor-host-state-manager are,

   * Host: RequestedHostTransition: On, Off, Reboot, GracefulWarmReboot,
           ForceWarmReboot.

* The supported items under phosphor-chassis-state-manager are,

   * Chassis: RequestedPowerTransition: On, Off.

* The not supported items are,

1. Currently, the phosphor-state-manager does not support power status or control
   for individual sub-chassis (slot power). which is supported in
   x86-power-control with chassis_system1...N.

2. The phosphor-state-manager currently supports On and Off control to the
   chassis (phosphor-chassis-state-manager) but not supporting to the reboot.

   Example:
   ```
   Chassis: RequestedPowerTransition: PowerCycle
   ```

3. Add support for monitoring power good status for individual host and the
   sub-chassis. As per the current implementation, the power good status is
   monitored only for the power supply.

4. Currently the phosphor-state-manager there is no support for systemd target
   mapping to the sled power control or (sled power cycle) is not available.
   The DBUS object for the sled power cycle would be,

   ```
   Example:
           Object path: /xyz/openbmc_project/state/chassis_system0
   ```

## Background and References

* The following are the object paths proposed to be created under
  phosphor-chassis-state-manager to handle the slot power control, ie on, off,
  and, cycle.

   ```

   Object path: /xyz/openbmc_project/state/chassis_system1
                /xyz/openbmc_project/state/chassis_system2
                  .
                  .
                  .
                /xyz/openbmc_project/state/chassis_systemN
   ```

   link for the reference:
   https://lists.ozlabs.org/pipermail/openbmc/2020-August/022481.html

* chassis_system0 object for the full power cycle of the entire system to be
  created under phosphor-chassis-state-manager.

  link for the reference:
  https://lists.ozlabs.org/pipermail/openbmc/2020-October/023540.html


## Requirements

1. The slot power control or sub-chassis power control supports are proposed to
   be added to the under phosphor-chassis-state-manager. For example, the
   transition request and DBUS object for the slot on, off, and slot power
   cycle would be,

    Example:
    ```
    object path: /xyz/openbmc_project/state/chassis_system1
                  .
                  .
                  .
                  /xyz/openbmc_project/state/chassis_systemN
    Transition: On, Off, PowerCycle
    ```

2. Currently, the chassis on/off is supported and it is proposed to add power
   cycle along with on/off under phosphor-chassis-state-manager, it would be,

    * Chassis: RequestedPowerTransition: PowerCycle

3. Add support for monitoring the power good status for the individual host and
   the sub-chassis.

4. Add support for the sled power cycle to control the complete AC power cycle.
   The DBUS object and state transition request for sled power cycle would be

    Example:

    ```
    Object path: /xyz/openbmc_project/state/chassis_system0
    RequestedPowerTransition: PowerCycle
    ```

## Proposed Design

1. This document proposes a design to implement the following state request
   transition for slot power on, off, and power cycle under phosphor-chassis-
   state-manager in phosphor-state-manager.

   The systemd targets and its transition for slot power control (sub-chassis)
   are,

   Example:
   ```
   {Transition::On, fmt::format("obmc-chassis-slot-poweron@{}.target"}
   {Transition::Off, fmt::format("obmc-chassis-slot-poweroff@{}.target"}
   {Transition::Powercycle, fmt::format("obmc-chassis-slot-powercycle@{}.target"}
   ```
   The respective targets are mapped to the state transition and it will invoke
   the corresponding bash scripts.

   The following are the DBUS objects to be exposed for slot power (sub-chassis)

   Example :

   ```
   `-/xyz
     `-/xyz/openbmc_project
       `-/xyz/openbmc_project/state
         `-/xyz/openbmc_project/state/chassis_system1

     `-/xyz/openbmc_project
       `-/xyz/openbmc_project/state
         `-/xyz/openbmc_project/state/chassis_system2
          .
          .
          .
   `-/xyz
     `-/xyz/openbmc_project
       `-/xyz/openbmc_project/state
         `-/xyz/openbmc_project/state/chassis_systemN
   ```
   The following interface is part of phosphor-chassis-state-manager, we can
   create only dbus object path along the interface to handle slot power
   control.

   ```
   xyz.openbmc_project.State.Chassis
   ```

   The following property is the part of slot power on and off and power cycle
   state transition request

   ```
   .RequestedPowerTransition - This property shows the power transition
               (xyz.openbmc_project.State.Chassis.Transition.On/Off/Powercycle)
   ```

2. Currently, host power on and host hard power off supports are available in
   phosphor-chassis-state-manager with the following DBUS object and systemd
   target.

   ```
   {Transition::Off, fmt::format("obmc-chassis-hard-poweroff@{}.target", id)},
   {Transition::On, fmt::format("obmc-chassis-poweron@{}.target", id)}};
   ```
   But the targeted platform need support to handle chassis power cycle. So
   add support for the chassis power cycle along with on and off.

   Example:
   ```
   {Transition::Powercycle, fmt::format("obmc-chassis-powercycle@{}.target")};
   ```
   The systemd target is mapped to the chassis power cycle or reboot transition.
   and finally, it will invoke the chassis power cycle bash script.

   The following are DBUS objects and their transition for the chassis power
   cycle,

   Example:

   ```
   `-/xyz
     `-/xyz/openbmc_project
       `-/xyz/openbmc_project/state
         `-/xyz/openbmc_project/state/chassis1

   .RequestedPowerTransition - This property shows the power transition
                          (xyz.openbmc_project.State.Chassis.Transition.Powercycle)
   ```

   ie The sub-chassis and chassis transition are the same on, off, and power
   cycle, which is going to be implemented under thephosphor-chassis-state-manager
   but the difference is the object path. Based on the  object path we can map the
   respective systemd target for both sub-chassis and chassis.

   Example:
   ```
   sub-chassis: /xyz/openbmc_project/state/chassis_system1
   chassis:     /xyz/openbmc_project/state/chassis1
   ```

3. As per the current implementation, the power good status is monitored only
   for the power supply in the phosphor-state-manager. So add support to read
   power good status for each sub-chassis and host. and it will determine the
   initial status of power.

   The following are DBUS object and it's property to read pgood status for
   sub-chassis and host.

   Example:
   ```
   `-/xyz
     `-/xyz/openbmc_project
       `-/xyz/openbmc_project/Chassis
         `-/xyz/openbmc_project/Chassis/Control
           `-/xyz/openbmc_project/Chassis/Control/Power

   property: PGood

   ```

4. The sled power cycle or reboot support is currently not available and it
   is proposed to be added to the under phosphor-chassis-state-manage.

   The systemd target and it's transition for sled power cycle are,

   Example:
   ```
   {Transition::PowerCycle, fmt::format("obmc-sled-powercycle@{}.target")};

   ```
   The following are DBUS object and its transition for the sled power cycle,

   Example:

   ```
     DBUS object: xyz.openbmc_project.State.Chassis1
                  .
                  .
                  .
                  xyz.openbmc_project.State.ChassisN
     Object path: /xyz/openbmc_project/state/chassis_system0
     .RequestedPowerTransition - PowerCycle
   ```
## Impact

We can ensure the existing functionality remains unchanged and does not affect
the other platforms. The sub-chassis power control feature configured from
yocto during compile time. So this feature can be disabled for other platforms
and should not affect their normal operation.

## Testing

The change tested with greatlakes platform.
