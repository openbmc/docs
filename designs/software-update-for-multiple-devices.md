# Software Update for Multiple Devices Support

Author: Priyatharshan P, [priyatharshanp@hcl.com]

Other contributors: Carlos J. Mazieri [carlos.mazieri@hcl.com]

Created: Nov 18 2020

Updated: Nov 12 2021

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


## Proposed Design
This document proposes a new design engaging a phosphor-bmc-code-mgmt to update
software/firmware for multiple devices/hosts in the system. Provided below the
implementation steps.

### BMC Inventory Management
The software manager will collect the devices/hosts list that can be updated
with the new software/firmware based on the software/firmware release. The
software/firmware "Image Type" include but not limited to:
- BIOS
- CPLD
- VR
- BIC firmware
- ME firmware

The software manager will collect also the list of "Image Type" each
device/host can be assigned to.

### Proposal: A new *ItemUpdater* with "ImageType" and "HostId" identification

Another *ItemUpdater* instance specific for device/hosts software update will
be in place in the software manager. The implementation steps are mentioned
below. All the software/firmware (BIOS, CPLD, VR, ME, etc) can be updated
through their respective interfaces easily through this method.


#### New objects creation based on inventory
- New objects will be created under *xyz.openbmc_project.Software.HOST.Updater*
  for all the devices/hosts identified from inventory. Each object will have
the *Activation* interface to easily perform the software update.

## Proposed Design High Level Specification

1. The number of hosts will be identified from machine
layer [OBMC_HOST_INSTANCES], example:
    > $ bitbake -e obmc-phosphor-image | grep OBMC_HOST_INSTANCES

    |for 'yosemitev2' machine        |  for 'romulus' machine |
    |--------------------------------|-----------------------|
    |OBMC_HOST_INSTANCES="1 2 3 4"   | OBMC_HOST_INSTANCES="0"|


### Image Type detection
The proposed solution sets as **mandatory** the *"Image Type"* detection, in
case it cannot be done, the update will not be performed.
There are three possibilities of the identification in the following order:
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

#### Binary Image Type confirmation
Once the image type is detected and for that exists a more robust
detection method such as "checksum", "magic number" among others, it must be
used.

### Image Type association with devices/hosts
It is also **mandatory** that the devices/hosts accept the detected
*"Image Type"*.
The inventory and/or the entity manager will provide information about what are
the "Image Type" accepted the devices/hosts, in case they do not match
the software update will not be performed.

### Objects creation based on Hosts and Image Type
The code will create N number of objects based on hosts and the image type
considering the *"Image Type association with devices/hosts"* explained before,
for the examples below consider that the inventory contains only images type of
*"BIOS"* and *"CPLD"* and hosts ids are: *"1"*, *"2"*, *"3"* and *"4"*:
- **single-host, "BIOS" image type detected**
    ```
        $ busctl tree xyz.openbmc_project.Software.HOST.Updater
        └─/xyz
            └─/xyz/openbmc_project
                └─/xyz/openbmc_project/software
                ├─/xyz/openbmc_project/software/1a56bff3
                └─/xyz/openbmc_project/software/1a56bff3/bios  # Activation

    ```
- **multi-host, "CPLD" image type detected, field "TargetHosts"
not present in MANIFEST file**
    ```
        $ busctl tree xyz.openbmc_project.Software.HOST.Updater
        └─/xyz
            └─/xyz/openbmc_project
                └─/xyz/openbmc_project/software
                ├─/xyz/openbmc_project/software/1a56bff3
                │   ├─/xyz/openbmc_project/software/1a56bff3/cpld_1 # Activation
                │   ├─/xyz/openbmc_project/software/1a56bff3/cpld_2 # Activation
                │   └─/xyz/openbmc_project/software/1a56bff3/cpld_3 # Activation
                └─────/xyz/openbmc_project/software/1a56bff3/cpld_4 # Activation
    ```
- **multi-host, "BIOS" image type detected, considering the field
"TargetHosts=4" present in MANIFEST file**
  ```
        $ busctl tree xyz.openbmc_project.Software.HOST.Updater
        └─/xyz
            └─/xyz/openbmc_project
                └─/xyz/openbmc_project/software
                ├─/xyz/openbmc_project/software/1a56bff3
                └──/xyz/openbmc_project/software/1a56bff3/bios_4  # Activation
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
    ## in multi-host machines be careful about the "type"
    ## as the environment is used they should not start at same time
    Type=exec
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

### Software Update Procedure
Software update will be performed by the following process:

- Copy the software/firmware image to "/tmp/images" directory.
- Activate the "Activation" flag on a single device/host to start the image
update. In case of multi-host machines, another "Activation" flag can be
activated performing in this case parallel update.
- After successful activation the image may be deleted if the requirements
match (see Image Cleaning section).


### Image Cleaning for multi-host machines
In the existing system, the image will be deleted after the successful
software/firmware update.

But in this new approach, the image has to be deleted only
if all the required devices/hosts have being updated, the following cases are
possible:
1. The field "TargetHosts" is present in the MANIFEST file defining one or more
devices/hosts, after all them be updated the image is removed from disk.
2. The field "TargetHosts" is NOT present in the MANIFEST file, the updated is
performed in all devices/hosts indentified by the software manager, then the
image is removed from disk.

In any other case not listed above the user has the responsibility of removing
the image manually.

The image delete logic in in both cases deleted by the system or deleted by the
user will result on the software objects path removal and software state
cleaning.

#### Listen to image removal event
For multi-host machines it may be necessary to have a mechanism of listening to
image cleaning because when it is performed by the user it is also necessary to
clean flags in the software manager.


### Impacts
- Current software manager creates an
object */xyz/openbmc_project/software/bios_active* which can be removed.
- The current service file "obmc-flash-host-bios@.service" will be renamed
to "obmc-flash-host-software@.service".

### MANIFEST File Changes Proposal
1. To reduce the risk of image type detection the field "ImageType" could
be added as optional in the MANIFEST file.
2. Multi-host machines could add the optional field "TargetHosts" to
clear say which set of devices/hosts the software update will be applied.

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
