# Firmware update over Redfish

Author: Andrew Geissler (geissonator), Vikram Bodireddy(vbodired)

Primary assignee: Andrew Geissler (geissonator), Vikram Bodireddy(vbodired)

Created: 2019-02-11
Updated: 2019-11-26

## Problem Description
OpenBMC is moving to [Redfish][1] as its standard for out of band management.
Redfish has its own API's and methods for updating firmware on a system and
implementing those is going to require some changes (and potential upstream work
with the DMTF).

The goal of this design document is to lay out the required changes of moving
OpenBMC's existing firmware update implementation over to Redfish.

## Background and References
The existing firmware update details for OpenBMC can be found [here][2].
It uses a custom REST api for uploading and then activating the input image.

The Redfish schema for firmware update can be found [here][3]. Note the
referenced doc points to the most recent version of the schema and that is
what you'll want to load into your browser (at the time of this writing it is
[v1_4_0][4]).

Some differences between the Redfish API and OpenBMC's existing API:
- Redfish has a single upload and update API. OpenBMC has a concept of uploading
  an image with one API and then activating with another.
- Redfish does not support multiple firmware images being associated with the
  same target. OpenBMC does have support for this concept (for example when a
  target like the BMC has multiple flash chips associated with it). A discussion
  has been started with the DMTF on this within [Issue 3357][10]
  - OpenBMC has the concept of a priority that allows a user to chose an
    image associated with a target for activation. This allows a user to easily
    switch back and forth between multiple images for a single target without
    uploading each image every time.
- Redfish does not support deleting a firmware image (this happens by default
  when a new image is uploaded)
- Redfish has support via a SimpleUpdate action which allows the user to
  pass in a variety of remote locations to retrieve a file from
  (FTP, HTTP, SCP, ...). Existing OpenBMC only has support for TFTP.
- Redfish has the ability to schedule when a firmware update is applied
  (Immediate, OnReset, AtMaintenanceWindowStart...). Existing OpenBMC firmware
  has no concept of this.

A lot of companies that are implementing Redfish have chosen to create OEM
commands for their firmware update implementations ([ref a][5], [ref b][6]).

Redfish firmware update within OpenBMC has already started within [bmcweb][7]
and this design will be proposing enhancements to that as a base.

## Requirements

Breaking this into two sections. The first is what should be done for the next
OpenBMC 2.7 release. The other section is future items that will get done, but
not guaranteed for the 2.7 release. The goal with 2.7 is to have feature parity
with what the existing OpenBMC REST api's provide.

### 2.7 Requirements
- Support FirmwareInventory definition for BMC and BIOS under UpdateService
  - Support FirmwareVersion under Managers/BMC
  - Support BiosVersion under Systems/system
- Support firmware update for all supported targets (BMC, BIOS) using existing
  Redfish API's (/redfish/v1/UpdateService)
  - Note that after further discussion with the DMTF, the existing UpdateService
    is considered to be OEM. [Issue 3296][11] addresses this and will be
    implemented by OpenBMC once approved. The existing API will continue to be
    supported within the Redfish specification so the OpenBMC 2.7 release will
    support this.
  - Must provide status to user on their image during an update
- Support a subset of ApplyTime (Immediate, OnReset)
- Support a TFTP based SimpleUpdate
  - This must be configurable (enable/disable) via compile option within bmcweb
- Support to update multiple targets with the same firmware image at once
  - Support to allow users to specify the targets to which the uploaded image to
    be applied. This is implemented by using the redfish property
    'HttpPushUriTargets'.
  - Support redfish clients to find out if the firmware targets are in use for
    updates or available for updates. Redfish property 'HttpPushUriTargetsBusy'
    is used for implementing this.

### Future Requirements

**Note:** TODO: The goal of this section is to document at a high
level what will be required post 2.7 release. An update to this design document
will be required before these requirements are implemented. They are here to
ensure nothing in the base design would inhibit the ability to implement these
later.

- Support new concept defined in [PR 3420][12] to be able to support multiple
  firmware images for a single target.
- Support new UpdateService firmware update implementation defined in
  [Issue 3296][11]
- Support firmware update for other targets (power supplies, voltage regulators,
  ...)
- Support scheduling of when update occurs (Maintenance windows)
- Support remaining TransferProtocolTypes in UpdateService (CIFS, FTP, SFTP,
  HTTP, HTTPS, NSF, SCP, NFS)
  - TODO: Any update mechanism that doesn't have transport security on its own,
    like FTP, needs a secondary verification mechanism. Update mechanisms that
    have transport security need a way to adjust the trusted root certificates.
- Support the Task schema to provide progress to user during upload and
  activation phases

## Proposed Design

### Update An Image

The pseudo flow for an update is:
```
Discover UpdateService location
HttpPushUri = GET https://${bmc}/redfish/v1/UpdateService
POST ApplyTime property in
  UpdateService/HttpPushUriOptions->HttpPushUriApplyTime object
  (Immediate or OnReset)
POST <image> https://${bmc}/${HttpPushUri}
```
This will cause the following on the back-end:
- Receive the image
- Copy it internally to the required location in the filesystem (/tmp/images)
- Wait for the InterfacesAdded signal from /xyz/openbmc_project/software
- Activate the new image
- If ApplyTime is Immediate, reboot the BMC or Host
- If ApplyTime is OnReset, do nothing (user will need to initiate host or bmc
    reboot to apply image)

Note that the ApplyTime property will be processed by the software management
stack at the end of the activation.
Note that the ApplyTime property is global to all firmware update packages and
will be used for all subsequent firmware update packages.

### Status of an Image

The user needs to know the status of their firmware images, especially during
an update. The Task concept is still being defined within the DMTF and will
provide the full status of an image during upload and activate.
Using the Status object associated with the software inventory items will be
used as a mechanism to provide status of inventory items back to the user.
Here is the mapping of [phosphor activation states][13] to
[Redfish Status States][14].
```
NotReady   -> Disabled
Invalid    -> Disabled
Ready      -> Disabled
Activating -> Updating
Active     -> Enabled
Failed     -> Disabled
```

### Change the Priority of an Image

On OpenBMC systems that support multiple flash images for one target, there will
still be pseudo support for this using Redfish. The first image will be uploaded
to a system, written to flash chip 0, and activated. If another image is
uploaded, it will be written to flash chip 1 and activated. The first image
still exists in flash chip 0 and could be re-activated by the user.

As defined in DMTF issue [3357][10], use the ActiveSoftwareImage concept within
the Redfish Settings object to allow the user to select the image to activate
during the next activation window. Internally OpenBMC has the concept of a
priority being assigned to each image associated with a target. OpenBMC firmware
will continue to do this, but will simply set the "ActiveSoftwareImage" image to
priority 0 (highest) to ensure it is activated during the next activation
window. After setting the priority to 0, the software manager automatically
updates the priority of the other images as needed.

### Remote Image Download Based Update ###

As noted above, Redfish supports a SimpleUpdate object under the UpdateService.
The SimpleUpdate schema supports a variety of transfer protocols (CIFS, FTP,
TFTP, ...). The existing back end of OpenBMC only supports TFTP so initially
that is all that will be supported via Redfish on OpenBMC.

The Redfish API takes a parameter, ImageURI, which contains both the server
address information and the name of the file. The back end software manager
interface on OpenBMC requires two parameters, the TFTP server address and the
file name so there will be some parsing required.

The pseudo flow for an update is:
```
# Discover SimpleUpdate URI Action location
GET https://${bmc}/redfish/v1/UpdateService

# Configure when the new image should be applied
POST ApplyTime property in
  UpdateService/HttpPushUriOptions->HttpPushUriApplyTime object
  (Immediate or OnReset)

# Request OpenBMC to download from TFTP server and activate the image
POST https://${bmc}/redfish/v1/UpdateService/Actions/UpdateService.SimpleUpdate
    [ImageURI=<tftp server ip>/<file name>, TransferProtocol=TFTP]
```

TFTP is an insecure protocol. There is no username or password associated with
it and no validation of the input URL. OpenBMC does have signed image support
which is, by default, part of the update process. The user must have
administration authority to request this update so it is assumed they know what
they are doing and the level of security available with TFTP.

Due to the security concerns with TFTP, this feature will be disabled by default
within bmcweb but can be enabled via a compile flag (see CMakeLists.txt within
bmcweb repository for details).

### Delete an Image
No support for deleting an image will be provided (until the DMTF provides it).
When new images are uploaded, they by default have no priority. When they are
activated, they are given the highest priority and all other images have their
priority updated as needed. When no more space is available in flash filesystem,
the lowest priority image will be deleted when a new one is uploaded and
activated.

### Multiple firmware target update with one image
### Allow users to specify the firmware target to be updated

The firmware update flow using 'HttPushUri' is described above in this document
which gives how a user uploads the image and activates it.

This design proposes to use the Redfish objects 'HttpPushUriTargets',
'HttpPushUriTargetsBusy' and 'HttPushUri' in conjunctions for allowing users to
specify the targets to which the uploaded firmware image to be applied.

Redfish Specification defines 'HttpPushUriTargets' and
'HttpPushUriTargetsBusy' objects for supporting target specific firmware
updates and also for synchronization between multiple redfish clients using the
'HttPushUri' URI.

Below given steps that describes the flow for how a redfish client can set the
firmware target for update and uploads the image.

1. set 'HttpPushUriTargetsBusy' property to 'true' by PATCH method.
2. set 'HttpPushUriTargets' property with the firmware target URIs or related
string to which the firmware to be updated by PATCH method.
3. Upload the firmware image to the UriTarget using 'HttPushUri' by POST
method.

```
+----------------+              +----------------+            +-----------------+              +-----------------+
|Redfish client  |              |     bmcweb     |            |phosphor-software|              |   fwupd script  |
|app like postman|              | (redfish_core) |            |-manager         |              |                 |
+-------+--------+              +-------+--------+            +--------+--------+              +---------+-------+
        |                               |                              |                                 |
  check |'HttpPushUriTargetsBusy'       |                              |                                 |
     as |'false' by GET                 |                              |                                 |
        |                               |                              |                                 |
        |                               |                              |                                 |
    set | 'HttpPushUriTargets' also with|                              |                                 |
        | 'HttpPushUriTargetsBusy'      |                              |                                 |
        +------------------------------->                              |                                 |
        | as true by PATCH              | validate and set internal    |                                 |
        |                               | parameters in update service |                                 |
        |                               |                              |                                 |
        +<-------+return_code-----------+                              |                                 |
        |                               |                              |                                 |
        |                               |                              |                                 |
 upload |image to 'HttPushUri'          |                              |                                 |
     by | POST                          |                              |                                 |
        |                               + Update Service writes        |                                 |
        |                               | image to '/tmp/images'       |                                 |
        |                               | waits for software interface |                                 |
        |                               | to be added by               |                                 |
        |                               | phosphor-software-manager    |                                 |
        |                               |                              + Image Watch routine finds       |
        +<------------------------------+                              | new image in '/tmp/images'      |
        |                               |                              | and triggers image_manager      |
        |                               |                              |                                 |
        |                               |                              + image_manager processes image   |
        |                               |                              | and if image is good then, adds |
        |                               |                              | image object as new software    |
        |                               |                              | interface for activation.       |
        |                               |<-----------------------------+                                 |
        |                               + Update Service finds         |                                 |
        |                               | the new software interface   |                                 |
        |                               | for the uploaded image.      |                                 |
        |                               | And does the image activation|                                 |
        |                               | by setting the appropriate   |                                 |
        |                               | redundancy priority for the  |                                 |
        |                               | uploaded image. Redundancy   |                                 |
        |                               | priority matches with        |                                 |
        |                               | as given by FirmwareInventory|                                 |
        |                               +----------------------------->|                                 |
        |                               |                              |                                 |
        |                               |                              + Activation module in bmcweb     |
        |                               |                              | matches the software interface  |
        |                               |                              | added with 'HttpPushUriTargets' |
        |                               |                              | and sets the appropriate        |
        |                               |                              | redundancy priority to the image|
        |                               |                              | object and sets the             |
        |                               |                              | RequestedActivations to image   |
        |                               |                              | object.                         |
        |                               |                              |                                 |
        |                               |                              +-------------------------------->+
        |                               |                              |                                 | Activation code in
        |                               |                              |                                 | phosphor-bmc-code-mgmt gets
        |                               |                              |                                 | notified about the
        |                               |                              |                                 | RequestedActivations and calls
        |                               |                              |                                 | appropriate flashwrite routine.
        |                               |                              |                                 |
        |                               |                              |                                 |
        +                               +                              +                                 +
```

The above steps from redfish interface makes phosphor-software-manager to read
and process the uploaded image. After the image being posted to
phosphor-software-manager, it acknowledges that the image being identified by
creating image object under /tmp/images/ which is monitored and
softwareInterfaceAdded is called in UpdateService. softwareInterfaceAdded
function matches the set 'HttpPushUriTargets' with the FirmwareInventory list
and sets the appropriate redundancy priority to the uploaded image object.

With the 'HttpPushUriTargets' and 'HttpPushUriTargetsBusy' properties included
in UpdateService URI, the GET response of
https://${bmc-ip}/redfish/v1/updateservice is as below, with empty target array.
(shown the new properties in bold)

```
{
  "@odata.context": "/redfish/v1/$metadata#UpdateService.UpdateService",
  "@odata.id": "/redfish/v1/UpdateService",
  "@odata.type": "#UpdateService.v1_2_0.UpdateService",
  "Description": "Service for Software Update",
  "FirmwareInventory": {
    "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory"
  },

  "HttpPushUri": "/redfish/v1/UpdateService",
  **"HttpPushUriTargets": [],**
  **"HttpPushUriTargetsBusy": "false"**
  "Id": "UpdateService",
  "Name": "Update Service",
  "ServiceEnabled": true
}
```

1. Get 'HttpPushUriTargetsBusy' property from UpdateService URI to check and set
it to true if its false. A 'True' on this property indicates that another
redfish client is using the 'HttpPushUriTargets' to upload image for specific
firmware target. This design doesn't allow setting 'HttpPushUriTargets' if
'HttpPushUriTargetsBusy' is false. This works as lock on 'HttpPushUriTargets' so
that no two redfish clients can use 'HttpPushUriTargets' simultaneously.

Set 'HttpPushUriTargetsBusy' to true and also set 'HttpPushUriTargets' to the
desired image target in the same PATCH command.

```
PATCH -d '{"HttpPushUriTargetsBusy": true,
"HttpPushUriTargets": "bmc_recovery"}'
https://${bmc}/redfish/v1/UpdateService
```
'HttpPushUriTargetsBusy' property need to be cleared by the redfish client after
the firmware update is done.

The list of firmware targets which supports firmware update to be found out from
the FirmwareInventory URI and the specific firmware target by GET method as
below. The firmware targets which has "Updateable" field set to "true" is to be
used for setting it in 'HttpPushUriTargets'.

GET https://${bmc-ip}/redfish/v1/updateService/FirmwareInventory

```
{
  "@odata.context": "/redfish/v1/$metadata#SoftwareInventoryCollection.SoftwareInventoryCollection",
  "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory",
  "@odata.type": "#SoftwareInventoryCollection.SoftwareInventoryCollection",
  "Members": [
    {
      "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/bmc_active"
    },
    {
      "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/bmc_recovery"
    }
  ],
  "Members@odata.count": 2,
  "Name": "Software Inventory Collection"
}
```

GET https://${bmc_ip}/redfish/v1/UpdateService/FirmwareInventory/bmc_recovery
```
{
  "@odata.context": "/redfish/v1/$metadata#SoftwareInventory.SoftwareInventory",
  "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/bmc_recovery",
  "@odata.type": "#SoftwareInventory.v1_1_0.SoftwareInventory",
  "Description": "BMC image",
  "Id": "bmc_recovery",
  "Members@odata.count": 1,
  "Name": "Software Inventory",
  "RelatedItem": [
    {
      "@odata.id": "/redfish/v1/Managers/bmc"
    }
  ],
  "Status": {
    "Health": "OK",
    "HealthRollup": "OK",
    "State": "Enabled"
  },
  "Updateable": true,
  "Version": "00.18-00-b946b8"
}
```

Also, the firmware update service in BMC validates the uploaded image against
set 'HttpPushUriTargets' validity with image. If there is no targets are being
set then the update service in BMC applies the image to the default applicable
firmware target.

3. Upload the firmware image for the target bmc_recovery

POST <image> https://${bmc}/redfish/v1/UpdateService

This uploads the image and installs onto bmc_recovery firmware target.

D-Bus property:

Activation.interface.yaml of D-Bus is modified to add "Staged" for Activations.

"xyz.openbmc_project.Software.Activation.interface.yaml"
   Activations:
        - name: Staged
          description: >
            The Software.Version is currently in staged flash area. This will
            be moved from staged flash area to active upon reset.

[Activations.interface Yaml change](https://gerrit.openbmc-project.xyz/c/openbmc/phosphor-dbus-interfaces/+/27632)

## Alternatives Considered
Could simply create Redfish OEM api's that look like OpenBMC's current custom
REST api's. The porting would be minimal but then OpenBMC would not conform
to any standard Redfish API's. Other vendors have done this but the Redfish
DMTF is making strides towards enhancing the UpdateService to contain the
features required to make an enterprise based firmware update API. OpenBMC
should just stay at the forefront of these DMTF changes and ensure they are
implemented as they are released.

## Impacts
This will be using the existing D-Bus API's for firmware update internally so
there should be minimal impact between the previous REST implementation.

OpenBMC has two implementations of firmware update. Different systems have
implemented different implementations. To be clear, this design will be using
the D-Bus API's behind the [Software Version Management][8] implementation.

## Testing
End to end [tests][9] are being written for the firmware update of BMC and BIOS
firmware, these must pass. Also unit tests must be written with the required
D-Bus API's mocked to ensure proper code coverage.

[1]: https://redfish.dmtf.org/
[2]: https://github.com/openbmc/docs/blob/master/code-update/ubi-code-update.md#steps-to-update
[3]: http://redfish.dmtf.org/schemas/v1/UpdateService.json
[4]: http://redfish.dmtf.org/schemas/v1/UpdateService.v1_4_0.json#/definitions/UpdateService
[5]: https://www.supermicro.com/manuals/other/RedfishRefGuide.pdf
[6]: https://github.com/dell/iDRAC-Redfish-Scripting/blob/master/Redfish%20Python/DeviceFirmwareDellUpdateServiceREDFISH.py
[7]: https://github.com/openbmc/bmcweb/blob/master/redfish-core/lib/update_service.hpp
[8]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Software/README.md
[9]: https://github.com/openbmc/openbmc-test-automation
[10]: https://github.com/DMTF/Redfish/issues/3357
[11]: https://github.com/DMTF/Redfish/pull/3296
[12]: https://github.com/DMTF/Redfish/pull/3420
[13]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Software/Activation.interface.yaml
[14]: http://redfish.dmtf.org/schemas/v1/Resource.json#/definitions/Status
