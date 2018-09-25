# The ObjectMapper

The `xyz.openbmc_project.ObjectMapper` service, commonly referred to as just
the mapper, is an OpenBMC application that attempts to ease the pain of
using D-Bus by providing APIs that help in discovering and associating
other D-Bus objects.

The mapper has two major pieces of functionality:

- [Methods](#methods) - Provides D-Bus discovery related functionality.
- [Associations](#associations) - Associates two different objects with each other.

## Methods

The official YAML interface definition can be found [here] (https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/ObjectMapper.interface.yaml).

```
busctl introspect --no-pager xyz.openbmc_project.ObjectMapper /xyz/openbmc_project/object_mapper
NAME                                     TYPE      SIGNATURE RESULT/VALUE FLAGS
...
xyz.openbmc_project.ObjectMapper         interface -         -            -
.GetAncestors                            method    sas       a{sa{sas}}   -
.GetObject                               method    sas       a{sas}       -
.GetSubTree                              method    sias      a{sa{sas}}   -
.GetSubTreePaths                         method    sias      as           -
...
```

### GetObject

Use this method to find the services, with their interfaces, that implement
a certain object path.  The output is a map of service names to their
implemented interfaces.  An optional list of interfaces may also be passed in
to constrain the output to services that implement those specific interfaces.

Inputs:
- path: object path
- param: interfaces - an optional list of interfaces to constrain the search to

Output:
- Map of service names to their interfaces

```
busctl call \
xyz.openbmc_project.ObjectMapper \
/xyz/openbmc_project/object_mapper \
xyz.openbmc_project.ObjectMapper \
GetObject sas /xyz/openbmc_project/sensors/voltage/ps1_input_voltage 0

a{sas} 1
"xyz.openbmc_project.Hwmon-1025936882.Hwmon1" 3 "xyz.openbmc_project.Sensor.Threshold.Critical" "xyz.openbmc_project.Sensor.Threshold.Warning" "xyz.openbmc_project.Sensor.Value"
```

#### Example Use Case:
Find the service name that has the desired object path so it can be passed into
a get property call.

### GetSubTree

Use this method to find the objects, services, and interfaces in the specified
subtree that implement a certain interface.  If no interfaces are passed in,
then all objects/services/interfaces in the subtree are returned.  If interfaces
are passed in, then only those interfaces are returned int the output.

Inputs:
- param: subtree - the root of the tree.  Using "/" will search the whole tree
- param: depth - the maximum depth of the tree past the root to search. Use 0 to search all
- param: interfaces - an optional list of interfaces to constrain the search to

Output:
- Map of object paths to a map of service names to their interfaces

```
busctl call \
xyz.openbmc_project.ObjectMapper \
/xyz/openbmc_project/object_mapper \
xyz.openbmc_project.ObjectMapper \
GetSubTree sias "/" 0 1 "xyz.openbmc_project.Sensor.Threshold.Warning"

a{sa{sas}} 18
"/xyz/openbmc_project/sensors/voltage/ps0_input_voltage" 1 "xyz.openbmc_project.Hwmon-1040041051.Hwmon1" 1 "xyz.openbmc_project.Sensor.Threshold.Warning"

"/xyz/openbmc_project/sensors/current/ps1_output_current" 1 "xyz.openbmc_project.Hwmon-1025936882.Hwmon1" 1 "xyz.openbmc_project.Sensor.Threshold.Warning"
...

```

#### Example Use Case
Find all object paths and services that implement a specific interface.

### GetSubTreePaths
This is the same as GetSubTree, but only returns object paths

Inputs:
- param: subtree - the root of the tree.  Using "/" will search the whole tree
- param: depth - the maximum depth of the tree past the root to search. Use 0 to search all
- param: interfaces - an optional list of interfaces to constrain the search to

Output:
- array of object paths in that subtree

```
busctl call \
xyz.openbmc_project.ObjectMapper \
/xyz/openbmc_project/object_mapper \
xyz.openbmc_project.ObjectMapper \
GetSubTreePaths sias "/" 0 1 "xyz.openbmc_project.Sensor.Threshold.Warning"

as 18
"/xyz/openbmc_project/sensors/voltage/ps0_input_voltage"

"/xyz/openbmc_project/sensors/temperature/ambient"

"/xyz/openbmc_project/sensors/voltage/ps1_output_voltage"
...
```

#### Example Use Case
Find all object paths that implement a specific interface.

### GetAncestors
Use this method to find all ancestors of an object that implement a specific
interface.  If no interfaces are passed in, then all ancestor
paths/services/interfaces are returned.

Inputs:
- param: path - the object path to find the ancestors of
- param: interfaces - an optional list of interfaces to constrain the search to

Output:
- A map of object paths to a map of services names to their interfaces

```

busctl call \
xyz.openbmc_project.ObjectMapper \
/xyz/openbmc_project/object_mapper \
xyz.openbmc_project.ObjectMapper \
GetAncestors sas "/xyz/openbmc_project/inventory/system" 0

a{sa{sas}} 3
"/xyz/openbmc_project" 1 "xyz.openbmc_project.ObjectMapper" 1 "org.freedesktop.DBus.ObjectManager"

"/xyz/openbmc_project/inventory" 1 "xyz.openbmc_project.Inventory.Manager" 2 "xyz.openbmc_project.Inventory.Manager" "org.freedesktop.DBus.ObjectManager"

"/" 1 "xyz.openbmc_project.Settings" 1 "org.freedesktop.DBus.ObjectManager"
```

#### Example Use Case
Find a parent object that implements a specific interface.

## Associations

Associations are special D-Bus objects created by the mapper to associate two
objects with each other.  For this to occur, some application must implement
the `org.openbmc.Associations` interface, and then when an association is
desired, the `associations` property on that interface needs to be written.

This associations property is an array of tuples of the form:
```
[forward, reverse, object path]
```
- forward: this is the name of the forward association object
- reverse: this is the name of the reverse assocation object
- object path: this is the other object to associate with

When an object with, for example, an object path of `pathA` uses
the following values:
```
["foo", "bar", "pathB"]
```
The mapper will create 2 new objects:

1. `pathA/foo`
2. `pathB/bar`

On each of these objects, the interface `xyz.openbmc_project.Association` will
be implemented, which has a single `endpoints` property.  This property is
an array that holds the object paths to the other end of the association.

So, `pathA/foo->endpoints` will contain `pathB`, and `pathB/bar->endpoints`
will contain `pathA`.

If another object, say `pathC`, also has an association to `pathB`, then a
second entry, `pathC`, will be added into `pathB`\'s endpoints property.

These new objects will match the lifetime of the associated objects.
For example, if `pathA` is deleted, then `pathA/foo` will also be deleted,
and `pathA` will be removed from the endpoints property of `pathB/bar`.  If
that was the last entry in that property, then `pathB/bar` will also be deleted.

#### Example Use Case

Associate an error log with the inventory item that caused it.

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
