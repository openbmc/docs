# Redfish Client Design

Author: Xinyuan Wang: <wang.x.6f@gmail.com>

Other contributors: Patrick Williams: <patrick@stwcx.xyz>

Created: Nov 04, 2024

## Problem Description

Certain Satellite Management Controllers (SMC), such as Nvidia's HMC, can only
be communicated using the Redfish protocol. Since currently OpenBMC lacks the
capability of talking Redfish as a client, BMC applications have no access to
resources that are controlled by such SMC (Even though there is Redfish
aggregation support in bmcweb, it is still not sufficient. See alternatives
considered session for detailed rationale). In some cases, having BMC
applications being able to access resources from such SMC is a must. For
example, thermal calibration needs sensor readings from the SMC, and bmcweb
needs event log from the SMC to fulfill event subscription because otherwise the
event logs being pushed to the subscriber would be an incomplete collection.

## Background and References

- [Redfish APIs Support — NVIDIA DGX H100/H200](https://docs.nvidia.com/dgx/dgxh100-user-guide/redfish-api-supp.html)
- [Satellite Management Controller (SMC) Specification](https://www.opencompute.org/documents/smc-specification-1-0-final-pdf-1)

## Requirements

BMC applications are able to perform read and write operations towards resources
controlled by SMC that can only be accessed via Redfish. The operations include
but not limited to:

- Sensor monitoring
- Event log collection
- Hardware and firmware inventory discovery
- Firmware upgrade
- Crash-dump retrieval

For each individual operation, the following aspect should be configurable by
config files and can be system specific:

- Target SMC to communicate with
- Target URL for Redfish requests
- Mapping of Redfish response

BMC applications should require no or minimum change to be able to access the
above-mentioned resources since the Redfish client will map the resources to
existing DBus interfaces.

## Proposed Design

### Redfish Client Daemon

This design proposes to develop a Redfish client daemon that will start at BMC
boot time. The daemon then will load all configurations and use compatible ones
to either create a task for read operations or expose proper D-Bus interfaces
and methods for write operations.

The read task will periodically send Redfish requests to the SMC to read
resources, and then map the responses to proper D-Bus objects which can be
inspected by other BMC applications. Each task would maintain an internal state
so that the difference between two reads from SMC can be calculated and be
reflected to the D-Bus objects. Some example as the following:

| Purpose              | Redfish Resource                         | D-Bus path namespace           |
| -------------------- | ---------------------------------------- | ------------------------------ |
| Sensor Monitoring    | Collection of [Sensor][Sensor]           | /xyz/openbmc_project/sensors/  |
| Event Log Collection | Collection of [Event][Event]             | /xyz/openbmc_project/logging/  |
| Firmware Inventory   | [Software Inventory][Software Inventory] | /xyz/openbmc_project/software/ |

[Sensor]:
  https://www.dmtf.org/sites/default/files/standards/documents/DSP0268_2024.3.html#sensor-1101
[Event]:
  https://www.dmtf.org/sites/default/files/standards/documents/DSP0268_2024.3.html#event-1110
[Software Inventory]:
  https://www.dmtf.org/sites/default/files/standards/documents/DSP0268_2024.3.html#softwareinventory-1102

For write operations, the daemon should expose D-Bus interfaces and methods that
conform with the expectation of the BMC application that handles the operation.
For example,
[xyz.openbmc_project.Software.Update](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Software/Update.interface.yaml)
interface would be exposed for other BMC applications to trigger the firmware
upgrade, and the actual implementation would conform with design for
[code update](https://github.com/openbmc/docs/blob/master/designs/code-update.md).

All the configuration files should reside within one directory that are close to
the Redfish client source code. For each configuration, the following trait
should be able to be specified if applicable:

- The compatible component/device, using
  [Compatible](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Inventory/Decorator/Compatible.interface.yaml)
  D-Bus interface
- The target SMC, which should refer to a
  [SatelliteController](https://github.com/openbmc/entity-manager/blob/e25a3890392093fec8f49b84c5bbce99055c6768/schemas/satellite_controller.json)
  exposed by an entity in entity manager configuration
- The target Redfish URL
- The cadence of sending Redfish requests for read task
- The mapping from Redfish responses to D-Bus objects, such as sensor name
  mapping and message id mapping

### Redfish Library

This design also proposes to develop a Redfish library that can do codegen from
Redfish schemas to C++ classes. Every Redfish schema will have one corresponding
C++ class generated with the following feature supported:

- An instance of that class can be constructed from a JSON object (string) that
  conforms to that Redfish schema
- An instance of that class can be converted to a JSON object (string) that
  conforms to that Redfish schema

This library would provide decent statically-typed class abstraction of the
Redfish data model for the Redfish client daemon so that direct validation and
manipulation towards raw json objects can be prevented. On the other hand, the
business logic of how to make Redfish query and how to manipulate those Redfish
objects are out of the scope of this library but reside on the daemon side.

## Alternatives Considered

### Redfish Aggregation

It is possible to leverage redfish aggregation on
[bmcweb](https://github.com/openbmc/bmcweb/blob/master/AGGREGATION.md) to enable
external access of the SMC, but the applications running on the local BMC host
still do not have access to those resources.

This implementation acts similar to a reverse proxy, which is exposing the SMC
directly onto the external network, presenting possible SMC security
vulnerabilities externally. The update cadence for the SMC may be decoupled from
the BMC, so while any vulnerabilities might be fixed rapidly for a BMC, it may
take a much longer time for a fix to be developed and deployed to SMCs.

Additionally, using Redfish aggregation (in its current implementation) puts
constraints on external Redfish clients:

- Many properties with Redfish URIs have SMC-relative paths and are not fixed up
  by bmcweb in the response.
- Redfish paths, identifiers, and message-ids are specified by the SMC vendor,
  which may be different from the BMC vendor and may follow different
  conventions. This in turn causes management systems to need to be “SMC-aware”
  for particular server designs, requiring unique development and limiting
  system management convergence.
- Some Redfish features, such as the “expand” keyword, do not traverse the
  aggregated endpoints.
- The Redfish schema versions may differ between the BMC and SMC endpoints,
  which is additional management complexity.

### Implement Redfish client within corresponding feature repositories

Rather than having a centralized redfish client daemon that supports multiple
features, an alternative would be to put sensor support into dbus-sensors,
firmware update support into phosphor-bmc-code-mgmt, and logging support into
phosphor-logging etc. The problem with this approach is that the Redfish client
implementation would need to be duplicated across multiple repositories,
resulting in increased maintenance efforts and complexity when making horizontal
changes, such as adding new Redfish authentication methods or implementing
security-related patches, because in such case changes would need to be made in
multiple repositories. In addition, [pldm](https://github.com/openbmc/pldm)
utilizes a similar approach where pldmd is the centralized daemon that talks
pldm and supports multiple features like firmware update and bios control.

### Configuration

Rather than having separate configuration files, an alternative could be to
expand the configuration options for the
[SatelliteController](https://github.com/openbmc/entity-manager/blob/e25a3890392093fec8f49b84c5bbce99055c6768/schemas/satellite_controller.json)
to meet the need of Redfish client daemon. The only problem with this approach
is that this would make the configuration always tied to the entity that exposes
the satellite controller, which makes it impossible to tune the configuration
between different machines but contains that same entity.

## Impacts

Despite developers needing to write additional configuration files to enable the
Redfish client feature, no API changes for other BMC applications.

If there is no SMC that the Redfish client daemon needs to talk to, it should
have no impact. If the Redfish client daemon is configured to pull data from the
SMC, it may consume some CPU time of the BMC but should be a reasonable amount.

A new repo will be created within the organization. A new recipe will be added
to OpenBMC. There will be additional documentation within the new repo but this
design would be the starting point.

## Organizational

### Does this repository require a new repository?

Yes, a new repository is required for both the daemon, configuration files and
the library.

### Who will be the initial maintainer(s) of this repository?

Xinyuan Wang and Patrick Williams.

## Testing

### Unit Testing

Having unit tests to validate all of the code paths that will be implemented as
mentioned in the design.

### Integration Testing

Using openbmc-test-automation to run end to end integration tests to validate
the code generated from Redfish client library is as expected and the Redfish
client daemon is able to read configuration files to create corresponding tasks
and D-bus objects.
