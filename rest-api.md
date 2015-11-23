# OpenBMC REST API
## HTTP GET operations
List directory:
```
curl -k https://bmc/<path>/
```
Examples:
```
curl -k https://bmc/
curl -k https://bmc/org/
curl -k https://bmc/org/openbmc/
curl -k https://bmc/org/openbmc/inventory/
curl -k https://bmc/org/openbmc/inventory/system/chassis/
```

List objects recursively:
```
curl -k https://bmc/<path>/list
```
Examples:
```
curl -k https://bmc/list
curl -k https://bmc/org/openbmc/inventory/list
```

Enumerate objects recursively:
```
curl -k https://bmc/<path>/enumerate
```
Examples:
```
curl -k https://bmc/enumerate
curl -k https://bmc/org/openbmc/inventory/enumerate

```

Get object:
```
curl -k https://bmc/<path>
```
Examples:
```
curl -k https://bmc/org/openbmc/inventory/system/chassis/fan2
```

Get property:
```
curl -k https://bmc/<path>/attr/<attr>
curl -k https://bmc/org/openbmc/inventory/system/chassis/fan2/attr/is_fru
```

## HTTP PUT operations
PUT operations are for updating an existing resource ( an object or property ), or for creating a new resource when the client already knows where to put it.
For a quick explanation of HTTP verbs and how they relate to a RESTful API see this page:
```
http://www.restapitutorial.com/lessons/httpmethods.html
```

These require a json formatted payload.  To get an example of what that looks like:
```
curl -k https://bmc/org/openbmc/control/flash/bios > bios.json # - or -
curl -k https://bmc/org/openbmc/control/flash/bios/attr/flasher_path > flasher_path.json
```

When turning around and sending these as requests, delete the message and status properties.

To make curl use the correct content type header use the -H option:
```
curl -k -H "Content-Type: application/json" -X POST -d <json> <url>
```
A put operation on an object requires a complete object.  For partial updates there is PATCH but that is not implemented yet.  As a workaround individual attributes are PUTable.

Make changes to the file and do a put (upload):

```
curl -k -H "Content-Type: application/json" -X PUT -T bios.json https://bmc/org/openbmc/control/flash/bios
curl -k -H "Content-Type: application/json" -X PUT -T flasher_path.json https://bmc/org/openbmc/control/flash/bios/attr/flasher_path
```

Alternatively specify the json inline with -d:
```
curl -k -H "Content-Type: application/json" -X PUT -d "{\"data\": <value>}" flasher_path.json https://bmc/org/openbmc/control/flash/bios/attr/flasher_path
```

When using '-d' Just remember that json requires double quotes and any shell metacharacters need to be escaped.

## HTTP POST operations
POST operations are for calling methods, but also for creating new resources when the client doesn't know where to put it.  OpenBMC does not support creating new resources via REST so any attempt to create a new resource will result in a HTTP 403 ( Forbidden ).
These also require a json formatted payload.

To invoke a method with parameters:
```
curl -k -H "Content-Type: application/json" -X POST -d "{\"data\": [<positional-parameters>]}" https://bmc/org/openbmc/control/fan0/action/setspeed
```
To invoke a method without parameters:
```
curl -k -H "Content-Type: application/json" -X POST -d "{\"data\": []}" https://bmc/org/openbmc/control/fan0/action/getspeed
```

## HTTP DELETE operations
DELETE operations are for removing instances.  Only DBUS objects (instances) can be removed.  If the underlying DBUS object implements the org.openbmc.Object.Delete interface the REST server will call it.  If org.openbmc.Object.Delete is not implemented, HTTP 403 will be returned.

```
curl -k -X DELETE https://bmc/org/openbmc/events/record/0
```
