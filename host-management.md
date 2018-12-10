Host Management with OpenBMC
============================

This document describes the host-management interfaces of the OpenBMC object
structure, accessible over REST.

Inventory
---------

The system inventory structure is under the `/xyz/openbmc_project/inventory` hierarchy.

In OpenBMC the inventory is represented as a path which is hierarchical to the
physical system topology. Items in the inventory are referred to as inventory
items and are not necessarily FRUs (field-replaceable units). If the system
contains one chassis, a motherboard, and a CPU on the motherboard, then the
path to that inventory item would be:

   inventory/system/chassis0/motherboard0/cpu0

The properties associated with an inventory item are specific to that item.
Some common properties are:

 * `Version`: A code version associated with this item.
 * `Present`: Indicates whether this item is present in the system (True/False).
 * `Functional`: Indicates whether this item is functioning in the system (True/False).

The usual `list` and `enumerate` REST queries allow the system inventory
structure to be accessed. For example, to enumerate all inventory items and
their properties:

    curl -b cjar -k https://${bmc}/xyz/openbmc_project/inventory/enumerate

To list the properties of one item:

    curl -b cjar -k https://${bmc}/xyz/openbmc_project/inventory/system/chassis/motherboard

Sensors
-------

The system sensor structure is under the `/xyz/openbmc_project/sensors`
hierarchy.

This interface allows monitoring of system attributes like temperature or
altitude, and are represented similar to the inventory, by object paths under
the top-level `sensors` object name. The path categorizes the sensor and shows
what the sensor represents, but does not necessarily represent the physical
topology of the system.

For example, all temperature sensors are under `sensors/temperature`. CPU
temperature sensors would be `sensors/temperature/cpu[n]`.

These are some common properties:

 * `Value`: Current value of the sensor
 * `Unit`: Unit of the value and "Critical" and "Warning" values
 * `Scale`: The scale of the value and "Critical" and "Warning" values
 * `CriticalHigh` & `CriticalLow`: Sensor device upper/lower critical threshold
bound
 * `CriticalAlarmHigh` & `CriticalAlarmLow`: True if the sensor has exceeded the
critical threshold bound
 * `WarningHigh` & `WarningLow`: Sensor device upper/lower warning threshold
bound
 * `WarningAlarmHigh` & `WarningAlarmLow`: True if the sensor has exceeded the
warning threshold bound

A temperature sensor might look like:

    curl -b cjar -k https://${bmc}/xyz/openbmc_project/sensors/temperature/pcie
    {
      "data": {
        "CriticalAlarmHigh": 0,
        "CriticalAlarmLow": 0,
        "CriticalHigh": 70000,
        "CriticalLow": 0,
        "Scale": -3,
        "Unit": "xyz.openbmc_project.Sensor.Value.Unit.DegreesC",
        "Value": 28187,
        "WarningAlarmHigh": 0,
        "WarningAlarmLow": 0,
        "WarningHigh": 60000,
        "WarningLow": 0
      },
      "message": "200 OK",
      "status": "ok"
    }

Note the value of this sensor is 28.187C (28187 * 10^-3).

Unlike IPMI, there are no "functional" sensors in OpenBMC; functional states are
represented in the inventory.

To enumerate all sensors in the system:

    curl -b cjar -k https://${bmc}/xyz/openbmc_project/sensors/enumerate

List properties of one inventory item:

    curl -b cjar -k https://${bmc}/xyz/openbmc_project/sensors/temperature/ambient

Event Logs
----------

The event log structure is under the `/xyz/openbmc_project/logging/entry`
hierarchy. Each event is a separate object under this structure, referenced by
number.

BMC and host firmware on POWER-based servers can report event logs to the BMC.
Typically, these event logs are reported in cases where host firmware cannot
start the OS, or cannot reliably log to the OS.

The properties associated with an event log are as follows:

 * `Message`: The type of event log (e.g. "xyz.openbmc_project.Inventory.Error.NotPresent").
 * `Resolved` : Indicates whether the event has been resolved.
 * `Severity`: The level of problem ("Info", "Error", etc.).
 * `Timestamp`: The date of the event log in epoch time.
 * `associations`: A URI to the failing inventory part.

To list all reported event logs:

    $ curl -b cjar -k https://${bmc}/xyz/openbmc_project/logging/entry/
    {
      "data": [
        "/xyz/openbmc_project/logging/entry/3",
        "/xyz/openbmc_project/logging/entry/2",
        "/xyz/openbmc_project/logging/entry/1",
        "/xyz/openbmc_project/logging/entry/7",
        "/xyz/openbmc_project/logging/entry/6",
        "/xyz/openbmc_project/logging/entry/5",
        "/xyz/openbmc_project/logging/entry/4"
      ],
      "message": "200 OK",
      "status": "ok"
    }

To read a specific event log:

    $ curl -b cjar -k https://${bmc}/xyz/openbmc_project/logging/entry/1
    {
      "data": {
        "AdditionalData": [
          "CALLOUT_INVENTORY_PATH=/xyz/openbmc_project/inventory/system/chassis/powersupply0",
          "_PID=1136"
        ],
        "Id": 1,
        "Message": "xyz.openbmc_project.Inventory.Error.NotPresent",
        "Resolved": 0,
        "Severity": "xyz.openbmc_project.Logging.Entry.Level.Error",
        "Timestamp": 1512154612660,
        "associations": [
          [
            "callout",
            "fault",
            "/xyz/openbmc_project/inventory/system/chassis/powersupply0"
          ]
        ]
      },
      "message": "200 OK",
      "status": "ok"
    }

To delete an event log (log 1 in this example), call the `delete` method on the event:

    curl -b cjar -k -H "Content-Type: application/json" -X POST \
        -d '{"data" : []}' \
        https://${bmc}/xyz/openbmc_project/logging/entry/1/action/Delete

To clear all event logs, call the top-level `deleteAll` method:

    curl -b cjar -k -H "Content-Type: application/json" -X POST \
        -d '{"data" : []}' \
        https://${bmc}/xyz/openbmc_project/logging/action/deleteAll

Host Boot Options
-----------------

With OpenBMC, the Host boot options are stored as D-Bus properties under the
`control/host0/boot` path. Properties include
[`BootMode`](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Control/Boot/Mode.interface.yaml)
and
[`BootSource`](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Control/Boot/Source.interface.yaml).


Host State Control
------------------

The host can be controlled through the `host` object. The object implements a
number of actions including power on and power off. These correspond to the IPMI
`power on` and `power off` commands.

Assuming you have logged in, the following will power on the host:

```
curl -c cjar -b cjar -k -H "Content-Type: application/json" -X PUT \
  -d '{"data": "xyz.openbmc_project.State.Host.Transition.On"}' \
  https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
```

To power off the host:

```
curl -c cjar -b cjar -k -H "Content-Type: application/json" -X PUT \
  -d '{"data": "xyz.openbmc_project.State.Host.Transition.Off"}' \
  https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
```

To issue a hard power off (accomplished by powering off the chassis):

```
curl -c cjar -b cjar -k -H "Content-Type: application/json" -X PUT \
  -d '{"data": "xyz.openbmc_project.State.Chassis.Transition.Off"}' \
  https://${bmc}/xyz/openbmc_project/state/chassis0/attr/RequestedPowerTransition
```

To reboot the host:

```
curl -c cjar -b cjar -k -H "Content-Type: application/json" -X PUT \
  -d '{"data": "xyz.openbmc_project.State.Host.Transition.Reboot"}' \
  https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
```


More information about Host State Management can be found here:
https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/xyz/openbmc_project/State

Host Clear GARD
================

On OpenPOWER systems, the host maintains a record of bad or non-working
components on the GARD partition. This record is referenced by the host on
subsequent boots to determine which parts should be ignored.

The BMC implements a function that simply clears this partition. This function
can be called as follows:

  * Method 1: From the BMC command line:

      ```
      busctl call org.open_power.Software.Host.Updater \
        /org/open_power/control/gard \
        xyz.openbmc_project.Common.FactoryReset Reset
      ```

  * Method 2: Using the REST API:

      ```
      curl -b cjar -k -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/org/open_power/control/gard/action/Reset
      ```

Implementation: https://github.com/openbmc/openpower-pnor-code-mgmt
