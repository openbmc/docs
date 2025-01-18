# Modbus based hardware inventory

Author: Jagpal Singh Gill <paligill@gmail.com>

Other contributors: Amithash Prasad <amithash@meta.com>

Created: 14th January 2025

## Problem Description

Certain devices can be connected to the BMC via Modbus interface, either RTU or
TCP. However, openBMC currently lacks support for discovering these devices and
exposing their relevant inventory and sensor access details. This proposal aims
to introduce a Modbus-based hardware inventory model. The scope of this document
will focus on RTU (Serial) connections, but the design is extensible to
accommodate TCP connections in the future.

## Background and References

- [Modbus Specification](https://www.modbus.org/docs/Modbus_Application_Protocol_V1_1b3.pdf)
- [Belimo Flow Meter Modbus Interface](https://www.belimo.com/mam/general-documents/system_integration/Modbus/belimo_Modbus-Register_22PF_V4_2_en-gb.pdf)

## Requirements

- Able to detect and identify Modbus devices connected to the system, including
  those daisy-chained on a serial link with varying device types.
- Able to read and expose inventory information from the discovered Modbus
  devices.
- Enables the representation of Modbus devices as inventory items via Entity
  Manager.
- Able to handle the online removal of Modbus devices, ensuring proper cleanup
  of device inventory and associated data.

## Proposed Design

### Proposed End to End flow

```mermaid
sequenceDiagram;
autonumber
participant EM as EntityManager
participant MDD as ModbusDeviceDetect<br> Service

%% EntityManager has <platform>.json and <modbus_device>.json as defined by the platform
Note over EM: <platform>.json <br> {"Exposes": { ..., "ModbusDeviceDetect" : ... },...}
Note over EM: <modbus_device>.json <br>{  "Exposes": {"ModbusDevice": ...}, <br>"Probe": "xyz.openbmc_project.Inventory.Source.Modbus.FRU..."}

%% Bootstrap Action for ModbusDeviceDetect
MDD ->> MDD: Create Matcher <br> (InterfaceAdded, <br>xyz.openbmc_project.Configuration.ModbusDeviceDetect)
MDD ->> MDD: Create Matcher <br> (InterfaceRemoved, <br>xyz.openbmc_project.Configuration.ModbusDeviceDetect)

%% Inventory Add Flow
rect rgba(0, 0, 200, .1)
Note over EM, MDD: Inventory Add Flow

%% EntityManager processes JSON configuration files
Note over EM: Process <modbus_device>.json
EM ->> EM: Create Matcher for Probe <br> xyz.openbmc_project.Inventory.Source.Modbus.FRU

Note over EM : Process <platform>.json
alt Is ModbusDeviceDetect
    EM ->> EM: Add Interface <br> xyz.openbmc_project.Configuration.ModbusDeviceDetect
    EM ->> MDD: Notify Interface Added
end

MDD ->> MDD: Discover Modbus devices for <br> ModbusDeviceDetect Address range

loop For Discovered Modbus Device
    MDD ->> MDD: Get inventory information <br>(PartNumber, SerialNumber etc)
    MDD ->> MDD: Add Interface <br> xyz.openbmc_project.Inventory.Source.Modbus.FRU
end

MDD -->> EM: Notify Interface Added

EM ->> EM: Probed for <br> xyz.openbmc_project.Inventory.Source.Modbus.FRU
EM ->> EM: Add Interface<br> xyz.openbmc_project.Inventory.Item.<ModbusDevice>
EM ->> EM: Add Interface<br> xyz.openbmc_project.Configuration.<ModbusDevice>

end

%% Inventory Delete FLow
rect rgba(0, 0, 100, .1)
Note over EM, MDD: Inventory Delete Flow

MDD->>MDD: Modbus device undetected

MDD->>MDD: Delete Interface <br> xyz.openbmc_project.Inventory.Source.Modbus.FRU

MDD -->> EM: Notify Interface Removed

EM ->> EM: Delete Interface<br> xyz.openbmc_project.Inventory.Item.<ModbusDevice>
EM ->> EM: Delete Interface<br> xyz.openbmc_project.Configuration.<ModbusDevice>

end
```

The _ModbusDetectDevice_ service is responsible for probing Modbus devices and
making their FRU inventory information available on the D-Bus, thereby
addressing requirements #1 and #2. Additionally, it handles the removal of
Modbus devices, which includes deleting their inventory data, thus fulfilling
requirement #4.

The Entity Manager's global.schema will be updated to include relevant Modbus
devices as inventory items, following the approach shown in the
[patch](https://gerrit.openbmc.org/c/openbmc/entity-manager/+/77350). This
change addresses requirement #3.

### Proposed D-Bus Interfaces

The DBus Interface for Modbus hardware inventory will consist of following -

- [xyz.openbmc_project.Configuration.ModbusDeviceDetect](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/77293)

  - Provides configuration details, including Modbus address range and inventory
    registers, to facilitate the discovery of Modbus devices.

- [xyz.openbmc_project.Inventory.Source.Modbus.FRU](https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/77294)
  - Provides device access details, including Modbus address and LinkTTY
    information, along with inventory data for all discovered Modbus devices.

### Proposed EM Schema Definitions

#### ModbusDeviceDetect

For details on the schema definition, please
[refer](https://gerrit.openbmc.org/c/openbmc/entity-manager/+/77223). Below is
an example of entity manager configuration for this schema -

```json
"Exposes": [
...
{
  "Type": "ModbusDeviceDetect",
  "Name": "Flow Meter 1",
  "AddressRangeStart": [
    12,
    50
  ],
  "AddressRangeEnd": [
    20,
    100
  ],
  "ConnectionType": "RTU",
  "RTUBaudRate": 115200,
  "DataParity": "Odd",
  "DataEndianness": "Little",
  "RegisterNames": [
    "PartNumber",
    "SerialNumber"
  ],
  "RegisterAddresses": [
    100,
    200
  ],
  "RegisterSizes": [
    6,
    4
  ],
  "RegisterFormats": [
    "String",
    "String"
  ]
}
...
]
```

## Alternatives Considered

An alternative approach is to have board configurations expose all relevant
Modbus registers, including inventory, which can be consumed by sensor
monitoring daemons. These daemons would then handle exposing inventory items,
simplifying the flow but making the monitoring daemon more complex as it would
need to handle inventory item interfaces and rewrite existing code from Entity
Manager. However, this proposed flow aligns better with existing standards,
where ModbusDeviceDetect service serves a similar purpose to FRU device daemon
for Modbus device discovery.

## Impacts

### Performance Impacts

The design may result in a slight decrease in system performance due to periodic
checks for the Modbus device's presence, but this impact is negligible.

### Organizational

- Does this proposal require a new repository?
  - Yes
- Who will be the initial maintainer(s) of this repository?
  - Patrick Williams, Jagpal S Gill, Amithash Prasad
- Which repositories are expected to be modified to execute this design?
  - EntityManager
- Make a list, and add listed repository maintainers to the gerrit review.

## Testing

### Unit Testing

All the functional testing of the reference implementation will be performed
using GTest.

### Integration Testing

The end to end integration testing involving Servers (for example BMCWeb) will
be covered using openbmc-test-automation.
