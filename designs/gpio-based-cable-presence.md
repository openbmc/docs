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
1. The openbmc IPMI interface should support exposing cable presence states.
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
In this example, we have both cable1 and cable2 and we expect to group them
together. The grouping handler first removes all the trailing index from the
sdr name. In this case, the sdr name is cable1 and cable2. After that, both
cable1 and cable2 become cable. Therefore, objects with the same name after
removing the trailing index indicates that they need to be grouped. After
grouping, the new sdr name is cable[1-2]. The sdr name implies that this sdr
has the presence state for cable1 and cable2. Meanwhile, the bit position
for cable1 is 0 and the bit position for cable2 is 1. In the case of having
more than 14 presence states, the group handler will automatically jump to use 
a new sdr. For example, if there are 20 cable indexed from 0 to 19, we
shall see two sdrs. One is cable[0-13]. One is cable[14-19]. If the object path
is not indexed by the user, it will take one sdr ID. For example, if the object
path is `/xyz/openbmc_project/inventory/item/cable`, it will be untouched and
use 1 sdr Id for its presence state. See the following for an example output
from `ipmitool sdr list`.
```
ipmitool sdr list event
cdfp[0-3]        | Event-Only        | ns
osfp             | Event-Only        | ns
osfp[1-2]        | Event-Only        | ns
```

During the fetch operation, the group handler extracts the range string from the
square bracket and reconstruct the original object path. Then, given the object
path, ipmid will fetch the OperationalStatus from dbus properties.

## Alternatives Considered

* Explore the option of reporting the presence state via type12 record instead 
  of discrete sensors. The challenge here is that type12 record also has a limit
  of 255 IDs. Meanwhile, it doesn't support stacking multiple presence states in
  one ID.

* We could also let the user define how to group different cables from config
  file. However, this requires users to have a deep understanding of how ipmid
  works. It is better if the ipmi daemon just takes care of this.

* Instead of having a sleep-poll mechanism, it is better to have a event
  listener on the gpio signals. However, the gpio_event_wait api requires all
  the lines to come from the same chip. See
  https://github.com/brgl/libgpiod/blob/cc23ef61f66d5a9055b0e9fbdcfe6744c8a637ae/lib/core.c#L993
  We could spawn threads to listen to each chips but I don't think we should
  increase the complexity of such a simple daemon.

* The polling interval is 10 seconds. Is this too long/short for some other
  projects?


## Testing
Testing can be accomplished via automated or manual testing to verify that:

* The presence state that is reported by GpioCableSevice is matching the result
  from linux gpioget command.

* Given the sdr name from `ipmitool sdr list event`, say cable[0-1], make sure
  the zeroth bit is matching the presence state of object
  `/xyz/openbmc_project/inventory/item/cable0` on the dbus and the first bit is
  matching the state of object `/xyz/openbmc_project/inventory/item/cable`.

* Unindex items from the config should have its own sdr ID. For example,
  `/xyz/openbmc_project/inventory/item/cable` should not be grouped to another
  group. Meanwhile, only the zero bit is used to report the presence state.

* The state of the cable presence need to change with 10 seconds once a cable is
  plug or unplug from the system.
