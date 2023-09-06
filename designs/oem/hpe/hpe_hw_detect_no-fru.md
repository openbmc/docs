# Detecting HPE HW w/o EEPROM-based FRU data

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
copied to D-Bus as properties of the 'xyz.openbmc_project.FruDevice' interface
by Entity-Manager's fru-device daemon.

Once the data is made available on D-Bus, it can be referenced by the
Entity-Manager configuration 'probe' statements that are used as
hardware/configuration detection triggers.

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
I2C-acessible EEPROM (but still in a proprietary data format). That avenue is no
longer available on newer HPE systems.

For GXP-based Gen 11 HPE paltforms, propietary HPE-controlled bootblock firmware
communiates with a secure element to provide necessary VPD. This process uses a
driver whose implimentation is strictly license-controlled and cannot be
publically released. The open source portions of the boot chain must adapt to
the bootblock's reserved memory application binary interface (ABI) for providing
VPD from the secure element.

Early in the boot process, an HPE-proprietary bootblock is validated, then an
HPE-specific bootloader in the block fetches the VPD blob from the secure
element via third party driver. The driver is only available inside the
bootblock due to vendor licensing restrictions.

From here, the data blob is copied to a predetermined location in RAM, where it
is retrieved and parsed by HPE-specific code in u-Boot (open-sourced, and
available at https://github.com/HewlettPackard/gxp-uboot).

Additionally, code in u-Boot gathers MAC addresses for the BMC's network
interfaces, along with retrieving a 'server_id' value from an embedded
CPLD-based memory device. This value is unique to each model of HPE platform,
and will be used for platform-wide identification. In comparison, the data in
the aforementioned blob may be used for recognizing finer details like specific
board revisions for a given platform.

u-Boot then makes relevant data available to BMC via modifications to the 'flat'
device tree, setting the values for
['model' and 'serial-number,' which both have well-known paths in the device tree root](dt-spec-chap3-root-node),
along with
['local-mac-address,' a well-known device binding](dt-spec-chap4-device-bindings).

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
affect how BMC picks up data from device-tree.

This document discusses leveraging a 'device-tree -> D-Bus' daemon in
Entity-Manager to enable BMC detection and handling of HPE Gen 11 hardware (and
potentially for other hardware as well).

## Requirements

- Must be able to retrieve descriptive product data available through well-known
  device tree handles

- Vital product data must be parsed and published to D-Bus

- D-Bus proprties of interface
  '[xyz.openbmc_project.Inventory.Decorator.Compatible](<(obmc-inventory-decorator)>)'
  may be populated using retrieved VPD values as appropriate to offer a
  well-known D-Bus path for common specific model identifying details.

[obmc-inventory-decorator]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Inventory/Decorator/Compatible.interface.yaml

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
  CPLD-based memory device, and makes it avaliable to BMC under
  /proc/device-tree/

- [A daemon in Entity-Manager](https://gerrit.openbmc.org/c/openbmc/docs/+/65678)
  retrieves product data from known device tree paths, then parses and publishes
  to D-Bus

- Entity-Manager probes that reference property names from the
  xyz.openbmc_project.DeviceTree interface on D-Bus can key off GXP hardware
  data and react accordingly.

## Alternatives Considered

- **Entity-Manager probes are extended to handle reading data from arbitrary
  device-tree nodes**

  - **Pros:**
    - a) No new daemon required
  - **Cons:**
    - a) May encourage undesired device-tree explotation

- **A reserved-memory node in device-tree is used for transport of
  'serial-number' and 'model' ID data**

  - **Pros:**
    - a) Fewer daemons, since HPE is already looking at using reserved-memory
      for HPE-specific VPD properties like board revision IDs (to be discussed
      in a separate OEM/HPE/ design doc)
    - b) Reserved-memory is commonly used for passing data from firmware
  - **Cons:**
    - a) Has no well-known handles for common properties like device-tree/model
      or serial-number like device-tree does; makes more sense for keeping
      arbitrary data instead of 'well-known' properties
    - b) The critical identifier HPE wants to rely on for platform
      identification (device-tree/model) comes from a different
      behind-the-scenes source vs. the rest of the data planned for output
      through reserved-memory. Using reserved-memory for 'model' instead of the
      well known device-tree 'model' handle would add additional complexity.
    - c) Since device-tree/model is well described in device-tree documentation,
      there is potential for non-HPE platform to make easy use of it. Relying on
      'model' as an arbitrary property in reserved-memory has less potential to
      be useful outside of HPE-specific applications.

- **All VPD is exposed as devicetree attributes under
  /proc/device-tree/chosen/**

  - **Pros:**
    - a) Is a well-established device-tree node
    - b) Is not being used by other BMC platforms, so easy to enable/disable
      service based on whether or not 'chosen' path exists
    - c) Values can be set from u-Boot via modifications to the 'flat' device
      tree available to it (applicable to arbitrary device-tree paths)
  - **Cons:**
    - a) Not a 'usual' location for passing this kind HW-tied data. Unlikely to
      ever be used for moving product data by other entities, so such a daemon
      would be defacto HPE-specific code.
    - b) Presenting HW-specific data here appears to go against the intended use
      of 'chosen' node

- **Vital product data is published to u-Boot enviornmental variables, then
  transfered to D-Bus by making use of phosphor-uboot-env-mgr**

  - **Pro:**
    - a) Would only affect images explictily importing phosphor-uboot-env-mgr
      (no platforms use it in any publicly avaliable recipes at this time)
  - **Cons:**
    - a) Requires a potentially longer-than-preferred chaining of d-bus services
    - b) Saves a negligible amount of cycles at Entity-Manager startup for
      unaffected platforms

- **Use an HPE-specific daemon in Entity-Manager or in an HPE-specific repo
  instead of a generalized '[channel]-> D-Bus' daemon**

  - **Pro:**
    - a) Simpler (slightly?) code
    - b) Would only fire for HPE platforms
  - **Cons:**
    - a) Doesn't save much effort
    - b) Harder to leverage code for potential future cases (like for a planned
      'GPIO -> D-Bus' daemon for more Entity-Manager probe options)

- **Use an HPE-specific u-Boot -> D-Bus daemon in phosphor-u-boot-env-mgr (or
  elsewhere) instead of an Entity-Manager-based solution**

  - **Pros:**
    - a) Further reduced (already minimal) impact to most platforms: No new
      servivce or daemons added to Entity-Manager. phosphor-u-boot-env-mgr must
      be explicitly imported, and no platforms have public recipes importing
      this subproject at this time.
  - **Cons:**
    - a) Could feel 'messy' to have HPE-specific 'VPD -> D-Bus' daemon code in
      something otherwise unrelated like phosphor-uboot-env-mgr _just_ because
      HPE utilizes u-Boot for passing data and this is a shorter path than some
      alternatives (such as relying on D-Bus calls from EM daemons to get data
      from u-Boot env variables)

## Impacts

The above-described process is only intended to scan selected paths -if they
exist- once during Entity-Manager startup.

If the path(s) in question do not exist (device-tree/model or
device-tree/serial-number), no further work will be done, so there is expected
to be no noticeable performance or functional impact to platforms that don't
rely on this service for Entity-Manager HW/config detection.

## Organizational:

No new repository should be needed.

## Testing

Can be tested on a GXP system by setting an Entity-Manager config's probe to
trigger on match with a known GXP-HW identifying property, then by confirming
'busctl tree xyz.openbmc_project.EntityManager' shows a listing of objects that
matches what's described in the modified EM config file.
