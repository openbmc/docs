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

Typical platforms provide baseboard HW identification data as a FRU storage blob
held in a physical EEPROM.

[As descrfibed in Entity-Manager documentation](https://github.com/openbmc/entity-manager/blob/master/docs/my_first_sensors.md),
this blob is discovered and read over I2C, then is decoded before the data is
copied to D-Bus as properties of the 'xyz.openbmc_project.FruDevice' interface
by Entity-Manager's fru-device daemon.

Once the data is made available on D-Bus, it can be referenced by the
Entity-Manager configuration 'probe' statements that are used as
hardware/configuration detection triggers.

HPE platforms in Gen 10 and earlier provided HW identification data through a
standard I2C-acessible EEPROM (but still in a proprietary data format). That
avenue is no longer available on newer HPE systems.

GXP-based Gen 11 HPE platforms do not include a BMC-accessible EEPROM to provide
hardware ID data. Instead, baseboard HW data is stored in a secure element with
no direct BMC access.

Early in the boot process, an HPE-propietary bootblock is validated, then an
HPE-specific bootloader in the block fetches the HW ID blob from the secure
element. The blob is copied to a predetermined location in RAM, where it is
retrieved and parsed by HPE-specific code in u-Boot (open-sourced, and avaliable
at https://github.com/HewlettPackard/gxp-uboot).

u-Boot then makes the data avaliable to BMC via modifications to the 'flat'
device tree, starting with ['model' and 'serial-number,' which both have
well-known paths in the device tree root]
(https://github.com/devicetree-org/devicetree-specification/blob/main/source/chapter3-devicenodes.rst#root-node).

From there, the GXP HW ID data must be published to D-Bus, or there will be no
way for Entity-Manager to reference it during the probe evaluations used in
detecting supported hardware configurations. The contents of 'device-tree/model'
being made avaliable on d-Bus will be enough to enable basic Entity-Manager
identification of HPE baseboard hardware.

This document discusses leveraging a 'device tree -> D-Bus' daemon in
Entity-Manager to enable BMC detection and handling of HPE Gen 11 hardware (and
potentialy for other hardware as well).

In the future, once device tree 'model' HW detection has been implimented, this
section will be updated to describe retrieving additional HPE HW data provided
through less well-defined devivce tree nodes.

## Requirements

- Must be able to retrieve descriptive HW ID data avaliable through device tree

- HW ID data must be parsed and published to D-Bus

- HW ID properties should be referenced in Entity-Manager config 'probe'
  statement(s) in order to key configuration enablement off them.

## Proposed Design

High Level Flow:

- At boot, a propietary HPE bootblock is validated, then HPE GXP-bootloader code
  (propietary) included in the block retrieves data from a secure element.
  Specific hardware identifying information is transferred into a pre-determined
  memory location in RAM for pickup.

- u-Boot code (via an
  [open source, but HPE-specific process](https://github.com/HewlettPackard/gxp-uboot))
  moves HW ID information from RAM location and makes it avaliable to BMC under
  /proc/device-tree/

- [A daemon in Entity-Manager](https://gerrit.openbmc.org/c/openbmc/docs/+/65678)
  retrieves HW ID data from known device tree paths, then parses and publishes
  to D-Bus

- Entity-Manager probes that reference property names from the chosen-HW-ID
  interface on D-Bus can key off GXP hardware data and react accordingly.

## Alternatives Considered

- All HW ID data is written to file paths in /proc/device-tree/chosen/

  - Pros:
  - Is a well-established device-tree node
  - Is not being used by other BMC platforms, so easy to enable/disable service
    based on whether or not 'chosen' path exists
  - Values can be set from u-Boot via modifications to the 'flat' device tree
    avaliable to it (applicable to arbitrary device-tree paths)
  - Cons:
  - Not a 'usual' location for passing this kind HW-tied data. Unlikely to ever
    be used for HW ID data by other entities, so such a daemon would be defacto
    HPE-specific code.
  - Presenting HW-specific data here appears to go against the intended use of
    'chosen' node

- HW ID data is published to u-Boot enviornmental variables, then transfered to
  D-Bus by making use of phosphor-uboot-env-mgr

  - Pro:
    - Would only affect images explictily importing phosphor-uboot-env-mgr (no
      platforms use it in any publicly avaliable recipes at this time)
  - Cons:
    - Requires a potentially longer-than-preferred chaining of d-bus services
  - Saves a negligible amount of cycles at Entity-Manager startup for unaffected
    platforms

- Use an HPE-specific daemon in Entity-Manager or in an HPE-specific repo
  instead of a generalized '[channel]-> D-Bus' daemon

  - Pro:
    - Simpler (slightly?) code
  - Would only fire for HPE platforms
  - Cons:
    - Doesn't save much effort
    - Harder to leverage code for potential future cases (like for a planned
      'GPIO -> D-Bus' daemon for more Entity-Manager probe options)

- Use an HPE-specific u-Boot -> D-Bus daemon in phosphor-u-boot-env-mgr (or
  elsewhere) instead of an Entity-Manager-based solution
  - Pros:
    - Further reduced (already minimal) impact to most platforms: No new
      servivce or daemons added to Entity-Manager. phosphor-u-boot-env-mgr must
      be explicitly imported, and no platforms have public recipes importing
      this subproject at this time.
  - Cons:
    - Could feel 'messy' to have HPE-specific 'HW ID -> D-Bus' daemon code in
      something otherwise unrelated like phosphor-uboot-env-mgr _just_ because
      HPE utilizes u-Boot for passing data and this is a shorter path than some
      alternatives (such as relying on D-Bus calls from EM daemons to get data
      from u-Boot env variables)

## Impacts

The above-described process is only intended to scan selected paths -if they
exist- once during Entity-Manager startup.

If the path(s) in question do not exist (device-tree/model or
device-tree/serial-number), no further work will be done, so there is expected
to be no noticible performance or functional impact to platforms that don't rely
on this service for Entity-Manager HW/config detection.

## Organizational:

No new repository should be needed.

## Testing

Can be tested on a GXP system by setting an Entity-Manager config's probe to
trigger on match with a known GXP-HW identifying property, then by confirming
'busctl tree xyz.openbmc_project.EntityManager' shows a listing of objects that
matches what's described in the modified EM config file.
