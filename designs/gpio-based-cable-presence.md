## GPIO-based Cable Presence Detection
Author:
  Chu Lin (linchuyuan@google.com)
Primary assignee:
  Chu Lin (linchuyuan@google.com)
Created:
  2021-07-29
## Problem Description
The intent of this new daemon design is to report GPIO-based cable presence
status. Ipmi currently doesn't support grouping discrete values and it could at
most support 255 sensor IDs. On a systems with 25 cables, and 255 sensor IDs,
taking up one sensor per slot would use 10% of the available space, and be a
huge waste. Therefore, we need a solution to pack multiple presence states into 
less sdr Ids.

## Background and References
1. https://www.intel.com/content/www/us/en/products/docs/servers/ipmi/ipmi-second-gen-interface-spec-v2-rev1-1.html
2. https://www.dmtf.org/sites/default/files/Redfish_Cable_Management_Proposal_WIP_04-2021.pdf

## Requirements
1. the openbmc IPMI interface should support exposing cable presence states.
2. The OpenBMC dbus interface should be compatible with redfish

## Proposed Design
Inventory.Item.Cable will be introduced as a new dbus interface for querying
cable objects. We will also need to introduce a new dbus daemon to collect
Presence states. This new daemon will reside in openbmc/dbus-sensors. Its
runtime configuration will be provided by EntityManager during startup. A
proposed config file looks like the following:
```
{
  "Exposes": [
    {
      "Name": "cable1",
      "GpioLine": "osfp1",
      "Polarity": "active_low",
      "Type": "GPIOCableSensing"
    },
    {
      "Name": "cable2",
      "GpioLine": "osfp2",
      "Polarity": "active_high",
      "Type": "GPIOCableSensing"
    }
  ]
}
```
The `Name` attribute is a unique name for each object from the config file.
The `GpioLine` is the gpio line label from the device tree. The `Polarity`
should tell the daemon whether it needs to revert the GPIO signal. Each entry
from the config would get translated into one object on the dbus. In addition,
the daemon polls the gpio presence signal for these objects in an interval of
10 seconds.

On the ipmi side, the presence states will be grouped into fewer sdr IDs in
order to save sdr IDs for ipmi. Given the following example,
```
└─/xyz
  └─/xyz/openbmc_project
    └─/xyz/openbmc_project/inventory
      └─/xyz/openbmc_project/inventory/item
        └─/xyz/openbmc_project/inventory/item/cable1
        └─/xyz/openbmc_project/inventory/item/cable2
```
In this example, we have cable1 and cable2 and we expect to group them together.
The grouping handler first removes all the trailing index from the
sdr name. In this case, the sdr name is cable1 and cable2. After that, both
cable1 and cable2 become cable. Therefore, objects that with the same name after
removing the trailing index indicates that they need to be grouped. In
the case of having more than 14 presence states in a group, the user is
responsible to divide the presence pins into smaller group with less
than 14 items in each group from the config file. For example, if there is
cable[0-15], the cables should be grouped into two different groups. The
following is an example of how to create two groups, cableGroupA and
cableGroupB. Note that after removing the trailing index from cableGroupA[0-7].
They are all cableGroupA. Hence, these items will be grouped. Same thing for
cableGroupB[0-7].
```
{
  "Exposes": [
    {
      "Name": "cableGroupA0",
      "GpioLine": "gpio_pin0",
      "IsActiveLow": true,
      "Type": "GPIOSensing"
    },
    ....
    {
      "Name": "cableGroupA7",
      "GpioLine": "gpio_pin7",
      "IsActiveLow": true,
      "Type": "GPIOSensing"
    },
    {
      "Name": "cableGroupB0",
      "GpioLine": "gpio_pin8",
      "IsActiveLow": true,
      "Type": "GPIOSensing"
    },
    ....
    {
      "Name": "cableGroupB7",
      "GpioLine": "gpio_pin15",
      "IsActiveLow": true,
      "Type": "GPIOSensing"
    },
  ]
}
```
GroupA is holding the presence state for cable[0-7]. Meanwhile, GroupB is
holding the presence state for cable[8-15]. Both groups is holding 8 presence
states. If more than 14 pins is provided in one group, ipmi would print error
message to the log and all the 14th pin and above would get ignored in the
response.

During a fetch operation, the sdr name is given. Then, it will be appended with
numeric index from 0 to 14. In this way, we should be able to reconstruct the
original object path. Then, given the object path, we can fetch the
OperationalStatus from dbus properties.

## Alternatives Considered

* Explore the option of reporting the presence state via type12 record instead
of discrete sensors.

## Testing
Testing can be accomplished via automated or manual testing to verify that:

* The running daemon is able to detect gpio signal correctly and mark the
presence state on the dbus to reflect the gpio signal.

* ipmitool is able to list the presence state by ipmitool sdr list.

* Invalid configuration from EntityManager should get rejected by the detection
daemon.
