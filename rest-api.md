# OpenBMC REST API

The primary management interface for OpenBMC is REST. This document provides
some basic structure and usage examples for the REST interface.

The schema for the rest interface is directly defined by the OpenBMC D-Bus
structure. Therefore, the objects, attributes and methods closely map to those
in the D-Bus schema.

For a quick explanation of HTTP verbs and how they relate to a RESTful API, see
<http://www.restapitutorial.com/lessons/httpmethods.html>.

## Logging in

Before you can do anything you first need to log in:

    curl -c cjar -k -X POST -H "Content-Type: application/json" \
        -d '{"data": [ "root", "0penBmc" ] }' \
        https://bmc/login


This performs a login using the provided username and password, and stores the
newly-acquired authentication credentials in the `curl` "Cookie jar" file (named
`cjar` in this example). We will use this file (with the `-b` argument) for
future requests.

To log out:

    curl -c cjar -b cjar -k -X POST -H "Content-Type: application/json" \
        -d '{"data": [ ] }' \
	https://bmc/logout

(or just delete your cookie jar file)

## HTTP GET operations & URL structure

There are a few conventions on the URL structure of the OpenBMC rest interface.
They are:

 - To query the attributes of an object, perform a GET request on the object
   name, with no trailing slash. For example:

        $ curl -b cjar -k https://bmc/org/openbmc/inventory/system
        {
          "data": {
            "Asset Tag": [],
            "Custom Field 1": "\tbuildroot-dcb6dc3",
            "Custom Field 2": "\tskiboot-5.1.15",
            "Custom Field 3": "\thostboot-c223637-017f5fd",
            "Custom Field 4": "\tlinux-4.4.6-openpower1-2291fe8",
            "Custom Field 5": "\tpetitboot-72928ed-a75299d",
            "Custom Field 6": "\tpalmetto-xml-8281868-6b8b2bb",
            "Custom Field 7": "\tocc-1093bf9",
            "Custom Field 8": "\thostboot-binaries-7f593a3",
            "FRU File ID": [],
            "Manufacturer": [],
            "Model Number": [],
            "Name": "OpenPOWER Firmware",
            "Serial Number": [],
             "Version": "open-power-palmetto-5a4a3d9",
            "fault": "False",
            "fru_type": "SYSTEM",
            "is_fru": 1,
            "present": "False",
            "version": ""
          },
          "message": "200 OK",
          "status": "ok"
        }

 - To query a single attribute, use the `attr/<name>` path. Using the
   `system` object from above, we can query just the `Name` value:

        $ curl -b cjar -k https://bmc/org/openbmc/inventory/system/attr/Name
        {
          "data": "OpenPOWER Firmware",
          "message": "200 OK",
          "status": "ok"
        }

 - When a path has a trailing-slash, the response will list the sub objects of
   the URL. For example, using the same object path as above, but adding a
   slash:

        $ curl -b cjar -k https://bmc/org/openbmc/inventory/system/
        {
          "data": [
            "/org/openbmc/inventory/system/systemevent",
            "/org/openbmc/inventory/system/chassis"
          ],
          "message": "200 OK",
          "status": "ok"
        }

   This shows that there are two children of the `system/` object: `systemevent`
   and `chassis`. This can be used with the base REST URL (ie., `http://bmc/`),
   to discover all objects in the hierarchy.

 - Performing the same query with `/list` will list the child objects
   *recursively*.

        $ curl -b cjar -k https://palm5-bmc/list
        [output omitted]

 - Adding `/enumerate` instead of `/list` will also include the attributes of
   the listed objects.


## HTTP PUT operations

PUT operations are for updating an existing resource (an object or property), or
for creating a new resource when the client already knows where to put it.
These require a json formatted payload. To get an example of what that looks
like:

    curl -b cjar -k \
        https://bmc/org/openbmc/control/flash/bios > bios.json

or

    curl -b cjar -k \
        https://bmc/org/openbmc/control/flash/bios/attr/flasher_path > flasher_path.json

When turning around and sending these as requests, delete the message and status
properties.

To make curl use the correct content type header use the -H option to specify
that we're sending JSON data:

    curl -b cjar -k -H "Content-Type: application/json" -X POST -d <json> <url>

A PUT operation on an object requires a complete object. For partial updates
there is PATCH but that is not implemented yet. As a workaround individual
attributes are PUTable.

For example, make changes to the file and do a PUT (upload):


    curl -b cjar -k -H "Content-Type: application/json" \
        -X PUT -T bios.json \
        https://bmc/org/openbmc/control/flash/bios

    curl -b cjar -k -H "Content-Type: application/json" \
        -X PUT -T flasher_path.json \
        https://bmc/org/openbmc/control/flash/bios/attr/flasher_path


Alternatively specify the json inline with -d:

    curl -b cjar -k -H "Content-Type: application/json" -X PUT
        -d '{"data": <value>}' \
        https://bmc/org/openbmc/control/flash/bios/attr/flasher_path

When using '-d' just remember that json requires quoting.

## HTTP POST operations
POST operations are for calling methods, but also for creating new resources
when the client doesn't know where to put it. OpenBMC does not support creating
new resources via REST so any attempt to create a new resource will result in a
HTTP 403 (Forbidden).

These also require a json formatted payload.

To invoke a method with parameters:

    curl -b cjar -k -H "Content-Type: application/json" -X POST \
        -d '{"data": [<positional-parameters>]}' \
        https://bmc/org/openbmc/control/fan0/action/setspeed

To invoke a method without parameters:

    curl -c cjar -b cjar -k -H "Content-Type: application/json" -X POST \
        -d '{"data": []}' \
        https://bmc/org/openbmc/control/fan0/action/getspeed


## HTTP DELETE operations
DELETE operations are for removing instances. Only D-Bus objects (instances) can
be removed. If the underlying D-Bus object implements the
`org.openbmc.Object.Delete` interface the REST server will call it. If
`org.openbmc.Object.Delete` is not implemented, the REST server will return a
HTTP 403 (Forbidden) error.

For example, to delete the event record with ID 0:

   curl -b cjar -k -X DELETE \
       https://bmc/org/openbmc/events/record/0


## Uploading images
It is possible to upload software upgrade images (for example to upgrade the BMC
or host software) via REST. The content-type should be set to
"application/octet-stream".

For example, to upload an image:

    curl -c cjar -b cjar -k -H "Content-Type: application/octet-stream" \
        -X POST -T <file_to_upload> https://bmc/upload/image

In above example, the filename on the BMC will be chosen by the REST server.

It is possible for the user to choose the uploaded file's remote name:

    curl -c cjar -b cjar -k -H "Content-Type: application/octet-stream" \
        -X PUT -T foo https://bmc/upload/image/bar

In above example, the file foo will be saved with the name bar on the BMC.

## Event subscription protocol
It is possible to subscribe to events, of interest, occurring on the BMC. The
implementation on the BMC uses WebSockets for this purpose, so that clients
don't have do employ polling. Instead, the rest server on the BMC can push
data to clients over a websocket. The BMC can push out information
pertaining to D-Bus InterfacesAdded and PropertiesChanged signals.

Following is a description of the event subscription protocol, with example
JS code snippets denoting client-side code.

a) The client needs to have logged on to the BMC.
b) The client needs to open a secure websocket with the URL <BMC IP>/subscribe.

```
   var ws = new WebSocket("wss://<BMC IP>/subscribe")
```

c) The client needs to send, over the websocket, a JSON dictionary, comprising
   of key-value pairs. This dictionary serves as the "events filter". All the
   keys are optinal, so the dictionary can be empty if no filtering is desired.
   The filters represented by each of the key-value pairs are ORed.

   One of the supported keys is "paths". The corresponding value is an array of
   D-Bus paths. The InterfacesAdded and PropertiesChanged D-Bus signals
   emanating from any of these path(s) alone, and not from any other paths, will
   be included in the event message going out of the BMC.

   The other supported key is "interfaces". The corresponding value is an
   array of D-Bus interfaces. The InterfacesAdded and PropertiesChanged D-Bus
   signal messages comprising of any of these interfaces will be included in
   the event message going out of the BMC.

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
   occurs, considering the client supplied filters. The rest servers notifies
   about InterfacesAdded and PropertiesChanged events. The reponse is a JSON
   dictionary as follows :

   InterfacesAdded
   ```
   "event": InterfacesAdded
   "path": <string : new D-Bus path that was created>
   "intfMap": <dict : a dictionary of interfaces> (similar to org.freedesktop.DBus.ObjectManager.InterfacesAdded )
   ```

   PropertiesChanged
   ```
   "event": PropertiesChanged
   "path": <string : D-Bus path whose property changed>
   "intf": <string : D-Bus interface to which the changed property belongs>
   "propMap": <dict : a dictionary of properties> (similar to org.freedesktop.DBus.Properties.PropertiesChanged)
   ```
