# PCIe Port and Port Metrics properties

Author: Harshit Aghera (harshit-aghera)

Created: September 9, 2025

## Problem Description

Introduce support for PCIe Port properties (for port type, current speed and
active width) and Port Metrics properties (for PCIe Error Counters) in both
bmcweb and dbus-sensors.

## Background and References

At present, bmcweb supports the
/redfish/v1/Systems/system/PCIeDevices/{PCIeDevice}/ URI. It relies on the
"xyz.openbmc_project.Inventory.Item.PCIeDevice" PDI to obtain PCIe Device
details.

## Requirements

Introduce support in bmcweb to handle multiple PCIe Ports associated with a PCIe
Device.

Expose the following PCIe Port related redfish properties at URI
/redfish/v1/Systems/system/PCIeDevices/{PCIeDevice}/Ports/{PCIePort}/ [1].

- PortProtocol
- PortType
- CurrentSpeedGbps
- ActiveWidth

Expose the following PCIe Port Metrics related redfish properties at URI
/redfish/v1/Systems/system/PCIeDevices/{PCIeDevice}/Ports/{PCIePort}/Metrics/
[2].

- PCIeErrors.CorrectableErrorCount
- PCIeErrors.NonFatalErrorCount
- PCIeErrors.FatalErrorCount
- PCIeErrors.L0ToRecoveryCount
- PCIeErrors.ReplayCount
- PCIeErrors.ReplayRolloverCount
- PCIeErrors.NAKSentCount
- PCIeErrors.NAKReceivedCount
- PCIeErrors.UnsupportedRequestCount

## Proposed Design

The DBus Object Path for the PCIe Port properties will be
/xyz/openbmc_project/system/PCIeDevices/{PCIeDevice}/Ports/{PCIePort}/. The DBus
Object paths of all PCIe Ports of a PCIe Device will be associated with the PCIe
Device DBus Object Path and will be discoverable using object mapper.

Introduce a new phosphor-dbus-interface
"xyz.openbmc_project.Inventory.Item.PCIePort" that exposes following PCIe Port
properties. dbus-sensors application and bmcweb will use this interface for
these PCIe Port properties.

- PortProtocol
- PortType
- CurrentSpeedGbps
- ActiveWidth

dbus-sensors application and bmcweb will use "xyz.openbmc_project.Metric.Value"
Interface for following PCIe Port Metrics properties.

- PCIeErrors.CorrectableErrorCount
- PCIeErrors.NonFatalErrorCount
- PCIeErrors.FatalErrorCount
- PCIeErrors.L0ToRecoveryCount
- PCIeErrors.ReplayCount
- PCIeErrors.ReplayRolloverCount
- PCIeErrors.NAKSentCount
- PCIeErrors.NAKReceivedCount
- PCIeErrors.UnsupportedRequestCount

## Alternatives Considered

None.

## Impacts

- A new phosphor-dbus-interface will be added for PCIe Port properties.
- Support for new redfish URIs
  /redfish/v1/Systems/system/PCIeDevices/{PCIeDevice}/Ports/{PCIePort}/, and
  /redfish/v1/Systems/system/PCIeDevices/{PCIeDevice}/Ports/{PCIePort}/Metrics/
  will be added in bmcweb.
- bmcweb will include association logic linking a PCIe Device to its PCIe Ports.
- The dbus-sensors application nvidiagpusensor will be updated to retrieve these
  properties from devices using MCTP VDM commands and to populate the
  corresponding dbus interfaces.

### Organizational

- Does this proposal require a new repository? No
- Which repositories are expected to be modified to execute this design?
  dbus-sensors, bmcweb, phosphor-dbus-interfaces
- Make a list, and add listed repository maintainers to the gerrit review.
  Patrick Williams and Ed Tanous

## Testing

The changes in bmcweb and the dbus-sensors application to enable these
properties will be validated using Nvidia devices that support these properties
via MCTP VDM commands.

[1] https://redfish.dmtf.org/schemas/v1/Port.v1_16_0.yaml

[2] https://redfish.dmtf.org/schemas/v1/PortMetrics.v1_7_0.yaml
