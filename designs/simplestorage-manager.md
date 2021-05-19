# SimpleStorage Manager design
Author: JunLin Chen

Primary assignee: JunLin Chen

Created: 2021-05-11

## Problem description

BMC has simple storage devices available. It would be useful to provide the system configuration information of these resources to users.

## Background and references

SATA device presence can be monitored via GPIO status:
It can use following modules to easier get GPIO status
[phosphor-gpio-monitor](https://github.com/openbmc/phosphor-gpio-monitor)

## Requirements

Gathering the status and information of the simple storage device.
Provide and maintain the interface for another service, e.g. bmcweb.

## Proposed design

This feature is intended for caching the Simple Storage resources and maintaining the D-Bus interfaces and properties.

**BLOCK DIAGRAM**

```
+-----------------------------+  +-----------------------------+  +---------------+
|    PHOSPHOR-GPIO-MONITOR    |  |    SIMPLESTORAGE MANAGER    |  |     BMCWEB    |
|                             |  |                             |  |               |
+--------------+--------------+  +--------------+--------------+  +-------+-------+
               |                                |                         |
               |                                |                         |
               |                                |                         |
               |    GATHER GPIO INFORMATION     |                         |
               +------------------------------->+----------+              |
               |                                |  CREATE  |              |
               |REGISTER PROPERTIESCHANGE SIGNAL|  D-BUS   |              |
               +<-------------------------------+<---------+              |
               |                                |                         |
+---------------------+------------------------------------------------------------------
| GPIO STATUS CHANGE  |                         |                         |
+---------------------+                         |                         |
               |                                |                         |
               |           SEND SIGNAL          |                         |
               +------------------------------->+----------+              |
               |                                |  UPDATE  |              |
               |                                |  D-BUS   |              |
               |                                +<---------+              |
               |                                |                         |
------------------------+----------------------------------------------------------------
               |        | REDFISH GET REQUEST   |                         |
               |        +-----------------------+                         |
               |                                |                         |
               |                                |                         |
               |                                |  GET DEVICE INFORMATION |
               |                                |       FROM D-BUS        |
               |                                +------------------------>+
               |                                |                         |
               |                                |                         |
-----------------------------------------------------------------------------------------
               |                                |                         |
               |                                |                         |

```



**phosphor-gpio-monitor**
The phosphor-gpio-monitor must create D-Bus based on the following standard.
1. Each device must base on the below object path.
`/xyz/openbmc_project/inventory/system/<SimpleStorageController>/<DeviceId>/<DeviceStatus>`

| PROPERTY NAME              | VALUE       | DESCRIPTION                       |
|----------------------------|-------------|-----------------------------------|
| SimpleStorageController | (user defined) | Device controller, e.g. SATA,USB and so on. |
| DeviceId | (user defined) | The GPIOs of the same device must use the same device id |
| DeviceStatus | presence/powergood | The GPIO name of this device, each device must have “presence” status. |
2. Add extra D-bus interface to provide SimpleStorage Manager to search devices.
`xyz.openbmc_project.Inventory.Item.StorageDevice`
Example:
```
NAME                                              TYPE      SIGNATURE RESULT/VALUE FLAGS
org.freedesktop.DBus.Introspectable               interface -         -             -
.Introspect                                       method    -         s             -
org.freedesktop.DBus.Peer                         interface -         -             -
.GetMachineId                                     method    -         s             -
.Ping                                             method    -         -             -
org.freedesktop.DBus.Properties                   interface -         -             -
.Get                                              method    ss        v             -
.GetAll                                           method    s         a{sv}         -
.Set                                              method    ssv       -             -
.PropertiesChanged                                signal    sa{sv}as  -             -
xyz.openbmc_project.Inventory.Item                interface -         -             -
.Present                                          property  b         true          emits-change writable
.PrettyName                                       property  s         "sata0_prsnt" emits-change writable
xyz.openbmc_project.Inventory.Item.StorageDevice  interface -         -             -
```

**SimpleStorage Manager D-Bus interface**
The following D-Bus names will be created for simplestorage manager service.

    Service name     -- xyz.openbmc_project.SimpleStorage.Manager

    Obj path name    -- /xyz/openbmc_project/inventory/<SimpleStorageController>_<ContorllerId>/<DeviceId>

    Interface name   -- xyz.openbmc_project.Inventory.Item.SimpleStorage
`<SimpleStorageController>` and `<DeviceId>` will inherit the GPIO object path from phosphor-gpio-monitor.
`<ControllerId>` will be 0, 1, 2,... for the index of controller, which will be service create.
e.g. `/xyz/openbmc_project/inventory/sata_0/1`


Example:
```
NAME                                              TYPE      SIGNATURE RESULT/VALUE FLAGS
org.freedesktop.DBus.Introspectable               interface -         -             -
.Introspect                                       method    -         s             -
org.freedesktop.DBus.Peer                         interface -         -             -
.GetMachineId                                     method    -         s             -
.Ping                                             method    -         -             -
org.freedesktop.DBus.Properties                   interface -         -             -
.Get                                              method    ss        v             -
.GetAll                                           method    s         a{sv}         -
.Set                                              method    ssv       -             -
.PropertiesChanged                                signal    sa{sv}as  -             -
xyz.openbmc_project.Inventory.Item.SimpleStorage  interface -         -             -
.CapacityBytes                                    property  s         ""            emits-change writable
.Manufacturer                                     property  s         ""            emits-change writable
.Model                                            property  s         ""            emits-change writable
.Name                                             property  s         "SATA_HDD"    emits-change writable
.State                                            property  s         "Enable"      emits-change writable
.Health                                           property  s         "OK"          emits-change writable
```
The `xyz.openbmc_project.Inventory.Item.SimpleStorage` interface should have the properties on redfish simplestorage schemas, Link, and can be parsed by bmcweb.


## Alternatives considered

It can directly to parser the D-Bus interface from another service, such the device GPIO presence from phosphor-gpio-monitor.
but it will cause bmcweb function too couple to the dbus path and hardly to expand in the future.

## Impacts

The D-bus object of simple storage GPIO needs to be add the specific D-bus interface for bmcweb getting.

## Testing

The D-bus path list should be created under xyz.openbmc_project.SimpleStorage.Manager



