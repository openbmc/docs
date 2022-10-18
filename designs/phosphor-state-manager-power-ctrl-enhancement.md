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

* The supported items are,

   * Host: RequestedHostTransition: On, Off, Reboot.

   * Chassis: RequestedPowerTransition: On, Off.

* The not supported items are,

1. Currently the phosphor-state-manager does not support power status for
   individual sub-chassis (slot power). which is supported in x86-power-control
   with chassis_system1...N.
   
2. The phosphor-state-manager currently supports On and Off control to the
   chassis but not supporting to the reboot.

   Example:
   ```
   Chassis: RequestedPowerTransition: Reboot
   ```

3. Add support for monitoring power good status for individual host and the
   sub-chassis. As per the current implementation the power good status is
   monitored only for the power supply.

4. Currently the phosphor-state-manager there is an option systemd target
   mapping to the sled power control or (sled power cycle) is not available.
   The DBUS object for the sled power cycle would be,

   ```
   Example:
           Object path: /xyz/openbmc_project/state/chassis_system0
   ```

## Background and References

The following are the dbus interfaces proposed to be created to handle the
slot power control, ie on, off, and, cycle.

   ```
   Interface: xyz.openbmc_project.State.Chassis

   Object path: /xyz/openbmc_project/state/chassis_system1
                  /xyz/openbmc_project/state/chassis_system2
                  .
                  .
                  .
                  /xyz/openbmc_project/state/chassis_systemN
   ```

link for the reference:
https://github.com/openbmc/x86-power-control/blob/master/src/power_control.cpp#L3058

## Requirements

1. The slot power control or sub-chassis power control is currently not supported
   and it is proposed to be added to the phosphor-state-manager. For example,
   the transition request and DBUS object for the slot on, off, and slot power
   cycle would be,

    * Interface: xyz.openbmc_project.State.Chassis
    * object path: /xyz/openbmc_project/state/chassis_system1
                  .
                  .
                  .
                  /xyz/openbmc_project/state/chassis_systemN
    * Transition: On, Off, PowerCycle

2. Currently the chassis on/off is supported and it is proposed to add reboot
   along with on/off, it would be,

    * Chassis: RequestedPowerTransition: Reboot

3. Add support for monitoring the power good status for the individual host and
   the sub-chassis.

4. Add support for sled power cycle to control complete sled power cycle. The
   DBUS object and state transition request for sled power cycle would be

    Example:

    ```
    Object path: /xyz/openbmc_project/state/chassis_system0
    RequestedPowerTransition: PowerCycle
    ```

## Proposed Design

1. This document proposes a design to implement the following state request
   transition for slot power on, off, and reboot in the phosphor-state-manager.

   The systemd targets and it's transition for slot power control (sub-chassis)
   are,

   Example:
   ```
   {Transition::On, fmt::format("obmc-chassis-slot-poweron@{}.target"}
   {Transition::Off, fmt::format("obmc-chassis-slot-poweroff@{}.target"}
   {Transition::Powercycle, fmt::format("obmc-chassis-slot-powercycle@{}.target"}
   ```
   The respective targets are mapped to the state transition and it will invokes
   the corresponding bash scripts.

   The following are the DBUS objects to exposed for slot power (sub-chassis)

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
   The following interface is created for slot power on, off, and cycle state
   transition request

   ```
   xyz.openbmc_project.State.Chassis
   ```

   The following property is the part of slot power on and off state transition
   request

   ```
   .RequestedPowerTransition - This property shows the power transition
               (xyz.openbmc_project.State.Chassis.Transition.On/Off/Powercycle)
   ```

2. Currently in the phosphor-state-manager for chassis control(Hard power)
   only on/off supports are available, for reboot there is support needs
   to add along with on and off.
   
   The systemd target and it's transition for chassis power contol shown below,

   Example:
   ```
   {Transition::Reboot, fmt::format("obmc-chassis-powercycle@{}.target")};
   ```
   The above systemd target is mapped to the chassis power cycle or reboot
   transition. and finally it will invoke the chassis power cycle bash script.

   The following are DBUS objects and its transition for the chassis power
   control are,

   Example:

   ```
   `-/xyz
     `-/xyz/openbmc_project
       `-/xyz/openbmc_project/state
         `-/xyz/openbmc_project/state/chassis1

   .RequestedPowerTransition - This property shows the power transition
                          (xyz.openbmc_project.State.Chassis.Transition.Reboot)
   ```

3. As per the current implementation the power good status is monitored only
   for the power supply in the phosphor-state-manager. So need add support
   to read power good status for each sub-chassis and host. and this will
   determine the initial status of power.

4. The sled power cycle or reboot support is currently not available and it
   is proposed to be added to the phosphor-state-manager.

   The systemd target and it's transition for sled power cycle are,

   Example:
   ```
   {Transition::PowerCycle, fmt::format("obmc-sled-powercycle@{}.target")};

   ```
   The following are DBUS objects and its transition for the sled power cycle,

   Example:

   ```
     /xyz/openbmc_project/state/chassis_system0
     .RequestedPowerTransition - PowerCycle
   ```
## Impact

The change only adds the new DBUS objects and interfaces to discover the slot
power control and chassis power cycle or reboot and it will configured from
yocto during compiletime(flag). So existing doesn't change and there is no
impact.

## Testing

The change tested with greatlakes platform.
