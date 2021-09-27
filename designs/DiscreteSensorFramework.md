Design - Discrete Sensor Framework
Author: J. Vinodhini (vinodhinij@ami.com)
Primary assignee: J. Vinodhini (vinodhinij@ami.com)
Other Contributors:     Pravinash Jeyapaul (pravinashj@ami.com)
                        Selvaganapathi (selvaganapathim@ami.com),
                        Krishnaraj(krishnar@ami.com)
Created: September 27 2020

Problem Description
        The purpose of this design is to provide a generic framework to support
        Discrete sensors.

Background and References

Requirements
1) Each type of discrete sensor should have an object for platform porting
   developer to invoke
2) Sensor should be monitored
3) Create SEL with 16 bytes of raw data

```ascii
Design – Block Diagram




                                                                                            Libgpiod     libsmbios    driver
                                                                                               ▲            ▲            ▲
                                                                                               │            │            │
                                                                                               │            │            │
                                                                                        ┌──────┼────────────┼────────────┼──────┐
                                                                                        │      │            │            │      │
                                                                                        │ ┌────┴────┐  ┌────┴────┐  ┌────┴────┐ │
                                                                                        │ │  Gpiod  │  │ smbiosd │  │ Daemon  │ │
                                                                                        │ └─▲──▲────┘  └▲───────▲┘  └───▲──▲──┘ │
                                                                                        │   │  │        │       │       │  │    │
                                                                                        └───┼──┼────────┼───────┼───────┼──┼────┘
                                                                                            │  │        │       │       │  │
                                                    ┌──────────────┐  ┌──────────────┐      │  │   ┌────┼───────┼───────┘  │
                                                    │   ADC Driver │  │  Fan Driver  │      │  │   │    │       │          │
                                                    └──────▲───────┘  └──────▲───────┘      │  └───┼────┼─────┐ └──┐       │
                                                           │                 │              │      │    │     │    │       │
                                                ┌──────────┼─────────────────┼──────────────┼──────┼────┼─────┼────┼───────┼────┐
┌───────────────────────────────────────┐       │          │                 │              │      │    │     │    │       │    │
│                                       │       │ ┌────────┼─────────────────┼─────────┐ ┌──┼──────┼────┼─────┼────┼───────┼──┐ │
│ ┌──────────────┐      ┌─────────────┐ │       │ │        │                 │         │ │  │      │    │     │    │       │  │ │
│ │  SDR Info    │      │  Interface  │ │       │ │ ┌──────▼───────┐  ┌──────▼───────┐ │ │ ┌▼──────▼────▼─┐  ┌▼────▼───────▼┐ │ │
│ └──────────────┘      └─────────────┘ │       │ │ │              │  │              │ │ │ │              │  │              │ │ │
│                                       │       │ │ │ ┌──────────┐ │  │ ┌──────────┐ │ │ │ │ ┌──────────┐ │  │ ┌──────────┐ │ │ │
└───────────────────┬───────────────────┘       │ │ │ │          │ │  │ │          │ │ │ │ │ │          │ │  │ │          │ │ │ │
                    │                           │ │ │ │   ADC    │ │  │ │   Fan    │ │ │ │ │ │ Service  │ │  │ │ Service  │ │ │ │
              ┌─────▼────────┐  ┌────────────┐  │ │ │ └──────────┘ │  │ └──────────┘ │ │ │ │ └──────────┘ │  │ └──────────┘ │ │ │
              │ Json configs │  │system.json │  │ │ │              │  │              │ │ │ │              │  │              │ │ │
              └───────────┬──┘  └──▲─────────┘  │ │ │ ┌──────────┐ │  │ ┌──────────┐ │ │ │ │ ┌──────────┐ │  │ ┌──────────┐ │ │ │
                          │        │            │ │ │ │          │ │  │ │          │ │ │ │ │ │          │ │  │ │          │ │ │ │
┌─────────────────────────┼────────┼────┐       │ │ │ │ Monitor  │ │  │ │ Monitor  │ │ │ │ │ │ Monitor  │ │  │ │ Monitor  │ │ │ │
│                         │        │    │       │ │ │ └──────────┘ │  │ └──────────┘ │ │ │ │ └──────────┘ │  │ └──────────┘ │ │ │
│ ┌──────────────┐      ┌─▼────────▼──┐ │       │ │ │              │  │              │ │ │ │              │  │              │ │ │
│ │ FRU Device   ├──────►Entitymanager│ ◄───────► │ └──────────────┘  └──────────────┘ │ │ └──────────────┘  └──────────────┘ │ │
│ └─────▲────────┘      └─────────────┘ │       │ │       Threshold Sensor             │ │            Discrete Sensor         │ │
│       │    Entity-manager             │       │ └────────────────────────────────────┘ └────────────────────────────────────┘ │
└───────┼───────────────────────────────┘       │                                                                               │
        │                                       └─────────▲────────────────────────────────┬────────────────────────────────────┘
        │Read/Write                                       │                                │
        │                              Sensor list        │                                │
┌───────▼───────────────────────────────┐       ┌─────────▼─────────┐      ┌───────────────▼───────┐
│                                       │       │                   │      │  phosphor-sel-logger  │
│            ipmid/intel-ipmi-oem       ◄───────►   Sensor dbus     │      │                       │
│                                       │       │      objects      │      └──▲─────▲──────────────┘
└───────────────────▲───────────────▲───┘       └─────────┬─────────┘         │     │
                    │               │                     │Sensor list        │     │
                    │               │                     │                   │     │
          ┌─────────▼─────────┐     │           ┌─────────▼─────────┐         │     │
          │    IPMITool       │     │           │  WebUI/Redfish    ◄─────────┘     │
          │                   │     │           │                   │               │
          └───────────────────┘     │           └───────────────────┘               │
                                    │                                               │
                                    └───────────────────────────────────────────────┘


Figure-1: Proposed Design Block Diagram

```


Proposed Design
           In json file, two info can be configured along with other
              properties of sensor
o	SDR Info                                -               Currently, SDR is part of the code and not
                                                                available to configure. In this framework a
                                                                provision to configure the SDR is done
o	Interface configuration	-       Info of interfaces w.r.t offsets

o       The sensors configured in json are available as inventory objects in
                entity manager
o       If sensor is configured as disabled, then the corresponding inventory
                objects are not available in entity manager
o       Similar, to threshold-based sensors, discrete sensors too would be
                creating different services based on different sensor types
o       The inventory objects are loaded to the services from entity manager
o       A sensor object is created for a particular sensor in the discrete
                sensor service
o       Each sensor has different offsets to be determined based on different
                hardware layer
o       A service/daemon is designed to process these offset data based on the
                underlying hardware layer

For example: Consider a sensor having an offset, to be determined by GPIO
o       The sensor’s discrete sensor type service will create an object
o       Each offset of a sensor will have a mapping interface associated with it.
o       Methods and properties are added to the interface and are exposed to the
                hardware layer service/daemon in common.
o       The properties are updated by the respective service/daemon based on the
                underlying hardware layer interaction.
o       If GPIO pin value to be known, then gpio daemon will be requested for the
                property update via the interface & method created for that sensor
                and specific offset.
o       The GPIO daemon, will in-turn fetch the gpio pin value via libgpio and
                update the property
o       Once the property value is updated, the discrete sensor process monitors it
o       The updated value is processed and checked for any specific conditions in
                discrete sensor service
o        A 16 byte, SEL raw data is logged

Sample SDR & Interface configuration for a discrete sensor in json file:
{
            "Unit": "xyz.openbmc_project.Sensor.Value.Unit.Digital",
            "EvStat": 231,
            "Status": "okay",
            "Name": "Discrete Test",
            "SdrInfo": [
                {
                        "RecordType": "0x01",
                        "EntityId": "0x20",
                        "EntityInstance": 1,
                        "SensorInit": "0xF7",
                        "SensorCap": "0x48",
                        "SensorType" : "0x0c",
                        "EventType": "0x6f",
                        "AssertEventMask": "0x0001",
                        "DeassertEventMask": "0x0000",
                        "SensorUnit2" : 0
						....
                }
            ],
       "Interfaces": [
            {
	      "Offset": 1,
                      "IntType": "GPIO",
                      "Pin": 202
            },
            {
	    “Offset”: 2,
                    “IntType”: “I2c”,
	     “Bus”: 0,
	     “Slave”: 0x12,
	     “Device”: “xxxxx”
            },
            {
	    “Offset”: 3,
	    "IntType": “CPLD”,
	     "Bus": 4,
	     "Slave": 0x32
            },
            {
	    “Offset”: 4
	    “IntType”: “GPIO”,
	    ‘Pin”:	5
            }
		....
        ],
            "Type": "DiscreteSensor"
}

```
Representation of a sensor in Entity Manager as inventory objects:

Service xyz.openbmc_project.EntityManager:
`-/xyz
  `-/xyz/openbmc_project
    |-/xyz/openbmc_project/EntityManager
    `-/xyz/openbmc_project/inventory
      `-/xyz/openbmc_project/inventory/system
        |-/xyz/openbmc_project/inventory/system/board
        | `-/xyz/openbmc_project/inventory/system/board/Vulcancity_Baseboard
        |   |-/xyz/openbmc_project/inventory/system/board/Vulcancity_Baseboard/Discrete_Test

Interfaces are as below in Entity-manager inventory path,

xyz.openbmc_project.Configuration.Processor        	interface -         -                                 	-
.Name                                           	property  s        	"Discrete_Test"                		emits-change
.Status                                         	property  s        	"okay"                         		emits-change
.Type                                           	property  s			"Processor"                    		emits-change
xyz.openbmc_project.Configuration.DiscreteSensor.Interfaces0 interface -         -                      	-
.Delete                                          	method    	-       -                                 	-
.IntType                                    		property  	s       "GPIO"								emits-change we
.Offset                                         	property  	y       1                         			emits-change we
.Pin                                         		property  	q       202                           		emits-change we
xyz.openbmc_project.Configuration.DiscreteSensor.Interfaces1 interface	-         							-            -
.Delete                                           	method    	-         -                                 -
.IntType                                        	property	s         "CPLD"                           	emits-change we
.Offset                                         	property  	y         2                     	    	emits-change we
.Slave                                          	property  	y         0x12                              emits-change we
.Bus                                            	property  	q         0	                         	    emits-change we
xyz.openbmc_project.Configuration.DiscreteSensor.Interfaces2 interface -         -                              -
.Delete                                           	method    	-         -                                 		-
.IntType                                        	property  	s         "I2C"                        	    emits-change we
.Bus                                            	property 	q         4	                       	        emits-change we
.Offset                                         	property  	y         3                       	        emits-change we
.Slave                                          	property  	y         0x32                             	emits-change we
.Device			       								property  	s         "xxxx"                           	emits-change we
xyz.openbmc_project.Configuration.DiscreteSensor.Interfaces3 interface -        -                                        -
.Delete                                           	method    	-         	-                                        -
.IntType                                        	property  	s         "GPIO"                     	    emits-change we
.Offset                                         	property  	y         4	                       	        emits-change we
.Pin                                         		property  	q         5                   	        	emits-change we


Representation of a sensor in Discrete Sensor service:

Service xyz.openbmc_project.Discretesensor:
`-/xyz
  `-/xyz/openbmc_project
    `-/xyz/openbmc_project/sensors
      |-/xyz/openbmc_project/sensors/processor
      | `-/xyz/openbmc_project/sensors/processor/Discrete_Test

NAME                                                  	TYPE      SIGNATURE RESULT/VALUE                             FLAGS
xyz.openbmc_project.Sensor.Discrete.State             	interface	-         -                                        	-
.updatereading                                     		Method		q         -                                     	-
xyz.openbmc_project.Sensor.Discrete.State               interface	-         -                                        	-
.State                                         			Property	q         2                                       	-
.EventData                                         		Property    ay        2                                       	-
xyz.openbmc_project.Sensor.Value          				interface 	-         -                                        	-
.AssertEventMask                          				property  	q         1											emits-change
.DeassertEventMask                   					property  	q         0           								emits-change
.EntityId                                 				property  	y         32          								emits-change
.EntityInstance                           				property  	y         0           								emits-change
.EventType                                				property  	y         111         								emits-change
.RecordType                               				property  	y	      1           								emits-change
.SensorType                               				property  	y         12          								emits-change
.SensorUnit1                              				property  	i         0           								emits-change
.Unit                                     				property  	s         "xyz.openbmc_project.Sensor.Value.Uni... emits-change
……………..

```

Alternatives Considered
•        Instead of creating services based on sensor type, services can also be
                 created based on interface types.

Impacts

Testing
•       Sensors configured in json are available as inventory objects in
                entity manager.
•       These inventory objects can be queried using the busctl commands
                for the tree hirarchy as well as details on each inventory object
•       The sensors objects created by the discrete sensor process can be validated via client interfaces like UI, Redfish and IPMItool

