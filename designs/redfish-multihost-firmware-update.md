# Redfish multi-host firmware update support

Author: Delphine CC Chiu <Delphine_CC_Chiu@Wiwynn.com> 

Created: 2022-10-06

## Problem Description
- Support non-BMC component firmware update such as CPLD, BIC via Redfish interface.
- Support muti-host server architecture firmware update, user could specify which server to update within Redfish request.

## Background and References
- In current design, Redfish interface only support BMC and BIOS update function.
- In multi-host server architecture, we have some the same component on each board which share the same firmware image.
 
```
                              +-----------------+
                              |Server board 1   |
                      path 1  |  +--------+     |
                    +---------+->|  CPLD  |     |
                    |         |  +--------+     |
|-------------- +   |         +-----------------+
|               |   |         
|     BMC       +-- +
|               |   |         +-----------------+
|-------------- +   |         |Server board 2   |
                    | path 2  |  +--------+     |
                    +---------+->|  CPLD  |     |
                    |         |  +--------+     |
                    |         +-----------------+
                    |                 .
                    |                 .
                    |                 .
                    +----------->     .
```

## Requirements
- For the upate packages for non-BMC components, should add "Compatible" property to specify the component.
- Each multi-host project need to provide hardware config file to indicate the access path.

## Proposed Design
- Provide the function to process firmware component other than BMC and BIOS.
  Get "Compatible" property value from MANIFEST, which can specify the component to update, then call the correspoding 
- Add a property in Redfish API to specify which server board to be updated.
E.g.,
```
curl -k --data-binary $image_path -u root:0penBmc -d '{"Target":"Server1"}' -H $HEADER $BMC_IP/redfish/v1/UpdateService/
```

Add a json configuration file on project layer to indicate the hardware path of each target.
E.g.,
```
{
	“CPLD”: {
		“server1”: {
			“bus”: 0x1, 
			“addr”: 0x44,
			“vendor”: “lattice”,
			“chip”: “LCMXO3_4300C”
		}, 
		“server2”: {
			“bus”: 0x2, 
			“addr”: 0x44, 
			“vendor”: “lattice”,
			“chip”: “LCMXO3_4300C”
		},
	},
	“BIC”: {
		“server1”: {
			“bus”: 0x1, 
			“addr”: 0x20, 
			“vendor”: “Aspeed”,
			“chip”: “AST1030”
		}, 
		“server2”: {
			“bus”: 0x2, 
			“addr”: 0x20, 
			“vendor”: “Aspeed”,
			“chip”: “AST1030”
		},
	}
```

## Alternatives Considered
We have considered to modify the "Compatible" property from MANIFEST of image to indicate which server board we want to update. 
In this way, we will need to prepare multiple update packages for each server board. And the same component on each server board could not share update package. The cost of firmware update will be larger if we have more host.

## Impacts
The property of Redfish API /redfish/v1/UpdateService/ need to be extended.

## Testing
To verify this API works for multi-host server, we can update components on each server, and get the firmware version after the update is done, make sure the firmware is updated successfully.


