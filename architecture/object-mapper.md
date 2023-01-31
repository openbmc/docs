# The ObjectMapper

The `xyz.openbmc_project.ObjectMapper` service, commonly referred to as just the
mapper, is an OpenBMC application that attempts to ease the pain of using D-Bus
by providing APIs that help in discovering and associating other D-Bus objects.

The mapper has two major pieces of functionality:

- [Methods](#methods) - Provides D-Bus discovery related functionality.
- [Associations](#associations) - Associates two different objects with each
  other.

## Methods

The official YAML interface definition can be found [here][1].

### GetObject

Use this method to find the services, with their interfaces, that implement a
certain object path. The output is a map of service names to their implemented
interfaces. An optional list of interfaces may also be passed in to constrain
the output to services that implement those specific interfaces.

Inputs:

- path: object path
- param: interfaces - an optional list of interfaces to constrain the search to

Output:

- Map of service names to their interfaces

```
dbus-send --system --print-reply \
--dest=xyz.openbmc_project.ObjectMapper \
/xyz/openbmc_project/object_mapper \
xyz.openbmc_project.ObjectMapper.GetObject \
string:"/xyz/openbmc_project/sensors/voltage/ps1_input_voltage" array:string:

   array [
      dict entry(
         string "xyz.openbmc_project.Hwmon-1025936882.Hwmon1"
         array [
            string "xyz.openbmc_project.Sensor.Threshold.Critical"
            string "xyz.openbmc_project.Sensor.Threshold.Warning"
            string "xyz.openbmc_project.Sensor.Value"
         ]
      )
   ]
```

#### Example Use Case

Find the service name that has the desired object path so it can be passed into
a get property call.

### GetSubTree

Use this method to find the objects, services, and interfaces in the specified
subtree that implement a certain interface. If no interfaces are passed in, then
all objects/services/interfaces in the subtree are returned. If interfaces are
passed in, then only those interfaces are returned in the output.

Inputs:

- param: subtree - the root of the tree. Using "/" will search the whole tree
- param: depth - the maximum depth of the tree past the root to search. Use 0 to
  search all
- param: interfaces - an optional list of interfaces to constrain the search to

Output:

- Map of object paths to a map of service names to their interfaces

```
dbus-send --system --print-reply \
--dest=xyz.openbmc_project.ObjectMapper \
/xyz/openbmc_project/object_mapper \
xyz.openbmc_project.ObjectMapper.GetSubTree \
string:"/" int32:0 array:string:"xyz.openbmc_project.Sensor.Threshold.Warning"

   array [
      dict entry(
         string "/xyz/openbmc_project/sensors/current/ps0_output_current"
         array [
            dict entry(
               string "xyz.openbmc_project.Hwmon-1040041051.Hwmon1"
               array [
                  string "xyz.openbmc_project.Sensor.Threshold.Critical"
                  string "xyz.openbmc_project.Sensor.Threshold.Warning"
                  string "xyz.openbmc_project.Sensor.Value"
               ]
            )
         ]
      )
      dict entry(
         string "/xyz/openbmc_project/sensors/current/ps1_output_current"
         array [
            dict entry(
               string "xyz.openbmc_project.Hwmon-1025936882.Hwmon1"
               array [
                  string "xyz.openbmc_project.Sensor.Threshold.Critical"
                  string "xyz.openbmc_project.Sensor.Threshold.Warning"
                  string "xyz.openbmc_project.Sensor.Value"
               ]
            )
         ]
      )
...

```

#### Example Use Case

Find all object paths and services that implement a specific interface.

### GetAssociatedSubTree

Use this method to find the objects, services, and interfaces in the specified
subtree that implement a certain interface and an endpoint of the inputed
associationPath. If no interfaces are passed in, then all
objects/services/interfaces in the subtree and associated entpoint are returned.
If interfaces are passed in, then only those interfaces are returned in the
output.

Inputs:

- param: associationPath - the path to look for the association endpoints.
- param: subtree - the root of the tree. Using "/" will search the whole tree
- param: depth - the maximum depth of the tree past the root to search. Use 0 to
  search all
- param: interfaces - an optional list of interfaces to constrain the search to

Output:

- Map of object paths to a map of service names to their interfaces that is in
  the associated endpoints

```
dbus-send --system --print-reply \
--dest=xyz.openbmc_project.ObjectMapper \
/xyz/openbmc_project/object_mapper \
xyz.openbmc_project.ObjectMapper.GetSubTree \
objpath:"/"
objpath:"/" int32:0 array:string:"xyz.openbmc_project.Sensor.Threshold.Warning"

   array [
      dict entry(
         string "/xyz/openbmc_project/sensors/current/ps0_output_current"
         array [
            dict entry(
               string "xyz.openbmc_project.Hwmon-1040041051.Hwmon1"
               array [
                  string "xyz.openbmc_project.Sensor.Threshold.Critical"
                  string "xyz.openbmc_project.Sensor.Threshold.Warning"
                  string "xyz.openbmc_project.Sensor.Value"
               ]
            )
         ]
      )
      dict entry(
         string "/xyz/openbmc_project/sensors/current/ps1_output_current"
         array [
            dict entry(
               string "xyz.openbmc_project.Hwmon-1025936882.Hwmon1"
               array [
                  string "xyz.openbmc_project.Sensor.Threshold.Critical"
                  string "xyz.openbmc_project.Sensor.Threshold.Warning"
                  string "xyz.openbmc_project.Sensor.Value"
               ]
            )
         ]
      )
...


# All output host be in the association endpoints
busctl get-property  xyz.openbmc_project.ObjectMapper \
   /associated_endpoints \
  xyz.openbmc_project.Association endpoints
as N "/xyz/openbmc_project/sensors/current/ps0_output_current" \
  "/xyz/openbmc_project/sensors/current/ps1_output_current" \
  ...
```

#### Example Use Case

Find all object paths and services that implement a specific interface.

### GetSubTreePaths

This is the same as GetSubTree, but only returns object paths

Inputs:

- param: subtree - the root of the tree. Using "/" will search the whole tree
- param: depth - the maximum depth of the tree past the root to search. Use 0 to
  search all
- param: interfaces - an optional list of interfaces to constrain the search to

Output:

- array of object paths in that subtree

```
dbus-send --system --print-reply \
--dest=xyz.openbmc_project.ObjectMapper \
/xyz/openbmc_project/object_mapper \
xyz.openbmc_project.ObjectMapper.GetSubTreePaths \
string:"/associated_endpoints" int32:0 array:string:"xyz.openbmc_project.Sensor.Threshold.Warning"

   array [
      string "/xyz/openbmc_project/sensors/current/ps0_output_current"
      string "/xyz/openbmc_project/sensors/current/ps1_output_current"
      string "/xyz/openbmc_project/sensors/power/ps0_input_power"
...
   ]
```

#### Example Use Case

Find all object paths that implement a specific interface.

### GetAssociatedSubTreePaths

This is the same as GetAssociatedSubTreePaths, but only returns object paths

Inputs:

- param: associationPath - the path to look for the association endpoints.
- param: subtree - the root of the tree. Using "/" will search the whole tree
- param: depth - the maximum depth of the tree past the root to search. Use 0 to
  search all
- param: interfaces - an optional list of interfaces to constrain the search to

Output:

- array of object paths in that subtree that is in the associated endpoints

```
dbus-send --system --print-reply \
--dest=xyz.openbmc_project.ObjectMapper \
/xyz/openbmc_project/object_mapper \
xyz.openbmc_project.ObjectMapper.GetAssociatedSubTreePaths \
objpath:"/associated_endpoints" objpath:"/" int32:0 array:string:"xyz.openbmc_project.Sensor.Threshold.Warning"

   array [
      string "/xyz/openbmc_project/sensors/current/ps0_output_current"
      string "/xyz/openbmc_project/sensors/current/ps1_output_current"
      string "/xyz/openbmc_project/sensors/power/ps0_input_power"
...
   ]

# All output host be in the association endpoints
busctl get-property  xyz.openbmc_project.ObjectMapper \
   /associated_endpoints \
  xyz.openbmc_project.Association endpoints
as N "/xyz/openbmc_project/sensors/current/ps0_output_current" \
  "/xyz/openbmc_project/sensors/current/ps1_output_current" \
  "/xyz/openbmc_project/sensors/power/ps0_input_power" \
  ...
```

#### Example Use Case

Find all object paths that implement a specific interface.

### GetAncestors

Use this method to find all ancestors of an object that implement a specific
interface. If no interfaces are passed in, then all ancestor
paths/services/interfaces are returned.

Inputs:

- param: path - the object path to find the ancestors of
- param: interfaces - an optional list of interfaces to constrain the search to

Output:

- A map of object paths to a map of services names to their interfaces

```

dbus-send --system --print-reply \
--dest=xyz.openbmc_project.ObjectMapper \
/xyz/openbmc_project/object_mapper \
xyz.openbmc_project.ObjectMapper.GetAncestors \
string:"/xyz/openbmc_project/inventory/system" array:string:

   array [
      dict entry(
         string "/xyz/openbmc_project"
         array [
            dict entry(
               string "xyz.openbmc_project.ObjectMapper"
               array [
                  string "org.freedesktop.DBus.ObjectManager"
               ]
            )
         ]
      )
      dict entry(
         string "/xyz/openbmc_project/inventory"
         array [
            dict entry(
               string "xyz.openbmc_project.Inventory.Manager"
               array [
                  string "xyz.openbmc_project.Inventory.Manager"
                  string "org.freedesktop.DBus.ObjectManager"
               ]
            )
         ]
      )
      dict entry(
         string "/"
         array [
            dict entry(
               string "xyz.openbmc_project.Settings"
               array [
                  string "org.freedesktop.DBus.ObjectManager"
               ]
            )
         ]
      )
   ]
```

#### Example Use Case

Find a parent object that implements a specific interface.

## Associations

Associations are special D-Bus objects created by the mapper to associate two
objects with each other. For this to occur, some application must implement the
`xyz.openbmc_project.Association.Definitions` interface, and then when an
association is desired, the `Associations` property on that interface needs to
be written.

This `Associations` property is an array of tuples of the form:

```
[forward, reverse, object path]
```

- forward: this is the name of the forward association object
- reverse: this is the name of the reverse association object
- object path: this is the other object to associate with

When an object with, for example, an object path of `pathA` uses the following
values:

```
["foo", "bar", "pathB"]
```

The mapper will create 2 new objects:

1. `pathA/foo`
2. `pathB/bar`

On each of these objects, the interface `xyz.openbmc_project.Association` will
be implemented, which has a single `endpoints` property. This property is an
array that holds the object paths to the other end of the association.

So, `pathA/foo->endpoints` will contain `pathB`, and `pathB/bar->endpoints` will
contain `pathA`.

If another object, say `pathC`, also has an association to `pathB`, then a
second entry, `pathC`, will be added into `pathB`\'s endpoints property.

These new objects will match the lifetime of the associated objects. For
example, if `pathA` is deleted, then `pathA/foo` will also be deleted, and
`pathA` will be removed from the endpoints property of `pathB/bar`. If that was
the last entry in that property, then `pathB/bar` will also be deleted. In
addition, if the endpoint path is removed from D-Bus, in this case `pathB`, then
the mapper will remove the 2 association paths until `pathB` shows back up
again.

Note: The original name of the association definition interface was
`org.openbmc.Associations`. While the mapper still supports this interface as
well for the time being, new code should use the `xyz` version.

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

[1]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/ObjectMapper.interface.yaml
