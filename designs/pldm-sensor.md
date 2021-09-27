# PLDM Sensor

Author: Gilbert Chen

Primary assignee: Gilbert Chen

Other contributors:

Created: September 23, 2021

## Problem Description
In OpenBMC, no sensor daemon in _dbus-sensors_ package is available to
map PLDM sensors to D-Bus Objects yet. And no daemon is available to handle
discovering PLDM devices and enumerating PLDM sensors yet.

## Background and References
The DMTF specifications DSP0236, DSP0240 and DSP0248 describe how BMC to 
discovery MCTP in the platform(DSP0236 section 8.14), how BMC to identify 
whether MCTP device support PLDM(DSP0240 section 7) and how BMC to access 
the sensor in discovered device(DSP0248 section 8.3).

## Requirements
In OpenBMC, the sensors could be monitored by PLDM device and accessed by
BMC through PLDM/MCTP indirectly. The command message is forwarded to PLDM 
device by mctp-demux or MCTP in-kernel driver.

The PLDM sensor could be in satellite Management or PCIe device controller 
and the device might or might not have PDRs to describe the sensors. OpenBMC
needs a mechanism to discover PLDM device in the platform, enumerate sensors
in PLDM device, get the sensor reading and map the sensor status to D-Bus
objects.

The mechanism is expected to be flexibility and reusability. It can work on
different platform design so the tasks of discovery, identification, 
initialization and monitoring are preferred to be handled in different 
services instead of signal service.

## Proposed Design

### Discovery

Discovering MCTP devices relies on MCTP service. The routing table of MCTP 
devices in platform can be retrieved by sending GetRoutingTableEntries to the
MCTP service.

A new service pldm-device is proposed to identify PLDM device and map its 
properties to D-Bus objects for other service (e.g. entity-manager) to 
reference. The pldm-device service is planed to be part of entity-manager git
repo because its feature is similar to fru-device service in the same git repo.
pldm-device gets the EID list from MCTP service first for sending 
GetMessageTypeSupport to each EID in list. pldm-device checks each EID if it
supports PLDM message and then sends GetPLDMType command to the EID to ensure
it supports PLDM type 2 and 4. For the EID passed the tests above, pldm-device 
should try to fetch its FRU data by PLDM type 4 commands and then expose these 
properties to D-Bus objects. The properties of D-Bus object usually includes 
EID#, Part#, Serial#, etc.

### Identification

Once pldm-device completed the discovery, Entity-Manage is notified to probe 
the D-Bus objects with its configuration json files. The configuration json 
files use probe function with particular key word to specify what kind of 
entity would match this configuration. If one of configuration json file is
matched entity-manage would expose the properties defined in configuration 
json file to D-Bus object for other service(e.g. dbus-sensors) to reference.
The configuration file can expose as many as needed properties like sensor id#,
reading unit or thresholds if the probed entity is expected a PLDM device
without PDRs.

### Initialization

two new service pldm-sensor and pldm-pdr-sensor are proposed to initialize the
pldm device exposed by entity-manager according. The pldm-sensor and 
pldm-pdr-sensor will be part of dbus-sensors git repo. There are two types of 
pldm device described in DSP0248. The first is "PLDM for access only". It is 
handled by pldm-sensor. It expects all the necessary setting should be defined
in D-Bus objects for a device which is "PLDM for access only". Second is "PLDM
with PDRs". Is is handled by pldm-pdr-sensor. It expects all the necessary 
setting can be retrieved from corresponding PDRs. When all of settings are 
gathered. pldm-sensor or pldm-pdr-sensor send the PLDM type 2 commands to 
initialize the sensor depending on its type. The PLDM type 2 commands might be
SetNumericSensorEnable, SetSensorThresholds, SetSensorHysteresis and 
InitNumericSensor for Numeric Sensor. After done that pldm-sensor should map 
the sensor's status, reading to a D-bus object for other service(e.g. bmcweb)
to reference.

### Monitoring

After done the initialization, pldm-sensor or pldm-pdr-sensor enters the
monitoring phase to poll the sensor reading or status from pldm device
periodically and update value to mapped D-Bus objects for other service
(e.g. bmcweb) to reference. The sensor reading or status is retrieved from pldm
device through PLDM type2 commands. The commands might be GetSEnsorReading,
GetStateSensorReadings depending on the sensor type.

## Alternatives Considered
In the pldm git repo of OpenBMC project, There is pldmd, libpldmresponder and 
libpldm. The features of pldm-device, pldm-sensor and pldm-pdr-sensor are 
implemented by pldmd. So all of pldm sensors is managed by pldmd. pldmd exposes
sensor as D-Bus objects for other service to reference. It is good for putting
all pldm related service in the same git repo but it could cause some drawbacks.
1)pldmd needs to rewrite or find a way to include the helper classes in
dbus-sensor repo for exposing sensor to D-Bus object. 2)pldm would need to 
develop configuration file like entity-manager does because the configuration
of pldm sensors are platform specific.

## Impacts

Developer is asked to prepare the json file in entity-manager configuration
folder for the pldm sensor to be supported.

## Testing

pldm-sensor, pldm-device and pldm-pdr-sensor can be tested by mocking PLDM
responder.