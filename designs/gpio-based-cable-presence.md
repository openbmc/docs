## GPIO-based Cable Presence Detection
Author:
  Chu Lin
\
Primary assignee:
  Chu Lin
\
Created:
  2021-07-29
## Problem Description
The intent of this new daemon design is to report GPIO-based cable presence status via ipmi. The challenge here is to stack the presence state into fewer sdrs in the ipmi world so that it doesn't take too many sdr slots. However, in the Redfish world, it has unlimited sdr slots. Therefore, all the necessary implementation to stack the presence state into fewer sdrs needs to go to ipmid so that we can keep the new GPIO-based cable presence daemon "Redfish" proof.

## Daemon Implementation
The solo task of the dbus daemon is to collect Presence state and post it to dbus for redfish/ipmi to query. Its runtime configuration will be provided by EntityManager during startup. A proposed config file looks like the following:
```
{
  "Exposes": [
    {
      "Name": "special_purpose_cable1",
      "CableType": "special_purpose_cable",
      "IsActiveLow": true,
      "Type": "GPIOSensing"
    },
    {
      "Name": "special_purpose_cable2",
      "CableType": "special_purpose_cable",
      "IsActiveLow": true,
      "Type": "GPIOSensing"
    }
  ]
}
```
The `Name`attribute needs to match the GPIO pin label. The `CableType` attribute is like a group by property for ipmi. In the ipmi world, we will group the different cable presence states with the same `CableType` into one sdr. The `IsActiveLow` will tell the daemon whether it needs to reverse the GPIO signal.

There will be a corresponding dbus object created per each entry in the expose list. During runtime, the daemon will follow this config and poll the presence signal in an interval of 5 seconds.

## Presence State Stacking
In order to save sdr slots, we will need to stack the presence states. When ipmi queries from dbus, it needs to group multiple objects with the same CableType into one sdr. In the above example, when ipmi queries from the cable interface, it sees two objects with the same CableType.
```
			"/xyz/openbmc_project/inventory/item/cable/special_purpose_cable/special_purpose_cable1" : {
				"xyz.openbmc_project.GPIOSensing" : [
					"org.freedesktop.DBus.Introspectable",
					"org.freedesktop.DBus.Peer",
					"org.freedesktop.DBus.Properties",
					"xyz.openbmc_project.Inventory.Decorator.AssetTag",
					"xyz.openbmc_project.Inventory.Decorator.CableType",
					"xyz.openbmc_project.Inventory.Item.Cable",
					"xyz.openbmc_project.State.Decorator.OperationalStatus"
				]
			},
			"/xyz/openbmc_project/inventory/item/cable/special_purpose_cable/special_purpose_cable2" : {
				"xyz.openbmc_project.GPIOSensing" : [
					"org.freedesktop.DBus.Introspectable",
					"org.freedesktop.DBus.Peer",
					"org.freedesktop.DBus.Properties",
					"xyz.openbmc_project.Inventory.Decorator.AssetTag",
					"xyz.openbmc_project.Inventory.Decorator.CableType",
					"xyz.openbmc_project.Inventory.Item.Cable",
					"xyz.openbmc_project.State.Decorator.OperationalStatus"
				]
			}
```
If we just leave the entries like this, it will take two sdrs slots. Therefore, we will apply a little hack here to rearrange things a little bit.
```
"/xyz/openbmc_project/inventory/item/cable/special_purpose_cable" : {
				"xyz.openbmc_project.GPIOSensing" : [
					"org.freedesktop.DBus.Introspectable",
					"org.freedesktop.DBus.Peer",
					"org.freedesktop.DBus.Properties",
					"xyz.openbmc_project.Inventory.Decorator.AssetTag",
					"xyz.openbmc_project.Inventory.Decorator.CableType",
					"xyz.openbmc_project.Inventory.Item.Cable",
					"xyz.openbmc_project.State.Decorator.OperationalStatus",
                    "/xyz/openbmc_project/inventory/item/cable/special_purpose_cable/special_purpose_cable1",
                    "/xyz/openbmc_project/inventory/item/cable/special_purpose_cable/special_purpose_cable2"
				]
			}
```
Note that we squezzed the real object path into the interface array.  The purpose of doing this is to make sure we still have a pointer to the real object. This is okay because we can still differentiate the interface and object by checking if it starts with `/`. Now, the `/xyz/openbmc_project/inventory/item/cable/special_purpose_cable` becomes an imaginary dbus object and it only takes 1 sdr slot. During read operation, we will first fetch the real object and sort it by alphabetical order. Then, we can read out the operation state and stack the bits into discrete sensor assertion bits.

## "Redfish" Proof
For the redfish daemon, it can query for all the objects that implement the cable interface and that is it. Each object that implements the cable interface should already have everything redfish needs to create a response.

## New Dbus Interfaces
There will be 2 new dbus interfaces. One is `xyz.openbmc_project.Inventory.Item.Cable`. This will allow ipmi and redfish to list all the cable instances. One is `xyz.openbmc_project.Inventory.Decorator.CableType`. This will allow ipmi to group different cables with the same CableType into one sdr. Both interfaces can be found in the redfish schema. Therefore, they are "Redfish" friendly.

## Testing
* Make sure the dbus daemon is able to detect GPIO signal.
* Make sure the dbus daemon is detecting the presence signal correctly (active low vs. active high).
* Make sure ipmi can group the cables correctly.

