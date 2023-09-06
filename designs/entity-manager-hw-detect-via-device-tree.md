# Entity-Manager HW Detect via Device-Tree Properties

Author: Christopher Sides (citysides)

Other contributors:

Created: September 14, 2023

## Problem Description

---

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
by Entity-Manager's fru-device daemon. The current fru-device daemon is able to
decode IPMI-FRU storage formatted blobs, as well as the Tyan data format.

Additionally, a separate daemon for handling OpenPower format VPD is hosted in
the 'openpower-vpd-parser' subproject. This daemon is relied for the
identification of certain IBM hardware.

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
element via third party driver. The driver is only available inside the
bootblock due to vendor licensing restrictions.

From here, the data blob is copied to a predetermined location in RAM, where it
is retrieved and parsed by HPE-specific code in u-Boot (open-sourced, and
available at https://github.com/HewlettPackard/gxp-uboot). In the future, we'll
be aiming to upstream as much 'HPE-critical' u-boot code as needed so that an
HPE-specific u-boot fork will not be needed to boot HPE hardware.

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

Additional data from HPE's VPD blob (like board revision identifying data) will
be made available on D-Bus through a separate daemon described in a separate
OEM/HPE design document.

In the future, HPE's behind-the-scenes reliance on u-Boot for this process may
be reduced or removed entirely, but but those implementation details should not
affect how a service would collect data from the device-tree passed to the
kernel from u-Boot (which is why we're looking at hosting this daemon in
phosphor-uboot-env-mgr)

This document discusses leveraging a 'device-tree -> D-Bus' daemon in
Entity-Manager to enable BMC detection and handling of HPE Gen 11 hardware (and
potentially for other hardware as well).

## Requirements

- Vital product data must be parsed and published to D-Bus

- D-Bus properties of interface `xyz.openbmc_project.BMCPrebootContext` may be
  populated using retrieved VPD values as appropriate to offer a D-Bus path for
  common specific model identifying details.

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

- [A daemon in Entity-Manager](<(https://gerrit.openbmc.org/c/openbmc/docs/+/65678)>)
  retrieves product data from known device tree paths, then parses and publishes
  to D-Bus

- Entity-Manager probes that reference property names from the
  `xyz.openbmc_project.BMCPrebootContext` interface (YAML documentation pending)
  on D-Bus can key off GXP hardware data and react accordingly.

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

## Impacts

The above-described process is only intended to scan selected paths -if they
exist- once during phosphor-uboot-env-mgr startup.

If the path(s) in question do not exist (device-tree/model or
device-tree/serial-number), no further work will be done, so there is expected
to be no noticeable performance or functional impact to platforms that don't
rely on this service for Entity-Manager HW/config detection.

## Organizational:

No new repository will be needed, but new maintainers fro HPE may be added to
the phosphor-uboot-env-mgr subproject.

Since HPE will be relying on code in phosphor-uboot-env-mgr for critical
functionality (HW identification), and since no other projects currently make
use of the phosphor-uboot-env-mgr subproject, HPE will provide new maintainers
to watch over the repo, starting with Christopher Sides
(Christopher.Sides@hpe.com, @CitySides on Discord). That roster may expand as
needed.

## Testing

To start, we will use GTest to create unit tests for the daemon code.

At this time, HPE hardware requires forked (publicly available) OBMC components
to fully boot, so automated testing of HPE hardware against 'mainstream' OBMC
images is not yet a viable option. Enabling this type of testing is a longer
term goal.

Practically, the whole of the service stack can be validated on an HPE system by
setting an Entity-Manager config's probe to trigger on match with a known HPE-HW
identifying property, then by confirming
`busctl tree  xyz.openbmc_project.EntityManager` shows a listing of objects that
matches what's described in the modified EM config file.

Any non-HPE platform that has a device-tree 'model' or 'serial-number' node
populated can be checked in automated testing by confirming the contents of the
device-tree/model and/or serial-number node matches the contents of 'model' and
'serial-number' properties of D-Bus interface
'xyz.openbmc_project.BMCPrebootContext'
