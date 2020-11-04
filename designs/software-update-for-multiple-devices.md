# Software Update for Multiple Devices Support

Author: Priyatharshan P, [priyatharshanp@hcl.com]

Other contributors: None

Created: Nov 18,2020

## Problem Description
The current implementation in the phosphor-bmc-code-mgmt supports firmware
update for single device, and the image will be deleted after the successful
activation in single device.

The new implementation should provide the support for anytime activation
of a software update which could be applicable for multiple devices.

## Requirements
The new firmware update implementation considers the following requirements.
- Identify software/firmware updatable BMC devices from inventory
- Loading single firmware image into multiple devices as applicable.
- User to configure the list of devices needed for an upgrade.
- The firmware image to be deleted only after updating all the devices
  configured for upgrade.
- Anytime software/firmware update to be supported

## Proposed Design
This document proposes a new design engaging a phosphor-bmc-code-mgmt to update
firmware image for multiple devices in the system. Provided below the
implementation steps.

### Inventory Management
The software manager will collect the devices list that can be updated with the
new firmware based on the firmware release. The firmware/software includes but
not limited to
- BIOS
- CPLD
- VR
- BIC firmware
- ME firmware

Provided below the implementation steps which identifies the required device
list from the inventory.
- The method "GetSubTreePaths" from Object mapper will be used to get all the
  device objects available from the inventory.
- The objects will be filtered by the property "Class" having "fw_version".
- By this way, the software manager will identify all the available devices
  eligible for firmware update.

After identifying the devices list, one of the two approaches details below
could be used to trigger the firmware update into the targeted device.

### Approach 1: Use of ExtendedVersion
This method will use ExtendedVersion field from the MANIFEST file which needs to
be created for the binary release for each firmware/software. This could be used
to update the firmware as mentioned in the steps below
- There are many devices like CPLD, BIC, ME, VR, etc are present in the system
  that requires an firmware update. The actual device that needs firmware update
  will be identified from the binary name present in the /tmp/images folder (for
  example: bios.bin).
- There might be N number of similar devices available in the system and
  all those devices cannot have firmware update because each might serve a
  different purpose. ExtendedVersion from MANIFEST file will be matched with the
  number of same devices identified from inventory.
- All the devices which matches the ExtendedVersion will be updated with the
  firmware image given.
- After successful activation of all the devices the image will be deleted.

### Approach 2: Create objects for all the devices identified from inventory
This approach uses creating as many interfaces based on the inventory objects
identified by the software manager. The user can pick the list of devices
applicable for the image to be updated. The implementation steps are mentioned
below. All the firmwares (BIOS, CPLD, VR, ME, etc) can be updated through their
respective interfaces easily through this method.

#### Create a new Update interface:
creating a new interface (xyz.openbmc_project.Software.Update) through
phosphor-dbus-interfaces with the property “Update” as shown below to
notify the BMC to trigger the software update for the configured devices.
```
properties:
    - name: Update
      type: boolean
      default: false
      description: >
        The host identifier for software update.
```
When this boolean flag is set to TRUE for a device, the respective device will
be included for firmware update.

#### New objects creation based on inventory
- New objects will be created under xyz.openbmc_project.Software.BMC.Updater
  for all the devices identified from inventory.
- The new  objects will be created with the interface
  xyz.openbmc_project.Software.Update

#### Firmware Update
Firmware update will be done by following process.

- Copy the Firmware Image to /tmp/images directory
- set Update property to true for the devices which needs firmware update.
- Firmware image will be updated for all devices which has Update property
'true'.
- After successful activation the image will be deleted.

#### Firmware image clean up
In the existing system, the firmware-image will be deleted after the successful
firmware update. But in this new approach, the image has to be deleted only
after successfully updating all the devices having "Update" property set.

The image delete logic in the existing will be updated to account “Update”
property of all devices. When this flag is false for all the
configured devices, the image will be deleted at the end.

#### Property reset in Update interface
After successful completion, the property "Update" will  be reset back to
default value for all the devices.

## Alternatives Considered

### Design 1: Multi Interface Approach

1. Number of host will be identified from machine layer [OBMC_HOST_INSTANCES]

2. Code will be modified to create n number of objects based on number of
hosts like below

```
      |-/xyz/openbmc_project/software/host1
      | `-/xyz/openbmc_project/software/host1/28bd62d9
      |-/xyz/openbmc_project/software/host2
      | `-/xyz/openbmc_project/software/host2/28bd62d9
      |-/xyz/openbmc_project/software/host3
      | `-/xyz/openbmc_project/software/host3/28bd62d9
      `-/xyz/openbmc_project/software/host4
        `-/xyz/openbmc_project/software/host4/28bd62d9
```

3. This will create activation interface for each host. For a multi-host
system if the  RequestedActivation is set to
 "xyz.openbmc_project.Software.Activation.RequestedActivations.Active",
then different BIOS service file will be called based the host.

For single host :
biosServiceFile = "obmc-flash-host-bios@" + versionId + ".service";
For multi host  :
biosServiceFile =  "obmc-flash-host" + host + "-bios@" + versionId +
".service";

Then it can be used for multi host even if the firmware image we want to
install is the same for multiple host targets.

#### Disadvantages

Even after the successful activation of software update, the image will not be
deleted from the BMC.We dont want to potentially fill up the /tmp space.

### Design 2 : MANIFEST File Changes Approach

Host id will be added as an array of host id in extra version field in the
MANIFEST file.Format can be defined, this way as soon as we see software image
uploaded and based on extra version, it will create as many version
interface and allow to activate them. Once we call all the software upgrade
script, we should delete image.

#### Disadvantages

The MANIFEST file is an factory ship-out, it may not be appropriate to add
the host IDs into it.

## Impacts
None

## Testing
meta-yosemitev2 is a multi-host system [4 host]. This feature will be
tested in meta-yosemitev2 platform.
