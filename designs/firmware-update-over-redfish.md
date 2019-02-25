# Firmware update over Redfish

Author: Andrew Geissler (geissonator)

Primary assignee: Andrew Geissler (geissonator)

Created: 2019-02-11

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
  target like the BMC has multiple flash chips associated with it)
  - OpenBMC has the concept of a priority that allows a user to chose an
    image associated with a target for activation. This allows a user to easily
    switch back and forth between multiple images for a single target without
    uploading each image every time.
- Redfish does not support deleting a firmware image (this happens by default
  when a new image is uploaded)
- Redfish does support the ability to update multiple targets with the same
  image with the same command. OpenBMC does not support this currently.
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
- Support a subset of ApplyTime (Immediate, OnReset)
- Support a TFTP based SimpleUpdate
- Support an OEM (or get upstream to DMTF) to select the priority of an existing
  image associated with a target

### Future Requirements

**Note:** TODO: The goal of this section is to document at a high
level what will be required post 2.7 release. An update to this design document
will be required before these requirements are implemented. They are here to
ensure nothing in the base design would inhibit the ability to implement these
later.

- Support firmware update for other targets (power supplies, voltage regulators,
  ...)
- Support to update multiple targets with the same firmware image at once
- Support scheduling of when update occurs
- Support remaining TransferProtocolTypes in UpdateService (CIFS, FTP, SFTP,
  HTTP, HTTPS, NSF, SCP, NFS)
  - TODO: Any update mechanism that doesn't have transport security on its own,
    like FTP, needs a secondary verification mechanism. Update mechanisms that
    have transport security need a way to adjust the trusted root certificates.

## Proposed Design

### Update An Image

The pseudo flow for an update is:
```
Discover UpdateService location
HttpPushUri = GET https://${bmc}/redfish/v1/UpdateService
POST <image> https://${bmc}/${HttpPushUri} ApplyTime=Immediate
```
This will cause the following on the back-end:
- Receive the image
- Copy it internally to the required location in the filesystem (/tmp/images)
- Wait for the InterfacesAdded signal from /xyz/openbmc_project/software
- Activate the new image (which may involve a reboot of the BMC)


### Change the Priority of an Image

On OpenBMC systems that support multiple flash images for one target, there will
still be pseudo support for this using Redfish. The first image will be uploaded
to a system, written to flash chip 0, and activated. If another image is
uploaded, it will be written to flash chip 1 and activated. The first image
still exists in flash chip 0 and could be re-activated by the user.

A new OpenBMC OEM property will be created, "Priority" under the UpdateService.
Setting the priority higher for an image will cause it to be activated
automatically. The activation will involve a BMC reboot if the image was for the
BMC.

### Delete an Image
No support for deleting an image will be provided (until the DMTF provides it).
Want to reduce OEM interfaces and the ability to set priority will allow the
user to select which image is replaced upon a new update request. The lowest
priority image will be automatically deleted when a new one is updated.

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
