# PSU firmware update

Author: Lei YU <mine260309@gmail.com> <LeiYU>

Other contributors: Su Xiao <suxiao@inspur.com> Derek Howard <derekh@us.ibm.com>

Created: 2019-06-03

## Problem Description

There is no support in OpenBMC to update the firmware for PSUs.

## Background and References

In OpenBMC, there is an existing interface for [software update][1].

The update process consists of:

1. Uploading an image to the BMC;
2. Processing the image to check the version and purpose of the image;
3. Verifying and activating the image.

Currently, BMC and BIOS firmware update are supported:

- [phosphor-bmc-code-mgmt][2] implements BMC code update, and it supports all
  the above 3 processes.
- [openpower-pnor-code-mgmt][3] implements BIOS code update, and it only
  implements "verifying and activating" the image. It shares the function of the
  above 1 & 2 processes.
- Both of the above use the same [Software DBus interface][1].

For PSU firmware update, it is preferred to re-use the same function for the
above 1 & 2.

## Requirements

The PSU firmware shall be updated in the below cases:

1. The user manually invokes the APIs to do the update;
2. After BMC code update and if there is a newer PSU image in the BMC's
   filesystem, BMC shall update the PSU firmware;
3. When a PSU is replaced and the version is older than the one in BMC's
   filesystem, BMC shall update the PSU firmware.
4. There are cases that a system could use different models of PSUs, and thus
   different PSU firmware images need to be supported.

For some PSUs, it is risky to do PSU code update while the host is running to
avoid power loss. This shall be handled by PSU vendor-specific tools, but not in
the generic framework.

Note: The "vendor-specific" referred below is the PSU vendor-specific.

So the below checks are optional and expected to be handled by vendor-specific
tool:

1. If the host is powered off;
2. If the redundant PSUs are all connected;
3. If the AC input and DC standby output is OK on all the PSUs;

## Proposed Design

As described in the above requirements, there are different cases where the PSU
firmware is updated:

- When the APIs are invoked;
- When a new version is updated together with BMC code update;
- When a PSU is replaced with an old version of the firmware.

### Update by API

This method is usually used by users who manually update PSU firmware.

It will re-use the current interfaces to upload, verify, and activate the image.

1. The "Version" interface needs to be extended:
   - Add a new [VersionPurpose][4] for PSU;
   - Re-use the existing `ExtendedVersion` as an additional string for
     vendor-specific purpose, e.g. to indicate the PSU model.
2. Re-use the existing functions implemented by [phosphor-bmc-code-mgmt][2] for
   uploading and processing the image.
   - The PSU update image shall be a tarball that consists of a MANIFEST,
     images, and signatures.
   - When the PSU image is uploaded and processed, a `VersionObject` shall be
     created to indicate the version and its purpose.
3. There will be a new service that implements the [Activation][5] interface to
   update the PSU firmware.
   - The service will be started by default when BMC starts;
   - On start, the service will check the PSU's existing firmware and create the
     `Version` and `Activation` interfaces.
   - The service shall watch the interface added on
     `/xyz/openbmc_project/Software`.
   - When a new object with PSU `VersionPurpose` is added, the service will
     verify the signature of the image;
   - The service shall check the `ExtendedVersion` to make sure the image
     matches the PSU model.
   - The service will have a configuration file to describe the PSU model and
     its related vendor-specific tools.
   - The service will find the matched vendor-specific tool to perform the code
     update. For example, if a vendor specific tool `foo` is configured in
     `psu-update@foo.service` which executes `foo psu.bin`, the service will
     find the `psu-update@foo.service` and start it by systemd, which performs
     the update.
   - When the PSU code update is completed, an informational event log shall be
     created.
   - When the PSU code update is completed, the image, MANIFEST, and optionally
     the signature will be saved to a pre-defined directory in read-write
     filesystem for future use, in case a new PSU with old firmware is plugged.
4. The vendor-specific tool shall run all the checks it needs to be run, before
   and after the PSU update, and return a status to the above service to
   indicate the result.
5. When the vendor-specific tool returns errors, the PSU update will be aborted
   and an error event log shall be created.
6. During the update, the service shall set the related sensors to
   non-functional, and when the update is done, it shall set the related sensors
   back to functional.

### Update by new BMC image

When BMC is updated and a new version of PSU firmware is included, it shall be
updated to the PSU. This will be done by the same service described above.

1. On start, the service will check the PSU image, model and version in its
   filesystem, compare with the ones in PSU hardware and decide if PSU firmware
   update shall be performed.
2. There could be two places containing the PSU images:
   - The pre-defined directory in read-only filesystem, which is part of BMC
     image.
   - The other pre-defined directory in read-write filesystem, which is the
     location for the saved PSU images by API update. Both places shall be
     checked and a newer version will be selected to compare with the PSU
     hardware.
3. If PSU update is needed, the service will find the matched vendor-specific
   tool to perform the code update.
4. The following process will be the same as [Update by API].

### Update on replaced PSU

When a PSU is replaced, and the firmware version is older than the one in BMC
filesystem, it shall be updated. This will be done by the same service described
above.

1. On start, the service will subscribe to the PropertiesChanged signal to the
   PSU object path to monitor the PSU presence status. (Or maybe subscribe the
   InterfacesAdded/Removed signal?)
2. When a PSU's presence status is changed from false to true (or the
   InterfacesAdded event occurs), the service will check the new PSU's model,
   firmware version to decide if the firmware needs to be updated.
3. If yes, the service will find the matched vendor-specific tool to perform the
   code update.
4. The following process will be the same as [Update by API].

## Alternatives Considered

### General implementation

The PSU firmware update could be implemented by separated recipes that only call
vendor-specific tools. It will be a bit simpler but loses the unified interface
provided by OpenBMC's existing [software update interface][1], and thus it will
become difficult to use a standard API to the PSU firmware update.

### VersionPurpose

It is possible to re-use the `VersionPurpose.Other` to represent the PSU image's
version purpose. But that requires additional information about the image,
otherwise, there is no way to tell if the image is for PSU, or CPLD, or other
peripherals. A new `VersionPurpose.PSU` is more specific and makes it easier to
implement and friendly for the user.

### Additional string

The design proposal uses `ExtendedVersion` as the additional string for
vendor-specific purpose, e.g. to indicate the PSU model, so the implementation
could check and compare if the image matches the PSU model. It is possible to
make it optional or remove this additional string, then the implementation will
not verify if the image matches the PSU. It could be OK if we trust the user who
is uploading the correct image, especially the image shall be signed. But it is
always risky in case the image does not match the PSU, and cause unintended
damage if the incorrect PSU firmware is updated.

## Impacts

This design only introduces a new `VersionPurpose` enum into the dbus
interfaces. The newly introduced PSU firmware update service will be a new
service that implements existing [Activation][5] interface. There will be new
configuration files for the service to:

- Link the vendor specific tool with PSU models.
- Get the sensors related to the PSU.
- etc.

So the impacts are minimal to existing systems.

## Testing

It requires the manual tests to verify the PSU code update process.

- Verify the PSU code update is done on all PSUs successfully;
- Verify the PSU code update will fail if the vendor-specific tool fails on
  pre-condition check, of fails on updating PSU.
- Verify the PSU code update is performed after a new BMC image is updated
  containing a new version of PSU firmware.
- Verify the PSU code update is performed after a PSU with old firmware is
  plugged in.

[1]:
  https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/yaml/xyz/openbmc_project/Software
[2]: https://github.com/openbmc/phosphor-bmc-code-mgmt/
[3]: https://github.com/openbmc/openpower-pnor-code-mgmt/
[4]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/57b878d048f929643276f1bf7fdf750abc4bde8b/xyz/openbmc_project/Software/Version.interface.yaml#L14
[5]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Software/Activation.interface.yaml
