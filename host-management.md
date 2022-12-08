# Host Management with OpenBMC

This document describes the host-management interfaces of the OpenBMC object
structure, accessible over REST.

Note: Authentication

See the details on authentication at
[REST-cheatsheet](https://github.com/openbmc/docs/blob/master/REST-cheatsheet.md#establish-rest-connection-session).

This document uses token based authentication method:

```
$ export bmc=xx.xx.xx.xx
$ export token=`curl -k -H "Content-Type: application/json" -X POST https://${bmc}/login -d '{"username" :  "root", "password" :  "0penBmc"}' | grep token | awk '{print $2;}' | tr -d '"'`
$ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/...
```

## Inventory

The system inventory structure is under the `/xyz/openbmc_project/inventory`
hierarchy.

In OpenBMC the inventory is represented as a path which is hierarchical to the
physical system topology. Items in the inventory are referred to as inventory
items and are not necessarily FRUs (field-replaceable units). If the system
contains one chassis, a motherboard, and a CPU on the motherboard, then the path
to that inventory item would be:

`inventory/system/chassis0/motherboard0/cpu0`

The properties associated with an inventory item are specific to that item. Some
common properties are:

- `Version`: A code version associated with this item.
- `Present`: Indicates whether this item is present in the system (True/False).
- `Functional`: Indicates whether this item is functioning in the system
  (True/False).

The usual `list` and `enumerate` REST queries allow the system inventory
structure to be accessed. For example, to enumerate all inventory items and
their properties:

    $ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/inventory/enumerate

To list the properties of one item:

    $ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/inventory/system/chassis/motherboard

## Sensors

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

- `Value`: Current value of the sensor
- `Unit`: Unit of the value and "Critical" and "Warning" values
- `Scale`: The scale of the value and "Critical" and "Warning" values
- `CriticalHigh` & `CriticalLow`: Sensor device upper/lower critical threshold
  bound
- `CriticalAlarmHigh` & `CriticalAlarmLow`: True if the sensor has exceeded the
  critical threshold bound
- `WarningHigh` & `WarningLow`: Sensor device upper/lower warning threshold
  bound
- `WarningAlarmHigh` & `WarningAlarmLow`: True if the sensor has exceeded the
  warning threshold bound

A temperature sensor might look like:

    $ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/sensors/temperature/ocp_zone
    {
      "data": {
        "CriticalAlarmHigh": false,
        "CriticalAlarmLow": false,
        "CriticalHigh": 65000,
        "CriticalLow": 0,
        "Functional": true,
        "MaxValue": 0,
        "MinValue": 0,
        "Scale": -3,
        "Unit": "xyz.openbmc_project.Sensor.Value.Unit.DegreesC",
        "Value": 34625,
        "WarningAlarmHigh": false,
        "WarningAlarmLow": false,
        "WarningHigh": 63000,
        "WarningLow": 0
      },
      "message": "200 OK",
      "status": "ok"
    }

Note the value of this sensor is 34.625C (34625 \* 10^-3).

Unlike IPMI, there are no "functional" sensors in OpenBMC; functional states are
represented in the inventory.

To enumerate all sensors in the system:

    $ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/sensors/enumerate

List properties of one inventory item:

    $ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/sensors/temperature/outlet

## Event Logs

The event log structure is under the `/xyz/openbmc_project/logging/entry`
hierarchy. Each event is a separate object under this structure, referenced by
number.

BMC and host firmware on POWER-based servers can report event logs to the BMC.
Typically, these event logs are reported in cases where host firmware cannot
start the OS, or cannot reliably log to the OS.

The properties associated with an event log are as follows:

- `Message`: The type of event log (e.g.
  "xyz.openbmc_project.Inventory.Error.NotPresent").
- `Resolved` : Indicates whether the event has been resolved.
- `Severity`: The level of problem ("Info", "Error", etc.).
- `Timestamp`: The date of the event log in epoch time.
- `Associations`: A URI to the failing inventory part.

To list all reported event logs:

    $ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/logging/entry
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

    $ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/logging/entry/1
    {
      "data": {
        "AdditionalData": [
          "_PID=183"
        ],
        "Id": 1,
        "Message": "xyz.openbmc_project.Common.Error.InternalFailure",
        "Purpose": "xyz.openbmc_project.Software.Version.VersionPurpose.BMC",
        "Resolved": false,
        "Severity": "xyz.openbmc_project.Logging.Entry.Level.Error",
        "Timestamp": 1563191362822,
        "Version": "2.8.0-dev-132-gd1c1b74-dirty",
        "associations": []
      },
      "message": "200 OK",
      "status": "ok"
    }

To delete an event log (log 1 in this example), call the `Delete` method on the
event:

    $ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST -d '{"data" : []}' https://${bmc}/xyz/openbmc_project/logging/entry/1/action/Delete

To clear all event logs, call the top-level `DeleteAll` method:

    $ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST -d '{"data" : []}' https://${bmc}/xyz/openbmc_project/logging/action/DeleteAll

## Host Boot Options

With OpenBMC, the Host boot options are stored as D-Bus properties under the
`control/host0/boot` path. Properties include
[`BootMode`](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Control/Boot/Mode.interface.yaml),
[`BootSource`](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Control/Boot/Source.interface.yaml)
and if the host is based on x86 CPU also
[`BootType`](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Control/Boot/Type.interface.yaml).

- Set boot mode:

  ```
   $ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT https://${bmc}/xyz/openbmc_project/control/host0/boot/attr/BootMode -d '{"data": "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular"}'
  ```

- Set boot source:

  ```
  $ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT https://${bmc}/xyz/openbmc_project/control/host0/boot/attr/BootSource -d '{"data": "xyz.openbmc_project.Control.Boot.Source.Sources.Default"}'
  ```

- Set boot type (valid only if host is based on the x86 CPU):

  ```
  $ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT https://${bmc}/xyz/openbmc_project/control/host0/boot/attr/BootType -d '{"data": "xyz.openbmc_project.Control.Boot.Type.Types.EFI"}'
  ```

Also there are boolean
[`Enable`](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Object/Enable.interface.yaml)
properties that control if the boot source override is persistent or one-time,
and if the override is enabled or not.

- Set boot override one-time flag:

  ```
  $ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT https://${bmc}/xyz/openbmc_project/control/host0/boot/one_time/attr/Enabled -d '{"data": "true"}'
  ```

- Enable boot override:

  ```
  $ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT https://${bmc}/xyz/openbmc_project/control/host0/boot/attr/Enabled -d '{"data": "true"}'
  ```

## Host State Control

The host can be controlled through the `host` object. The object implements a
number of actions including power on and power off. These correspond to the IPMI
`power on` and `power off` commands.

Assuming you have logged in, the following will power on the host:

```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -d '{"data": "xyz.openbmc_project.State.Host.Transition.On"}' -X PUT https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
```

To power off the host:

```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -d '{"data": "xyz.openbmc_project.State.Host.Transition.Off"}' -X PUT https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
```

To issue a hard power off (accomplished by powering off the chassis):

```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d '{"data":"xyz.openbmc_project.State.Chassis.Transition.Off"}' https://${bmc}//xyz/openbmc_project/state/chassis0/attr/RequestedPowerTransition
```

To reboot the host:

```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d '{"data":"xyz.openbmc_project.State.Host.Transition.Reboot"}' https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
```

More information about Host State Management can be found here:
https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/yaml/xyz/openbmc_project/State

## Host Clear GARD

On OpenPOWER systems, the host maintains a record of bad or non-working
components on the GARD partition. This record is referenced by the host on
subsequent boots to determine which parts should be ignored.

The BMC implements a function that simply clears this partition. This function
can be called as follows:

- Method 1: From the BMC command line:

  ```
  busctl call org.open_power.Software.Host.Updater \
    /org/open_power/control/gard \
    xyz.openbmc_project.Common.FactoryReset Reset
  ```

- Method 2: Using the REST API:

  ```
  $ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST -d '{"data":[]}' https://${bmc}/org/open_power/control/gard/action/Reset
  ```

Implementation: https://github.com/openbmc/openpower-pnor-code-mgmt

## Host Watchdog

The host watchdog service is responsible for ensuring the host starts and boots
within a reasonable time. On host start, the watchdog is started and it is
expected that the host will ping the watchdog via the inband interface
periodically as it boots. If the host fails to ping the watchdog within the
timeout then the host watchdog will start a systemd target to go to the quiesce
target. System settings will then determine the recovery behavior from that
state, for example, attempting to reboot the system.

The host watchdog utilizes the generic [phosphor-watchdog][1] repository. The
host watchdog service provides 2 files as configuration options into
phosphor-watchdog:

    /lib/systemd/system/phosphor-watchdog@poweron.service.d/poweron.conf
    /etc/default/obmc/watchdog/poweron

`poweron.conf` contains the "Conflicts" relationships to ensure the watchdog
service is stopped at the correct times. `poweron` contains the required
information for phosphor-watchdog (more information on these can be found in the
[phosphor-watchdog][1] repository).

The 2 service files involved with the host watchdog are:

    phosphor-watchdog@poweron.service
    obmc-enable-host-watchdog@0.service

`phosphor-watchdog@poweron` starts the host watchdog service and
`obmc-enable-host-watchdog` starts the watchdog timer. Both are run as a part of
the `obmc-host-startmin@.target`. Service dependencies ensure the service is
started before the enable is called.

The default watchdog timeout can be found within the [dbus interface
specification][2] (Interval property).

The host controls the watchdog timeout and enable/disable once it starts.

[1]: https://github.com/openbmc/phosphor-watchdog
[2]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/Watchdog.interface.yaml
