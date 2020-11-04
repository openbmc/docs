# Software Update for Multiple Devices Support

Author: Priyatharshan P, [priyatharshanp@hcl.com]

Other contributors: Carlos J. Mazieri [carlos.mazieri@hcl.com]

Created: Nov 18 2020

Updated: Aug 25 2021

## Terms Used In This Document
| TERM | MEANING |
-------|-----------
| **firmware**  | synonym of *software* or a specific type of software|
| **image**     | synonym of *software*, a package ready to install|
| **device**    | hardware controlled by a BMC|
| **host**      | synonym of *device* |
| **A/B**       | both terms  **A** and **B** apply to the meaning|

## Problem Description
The current implementation in the phosphor-bmc-code-mgmt supports
firmware/image update for single device/host, and the image will be
deleted after the successful activation in this single device/host.

The new implementation should provide the support for anytime activation
of a software/firmware update which could be applicable for multiple
 devices/hosts.

## Requirements
The new software/firmware update implementation considers the
following requirements.
- Identify from BMC inventory software/firmware and devices/hosts
that can be updated.
- Loading single software/firmware image into multiple
devices/hosts as applicable.
- When there are multiple devices/hosts to be updated, that identification
list needs to be configured by the user or that list comes from the
MANIFEST file (see the section MANIFEST File Changes Proposal).
- The software/firmware image will be deleted after updating only if the
whole set of devices/hosts have been updated. It means, the image will not be
deleted if the image is applicable for four devices/hosts and the user has
updated three of four. In the other hand if these three devices/hosts list
comes from the MANIFEST file it is considered that the image is applicable
only  for these three devices/hosts and the image will be deleted after
updating them  (see the section MANIFEST File Changes Proposal).
- Anytime software/firmware update to be supported, it means, in case of
multiple devices/hosts the user can update one each time (not all once).

## Proposed Design
This document proposes a new design engaging a phosphor-bmc-code-mgmt to update
software/firmware for multiple devices/hosts in the system. Provided below the
implementation steps.

### BMC Inventory Management
The software manager will collect the devices/hosts list that can be updated
with the new software/firmware based on the software/firmware release. The
software/firmware includes but not limited to:
- BIOS
- CPLD
- VR
- BIC firmware
- ME firmware

After identifying the devices/hosts list and, one of the two approaches
details below could be used to trigger the firmware update into the targeted
device.

### Proposed Approach: Create objects that identify "ImageType" and "HostId"

This approach creates as many interfaces based on the inventory objects
identified by the software manager. The user can pick the list of devices/hosts
applicable for the image to be updated. The implementation steps are mentioned
below. All the software/firmware (BIOS, CPLD, VR, ME, etc) can be updated
through their respective interfaces easily through this method.

#### Create a new Update interface:
creating a new interface *xyz.openbmc_project.Software.FirmwareUpdate* through
phosphor-dbus-interfaces with the property “Update” as shown below to
notify the BMC to trigger the software update for the configured devices/hosts.
```
properties:
    - name: Update
      type: boolean
      default: false
      description: >
        The host identifier for software update.
    - name: State
      type: string
      flags:
        - readonly
      description: >
               The default is "None",
                  "OnGoing" when the update starts,
                  "Done" when the update finishes successful
```
When this 'Update' boolean flag is set to TRUE for a device/host, the
respective device/host will be included for software/firmware update.

#### New objects creation based on inventory
- New objects will be created under *xyz.openbmc_project.Software.BMC.Updater*
  for all the devices/hosts identified from inventory.
- The new  objects will be created with the interface
  *xyz.openbmc_project.Software.FirmwareUpdate*

## Proposed Design High Level Specification

1. The number of hosts will be identified from machine
layer [OBMC_HOST_INSTANCES], example:
    > $ bitbake -e obmc-phosphor-image | grep OBMC_HOST_INSTANCES

    |for 'yosemitev2' machine        |  for 'romulus' machine |
    |--------------------------------|-----------------------|
    |OBMC_HOST_INSTANCES="1 2 3 4"   | OBMC_HOST_INSTANCES="0"|


### Image Type detection (three possibilities in the following order)
- A new and specific field present in the MANIFEST file like "ImageType"
just identifies the image type
(see the section MANIFEST File Changes Proposal).
- The file name, the type of the image can be detected by the "image name"
or any other "file name" that is present in the image directory.
In both cases when the name contains an isolated sub-string such as
"BIOS","CPLD", "VR", "BIC" or "ME" (both upper and lower cases) just identifies
the image type.
Examples: "bios.bin" identifies "BIOS" image because the word "bios" is
isolated (followed by a *dot*), but  "biosxx.bin" does not identify any image
type. Besides *dots*, *underscores* and *minus signs* are also considered
delimiters in file names.
- File text content, an isolated word in uppercase present in a text file other
than MANIFEST such as "BIOS", "CPLD", "VR", "BIC" or "ME" just identifies the
image type.


### The code will be modified to create N number of objects based on the number
of hosts and the image type like below, for the multi-host examples below
consider that the inventory contains only images type of *"BIOS"* and
*"CPLD"* and hosts ids are: *"1"*, *"2"*, *"3"* and *"4"*:
- **single-host, "BIOS" image type detected**
    ```
        $ busctl tree xyz.openbmc_project.Software.BMC.Updater
        └─/xyz
            └─/xyz/openbmc_project
                └─/xyz/openbmc_project/software
                ├─/xyz/openbmc_project/software/1a56bff3        # Activation
                │ ├─/xyz/openbmc_project/software/1a56bff3/bios # Update
                └─/xyz/openbmc_project/software/f8515731        # BMC image
    ```
- **single-host, unknown image type**
    ```
        $ busctl tree xyz.openbmc_project.Software.BMC.Updater
        └─/xyz
            └─/xyz/openbmc_project
                └─/xyz/openbmc_project/software
                ├─/xyz/openbmc_project/software/1a56bff3
                │ ├─/xyz/openbmc_project/software/1a56bff3/bios
                │ ├─/xyz/openbmc_project/software/1a56bff3/cpld
                └─/xyz/openbmc_project/software/f8515731

    ```
- **multi-host, "CPLD" image type detected, field "TargetHosts"
not present in MANIFEST file**
    ```
        $ busctl tree xyz.openbmc_project.Software.BMC.Updater
        └─/xyz
            └─/xyz/openbmc_project
                └─/xyz/openbmc_project/software
                ├─/xyz/openbmc_project/software/1a56bff3
                │ └─/xyz/openbmc_project/software/1a56bff3/cpld
                │   ├─/xyz/openbmc_project/software/1a56bff3/cpld/1
                │   ├─/xyz/openbmc_project/software/1a56bff3/cpld/2
                │   └─/xyz/openbmc_project/software/1a56bff3/cpld/3
                │   └─/xyz/openbmc_project/software/1a56bff3/cpld/4
                └─/xyz/openbmc_project/software/f8515731
    ```
- **multi-host machine, unknown image type,
field "TargetHosts" not present in MANIFEST file**
    ```
        $ busctl tree xyz.openbmc_project.Software.BMC.Updater
        └─/xyz
            └─/xyz/openbmc_project
                └─/xyz/openbmc_project/software
                ├─/xyz/openbmc_project/software/1a56bff3
                │ ├─/xyz/openbmc_project/software/1a56bff3/bios
                │ │ ├─/xyz/openbmc_project/software/1a56bff3/bios/1
                │ │ ├─/xyz/openbmc_project/software/1a56bff3/bios/2
                │ │ └─/xyz/openbmc_project/software/1a56bff3/bios/3
                │ │ └─/xyz/openbmc_project/software/1a56bff3/bios/4
                │ ├─/xyz/openbmc_project/software/1a56bff3/cpld
                │ │ ├─/xyz/openbmc_project/software/1a56bff3/cpld/1
                │ │ ├─/xyz/openbmc_project/software/1a56bff3/cpld/2
                │ │ └─/xyz/openbmc_project/software/1a56bff3/cpld/3
                │   └─/xyz/openbmc_project/software/1a56bff3/cpld/4
                └─/xyz/openbmc_project/software/f8515731
    ```
- **multi-host, unknown image type, field "TargetHosts=1 3" present in MANIFEST
file**
    ```
        $ busctl tree xyz.openbmc_project.Software.BMC.Updater
        └─/xyz
            └─/xyz/openbmc_project
                └─/xyz/openbmc_project/software
                ├─/xyz/openbmc_project/software/1a56bff3
                │ ├─/xyz/openbmc_project/software/1a56bff3/bios
                │ │ ├─/xyz/openbmc_project/software/1a56bff3/bios/1
                │ │ └─/xyz/openbmc_project/software/1a56bff3/bios/3
                │ ├─/xyz/openbmc_project/software/1a56bff3/cpld
                │ │ ├─/xyz/openbmc_project/software/1a56bff3/cpld/1
                │ │ └─/xyz/openbmc_project/software/1a56bff3/cpld/3
                └─/xyz/openbmc_project/software/f8515731
    ```
- **multi-host, "BIOS" image type detected, considering the field
"TargetHosts=4" present in MANIFEST file**
  ```
        $ busctl tree xyz.openbmc_project.Software.BMC.Updater
        └─/xyz
            └─/xyz/openbmc_project
                └─/xyz/openbmc_project/software
                ├─/xyz/openbmc_project/software/1a56bff3
                │ ├─/xyz/openbmc_project/software/1a56bff3/bios
                │ │ ├─/xyz/openbmc_project/software/1a56bff3/bios/4
                └─/xyz/openbmc_project/software/f8515731
    ```

### Service Files
The current software version that supports flashing "BIOS" firmware and
defines the Service File as "obmc-flash-host-bios@.service" which receives
the "image id" as parameter.
Having a generic devices/hosts list and supported image types coming from the
inventory requires a unique Service File for all image types such as
"obmc-flash-host-software@.service".
The parameter brings all the information in the
form: **ImageID-ImageType[-HostID]**, where the "HostID" part will be present
only for multi-host machines.
- Service File and calls example using the
ImageID "1a56bff3", the ImageType "BIOS" and the HostID "1":

    | Type | Service File|
    |:-----| :----|
    | name| obmc-flash-host-software@.service |
    |calls single-host|obmc-flash-host-software@1a56bff3-bios.service|
    |calls mulit-host|obmc-flash-host-software@1a56bff3-bios-1.service|
- Service File example
    ```
    [Unit]
    Description=host service flash, the %i can be in the form \\
                     imageid-imagetype-hostid (minus sign between them)

    [Service]
    Type=oneshot
    RemainAfterExit=no

    ExecStartPre=/bin/sh -c "image=`/bin/echo %i | /usr/bin/awk  \
                  -F'-' '{print $1}'` && \
                  /bin/systemctl set-environment IMAGEID=$image"

    ExecStartPre=/bin/sh -c "type=`/bin/echo  %i | /usr/bin/awk  \
                  -F'-' '{print $2}'` && \
                  /bin/systemctl set-environment IMAGETYPE=$type"

    ExecStartPre=/bin/sh -c "host=`/bin/echo  %i | /usr/bin/awk  \
                 -F'-' '{print $3}'` && \
                 /bin/systemctl set-environment HOSTID=$host"


    ## Note: single-host machines will bring empty HOSTID variable

    ExecStart=[flash tool here]  /tmp/images/${IMAGEID} ${IMAGETYPE} ${HOSTID}
    ```

### Firmware Update Procedure
Firmware update will be performed by the following process:

- Copy the software/firmware image to "/tmp/images" directory.
- Set "Update" property to "true" for the devices/hosts that
need software/firmware update according to the image type, these step is not
necessary for single-host machines with image type detected nor for multi-host
machines when the field "TargetHosts" is present in MANIFEST file.
- Activate the image "Activation" flag to start the image update, it
will perform the update for all selected devices/hosts in parallel mode.
- After successful activation the image may be deleted if the requirements
match (see Image Cleaning section).

### Image Type confirmation against a such host
Before starting the update, the image type must confirmed against the target
host to verify if that host really accepts the image.
The image itself against its detected type must be also confirmed if there is
some available method such as "checksum", "magic number" among others, this can
be performed when attempting to detect the image type or before starting the
software/firmware update.


### Image Cleaning for multi-host machines
In the existing system, the image will be deleted after the successful
software/firmware update.

But in this new approach, the image has to be deleted only
if all the required devices/hosts have the "State" property as "Done", the
following cases are possible:
1. The field "TargetHosts" is present in the MANIFEST file (in this case the
user just activates the image).
2. The field "TargetHosts" is NOT present in the MANIFEST file, the user
performs the update in many steps, but completes all required devices/hosts.
3.  The field "TargetHosts" is NOT present in the MANIFEST file, the user
performs the update a single step after identifying all required hosts to
receive the update.

In any other case not listed above the use has the responsibility of removing
the image manually.

The image delete logic in in both cases deleted by the system or deleted by the
user will result on the software objects path removal and software state
cleaning.


### Impacts
- Current software manager creates an
object */xyz/openbmc_project/software/bios_active* which can be removed.
- The current service file "obmc-flash-host-bios@.service" will be renamed
to "obmc-flash-host-software@.service".

### MANIFEST File Changes Proposal
1. To reduce the risk of image type detection the field "ImageType" could
be added as optional in the MANIFEST file.
2. Multi-host machines could add the optional field "TargetHosts" to
clear say which set of devices/hosts the flashing will be applied.

Example:

   >
   > ImageType=BIOS
   >
   > TargetHosts=hostId-1 manufacturerId-x ...

**Note:** Whatever content present in "TargetHosts" will be checked against the
inventory to match a host id.

## Testing
meta-yosemitev2 is a multi-host system [4 hosts].

For Image Detection, Interfaces and Objects handling an Automated Test can
be added into "utest" application.

This feature will be tested in meta-yosemitev2 platform.
