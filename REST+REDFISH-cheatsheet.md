# OpenBMC User's Guide

##  1  OpenBMC REST API

The primary management interface for OpenBMC is REST. This document provides some basic structure and usage examples for the REST interface. The schema for the rest interface is directly defined by the OpenBMC D-Bus structure. Therefore, the objects, attributes and methods closely map to those in the D-Bus schema.For a quick explanation of HTTP verbs and how they relate to a RESTful API, see http://www.restapitutorial.com/lessons/httpmethods.html.

### Notes on authentication:

The original REST server, from the phosphor-rest-server repository, uses authentication handled by the curl cookie jar files. The bmcweb REST server can use the same cookie jar files for read-only REST methods like GET, but requires either an authentication token or the username and password passed in as part of the URL for non-read-only methods.

The phosphor-rest server will no longer be the default REST server after the 2.6 OpenBMC release.

### 1.1 Logging in

* Using just the cookie jar files for the phosphor-rest server:

  ```
  $ export bmc=xx.xx.xx.xx
  $ curl -c cjar -b cjar -k -H "Content-Type: application/json" -X POST https://${bmc}/login -d "{\"data\": [ \"root\", \"0penBmc\" ] }"
  ```

*  If passing in the username/password as part of the URL, no unique login call is required. The URL format is:

  ```
  <username>:<password>@<hostname>/<path>...
  ```
  
  For example:
  
  ```
  $ export bmc=xx.xx.xx.xx
  $ curl -k -X GET https://root:0penBmc@${bmc}/xyz/openbmc_project/list
  ```

*  Token based authentication.

  ```
  $ export bmc=xx.xx.xx.xx
  $ export token=`curl -k -H "Content-Type: application/json" -X POST https://${bmc}/login -d '{"username" :  "root", "password" :  "0penBmc"}' | grep token | awk '{print $2;}' | tr -d '"'`
  $ curl -k -H "X-Auth-Token: $token" https://$bmc/xyz/openbmc_project/...
  ```

The third method is recommended.

### 1.2 HTTP GET operations & URL structure

There are a few conventions on the URL structure of the OpenBMC rest interface. They are:

1. To query the attributes of an object, perform a GET request on the object name, with no trailing slash. For example:

```
$ curl -b cjar -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/inventory/system
{
"data": {
​    "AssetTag": "",
​    "BuildDate": "",
​    "Cached": true,
​    "FieldReplaceable": true,
​    "Manufacturer": "",
​    "Model": "",
​    "PartNumber": "",
​    "Present": true,
​    "PrettyName": "",
​    "SerialNumber": ""
  },
  "message": "200 OK",
  "status": "ok"
}
```



2. To query a single attribute, use the attr/<name> path. Using the system object from above, we can query just the Name value:

```
$ curl -b cjar -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/inventory/system/attr/Cached
{
  "data": true,
  "message": "200 OK",
  "status": "ok"
}
```



3. When a path has a trailing-slash, the response will list the sub objects of the URL. For example, using the same object path as above, but adding a slash:

```
$ curl -b cjar -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/
{
  "data": [
​    "/xyz/openbmc_project/Chassis",
​    "/xyz/openbmc_project/Hiomapd",
​    "/xyz/openbmc_project/Ipmi",
​    "/xyz/openbmc_project/certs",
​    "/xyz/openbmc_project/console",
​    "/xyz/openbmc_project/control",
​    "/xyz/openbmc_project/dump",
​    "/xyz/openbmc_project/events",
​    "/xyz/openbmc_project/inventory",
​    "/xyz/openbmc_project/ipmi",
​    "/xyz/openbmc_project/led",
​    "/xyz/openbmc_project/logging",
​    "/xyz/openbmc_project/network",
​    "/xyz/openbmc_project/object_mapper",
​    "/xyz/openbmc_project/sensors",
​    "/xyz/openbmc_project/software",
​    "/xyz/openbmc_project/state",
​    "/xyz/openbmc_project/time",
​    "/xyz/openbmc_project/user"
  ],
  "message": "200 OK",
  "status": "ok"
}
```

This shows that there are 19 children of the openbmc_project/ object: dump, software, control, network, logging,etc. This can be used with the base REST URL (ie., http://${bmc}/), to discover all objects in the hierarchy.

4. Performing the same query with /list will list the child objects *recursively*.

```
$ curl -b cjar -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/network/list
{
  "data": [
​    "/xyz/openbmc_project/network/config",
​    "/xyz/openbmc_project/network/config/dhcp",
​    "/xyz/openbmc_project/network/eth0",
​    "/xyz/openbmc_project/network/eth0/ipv4",
​    "/xyz/openbmc_project/network/eth0/ipv4/3b9faa36",
​    "/xyz/openbmc_project/network/eth0/ipv6",
​    "/xyz/openbmc_project/network/eth0/ipv6/ff81b6d6",
​    "/xyz/openbmc_project/network/eth1",
​    "/xyz/openbmc_project/network/eth1/ipv4",
​    "/xyz/openbmc_project/network/eth1/ipv4/3b9faa36",
​    "/xyz/openbmc_project/network/eth1/ipv4/66e63348",
​    "/xyz/openbmc_project/network/eth1/ipv6",
​    "/xyz/openbmc_project/network/eth1/ipv6/ff81b6d6",
​    "/xyz/openbmc_project/network/host0",
​    "/xyz/openbmc_project/network/host0/intf",
​    "/xyz/openbmc_project/network/host0/intf/addr",
​    "/xyz/openbmc_project/network/sit0",
​    "/xyz/openbmc_project/network/snmp",
​    "/xyz/openbmc_project/network/snmp/manager"
  ],
  "message": "200 OK",
  "status": "ok"
}
```

5. Adding /enumerate instead of /list will also include the attributes of the listed objects.

```
$ curl -b cjar -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/time/enumerate
{
  "data": {
​    "/xyz/openbmc_project/time/bmc": {
​      "Elapsed": 1563209492098739
​    },
​    "/xyz/openbmc_project/time/host": {
​      "Elapsed": 1563209492101678
​    },
​    "/xyz/openbmc_project/time/owner": {
​      "TimeOwner": "xyz.openbmc_project.Time.Owner.Owners.BMC"
​    },
​    "/xyz/openbmc_project/time/sync_method": {
​      "TimeSyncMethod": "xyz.openbmc_project.Time.Synchronization.Method.NTP"
​    }
  },
  "message": "200 OK",
  "status": "ok"
}
```

### 1.3 HTTP PUT operations

PUT operations are for updating an existing resource (an object or property), or for creating a new resource when the client already knows where to put it. These require a json formatted payload. To get an example of what that looks like:

```
$ curl -b cjar -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/state/host0 > host.json
```

```
$ cat host.json
{
  "data": {
​    "AttemptsLeft": 3,
​    "BootProgress": "xyz.openbmc_project.State.Boot.Progress.ProgressStages.Unspecified",
​    "CurrentHostState": "xyz.openbmc_project.State.Host.HostState.Off",
​    "OperatingSystemState": "xyz.openbmc_project.State.OperatingSystem.Status.OSStatus.Inactive",
​    "RequestedHostTransition": "xyz.openbmc_project.State.Host.Transition.Off"
  },
  "message": "200 OK",
  "status": "ok"
}
```

or

```
$ curl -b cjar -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition > requested_host.json
```

```
$ cat requested_host.json
{
  "data": "xyz.openbmc_project.State.Host.Transition.Off",
  "message": "200 OK",
  "status": "ok"
}
```

When turning around and sending these as requests, delete the message and status properties.

To make curl use the correct content type header use the -H option to specify that we're sending JSON data:

```
$ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d <json> <url>
```

A PUT operation on an object requires a complete object. For partial updates there is PATCH but that is not implemented yet. As a workaround individual attributes are PUTable.

For example, make changes to the requested_host.json file and do a PUT (upload):

```
$ cat requested_host.json
{
  "data": "xyz.openbmc_project.State.Host.Transition.Off",
  "message": "200 OK",
  "status": "ok"
}
```

```
$ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -T requested_host.json https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
```

Alternatively specify the json inline with -d:

```
$ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d '{"data": "xyz.openbmc_project.State.Host.Transition.On"}' https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
```

When using '-d' just remember that json requires quoting.

### 1.4 HTTP POST operations

POST operations are for calling methods, but also for creating new resources when the client doesn't know where to put it. OpenBMC does not support creating new resources via REST so any attempt to create a new resource will result in a HTTP 403 (Forbidden).

These also require a json formatted payload.

To delete logging entries:

```
$ curl -b cjar -k -H "X-Auth-Token: $token" -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/logging/action/DeleteAll
```

To invoke a method without parameters (Factory Reset of BMC and Host):

```
$ curl -b cjar -k -H "X-Auth-Token: $token" -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/software/action/Reset
```

### 1.5 HTTP DELETE operations

DELETE operations are for removing instances. Only D-Bus objects (instances) can be removed. If the underlying D-Bus object implements the xyz.openbmc_project.Object.Delete interface the REST server will call it. If xyz.openbmc_project.Object.Delete is not implemented, the REST server will return a HTTP 403 (Forbidden) error.

For example, to delete a event record :

Display logging entries:

```
$ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X GET https://${bmc}/xyz/openbmc_project/logging/entry/enumerate
{
  "data": {
​    "/xyz/openbmc_project/logging/entry/1": {
​      "AdditionalData": [
​        "_PID=185"
​      ],
​      "Id": 1,
​      "Message": "xyz.openbmc_project.Common.Error.InternalFailure",
​      "Purpose": "xyz.openbmc_project.Software.Version.VersionPurpose.BMC",
​      "Resolved": false,
​      "Severity": "xyz.openbmc_project.Logging.Entry.Level.Error",
​      "Timestamp": 1563191306359,
​      "Version": "2.8.0-dev-132-gd1c1b74-dirty",
​      "associations": []
​    }
  },
  "message": "200 OK",
  "status": "ok"
}
```

Then delete the event record with ID 1:

```
$ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X DELETE  https://${bmc}/xyz/openbmc_project/logging/entry/1
{
  "data": null,
  "message": "200 OK",
  "status": "ok"
}
```

### 1.6 Uploading images    (abnormal){No /upload object}

```
curl -k -X GET https://root:0penBmc@${bmc}/list
{
  "data": [
    "/",
    "/org",
    "/org/open_power",
    "/org/open_power/control",
    "/org/open_power/control/gard",
    "/org/open_power/control/host0",
    "/org/open_power/control/occ0",
    "/org/open_power/control/occ1",
    "/org/open_power/control/volatile",
    "/org/openbmc",
    "/org/openbmc/HostIpmi",
    "/org/openbmc/HostIpmi/1",
    "/org/openbmc/control",
    "/org/openbmc/control/power0",
    "/org/openbmc/mboxd",
    "/xyz",
    "/xyz/openbmc_project",
    "/xyz/openbmc_project/Chassis",
    "/xyz/openbmc_project/Chassis/Buttons",
    "/xyz/openbmc_project/Chassis/Buttons/Power0",
    "/xyz/openbmc_project/Chassis/Buttons/Reset0",
    "/xyz/openbmc_project/Hiomapd",
    "/xyz/openbmc_project/Ipmi",
    "/xyz/openbmc_project/certs",
    "/xyz/openbmc_project/certs/authority",
    "/xyz/openbmc_project/certs/authority/ldap",
    "/xyz/openbmc_project/certs/client",
    "/xyz/openbmc_project/certs/client/ldap",
    "/xyz/openbmc_project/certs/server",
    "/xyz/openbmc_project/certs/server/https",
    "/xyz/openbmc_project/console",
    "/xyz/openbmc_project/control",
    "/xyz/openbmc_project/control/host0",
    "/xyz/openbmc_project/control/host0/TPMEnable",
    "/xyz/openbmc_project/control/host0/auto_reboot",
    "/xyz/openbmc_project/control/host0/boot",
    "/xyz/openbmc_project/control/host0/boot/one_time",
    "/xyz/openbmc_project/control/host0/power_cap",
    "/xyz/openbmc_project/control/host0/power_restore_policy",
    "/xyz/openbmc_project/control/host0/restriction_mode",
    "/xyz/openbmc_project/control/host0/turbo_allowed",
    "/xyz/openbmc_project/control/minimum_ship_level_required",
    "/xyz/openbmc_project/control/power_supply_attributes",
    "/xyz/openbmc_project/control/power_supply_redundancy",
    "/xyz/openbmc_project/dump",
    "/xyz/openbmc_project/dump/internal",
    "/xyz/openbmc_project/dump/internal/manager",
    "/xyz/openbmc_project/events",
    "/xyz/openbmc_project/inventory",
    "/xyz/openbmc_project/inventory/system",
    "/xyz/openbmc_project/inventory/system/chassis",
    "/xyz/openbmc_project/inventory/system/chassis/activation",
    "/xyz/openbmc_project/ipmi",
    "/xyz/openbmc_project/ipmi/session",
    "/xyz/openbmc_project/ipmi/session/eth1",
    "/xyz/openbmc_project/ipmi/session/eth1/0",
    "/xyz/openbmc_project/led",
    "/xyz/openbmc_project/led/groups",
    "/xyz/openbmc_project/led/groups/bmc_booted",
    "/xyz/openbmc_project/led/groups/enclosure_fault",
    "/xyz/openbmc_project/led/groups/enclosure_identify",
    "/xyz/openbmc_project/led/groups/fan_fault",
    "/xyz/openbmc_project/led/groups/fan_identify",
    "/xyz/openbmc_project/led/groups/power_on",
    "/xyz/openbmc_project/led/physical",
    "/xyz/openbmc_project/led/physical/power",
    "/xyz/openbmc_project/logging",
    "/xyz/openbmc_project/logging/config",
    "/xyz/openbmc_project/logging/config/remote",
    "/xyz/openbmc_project/logging/internal",
    "/xyz/openbmc_project/logging/internal/manager",
    "/xyz/openbmc_project/logging/rest_api_logs",
    "/xyz/openbmc_project/network",
    "/xyz/openbmc_project/network/config",
    "/xyz/openbmc_project/network/config/dhcp",
    "/xyz/openbmc_project/network/eth0",
    "/xyz/openbmc_project/network/eth0/ipv4",
    "/xyz/openbmc_project/network/eth0/ipv4/bcb6b236",
    "/xyz/openbmc_project/network/eth0/ipv6",
    "/xyz/openbmc_project/network/eth0/ipv6/50fad4ec",
    "/xyz/openbmc_project/network/eth1",
    "/xyz/openbmc_project/network/eth1/ipv4",
    "/xyz/openbmc_project/network/eth1/ipv4/5405a73e",
    "/xyz/openbmc_project/network/eth1/ipv6",
    "/xyz/openbmc_project/network/eth1/ipv6/1de17e7e",
    "/xyz/openbmc_project/network/host0",
    "/xyz/openbmc_project/network/host0/intf",
    "/xyz/openbmc_project/network/host0/intf/addr",
    "/xyz/openbmc_project/network/sit0",
    "/xyz/openbmc_project/network/sit0/ipv6",
    "/xyz/openbmc_project/network/sit0/ipv6/68931764",
    "/xyz/openbmc_project/network/snmp",
    "/xyz/openbmc_project/network/snmp/manager",
    "/xyz/openbmc_project/object_mapper",
    "/xyz/openbmc_project/sensors",
    "/xyz/openbmc_project/sensors/fan_tach",
    "/xyz/openbmc_project/sensors/fan_tach/fan0_0",
    "/xyz/openbmc_project/sensors/fan_tach/fan0_1",
    "/xyz/openbmc_project/sensors/fan_tach/fan1_0",
    "/xyz/openbmc_project/sensors/fan_tach/fan1_1",
    "/xyz/openbmc_project/sensors/fan_tach/fan2_0",
    "/xyz/openbmc_project/sensors/fan_tach/fan2_1",
    "/xyz/openbmc_project/sensors/fan_tach/fan3_0",
    "/xyz/openbmc_project/sensors/fan_tach/fan3_1",
    "/xyz/openbmc_project/sensors/temperature",
    "/xyz/openbmc_project/sensors/temperature/bmc_zone",
    "/xyz/openbmc_project/sensors/temperature/ocp_zone",
    "/xyz/openbmc_project/sensors/temperature/outlet",
    "/xyz/openbmc_project/sensors/temperature/psu_inlet",
    "/xyz/openbmc_project/sensors/voltage",
    "/xyz/openbmc_project/sensors/voltage/P12V",
    "/xyz/openbmc_project/sensors/voltage/P3V3",
    "/xyz/openbmc_project/sensors/voltage/P5V",
    "/xyz/openbmc_project/sensors/voltage/PVCS_CPU0",
    "/xyz/openbmc_project/sensors/voltage/PVCS_CPU1",
    "/xyz/openbmc_project/sensors/voltage/PVDDQ_CPU0_CH01",
    "/xyz/openbmc_project/sensors/voltage/PVDDQ_CPU0_CH67",
    "/xyz/openbmc_project/sensors/voltage/PVDDQ_CPU1_CH01",
    "/xyz/openbmc_project/sensors/voltage/PVDDQ_CPU1_CH67",
    "/xyz/openbmc_project/sensors/voltage/PVDD_CPU0",
    "/xyz/openbmc_project/sensors/voltage/PVDD_CPU1",
    "/xyz/openbmc_project/sensors/voltage/PVDN_CPU0",
    "/xyz/openbmc_project/sensors/voltage/PVDN_CPU1",
    "/xyz/openbmc_project/sensors/voltage/PVIO_CPU0",
    "/xyz/openbmc_project/sensors/voltage/PVIO_CPU1",
    "/xyz/openbmc_project/sensors/voltage/p3v_bat",
    "/xyz/openbmc_project/software",
    "/xyz/openbmc_project/software/393ee7b0",
    "/xyz/openbmc_project/software/393ee7b0/software_version",
    "/xyz/openbmc_project/software/active",
    "/xyz/openbmc_project/software/apply_time",
    "/xyz/openbmc_project/software/fb41e6ba",
    "/xyz/openbmc_project/software/fb41e6ba/inventory",
    "/xyz/openbmc_project/software/fb41e6ba/software_version",
    "/xyz/openbmc_project/software/functional",
    "/xyz/openbmc_project/state",
    "/xyz/openbmc_project/state/bmc0",
    "/xyz/openbmc_project/state/chassis0",
    "/xyz/openbmc_project/state/host0",
    "/xyz/openbmc_project/time",
    "/xyz/openbmc_project/time/bmc",
    "/xyz/openbmc_project/time/host",
    "/xyz/openbmc_project/time/owner",
    "/xyz/openbmc_project/time/sync_method",
    "/xyz/openbmc_project/user",
    "/xyz/openbmc_project/user/ldap",
    "/xyz/openbmc_project/user/ldap/active_directory",
    "/xyz/openbmc_project/user/ldap/openldap",
    "/xyz/openbmc_project/user/root"
  ],
  "message": "200 OK",
  "status": "ok"

```





It is possible to upload software upgrade images (for example to upgrade the BMC or host software) via REST. The content-type should be set to "application/octet-stream".

For example, to upload an image:

```
$ curl -c cjar -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/octet-stream" -X POST -T <file_to_upload> https://${bmc}/upload/image
```

In above example, the filename on the BMC will be chosen by the REST server.

It is possible for the user to choose the uploaded file's remote name:

```
$ curl -c cjar -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/octet-stream" -X PUT -T foo https://${bmc}/upload/image/bar
```

In above example, the file "foo" will be saved with the name "bar" on the BMC.

The operation will either return the version id (hash) of the uploaded file on success:

{

"data": "ffdaab9b",

"message": "200 OK",

"status": "ok"

}

or an error message:

{

"data": {

"description": "Version already exists or failed to be extracted"

},

"message": "400 Bad Request",

"status": "error"

}

## 2  Host Management with OpenBMC

This document describes the host-management interfaces of the OpenBMC object structure, accessible over REST.

### 2.1 Inventory

The system inventory structure is under the /xyz/openbmc_project/inventory hierarchy.
In OpenBMC the inventory is represented as a path which is hierarchical to the physical system topology. Items in the inventory are referred to as inventory items and are not necessarily FRUs (field-replaceable units). If the system contains one chassis, a motherboard, and a CPU on the motherboard, then the path to that inventory item would be:

inventory/system/chassis0/motherboard0/cpu0

The properties associated with an inventory item are specific to that item. Some common properties are:

• Version: A code version associated with this item.

• Present: Indicates whether this item is present in the system (True/False).

• Functional: Indicates whether this item is functioning in the system (True/False).

The usual list and enumerate REST queries allow the system inventory structure to be accessed. For example, to enumerate all inventory items and their properties:

```
$ curl -b cjar -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/inventory/enumerate
```

To list the properties of one item:

```
$ curl -b cjar -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/inventory/system/chassis/motherboard
```

### 2.2 Sensors

The system sensor structure is under the /xyz/openbmc_project/sensors hierarchy.

This interface allows monitoring of system attributes like temperature or altitude, and are represented similar to the inventory, by object paths under the top-level sensors object name. The path categorizes the sensor and shows what the sensor represents, but does not necessarily represent the physical topology of the system.

For example, all temperature sensors are under sensors/temperature. CPU temperature sensors would be sensors/temperature/cpu

These are some common properties:

* Value: Current value of the sensor

* Unit: Unit of the value and “Critical” and “Warning” values

* Scale: The scale of the value and “Critical” and “Warning” values

* CriticalHigh & CriticalLow: Sensor device upper/lower critical threshold bound

* CriticalAlarmHigh & CriticalAlarmLow: True if the sensor has exceeded the critical threshold bound

* WarningHigh & WarningLow: Sensor device upper/lower warning threshold bound

* WarningAlarmHigh & WarningAlarmLow: True if the sensor has exceeded the warning threshold bound

A temperature sensor might look like:

```
$ curl -b cjar -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/sensors/temperature/ocp_zone
{
  "data": {
​    "CriticalAlarmHigh": false,
​    "CriticalAlarmLow": false,
​    "CriticalHigh": 65000,
​    "CriticalLow": 0,
​    "Functional": true,
​    "MaxValue": 0,
​    "MinValue": 0,
​    "Scale": -3,
​    "Unit": "xyz.openbmc_project.Sensor.Value.Unit.DegreesC",
​    "Value": 34625,
​    "WarningAlarmHigh": false,
​    "WarningAlarmLow": false,
​    "WarningHigh": 63000,
​    "WarningLow": 0
  },
  "message": "200 OK",
  "status": "ok"
}
```

Note the value of this sensor is 34.625℃ (34625 * 10^-3).

Unlike IPMI, there are no “functional” sensors in OpenBMC; functional states are represented in the inventory.
To enumerate all sensors in the system:

```
$ curl -b cjar -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/sensors/enumerate
```

List properties of one inventory item:

```
$ curl -b cjar -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/sensors/temperature/outlet
```

### 2.3 Event Logs

The event log structure is under the /xyz/openbmc_project/logging/entry hierarchy. Each event is a separate object under this structure, referenced by number.

BMC and host firmware on POWER-based servers can report event logs to the BMC. Typically, these event logs are reported in cases where host firmware cannot start the OS, or cannot reliably log to the OS.

The properties associated with an event log are as follows:

* Message: The type of event log (e.g. “xyz.openbmc_project.Inventory.Error.NotPresent”).

* Resolved : Indicates whether the event has been resolved.

* Severity: The level of problem (“Info”, “Error”, etc.).

* Timestamp: The date of the event log in epoch time.

* Associations: A URI to the failing inventory part.

To list all reported event logs:

```
$ curl -b cjar -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/logging/entry
{
"data": [
"/xyz/openbmc_project/logging/entry/1",
"/xyz/openbmc_project/logging/entry/2",
],
"message": "200 OK",
"status": "ok"
}
```

To read a specific event log:

```
$ curl -b cjar -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/logging/entry/1
{
  "data": {
​    "AdditionalData": [
​      "_PID=183"
​    ],
​    "Id": 1,
​    "Message": "xyz.openbmc_project.Common.Error.InternalFailure",
​    "Purpose": "xyz.openbmc_project.Software.Version.VersionPurpose.BMC",
​    "Resolved": false,
​    "Severity": "xyz.openbmc_project.Logging.Entry.Level.Error",
​    "Timestamp": 1563191362822,
​    "Version": "2.8.0-dev-132-gd1c1b74-dirty",
​    "associations": []
  },
  "message": "200 OK",
  "status": "ok"
}
```

To delete an event log (log 1 in this example), call the delete method on the event:

```
$ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST -d '{"data" : []}' https://${bmc}/xyz/openbmc_project/logging/entry/1/action/Delete
```

To clear all event logs, call the top-level deleteAll method:

```
$ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST -d '{"data" : []}' https://${bmc}/xyz/openbmc_project/logging/action/deleteAll
```

### 2.4 Host Boot Options

With OpenBMC, the Host boot options are stored as D-Bus properties under the control/host0/boot path. Properties include

https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Control/Boot/Mode.interface.yaml

and

 https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Control/Boot/Source.interface.yaml.

### 2.5 Host State Control

The host can be controlled through the host object. The object implements a number of actions including power on and power off. These correspond to the IPMI power on and power off commands.

Assuming you have logged in, the following will power on the host:

```
$ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -d '{"data": "xyz.openbmc_project.State.Host.Transition.On"}' -X PUT https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
```

To power off the host:

```
$ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -d '{"data": "xyz.openbmc_project.State.Host.Transition.Off"}' -X PUT https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
```

To issue a hard power off (accomplished by powering off the chassis):

```
$ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d '{"data":"xyz.openbmc_project.State.Chassis.Transition.Off"}' https://${bmc}//xyz/openbmc_project/state/chassis0/attr/RequestedPowerTransition
```

To reboot the host:

```
$ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d '{"data":"xyz.openbmc_project.State.Host.Transition.Reboot"}' https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
```

More information about Host State Management can be found here: https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/xyz/openbmc_project/State

## 3  Host Clear GARD

On OpenPOWER systems, the host maintains a record of bad or non-working components on the GARD partition. This record is referenced by the host on subsequent boots to determine which parts should be ignored.

```
$ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST -d '{"data":[]}' https://${bmc}/org/open_power/control/gard/action/Reset
```

Implementation: https://github.com/openbmc/openpower-pnor-code-mgmt

## 4  OpenBMC host console support

This document describes how to connect to the host UART console from an OpenBMC management system.
The console infrastructure allows multiple shared connections to a single host UART. UART data from the host is output to all connections, and input from any connection is sent to the host.

### 4.1 Remote console connections

To connect to an OpenBMC console session remotely, just ssh to your BMC on port 2200. Use the same login credentials you would for a normal ssh session:

```
$ ssh root@xx.xx.xx.xx -p 2200
```

### 4.2 Local console connections (abnormal)(Could not return)

If you're already logged into an OpenBMC machine, you can start a console session directly, using:

```
$ obmc-console-client
```

To exit from a console, type:

return ~

Note that if you're on an ssh connection, you'll need to 'escape' the ~ character, by entering it twice.

### 4.3 Logging

Console logs are kept in:

/var/log/obmc-console.log

This log is limited in size, and will wrap after hitting that limit (currently set at 16kB).

## 5  OpenBMC non-UBI Code Update

Two BMC Code Updates layouts are available:

• Static, non-UBI layout - The default code update

• UBI layout - enabled via obmc-ubi-fs machine feature

The host code update can be found here: https://github.com/openbmc/docs/blob/master/code-update/host-code-update.md

After building OpenBMC, you will end up with a set of image files in tmp/deploy/images/<platform>/. The
image-* symlinks correspond to components that can be updated on the BMC:

• image-bmc → obmc-phosphor-image-<platform>-<timestamp>.static.mtd

The complete flash image for the BMC

• image-kernel → fitImage-obmc-phosphor-initramfs-<platform>.bin

The OpenBMC kernel FIT image (including the kernel, device tree, and initramfs)

• image-rofs → obmc-phosphor-image-<platform>.squashfs-xz

The read-only OpenBMC filesystem

• image-rwfs → rwfs.jffs2

The read-write filesystem for persistent changes to the OpenBMC filesystem

• image-u-boot → u-boot.bin

The OpenBMC bootloader

Additionally, there are two tarballs created that can be deployed and unpacked by REST:

```
• <platform>-<timestamp>.all.tar
```

The complete BMC flash content: A single file (image-bmc) wrapped in a tar archive.

```
• <platform>-<timestamp>.tar
```

Partitioned BMC flash content: Multiple files wrapped in a tar archive, one for each of the u-boot, kernel, ro
and rw partitions.

### 5.1 Preparing for BMC code Update

The BMC normally runs with the read-write and read-only file systems mounted, which means these images may be read (and written, for the read-write filesystem) at any time. Because the updates are distributed as complete file system images, these filesystems have to be unmounted to replace them with new images. To unmount these file systems all applications must be stopped.
By default, an orderly reboot will stop all applications and unmount the root filesystem, and the images copied into the /run/initramfs directory will be applied at that point before restarting. This also applied to the shutdown and halt commands – they will write the flash before stopping.
As an alternative, an option can be parsed by the init script in the initramfs to copy the required contents of these filesystems into RAM so the images can be applied while the rest of the application stack is running and progress can be monitored over the network. The update script can then be called to write the images while the system is operational and its progress output monitored.

### 5.2 Update from the OpenBMC shell

To update from the OpenBMC shell, follow the steps in this section.

Firstly, use scp command copy the image-bmc file to directory  /run/initramfs  : 

(The "image-bmc" is soft-link to the image "obmc-phosphor-image-xxxxxx-xxxxxxxx.static.mtd")

```
$ sudo scp -r image-bmc xx.xx.xx.xx:/run/initramfs
```

Then reboot to finish applying:

```
# reboot
```

### 5.3 Update via REST (abnormal)

 (No  /org/openbmc/control/flash)

An OpenBMC system can download an update image from a TFTP server, and apply updates, controlled via REST.
The general procedure is:

1. Prepare system for update

2. Configure update settings

3. Initiate update

4. Check flash status

5. Apply update

6. Reboot the BMC

#### 5.3.1 Prepare system for update

Perform a POST to invoke the PrepareForUpdate method of the /flash/bmc object:

```
$ curl -b cjar -k -H "Content-Type: application/json" -X POST  -d '{"data": []}' https://${bmc}/org/openbmc/control/flash/bmc/action/prepareForUpdate
```

This will setup the u-boot environment and reboot the BMC. If no other images were pending the BMC should return in about 2 minutes.

#### 5.3.2 Configure update settings

There are a few settings available to control the update process:
• preserve_network_settings: Preserve network settings, only needed if updating the whole flash
• restore_application_defaults: update (clear) the read-write file system
• update_kernel_and_apps: update kernel and initramfs. If the partitioned tarball will be used for update then
this option must be set. Otherwise, if the complete tarball will be used then this option must not be set.
• clear_persistent_files: ignore the persistent file list when resetting applications defaults
• auto_apply: Attempt to write the images by invoking the Apply method after the images are unpacked.
To configure the update settings, perform a REST PUT to /control/flash/bmc/attr/<setting>. For example:

```
curl -b cjar -k -H "Content-Type: application/json" -X PUT \
-d '{"data": 1}' \
https://${bmc}/org/openbmc/control/flash/bmc/attr/preserve_network_settings
```



#### 5.3.3 Initiate update

Perform a POST to invoke the updateViaTftp method of the /flash/bmc object:

```
curl -b cjar -k -H "Content-Type: application/json" -X POST \
-d '{"data": ["<TFTP server IP address>", "<filename>"]}' \
https://${bmc}/org/openbmc/control/flash/bmc/action/updateViaTftp
```

Note the <filename> shall be a tarball.



#### 5.3.4 Check flash status

You can query the progress of the download and image verification with a simple GET request:
curl -b cjar -k https://${bmc}/org/openbmc/control/flash/bmc
Or perform a POST to invoke the GetUpdateProgress method of the /flash/bmc object:

curl -b cjar -k -H "Content-Type: application/json" -X POST \
-d '{"data": []}' \
https://${bmc}/org/openbmc/control/flash/bmc/action/GetUpdateProgress
Note:
• During downloading the tarball, the progress status is Downloading
• After the tarball is downloaded and verified, the progress status becomes Image ready to apply.



#### 5.3.5 Apply update

If the status is Image ready to apply. then you can either initiate a reboot or call the Apply method to start the process of writing the flash:
curl -b cjar -k -H "Content-Type: application/json" -X POST \
-d '{"data": []}' \
https://${bmc}/org/openbmc/control/flash/bmc/action/Apply
Now the image is being flashed, you can check the progress with above step s command as well.
• During flashing the image, the status becomes Writing images to flash
• After it’s flashed and verified, the status becomes Apply Complete. Reboot to take effect.



#### 5.3.6 Reboot the BMC

To start using the new images, reboot the BMC using the warmReset method of the BMC control object:

```
$ curl -b cjar -k -H "Content-Type: application/json" -X POST -d '{"data": []}' https://${bmc}/org/openbmc/control/bmc0/action/warmReset
```

## 6  The ObjectMapper

The xyz.openbmc_project.ObjectMapper service, commonly referred to as just the mapper, is an OpenBMC application that attempts to ease the pain of using D-Bus by providing APIs that help in discovering and associating other D-Bus objects.

The mapper has two major pieces of functionality:

• Methods - Provides D-Bus discovery related functionality.
• Associations - Associates two different objects with each other.

### 6.1 Methods

The official YAML interface definition can be found here : https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/ObjectMapper.interface.yaml 

#### 6.1.1 GetObject

Use this method to find the services, with their interfaces, that implement a certain object path. The output is a map of service names to their implemented interfaces. An optional list of interfaces may also be passed in to constrain the output to services that implement those specific interfaces.

Inputs: - path: object path - param: interfaces - an optional list of interfaces to constrain the search to
Output: - Map of service names to their interfaces

```
# dbus-send --system --print-reply --dest=xyz.openbmc_project.ObjectMapper /xyz/openbmc_project/object_mapper xyz.openbmc_project.ObjectMapper.GetObject string:"/xyz/openbmc_project/sensors/voltage/psu0_vin" array:string:

method return time=1563250049.514487 sender=:1.21 -> destination=:1.99 serial=811 reply_serial=2
   array [
      dict entry(
         string "xyz.openbmc_project.Hwmon-1772350680.Hwmon1"
         array [
            string "org.freedesktop.DBus.Introspectable"
            string "org.freedesktop.DBus.Peer"
            string "org.freedesktop.DBus.Properties"
            string "xyz.openbmc_project.Sensor.Value"
            string "xyz.openbmc_project.State.Decorator.OperationalStatus"
         ]
      )
   ]
```

**Example Use Case**  Find the service name that has the desired object path so it can be passed into a get property call.



#### 6.1.2 GetSubTree

Use this method to find the objects, services, and interfaces in the specified subtree that implement a certain interface.

If no interfaces are passed in, then all objects/services/interfaces in the subtree are returned. If interfaces are passed in, then only those interfaces are returned in the output.

Inputs: - param: subtree - the root of the tree. Using “/” will search the whole tree - param: depth - the maximum depth of the tree past the root to search. Use 0 to search all - param: interfaces - an optional list of interfaces to constrain the search to

Output: - Map of object paths to a map of service names to their interfaces

```
# dbus-send --system --print-reply --dest=xyz.openbmc_project.ObjectMapper /xyz/openbmc_project/object_mapper xyz.openbmc_project.ObjectMapper.GetSubTree
string:"/" int32:0 array:string:"xyz.openbmc_project.Sensor.Threshold.Warning"

method return time=1563250227.478618 sender=:1.21 -> destination=:1.100 serial=813 reply_serial=2
   array [
      dict entry(
         string "/xyz/openbmc_project/sensors/temperature/bmc_zone"
         array [
            dict entry(
               string "xyz.openbmc_project.Hwmon-2819114370.Hwmon1"
               array [
                  string "org.freedesktop.DBus.Introspectable"
                  string "org.freedesktop.DBus.Peer"
                  string "org.freedesktop.DBus.Properties"
                  string "xyz.openbmc_project.Sensor.Threshold.Critical"
                  string "xyz.openbmc_project.Sensor.Threshold.Warning"
                  string "xyz.openbmc_project.Sensor.Value"
                  string "xyz.openbmc_project.State.Decorator.OperationalStatus"
               ]
            )
         ]
      )
      dict entry(
         string "/xyz/openbmc_project/sensors/temperature/ocp_zone"
         array [
            dict entry(
               string "xyz.openbmc_project.Hwmon-4226741464.Hwmon1"
               array [
                  string "org.freedesktop.DBus.Introspectable"
                  string "org.freedesktop.DBus.Peer"
                  string "org.freedesktop.DBus.Properties"
                  string "xyz.openbmc_project.Sensor.Threshold.Critical"
                  string "xyz.openbmc_project.Sensor.Threshold.Warning"
                  string "xyz.openbmc_project.Sensor.Value"
                  string "xyz.openbmc_project.State.Decorator.OperationalStatus"
               ]
            )
         ]
      )
......
```

**Example Use Case**   Find all object paths and services that implement a specific interface.



#### 6.1.3 GetSubTreePath

This is the same as GetSubTree, but only returns object paths

Inputs: - param: subtree - the root of the tree. Using “/” will search the whole tree - param: depth - the maximum depth of the tree past the root to search. Use 0 to search all - param: interfaces - an optional list of interfaces to constrain the search to

Output: - array of object paths in that subtree

```
# dbus-send --system --print-reply --dest=xyz.openbmc_project.ObjectMapper /xyz/openbmc_project/object_mapper xyz.openbmc_project.ObjectMapper.GetSubTreePaths string:"/" int32:0 array:string:"xyz.openbmc_project.Sensor.Threshold.Warning"

method return time=1563250589.501451 sender=:1.21 -> destination=:1.101 serial=815 reply_serial=2
   array [
      string "/xyz/openbmc_project/sensors/temperature/bmc_zone"
      string "/xyz/openbmc_project/sensors/temperature/ocp_zone"
      string "/xyz/openbmc_project/sensors/temperature/outlet"
      string "/xyz/openbmc_project/sensors/temperature/psu_inlet"
      string "/xyz/openbmc_project/sensors/voltage/P12V"
      string "/xyz/openbmc_project/sensors/voltage/P3V3"
      string "/xyz/openbmc_project/sensors/voltage/P5V"
      string "/xyz/openbmc_project/sensors/voltage/PVCS_CPU0"
      string "/xyz/openbmc_project/sensors/voltage/PVCS_CPU1"
      string "/xyz/openbmc_project/sensors/voltage/PVDDQ_CPU0_CH01"
      string "/xyz/openbmc_project/sensors/voltage/PVDDQ_CPU0_CH67"
      string "/xyz/openbmc_project/sensors/voltage/PVDDQ_CPU1_CH01"
      string "/xyz/openbmc_project/sensors/voltage/PVDDQ_CPU1_CH67"
      string "/xyz/openbmc_project/sensors/voltage/PVDD_CPU0"
      string "/xyz/openbmc_project/sensors/voltage/PVDD_CPU1"
      string "/xyz/openbmc_project/sensors/voltage/PVDN_CPU0"
      string "/xyz/openbmc_project/sensors/voltage/PVDN_CPU1"
      string "/xyz/openbmc_project/sensors/voltage/PVIO_CPU0"
      string "/xyz/openbmc_project/sensors/voltage/PVIO_CPU1"
   ]
```

**Example Use Case**  Find all object paths that implement a specific interface.

#### 6.1.4  GetAncestors

Use this method to find all ancestors of an object that implement a specific interface. If no interfaces are passed in, then all ancestor paths/services/interfaces are returned.

Inputs: - param: path - the object path to find the ancestors of - param: interfaces - an optional list of interfaces to constrain the search to
Output: - A map of object paths to a map of services names to their interfaces

```
# dbus-send --system --print-reply  --dest=xyz.openbmc_project.ObjectMapper /xyz/openbmc_project/object_mapper xyz.openbmc_project.ObjectMapper.GetAncestors string:"/xyz/openbmc_project/inventory/system" array:string:

method return time=1563250791.307637 sender=:1.21 -> destination=:1.102 serial=817 reply_serial=2
   array [
      dict entry(
         string ""
         array [
            dict entry(
               string "xyz.openbmc_project.Dump.Manager"
               array [
                  string "org.freedesktop.DBus.Introspectable"
                  string "org.freedesktop.DBus.Peer"
                  string "org.freedesktop.DBus.Properties"
               ]
            )
            dict entry(
               string "xyz.openbmc_project.Ipmi.Host"
               array [
                  string "org.freedesktop.DBus.Introspectable"
                  string "org.freedesktop.DBus.Peer"
                  string "org.freedesktop.DBus.Properties"
               ]
            )
         ]
      )
......


```

**Example Use Case**  Find a parent object that implements a specific interface.



### 6.2 Associations

Associations are special D-Bus objects created by the mapper to associate two objects with each other. For this to occur, some application must implement the org.openbmc.Associations interface, and then when an association is desired, the associations property on that interface needs to be written.

This associations property is an array of tuples of the form:

[forward, reverse, object path]

• forward: this is the name of the forward association object

• reverse: this is the name of the reverse association object

• object path: this is the other object to associate with

When an object with, for example, an object path of pathA uses the following values:

["foo", "bar", "pathB"]

The mapper will create 2 new objects:

1. pathA/foo
2. pathB/bar

On each of these objects, the interface xyz.openbmc_project.Association will be implemented, which has a single endpoints property. This property is an array that holds the object paths to the other end of the association.
So, pathA/foo->endpoints will contain pathB, and pathB/bar->endpoints will contain pathA.
If another object, say pathC, also has an association to pathB, then a second entry, pathC, will be added into pathB's endpoints property.
These new objects will match the lifetime of the associated objects. For example, if pathA is deleted, then pathA/foo will also be deleted, and pathA will be removed from the endpoints property of pathB/bar. If that was the last entry in that property, then pathB/bar will also be deleted.

**Example Use Case**  Associate an error log with the inventory item that caused it.

```
# Error log
"/xyz/openbmc_project/logging/entry/3": {
...
"associations": [
[
"callout",
"fault",
"/xyz/openbmc_project/inventory/system/chassis/motherboard/powersupply0"
]
]
}
# Newly created forward association object
"/xyz/openbmc_project/logging/entry/3/callout": {
"endpoints": [
"/xyz/openbmc_project/inventory/system/chassis/motherboard/powersupply0"
]
}
# Newly created reverse association object
"/xyz/openbmc_project/inventory/system/chassis/motherboard/powersupply0/fault": {
"endpoints": [
"/xyz/openbmc_project/logging/entry/3"
]
}
```



## 7 Cheatsheet

Note :The login method is :

```
$ export bmc=xx.xx.xx.xx
$ export token=`curl -k -H "Content-Type: application/json" -X POST https://${bmc}/login -d '{"username" :  "root", "password" :  "0penBmc"}' | grep token | awk '{print $2;}' | tr -d '"'`
$ curl -k -H "X-Auth-Token: $token" https://$bmc/xyz/openbmc_project/...
```

### 7.1 List and enumerate

```
$ curl -b cjar -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/list
$ curl -b cjar -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/enumerate
```

### 7.2 List sub-objects

```
$ curl -b cjar -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/
$ curl -b cjar -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/state/
```

### 7.3 Host power

- Host soft power off:

  ```
  $ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -d '{"data": "xyz.openbmc_project.State.Host.Transition.Off"}' -X PUT https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
  ```

- Host hard power off:

  ```
  $ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d '{"data":"xyz.openbmc_project.State.Chassis.Transition.Off"}' https://${bmc}//xyz/openbmc_project/state/chassis0/attr/RequestedPowerTransition
  ```

- Host power on:

  ```
  $ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -d '{"data": "xyz.openbmc_project.State.Host.Transition.On"}' -X PUT https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
  ```

- Reboot Host:

  ```
  $ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d '{"data":"xyz.openbmc_project.State.Host.Transition.Reboot"}' https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
  ```

### 7.4 Reboot BMC

```
$ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d '{"data":"xyz.openbmc_project.State.BMC.Transition.Reboot"}' https://${bmc}//xyz/openbmc_project/state/bmc0/attr/RequestedBMCTransition
```

### 7.5 Logging entries

- Display logging entries:

  ```
  $ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X GET https://${bmc}/xyz/openbmc_project/logging/entry/enumerate
  ```

- Delete logging entries:

  ```
  $ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X DELETE https://${bmc}/xyz/openbmc_project/logging/entry/<entry_id>
  $ curl -b cjar -k -H "X-Auth-Token: $token" -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/logging/action/DeleteAll
  ```

### 7.6 Delete dump entries

```
$ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X DELETE https://${bmc}/xyz/openbmc_project/dump/entry/<entry_id>
$ curl -b cjar -k -H "X-Auth-Token: $token" -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/dump/action/DeleteAll
```

### 7.7 Delete images from system(abnormal)

- Delete images from system:

  - Delete image:

  ```
  $ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST -d '{"data": []}' https://${bmc}/xyz/openbmc_project/software/<image id>/action/Delete
  ```

  - Delete all non-running images:

    (Failed to delete all not active images. Images' information also remains on the Web .

    The commands and feedback are as follows)

  ```
$ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST -d '{"data": []}' https://${bmc}/xyz/openbmc_project/software/action/DeleteAll
  ```
  
  ```
$ curl -k -X GET https://root:0penBmc@${bmc}/xyz/openbmc_project/software/list
  {
  "data": [
    "/xyz/openbmc_project/software/6a70899d",                                ##pnor non-running image 1
    "/xyz/openbmc_project/software/6a70899d/inventory",
    "/xyz/openbmc_project/software/6a70899d/software_version",
    "/xyz/openbmc_project/software/active",
    "/xyz/openbmc_project/software/apply_time",
    "/xyz/openbmc_project/software/e2120de7",                                  ##bmc non-running image 1
    "/xyz/openbmc_project/software/e2120de7/inventory",
    "/xyz/openbmc_project/software/eb566212",                                  ##bmc non-running image 2
    "/xyz/openbmc_project/software/eb566212/inventory",
    "/xyz/openbmc_project/software/fb41e6ba",
    "/xyz/openbmc_project/software/fb41e6ba/inventory",
    "/xyz/openbmc_project/software/fb41e6ba/software_version",
    "/xyz/openbmc_project/software/functional"
  ],
  "message": "200 OK",
  "status": "ok"
  }
  $ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST -d '{"data": []}' https://${bmc}/xyz/openbmc_project/software/action/DeleteAll
  {
    "data": null,
    "message": "200 OK",
    "status": "ok"
  }
  $ curl -k -X GET https://root:0penBmc@${bmc}/xyz/openbmc_project/software/list
  {
    "data": [
      "/xyz/openbmc_project/software/6a70899d",
      "/xyz/openbmc_project/software/6a70899d/inventory",
      "/xyz/openbmc_project/software/6a70899d/software_version",
      "/xyz/openbmc_project/software/active",
      "/xyz/openbmc_project/software/apply_time",
      "/xyz/openbmc_project/software/e2120de7",
      "/xyz/openbmc_project/software/eb566212",
      "/xyz/openbmc_project/software/fb41e6ba",
      "/xyz/openbmc_project/software/fb41e6ba/inventory",
      "/xyz/openbmc_project/software/fb41e6ba/software_version",
      "/xyz/openbmc_project/software/functional"
    ],
    "message": "200 OK",
    "status": "ok"
  }
  ```

###  7.8 Boot option(abnormal)

(Property changes were successful, but it was not known how to validate the phenomenon.)

- Set boot mode:

  ```
  $ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT https://${bmc}/xyz/openbmc_project/control/host0/boot/one_time/attr/BootMode -d '{"data": "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular"}'
  ```
- Set boot source:

  ```
  $ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT https://${bmc}/xyz/openbmc_project/control/host0/boot/one_time/attr/BootSource -d '{"data": "xyz.openbmc_project.Control.Boot.Source.Sources.Default"}
  ```
### 7.9 Set NTP and Nameserver

Examples using public server.

* NTP server:

  ```
  $ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d '{"data": ["pool.ntp.org"] }' https://${bmc}/xyz/openbmc_project/network/eth0/attr/NTPServers
  ```
* Name server:

  ```
  $ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d '{"data": ["time.google.com"] }' https://${bmc}/xyz/openbmc_project/network/eth0/attr/Nameservers
  ```
### 7.10 Configure time ownership and time sync method

* Read:

  ```
    $ curl -b cjar -k -X -H "X-Auth-Token: $token" GET https://${bmc}/xyz/openbmc_project/time/owner/attr/TimeOwner
    $ curl -b cjar -k -X -H "X-Auth-Token: $token" GET https://${bmc}/xyz/openbmc_project/time/sync_method/attr/TimeSyncMethod
  ```
* Write:

  ```
  $ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Synchronization.Method.NTP" }' https://${bmc}/xyz/openbmc_project/time/sync_method/attr/TimeSyncMethod
  $ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Synchronization.Method.Manual" }' https://${bmc}/xyz/openbmc_project/time/sync_method/attr/TimeSyncMethod

  $ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Owner.Owners.BMC" }' https://${bmc}/xyz/openbmc_project/time/owner/attr/TimeOwner
  $ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Owner.Owners.Host” }' https://${bmc}/xyz/openbmc_project/time/owner/attr/TimeOwner
  $ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Owner.Owners.Split" }' https://${bmc}/xyz/openbmc_project/time/owner/attr/TimeOwner
  $ curl -b cjar -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Owner.Owners.Both” }' https://${bmc}/xyz/openbmc_project/time/owner/attr/TimeOwner
  ```
### 7.11 Update "root" password(abnormal)

(The command failed, and the feedback is as follows.)

Change password from "OpenBmc" to "abc123":

  ```
$ curl -c cjar -b cjar -k -H "Content-Type: application/json" -X POST https://${bmc}/login -d "{\"data\": [ \"root\", \"0penBmc\" ] }"
$ curl -b cjar -k -H "Content-Type: application/json" -d "{\"data\": [\"abc123\"] }" -X POST  https://${bmc}/xyz/openbmc_project/user/root/action/SetPassword

{
  "data": {
    "description": "The specified method cannot be found"
  },
  "message": "404 Not Found",
  "status": "error"
}
  ```
### 7.12 Factory Reset

- Factory reset host and BMC software:

```
$ curl -b cjar -k -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/software/action/Reset
```
- Factory reset network setting:

```
$ curl -b cjar -k -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/network/action/Reset
```
- Enable field mode:

```
$ curl -b cjar -k -H 'Content-Type: application/json' -X PUT -d '{"data":1}' https://${bmc}/xyz/openbmc_project/software/attr/FieldModeEnabled
```
and then reboot BMC.

## 8 Redfish cheat sheet

This chapter is intended to provide a set of [Redfish][1] client commands for OpenBMC usage.

(Using CURL commands)

### 8.1 Query Root

```
$ export bmc=xx.xx.xx.xx
$ curl -b cjar -k https://${bmc}/redfish/v1
```

### 8.2 Establish Redfish connection session

Note : The method was Not used in this document.

```
$ export bmc=xx.xx.xx.xx
$ curl --insecure -X POST -D headers.txt https://${bmc}/redfish/v1/SessionService/Sessions -d '{"UserName":"root", "Password":"0penBmc"}'
```

 A file, headers.txt, will be created. Find the "X-Auth-Token"
 in that file. Save it away in an env variable like so:

```
 export bmc_token=<token>
```

### 8.3 View Redfish Objects

```
$ curl -k -H "X-Auth-Token: $token" -X GET https://${bmc}/redfish/v1/Chassis
$ curl -k -H "X-Auth-Token: $token" -X GET https://${bmc}/redfish/v1/Managers
$ curl -k -H "X-Auth-Token: $token" -X GET https://${bmc}/redfish/v1/Systems
```

### 8.4 Host power

- Host soft power off:

  ```
  $ curl -k -H "X-Auth-Token: $token" -X POST https://${bmc}/redfish/v1/Systems/system/Actions/ComputerSystem.Reset -d '{"ResetType": "GracefulShutdown"}'
  ```

- Host hard power off:

  ```
  $ curl -k -H "X-Auth-Token: $token" -X POST https://${bmc}/redfish/v1/Systems/system/Actions/ComputerSystem.Reset -d '{"ResetType": "ForceOff"}'
  ```

- Host power on:

  ```
  $ curl -k -H "X-Auth-Token: $token" -X POST https://${bmc}/redfish/v1/Systems/system/Actions/ComputerSystem.Reset -d '{"ResetType": "On"}'
  ```

- Reboot Host:

  ```
  $ curl -k -H "X-Auth-Token: $token" -X POST https://${bmc}/redfish/v1/Systems/system/Actions/ComputerSystem.Reset -d '{"ResetType": "GracefulRestart"}'
  ```

### 8.5 Log entry

- Display logging entries:

  ```
  $ curl -k -H "X-Auth-Token: $token" -X GET https://${bmc}/redfish/v1/Systems/system/LogServices/EventLog/Entries
  {
    "@odata.context": "/redfish/v1/$metadata#LogEntryCollection.LogEntryCollection",
    "@odata.id": "/redfish/v1/Systems/system/LogServices/EventLog/Entries",
    "@odata.type": "#LogEntryCollection.LogEntryCollection",
    "Description": "Collection of System Event Log Entries",
    "Members": [
      {
        "@odata.context": "/redfish/v1/$metadata#LogEntry.LogEntry",
        "@odata.id": "/redfish/v1/Systems/system/LogServices/EventLog/Entries/1",
        "@odata.type": "#LogEntry.v1_4_0.LogEntry",
        "Created": "2019-07-15T11:48:51+00:00",
        "EntryType": "Event",
        "Id": "1",
        "Message": "xyz.openbmc_project.Common.Error.InternalFailure",
        "Name": "System Event Log Entry",
        "Severity": "Critical"
      }
    ],
    "Members@odata.count": 1,
    "Name": "System Event Log Entries"
  }
  ```
  
- Delete logging entries:

  ```
  $ curl -k -H "X-Auth-Token: $token" -X POST https://${bmc}/redfish/v1/Systems/system/LogServices/EventLog/Actions/LogService.Reset
  ```

### 8.6 Firmware ApplyTime:(abnormal)

(Failed)

```
$ curl -k -H "X-Auth-Token: $token" -X PATCH -d '{ "ApplyTime":"Immediate"}' https://${bmc}/redfish/v1/UpdateService

Method Not Allowed
```

```
$ curl -k -H "X-Auth-Token: $token" -X PATCH -d '{ "ApplyTime":"OnReset"}' https://${bmc}/redfish/v1/UpdateService

Method Not Allowed
```

### 8.7 Firmware update

- Firmware update:

  Note the <image file path> shall be a tarball.

  ```
  $ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/octet-stream" -X POST -T <image file path> https://${bmc}/redfish/v1/UpdateService
  {
    "@Message.ExtendedInfo": [
      {
        "@odata.type": "/redfish/v1/$metadata#Message.v1_0_0.Message",
        "Message": "Successfully Completed Request",
        "MessageArgs": [],
        "MessageId": "Base.1.4.0.Success",
        "Resolution": "None",
        "Severity": "OK"
      }
    ]
  }
  ```
  
- TFTP Firmware update using TransferProtocol:(abnormal,no service)

  ```
  curl -k -H "X-Auth-Token: $token" -X POST https://${bmc}/redfish/v1/UpdateService/Actions/UpdateService.SimpleUpdate -d '{"TransferProtocol":"TFTP","ImageURI":"<image file path>"}'
  Not Found
  ```
  
- TFTP Firmware update with protocol in ImageURI:(abnormal,no service)

  ```
  curl -k -H "X-Auth-Token: $token" -X POST https://${bmc}/redfish/v1/UpdateService/Actions/UpdateService.SimpleUpdate -d '{"ImageURI":"tftp://<image file path>"}'
  Not Found
  ```

