Host Management with OpenBMC
============================

This document describes the host-management interfaces of the OpenBMC object
structure, accessible over REST.

Inventory
---------

The system inventory structure is under the `/org/openbmc/inventory` hierarchy.

In OpenBMC the inventory is represented as a path which is hierarchical to the
physical system topology. Items in the inventory are referred to as inventory
items and are not necessarily FRUs. If the system contains one chassis, a
motherboard, and a CPU on the motherboard, then the path to that inventory item
would be:

   inventory/system/chassis0/motherboard0/cpu0

The properties associated with an inventory item are specific to that item with
the addition of these common properties:

 * `version`: a code version associated with this item
 * `fault`: indicates whether this item has reported an error condition (True/False)
 * `present`: indicates whether this item is present in the system (True/False)

The usual `list` and `enumerate` REST queries allow the system inventory
structure to be accessed. For example, to enumerate all inventory items and
their properties:

    curl -b cjar -k https://bmc/org/openbmc/inventory/enumerate

To list the properties of one item:

    curl -b cjar -k https://bmc/org/openbmc/inventory/system/chassis/motherboard

Sensors
-------

The system inventory structure is under the `/org/openbmc/sensors` hierarchy.

This interface allows monitoring of system attributes like temperature or
altitude, and are represented similar to the inventory, by object paths under
the top-level `sensors` object name. The path categorizes the sensor and shows
what the sensor represents, but does not necessarily represent the physical
topology of the system.

For example, all temperature sensors are under `sensors/temperature`. CPU
temperature sensors would be `sensors/temperature/cpu[n]`.

These are the common properties associated all sensors:

 * `value`: current value of sensor
 * `units`: units of value
 * `warning_upper` & `warning_lower`: upper & lower threshold for a warning error
 * `critical_upper` & `critical_lower`: upper & lower threshold for a critical error
 * `threshold_state`: current threshold state (normal, warning, critical)
 * `worst_threshold_state`: highest threshold state seen for all time. There is
   a REST call to reset this state to normal.

Unlike IPMI, there are no "functional" sensors in OpenBMC; functional states are
represented in the inventory.

To enumerate all sensors in the system:

    curl -b cjar -k https://bmc/org/openbmc/sensors/enumerate

List properties of one inventory item:

    curl -b cjar -k https://bmc/org/openbmc/sensors/temperature/ambient

Event Logs
----------

The event log structure is under the `/org/openbmc/records/events` hierarchy.
Each event is a separate object under this structure, referenced by number.

BMC and host firmware on POWER-based servers can report logs to the BMC.
Typically, these logs are reported in cases where host firmware cannot start the
OS, or cannot reliably log to the OS.

The properties associated with an error log are as follows:

 * `Association`: a uri to the failing inventory part
 * `Message`: An English text string indicating the failure
 * `Reported By`: Firmware region that created the log ('Host', 'BMC')
 * `Severity`: level of problem (Info, Predictive Error, etc)
 * `Development Data`: Data dump for use by Development to analyze the failure

To list all reported events:

    $ curl -b cjar -k https://bmc/org/openbmc/records/events/
    {
      "data": [
        "/org/openbmc/records/events/1",
        "/org/openbmc/records/events/2",
        "/org/openbmc/records/events/3",
      ],
      "message": "200 OK",
      "status": "ok"
    }

To read a specific event log:

    $ curl -b cjar -k https://bmc/org/openbmc/records/events/1
    {
      "data": {
        "association": "/org/openbmc/inventory/system/systemevent",
        "debug_data": [ ... ],
        "message": "A systemevent has experienced a unrecoverable error",
        "reported_by": "Host",
        "severity": "unrecoverable error",
        "time": "2016:05:04 01:43:57"
      },
      "message": "200 OK",
      "status": "ok"
    }

To delete an event log (log 1 in his example), call the `delete` method on the event:

    curl -b cjar -k -H "Content-Type: application/json" -X POST \
        -d '{"data" : []}' \
        https://bmc/org/openbmc/records/events/1/action/delete

To clear all logs, call the top-level `clear` method:

    curl -b cjar -k -H "Content-Type: application/json" -X POST \
        -d '{"data" : []}' \
        https://bmc/org/openbmc/records/events/action/clear

BIOS boot options
-----------------

With OpenBMC, the BIOS boot options are stored as dbus properties under the
`settings/host0` path, in the `boot_flags` property. Their format adheres to the
Boot Options Parameters table specified in the IPMI Specification Document,
section 28.13.

Each boot parameter is represented by an individual property, and their hex
value is displayed in a string format. For example, a boot flags parameter value
of `0x8014000000` would be stored as a `8014000000` string in the
`settings/host0/boot_flags` property.

Host power control
------------------

The host can be controlled through the `chassis` object. It implements a number
of actions including powerOn and powerOff. These correspond to the IPMI
`chassis power on` and `chassis power off` commands.

Assuming you have logged in, the following will issue a POST with an empty data
payload that powers the host on:

```
curl -c cjar -b cjar -k -H "Content-Type: application/json" -X POST \
   -d '{"data": []}'  https://bmc/org/openbmc/control/chassis0/action/powerOn
```

Other actions available are:

 * `setIdentify` / `clearIdentify`
 * `powerOff`
 * `softPowerOff`
 * `reboot`
 * `softReboot`
 * `getPowerState`
