## GPIO-based Cable Presence Detection
Author:
  Chu Lin
Primary assignee:
  Chu Lin
Created:
  2021-07-29
## Problem Description
The intent of this new daemon design is to report GPIO-based cable presence
status via ipmi. The challenge here is to stack the presence state into fewer
sdrs in the ipmi world so that it doesn't take too many sdr slots. However,
in the Redfish world, it has unlimited sdr slots. Therefore, all the necessary
implementation to stack the presence state into fewer sdrs needs to go to ipmid
so that we can be compatible with the Redfish design.

## Background and References
Ipmi currently doesn't support grouping boolean values. This would be a waste of
sdr slots since at most, we could support 256 sdr slots. In enterprice projects,
it would result in not having enough sdr slots.

## Requirements
1). IPMID need to be able to stake multiple gpio states into fewer sdr slots.
2). The new design needs to be Redfish compatible.

## Proposed Design
There will be two new dbus interfaces. One is Inventory.Item.Cable. One is
Inventory.Decorator.CableClass. The cable interface is a generic
empty interface to list cable items. Meanwhile, the cable class interface is
used to specify the cable class enum.

We will also need to introduce a new dbus daemon to collect Presence states.
This new daemon will reside in openbmc/dbus-sensors. Its runtime configuration
will be provided by EntityManager during startup. A proposed config file looks
like the following:
```
{
  "Exposes": [
    {
      "Name": "cable1",
      "PinLabel": "osfp1",
      "IsActiveLow": true,
      "Type": "GPIOSensing"
    },
    {
      "Name": "cable2",
      "PinLabel": "osfp2",
      "IsActiveLow": true,
      "Type": "GPIOSensing"
    }
  ]
}
```
The `Name` attribute needs to match the GPIO pin label. The `PinLabel` shall
contain gpio pin label that will be accepted by the system. The `IsActiveLow`
will tell the daemon whether if it needs to reverse the GPIO signal. Each entry
from the expose would get translated to one object on the dbus. In addition,
the daemon polls the gpio presence signal for these objects in an interval of
10 seconds.

On ipmi side, the presence states will be grouped into a fewer sdr slots in
order to save sdr slots for ipmi. Given the following example,
```
└─/xyz
  └─/xyz/openbmc_project
    └─/xyz/openbmc_project/inventory
      └─/xyz/openbmc_project/inventory/item
        └─/xyz/openbmc_project/inventory/item/cable/cable1
        └─/xyz/openbmc_project/inventory/item/cable/cabl22
```
Both `cable1` and `cable2` will be grouped together and the common name
for `cable1` and `cable2` is `cable`. Therefore, `cable` will be used as the 
sdr name. In the case of having more than 14 presence states in a group,
the user is responible to divide the presence pins into smaller group with less
than 14 items in each group from config file. For example, if there is
cable[1-16], the config file can be divided into two group like the following:
```
{
  "Exposes": [
    {
      "Name": "cableGroupA0",
      "PinLabel": "gpio_pin0",
      "IsActiveLow": true,
      "Type": "GPIOSensing"
    },
    ....
    {
      "Name": "cableGroupA7",
      "PinLabel": "gpio_pin7",
      "IsActiveLow": true,
      "Type": "GPIOSensing"
    },
    {
      "Name": "cableGroupB0",
      "PinLabel": "gpio_pin8",
      "IsActiveLow": true,
      "Type": "GPIOSensing"
    },
    ....
    {
      "Name": "cableGroupB7",
      "PinLabel": "gpio_pin15",
      "IsActiveLow": true,
      "Type": "GPIOSensing"
    },
  ]
}
```
If more than 14 pins is provided in one group, ipmi would print error message to
the log and all the 14th pin and above would get ignored in the response.

During a fetch operation, the sdr name is given. Then, it will be appended with
numeric index from 0 to 14. In this way, we should be able to reconstruct the
original object path. Then, given the object path, we can fetch the
OperationalStatus from dbus properties.

## Testing
Testing can be accomplished via automated or manual testing to verify that:

# The running daemon is able to detect gpio signal correctly and mark the
presence state on the dbus to reflect the gpio signal.

# ipmitool is able to list the presence state by ipmitool sdr list.

# Invalid configuration from EntityManager should get rejected by the detection
daemon.
