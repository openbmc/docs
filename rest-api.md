# OpenBMC REST API

The OpenBMC REST API is disabled by default in bmcweb. Most of the functionality
is available on Redfish. This document provides some basic structure and usage
examples for the OpenBMC REST interface.

The schema for the rest interface is directly defined by the OpenBMC D-Bus
structure. Therefore, the objects, attributes and methods closely map to those
in the D-Bus schema.

For a quick explanation of HTTP verbs and how they relate to a RESTful API, see
<http://www.restapitutorial.com/lessons/httpmethods.html>.

## Authentication

See the details on authentication at
[REST-cheatsheet](https://github.com/openbmc/docs/blob/master/REST-cheatsheet.md#establish-rest-connection-session).

This tutorial uses token based authentication method:

```
$ export bmc=xx.xx.xx.xx
$ export token=`curl -k -H "Content-Type: application/json" -X POST https://${bmc}/login -d '{"username" :  "root", "password" :  "0penBmc"}' | grep token | awk '{print $2;}' | tr -d '"'`
$ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/...
```

## HTTP GET operations & URL structure

There are a few conventions on the URL structure of the OpenBMC rest interface.
They are:

- To query the attributes of an object, perform a GET request on the object
  name, with no trailing slash. For example:

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

- To query a single attribute, use the `attr/<name>` path. Using the `system`
  object from above, we can query just the `Name` value:

  ```
  $ curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/inventory/system/attr/Cached
  {
   "data": true,
   "message": "200 OK",
   "status": "ok"
  }
  ```

- When a path has a trailing-slash, the response will list the sub objects of
  the URL. For example, using the same object path as above, but adding a slash:

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

  This shows that there are 19 children of the `openbmc_project/` object:
  `dump`, `software`, `control`, `network`, `logging`,etc. This can be used with
  the base REST URL (ie., `http://${bmc}/`), to discover all objects in the
  hierarchy.

- Performing the same query with `/list` will list the child objects
  _recursively_.

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

- Adding `/enumerate` instead of `/list` will also include the attributes of the
  listed objects.

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

## HTTP PUT operations

PUT operations are for updating an existing resource (an object or property), or
for creating a new resource when the client already knows where to put it. These
require a json formatted payload. To get an example of what that looks like:

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

When turning around and sending these as requests, delete the message and status
properties.

To make curl use the correct content type header use the -H option to specify
that we're sending JSON data:

```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d <json> <url>
```

A PUT operation on an object requires a complete object. For partial updates
there is PATCH but that is not implemented yet. As a workaround individual
attributes are PUTable.

For example, make changes to the requested_host.json file and do a PUT (upload):

```
$ cat requested_host.json
{"data": "xyz.openbmc_project.State.Host.Transition.Off"}
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -T requested_host.json https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
```

Alternatively specify the json inline with -d:

```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d '{"data": "xyz.openbmc_project.State.Host.Transition.On"}' https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
```

When using '-d' just remember that json requires quoting.

## HTTP POST operations

POST operations are for calling methods, but also for creating new resources
when the client doesn't know where to put it. OpenBMC does not support creating
new resources via REST so any attempt to create a new resource will result in a
HTTP 403 (Forbidden). These also require a json formatted payload. To delete
logging entries:

```
$ curl -k -H "X-Auth-Token: $token" -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/logging/action/DeleteAll
```

To invoke a method without parameters (Factory Reset of BMC and Host):

```
$ curl -k -H "X-Auth-Token: $token" -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/software/action/Reset
```

## HTTP DELETE operations

DELETE operations are for removing instances. Only D-Bus objects (instances) can
be removed. If the underlying D-Bus object implements the
`xyz.openbmc_project.Object.Delete` interface the REST server will call it. If
`xyz.openbmc_project.Object.Delete` is not implemented, the REST server will
return a HTTP 403 (Forbidden) error.

For example, to delete a event record:

Display logging entries:

```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X GET https://${bmc}/xyz/openbmc_project/logging/entry/enumerate
```

Then delete the event record with ID 1:

```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X DELETE  https://${bmc}/xyz/openbmc_project/logging/entry/1
```

## Uploading images

It is possible to upload software upgrade images (for example to upgrade the BMC
or host software) via REST. The content-type should be set to
"application/octet-stream".

For example, to upload an image:(the `<file_to_upload>` must be a tar ball)

```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/octet-stream" -X POST -T <file_to_upload> https://${bmc}/upload/image
```

In above example, the filename on the BMC will be chosen by the REST server.

It is possible for the user to choose the uploaded file's remote name:

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/octet-stream" -X PUT -T foo https://${bmc}/upload/image/bar
```

In above example, the file foo will be saved with the name bar on the BMC.

The operation will either return the version id (hash) of the uploaded file on
success:

```
{
  "data": "afb92384",
  "message": "200 OK",
  "status": "ok"
}
```

or an error message

```
{
    "data": {
        "description": "Version already exists or failed to be extracted"
    },
    "message": "400 Bad Request",
    "status": "error"
}
```

For more details on uploading and updating software, see:
https://github.com/openbmc/docs/tree/master/architecture/code-update

## Event subscription protocol

It is possible to subscribe to events, of interest, occurring on the BMC. The
implementation on the BMC uses WebSockets for this purpose, so that clients
don't have do employ polling. Instead, the rest server on the BMC can push data
to clients over a websocket. The BMC can push out information pertaining to
D-Bus InterfacesAdded and PropertiesChanged signals.

Following is a description of the event subscription protocol, with example JS
code snippets denoting client-side code.

a) The client needs to have logged on to the BMC. b) The client needs to open a
secure websocket with the URL <BMC IP>/subscribe.

```
   var ws = new WebSocket("wss://<BMC IP>/subscribe")
```

c) The client needs to send, over the websocket, a JSON dictionary, comprising
of key-value pairs. This dictionary serves as the "events filter". All the keys
are optional, so the dictionary can be empty if no filtering is desired. The
filters represented by each of the key-value pairs are ORed.

One of the supported keys is "paths". The corresponding value is an array of
D-Bus paths. The InterfacesAdded and PropertiesChanged D-Bus signals emanating
from any of these path(s) alone, and not from any other paths, will be included
in the event message going out of the BMC.

The other supported key is "interfaces". The corresponding value is an array of
D-Bus interfaces. The InterfacesAdded and PropertiesChanged D-Bus signal
messages comprising of any of these interfaces will be included in the event
message going out of the BMC.

All of the following are valid:

```
var data = JSON.stringify(
{
    "paths": ["/xyz/openbmc_project/logging", "/xyz/openbmc_project/sensors"],
    "interfaces": ["xyz.openbmc_project.Logging.Entry", "xyz.openbmc_project.Sensor.Value"]
});
ws.onopen = function() {
    ws.send(data);
};
```

```
var data = JSON.stringify(
{
    "paths": ["/xyz/openbmc_project/logging", "/xyz/openbmc_project/sensors"],
});
ws.onopen = function() {
    ws.send(data);
};
```

```
var data = JSON.stringify(
{
    "interfaces": ["xyz.openbmc_project.Logging.Entry", "xyz.openbmc_project.Sensor.Value"]
});
ws.onopen = function() {
    ws.send(data);
};
```

```
var data = JSON.stringify(
{
});
ws.onopen = function() {
    ws.send(data);
};
```

d) The rest server on the BMC will respond over the websocket when a D-Bus event
occurs, considering the client supplied filters. The rest servers notifies about
InterfacesAdded and PropertiesChanged events. The response is a JSON dictionary
as follows :

InterfacesAdded

```
"event": InterfacesAdded
"path": <string : new D-Bus path that was created>
"interfaces": <dict : a dictionary of interfaces> (similar to org.freedesktop.DBus.ObjectManager.InterfacesAdded )
```

PropertiesChanged

```
"event": PropertiesChanged
"path": <string : D-Bus path whose property changed>
"interface": <string : D-Bus interface to which the changed property belongs>
"properties": <dict : a dictionary of properties> (similar to org.freedesktop.DBus.Properties.PropertiesChanged)
```
