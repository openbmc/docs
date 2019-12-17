# PSU Cold Redundancy

Author: Yang Cheng C

Primary assignee: Bills Jason M

Other contributors: None

Created: 2019-12-16

## Problem Description
To support Power Supply Cold Redundancy feature, BMC needs to check and rotate
ranking orders, BMC also needs to create some interface for IPMI and Redfish
commands to set or get configurations of Cold Redundancy.

## Background and References
Power supplies that support Cold Redundancy can be enabled to go into a
low-power state to provide increased power usage efficiency when system loads
are such that both power supplies are not needed. When the power subsystem is in
Cold Redundant mode; only the needed power supplies to support the best power
delivery efficiency are ON. Any additional power supplies, including the
redundant power supply, are in Cold Standby state.

## Requirements
1. To support rank order rotation, BMC needs to write PSU register through an
I2C interface.
2. BMC also needs to create a D-Bus interface with some properties for IPMI
Command and Redfish to set or get the configuration of Cold Redundancy.

## Proposed Design
There are some D-Bus properties stored in settings daemon, Cold Redundancy
service will use the sdbusplus match to monitor these properties. If users
changed these properties through the D-Bus interface, Cold Redundancy service
will know it and then the service will reload the parameters from settings
daemon.

Cold Redundancy will first check D-Bus interfaces such as entity-manager which
keep the properties of bus and address of the power supplies on the system and
keeps monitoring the PSU OperationalStatus through the D-Bus interface to find
which PSUs are in normal status.

According to the configure parameters getting from settings daemon, Cold
Redundancy Service will set the rank orders to Cold Redundancy Config
register(0xd0) in PSUs through the I2C driver.
If PSU OperationalStatus has been changed, Cold Redundancy service needs to
reset rank orders and set them to PSU again. If any PSU is not in normal status,
the rank order of it will always be 0 which means that PSU will never go into
the standby state.

Power Supplies will decide whether to go into cold standby state by themselves,
if current power usage is higher than one PSU can support, then one more PSU
will leave cold standby state. Cold Redundancy service can only set the rank
orders for PSUs in normal status.

There is a property RotationAlgorithm which can be bmcSpecific or userSpecific
. If RotationAlgorithm is set to userSpecific, users also need to set ranking
orders for each PSU. When users are trying to set RotationAlgorithm to
bmcSpecific, users do not need to provide specific ranking order, BMC will set
ranking order to PSU according to PSU sequence.

### Automatic Cycling of Redundancy Ranking

If RotationEnabled is true and RotationAlgorithm is not userSpecific then BMC
will periodically rotate the rank assignments according to the value of a
property called PeriodOfRotation which keeps the interval time of rotation.

The original rank order in PSU will be added one and the last rank order PSU
will be set to rank order 1. If any PSU Event happens, rank order will be reset.
Rotation period will be restarted whenever there is a change in the Master BMC
or BMC reset happens.

### Ipmi command

cmdSetColdRedundancyConfig = 0x2d  
cmdGetColdRedundancyConfig = 0x2e

### Redfish
API     : ``/redfish/v1/Chassis/Power/Oem/ColdRedundancy``  
METHOD  : ``PATCH``  
PAYLOAD :
``{
  "Enabled" : true,
  "RotationEnabled" : true,
  "RotationAlgorithm" : "BMCSpecific",
  "RotationRankOrder" : [
    {
      "@odata.id": "/redfish/v1/Chassis/Flextronics_S_1100ADU00_201_PSU_1"
    },
    {
      "@odata.id": "/redfish/v1/Chassis/Flextronics_S_1100ADU00_201_PSU_2"
    }
  ],
  "PeriodOfRotation" : PT86400S
}``

RotationAlgorithm can only be ``BMCSpecific`` or ``UserSpecific``

### Properties in settings

PATH: "/xyz/openbmc_project/control/power_supply_redundancy"  
INTERFACE: "xyz.openbmc_project.Control.PowerSupplyRedundancy"

Property: "ColdRedundancyStatus"  
Default Value:
``"xyz.openbmc_project.Control.PowerSupplyRedundancy.Status.complete"``

Property: "ColdRedundancyEnabled"
Default Value: ``true``

Property: "RotationEnabled"  
Default Value: ``true``

Property: "RotationAlgorithm"  
Default Value:
``"xyz.openbmc_project.Control.PowerSupplyRedundancy.Algo.bmcSpecific"``

Property: "RotationRankOrder"  
Default Value: ``{0}``

Property: "PeriodOfRotation"  
Default Value: ``86400``

RotationRankOrder is an array which a user may want to set in order to specify
the specific RotationAlgorithm. For example, if user want to set rank order 1,
3, 2 to PSU 1, 2, 3, then RotationRankOrder needs to be {1, 3 , 2}

### D-Bus interface in Cold Redundancy Service

/xyz/openbmc_project/Control:  
    - Interface: xyz.openbmc_project.Control.PowerSupplyRedundancy  
      Properties:  
        NumberOfPSUs: byte  
NumberOfPSUs here is the number of PSUs currently inserted on system.

## Alternatives Considered
Cold Redundancy service get PSU status such as AC Lost, failure, predictive
directly from PSU registers instead of getting them from the PSU Event D-Bus
interface.

## Impacts
The feature is an independent process, which will set a PSU register, so before
PSU Firmware update start, need to stop this daemon from reading and writing
PSU register so that it will not impact the PSU Firmware update. PSU firmware
update daemon can stop PSU Cold Redundancy service before trying to update PSU
firmware and restart this service after the update finish.

Both this daemon and PSU Sensor will access the same I2C bus, need to make sure
no conflict will happen when accessing the I2C bus at the same time.

Since every time PSU is inserted or removed, PSU Manager will ask entity-manager
rescan all the FRU on the system, if this happens very frequently, it will bring
too many workloads to the system.

## Testing

### Ipmi command test

Run Get Cold Redundancy Configuration command to check all the Cold Redundancy
related properties.  
For example: Check if ColdRedundancy is enabled
ipmitool raw 0x30 0x2e 0x01, returns 00 01 01.

Run Set Cold Redundancy Configuration command to change the properties and then
use Get Cold Redundancy Configuration command again to see if the properties are
truely changed.  
For example: Disable ColdRedundancy
ipmitool raw 0x30 0x2d 0x01 0x00, returns 00 00.

## Redfish command test

Run below command in Restful API Client such as Postman

PATCH
``https://ip/redfish/v1/Chassis/Power/Oem/ColdRedundancy``  
Body:
``
{
    "ColdRedundancyEnabled": false
}``

The property ColdRedundancyEnabled in settings should also be changed to false.
Then test all other properties by such way.

### Rank order test

With two normal PSUs, set PeriodOfRotation to 1 minute(this is only for test,
for normal use, PeriodOfRotation are not allowed to lower than 1 day). Check
current PSU orders by reading 0xd0 register in PSUs. After 1 minute, the
original rank order 1 PSU should be changed to rank order 2, and original rank
order 2 should be changed to 1.
Disable ColdRedundancyEnabled or RotationEnabled, rank order will not change any
more.

Remove AC cable from one PSU. The rank order of this PSU is always 0. Another
normal status PSU will always keep to rank order 1.
