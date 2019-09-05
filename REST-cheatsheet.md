# OpenBMC REST-cheatsheet
##  1  OpenBMC REST API
The primary management interface for OpenBMC is REST. This document provides some basic structure and usage examples for the REST interface. The schema for the rest interface is directly defined by the OpenBMC D-Bus structure. Therefore, the objects, attributes and methods closely map to those in the D-Bus schema. For a quick explanation of HTTP verbs and how they relate to a RESTful API, see http://www.restapitutorial.com/lessons/httpmethods.html.
### Notes on authentication:
The original REST server, from the phosphor-rest-server repository, uses authentication handled by the curl cookie jar files. The bmcweb REST server can use the same cookie jar files for read-only REST methods like GET, but requires either an authentication token or the username and password passed in as part of the URL for non-read-only methods.
The phosphor-rest server will no longer be the default REST server after the 2.6 OpenBMC release.
### 1.1 Logging in
* Using the cookie jar files for the bmcweb:
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
  Note: This method can also be used to the Redfish sessions.
```
$ export bmc=xx.xx.xx.xx
$ export token=`curl -k -H "Content-Type: application/json" -X POST https://${bmc}/login -d '{"username" :  "root", "password" :  "0penBmc"}' | grep token | awk '{print $2;}' | tr -d '"'`
$ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/...
```
The third method is recommended.
### 1.2 HTTP GET operations & URL structure
There are a few conventions on the URL structure of the OpenBMC rest interface. They are:
1. To query the attributes of an object, perform a GET request on the object name, with no trailing slash. For example:
```
$ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/inventory/system
{
"data": {
    "AssetTag": "",
    "BuildDate": "",
    "Cached": true,
    "FieldReplaceable": true,
    "Manufacturer": "",
    "Model": "",
    "PartNumber": "",
    "Present": true,
    "PrettyName": "",
    "SerialNumber": ""
  },
  "message": "200 OK",
  "status": "ok"
}
```
2. To query a single attribute, use the `attr/<name>` path. Using the `system` object from above, we can query just the `Name` value:
```
$ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/inventory/system/attr/Cached
{
  "data": true,
  "message": "200 OK",
  "status": "ok"
}
```
3. When a path has a trailing-slash, the response will list the sub objects of the URL. For example, using the same object path as above, but adding a slash:
```
$ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/
{
  "data": [
    "/xyz/openbmc_project/Chassis",
    "/xyz/openbmc_project/Hiomapd",
    "/xyz/openbmc_project/Ipmi",
    "/xyz/openbmc_project/certs",
    "/xyz/openbmc_project/console",
    "/xyz/openbmc_project/control",
    "/xyz/openbmc_project/dump",
    "/xyz/openbmc_project/events",
    "/xyz/openbmc_project/inventory",
    "/xyz/openbmc_project/ipmi",
    "/xyz/openbmc_project/led",
    "/xyz/openbmc_project/logging",
    "/xyz/openbmc_project/network",
    "/xyz/openbmc_project/object_mapper",
    "/xyz/openbmc_project/sensors",
    "/xyz/openbmc_project/software",
    "/xyz/openbmc_project/state",
    "/xyz/openbmc_project/time",
    "/xyz/openbmc_project/user"
  ],
  "message": "200 OK",
  "status": "ok"
}
```
This shows that there are 19 children of the `openbmc_project/` object: `dump`, `software`, `control`, `network`, `logging`,etc. This can be used with the base REST URL (ie., `http://${bmc}/`), to discover all objects in the hierarchy.
4. Performing the same query with `/list` will list the child objects *recursively*.
```
$ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/network/list
{
  "data": [
    "/xyz/openbmc_project/network/config",
    "/xyz/openbmc_project/network/config/dhcp",
    "/xyz/openbmc_project/network/eth0",
    "/xyz/openbmc_project/network/eth0/ipv4",
    "/xyz/openbmc_project/network/eth0/ipv4/3b9faa36",
    "/xyz/openbmc_project/network/eth0/ipv6",
    "/xyz/openbmc_project/network/eth0/ipv6/ff81b6d6",
    "/xyz/openbmc_project/network/eth1",
    "/xyz/openbmc_project/network/eth1/ipv4",
    "/xyz/openbmc_project/network/eth1/ipv4/3b9faa36",
    "/xyz/openbmc_project/network/eth1/ipv4/66e63348",
    "/xyz/openbmc_project/network/eth1/ipv6",
    "/xyz/openbmc_project/network/eth1/ipv6/ff81b6d6",
    "/xyz/openbmc_project/network/host0",
    "/xyz/openbmc_project/network/host0/intf",
    "/xyz/openbmc_project/network/host0/intf/addr",
    "/xyz/openbmc_project/network/sit0",
    "/xyz/openbmc_project/network/snmp",
    "/xyz/openbmc_project/network/snmp/manager"
  ],
  "message": "200 OK",
  "status": "ok"
}
```
5. Adding `/enumerate` instead of `/list` will also include the attributes of the listed objects.
```
$ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/time/enumerate
{
  "data": {
    "/xyz/openbmc_project/time/bmc": {
      "Elapsed": 1563209492098739
    },
    "/xyz/openbmc_project/time/host": {
      "Elapsed": 1563209492101678
    },
    "/xyz/openbmc_project/time/owner": {
      "TimeOwner": "xyz.openbmc_project.Time.Owner.Owners.BMC"
    },
    "/xyz/openbmc_project/time/sync_method": {
      "TimeSyncMethod": "xyz.openbmc_project.Time.Synchronization.Method.NTP"
    }
  },
  "message": "200 OK",
  "status": "ok"
}
```
### 1.3 HTTP PUT operations
PUT operations are for updating an existing resource (an object or property), or for creating a new resource when the client already knows where to put it. These require a json formatted payload. To get an example of what that looks like:
```
$ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/state/host0 > host.json
$ cat host.json
{
  "data": {
    "AttemptsLeft": 3,
    "BootProgress": "xyz.openbmc_project.State.Boot.Progress.ProgressStages.Unspecified",
    "CurrentHostState": "xyz.openbmc_project.State.Host.HostState.Off",
    "OperatingSystemState": "xyz.openbmc_project.State.OperatingSystem.Status.OSStatus.Inactive",
    "RequestedHostTransition": "xyz.openbmc_project.State.Host.Transition.Off"
  },
  "message": "200 OK",
  "status": "ok"
}
```
or
```
$ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition > requested_host.json
$ cat requested_host.json
{
  "data": "xyz.openbmc_project.State.Host.Transition.Off",
  "message": "200 OK",
  "status": "ok"
}
```
To make curl use the correct content type header use the -H option to specify that we're sending JSON data:
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d <json> <url>
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
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -T requested_host.json https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
```
Alternatively specify the json inline with -d:
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d '{"data": "xyz.openbmc_project.State.Host.Transition.On"}' https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
```
When using '-d' just remember that json requires quoting.
### 1.4 HTTP POST operations
POST operations are for calling methods, but also for creating new resources when the client doesn't know where to put it. OpenBMC does not support creating new resources via REST so any attempt to create a new resource will result in a HTTP 403 (Forbidden).
These also require a json formatted payload.
To delete logging entries:
```
$ curl -k -H "X-Auth-Token: $token" -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/logging/action/DeleteAll
```
To invoke a method without parameters (Factory Reset of BMC and Host):
```
$ curl -k -H "X-Auth-Token: $token" -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/software/action/Reset
```
### 1.5 HTTP DELETE operations
DELETE operations are for removing instances. Only D-Bus objects (instances) can be removed. If the underlying D-Bus object implements the `xyz.openbmc_project.Object.Delete` interface the REST server will call it. If `xyz.openbmc_project.Object.Delete` is not implemented, the REST server will return a HTTP 403 (Forbidden) error.
For example, to delete a event record:
Display logging entries:
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X GET https://${bmc}/xyz/openbmc_project/logging/entry/enumerate
{
  "data": {
    "/xyz/openbmc_project/logging/entry/1": {
      "AdditionalData": [
        "_PID=185"
      ],
      "Id": 1,
      "Message": "xyz.openbmc_project.Common.Error.InternalFailure",
      "Purpose": "xyz.openbmc_project.Software.Version.VersionPurpose.BMC",
      "Resolved": false,
      "Severity": "xyz.openbmc_project.Logging.Entry.Level.Error",
      "Timestamp": 1563191306359,
      "Version": "2.8.0-dev-132-gd1c1b74-dirty",
      "associations": []
    }
  },
  "message": "200 OK",
  "status": "ok"
}
```
Then delete the event record with ID 1:
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X DELETE  https://${bmc}/xyz/openbmc_project/logging/entry/1
{
  "data": null,
  "message": "200 OK",
  "status": "ok"
}
```
### 1.6 Uploading images
It is possible to upload software upgrade images (for example to upgrade the BMC or host software) via REST. The content-type should be set to "application/octet-stream".
For example, to upload an image:(the `file_to_upload` shell be a tar ball)
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/octet-stream" -X POST -T <file_to_upload> https://${bmc}/upload/image
```
The operation will either return the version id (hash) of the uploaded file on success:
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/octet-stream" -X POST -T 
obmc-phosphor-image-fp5280g2-20190906095051.static.mtd.tar https://${bmc}/upload/image
{
  "data": "afb92384",
  "message": "200 OK",
  "status": "ok"
}
```
For more details on uploading and updating software, see:
https://github.com/openbmc/docs/tree/master/code-update
## 2  Host Management with OpenBMC
This document describes the host-management interfaces of the OpenBMC object structure, accessible over REST.
### 2.1 Inventory
The system inventory structure is under the `/xyz/openbmc_project/inventory` hierarchy.
In OpenBMC the inventory is represented as a path which is hierarchical to the physical system topology. Items in the inventory are referred to as inventory items and are not necessarily FRUs (field-replaceable units). If the system contains one chassis, a motherboard, and a CPU on the motherboard, then the path to that inventory item would be:
`inventory/system/chassis0/motherboard0/cpu0`
The properties associated with an inventory item are specific to that item. Some common properties are:
• `Version`: A code version associated with this item.
• `Present`: Indicates whether this item is present in the system (True/False).
• `Functional`: Indicates whether this item is functioning in the system (True/False).
The usual `list` and `enumerate` REST queries allow the system inventory structure to be accessed. For example, to enumerate all inventory items and their properties:
```
$ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/inventory/enumerate
```
To list the properties of one item:
```
$ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/inventory/system/chassis/motherboard
```
### 2.2 Sensors
The system sensor structure is under the `/xyz/openbmc_project/sensors` hierarchy.
This interface allows monitoring of system attributes like temperature or altitude, and are represented similar to the inventory, by object paths under the top-level `sensors` object name. The path categorizes the sensor and shows what the sensor represents, but does not necessarily represent the physical topology of the system.
For example, all temperature sensors are under `sensors/temperature`. CPU temperature sensors would be `sensors/temperature/cpu`.
These are some common properties:

* `Value`: Current value of the sensor
* `Unit`: Unit of the value and “Critical” and “Warning” values
* `Scale`: The scale of the value and “Critical” and “Warning” values
* `CriticalHigh` & `CriticalLow`: Sensor device upper/lower critical threshold bound
* `CriticalAlarmHigh` & `CriticalAlarmLow`: True if the sensor has exceeded the critical threshold bound
* `WarningHigh` & `WarningLow`: Sensor device upper/lower warning threshold bound
* `WarningAlarmHigh` & `WarningAlarmLow`: True if the sensor has exceeded the warning threshold bound
A temperature sensor might look like:
```
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
```
Note the value of this sensor is 33.625℃ (34625 * 10^-3).
Unlike IPMI, there are no “functional” sensors in OpenBMC; functional states are represented in the inventory.
To enumerate all sensors in the system:
```
$ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/sensors/enumerate
```
List properties of one inventory item:
```
$ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/sensors/temperature/outlet
```
### 2.3 Event Logs
The event log structure is under the `/xyz/openbmc_project/logging/entry` hierarchy. Each event is a separate object under this structure, referenced by number.
BMC and host firmware on POWER-based servers can report event logs to the BMC. Typically, these event logs are reported in cases where host firmware cannot start the OS, or cannot reliably log to the OS.
The properties associated with an event log are as follows:
* `Message`: The type of event log (e.g. “xyz.openbmc_project.Inventory.Error.NotPresent”).
* `Resolved`: Indicates whether the event has been resolved.
* `Severity`: The level of problem (“Info”, “Error”, etc.).
* `Timestamp`: The date of the event log in epoch time.
* `Associations`: A URI to the failing inventory part.
To list all reported event logs:
```
$ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/logging/entry
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
```
To delete an event log (log 1 in this example), call the `delete` method on the event:
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST -d '{"data" : []}' https://${bmc}/xyz/openbmc_project/logging/entry/1/action/Delete
```
To clear all event logs, call the top-level `deleteAll` method:
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST -d '{"data" : []}' https://${bmc}/xyz/openbmc_project/logging/action/deleteAll
```
### 2.4 Host Boot Options
With OpenBMC, the Host boot options are stored as D-Bus properties under the `control/host0/boot` path. Properties include
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Control/Boot/Mode.interface.yaml
and
 https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Control/Boot/Source.interface.yaml.
- Set boot mode:
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT https://${bmc}/xyz/openbmc_project/control/host0/boot/one_time/attr/BootMode -d '{"data": "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular"}'
```
- Set boot source:
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT https://${bmc}/xyz/openbmc_project/control/host0/boot/one_time/attr/BootSource -d '{"data": "xyz.openbmc_project.Control.Boot.Source.Sources.Default"}
```
### 2.5 Host State Control
The host can be controlled through the `host` object. The object implements a number of actions including `power on` and `power off`. These correspond to the IPMI power on and power off commands.
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
More information about Host State Management can be found here: https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/xyz/openbmc_project/State
### 2.6  Host Clear GARD
On OpenPOWER systems, the host maintains a record of bad or non-working components on the GARD partition. This record is referenced by the host on subsequent boots to determine which parts should be ignored.
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST -d '{"data":[]}' https://${bmc}/org/open_power/control/gard/action/Reset
```
Implementation: https://github.com/openbmc/openpower-pnor-code-mgmt
## 3  OpenBMC host console support
This document describes how to connect to the host UART console from an OpenBMC management system.
The console infrastructure allows multiple shared connections to a single host UART. UART data from the host is output to all connections, and input from any connection is sent to the host.
### 3.1 Remote console connections
To connect to an OpenBMC console session remotely, just ssh to your BMC on port 2200. Use the same login credentials you would for a normal ssh session:
```
$ ssh root@xx.xx.xx.xx -p 2200
```
### 3.2 Local console connections
If you're already logged into an OpenBMC machine, you can start a console session directly, using:
```
$ obmc-console-client
```
### 3.3 Logging
Console logs are kept in:
```
/var/log/obmc-console.log
```
This log is limited in size, and will wrap after hitting that limit (currently set at 16kB).
## 4 Cheatsheet
Note: The login method is:
```
$ export bmc=xx.xx.xx.xx
$ export token=`curl -k -H "Content-Type: application/json" -X POST https://${bmc}/login -d '{"username" :  "root", "password" :  "0penBmc"}' | grep token | awk '{print $2;}' | tr -d '"'`
$ curl -k -H "X-Auth-Token: $token" https://$bmc/xyz/openbmc_project/...
```
### 4.1 List and enumerate
```
$ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/list
$ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/enumerate
```
### 4.2 List sub-objects
```
$ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/
$ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/state/
```
### 4.3 Host power
- Host soft power off:
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -d '{"data": "xyz.openbmc_project.State.Host.Transition.Off"}' -X PUT https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
```
- Host hard power off:
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d '{"data":"xyz.openbmc_project.State.Chassis.Transition.Off"}' https://${bmc}//xyz/openbmc_project/state/chassis0/attr/RequestedPowerTransition
```
- Host power on:
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -d '{"data": "xyz.openbmc_project.State.Host.Transition.On"}' -X PUT https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
```
- Reboot Host:
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d '{"data":"xyz.openbmc_project.State.Host.Transition.Reboot"}' https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
```
### 4.4 Reboot BMC
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d '{"data":"xyz.openbmc_project.State.BMC.Transition.Reboot"}' https://${bmc}//xyz/openbmc_project/state/bmc0/attr/RequestedBMCTransition
```
### 4.5 Logging entries
- Display logging entries:
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X GET https://${bmc}/xyz/openbmc_project/logging/entry/enumerate
```
- Delete logging entries:
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X DELETE https://${bmc}/xyz/openbmc_project/logging/entry/<entry_id>
$ curl -k -H "X-Auth-Token: $token" -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/logging/action/DeleteAll
```
### 4.6 Delete dump entries
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X DELETE https://${bmc}/xyz/openbmc_project/dump/entry/<entry_id>
$ curl -k -H "X-Auth-Token: $token" -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/dump/action/DeleteAll
```
### 4.7 Set NTP and Nameserver
Examples using public server.
* NTP server:
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d '{"data": ["pool.ntp.org"] }' https://${bmc}/xyz/openbmc_project/network/eth0/attr/NTPServers
```
* Name server:
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d '{"data": ["time.google.com"] }' https://${bmc}/xyz/openbmc_project/network/eth0/attr/Nameservers
```
### 4.8 Configure time ownership and time sync method
The introduction about time setting is here: 
https://github.com/openbmc/phosphor-time-manager
Note: Starting from OpenBMC 2.6 (with systemd v239), systemd's timedated introduces a new beahvior that it checks the NTP services' status during setting time, instead of checking the NTP setting:
-When NTP server is set to disabled, and the NTP service is stopping but not stopped, setting time will get an error.
In OpenBMC 2.4 (with systemd v236), the above will always succeed.
This results in [openbmc/openbmc#3459](https://github.com/openbmc/openbmc/issues/3459), and the related test cases are updated to cooperate with this behavior change.
* Read:
```
$ curl -k -X -H "X-Auth-Token: $token" GET https://${bmc}/xyz/openbmc_project/time/owner/attr/TimeOwner
$ curl -k -X -H "X-Auth-Token: $token" GET https://${bmc}/xyz/openbmc_project/time/sync_method/attr/TimeSyncMethod
```
* Write:
  Time owner:
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Owner.Owners.BMC" }' https://${bmc}/xyz/openbmc_project/time/owner/attr/TimeOwner
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Owner.Owners.Host” }' https://${bmc}/xyz/openbmc_project/time/owner/attr/TimeOwner
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Owner.Owners.Split" }' https://${bmc}/xyz/openbmc_project/time/owner/attr/TimeOwner
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Owner.Owners.Both” }' https://${bmc}/xyz/openbmc_project/time/owner/attr/TimeOwner
```
  Time sync method:
```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Synchronization.Method.NTP" }' https://${bmc}/xyz/openbmc_project/time/sync_method/attr/TimeSyncMethod
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Synchronization.Method.Manual" }' https://${bmc}/xyz/openbmc_project/time/sync_method/attr/TimeSyncMethod
```
### 4.9 Factory Reset
- Factory reset host and BMC software:
```
$ curl -k -H "X-Auth-Token: $token" -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/software/action/Reset
```
- Factory reset network setting:
```
$ curl -k -H "X-Auth-Token: $token" -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/network/action/Reset
```
- Enable field mode:
```
$ curl -k -H "X-Auth-Token: $token" -H 'Content-Type: application/json' -X PUT -d '{"data":1}' https://${bmc}/xyz/openbmc_project/software/attr/FieldModeEnabled
```
and then reboot BMC.