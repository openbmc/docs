# CMOS Battery Status Support

Author: Mahalakshmi K <mahalakshmik@ami.com>
Other contributors: None
Created: June 10, 2026

## Problem Description

OpenBMC platforms already represent CMOS battery voltage with existing ADC
sensors. This provides telemetry, but it does not provide a clear and common
battery health semantic for system consumers.

Today, platform integrators can expose voltage and threshold alarms, but
external consumers still need platform-specific logic to identify which sensor
represents the coin-cell battery and to interpret warning and critical low
thresholds as battery health. The inconsistency is therefore not in the ADC
telemetry itself, but in the lack of a common battery-health object for system
consumers. Different platforms can publish equivalent voltage data while still
requiring different consumer-side rules to decide whether the battery is
healthy, low, or failed.

The goal of this design is to define a common way to expose CMOS battery health
state at runtime while continuing to use existing ADC telemetry.

## Background and References

Entity Manager publishes platform configuration records on D-Bus based on probe
conditions. dbus-sensors consumes these records and publishes sensor telemetry.

Existing CMOS battery implementations are typically represented as voltage
sensors under:

- /xyz/openbmc_project/sensors/voltage/{SensorName}

with standard threshold interfaces:

- xyz.openbmc_project.Sensor.Threshold.Warning
- xyz.openbmc_project.Sensor.Threshold.Critical

This design builds on that model and adds explicit battery health state
publication for consumers.

References:

- [dbus-sensors adcsensor](https://github.com/openbmc/dbus-sensors/tree/master/src/adc)
- [phosphor-dbus-interfaces OperationalStatus](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/Decorator/OperationalStatus.interface.yaml)
- [phosphor-dbus-interfaces Availability](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/Decorator/Availability.interface.yaml)
- [entity-manager configuration format](https://github.com/openbmc/entity-manager/blob/master/CONFIG_FORMAT.md)

## Requirements

- Batteries should be detectable based on FRU and platform configuration, and
  exposed only when present.
- OpenBMC shall continue to publish raw battery voltage telemetry from ADC
  sensors.
- OpenBMC shall publish explicit battery health state for runtime consumers.
- Battery health publication shall include explicit D-Bus paths and interfaces,
  so consumers do not need platform-specific interpretation logic.
- The design shall support external interface mapping for Redfish and IPMI
  based on common runtime battery health state.
- The runtime battery health object shall describe health and availability only;
  voltage telemetry remains published through the existing ADC sensor object.
- Runtime battery health state shall be updated when threshold conditions change
  and removed when the backing platform configuration is removed.

## Proposed Runtime Interfaces

Battery health state is published at:

- /xyz/openbmc_project/state/battery/{Name}

using:

- xyz.openbmc_project.State.Decorator.OperationalStatus
  - Functional: true when battery is healthy enough for operation
- xyz.openbmc_project.State.Decorator.Availability
  - Available: true when backing ADC source is reachable
- xyz.openbmc_project.Association.Definitions
  - association to related platform objects

Raw voltage remains published at the existing sensor path:

- /xyz/openbmc_project/sensors/voltage/{SensorName}

## Proposed External Consumer Model

Redfish and IPMI should consume battery health from
/xyz/openbmc_project/state/battery/{Name}, while raw voltage remains available
from sensor telemetry.

Expected mapping direction:

- Redfish Battery Status.State and Status.Health derive from
  /xyz/openbmc_project/state/battery/{Name} state interfaces.
- IPMI battery-related health state derives from the same
  /xyz/openbmc_project/state/battery/{Name} interfaces.

Initial mapping expectation:

| Runtime condition | D-Bus state | Redfish mapping direction | IPMI mapping direction |
| --- | --- | --- | --- |
| Source unavailable | Available=false, Functional=false | Status.State=UnavailableOffline, Status.Health=Critical | Sensor/state reported as unavailable |
| Healthy | Available=true, Functional=true | Status.State=Enabled, Status.Health=OK | Sensor/state reported as normal |
| Low warning | Available=true, Functional=true | Status.State=Enabled, Status.Health=Warning | Sensor/state reported as warning |
| Critical low | Available=true, Functional=false | Status.State=Enabled, Status.Health=Critical | Sensor/state reported as critical/failure |

Exact payload and command-specific encoding remain implementation details in
consumer repositories, but they are expected to derive from the D-Bus object
and interfaces defined above.

## Proposed Design

The preferred implementation is to extend existing ADC sensor handling in
`dbus-sensors` rather than adding a new standalone daemon.

High-level flow:

- Entity Manager publishes a battery-capable configuration record for platforms
  that include a coin-cell battery monitor.
- ADC sensor logic continues to publish voltage and threshold alarms.
- Battery health logic in dbus-sensors derives common health state from warning
  and critical low thresholds.
- dbus-sensors publishes battery health objects in the state namespace under
  /xyz/openbmc_project/state/battery/{Name}.
- Redfish and IPMI consume /xyz/openbmc_project/state/battery/{Name}.

## State Mapping

| Condition | Available | Functional | Meaning |
| --- | --- | --- | --- |
| ADC source unreachable | false | false | Source unavailable |
| No low alarms | true | true | Battery healthy |
| Warning low alarm active | true | true | Battery low warning |
| Critical low alarm active | true | false | Battery failed or critically low |

## Proposed Entity Manager Model

Platforms that expose CMOS battery health would add a battery-specific expose
record that identifies the backing ADC source. Configuration values remain
platform-specific; this design defines the runtime D-Bus behavior that is
common across platforms.

Proposed fields:

- Name
- Type (for example CoinCellBattery)
- SourceSensor (ADC sensor name used as battery source)
- State labels (optional human-readable labels)

Notes:

- EntityInstance is intentionally not included.
- Configuration remains platform-specific; only the schema and runtime behavior
  are common.

## Example Entity Manager Config Fragment

```json
{
  "Exposes": [
    {
      "Name": "CMOS Battery",
      "Type": "CoinCellBattery",
      "SourceSensor": "P3V3_AUX",
      "State": ["Battery Low", "Battery Failed"]
    }
  ],
  "Name": "AST2600 EVB Baseboard",
  "Probe": [
    "xyz.openbmc_project.FruDevice({'BOARD_PRODUCT_NAME': 'AST2600 EVB', 'BOARD_MANUFACTURER': 'ASPEED'})",
    "OR",
    "xyz.openbmc_project.FruDevice({'PRODUCT_PRODUCT_NAME': 'AST2600 EVB', 'PRODUCT_MANUFACTURER': 'ASPEED'})",
    "MATCH_ONE"
  ],
  "Type": "Board"
}
```

## Alternatives Considered

- Keep only voltage telemetry and no explicit battery health object.
  Rejected because consumers must keep platform-specific interpretation logic.
- Add a standalone batterystatus daemon that polls ADC state.
  Rejected because extending existing ADC sensor handling is simpler and avoids
  redundant D-Bus IPC and duplicate polling paths.
- Move runtime health evaluation into Entity Manager.
  Rejected because Entity Manager should remain focused on configuration
  publication.

## Impacts

Repositories expected to be modified:

- openbmc/docs
- entity-manager
- dbus-sensors

Follow-up repositories for consumer mapping:

- bmcweb
- IPMI repository

## Testing

- Validate FRU-gated publication and runtime object creation only on matching
  platforms.
- Validate state transitions for source unavailable, warning-low, and
  critical-low conditions.
- Validate removal of runtime battery objects when platform configuration is
  removed.
- Validate that Redfish/IPMI follow-up mappings consume the runtime battery
  health object and no longer depend on platform-specific threshold logic.
