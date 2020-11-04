# Software Update for Multiple Devices Support

Author: Kumar Thangavel, [thangavel.k@hcl.com]

Other contributors: Carlos J. Mazieri [carlos.mazieri@hcl.com]

Created: Nov 18 2020

Updated: Jul 26 2022

## Terms Used In This Document

- **firmware**  - synonym of *software* or a specific type of software
- **image**     - synonym of *software*, a package ready to install
- **device**    - hardware controlled by a BMC
- **host**      - synonym of *device*
- **A/B**       - both terms  **A** and **B** apply to the meaning

## Problem Description
The current implementation in the phosphor-bmc-code-mgmt supports firmware/image
update for single device/host, and the image will be deleted after the
successful activation in this single device/host.

The new implementation should provide the support for anytime activation of a
software/firmware update which could be applicable for multiple devices/hosts.

## Requirements
The new software/firmware update implementation considers the following
requirements.
- ItemUpdater will Identify software/firmware and devices/hosts from BMC
  Inventory that can be updated and stored in the interface
xyz.openbmc_project.Inventory.Decorator.Compatible under corresponding dbus
object of the images. This happens in within the system.
- Loading single software/firmware image into multiple devices/hosts as
  applicable.
- The software/firmware image will be deleted after updating only if the whole
  set of devices/hosts have been updated. It means, the image will not be
deleted if the image is applicable for four devices/hosts and the user has
updated three of four.
- If update gets failed in between or partially applied, then update will not
  considered as success. Images will be deleted only if all the updates success.
- Single image update applied for single device as well as multiple devices.if
  any of multiple device update is not success, then image will not be deleted.

## Proposed Design
This document proposes a new design engaging a phosphor-bmc-code-mgmt to update
software/firmware for multiple devices/hosts in the system. Provided below are
the implementation steps.

### Software update details from the BMC inventory
The ItemUpdater collects the software update details like devices/hosts list
from BMC inventory. ItemUpdater collects type, sub type of Firmware image and
versions information also.

The ItemUpdater also collects the list of "Image Type" each device/host can be
assigned to.

The software/firmware "Image Type" include but not limited to:

- BIOS or BIOS_XXX
- CPLD
- VR
- BIC firmware

Similarly, the host details for software updates are retrieved from inventory.
ItemUpdater supports all the "Image Type" as like BMC update.

### Proposal: A new *ItemUpdater* with "ImageType" and "HostId" identification

Another new single *ItemUpdater* instance like "phosphor-bmc-image-updater" and
"phosphor-host-image-updater" will be created and specific for device/hosts
software update will be in place in the software manager.

The implementation steps are mentioned below. All the software/firmware (BIOS,
CPLD, VR, etc) can be updated through their respective interfaces easily through
this method. This new instance is generic for all image types.

#### New objects creation based on inventory
- Each new dbus service *xyz.openbmc_project.Software.Device.Updater* will have
  individual deamons. Example, BMC Updater or Host Updater. New objects will be
created under the dbus service *xyz.openbmc_project.Software.Device.Updater* for
all the devices/hosts identified from inventory. Each object will have the
*Activation* interface to perform the software update.

## Proposed High Level Design

1. The number of hosts will be identified from machine layer
[OBMC_HOST_INSTANCES], example:
    > $ bitbake -e obmc-phosphor-image | grep OBMC_HOST_INSTANCES

    |for 'yosemitev2' machine        |  for 'romulus' machine |
    |--------------------------------|------------------------|
    |OBMC_HOST_INSTANCES="1 2 3 4"   | OBMC_HOST_INSTANCES="0"|


### Image Type detection
The proposed solution sets as **mandatory** the *"Image Type"* detection, in
case Image Type detection cannot be done, the software update will not be
performed.

There are two possibilities of the identification in the following order:
- A new and specific field "Compatible" present in the MANIFEST file, It just
identifies the image type. This Compatible follows the compatible string
format with sub-string such as "BIOS", "CPLD". The example format has been added
in MANIFEST file.  (see the section MANIFEST File Changes Proposal).

- The type of the image can be detected by the compatibility string format in
Decorator.Compatible interface under dbus object of each image. It contains an
isolated sub-string such as "BIOS", CPLD", "VR" or "BIC"
(both upper and lower cases) just identifies the image type.

Isolated sub-string is BIOS, CPLD, BIC etc after the filename with '.' The
format of the image.

The Compatible string has to be parsed and compared the each element separated
by '.'. Element is TwinLake and Type is BMC or host. Type and revision needs to
be compared with exact match.

Examples: "bios.bin" identifies "BIOS" image because the word "bios" is isolated
(followed by a *dot*), but  "biosxx.bin" does not identify any image type.
Besides *dots*, *underscores* and *hyphen* are also considered delimiters in
file names.
- File text content, an isolated word in uppercase present in a text file other
  than MANIFEST such as "BIOS", "CPLD", "VR", "BIC" just identifies the image
type.

### Image Type association with devices/hosts
It is also **mandatory** that the devices/hosts accept the detected *"Image
Type"*.  The inventory and/or the entity manager will provide information about
what are the "Image Type" accepted the devices/hosts, in case they do not match
the software update will not be performed for that device/hosts.

### Objects creation based on Hosts and Image Type
The code will create N number of objects based on hosts and the image type
considering the *"Image Type association with devices/hosts"* explained before,
for the examples below consider that the inventory contains only images type of
*"BIOS"* and *"CPLD"* and hosts ids are: *"1"*, *"2"*, *"3"* and *"4"*:
- **single-host, "BIOS" image type detected**

```
$ busctl tree xyz.openbmc_project.Software.Device.Updater
└─/xyz
    └─/xyz/openbmc_project
        └─/xyz/openbmc_project/software
        ├─/xyz/openbmc_project/software/1a56bff3
        └─/xyz/openbmc_project/software/1a56bff3/bios  # Activation

```
- **multi-host, "BIOS" image type detected**

```
$ busctl tree xyz.openbmc_project.Software.Device.Updater
└─/xyz
    └─/xyz/openbmc_project
        └─/xyz/openbmc_project/software
        ├─/xyz/openbmc_project/software/1a56bff3
        |   ├─/xyz/openbmc_project/software/1a56bff3/bios_1 # Activation
        |   ├─/xyz/openbmc_project/software/1a56bff3/bios_2 # Activation
        |   └─/xyz/openbmc_project/software/1a56bff3/bios_3 # Activation
        └─────/xyz/openbmc_project/software/1a56bff3/bios_4 # Activation
```

### Service Files
The current software version that supports flashing "BIOS" firmware and defines
the Service File as "obmc-flash-bios@.service" which receives the "image id" as
parameter.  Having a generic devices/hosts list and supported image types coming
from the inventory requires a unique Service File for all image types such as
"obmc-flash-software@.service".

The parameter brings all the information in the form:
**ImageID-ImageType[-HostID]**, where the "HostID" part will be present only
for multi-host machines.

- Service File and calls example using the ImageID "1a56bff3", the ImageType
  "BIOS" and the HostID "1":

- **name**              - obmc-flash-software@.service
- **calls single-host** - obmc-flash-software@1a56bff3-bios.service
- **calls multi-host**  - obmc-flash-software@1a56bff3-bios-1.service

- Service File example
```
    [Unit]
    Description=host service flash, the %i can be in the form \\
                   imageid-imagetype-hostid

    [Service]
    Type=exec
    RemainAfterExit=no

    ExecStart=[flash tool here]  %i
```
Different BIOS update is possible with differnt hardware using compatible string
format using type with EVT/DVT/PVT.

Example : If different BIOS is required for two hardware revisions, we can use
this compatible string and check the type as Host.Bios and revisions as
EVT/DVT/PVT. If the EVT/DVT/PVT has revisions, we can have EVT1, EVT2, etc,. We
compare with exact string match with type and Revision.

com.facebook.Hardware.YosemiteV2.TwinLake.EVT.BIOS
com.facebook.Hardware.YosemiteV2.TwinLake.EVT1.BIOS
com.facebook.Hardware.YosemiteV2.TwinLake.EVT2.BIOS
com.facebook.Hardware.YosemiteV2.TwinLake.DVT.BIOS
com.facebook.Hardware.YosemiteV2.TwinLake.PVT.BIOS

With Different Hardware Revisions, we can handle different BIOS methods.
Revision number and Type should vary between the different BIOS update methods.

Images can be differentiated and identified by compatible string format.

All BIOS update methods are handling using Ipmb commands in Bridge IC.  So,BIC
accepts only valid format of data for firmware update commands for different
cards. The command data is different for each card.

### Software Update Procedure
Software update will be performed by the following process:

- Copy the software/firmware image to "/tmp/images" directory.
- Activate the "Activation" flag on a single device/host to start the image
  update. In case of multi-host machines, another "Activation" flag can be
activated performing in this case parallel update.
- After successful activation the image may be deleted if the requirements match
  (see Image Cleaning section).

### Software Update Progress
Software update progress can be monitored using Progress D-bus property under
ActivationProgress interface in xyz.openbmc_project.Software.Version
dbus-service.

Progress : This dbus property is used to get the software update progress or
completion percentage. This is integer type Dbus-property which have completed
percentage.

Software update progress can be calculated using systemd service states like
active, running, done, etc,. The custom flash tool can be invoked by systemd
service and based on these service states the completion percentage will be
updated in the progress dbus property.

This ActivationProgress interface is enabled only when the activation of the
image starts. This ActivationProgress is disabled only when the activation of
the image is done.

```
xyz.openbmc_project.Software.ActivationProgress interface -  -     -
.Progress                              property  d  50    emits-change writable

```

If the Systemd service which invokes the flash tool encounters an error or the
update fails, the status of the service will also report failed or inactive
status. Based on these systemd service states, ItemUpdater can get the error
status.

Based on the completed progress percentage, two image updates on same device can
be avoided. ItemUpdater can activate the images only if that completed progress
is 0 or 100.

### Image Cleaning for multi-host machines
In the existing system, the image will be deleted after the successful
software/firmware update.

But in this new approach, The software update is performed in all the required
devices/hosts identified by the ItemUpdater and Activation flag, then the image
is removed from disk if D-bus property "AutoDelete" flag is enabled.

"AutoDelete" flag option is added to delete only for required device/hosts as
per compatible image types if this "AutoDelete" flag is enabled and user not
decided to do furthur updates. If this flag is not enabled, user has to remove
the image manually with delete dbus call.

For example : Four hosts are compatible with this image and user wants only two
hosts to be updated now and two hosts later. In this case, image delete should
not be done. So AutoDelete flag is used to handle this case.

New D-bus property "AutoDelete" can be added under
"xyz.openbmc_project.Object.Delete" interface for removing the images from disk
once the image update is completed. If "AutoDelete" is disabled the image will
not be removed from the disk. User has to remove the image manually.

The image delete logic in both cases deleted by the system or deleted by the
user will result on the software objects path removal and software state
cleaning.

This image delete logic is used for unused or un-applied images.

``` xyz.openbmc_project.Object.Delete  interface -  -    -
    .Delete                            method    -  -    -
    .AutoDelete                        property  b  true emits-change writable
```

If "AutoDelete" flag is set as "no" then existing image will not be deleted. If
we try to copy new image for FW update, then BMC may run out of storage. To
prevent this out of storage condition, the BMC will not allow the upload of
another firmware update if the "AutoDelete" flag is set to "no". Firmware update
will only be allowed to proceed if the "AutoDelete" flag is set to "yes".

#### Using Redfish
Redfish DMTF standard (DSP2062_1.0.0) supports for firmware update of devices
like BMC, BIOS, CPLD, etc. User can specify the host which needs to be updated
using Redfish. The device list can be generated using BMC Inventory.

Following commands could possibly be considered for the redfish support.

To configure the host for bios update, For Host1:
```
    property : system1 path :
    https://${bmc}/redfish/v1/UpdateService/FirmwareInventory/system1/BIOS.

```
For Host3:
```
    property : system3 path :
    https://${bmc}/redfish/v1/UpdateService/FirmwareInventory/system3/BIOS.

```
To update the bios image, UpdateService type can be used with image file path.

### Impacts
- Current software manager creates an object
  */xyz/openbmc_project/software/bios_active* which can be removed.
- The current service file "obmc-flash-host-bios@.service" will be renamed to
  "obmc-flash-software@.service".

### MANIFEST File Changes Proposal
To reduce the risk of image type detection the field "Compatible" could be added
as optional in the MANIFEST file.

Example:

   >
   > Compatible=com.facebook.Software.Element.TwinLake.Type.Host.SubType.Bios.
Rev
   >
   >

## Testing
Yosemitev2 is a multi-host system [4 hosts].

Manual test - The single image can be flashed into single devices and multiple
devices of BIC, CPLD, VR, BIOS. Once image is flashed on device, Activation has
been done and image update will be started. Once update is done, will verify and
validate new firmware and versions.

Unit test - This test is added for Image Type Detection, Interfaces and Objects
handling an Automated Test. Those can be added into "utest" application.

Regression test - Software update will be tested with multiple times with loops
for single device and multiple devices.

Refish test - Software update will be tested with Redfish curl commands.

This feature will be tested in meta-yosemitev2 platform.
