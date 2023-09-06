# Detecting HPE HW w/o EEPROM-based FRU data

Author: Christopher Sides (citysides)

Other contributors:

Created: September 14, 2023

## Problem Description

BMC needs a process to identify and handle HPE GXP baseboards that provide HW ID
data via a non-I2C channels and in a proprietary format that is not covered by
Entity-Manager's 'fru-device' daemon that most platforms rely on.

## Background and References

Typical platforms provide baseboard HW identification data as a FRU storage blob
held in a physical EEPROM.

[As descrfibed in Entity-Manager documentation](https://github.com/openbmc/entity-manager/blob/master/docs/my_first_sensors.md),
this blob is discovered and read over I2C, then is decoded before the data is
copied to D-Bus object properties under the 'xyz.openbmc_project.FruDevice' name
by Entity-Manager's fru_device daemon.

Once the data is made available on D-Bus, it can be referenced by the
Entity-Manager configuration 'probe' statements that are used as
hardware/configuration detection triggers.

HPE platforms in Gen 10 and earlier provided HW identification data through a
standard I2C-acessible EEPROM (but still in a proprietary data format). That
avenue is no longer available on newer HPE systems.

GXP-based Gen 11 HPE platforms do not include a BMC-accessible EEPROM to provide
hardware ID data. Instead, baseboard HW data is stored in a secure element with
no direct BMC access.

Early in the boot process, a blob is retrieved from the SE by HPE-specific code
in u-BootBlock (proprietary) and copied to a predetermined location in RAM,
where it is retrieved and parsed by HPE-specific code in u-Boot (open-sourced,
https://github.com/HewlettPackard/gxp-uboot)

u-Boot then makes the data avaliable through a device tree overlay in
/proc/device-tree/chosen/ with relevant file handles for each entity.
https://www.kernel.org/doc/Documentation/devicetree/bindings/chosen.txt

From there, the GXP HW ID data must be published to D-Bus, or there will be no
way for Entity-Manager to reference it during the probe evaluations used in
detecting supported hardware configurations.

This document discusses leveraging a 'chosen-node -> D-Bus' daemon in
Entity-Manager as described in the
[chosenHWID design doc](https://gerrit.openbmc.org/c/openbmc/docs/+/65678) to
enable BMC detection and handling of HPE Gen 11 hardware.

## Requirements

- Must be able to retrieve HW ID data passed through u-Boot

- ID data must be parsed and published to D-Bus before Entity-Manager
  configuration probes are evaluated

- An HPE-specific D-Bus interface YAML must be published to
  [phosphor-dbus-interfaces](https://github.com/openbmc/phosphor-dbus-interfaces)
  with HPE-specific fields such as 'Server_ID.' Objects with this interface
  would be added under the 'xyz.openbmc_project.chosen-args' D-Bus interface.

- HPE-specific HW ID properties should be referenced in Entity-Manager config
  'probe' statement(s) in order to key configuration enablement off them.

## Proposed Design

High Level Flow:

- At BMC boot -before u-Boot comes into play- HPE code in u-BootBlock
  (proprietary) retrieves data from a secure element. Specific hardware
  identifying information is transferred into a pre-determined memory location
  in RAM for pickup.

- u-Boot code (via an open source, but HPE-specific process) moves HW ID
  information from RAM location and makes them avaliable to BMC at
  /proc/device-tree/chosen/.

- [chosen-node daemon in Entity-Manager](https://gerrit.openbmc.org/c/openbmc/docs/+/65678)
  retrieves GXP HW ID data from 'chosen node' then parses and publishes to D-Bus

- Entity-Manager probes that reference property names from the chosen-HW-ID
  interface on D-Bus can key off GXP hardware data and react accordingly.

## Alternatives Considered

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

If the path(s) in question do not exist, no further work will be done, so there
is not expected to be any noticible performance or functional impact to
platforms that don't rely on this service for Entity-Manager HW/config
detection.

## Organizational:

No new repository should be needed.

## Testing

Can be tested on a GXP system by setting an Entity-Manager config's probe to
trigger on match with a known GXP-HW identifying property, then by confirming
'busctl tree xyz.openbmc_project.EntityManager' shows a listing of objects that
matches what's described in the modified EM config file.
