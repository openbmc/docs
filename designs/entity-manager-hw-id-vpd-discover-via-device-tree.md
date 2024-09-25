# Entity-Manager HW ID: VPD Discovery via Device-Tree Properties

Author: Christopher Sides (citysides)

Other contributors:

Created: September 14, 2023

## Problem Description

BMC needs a process to identify and handle HPE GXP baseboards that provide HW ID
data via non-I2C channels and in a proprietary format that is not covered by
Entity-Manager's 'fru-device' daemon that most platforms rely on.

This proposal describes a method of handling for hardware where HW ID data is
gathered from device tree file paths for Entity-Manager consumption.

## Background and References

Typical platforms provide HW ID data - often referred to as 'vital product data'
(VPD) - for the baseboard as a FRU storage blob held in a physical EEPROM.

[As described in Entity-Manager documentation](https://github.com/openbmc/entity-manager/blob/master/docs/my_first_sensors.md),
this blob is discovered and read over I2C, then is decoded before the data is
copied to D-Bus as properties of the `xyz.openbmc_project.FruDevice` interface
by Entity-Manager's fru-device daemon. The current FRU-device daemon is able to
decode IPMI-FRU storage formatted blobs, as well as the Tyan data format.

Additionally, a separate daemon for handling OpenPower format VPD is hosted in
the 'openpower-vpd-parser' subproject. This daemon is relied on for the
identification of certain IBM-supported hardware.

HPE hardware uses different channels and data formats from the above, relying on
modifications to the device-tree to output properties as one file handle per
attribute.

Once VPD is made available on D-Bus, it can be referenced by the Entity-Manager
configuration 'probe' statements that are used as hardware/configuration
detection triggers.

HPE platforms provide the following properties for baseboard VPD:

- server_id (platform model id)
- pca_part_number (baseboard part no)
- pca_serial_number (baseboard serial no)
- part_number (platform product no, set at assembly by factory)
- serial_number (system serial no, assigned at assembly by factory)
- mac0 (if present: MAC address for dedicated BMC network interface)
- mac1 (if present: MAC address for secondary network interface that may or may
  not be shared with host)

HPE platforms in Gen 10 and earlier provided VPD through a standard
I2C-accessible EEPROM (but still in a proprietary data format). That avenue is
no longer available on newer HPE systems.

For GXP-based Gen 11 HPE platforms, proprietary HPE-controlled bootblock
firmware communicates with a secure element to provide necessary VPD. This
process uses a driver whose implementation is strictly license-controlled and
cannot be publicly released. The open source portions of the boot chain must
adapt to the bootblock's reserved memory application binary interface (ABI) for
providing VPD from the secure element.

Early in the boot process, an HPE-proprietary bootblock is validated, then an
HPE-specific bootloader in the block fetches the VPD blob from the secure
element via third party driver. From here, the data blob is copied to a
predetermined location in RAM, where it is retrieved and parsed by HPE-specific
code in u-Boot (open-sourced, and available at
https://github.com/HewlettPackard/gxp-uboot). In the future, we'll be aiming to
upstream as much 'HPE-critical' u-boot code as needed so that an HPE-specific
u-boot fork will not be needed to boot HPE hardware.

Additionally, code in u-Boot gathers MAC addresses for the BMC's network
interfaces, along with retrieving a 'server_id' value from an embedded
CPLD-based memory device. This value is unique to each model of HPE platform,
and will be used for platform-wide identification. In comparison, the data in
the aforementioned blob may be used for recognizing finer details like specific
board revisions for a given platform.

The HPE process described above relies on shared memory instead of u-boot env
variables because the HPE bootloader does not know that u-boot will run next,
and does not know where the u-boot variable store is, if it exists at all --
since a customer could decide to use something other than u-boot, or to store
their environmental variables in a different place or format.

After that, u-Boot makes relevant data available to Linux via modifications to
the flattened device tree, setting the values for ['/model' and
'/serial-number,' which both have well-known paths in the device tree
root][dt-spec-chap3-root-node], along with ['local-mac-address,' a property
provided by the network binding][dt-spec-chap4-device-bindings].

[dt-spec-chap3-root-node]:
  https://github.com/devicetree-org/devicetree-specification/blob/main/source/chapter3-devicenodes.rst#root-node
[dt-spec-chap4-device-bindings]:
  https://github.com/devicetree-org/devicetree-specification/blob/main/source/chapter4-device-bindings.rst#local-mac-address-property

From there, the GXP product data must be published to D-Bus, or there will be no
way for Entity-Manager to reference it during the probe evaluations used in
detecting supported hardware configurations. The contents of 'device-tree/model'
being made available on D-Bus will be enough to enable basic Entity-Manager
identification of HPE baseboard hardware.

In the future, HPE's behind-the-scenes reliance on u-Boot for this process may
be reduced or removed entirely, but but those implementation details should not
affect how a service would collect data from the device-tree passed to the
kernel from u-Boot. One alternative to modifying the flat device-tree would be
to have values like 'model' set in the platform's device-tree source at image
build time.

This document discusses leveraging a 'device-tree -> D-Bus' daemon in
Entity-Manager to enable BMC detection and handling of HPE Gen 11 hardware (and
potentially for other hardware as well).

## Requirements

- Vital product data must be parsed and published to D-Bus

- D-Bus properties for interface(s) at path `xyz/openbmc_project/MachineContext`
  may be populated using retrieved VPD values as appropriate to offer a D-Bus
  path for common specific model identifying details.

- Vital product data properties should be referenced in Entity-Manager config
  'probe' statement(s) in order to key configuration enablement off them.

## Proposed Design

High Level Flow:

- At boot, a proprietary HPE bootblock is validated, then HPE GXP-bootloader
  code (proprietary) included in the block retrieves data from a secure element.
  Specific hardware identifying information is transferred into a pre-determined
  memory location in RAM for pickup.

- u-Boot code (via an
  [open source, but HPE-specific process](https://github.com/HewlettPackard/gxp-uboot))
  gathers MAC addresses and moves product data from the RAM location, plus a
  CPLD-based memory device, and exposes it in Linux under /proc/device-tree/

- A daemon in DeviceTree-VPD-Parser retrieves product data from known device
  tree paths, then parses and publishes to D-Bus

- Entity-Manager probes that reference properties from the
  `xyz.openbmc_project.Inventory.Decorator.Asset` (.Model & .SerialNumber
  properties) interface on D-Bus can key off GXP hardware data and react
  accordingly.

## Alternatives Considered

### Entity-Manager probes are extended to handle reading data from arbitrary<br/>device-tree nodes

#### Pros:

1. No new daemon required

#### Cons:

1. May encourage undesired device-tree exploitation

### A reserved-memory node in device-tree is used for transport of<br/>'serial-number' and 'model' ID data

#### Pros:

1. Reserved-memory is commonly used for passing data from firmware

#### Cons:

1. Has no well-known handles for common properties like /model or /serial-number
   like device-tree does; makes more sense for keeping arbitrary data instead of
   'well-known' properties
2. The critical identifier HPE wants to rely on for platform identification
   (/model) comes from a different behind-the-scenes source vs. the rest of the
   data planned for output through reserved-memory. Using reserved-memory for
   'model' instead of the well known device-tree /model handle would add
   additional complexity.
3. Since /model is well described in device-tree documentation, there is
   potential for non-HPE platform to make easy use of it. Relying on 'model' as
   an arbitrary property in reserved-memory has less potential to be useful
   outside of HPE-specific applications.

### All VPD is exposed as devicetree attributes under /chosen/

#### Pros:

1. Is a well-established device-tree node
2. Is not being used by other BMC platforms, so easy to enable/disable service
   based on whether or not 'chosen' path exists
3. Values can be set from u-Boot via modifications to the flattened device tree
   available to it (applicable to arbitrary device-tree paths)

#### Cons:

1. Not a 'usual' location for passing this kind HW-tied data. Unlikely to ever
   be used for moving product data by other entities, so such a daemon would be
   defacto HPE-specific code.
2. Presenting HW-specific data here appears to go against the intended use of
   'chosen' node

### Vital product data is published to u-Boot environment variables, then<br/>transferred to D-Bus

#### Pros:

1. Would increase code reuse and testing

#### Cons:

1. Potential issues with limited writes to memory where u-boot env variables are
   stored in memory.
2. HPE component that retrieves VPD from HPE's secure element has no
   knowledge/ability to alter u-boot environmental variables

### Use an HPE-specific daemon instead of a generalized'[channel]-> D-Bus'<br/>daemon

#### Pros:

1. Simpler (slightly?) code
2. Would only fire for HPE platforms
3. Better abstraction

#### Cons:

1. Doesn't save much effort compared to a generalized non-HPE-specific solution
2. Harder to leverage code for potential future cases

### Service is hosted in Entity-Manager [ACCEPTED after TOF re-examination]

#### Pros:

1. Would operate in parallel to existing Fru_Device daemon

#### Cons:

1. Community doesn't want to host VPD -> DBus daemons in Entity-Manager repo

Follow-up: A Technical Oversight Forum discussion has reached consensus that it
would be better to host this device-tree VPD daemon in Entity-Manager alongside
the existing fru-device daemon rather than to create a dedicated repo for a
daemon as simple as this one, or to force into another even less-related repo.

https://github.com/openbmc/technical-oversight-forum/issues/38

### Service is hosted in Phosphor-u-boot-env-mgr

#### Pros:

1. Repo is already associated with u-boot operations

#### Cons:

1. Current repo maintainer feels the service doesn't fit this repo because the
   purpose is explicitly to deal with manipulation of u-boot env variables,
   which this service is not involved with.

   https://gerrit.openbmc.org/c/openbmc/phosphor-u-boot-env-mgr/+/71512

### Addendum: Rejected D-Bus interfaces for pending device-tree -> D-Bus<br/>daemon

#### Existing interfaces (rejected):

1. Proposal: xyz.openbmc_project.Decorator.\*

   Rejection: "doesn't represent anything about the physical board" -Ed Tanous

   Followup: Phosphor-dbus-interfaces maintenance (Patrick Williams) has
   rejected hosting an interface with properties like 'Model' and 'SerialNumber'
   that already exist as part of other interfaces, and has specifically
   suggested relying on xyz.openbmc_project.Inventory.Decorator.Asset (for
   .model and .serialnumber properties) along with
   xyz.openbmc_project.Inventory.Item.NetworkInterface (for .MACAddress
   property).

   Discussion @
   https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/73260/comment/e0226ae4_d7e514dd/

   Follow-up 2: Sticking with Inventory.Decorator.Asset, but removing MAC
   address properties to reduce unneeded complexity.

2. Proposal: xyz.openbmc_project.FRUDevice

   Rejection: The data is not coming from a FRU device

#### Proposed interfaces (rejected):

1. Proposal: xyz.openbmc_project.DeviceTree
2. Proposal: xyz.openbmc_project.uBootHWID
3. Proposal: xyz.openbmc_project.GXPFRU

   Rejection: Above interfaces do not reflect underlying hardware

4. Proposal: xyz.openbmc_project.BMCPrebootContext

   Rejection: Arguably doesn't represent a hardware concept.

5. Proposal: xyz.openbmc_project.UbootDeviceModel

   Rejection: There are ongoing efforts to enable dynamic device-tree
   modifications post-u-boot. The nodes this daemon reads are unlikely to be
   affected, but there's no guarantee of that.

6. Proposal: xyz.openbmc_project.Platform

   Rejection: Some attributes like serial-number or MAC don't apply across a
   given platform

7. Proposal: xyz.openbmc_project.Machine

   Rejection: Promising, but the name seems vague re: its purpose.
   xyz.openbmc_project.MachineContext has been offered as an alternative.

8. Proposal: xyz.openbmc_project.MachineContext

   Rejection: "We have all this stuff [.Model, .SerialNumber, ect. properties] defined
   already. I'm not going to accept a new "bunch of random properties HPe thinks
   are important [today] globbed into a new interface" interface"

   - Patrick Williams, Phosphor-dbus-interfaces maintainer.

   Counter-proposal: "Reuse the existing dbus interfaces and put them at a
   well-defined location? Inventory.Decorator.[Asset] = Model
   Inventory.Decorator.Asset = SerialNumber Inventory.Item.NetworkInterace =
   MACAddress.." - Patrick Williams

   Discussion @
   https://gerrit.openbmc.org/c/openbmc/phosphor-dbus-interfaces/+/73260/comment/e0226ae4_d7e514dd/

   Follow-up: Landed on interface: xyz.openbmc_project.Inventory.Decorator.Asset
   @ path xyz/openbmc_project/MachineContext

## Impacts

The above-described process is only intended to scan selected paths -if they
exist- once during initial service startup.

If the path(s) in question do not exist (device-tree/model or
device-tree/serial-number), no further work will be done, so there is expected
to be no noticeable performance or functional impact to platforms that don't
rely on this service for Entity-Manager HW/config detection.

## Organizational:

### Does this design require a new repository?

No. After some discussion, a Technical Oversight Forum consensus was reached to
have the device-tree VPD parser daemon hosted in the Entity-Manager repo
alongside the existing fru-device daemon.

### Which repositories are expected to be modified to execute this design?

Technical Oversight Forums consensus has been reached that this device-tree VPD
daemon will be hosted in Entity-Manager alongside the existing FRU-device VPD
daemon.

Entity-Manager will also have configuration files added for HPE hardware that
make use of the new interface properties via probe statements.

## Testing

Any platform that has a device-tree 'model' or 'serial-number' node populated in
Device-Tree (this can be done by hardcoding a value into the DTS file before
building an image, if desired) can be checked by confirming the contents of the
device-tree/model and/or serial-number node matches the contents of 'model' and
'serial-number' properties of D-Bus interface at D-Bus path
'xyz/openbmc_project/MachineContext'

Practically, the whole of the service stack (including Entity-Manager probe
validation) of the new D-Bus path can be validated on an HPE system by setting
an Entity-Manager config's probe to trigger on match with a known HPE-HW
identifying property, then by confirming
`busctl tree  xyz.openbmc_project.EntityManager` shows a listing of objects that
matches what's described in the modified EM config file.

At this time, HPE hardware requires forked (publicly available) OBMC components
to fully boot, so automated testing of HPE hardware against 'mainstream' OBMC
images is not yet a viable option. Enabling this type of testing is a longer
term goal.
