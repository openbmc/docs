# Namespaces in Phosphor Logging

Author: Shanjit Jajmann (shanjitsingh@gmail.com, liuc), Kun Zhao
(zkxz@hotmail.com, zkxz)

Primary assignee: Shanjit Jajmann (liuc)

Other contributors: Kun Zhao (zkxz), Deepak Kodahilli (dkodihal)

Created: Dec 19, 2021

## Problem Description
Currently phosphor-log-manager doesn't support any kind of binning of openbmc
event logs which it creates. This binning of a specific type of event logs can
be used to implement distinct properties for each bin. These properties can be
persistent storage location, Error and Information Capacity etc.

This idea of namespaces (binning) in the phosphor-log-manager is being
introduced in this proposal.

## Background and References
The idea of namespaces in logging isn't a new one. Implementing namespaces in
loggers creates a flexibility so that logs of specific type can be binned
together and can follow certain common bin properties.

Currently phosphor-log-manager configurations are managed via a header file
inside `config/config.h.meson`. All the logs are logged under a single
persistent location with global settings for all of the logs i.e. Error Capacity
& Error Info Capacity.

This proposal is to enable phosphor-logging to,
1. Store specific event logs to a special destinations.
2. Provide flexibility wrt certain logging properties to specific event logs.

As a use case in phosphor-log-manager, the namespaces could be either device
names or custom application names. Specific event logs pertaining to a device or
an application can be stored in specific persistent destinations. Event logging
from a certain device or application can be customized to specify varying error
and info capacity.

The segmentation of event logs to apply different capacity limits can help in
optimizing storage for namespaces (e.g. devices/applications) which generate
event logs than those which don't. E.g. Say there are two custom applications
'X' and 'Y' which use phosphor logging for their event logging. After the system
runs for a while, suppose its found that application 'X' generates a lot less
event logs compared to application 'Y' - then storage space can be optimally
adjusted so that application 'Y' is able to have higher log capacity than X.
This would give application developers knobs for properties of their namespaces.

With this proposal implemented, phosphor-log-manager will have the ability to
consume a user defined file which contains namespace names and properties (per
namespace) like Error Capacity, Error Info Capacity and Persistent Location,
ability to toggle namespace on/off. The user defined file is going to be in a
YAML format.

## Requirements

1. *User defined location for persistent log collection per namespace*

   Currently all OpenBMC event logs are persisted as files under the location
   (`/var/lib/phosphor-logging/errors/`) specified in `config/config.h.meson`.

   This requirement introduces flexibility to specify persistant storage
   location per namespace.

2. *User defined error and info capacity per namespace*

    Similar as 1 above, currently global error and info capacity configurations
    are specified in `config/config.h.meson`.

    This requirement will introduce error and info capacity configurations per
    namespace.

3. *D-Bus Path for Event Logs*

   Currently as part of event logging phosphor-log-manager creates OpenBMC event
   logs. These logs are a collection of D-Bus interfaces and are currently
   created under,
    > `/xyz/openbmc_project/logging/entry/X`, where X starts at 1 and is
    > incremented for each new log

    The object tree with two entries right now looks like,
    ```
    `-/xyz
    `-/xyz/openbmc_project
    `-/xyz/openbmc_project/logging
      |-/xyz/openbmc_project/logging/entry
      | |-/xyz/openbmc_project/logging/entry/1
      | `-/xyz/openbmc_project/logging/entry/2
      `-/xyz/openbmc_project/logging/internal
        `-/xyz/openbmc_project/logging/internal/manager
    ```

    We want to create similar object tree under respective namespaces with D-Bus
    paths of event logs.

4. *Support original behavior of root path `/xyz/openbmc_project/logging`*

    To not break existing applications, we have to support original behavior of
    all methods of `/xyz/openbmc_project/logging` as if namespaces don't exist
    at all.

5. *Backward compatibility*

    Implementation must ensure that there’s backward compatibility with the
    current design.

## Proposed Design
We are proposing creation of a yaml file to be consumed by phosphor-log-manager.
This yaml will contain the various user-defined namespaces and relevant
properties for those namespaces.

The yaml file would look like,
```
config:
    deletePolicy: all
namespaces:
    gpu0:
        enabled: 1
        error_capacity: 20
        info_error_capacity: 10
        persistent_location: /var/lib/phosphor-logging/errors/gpu0
    gpu1:
        enabled: 1
        error_capacity: 25
        info_error_capacity: 15
        persistent_location: /var/lib/phosphor-logging/errors/gpu1
    gpu2:
        enabled: 1
        error_capacity: 30
        info_error_capacity: 20
        persistent_location: /var/lib/phosphor-logging/errors/gpu2
...
...
```
More about the config section ahead.

The design will add new root paths similar to `/xyz/openbmc_project/logging` for
each namespace created.

E.g. D-Bus object tree would look something like, (Note: gpu0 & gpu1 are device
names used as namespaces in the example below and there is 1 event log entry
each for the default namespace and gpu0, gpu1)

```
`-/xyz
    `-/xyz/openbmc_project
    `-/xyz/openbmc_project/logging
    |-/xyz/openbmc_project/logging/entry
    | |-/xyz/openbmc_project/logging/entry/1
    `-/xyz/openbmc_project/logging/internal
        `-/xyz/openbmc_project/logging/internal/manager
    |-/xyz/openbmc_project/logging/gpu0
    | |-/xyz/openbmc_project/logging/gpu0/entry
    | | `-/xyz/openbmc_project/logging/gpu0/entry/1
    | `-/xyz/openbmc_project/logging/gpu0/internal
    |   `-/xyz/openbmc_project/logging/gpu0/internal/manager
    `-/xyz/openbmc_project/logging/gpu1
        |-/xyz/openbmc_project/logging/gpu1/entry
        | `-/xyz/openbmc_project/logging/gpu1/entry/1
        `-/xyz/openbmc_project/logging/gpu1/internal
        `-/xyz/openbmc_project/logging/gpu1/internal/manager
```
Both `/xyz/openbmc_project/logging/gpu1` and `/xyz/openbmc_project/logging/gpu1`
are new root paths added for the `gpu0` and `gpu1` namespaces respectively. Both
paths will support addition of D-Bus objects. The number of root paths created
will depend on the contents of the yaml file.


Another thing the design needs to account for is the behavior of
`/xyz/openbmc_project/logging` to not break existing applications. To ensure
this all methods below need to be supported as is even with introduction of
namespaces.

e.g. The `Create` method should create an entry under the default D-Bus object
path `/xyz/openbmc_project/logging` and persist file under the location defined
in `config/config.h.meson`.

```
root@system:~# busctl introspect xyz.openbmc_project.Logging /xyz/openbmc_project/logging
NAME                                     TYPE      SIGNATURE      RESULT/VALUE  FLAGS
org.freedesktop.DBus.Introspectable      interface -              -             -
.Introspect                              method    -              s             -
org.freedesktop.DBus.ObjectManager       interface -              -             -
.GetManagedObjects                       method    -              a{oa{sa{sv}}} -
.InterfacesAdded                         signal    oa{sa{sv}}     -             -
.InterfacesRemoved                       signal    oas            -             -
org.freedesktop.DBus.Peer                interface -              -             -
.GetMachineId                            method    -              s             -
.Ping                                    method    -              -             -
org.freedesktop.DBus.Properties          interface -              -             -
.Get                                     method    ss             v             -
.GetAll                                  method    s              a{sv}         -
.Set                                     method    ssv            -             -
.PropertiesChanged                       signal    sa{sv}as       -             -
xyz.openbmc_project.Collection.DeleteAll interface -              -             -
.DeleteAll                               method    -              -             -
xyz.openbmc_project.Logging.Create       interface -              -             -
.Create                                  method    ssa{ss}        -             -
.CreateWithFFDCFiles                     method    ssa{ss}a(syyh) -             -
```

Thinking similarly about the methods above keeping the `Create` method in mind.

1. Firstly the `DeleteAll` method functionality needs to be looked into.
   Currently, `DeleteAll` method will delete all the event logs under the path
   `/xyz/openbmc_project/logging`.

    To account for this and to give the user flexibility if they want to delete
    all event logs under the default namespace or if they want to delete all
    event logs under all the namespaces completely, we propose to introduce a
    new `DeletePolicy` property for the
    `xyz.openbmc_project.Collection.DeleteAll` interface under
    `/xyz/openbmc_project/logging`.

    ```
    xyz.openbmc_project.Collection.DeleteAll interface -              -             -
    .DeleteAll                               method    -              -             -
    .DeletePolicy                            Property  s              -             -
    ```

    The `DeletePolicy` property will control the behavior of calling the
    `DeleteAll` method. The property will be set via the yaml file. There are
    two possible strings for the DeletePolicy: `'all'` and `'namespace'`. By
    default, DeletePolicy will be set to `all` to delete all the objects for all
    namespaces beneath the root object (this behavior is in line with current
    call of `DeleteAll`, it deletes all managed objects beneath the root path).
    The other option (`namespace`) which `DeletePolicy` will provide is to be
    able to delete objects for the default namespace only.

    Proposed `DeleteAll` behavior for default and custom namespaces with varying
    `DeletePolicy` property,

    |   | DeletePolicy = `'all'`    | DeletePolicy = `'namespace'`     |   |   |
    |---    |:---:  |:---:  |---    |---    |
    | Default Namespace     | Deletes all log entry objects in default namespace and all other namespaces as well (Current behaviour of calling DeleteAll on the default namespace)     | Deletes all log entry objects in the default namespace only (log entry objects for other namespaces untouched)    |   |   |
    | Custom Namespace  | Deletes all log entry objects in the specific namespace only (Deletes the persistent storage directory for this namespace)    | Deletes all log entry objects in the specific namespace only (Deletes the persistent storage directory for this namespace)    |   |   |


2. Secondly, calling the `GetManagedObjects` method under the
   `org.freedesktop.DBus.ObjectManager` interface should return all the event
   log entries under all the namespaces. The `bmcweb` is an example of an
   application which uses the `GetManagedObjects` method to populate the log
   entries. Unless all the log entries for all namespaces are covered, there
   will be disconnect in the log entries created vs log entries shown.

From the code perspective, during initialization the internal manager shall have
an additional data structure to keep track of per namespace and its properties.
These properties will then be consumed by the internal manager itself. The
design will make sure default behavior persists if the yaml file doesn't exist
or is invalid.

## Alternatives Considered
Another way the idea of namespaces can be implemented in phosphor-log-manager is
by building upon event log extensions. The drawback of that approach is
that it isn't memory efficient since multiple copies of the log are made.

## Impacts
This feature would need to be documented in the readme file for the
phosphor-logging repo to guide future developers for using this feature.

API Impact: No API impact. But a new `DeletePolicy` property is introduced in
the `xyz.openbmc_project.Collection.DeleteAll` interface.

## Testing
Google mock and Google test frameworks to be used to develop unit tests.