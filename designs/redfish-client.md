# Redfish Client Design

Author: Xinyuan Wang: <wang.x.6f@gmail.com>

Other contributors: Patrick Williams: <patrick@stwcx.xyz>

Created: Nov 04, 2024

## Problem Description

Certain Satellite Management Controllers (SMC), such as Nvidia's HMC, utilize
Redfish as its primary communication protocol. Since currently OpenBMC only
provides limited capability for talking to Redfish as a client, e.g. bmcweb
Redfish aggregation, BMC applications have no access to resources that are
controlled by such SMC (See "Alternatives Considered" session for detailed
rationale on why bmcweb Redfish aggregation is not sufficient). In some cases,
having BMC applications able to access resources from such SMC is a must. For
example, thermal calibration needs sensor readings from the SMC. Additionally,
some BMC users may enforce specific rules on how resources should be exposed by
the BMC (for example, sensor naming). Therefore, it is essential to have the
capability to apply the same rules to the SMC resources before they are exposed
outside of the BMC.

## Background and References

- What is Satellite Management Controller?
  [Satellite Management Controller (SMC) Specification](https://www.opencompute.org/documents/smc-specification-1-0-final-pdf-1)
- One example of SMC that utilizes Redfish as its primary communication
  protocol:
  [Redfish APIs Support — NVIDIA DGX H100/H200](https://docs.nvidia.com/dgx/dgxh100-user-guide/redfish-api-supp.html)
- There is a prior
  [design proposal](https://gerrit.openbmc.org/c/openbmc/docs/+/58954) for
  having a daemon to read sensor data via Redfish, as outlined in the Redfish
  Sensor Design Proposal. This proposal is similar to the current one, but it
  only supports the sensor monitoring feature. For a more detailed comparison,
  please refer to the "Alternatives Considered" section.
- While it is possible to access SMC resources from outside the BMC using
  [Redfish aggregation supported by bmcweb](https://github.com/openbmc/bmcweb/blob/master/AGGREGATION.md),
  this approach does not provide direct access to these resources for
  applications running inside the BMC. For a more detailed comparison and an
  analysis of the limitations of this approach, please refer to the Alternatives
  Considered section.

## Requirements

BMC applications are able to (directly or indirectly) access the following
resources controlled by SMC that utilizes Redfish as its primary communication
protocol:

- Collection of
  [Sensor](https://www.dmtf.org/sites/default/files/standards/documents/DSP0268_2024.4.html#sensor-1101)
- Collection of
  [LogEntry](https://www.dmtf.org/sites/default/files/standards/documents/DSP0268_2024.4.html#logentry-1170)
- [Software Inventory](https://www.dmtf.org/sites/default/files/standards/documents/DSP0268_2024.4.html#softwareinventory-1102)

Each of the resources should have their corresponding dbus interfaces being
created that are indistinguishable from normal resources, and some of the
transformation can be made configurable.

When making Redfish requests to the SMC, the following features are required:

- HTTP keep-alive to prevent overloading the SMC
- Multiple requests can be sent and received concurrently
- Being compatible with up to one year old from the latest DMTF released Redfish
  schema

The following features are optional unless the target SMC requires:

- Authentication
- HTTPS

The following features are not going to be supported:

- OEM schemas
- Out of spec SMC

## Proposed Design

This design proposes a Redfish client daemon running on the BMC to perform the
required tasks and meet the requirements above. As described above, it is
responsible for:

1. Configuration
2. Resource collection from the target SMC via Redfish
3. Transformation from Redfish to D-Bus representation for collected resources
4. Resource hosting

### Configuration

For configuration, each platform will need to provide their own configuration
files and the format is as the following:

```json
{
  "SatelliteControllerName": "HMC",
  "Compatible": ["com.meta.Hardware.BMC.Model.Catalina"],
  "SensorConfig": {
    "Mappers": [
      {
        "FromId": "HGX_Chassis_0_TotalGPU_Power_0",
        "ToId": "HGX_TOTALGPU_PWR_W"
      },
      {
        "FromId": "ProcessorModule_0_CPU_0_CoreUtil_0",
        "ToId": "HGX_CPU0_CORE0_UTIL_PCT"
      }
    ],
    "IntervalMilliseconds": 2000,
    "MaxRetries": 3,
    "RetryIntervalMilliseconds": 200,
    "MaxExpandLevels": 3
  },
  "LogServiceConfig": {
    "Targets": [
      {
        "SystemId": "HGX_Baseboard_0",
        "LogServiceId": "EventLog"
      },
      {
        "ManagerId": "HGX_BMC_0",
        "LogServiceId": "Journal"
      }
    ],
    "IntervalMilliseconds": 2000,
    "MaxRetries": 2,
    "RetryIntervalMilliseconds": 300
  },
  "UpdateServiceConfig": {
    "FirmwareMappers": [
      {
        "FromId": "HGX_FW_CPU_0",
        "ToId": "HGX_cpu_0"
      }
    ],
    "SoftwareMappers": [
      {
        "FromId": "HGX_Driver_GPU_0",
        "ToId": "HGX_gpu_driver_0"
      }
    ],
    "IntervalMilliseconds": 60000,
    "MaxRetries": 3,
    "RetryIntervalMilliseconds": 2000
  }
}
```

To determine which configuration files to use, the daemon first collects all
names from entities managed by entity-manager that implement the
`xyz.openbmc_project.Inventory.Decorator.Compatible` D-Bus interface. If the
names specified in the "Compatible" field of a configuration file are a subset
of that list, the configuration file will be considered activated and used later
to create tasks accordingly. Therefore, multiple configuration files can become
activated simultaneously if more than one configuration file matches.

For example, the above configuration file will be considered active if the
system has the following entity, as it matches
`["com.meta.Hardware.BMC.Model.Catalina"]`:

```text
NAME                                               TYPE      SIGNATURE RESULT/VALUE                             FLAGS
org.freedesktop.DBus.Introspectable                interface -         -                                        -
.Introspect                                        method    -         s                                        -
org.freedesktop.DBus.Peer                          interface -         -                                        -
.GetMachineId                                      method    -         s                                        -
.Ping                                              method    -         -                                        -
org.freedesktop.DBus.Properties                    interface -         -                                        -
.Get                                               method    ss        v                                        -
.GetAll                                            method    s         a{sv}                                    -
.Set                                               method    ssv       -                                        -
.PropertiesChanged                                 signal    sa{sv}as  -                                        -
xyz.openbmc_project.AddObject                      interface -         -                                        -
.AddObject                                         method    a{sv}     -                                        -
xyz.openbmc_project.Inventory.Decorator.Compatible interface -         -                                        -
.Names                                             property  as        1 "com.meta.Hardware.BMC.Catalina"       emits-change
xyz.openbmc_project.Inventory.Decorator.Revision   interface -         -                                        -
.Version                                           property  s         "MP"                                     emits-change
xyz.openbmc_project.Inventory.Item.Board           interface -         -                                        -
.Name                                              property  s         "Catalina SCM"                           emits-change
.Probe                                             property  s         "xyz.openbmc_project.FruDevice({\'BOARD… emits-change
.Type                                              property  s         "Board"                                  emits-change
```

### Resource collection via Redfish

For each of the activated configuration files, the daemon needs to figure out
the target SMC first and then create one or more tasks to collect resources from
the target SMC based on the configuration.

To figure out the target SMC, the daemon will look for
[SatelliteController](https://github.com/openbmc/entity-manager/blob/e25a3890392093fec8f49b84c5bbce99055c6768/schemas/satellite_controller.json)
with the exact same name specified in the `SatelliteControllerName` field of the
configuration file. If found, the `HostName` and `Port` will be used for setting
up HTTP connection later on. If not found, the configuration will become
deactivated. The daemon will also periodically check for the existence of the
target SMC to dynamically activate/deactivate the configuration.

For example, the above example configuration file will be considered activated
and hostname `0.1.1.1` + port `80` will be used to establish HTTP connection if
the system has the following entity, as it matches `HMC`:

```text
NAME                                                  TYPE      SIGNATURE RESULT/VALUE          FLAGS
org.freedesktop.DBus.Introspectable                   interface -         -                     -
.Introspect                                           method    -         s                     -
org.freedesktop.DBus.Peer                             interface -         -                     -
.GetMachineId                                         method    -         s                     -
.Ping                                                 method    -         -                     -
org.freedesktop.DBus.Properties                       interface -         -                     -
.Get                                                  method    ss        v                     -
.GetAll                                               method    s         a{sv}                 -
.Set                                                  method    ssv       -                     -
.PropertiesChanged                                    signal    sa{sv}as  -                     -
xyz.openbmc_project.Configuration.SatelliteController interface -         -                     -
.AuthType                                             property  s         "None"                emits-change
.Hostname                                             property  s         "0.1.1.1"             emits-change
.Name                                                 property  s         "HMC"                 emits-change
.Port                                                 property  t         80                    emits-change
.Type                                                 property  s         "SatelliteController" emits-change
```

After the target SMC is determined, the daemon will create one or more tasks
based on the configuration. At the time of this design proposal, there are three
types of tasks that can be created:

1. Sensor collection task: This task is configured by `SensorConfig` and will
   periodically send Redfish requests to the target SMC to retrieve all of the
   sensors that are of interest. The frequency of the requests is specified by
   `IntervalMilliseconds`. The maximum number of retries is specified by
   `MaxRetries`. The interval between retries is specified by
   `RetryIntervalMilliseconds`. Depends on the `MaxExpandLevels` specified in
   the configuration, for each cycle, the task will either send a single request
   to retrieve all of the sensors for all of the chassis, i.e.
   `/​redfish/​v1/​Chassis?$expand=.($levels=3)`, or it will send multiple
   requests to retrieve sensors for each chassis individually, i.e.
   `/​redfish/​v1/​Chassis/​{ChassisId}/​Sensors?$expand=.($levels=1)`.

2. Log collection task: This task is configured by `LogServiceConfig` and will
   periodically send Redfish requests to the target SMC to retrieve all of the
   log entries that are of interest. The frequency of the requests is specified
   by `IntervalMilliseconds`. The maximum number of retries is specified by
   `MaxRetries`. The interval between retries is specified by
   `RetryIntervalMilliseconds`. `Targets` specified in the configuration will be
   used to construct the URIs for the requests. For example, the following
   target:

   ```json
   {
     "SystemId": "HGX_Baseboard_0",
     "LogServiceId": "EventLog"
   }
   ```

   will result in sending requests to
   `/redfish/v1/Systems/HGX_Baseboard_0/LogServices/EventLog/Entries`.

3. Software/firmware inventory collection task: This task is configured by
   `UpdateServiceConfig` and will periodically send Redfish requests to the
   target SMC to retrieve all of the software/firmware inventory items that are
   of interest. The frequency of the requests is specified by
   `IntervalMilliseconds`. The maximum number of retries is specified by
   `MaxRetries`. The interval between retries is specified by
   `RetryIntervalMilliseconds`. If `FirmwareMappers` is specified and not empty,
   the task will send requests to
   `/redfish/v1/UpdateService/FirmwareInventory?$expand=.($levels=1)`. If
   `SoftwareMappers` is specified and not empty, the task will send a requests
   to `/redfish/v1/UpdateService/SoftwareInventory?$expand=.($levels=1)`.

### Transformation from Redfish to D-Bus representation

After the resources are collected, each task will need to do transformation from
Redfish to D-Bus representation for them. For sensor collection task, `Mappers`
inside the `SensorConfig` will be used to educate the task on which sensor is of
interest and additional customized transformation rule. Any sensor that does not
find a match will be discarded. For each of the sensor that is of interest, a
D-Bus object with path `/xyz/openbmc_project/sensors/{namespace}/{id}` will be
created.

For example, with the above example configuration, the following sensor from
Redfish:

```json
{
  "@odata.id": "/redfish/v1/Chassis/HGX_Chassis_0/Sensors/HGX_Chassis_0_TotalGPU_Power_0",
  "@odata.type": "#Sensor.v1_7_0.Sensor",
  "Id": "HGX_Chassis_0_TotalGPU_Power_0",
  "Implementation": "Synthesized",
  "Name": "HGX Chassis 0 TotalGPU Power 0",
  "PhysicalContext": "GPU",
  "Reading": 810.718,
  "ReadingType": "Power",
  "ReadingUnits": "W",
  "RelatedItem": [
    {
      "@odata.id": "/redfish/v1/Systems/HGX_Baseboard_0"
    }
  ],
  "Status": {
    "Conditions": [],
    "Health": "OK",
    "HealthRollup": "OK",
    "State": "Enabled"
  }
}
```

will be transformed to a D-Bus object with path
`/xyz/openbmc_project/sensors/power/HGX_TOTALGPU_PWR_W`:

```text
NAME                                        TYPE      SIGNATURE RESULT/VALUE                                  FLAGS
org.freedesktop.DBus.Introspectable         interface -         -                                             -
.Introspect                                 method    -         s                                             -
org.freedesktop.DBus.Peer                   interface -         -                                             -
.GetMachineId                               method    -         s                                             -
.Ping                                       method    -         -                                             -
org.freedesktop.DBus.Properties             interface -         -                                             -
.Get                                        method    ss        v                                             -
.GetAll                                     method    s         a{sv}                                         -
.Set                                        method    ssv       -                                             -
.PropertiesChanged                          signal    sa{sv}as  -                                             -
xyz.openbmc_project.Sensor.Value            interface -         -                                             -
.MaxValue                                   property  d         inf                                           emits-change writable
.MinValue                                   property  d         -inf                                          emits-change writable
.Unit                                       property  s         "xyz.openbmc_project.Sensor.Value.Unit.Watts" emits-change writable
.Value                                      property  d         810.718                                       emits-change writable
```

For software/firmware inventory collection task, it will have a similar
transformation process as the sensor collection task. It will create D-Bus
objects under path `/xyz/openbmc_project/software/` and implement
`xyz.openbmc_project.Software.Version` interface.

For log collection task, instead of creating D-Bus objects, it will use
`lg2::commit()` API provided by phosphor-logging to commit the log entries to
the log service. To prevent duplicated log entries being committed, the task
will keep track of the log entries that have been committed by memorizing their
`Id` in memory. In addition, to prevent duplicated log entries being committed
if the daemon restarts, the task will also persist memorized `Id` to the disk
and load it back when the daemon restarts.

### Resource hosting

For sensor collection and software/firmware inventory collection tasks, these
tasks will host the transformed D-Bus objects and update them in each cycle when
changes are detected.

### Redfish request handling

The daemon will leverage the async library provided by
[sdbusplus](https://github.com/openbmc/sdbusplus) to handle D-Bus operations and
Redfish requests. For D-Bus operations, the library already has good support.
For Redfish requests, libcurl will be used to integrate with the
`sdbusplus/async` library so that the whole daemon can be run under one
`sdbusplus::async::context` and no extra thread modeling needs to be considered
while ensuring concurrency. The integration can be accomplished by leveraging
the [`curl_multi_waitfds`](https://curl.se/libcurl/c/curl_multi_waitfds.html)
API provided by the curl multi interface, and using `sdbusplus::async::fdio` to
wait for the returned file descriptor. Additionally, the curl multi interface
provides connection caching so that each task can reuse connections across
cycles as they are targeting the same url.

For parsing Redfish responses, necessary data classes will be created based on
the Redfish schema and will be integrated with the `nlohmann/json` library for
JSON parsing. In the first iteration, these classes will be written manually and
can later be switched to code generation from the Redfish schema.

### Error handling

The daemon should not crash if any of the tasks fail. Instead, for tasks that
are failing after retries are exhausted, the task should emit an error log and
remove corresponding hosted D-Bus objects. In the next cycle, the task should
continue to try to collect resources from the target SMC, and if successful, the
task will recreate the D-Bus objects and start hosting them again.

### Future work

- Support firmware/software update workflow
- Support dump file retrieve workflow
- Enable Event subscription to the SMC

## Alternatives Considered

### Redfish Aggregation

It is possible to leverage redfish aggregation on
[bmcweb](https://github.com/openbmc/bmcweb/blob/master/AGGREGATION.md) to enable
external access of the SMC, but there are some limitation and development
overhead.

The following features can be fulfilled by a Redfish client, but are challenging
to support with Redfish aggregation:

- Enabling Applications within BMC to Access SMC Resources
  - Thermal Calibration: "phosphor-pid-control" requires sensor values from SMC
    as input for the fan control algorithm.
  - Sensor Threshold Monitoring: Some SMC vendors do not implement sensor
    threshold monitoring; the BMC must obtain sensor values from the SMC to
    perform monitoring and emit event logs accordingly.
- Middle Layer Control for SMC Resource Exposure: A middle layer within the BMC
  controls how SMC resources are exposed outside the BMC, decoupling
  SMC-vendor-flavored Redfish from tooling outside the BMC.
  - Provide a unified flavor of Redfish to ensure that external tooling can
    interact seamlessly with both SMC and BMC implementations, thereby avoiding
    the need for duplicated development efforts and reducing overall development
    costs.
  - Change Management: Ensures that changes in the SMC do not require every
    single downstream tool to change accordingly. For example:
    - Suppose there is a sensor named X controlled by SMC and multiple
      downstream tools depend on it to function correctly.
    - At some point, the SMC vendor decides to rename sensor X to Y.
    - If the sensor X is directly exposed as it is, then all of the downstream
      tools need to change because the vendor just changed X to Y.
    - However, if there is a middle layer that has some sort of configuration to
      map sensor X to X*, changing that configuration to Y to X* should be good
      enough to make sure all the downstream tools can still function properly
      with the updated SMC.
  - Some resources or data generated by the SMC may not be intended for external
    exposure outside of the BMC.
    - For example, not exposing Redfish OEM commands helps prevent external
      tools from depending on them, thereby avoiding compatibility issues that
      may arise since these commands are specific to certain systems or firmware
      versions.

The following features can be fulfilled by a Redfish client and could
potentially be fulfilled by Redfish Aggregation as well, but with additional
development work:

- Redfish URIs generated by the SMC need to be accessible outside of the BMC.
  - There is existing logic within bmcweb's Redfish Aggregation to adjust the
    URIs returned by the SMC. However, this logic may need to be extended to
    handle cases where "messageArgs" could contain URIs.
- Event Service Support: Events emitted by the SMC should be pushed to the event
  subscriber if there is any at the time of generation.
- Multi-SMC Support

Implementing the above features using Redfish Aggregation would require
significant development effort. In contrast, a Redfish client can provide these
capabilities out-of-the-box.

There is also a security concern associated with utilizing Redfish aggregation,
which involves exposing the SMC directly to the external network and potentially
presenting SMC security vulnerabilities externally. The update cadence for the
SMC may be decoupled from that of the BMC, so while any vulnerabilities might be
fixed rapidly for a BMC, it may take a much longer time for a fix to be
developed and deployed to SMCs.

### Implement Redfish client within corresponding feature repositories

Rather than having a redfish client daemon that supports multiple features, an
alternative would be to put sensor support into dbus-sensors, firmware update
support into phosphor-bmc-code-mgmt, and logging support into phosphor-logging
etc. The problem with this approach is that the Redfish client implementation
would need to be duplicated across multiple repositories, resulting in increased
maintenance efforts and complexity when making horizontal changes, such as
adding new Redfish authentication methods or implementing security-related
patches, because in such case changes would need to be made in multiple
repositories. In addition, [pldm](https://github.com/openbmc/pldm) utilizes a
similar approach where pldmd is the centralized daemon that talks pldm and
supports multiple features like firmware update and bios control.

### Configuration

Rather than having separate configuration files, an alternative could be to
expand the configuration options for the
[SatelliteController](https://github.com/openbmc/entity-manager/blob/e25a3890392093fec8f49b84c5bbce99055c6768/schemas/satellite_controller.json)
to meet the need of Redfish client daemon. The only problem with this approach
is that this would make the configuration always tied to the entity that exposes
the satellite controller, which makes it impossible to tune the configuration
between different machines but contains that same entity.

### Libredfish

Libredfish is a C library built on top of libcurl, necessitating a C++ wrapper
if we choose to depend on it, as we aim to minimize C code and leverage C++
coroutines for asynchronous operations. However, compared to building a C++
wrapper directly on top of libcurl, there is little benefit in using libredfish
for asynchronous operations, as its threading and queuing model would likely
differ. Another feature offered by libredfish is Redpath, but it consists of
approximately
[200 lines of string parsing code](https://github.com/DMTF/libredfish/blob/main/src/redpath.c),
which can be replicated in modern C++ code easily if needed.

### Redfish-Schema-C-Struct-Generator

This tool generates pure C structs that contain raw pointers and char\*, which
is not a favorable practice in modern C++ development, as it can lead to errors
and make the code more prone to memory leaks. Furthermore, this tool does not
appear to be well-maintained or actively developed, with the last commit being
six months ago, and its compatible Redfish schema dating back to
`DSP8010_Schema_2020.4`.

### Boost.Beast

Boost.Beast is an alternative HTTP library that the daemon can utilize. However,
it is built on top of Boost.Asio, which has compatibility issues with the
sdbusplus/async library. This incompatibility may cause problems when the daemon
uses Boost.Asio for HTTP operations while simultaneously using sdbusplus for
dbus operations.

## Impacts

Despite developers needing to write additional configuration files to enable the
Redfish client feature, no API changes for other BMC applications.

If there is no SMC that the Redfish client daemon needs to talk to, it should
have minimal impact. If the Redfish client daemon is configured to pull data
from the SMC, it may consume some CPU time of the BMC but should be a reasonable
amount.

A new repo will be created within the organization. A new recipe will be added
to OpenBMC. There will be additional documentation within the new repo but this
design would be the starting point.

## Organizational

### Does this design require a new repository?

Yes, a new repository is required for both the daemon and the configuration
files.

### Who will be the initial maintainer(s) of this repository?

Patrick Williams.

## Testing

### Unit Testing

Unit tests will validate all of the code paths that will be implemented as
mentioned in the design..

### Integration Testing

End-to-end integration tests will validate that the Redfish client daemon can
read configuration files and perform the expected work.
