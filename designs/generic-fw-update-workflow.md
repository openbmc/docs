# Generic Firmware Update Workflow

Author: Willy Tu <wltu!>

Other contributors: Brandon Kim <brandonk!>,  William Kennington <wak!>

Created: 2022-07-10

## Problem Description

The goal is to create a common workflow to update any firmware from the
BMC and providing the flexibility for each system to configure the
specific implementation. We want to provide this workflow to close the
diff for firmware updates method used between the company and vendors.
The goal is not to discuss a specific implementation of flashing
or verifying the image, but to provide a platform for people to
share/improve a common workflow and improve the useability among
different users to limit duplicate implementations.


## Background and References

The Google BMC have a internal implementation of BIOS update that is
not available to our vendor. This create a gap between the BMCs and
prevent the vendors from helping us to validate the change as the
system is being brought up. This issue is not limited to only Google or
just BIOS update. We will need a common workflow to support most
firmware updates by the BMC to make it simple to support new firmware
as needed.

This document is focusing on the update workflow and not any
specific implementation. It use the following dependencies as an
initial example:
- [Flasher Tool](Insert design doc here) for writing the firmware
- [openbmc/google-misc/libcr51sign](https://github.com/openbmc/google-misc/tree/master/subprojects/libcr51sign)
  for firmware validation.

Some related open sourced work includes [fwupd](https://github.com/fwupd/fwupd)
that provides a rich support for different firmware updates. Fwupd can
be used as part of the workflow, but does not replace the workflow as
a whole.


## Requirements

The main requirement for the design is that the workflow is being
flexible on implementation for read/writing and verifying the image.
These will also need to be configurable to select the method to use.

- Read/Write

The tool shall be able to support read/write any device on the system
to support the firmware update workflow. As a basic example,
reading/writing a SPI flash for BIOS update.

- Validation

The image shall support different methods of validation for the different
supported firmware . As an example, the CR51 descriptor is used to verify that
the image has not been tamper with.

- Configuration

The workflow much support flexible configuration to select the different
read/write and validation implementations for all supported firmware.

- Metadata

The system must have a persistent storage to save the update workflow
metadata per firmware. This will be used to track the state of the
update for optimization.

## Proposed Design

(2-5 paragraphs) A short and sweet overview of your implementation ideas. If

you have alternative solutions to a problem, list them concisely in a bullet

list. This should not contain every detail of your implementation, and do

not include code. Use a diagram when necessary. Cover major structural

elements in a very succinct manner. Which technologies will you use? What

new components will you write? What technologies will you use to write them?

-----

At a high level, the update workflow will be split into three sections.
- Stage
- Activate
- Update

The main goal of the workflow is to provide flexibility in the
implementation while having a common structure.

This doc will not discuss how the firmware image get moved to the BMC.
It can be with `phosphor-ipmi-flash` or just over the network.


### Stage
```
  +-----------------+  Failed to verify image
  |  Initial State  |--------------------|
  +-----------------+                    |
          | Verified Image               |
          V                              |
  +---------------+   Stage Image Failed |
  |   Verified    |----------------------|
  +---------------+                      |
          | Staged Image                 |
          V                              |
  +---------------+ Update Metadata      |
  | Image Staged  |----------------------|
  +---------------+     Failed           |
          | Updated Metadata             |
          V                              V
 +--------------------+         +-------------------+
 | Final Staged State |         |  Exit with error  |
 +--------------------+         +-------------------+
```

The Stage workflow focus on getting verifying the image before we
attempt to update it. The image could be staged on RAM or some
persistent storage to be activated later. The metadata is updated to
prevent block teh same image from being staged again.

### Activate
```
  +-----------------+  Image note staged
  |  Initial State  |--------------------|
  +-----------------+                    |
          | Image Staged                 | Failed
          V                              |
  +---------------+   Failed to mark as  |
  |    Staged     |----------------------|
  +---------------+      Activated       |
          | Marked as Activated          |
          V                              |
   +-------------+           +-------------------+
   |  Activated  |           |  Exit with error  |
   +-------------+           +-------------------+
```

The Activate workflow just check that the image we want for update is
staged and mark it as ready for the Update workflow. Nothing happen if
the staged image we want to update is not staged. Each system will have
to trigger a stage again if needed.

### Update
```
  +-----------------+  Image not activated
  |  Initial State  |------------------------------------|
  +-----------------+                                    |
          | Image activated                              |
          V                                              |
  +----------------+    Failed to read                   |
  |   Activated    |-------------------------------------|
  +----------------+    Staged image                     |
          | Read Stage Image                             |
          V                                              |
  +--------------------+ Failed to process               |
  | Staged Image Ready |---------------------------------|
  +--------------------+    Staged Image                 |
          | Process Staged Image                         |
          |                                              |
          V                                              |
+-----------------------+  Image not ready               |
| Processed Image Ready | -----------------|             |
+-----------------------+    for update    |             |
          | Image ready for update         |             |
          |                  +-------------------------+ |
          |                  |Unexpected Update Request| |
          |                  +-------------------------+ |
+------------------------+          | Cleanup metadata   |
| Image Ready for Update |-------------------------------|
+------------------------+   Failed to update the image  |
          | Updated image                                |
          | and updated Metadata                         |
          V                                              |
+------------------------+                       +--------------+
| Image Ready for Update |                       |  Powercycle  |
+------------------------+                       +--------------+
```

The Update workflow will focus on actually getting the image installed.
The assumption here is that we will always need to powercycle the
system after the update, so the update workflow will be triggered
after the powercycle command it triggered.

In the Update workflow, it will verify the staged image again before
processing it based on each system's need. As an example for BIOS, we
might need to inject the persistent regions from the running BIOS to
the installing image. With the processed image, we will verify to make
sure that the image is what we expected to be installing. If not, then
it will clear the metadata to reset the entire workflow (This is to
make sure nothing gets tamper with during the process. Set state to
CORRUPTED). If it is ready, it will attempt to write the image and
update the metadata accordingly. On any failures, it will exit workflow
and powercycle the system.

#### Retries (Optional)

The Retires workflow is good to have and just call powercycle again if
the current state is in ACTIVATED instead of UPDATED. This is checked
during the system boot up script and the assumption is that we will
never hit ACTIVATED state at boot up since powercycle (Update workflow)
will either reset it or set to UPDATED after successfully workflow

#### Others

The stage, activate, update workflows will be the core workflows, however,
the information on the workflows will be available for each system to
customize the overall workflow.

For example.
- Staged version
- Activated version
- Verify Staged Image
- Verify Activated Image
- Update Version
- Update image status
  - CORRUPTED
  - STAGED
  - ACTIVATED
  - UPDATED
  - UNKNOWN
- Overall Info

#### Configuration

The configuration should be flexible to support different
implementations. This is out scope for this doc, but here is an
example.

Example config,
```json
{
  "flash": {
    "validation_key": [
      "/usr/share/bios-key/secure.pem"
    ],
    "primary": {
      // The `mtd` indicate it is a flash memory and will select the
      // appropriate implementation for the read/write
      "location": "mtd,bios-primary",
      "mux_select": 169
    },
    "secondary": [
      {
        // The `fake` indicate just a file trying to be a flash memory.
        "location": "fake,type=nor,erase=33554432,/tmp/image-bios-staged",
        "mux_select": null
      },
      {
        "location": "mtd,bios-2-primary",
        "mux_select": null
      }
    ],
    "device_id": "xxx.spi",
    "driver": "/sys/bus/platform/drivers/SPI"
  },
  "validation":{
    "type": "cr51",
    // The metadata is dependent on the type of validation used.
    "metadata": {
      "path": "fake,type=simple,erase=0,/sys/class/i2c-adapter/i2c-X/X-0050/eeprom",
      "offset": 0
    },
    // Cr51 Validator specific config
    "prod_to_dev": false,
    "production_mode": true,
    "unsigned_to_dev": false,
    "image_family": 18
  }
}
```


## Alternatives Considered

WIP

Not sure on this yet.

## Impacts


WIP

API impact?
- N/A

Security impact?
- N/A

Documentation impact?
- N/A

Performance impact?
- The performance impact will depends on each read/write/verify
implementation. In general, only the Stage workflow will block while
the host is up. The Activate workflow is quick and Update workflow
happens when the host is going for powercycle.

Developer impact?
- This should benefits developers on enabling firmware updates from
the BMC with a common workflow.

Upgradability impact?
- N/A



### Organizational

Does this repository require a new repository? (Yes, No)

Probably yes.

Who will be the initial maintainer(s) of this repository?

Probably wak and wltu


## Testing

How will this be tested? How will this feature impact CI testing?

This will be tested with a physical machine with BIOS update workflow
on Google BMC. The feature will not affect CI testing.
