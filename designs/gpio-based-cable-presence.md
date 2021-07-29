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
      "CableClass": "osfp",
      "IsActiveLow": true,
      "Type": "GPIOSensing"
    },
    {
      "Name": "cable2",
      "CableClass": "osfp",
      "IsActiveLow": true,
      "Type": "GPIOSensing"
    }
  ]
}
```
The `Name` attribute needs to match the GPIO pin label. The `CableClass` shall
contain a user-defined type for this cable. The `IsActiveLow` will tell the
daemon whether if it needs to reverse the GPIO signal. Each entry from the expose
would get translated to one object on the dbus. In addition, the detection daemon
polls the gpio presence signal for these objects in an interval of 5 seconds.

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
sdr name. In the case of having more than 8 presence states in a group,
the sdrs names will be appended with hyphen and a alphabet letters starting
from A.

During a fetch operation, the sdr name is given. Then, it will be appended with
numeric index from 0 to 8. In this way, we should be able to reconstruct the
original object path. Then, given the object path, we can fetch the
OperationalStatus from dbus properties.

## Testing
Testing can be accomplished via automated or manual testing to verify that:

# The running daemon is able to detect gpio signal correctly and mark the
presence state on the dbus to reflect the gpio signal.

# ipmitool is able to list the presence state by ipmitool sdr list.

# Invalid configuration from EntityManager should get rejected by the detection
daemon.
