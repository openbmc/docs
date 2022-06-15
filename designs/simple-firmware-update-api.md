# Simple Firmware Update API

Author:
  Justin Ledford (justinledford@google.com)

Created:
  June 14, 2022

## Problem Description
The existing software update process is useful for software update systems that
need to stage images from outside the BMC, and activate them at a later time.
But for software update systems where the upgrade process is entirely contained
within the BMC, the concept of a distinct image manager process may not provide
any benefit, as well as add unnecessary complexity. This process may also be
difficult to get vendor buy-in as the boundary is not clearly defined, and
requires the vendor to fully understand the entire workflow, rather than to
simply implement one or two methods.

## Background and References
The
[existing Software API](https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/yaml/xyz/openbmc_project/Software)
consists of an ImageManager to manage all of the images available for update,
ItemUpdaters to handle updating firmware on a device, and a client to coordinate
the entire process.

The ImageManager monitors for newly added files to the BMC and creates D-Bus
objects for available images. The ItemUpdater monitors for images corresponding
to devices it manages and creates D-Bus objects for activations of an image on a
device. The client is then responsible for monitoring for activation objects it
is interested in and updating properties to start the activation process.

The intent of this design is to propose a new simple API to facilitate firmware
updates.

## Requirements
The requirements for a simple update API are the following:
- Provide a method to start the update given a firmware file along with an
  optional signature.
- Provide an optional method to apply the given firmware for devices that
  have a stage and apply type update flow.

## Proposed Design
The proposed API contains the following methods:

```
methods:
  - name: UpdateFirmware
    description: >
      Update device with firmware specified in the image path. If a non-empty
      signature path is given the firmware is also verified.
    parameters:
      - name: ImagePath
        type: string
        description: >
          Filepath to the firmware image.
      - name: SignaturePath
        type: string
        description: >
          An optional filepath to the signature.
    errors:
      - self.Error.DeviceNotReady
      - self.Error.UpdateInProgress
      - self.Error.UpdateError
      - self.Error.VerifyError
methods:
  - name: ApplyFirmware
    description: >
      Apply the updated firmware, either immediately or on the next device
      reset.
    parameters:
      - name: ApplyOnReset
        type: boolean
        description: >
          If false, the updated firmware will apply immediately, else it will
          be applied on the next device reset.
    errors:
      - self.Error.ApplyFirmwareError
```

A client is then able to call UpdateFirmware with the firmware image to start
the update process, and then poll GetFirmwareUpdateStatus until the process
has finished or failed. A daemon supporting firmware upgrades for a device
should then implement these methods on an object representing that device.

These methods will be added under xyz.openbmc_project.Software.

The existing ActivationProgress and Version interfaces will still be used
to supply progress and version information.

## Alternatives Considered
None

## Impacts
With the introduction of this API, there will be two APIs for updating firmware.
Support in BMCWeb can initially be added for both interfaces, with a plan to
eventually deprecate the existing ImageManager/ItemUpdater API.

### Organizational
Does this repository require a new repository?  No

## Testing
None
