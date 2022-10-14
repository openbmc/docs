# Entity-Manager: Extend Blacklist Function

Author:
  Delphine CC Chiu <Delphine_CC_Chiu@wiwynn.com>

Created:
  2022-10-13

## Problem Description
Currently, the blacklist can only block certain bus from being scanned by fru-device.

If the eeprom and the device which we want to skip are on the same i2c bus. It is needed to skip certain address instead of a entire bus.

Therefore, we want to extend its function to block certain addresses on buses.


## Background and References
Our experiment suggested that the HSC might be hurt by the scanning.

## Requirements
None.

## Proposed Design
The blacklist.json will be like,

```json
{
    "buses": [
        1,
        3,
        {
            "bus": 5,
            "addresses": [
                "0xXX"
                "0xXX"
            ]
        }
    ]
}

bus:        i2c bus
addresses:  Device's addresses
```
Base on the configuration, the FruDevice should skip those addresses.
## Alternatives Considered
None.

## Impacts
None.

## Testing
Run FruDevice and see if the implementation work.
