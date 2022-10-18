# phosphor-state-manager power-control enhancement

Author:
   Karthikeyan P(Karthik), [p_karthikeya@hcl.com](mailto:p_karthikeya@hcl.com)
   Naveen Moses S(naveen.moses), [naveen.mosess@hcl.com](mailto:naveen.mosess@hcl.com)
   Velumani T(velu),  [velumanit@hcl](mailto:velumanit@hcl.com)

other contributors:

created:
    Oct 18, 2022

## Problem Description

The x86-power-control assumes GPIOs for power-control and that is not suitable
for the targeted platform, The phosphor-state-manager seems like the better
option with some enhancements.

The plan is to use phosphor-state-manager for the power control of the machine.

A targeted platform has a sub-chassis or slot, which needs to control sled power.
Currently, the phosphor-state-manager doesn't have the support.

Additionally, need to monitor the power ok(pgood status) for sub-chassis
as well as an individual slots. Currently, the phosphor-state-manager doesn't
have the support.

The supported items under phosphor-host-state-manager are,

 - Host: RequestedHostTransition: On, Off, Reboot, GracefulWarmReboot,
         ForceWarmReboot.

The supported items under phosphor-chassis-state-manager are,

 - Chassis: RequestedPowerTransition: On, Off.

Currently unsupported items are,

 - Currently, the phosphor-state-manager does not support power control for the
   individual slot (sub-chassis or slot power).

 - The phosphor-state-manager currently supports On and Off control to the
   chassis (phosphor-chassis-state-manager) but does not support to the reboot or
   power cycle.

 - Monitoring power good status for individual slot and the sub-chassis of a
   system is not available. As per the current implementation, the power good
   status is monitored only for the power supply.

 - Currently the phosphor-state-manager, There is no support for systemd target
   mapping to the sled power control or (sled power cycle). The
   **chassis_system0** object will be used for full power cycle of the
   entire system and the DBUS object would be
   **yz.openbmc_project.State.ChassisN**.

## Background and References

 - The following are the object paths proposed to be created under
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

 - link for the reference:
   https://lists.ozlabs.org/pipermail/openbmc/2020-August/022481.html

 - chassis_system0 object for the full power cycle of the entire system to be
   created under phosphor-chassis-state-manager.

 - link for the reference:
   https://lists.ozlabs.org/pipermail/openbmc/2020-October/023540.html

## Requirements

 - The slot power control or sub-chassis power control supports are proposed to
   be added to the under phosphor-chassis-state-manager. For example, the
   transition request and DBUS object for the slot on, off, and slot power
   cycle would be,

**Example**
```
    object path: /xyz/openbmc_project/state/chassis_system1
                  .
                  .
                  .
                  /xyz/openbmc_project/state/chassis_systemN
    Transition: On, Off, PowerCycle
```

 - Currently, the chassis on/off is supported. it is proposed to add power
   cycle along with on/off under phosphor-chassis-state-manager.

 - Add support for monitoring the power good status for the individual host and
   the sub-chassis.

 - Add support for the sled power cycle to control the complete AC power cycle.
   The DBUS object and state transition request for sled power cycle would be

**Example**
```
   Object path: /xyz/openbmc_project/state/chassis_system0
   RequestedPowerTransition: PowerCycle
```

 - Based on POST completing, add support for OS (Operating System) state
   change.

## Proposed Design

 - This document proposes a design to implement the following state request
   transition for slot power on, off, and power cycle under phosphor-chassis-
   state-manager in phosphor-state-manager.

   The systemd targets and its transition for slot power control (sub-chassis)
   are,

**Example**
```
   {Transition::On, fmt::format("obmc-chassis-system-poweron@{}.target"}
   {Transition::Off, fmt::format("obmc-chassis-system-poweroff@{}.target"}
   {Transition::Powercycle, fmt::format("obmc-chassis-system-powercycle@{}.target"}
```
   The respective targets are mapped to the state transition and it will invoke
   the corresponding bash scripts.

   The following are the DBUS objects to be exposed for slot power (sub-chassis)

**Example**
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

   The following property is the part of slot power on and off and power cycle
   state transition request

   ```
   .RequestedPowerTransition - This property shows the power transition
               (xyz.openbmc_project.State.Chassis.Transition.On/Off/Powercycle)
   ```

 - Currently, host power on and host hard power off supports are available in
   phosphor-chassis-state-manager with the following DBUS object and systemd
   target.

   ```
   {Transition::Off, fmt::format("obmc-chassis-hard-poweroff@{}.target", id)},
   {Transition::On, fmt::format("obmc-chassis-poweron@{}.target", id)}};
   ```
   In addition, the targeted platform needs to add support to handle chassis
   power cycle.

**Example**
```
   {Transition::Powercycle, fmt::format("obmc-chassis-powercycle@{}.target")};
```
   The systemd target is mapped to the chassis power cycle or reboot
   transition. It will invokes the chassis power cycle bash script.

   The following are DBUS objects and their transition for the chassis power
   cycle,

**Example**
```
   `-/xyz
     `-/xyz/openbmc_project
       `-/xyz/openbmc_project/state
         `-/xyz/openbmc_project/state/chassis1

   .RequestedPowerTransition - This property shows the power transition
                          (xyz.openbmc_project.State.Chassis.Transition.Powercycle)
```

   ie The sub-chassis and chassis transition are the same: on, off, and power
   cycle. The change needed is to implement a different object path under
   phosphor-chassis-state-manager. Based on the object path we can map the
   respective systemd target for both sub-chassis and chassis.

**Example**
```
   sub-chassis: /xyz/openbmc_project/state/chassis_system1
                .
                .
                .
                /xyz/openbmc_project/state/chassis_systemN

   chassis:     /xyz/openbmc_project/state/chassis1
                .
                .
                .
                /xyz/openbmc_project/state/chassisN
```

 - As per the current implementation, the power good status is monitored only
   for the power supply in the phosphor-state-manager. A change to read the
   power good status for each sub-chassis and host to determine the initial
   power status is needed.

   The following are DBUS object and it's property to read pgood status for
   sub-chassis and host.

**Example**
```
   `-/xyz
     `-/xyz/openbmc_project
       `-/xyz/openbmc_project/Chassis
         `-/xyz/openbmc_project/Chassis/Control
           `-/xyz/openbmc_project/Chassis/Control/Power

   property: PGood
```

 - The sled power cycle or reboot (a full power cycle of the entire system)
   support is currently not available. It is proposed to be added to the
   under phosphor-chassis-state-manage.

 - The systemd target and it's transition for sled power cycle are,

**Example**
```
   {Transition::PowerCycle, fmt::format("obmc-sled-powercycle@{}.target")};

```
   The following are DBUS object and its transition for the sled power cycle,

**Example**
```
   Object path:
   -/xyz/
     -/xyz/openbmc_project/
      -/xyz/openbmc_project/state
         |-/xyz/openbmc_project/state/chassis2
         |-/xyz/openbmc_project/state/chassis_system0
         .
         .
         .
    -/xyz/
     -/xyz/openbmc_project/
      -/xyz/openbmc_project/state
         |-/xyz/openbmc_project/state/chassisN
         |-/xyz/openbmc_project/state/chassis_system0

     DBUS object: xyz.openbmc_project.State.Chassis1
     .RequestedPowerTransition - PowerCycle
```

 - The operating system state change will be updated based on indication of
   POST complete. Code needs to be added in phosphor-state-manager to capture
   the OS state change.

 - A separate script will monitor and read the POST complete status for each
   host. If the status is true the OS state will be standby or else inactive.

## Impact

We can ensure the existing functionality remains unchanged and does not affect
the other platforms.

## Testing

This proposed design is tested in multiple host platforms.

***Unit Test*** - The proposed code can be covered by the unit tests.
