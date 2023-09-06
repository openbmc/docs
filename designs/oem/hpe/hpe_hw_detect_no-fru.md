# Detecting HPE HW w/o EEPROM-based FRU data

Author: Christopher Sides (citysides)

Other contributors:

Created: September 14, 2023

## Problem Description

BMC needs a process to identify and handle HPE GXP baseboards that provide HW ID
data via a non-I2C channel (passing instead through u-Boot interactions) and in
a propietary format that is not covered by Entity-Manager's 'fru_device' daemon
that most platforms rely on.

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
standard I2C-acessible EEPROM (but still in a propietary data format). That
avenue is no longer available on newer HPE systems.

GXP-based Gen 11 HPE platforms do not include a BMC-accessible EEPROM to provide
hardware ID data. Instead, baseboard HW data is stored in a secure element with
no direct BMC access. A proprietary HPE process retrieves the data early in
boot, then copies selected hardware-identifying fields to a pre-determined
memory location in RAM, where it is then retrieved by u-Boot using an open
source (but HPE-specific) process.

From there, the GXP HW ID data must be published to D-Bus, or there will be no
way for Entity-Manager to reference it during the probe evaluations used in
detecting supported hardware configurations.

This document discusses leveraging a u-Boot -> D-Bus daemon in Entity-Manager as
described in the
[u-Boot_FRU_Device_Alternative design doc](https://gerrit.openbmc.org/c/openbmc/docs/+/65678)
to enable BMC detection and handling of HPE Gen 11 hardware.

## Requirements

- Must be able to retrieve HW ID data stored in u-Boot as env variables

- ID data must be parsed and published to D-Bus before Entity-Manager
  configuration probes are evaluated

- An HPE-specific D-Bus interface YAML must be published to
  [phosphor-dbus-interfaces](https://github.com/openbmc/phosphor-dbus-interfaces)
  with HPE-specific fields such as 'Server_ID.' Objects with this interface
  would be added under the 'xyz.openbmc_project.FruDeviceAlternative' D-Bus
  namespace.

- HPE-specific HW ID properties should be referenced in Entity-Manager config
  'probe' statement(s) in order to key configuration enablement off them.

## Proposed Design

High Level Flow:

- At BMC boot -before u-Boot comes into play- a proprietary HPE process
  retrieves data from a secure element. Specific hardware identifying
  information is transferred into a pre-determined memory location in RAM for
  pickup.

- u-Boot code (via an open source, but HPE-specific process) moves HW ID
  information from RAM location into u-Boot environmental variables.

- [u-Boot_FRU_Device_Alternative daemon in Entity-Manager](https://gerrit.openbmc.org/c/openbmc/docs/+/65678)
  retrieves GXP HW ID data from u-Boot, then parses and publishes to D-Bus

- Entity-Manager probes that reference property names from the HPE device
  namespace on D-Bus can key off GXP hardware data and react accordingly.

## Alternatives Considered

- u-Boot publishes HW ID data to filesystem instead of saving as u-Boot env
  variables

  - Pro:
    - no reliance on posphor-u-boot-env-mgr
  - Cons:
    - uses file system for IPC
    - requires similar level of effort

- Use an HPE-specific daemon in Entity-Manager instead of a 'generalized' u-Boot
  -> D-Bus daemon

  - Pro:
    - Simpler (slightly?) code
  - Cons:
    - Doesn't save much effort
    - Harder to leverage code for potential future cases

- Use an HPE-specific u-Boot -> D-Bus daemon in phosphor-u-boot-env-mgr (or
  elsewhere) instead of an Entity-Manager-based solution
  - Pros:
    - Further reduced (already minimal) impact to most platforms: No new
      servivce or daemons added to Entity-Manager. phosphor-u-boot-env-mgr must
      be explicitly imported, and no platforms have public recipes importing
      this subproject at this time.
  * Fewer dependencies if Entity-Manager doesn't need to rely on
    phosphor-u-boot-env-mgr service: HPE hardware detection relies on moving
    data from u-Boot to D-Bus, and this subproject is focused on u-Boot
    interaction via D-Bus
  - Cons:
    - Makes more logical sense to keep hardware ID data -> D-Bus daemons grouped
      together under Entity-Manager, rather than spreading them around to
      'random' sub-projects like posphor-u-boot-env-mgr

## Impacts

No major impacts expected. At the time of this writing, no public projects
actually include phosphor-u-boot-env-mgr in a recipe, and the proposed service &
daemon will not run otherwise.

Aditionally, platforms that make use of the phosphor-u-boot-env-mgr subproject
will only encounter a low-impact file existence check (fired one time at
Entity-Manager startup) for as many 'types' as are supported. To start, HPE's
GXP 'type' will be the only one to start, and may well never be any other
subscribers.

## Organizational:

No new repository should be needed.

## Testing

Can be tested on a GXP system by setting an Entity-Manager config's probe to
trigger on match with a known GXP-HW identifying property, then by confirming
'busctl tree xyz.openbmc_project.EntityManager' shows a listing of objects that
matches what's described in the modified EM config file.
