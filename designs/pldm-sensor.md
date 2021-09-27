# PLDM Sensor

Author: Gilbert Chen

Primary assignee: Gilbert Chen

Other contributors:

Created: September 23, 2021

## Problem Description
In OpenBMC currently, there is no service is available to manage PLDM terminus
that has static or self-described sensors.

## Background and References
The DMTF specifications DSP0236 v1.3.1, DSP0240 v1.1.0 and DSP0248 v1.2.1
describe how to discovery MCTP endpoint(DSP0236 section 8.14), how to identify
the PLDM terminus(DSP0240 section 7) from endpoints and how to access the
sensors in PLDM terminus(DSP0248 section 8.3).

In OpenBMC, the sensors managed by PLDM terminus usually are accessed through
MCTP over I2C or PCIe VDM. This document does not covered the design of MCTP
and they are supported by related MCTP service or driver, for example,
mctp-demux or MCTP in-kernel driver.

## Requirements
The PLDM sensors could be static or self-described(with PDRs) and they could
be in satellite Management Controller, on board device or PCIe add-on card.
OpenBMC needs a mechanism to discover PLDM terminus in the platform, sensors
in PLDM terminus and then monitor the status of discovered sensors.

## Proposed Design

Discovering MCTP endpoints relies on MCTP service. The MCTP endpoint routing
table could be retrieved by sending GetRoutingTableEntries command to the
MCTP service.

In OpenBMC pldm repository, the pldmd service has implemented some PLDM types
message responder and a requester helper class to register call back handle to
process response from PLDM terminus so the proposal is based on pldmd service
infrastructure to enable the feature of discovering PLDM terminus and
monitoring PLDM sensors in terminus.

### Discovery

pldmd exposes a "ReScan" method to xyz.openbmc_project.PLDM interface and the
it should be invoked when MCTP network configuration gets changed. When ReScan
method get executed, pldmd sends GetRoutingTableEntries command to MCTP service
to get Routing Table for the latest EID list and then finds out all the PLDM
terminus from the EID list and the sensors in PLDM terminus.

To identify the PLDM terminus, pldmd send GetMessageTypeSupport MCTP command to
check if the EID support PLDM command and then pldmd sends GetPLDMType PLDM
command to check if the EID support PLDM type2 and type4 to ensure the EID is a
PLDM terminus. pldmd should send SetTID command to found PLDM terminus for
assigning a unique Terminus ID.

To find out all sensors from PLDM terminus, pldmd should retrieve all the
Numeric Sensor PDRs by PDR Repository commands for the value of sensorID to make
a sensor name with TID for D-bus paths in the format below.
"/xyz/openbmc_project/sensors/<sensor_type>/<TID_sensorID>"

Regarding to the static sensor(PLDM terminus with PDRs), The Sensor PDRs needs to
be encoded by Platform specific PDR JSON file(2.json) of pldmd by the platform
developer. pldmd will generate theses numeric sensor PDRs encoded by 2.json and
then pldmd be able to parse these PDRs during discovery phase.

### Monitoring

pldmd should initialize new added sensor by necessary commands
(e.g. SetNumericSensorEnable, SetSensorThresholds, SetSensorHysteresis and
InitNumericSensor) and then expose the sensor to D-bus object at path /xyz/
openbmc_project/sensors/<sensor_type>/<TID_sensorID> with
xyz.openbmc_project.Sensor.Value.interface defined at [1]. pldmd will update
the state and reading of PLDM sensor to D-bus objects by polling or aync event
method depending on what type of sensor is.

[1] https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Sensor/Value.interface.yaml

## Alternatives Considered

At first, the proposal is based on the framework of dbus-sensor and
entity-manager to add two new services to openbmc. First is adding pldm-device
which is similar to fru-device service in Entity-Manager repo to expose FRU
data and Sensor setting to D-bus objects. Second is adding pldm-sensor which
is similar to hwmontempsensor service in dbus-sensor repo to monitor PLDM
sensor based on the information of the D-bus objects exposed by Entity-Manager.
However it created unnecessary dependence and got feedback that these functions
should be integrated into pldmd.

## Impacts

## Testing

Creating test cases to verify new developed class APIs based on gtest framework
.The testing can be done without having real PLDM terminus to interactive. The
test case feeds the mockup response of PLDM commands which will be needed during
discovery and monitoring to the tested APIs.