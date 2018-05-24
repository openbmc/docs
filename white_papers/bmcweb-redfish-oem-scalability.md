# Bmcweb Redfish OEM schema scalability proposal

## Disclaimer

This document describes a method of how to address customization of different
redfish schemas under `bmcweb` (`OpenBMC`) depending on OEM needs.

Please note that this is a Request For Comments document, and is not officially
approved design. Some or even all assumptions here might be changed depending on
discussion on community needs.

The concept presented here has been proven by Proof of Concept, presented on
weekly `OpenBMC Redfish Super Group` meeting.

## Description

The idea is to allow Redfish to blindly map Redfish JSON schemas into certain
D-Bus calls to appropriate D-Bus Service. DBus Service shall implement certain
standardize signals, as well as specially defined Redfish D-Bus interfaces.

This document will cover the following use cases:
* Add new OEM custom schema
* Add new OEM custom collection schema
* Add OEM extension to existing OCP Profile implemented schemas
* Add custom fields to existing OCP Profile implemented schemas (This shall be
  widely discussed, since it may arise may concerns)
* Same Redfish Path available among two or more service (i.e. two services are
  preparing same Schema, but are responsible for filling different properties)

This document does not cover (but the following are under consideration and internal design)
* Redfish Interface for taking actions
* As for now the intention is NOT to extend already existing collections
  (However this might be possible, and may change in future)
* Errors returned through POST, PATCH, and DELETE methods
* Schema reference description for service validation (including auto-update
  of `$metadata`)


When the HTTP method comes to `bmcweb` application with requested URI not directly
implemented (usually out of existing OCP Profile), The `bmcweb` is going to
make internal lookup into `OemDynamicUriLookuptable` based on URL.
While lookup succeed it will make a direct D-Bus call to appropriate method
(HTTP method mapped 1:1 to D-Bus methods) and will retrieve a D-Bus object
that can be easily transferred 1:1 into JSON format. In this case `bmcweb` is
not doing any logic (besides permissions check), and all of the work is being
deferred to appropriate service.

This method allows to runtime dynamically plug in OEM schemas, or it parts.

## Redfish startup

During `bmcweb` startup process it will register for the following signals from
`org.freedesktop.DBus.ObjectManager` interface:
* `InterfacesAdded`
* `InterfacesRemoved`

The signal handler shall search for newly created/removed D-Bus interfaces:
`xyz.openbmc_project.Redfish.*`. Upon the signal retrieval, the
`OemDynamicUriLookuptable` will be updated.

In case `bmcweb` is not going to startup before Services exposing Redfish
interface, `bmcweb` shall ask `ObjectMapper` for all services that implements
`xyz.openbmc_project.Redfish.Config`

## OEM Application exposing Redfish D-Bus interfaces

The OEM application that would like to serve Redfish over D-Bus, shall implement
the following interfaces, with appropriate properties and methods, on given
Object Path, that starts with `/xyz/openbmc_project/redfish/v1/XXX/YYY/ZZZ`

The minimal set of interfaces required for Redfish to present particular schema are:
`xyz.openbmc_project.Redfish.Config`
`xyz.openbmc_project.Redfish`

### Interfaces detailed description

#### Interface `xyz.openbmc_project.Redfish.Config`

This shall implement basic configuration knowledge for OEM extensions
* `Version` - schema Version (for dynamically `@odata.*` fields creation)
* `Type` - to determine single schema/collection
* `Privileges` - a dictionary that contains required privileges required to
access this resource

example:
```
{
    "Version": "1_0_0",
    "Type": "Schema",
    "Privileges": {
        "GET": ["Login", "ConfigureManagers"],
        "POST": ["Login", "ConfigureManagers"],
        "PATCH": ["Login", "ConfigureManagers"],
        "DELETE": ["Login", "ConfigureManagers"]
    }
}
```

#### Interface `xyz.openbmc_project.Redfish`

This shall implement all the methods that given OEM extension shall expose over HTTP
* GET
  * input: None
  * returns: `a{sv}`
* POST
  * input: desired format shall follow `a{sv}` guidelines to be easily parsed from JSON
  * returns: `a{sv}`
* PATCH:
  * input: `a{sv}`
  * returns: `a{sv}`
* DELETE:
  * input: None
  * returns: `a{sv}`

#### Interface `xyz.openbmc_project.Redfish.OEM`

This is being used to fill OEM fields for certain schema in that case the
`Config` interface shall point to an existing schema, as well as the object path, and
shall implement appropriate methods similar to the above Redfish interface.

* GET
  * input: None
  * returns: `a{sv}`
* POST
  * input: desired format shall follow `a{sv}` guidelines to be easily parsed from JSON
  * returns: `a{sv}`
* PATCH:
  * input: `a{sv}`
  * returns: `a{sv}`

#### Interface `xyz.openbmc_project.Redfish.Collection`

`Note: This might not be necessary - currently investigating collection auto detection
based on object_path`

This interface shall be implemented only on items that are part of a collection,
and shall consists of one property called `CollectionId` which describes collection Id
it will link to.

Example:

```
# Collection:

/xyz/openbmc_project/redfish/v1/oemcollection
- xyz.openbmc_project.Redfish.Config:
  'Service DBus name': "xyz.openbmc_project.myOEMRedfishService",
  'Id': "myOEMRedfishService",
  'Version': "1_0_0",
  'Type': "Collection",
  "Privileges": {"GET": ["Login", "ConfigureComponents"], "POST": .....}

# Collection Item:
`/xyz/openbmc_project/redfish/v1/oemcollection/oemext1`
- xyz.openbmc_project.Redfish.Config:
  'Service DBus name': "xyz.openbmc_project.myOEMRedfishService",
  'Id': "myOEMRedfishServiceItem",
  'Version': "1_0_0",
  'Type': "SingleSchema",
  "Privileges": {"GET": ["Login", "ConfigureComponents"], "POST": .....}

- xyz.openbmc_project.Redfish
  'Get': .......

- xyz.openbmc_project.Redfish.Collection:
  'CollectionId': "myOEMRedfishService"
```

### Python MOCK example:
Below is a fully functional example of implementing a simple OEM Extention in Python.


```python
import gobject
import dbus
import dbus.service
import dbus.mainloop.glib
import threading

class RootObject(dbus.service.Object):
    def __init__(self, bus):
        self.object_name = "/xyz/openbmc_project/redfish/v1/MyOemCustomService"
        dbus.service.Object.__init__(self, bus, self.object_name)

        self.redfish_config = {
            "Version": "1_0_0",
            "Type": "Schema",
            "Privileges": {
                "GET": ["Login", "ConfigureManagers"],
                "POST": ["Login", "ConfigureManagers"],
                "PATCH": ["Login", "ConfigureManagers"],
                "DELETE": ["Login", "ConfigureManagers"]
            }}

        self.redfish_properties = dbus.Dictionary({
                "Id": "OEMCustomService",
                "Name": "OEMCustomService",
                "Description": "OEMCustomService",
                "Status": dbus.Dictionary({
                    "State": "Disabled",
                    "Health": "OK"
                    }, signature='sv'),
                "CustomBoolProperty": True,
                "CustomNumericProperty": 0x123,
            }, signature='sv')

        self.InterfacesAdded(
            self.object_name, {
                "xyz.openbmc_project.Redfish.Config": self.redfish_config,
                "xyz.openbmc_project.Redfish": self.redfish_properties
            })

    def release(self):
        self.InterfacesRemoved(
            self.object_name, [
                "xyz.openbmc_project.Redfish.Config",
                "xyz.openbmc_project.Redfish"
            ])

    @dbus.service.signal('org.freedesktop.DBus.ObjectManager', signature='oa{sa{sv}}')
    def InterfacesAdded(self, object_path, interfaces):
        pass

    @dbus.service.signal('org.freedesktop.DBus.ObjectManager', signature='oas')
    def InterfacesRemoved(self, object_path, interfaces):
        pass

    @dbus.service.method('org.freedesktop.DBus.ObjectManager', in_signature="", out_signature='a{oa{sa{sv}}}')
    def GetManagedObjects(self):
        return {self.object_name: {
            "xyz.openbmc_project.Redfish.Config": self.redfish_config,
            "xyz.openbmc_project.Redfish": self.redfish_properties
        }}

    @dbus.service.method('xyz.openbmc_project.Redfish', in_signature="", out_signature='a{sv}')
    def Get(self):
        return self.redfish_properties

if __name__ == "__main__":
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
    bus = dbus.SystemBus()
    name = dbus.service.BusName("xyz.openbmc_project.RedfishTestApp", bus)
    gobject.threads_init()
    root = RootObject(bus)


    loop = gobject.MainLoop()
    try:
        loop.run()
    except:
        print("Exitting.")
    finally:
        root.release()
```

