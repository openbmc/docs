# PLDM Sensor

Author: Gilbert Chen

Primary assignee: Gilbert Chen

Other contributors:

Created: September 23, 2021

## Problem Description
In OpenBMC currently, there is no service is available to manage PLDM devices
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

In OpenBMC pldm repository, the pldmd service has implemented some PLDM type
message responder and a requester helper class to register call back handle to
process response from PLDM terminus so the proposal is based on pldmd service 
infrastructure to enable the feature of discovering PLDM terminus and 
monitoring PLDM sensors in terminus.

### Discovery

After the EID routing table is ready from MCTP service, pldmd should start to
find out all the PLDM terminus from the EIDs and the sensor in PLDM terminus.
Firstly, pldmd should test each EID if it supports PLDM message according to
the response of GetMessageTypeSupport and if it supports PLDM type 2 or type 4
based on response of GetPLDMType command. During the discovering, pldmd would
assign a Terminus ID(TID) for a PLDM terminus to the EID by SetTID command.

Secondly, pldmd should try to gather Entity Association PDRs from PLDM terminus
by PDR Repository commands and then expose the FRU information to D-bus object
at path /xyz/openbmc_project/Inventory/Source/PLDM/$tid/$instance# with D-bus
interface defined at https://github.com/openbmc/phosphor-dbus-interfaces/blob
/master/yaml/xyz/openbmc_project/Inventory/Source/PLDM/Entity.interface.yaml. 

Thirdly, pldmd should try to gather Numeric Sensor PDRs from PLDM terminus by
PDR Repository commands and then expose the Numeric Sensor to corresponding
D-bus object at path /xyz/openbmc_project/Inventory/Source/PLDM/$tid/$instance#
with D-bus interface, xyz.openbmc_project.Inventory.Source.PLDM.NumericSensor
which should have following properties with the values from PDRs.
- sensorID
- baseUnit
- sensorDataSize
- Resolution
- Offset
- maxReadable
- minReadable
- warningHigh/Low
- critical/High/Low
- FatalHigh/Low

Regarding to the static sensor(without PDRs), they are proposed to be described
by entity-manager configuration json file. The configuration file makes Entity-
Manager to expose the properties to D-bus Objects with interface
xyz.openbmc_project.Configuration.PldmNumericSensor so that pldmd could the use
these D-bus objects as another source instead of PDRs to expose sensor to D-bus
Object on path /xyz/openbmc_project/Inventory/Source/PLDM/$tid/$instance#.

Lastly, pldmd could optionally fetch FRU data from PLDM terminus by PLDM type 4
messages and find out which D-bus object that is matched containerID and 
instance# in FRU record Set record so that pldmd can expose the fetched FRU data 
to corresponding D-bus object with the interface defined at https://github.com
/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Inventory
/Source/PLDM/FRU.interface.yaml

### Monitoring

When pldmd service started, it should register a callback for PropertiesChanged
signal on the path /xyz/openbmc_project/Inventory/Source/PLDM. When registered
callback get invoked, pldmd should search the D-bus objects which have
xyz.openbmc_project.Inventory.Source.PLDM.NumericSensor interface and then
keeps the sensor properties found in the interface to a sensor list. At the
end of callback, pldmd should initialize new added sensor by necessary commands
(e.g. SetNumericSensorEnable, SetSensorThresholds, SetSensorHysteresis and 
InitNumericSensor) and then expose the sensor to D-bus object with interfaces
defined at https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/yaml
/xyz/openbmc_project/Sensor. pldmd will have a thread to poll all the sensors
in list periodically by GetSEnsorReading and update the reading to D-bus
objects.

## Alternatives Considered

At first, the proposal is trying to two new services to openbmc. First is
adding pldm-device which is similar to fru-device service in Entity-Manager
repo to expose FRU data and Sensor setting to D-bus objects. Second is adding
pldm-sensor which is similar to hwmontempsensor service in dbus-sensor repo to
monitor PLDM sensor based on the information of the D-bus objects exposed by
Entity-Manager. However it created unnecessary dependence and got feedback that
these functions should be integrated into pldmd.

## Impacts

## Testing

Creating test cases to verify new developed class APIs based on gtest framework.