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
difficult to get vendor buy-in as the boundary is not cleary defined, and
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
- Provide a method to monitor the update.

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
          Filepath to the signature.
    errors:
      - self.Error.UpdateFirmwareFailure

  - name: GetFirmwareUpdateStatus
    description: >
      Get the status of the firmware update process.
    returns:
      - name: Status
        type: enum[self.FirmwareUpdateStatus]
        description: >
          Status of the firmware update

enumerations:
   - name: FirmwareUpdateStatus
     description: >
       The status of a firmware update.
     values:
       - name: 'None'
         description: >
           No update initiated.
       - name: 'InProgress'
         description: >
           Update still in progress.
       - name: 'UpdateError'
         description: >
           Update has failed.
       - name: 'VerifyError'
         description: >
           Verify has failed.
       - name: 'Done'
         description: >
           Update has completed successfully.
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
