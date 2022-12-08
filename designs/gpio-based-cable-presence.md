## GPIO-based Cable Presence Detection

Author: Chu Lin (linchuyuan@google.com)

Created: 2021-07-29

## Problem Description

The intent of this new daemon design is to report GPIO-based cable presence
status. IPMI currently doesn't support grouping discrete values and it could at
most support 255 sensor IDs. On a systems with 25 cables, and 255 sensor IDs,
taking up one sensor per slot would use 10% of the available space, and be a
huge waste. Therefore, we need a solution to pack multiple presence states into
less SDR IDs.

## Background and References

1. https://www.intel.com/content/www/us/en/products/docs/servers/ipmi/ipmi-second-gen-interface-spec-v2-rev1-1.html
2. https://www.dmtf.org/sites/default/files/Redfish_Cable_Management_Proposal_WIP_04-2021.pdf

## Requirements

1. The openbmc IPMI interface should support exposing cable presence states.
2. The OpenBMC dbus interface should be compatible with redfish

## Proposed Design

Inventory.Item.Cable will be introduced as a new dbus interface for querying
cable objects. We will also need to introduce a new dbus daemon to collect
Presence states. This new daemon will resIDe in openbmc/dbus-sensors. Its
runtime configuration will be provIDed by EntityManager during startup. A
proposed config file looks like the following:

```
{
  "Exposes": [
    {
      "Name": "cable0",
      "GpioLine": "osfp0",
      "Polarity": "active_low",
      "Type": "GPIOCableSensing",
      "FaultLedGroup": [
        "attention"
      ]
    },
    {
      "Name": "cable1",
      "GpioLine": "osfp1",
      "Polarity": "active_high",
      "Type": "GPIOCableSensing",
      "FaultLedGroup": [
        "attention"
      ]
    }
  ]
}
```

The `Name` attribute is a unique name for each object from the config file. The
`GpioLine` is the gpio line label from the device tree. The `Polarity` should
tell the daemon whether it needs to revert the GPIO signal. Each entry from the
config would get translated into one object on the dbus. In addition, the daemon
polls the gpio presence signal for these objects in an interval of 10 seconds.
When the cable is not properly seated, the daemon will assert on the
corresponding fault led group. This should cause the corresponding led to blink
or to turn on base on how phosphor-led-manager is configured. No action is taken
if FaultLedGroup is empty.

On the IPMI sIDe, the presence states will be grouped into fewer SDR IDs in
order to save SDR IDs for ipmi. Given the following example,

```
└─/xyz
  └─/xyz/openbmc_project
    └─/xyz/openbmc_project/inventory
      └─/xyz/openbmc_project/inventory/item
        └─/xyz/openbmc_project/inventory/item/cable0
        └─/xyz/openbmc_project/inventory/item/cable1
```

In this example, we have both cable0 and cable1 and we expect to group them
together. The grouping handler first removes all the trailing index from the SDR
name. In this case, the SDR name is cable0 and cable1. After that, both cable0
and cable1 become cable. Therefore, objects with the same name after removing
the trailing index indicates that they need to be grouped. After grouping, the
new SDR name is cable[0-1]. The SDR name implies that this SDR has the presence
state for cable0 and cable1. Meanwhile, the bit position for cable0 is 0 and the
bit position for cable1 is 1. In the case of having more than 14 presence
states, the group handler will automatically jump to use a new SDR. For example,
if there are 20 cable indexed from 0 to 19, we shall see two SDRs. One is
cable0-13. One is cable[14-19]. If the object path is not indexed by the user,
it will take one SDR ID.

```
ipmitool sdr list event
# /xyz/openbmc_project/inventory/item/cdfp0 to
# /xyz/openbmc_project/inventory/item/cdfp3
cdfp[0-3]        | Event-Only        | ns
# /xyz/openbmc_project/inventory/item/osfp0 and
# /xyz/openbmc_project/inventory/item/osfp1
osfp[0-1]        | Event-Only        | ns
```

On the other hand, if the object name from the config file is not indexed. For
example, `/xyz/openbmc_project/inventory/item/cable`. The group handler will not
try to group it with anything and use 1 SDR ID for its presence state. See the
following for an example output.

```
ipmitool sdr list event
cdfp[0-3]        | Event-Only        | ns
# /xyz/openbmc_project/inventory/item/osfp
osfp             | Event-Only        | ns
osfp[1-2]        | Event-Only        | ns
```

During the fetch operation, once the ipmi daemon receives the client request. It
forwards the request to the dynamic ipmi commands. In the get method, the range
string is extracted from the tailing square bracket. The original dbus path can
be reconstructed given the index. Then, the SDR assertion value can be
constructed after collecting the OperationalStatus from the dbus from each
individual dbus objects.

The design is also compatible with the Redfish daemon. Redfish can call the
GetSubTree method to list all the instances under the cable interface. In
addition, Redfish doesn't have a limit of 256 SDR IDs. Therefore, no need to
implement grouping mechanism. It should work out of the box.

## Alternatives Considered

- Explore the option of reporting the presence state via type12 record instead
  of discrete sensors. The challenge here is that type12 record also has a limit
  of 255 IDs. Meanwhile, it doesn't support stacking multiple presence states in
  one ID.

- We could also let the user define how to group different cables from config
  file. However, this requires users to have a deep understanding of how ipmid
  works. It is better if the ipmi daemon just takes care of this.

- Instead of having a sleep-poll mechanism, it is better to have a event
  listener on the gpio signals. However, the gpio_event_wait api requires all
  the lines to come from the same chip. See
  https://github.com/brgl/libgpiod/blob/cc23ef61f66d5a9055b0e9fbdcfe6744c8a637ae/lib/core.c#L993
  We could spawn threads to listen to each chips but I don't think we should
  increase the complexity of such a simple daemon.

- The polling interval is 10 seconds. Is this too long/short for some other
  projects? For now, let's make it a compiler option.

- IPMI name string can only be 16 characters. Using square brackets would take
  away 2 chars. However, using square brackets gives a better indication for the
  start of the index string. Hence, the index string will be enclosed by square
  bracket.

## Testing

Testing can be accomplished via automated or manual testing to verify that:

- The presence state that is reported by GpioCableSevice is matching the result
  from linux gpioget command.

- Given the SDR name from `ipmitool sdr list event`, say cable[0-1], make sure
  the zeroth bit is matching the presence state of object
  `/xyz/openbmc_project/inventory/item/cable0` on the dbus and the first bit is
  matching the state of object `/xyz/openbmc_project/inventory/item/cable`.

- Unindex items from the config should have its own SDR ID. For example,
  `/xyz/openbmc_project/inventory/item/cable` should not be grouped to another
  group. Meanwhile, only the zero bit is used to report the presence state.

- The state of the cable presence need to change with 10 seconds once a cable is
  plug or unplug from the system.
