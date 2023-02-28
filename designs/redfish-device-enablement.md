# Redfish Device Enablement Requester Library

Author: Harsh Tyagi !harshtya

Other contributors: Kasun Athukorala

Created: 2023-02-27

## Problem Description

Implementing Redfish Device Enablement (RDE) with openBMC as specified by the
following DMTF spec:
https://www.dmtf.org/sites/default/files/standards/documents/DSP0218_1.1.0.pdf

The intent of this design doc is to summarize the Redfish Device Enablement
(RDE) requester library that would be responsible for providing the next RDE
operation in sequence required to achieve a particular RDE task such as
discovery, read operation, etc.

RDE Daemon (**in PLDM repo**) would be the primary consumer of this library but
it can be used and extended as required by anyone trying to work with RDE. To
summarize, RDE daemon would be responsible for setting up the RDE stack, i.e.,
doing the PLDM base discovery, RDE discovery and providing handlers to perform
equivalent CURD operations with RDE.

**Why we need this library?**

This library is a general purpose solution for setting up the RDE stack. For
instance, the requester would not have to take care of what all operations are
required for performing the base or RDE discovery, rather just make a call to
base and RDE discovery handlers from the library and ask for the next command in
sequence to be executed. This makes the requester independent of the sequence of
operations required to perform a particular RDE operation.

## Background and References

- PLDM Base specification:
  https://www.dmtf.org/sites/default/files/standards/documents/DSP0240_1.1.0.pdf
- PLDM RDE specification:
  https://www.dmtf.org/sites/default/files/standards/documents/DSP0218_1.1.2.pdf

## Requirements

- Must minimize D-Bus calls needed to get health rollup
- Must minimize new D-Bus objects being created
- Should try to utilize existing D-Bus interfaces

## Proposed Design

### Individual Sensor Health

Map critical thresholds to health critical, and warning thresholds to health
warning. If thresholds do not exist or do not indicate a problem, map
OperationalStatus failed to critical.

### Chassis / System Health

Chassis have individual sensors. Cross reference the individual sensors with the
global health. If any of the sensors of the chassis are in the global health
association for warning, the chassis rollup is warning. Likewise if any
inventory for the chassis is in the global health critical, the chassis is
critical. The global inventory item will be
xyz.openbmc_project.Inventory.Item.Global.

### Chassis / System Health Rollup

Any other associations ending in "critical" or "warning" are combined, and
searched for inventory. The worst inventory item in the Chassis is the rollup
for the Chassis. System is treated the same.

### Fan example

A fan will be marked critical if its threshold has crossed critical, or its
operational state is marked false. This fan may then be placed in the global
health warning association if the system determines this failure should
contribute as such. The chassis that the fan is in will then be marked as
warning as the inventory that it contains is warning at a global level.

## Alternatives Considered

A new daemon to track global health state. Although this would be difficult to
reuse to track individual component health.

## Impacts

## Testing

Testing will be performed using sensor values and the status LED.
