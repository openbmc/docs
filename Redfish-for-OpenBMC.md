# Redfish for OpenBMC

This document describes the design principles for OpenBMC supporting Redfish.  The overriding guidance is to be an interpreter between dbus and Redfish.  Redfish will not containment the architecture of OpenBMC, Systemd nor D-Bus.  This is the same guiding principal between IPMI and OpenBMC.  

This Redfish implementation will adhere to the [OCP Schema](https://www.dmtf.org/sites/default/files/standards/documents/DSP2049_0.2.2b.pdf)

## Areas of Development

1. System Schema Definitions
2. Translations (dbus <---> redfish)
3. Function calls
4. OEM Extensions


## Walk through of the Technology
The DMTF provides examples of Redfish mockups such as the [Simple Rack-mounted Server](http://redfish.dmtf.org/redfish/v1/mockup/841).  Some of the interesting examples are embedded in to this document to provide some context behind decisions being made. 


In this example there are Redfish infrastructure (RI), dynamic values (DV), and links (LS) to other schemas.

```
redfish » v1 
"@odata.type": "#ServiceRoot.v1_0_2.ServiceRoot",    <--RI
"Id": "RootService",                                 <--RI
"Name": "Root Service",                              <--RI
"RedfishVersion": "1.0.2",                           <--RI
"UUID": "92384634-2938-2342-8820-489239905423",      <--DV
"Systems": {                                         <--LS
    "@odata.id": "/redfish/v1/Systems"               <--LS
},
```

The system shows additional requirements such as building a list of possible values as with _BootSourceOverrideTarget@Redfish.AllowableValues_, and a mechanism for calling methods (both OEM and standard)  

```
redfish » v1 » Systems » 437XR1138R2
"Boot": {
	"BootSourceOverrideEnabled": "Once",
	"BootSourceOverrideTarget": "Pxe",
	"BootSourceOverrideTarget@Redfish.AllowableValues": [
		"None",
		"Pxe",
		"Cd",
	],
	"BootSourceOverrideMode": "UEFI",
	"UefiTargetBootSourceOverride": "/0x31/0x33/0x01/0x01"
} ,
"Actions": {
	"#ComputerSystem.Reset": {
		"target": "/redfish/v1/Systems/437XR1138R2/Actions/ComputerSystem.Reset",
		"ResetType@Redfish.AllowableValues": [
			"On",
			"ForceOff",
			"GracefulShutdown",
			"PushPowerButton"
		]
	} ,
"Oem": {
	"#Contoso.Reset": {
		"target": "/redfish/v1/Systems/437XR1138R2/Oem/Contoso/Actions/Contoso.Reset"
	}
},
```

The interesting point of this collection node is that it is a summary of DIMMS.  This node is not defined as a schema but is a result of multiple same schema types known as a collection

```
redfish » v1 » Systems » 437XR1138R2 » Memory
Members@odata.count": 4,
"Members": [
	{
		"@odata.id": "/redfish/v1/Systems/437XR1138R2/Memory/DIMM1"
	} ,
	{
		"@odata.id": "/redfish/v1/Systems/437XR1138R2/Memory/DIMM2"
	} ,
	{
		"@odata.id": "/redfish/v1/Systems/437XR1138R2/Memory/DIMM3"
	} ,
	{
		"@odata.id": "/redfish/v1/Systems/437XR1138R2/Memory/DIMM4"
	}
],
```

The duplicate node example.  A pattern of using the same schema for endpoints is common.  Examples include CPU and DIMMs.  The key point is the Redfish repository will need to keep track of each D-Bus end point and and signal as properties will change (think fan RPM or voltage sensors).  

```
redfish » v1 » Systems » 437XR1138R2 » Memory » DIMM2
"@odata.type": "#Memory.v1_1_0.Memory",
"Name": "DIMM Slot 2",
"Id": "DIMM2",
"RankCount": 2,
"MaxTDPMilliWatts": [
	12000
],
"CapacityMiB": 32768,
"DataWidthBits": 64,
"BusWidthBits": 72,
"ErrorCorrection": "MultiBitECC",
	"MemoryLocation": {
	"Socket": 1,
	"MemoryController": 1,
	"Channel": 1,
	"Slot": 2
} ,
"MemoryType": "DRAM",
"MemoryDeviceType": "DDR4",
"BaseModuleType": "RDIMM",
"MemoryMedia": [
	"DRAM"
],
"Status": {
	"State": "Enabled",
	"Health": "OK"
} ,
"@odata.context": "/redfish/v1/$metadata#Memory.Memory",
"@odata.id": "/redfish/v1/Systems/437XR1138R2/Memory/DIMM2"
```


The above examples cover the requirements listed in .  In summary, any Redfish implementation attempt must satisfy...

- Fixed data
- Dynamic Data
- Links
- Actions
- Collections
- Multiple similar schemas



### System Schema Definitions
OpenBMC relies on Yocto for layer systems.  This allows a single directory subsystem to support system specific items.  The [meta-openbmc-machines](https://github.com/openbmc/openbmc/tree/master/meta-openbmc-machines) filesystem is where all Redfish items should be defined.  The schemas will define any hardcoded values

In order to add support to a system the desired Redfish Schema's need to be added.  Some important points...

- Data in the schema is hardcoded 
- Schema type and version definitions are not supported

```
meta-openbmc-machines/.../meta-<machine>/recipies-phosphor/redfish
``` 

Including any data in the systemdb.json will overwrite the default values from the redfish service.  In this example there is a request to change the supported list of bootable devices.  Note the AllowableValues still must be understood by the redfish service.  The values are not arbitrary..

```
redfish/systemdb.json:

    "Boot": {
        "BootSourceOverrideTarget@Redfish.AllowableValues": [
            "None",
            "Pxe",
        ],
        "BootSourceOverrideTarget": null,
        "BootSourceOverrideEnabled": null
    },
```

Supporting OEM entries is similar to over riding defaults.


```
"Oem": {
	"#Contoso.Reset": {
		"target": "/redfish/v1/Systems/<model>/Oem/Contoso/Actions/Contoso.Reset"
		}
}
```

## Resolving the delayed discovery dilmma 
Not all information about a system is known when the system is in the powered off state.  This is because supporting infrastructure such as BIOS or Error logs have not run and therefore no dbus interface exists.  This creates a problem for the Redfish server.  As dbus signals unfold the Redfish will need to update various entries and Collection interfaces.

####Example

Powered Off

```
GET: /xyz/openbmc_project/inventory/

  "data": [] 
```

Powered On

```
GET: /xyz/openbmc_project/inventory/system/chassis/motherboard/

  "data": [
    "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu0", 
    "/xyz/openbmc_project/inventory/system/chassis/motherboard/membuf", 
    "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu", 
    "/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm0", 
    "/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm1", 
    "/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm2", 
    "/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm3"
  ]
```

### 'Interface Added' signal dbus
Whenever a signal of Interface Added is broadcasted over dbus the Redfish service must create/remove the corresponding collection node

