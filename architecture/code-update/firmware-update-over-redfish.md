# Firmware update over Redfish

Author: Andrew Geissler (geissonator)

Created: 2019-02-11

## Problem Description

OpenBMC is moving to [Redfish][1] as its standard for out of band management.
Redfish has its own API's and methods for updating firmware on a system and
implementing those is going to require some changes (and potential upstream work
with the DMTF).

The goal of this design document is to lay out the required changes of moving
OpenBMC's existing firmware update implementation over to Redfish.

## Background and References

The existing firmware update details for OpenBMC can be found [here][2]. It uses
a custom REST api for uploading and then activating the input image.

The Redfish schema for firmware update can be found [here][3]. Note the
referenced doc points to the most recent version of the schema and that is what
you'll want to load into your browser (at the time of this writing it is
[v1_4_0][4]).

Some differences between the Redfish API and OpenBMC's existing API:

- Redfish has a single upload and update API. OpenBMC has a concept of uploading
  an image with one API and then activating with another.
- Redfish does not support multiple firmware images being associated with the
  same target. OpenBMC does have support for this concept (for example when a
  target like the BMC has multiple flash chips associated with it). A discussion
  has been started with the DMTF on this within [Issue 3357][10]
  - OpenBMC has the concept of a priority that allows a user to chose an image
    associated with a target for activation. This allows a user to easily switch
    back and forth between multiple images for a single target without uploading
    each image every time.
- Redfish does not support deleting a firmware image (this happens by default
  when a new image is uploaded)
- Redfish does support the ability to update multiple targets with the same
  image with the same command. OpenBMC does not support this currently.
- Redfish has support via a SimpleUpdate action which allows the user to pass in
  a variety of remote locations to retrieve a file from (FTP, HTTP, SCP, ...).
  Existing OpenBMC only has support for TFTP.
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

### Future Requirements

**Note:** TODO: The goal of this section is to document at a high level what
will be required post 2.7 release. An update to this design document will be
required before these requirements are implemented. They are here to ensure
nothing in the base design would inhibit the ability to implement these later.

- Support new concept defined in [PR 3420][12] to be able to support multiple
  firmware images for a single target.
- Support new UpdateService firmware update implementation defined in [Issue
  3296][11]
- Support firmware update for other targets (power supplies, voltage regulators,
  ...)
- Support to update multiple targets with the same firmware image at once
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
stack at the end of the activation. Note that the ApplyTime property is global
to all firmware update packages and will be used for all subsequent firmware
update packages.

### Status of an Image

The user needs to know the status of their firmware images, especially during an
update. The Task concept is still being defined within the DMTF and will provide
the full status of an image during upload and activate. Using the Status object
associated with the software inventory items will be used as a mechanism to
provide status of inventory items back to the user. Here is the mapping of
[phosphor activation states][13] to [Redfish Status States][14].

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

### Remote Image Download Based Update

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

## Alternatives Considered

Could simply create Redfish OEM api's that look like OpenBMC's current custom
REST api's. The porting would be minimal but then OpenBMC would not conform to
any standard Redfish API's. Other vendors have done this but the Redfish DMTF is
making strides towards enhancing the UpdateService to contain the features
required to make an enterprise based firmware update API. OpenBMC should just
stay at the forefront of these DMTF changes and ensure they are implemented as
they are released.

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
[2]:
  https://github.com/openbmc/docs/blob/master/architecture/code-update/code-update.md#steps-to-update
[3]: http://redfish.dmtf.org/schemas/v1/UpdateService.json
[4]:
  http://redfish.dmtf.org/schemas/v1/UpdateService.v1_4_0.json#/definitions/UpdateService
[5]: https://www.supermicro.com/manuals/other/RedfishRefGuide.pdf
[6]:
  https://github.com/dell/iDRAC-Redfish-Scripting/blob/master/Redfish%20Python/DeviceFirmwareDellUpdateServiceREDFISH.py
[7]:
  https://github.com/openbmc/bmcweb/blob/master/redfish-core/lib/update_service.hpp
[8]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Software/README.md
[9]: https://github.com/openbmc/openbmc-test-automation
[10]: https://github.com/DMTF/Redfish/issues/3357
[11]: https://github.com/DMTF/Redfish/pull/3296
[12]: https://github.com/DMTF/Redfish/pull/3420
[13]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Software/Activation.interface.yaml
[14]: http://redfish.dmtf.org/schemas/v1/Resource.json#/definitions/Status
