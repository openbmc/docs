# Redfish multi-host firmware update support

Author: Delphine CC Chiu <Delphine_CC_Chiu@Wiwynn.com>

Created: 2022-10-06

## Problem Description
- Support non-BMC component firmware update such as CPLD, BIC via
  Redfish interface.
- Support muti-host server architecture firmware update,
  user could specify which server to update within Redfish request.

## Background and References
- In current design, Redfish interface only support BMC and BIOS update.
- In multi-host server architecture, we have some the same component on
  each board which share the same firmware image.

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
- For the update packages for non-BMC components, should add "Compatible"
  property to specify the component.
- Each multi-host project need to provide hardware config file to indicate
  the access path.

## Proposed Design
- Provide the function to process firmware component other than BMC and BIOS.
  Get "Compatible" property value from MANIFEST, which can specify the
  component to update, then call the correspoding update flow.
- Add a property in Redfish API to specify which server board to be updated.
  E.g.,
  ```
  curl -k --data-binary $image_path -u root:0penBmc -d '{"Target":"server1"}'\
  -H $HEADER $BMC_IP/redfish/v1/UpdateService/
  ```
  In the above case, the property "Target", that is, "server1", will be set as
  a property on object "/xyz/openbmc_project/software/update_target" under
  "xyz.openbmc_project.Settings".

- Add a json configuration file on project layer to indicate the hardware path
  of each target, which will be installed under /etc.
  "xyz.openbmc_project.Software.BMC.Updater" service under
  phosphor-bmc-code-mgmt will get the property under
  /xyz/openbmc_project/software/update_target and find the target on this
  config to select corresponding update path.
  E.g.,
```
  {
      "CPLD": {
          "server1": {
              "bus": 1,
              "addr": "0x44",
              "vendor": "lattice",
              "chip": "LCMXO3_4300C",
              "interface": "I2C"
          },
          "server2": {
              "bus": 2,
              "addr": "0x44",
              "vendor": "lattice",
              "chip": "LCMXO3_4300C",
              "interface": "I2C"
        },
      },
      "BIC": {
          "server1": {
              "bus": 1,
              "addr": "0x20",
              "vendor": "Aspeed",
              "chip": "AST1030",
              "interface": "I2C"
          },
          "server2": {
              "bus": 2,
              "addr": "0x20",
              "vendor": "Aspeed",
              "chip": "AST1030",
              "interface": "I2C"
          },
      }
  }
```
If we get '{"Target":"server1"}' from Redfish UpdateService API, the CPLD
firmware update path will be bus 1 address 0x44.

## Alternatives Considered
We have considered to modify the "Compatible" property from MANIFEST of image
to indicate which server board we want to update.
In this way, we will need to prepare multiple update packages for each server
board. And the same component on each server board could not share update
package. The cost of firmware update will be larger if we have more host.

## Impacts
The property of Redfish API /redfish/v1/UpdateService/ need to be extended.

## Testing
To verify this API works for multi-host server, we can update components on
each server, and get the firmware version after the update is done,
make sure the firmware is updated successfully.


