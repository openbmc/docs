# Redfish Client Design

Author: Xinyuan Wang: <wang.x.6f@gmail.com>

Other contributors: Patrick Williams: <patrick@stwcx.xyz>

Created: Nov 04, 2024

## Problem Description

Certain Satellite Management Controllers (SMC), such as Nvidia's HMC, utilize
Redfish as its primary communication protocol. Since currently OpenBMC only
provides limited capability of talking Redfish as a client, e.g. bmcweb Redfish
aggregation, BMC applications have no access to resources that are controlled by
such SMC (See "Alternatives Considered" session for detailed rationale on why
bmcweb Redfish aggregation is not sufficient). In some cases, having BMC
applications being able to access resources from such SMC is a must. For
example, thermal calibration needs sensor readings from the SMC.

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
- [Update Service](https://www.dmtf.org/sites/default/files/standards/documents/DSP0268_2024.4.html#updateservice-1150)
- Dump files

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

### Redfish Client Daemon

This design proposes to develop a Redfish client daemon that will start at BMC
boot time. The daemon then will load configuration files and create internal
tasks accordingly. Each task will be responsible for accessing one kind of
resources and the general workflow is as the following:

1. sending Redfish requests to the target SMC periodically or on-trigger
2. transforming the Redfish responses to D-Bus objects with appropriate
   interfaces
3. hosting the transformed D-Bus objects under proper D-Bus path

The daemon will handle the above operations asynchronously by leveraging c++
coroutine. For D-Bus operations,
[sdbusplus library](https://github.com/openbmc/sdbusplus) and its binding tool
will be used. For http requests, async libcurl functions will be used and
integrated with
[sdbusplus /async](https://github.com/openbmc/sdbusplus/tree/master/include/sdbusplus/async)
so that the whole daemon can be written under the same coroutine framework, and
thus the threading model will be following the same rule that the framework
provides.

For each of the configuration file, the following trait should be able to be
specified if applicable:

- Activation condition, which is a set of rules for determining whether the
  configeration should be activated or not.
- The target SMC, which should refer to a
  [SatelliteController](https://github.com/openbmc/entity-manager/blob/e25a3890392093fec8f49b84c5bbce99055c6768/schemas/satellite_controller.json)
  exposed by an entity in entity manager configuration. This enable the daemon
  to figure out the correct address to send Redfish request.
- Feature specific configuration, such as Redfish->D-Bus transformation rules or
  HTTP request parameters

#### Example workflow

Below is an example workflow of retrieving sensors. This example is for
demonstration purpose only, so the actual implementation (including the schema
of the configeration) will likely be different, but the concept remains
unchanged.

The daemon will first load all the configeration files. For each configeration,
it will first check against the activation condition to determine whether it
should be activated or not.

Take the following snippet of configeration file as example:

```json
{
  "SatelliteControllerName": "HMC",
  "ActivationCondition": "xyz.openbmc_project.Inventory.Decorator.Compatible({'Names': ['com.meta.Hardware.BMC.Model.Catalina']})",
  "Sensors": {
    "Transforms": [
      {
        "Match": "Name=HGX_(.+)",
        "ConvertTo": "Name=$1"
      }
    ],
    "IntervalMs": 200,
    "Retries": 3
  },
  ...
}
```

This configeration will be considered activated if there is any entity with
`{'Names': ['com.meta.Hardware.BMC.Model.Catalina']}` presented under
`xyz.openbmc_project.Inventory.Decorator.Compatible` appearing within the
system. Additionally, the daemon will look for the exposed
[SatelliteController](https://github.com/openbmc/entity-manager/blob/e25a3890392093fec8f49b84c5bbce99055c6768/schemas/satellite_controller.json)
with name `HMC`. If found, the `HostName` and `Port` will be used for setting up
HTTP connection later on. If not found, the configeration will flip back to
inactivated.

Once the configeration becomes activated, some internal tasks will be created
accordingly. For example, the `"Sensors"` part of the configeration will trigger
the following task run every `200ms`, configured by `IntervalMs`:

- Send http requests to `HMC` to gather all of the Redfish sensors:
  - Request `/redfish/v1/Chassis` to get a list of chassis
  - For each of the chassis, request
    `/redfish/v1/Chassis/<chassis>/Sensors/?$expand=.($levels=1)`
- Once all of the Redfish sensors are received and parsed, each one of them will
  be transformed to a D-Bus sensor object that inherented from
  `sdbusplus::aserver::xyz::openbmc_project::sensor::Value<>`, which is a
  generated class by `sdbus++`. The created D-Bus sensor object will be kept in
  memory so that in the following iteration no new object will be created for
  the same Redfish sensor but the state of that D-Bus sensor object will be
  updated accordingly.
- The transformation includes general transformation such as from Redfish
  `ReadingUnits` to
  [D-Bus Sensor Unit](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Sensor/Value.interface.yaml#L54)
  and customized transformation specified in configeration. In this case, `HGX_`
  prefix will be removed from the name for every Redfish sensors.

### Redfish Codegen Tool

This design also proposes to develop a Redfish Codegen Tool that can generate
C++ classes from Redfish schemas, which is similar to DMTF
[Redfish-Schema-C-Struct-Generator](https://github.com/DMTF/Redfish-Schema-C-Struct-Generator).
The Tool will take a collection of Redfish Schemas as input and generate c++
classes or type aliases to represent definitions defined in the schema. The
following features should be supported for those generated classes or type
aliases:

- JSON Serializable/Deserializable: An instance of this type can be deserialized
  from a JSON string and, conversely, can also be serialized to a JSON string.
- For properties that conform to the source Redfish schemas, they should be
  accessible in a type-safe manner, eliminating the need for hard-coded strings,
  e.g. `serviceRoot.Chassis()` instead of `serviceRoot["Chassis"]`.
  Additionally, these properties are nullable, so it is essential to perform
  null checks before accessing their underlying values.
- For properties that do not conform to the source Redfish schemas, they will be
  captured by the "leftover" property, which is of raw JSON type. This includes
  unknown keys or unmatched types, which can occur when the Redfish server uses
  a different version of the schema, an OEM schema, or a non-standard schema. By
  doing so, we provide flexibility for callers to handle out-of-spec data
  without throwing errors during deserialization, ensuring that no data is lost.
- Each generated class will contain an "error" property, which captures error
  responses as defined in
  [Redfish spec 8.6](https://www.dmtf.org/sites/default/files/standards/documents/DSP0266_1.21.1.html#error-responses).

The main goal of this tool is to prevent the necessity of hand writing c++
wrapper class/struct for parsing and dumping Redfish json string if the
class/struct is able to generated from the schemas.

Non-goal of this tool:

- Redfish json validator: can be done but not enabled by default
- Redfish URL/Redpath: how to do Redfish query is out of scope of this tool

For implementation, the generated classes will be built on top of the
`nlohmann/json` library for the JSON serializer and deserializer, and the tool
itself will be written in python.

For example, the generated `Sensor` class may exposed the following API:

```c++
// parse from json
Sensor parseSensor(const std::string& json);
// property getter
Property<odata_v4::context>& get_odata_context();
Property<odata_v4::etag>& get_odata_etag();
Property<odata_v4::id>& get_odata_id();
Property<odata_v4::type>& get_odata_type();
Property<double>& getReading();
Property<std::string>& getReadingUnits();
Property<Resource::Name>& getName();
Property<Links>& getLinks();
// ...
// handle error
Property<Error>& error();
// capcture out of spec properties or values
nlohmann::json& leftover();
// dump to json
std::string toJson();
...
```

### Future work

- Firmware update and crashdump retrieve workflow
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

### Direct manipulation of json object

Instead of relying on the Redfish Codegen Tool to generate C++ classes for
Redfish resources, we could have the daemon directly manipulate JSON objects.
However, this approach would result in hard-coded string literals scattered
throughout the codebase, making it difficult to maintain and prone to errors as
the project scales. A single typo in one of these string literals would only be
caught at runtime when the actual Redfish resource is being serialized or
deserialized. In contrast, using proper class abstractions on top of the JSON
objects can prevent such issues and enable compile-time checks, ensuring greater
reliability and maintainability.

### Libredfish

Libredfish is a C library built on top of libcurl, necessitating a C++ wrapper
if we choose to depend on it, as we aim to minimize C code and leverage C++
coroutines for asynchronous operations. However, compared to building a C++
wrapper directly on top of libcurl, there is little benefit in using libredfish
for asynchronous operations, as its threading and queuing model would likely
differ. Another feature offered by libredfish is Redpath, but it consists of
approximately
[200 lines of string parsing code](https://github.com/DMTF/libredfish/blob/main/src/redpath.c),
which can be replicated in modern C++ code easily if needed. Lastly, libredfish
does not appear to be well-maintained or actively developed, with the last
commit being six months ago.

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

Yes, a new repository is required for both the daemon, configuration files and
the codegen tool.

### Who will be the initial maintainer(s) of this repository?

Patrick Williams.

## Testing

### Unit Testing

Having unit tests to validate all of the code paths that will be implemented as
mentioned in the design.

### Integration Testing

Having end to end integration tests to validate the Redfish client daemon is
able to read configuration files and do expected work.
