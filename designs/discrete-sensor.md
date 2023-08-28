# Generic Discrete Sensor Design
Authors: Selvaganapathi M (selvaganapathim@ami.com), Vinodhini J
(vinodhinij@ami.com), Krishna Raj (krishnar@ami.com), 
Vipin Chandran(vipinc@ami.com)
Created: Aug 28, 2023
## Problem Description 
- Generic discrete interface support.
## Background and reference
ipmi-second-gen-interface-spec-v2-rev1-1.html
## Requirements
- Expose discrete sensor via D-Bus using a generic interface.
- Provide a class and method for discrete sensors
[ like D-Bus , GPIO generic signal creation]
- Clients like IPMI,Webui,Redfish,SNMP should be able to query any given 
discrete sensors.
## Proposed Design

```
┌──────────────────────────┐           ┌────────────────────────┐
│                          │           │                        │
│ ┌──────────────────────┐ │           │ ┌────────────────────┐ │
│ │  JSON Configuration  │ │   ┌───────┼─►  Discrete Sensors  ◄─┼────────┐
│ │                      │ │   │       │ │                    │ │        │
│ └───────────┬──────────┘ │   │       │ └──────────┬─────────┘ │        │
│             │            │   │       │            │           │        │
│ ┌───────────▼──────────┐ │   │       │ ┌──────────▼─────────┐ │        │
│ │  Entity-Manager      │ │   │       │ │  D-Bus Objects     │ │        │
│ │                      │ │   │       │ │                    │ │        │
│ └───────────┬──────────┘ │   │       │ └─────────────▲──────┘ │        │
│             │            │   │       │dbus-sensors   │        │        │
│ ┌───────────▼──────────┐ │   │       └───────────────┼────────┘        │
│ │  D-Bus Objects       ├─┼───┘                       │                 │
│ │                      │ │                           │                 │
│ └──────────────────────┘ │                           │  ┌──────────────┴─────┐
│Entity-Manager            │                           │  │  Sysfs path (OR)   │
└──────────────────────────┘                           │  │  D-Bus Objects (OR)│
                              ┌────────────────────────┤  │  syscalls          │
                              │                        │  └────────────────────┘
                              │                        │
                              │                        │
                   ┌──────────▼──────────┐     ┌───────▼───┐
                   │  Phosphor-ipmi-host │     │  bmcweb   │
                   │                     │     │           │
                   └──────────▲──────────┘     └─────▲─────┘
                              │                      │
                              │                      │
                        ┌─────┴───────┐        ┌─────┴───────┐
                        │  IPMITool   │        │  Redfish    │
                        └─────────────┘        │       call  │
                                               └─────────────┘
```

- Each type of discrete sensor has a service to handle the states in 
the dbus-sensor. 
- A new interface xyz.openbmc_project.Sensor.State is added to query 
discrete sensors events. 
- The state[0-n] string for the sensor are available as part of 
Entity-manager configuration, which can be used for redfish to map the 
state to string.

Configuration looks like below:
```json
{
    "Exposes": [
	    {
            "Name": "Power Unit",
            "EntityId": "0x13",
            "EntityInstance": "0x01",
            "State": [
                "Power Down",
                "Power Cycle",
                "240VA Power Down",
                "Interlock Power Down",
                "AC Lost",
                "Soft Power Control Failure",
                "Power Unit Failure",
                "Predictive Failure"
            ],
            "Type": "Powerunit"
        },
    ]
}
```
Given the following interface example:
```shell
/xyz/openbmc_project/sensors/powerunit/Power_Unit
xyz.openbmc_project.Sensor.State	interface	-	-	-
.State					property	y	0	emits-change writable
```

- Generic discrete design implements discrete class or utils under 
dbus-sensors for creating dbus properties changed signal match, GPIO signal, 
I2C ioctl call for reading I2C register from CPLD or any specific devices. 
- This generic discrete class can be used by multiple discrete sensors services
to read the data from hardware or dbus and handled in the handler functions.
- Design is compatible with redfish, as json file contains the state[0-n] 
property.
- Redfish can call GetSubTree method to list all the sensor instances. Each 
sensor's state can be read by association and can be mapped with state 
property.

Given the following EM interface example:
```shell
/xyz/openbmc_project/inventory/system/board/baseboard/Power_Unit
xyz.openbmc_project.Configuaration.Powerunit	interface	-	-			-
.EntityId					property	t	 3			   
emits-change
.EntityInstance		property	t	 1			          emits-change
.Name						  property	s	 "Power Unit"		 
emits-change
.State						property	as 8 "Power Down" "Power Cycle"
"240VA P... 
emits-change
```

- For an ipmi request(like get SDR), the ipmd service will
populate the SDR data for each sensor based on the state interface. 
- The getSubTree method along with the discrete interface lists the sensor
objects & their respective services collected for the discrete sensors.
- All the sensor properties are read by "getManagedObject" method. SDR is
populated from object path and properties. 
- Get sensor reading command fetches state property under discrete sensor 
interface and maps to IPMI response.

## Alternatives Considered
### Interface per sensor type
- Define different interface for each sensor type. Based on interface, SDR 
can be populated. Redfish and IPMI need to parse each type of discrete sensor.
- Example power supply sensor, "xyz.openbmc_project.Inventory.Item.PowerSupply" 
 interface used to identify the sensor. Ipmid will get power supply type
 sensor based on interface and SDR is populated.
    ```shell
    /xyz/openbmc_project/sensors/powersuppply/PSU1
    xyz.openbmc_project.Inventory.Item	interface	-	-	-
   .Present					property	b	false   
   emits-change writable
    .PrettyName                 property    s   "PSU1"  emits-change writable
    xyz.openbmc_project.Inventory.Item.PowerSupply	interface	-	-	-
    ```
- Power supply presence state is detected by the sensor daemon from PSU and
  updated into "Present" property under "xyz.openbmc_project.Inventory.Item"
  interface.
- "Present" property can be used by ipmid service to show presence detected 
state. Similarly multiple interfaces and properties are added to represent 
diffrent states(Failure, Input lost).
## Testing
- SDR populated as per object path and sensor listed in redfish and UI.
- As per power unit status, state should be asserted/de-asserted in D-Bus 
 properties and IPMI sensor list.
