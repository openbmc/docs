# HW detection w/ u-Boot-based FRU_Device_Alternative

Author: Christopher Sides (citysides)

Other contributors:

Created: September 14, 2023

## Problem Description

BMC needs ways to detect, parse, and make hardware identification data avaliable
over D-Bus for HW that does not provide HW ID info as FRU formated blobs over
I2C. Without the ability to process non-standard HW ID data, Entity-Manager will
be unable to detect and respond to hardware like HPE's GXP-based platforms.

This proposal describes a method of handling for hardware where HW ID data is
retrieved at boot from a device with no direct BMC access, then is transfered
through u-Boot to D-Bus.

## Background and References

Typical platforms provide baseboard HW ID data as a FRU storage blob held in an
EEPROM, and accessible by the BMC over I2C (or possibly as a blob stored on the
filesystem).

[As described in the Entity-Manager documentation](https://github.com/openbmc/entity-manager/blob/master/docs/my_first_sensors.md),
EEPROMs are discovered and read over I2C, then is decoded before the data is
copied to D-Bus object properties under the 'xyz.openbmc_project.FruDevice' name
by Entity-Manager's fru_device daemon.

Once the data is made available on D-Bus, it can be referenced by the
Entity-Manager configuration 'probe' statements that are used for
hardware/configuration detection.

Some HW, such as
[HPE's GXP baseboards](https://gerrit.openbmc.org/c/openbmc/docs/+/66369), may
provide ID data over alternative channels - such being passed through u-Boot
instead of over I2C - and in non-standard, potentially propietary data formats.

This proposal aims to leverage
[phosphor-u-boot-enviornmental-manager](https://github.com/openbmc/phosphor-u-boot-env-mgr)'s
u-Boot -> D-Bus data transfer functionality in order to bridge the gap from
u-Boot to make passed-through HW data available on D-Bus, where it can be acted
upon by Entity-Manager processes.

## Requirements

During BMC boot, before Entity-Manager probe statements are evaluated:

- u-Boot_FRU_Device_Alternative service must start only if the u-Boot
  Enviornmental Manager service is explicitly enabled and avaliable.

- Once service is started, u-Boot_FRU_Device_Alternative daemon must check
  (once) for supported-device identifiers, and will stop if none are found

- Daemon must recognize, parse, then transfer labled HW identification data from
  u-Boot enviornmental variables to D-Bus

- There should be a 'xyz.openbmc_project.FruDeviceAlternative' namespace on
  D-Bus for devices that need an alternative to FRUDevice handling for
  detection. If 'new' D-Bus interfaces or properties are required to contain
  specific ID data, then
  [phosphor-dbus-interfaces](https://github.com/openbmc/phosphor-dbus-interfaces)
  must be updated accordingly

- One or more Entity-Manager config files' probe statement should reference the
  appropriate bus and property name(s) in order to access the new data during EM
  probe evaluations.

## Proposed Design

High Level Flow:

[outside of OBMC control]

- Early in boot process, an intermediary retrieves HW ID data from a
  non-BMC-accessible device, then copies it to a predetermined memory location

- u-Boot -via manufacturer-specific code (open sourced in HPE's case)- moves HW
  ID information into u-Boot environmental variables

[OBMC controlled]

- Entity-Manager u-Boot_FRU_Device_Alternative service starts (only if u-Boot
  Env Mgr was imported at build and is enabled at runtime)
- u-Boot_Fru_Device_Alternative daemon checks for markers that indicate
  supported HW. Stops if none are found.

- Daemon makes a call over D-Bus to u-Boot Enviornment Manager for a list of
  u-Boot Env variables and stores them for parsing
- Results are parsed for HW ID data. Parsed data is transfered to an appropriate
  D-Bus object under a 'FRU_Device_Alternative' namespace. Interface should
  match what's described in phosphor-dbus-interfaces.
- Entity-Manager probes that reference properties from the
  FRU_Device_Alternative namespace on D-Bus are able to key off discovered HW ID
  data and react accordingly.

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

No major impacts expected. This is currently aimed as a one-time process at
Entity-Manager startup. The service in question will only start if
phosphor-u-boot-env-mgr is explicitly included and enabled in the build. No
platform as of this writing includes it in a recipe.

Once the service is started, the u-Boot_FRU_Device_Alternative daemon will first
check for compatible-HW identifying markers (such as checking for a specific
manufacturer-specific file in the device tree), and will stop if none are found.

Organizational:

No new repository should be needed.

## Testing

Test by confirming that the expected data is viewable on D-Bus after BMC boot,
and that Entity-Manager probe evaluations can trigger from that data.

- Boot OBMC on a system with FRU_Device_alternative copatible HW and
  Entity-Manager configs with probes that reference platform-specific HW ID data

- call 'busctl | grep fru -I' for a listing of FRU devices on D-Bus to confirm a
  device with the expected name is listed

- Call 'busctl introspect [FRU path] [FRU path of choice here]' and confirm the
  expected properties appear
