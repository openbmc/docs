# Enable detection of HPE HW w/ no EEPROM-based FRU data

Author: Christopher Sides (citysides)

Other contributors: 

Created: September 06, 2023

## Problem Description

BMC needs a process to identify and handle HPE GXP baseboards that provide 
hardware identification data in a non-standard format which is not covered by 
Entity-Manager's 'fru_device' daemon that most platforms rely on. 

## Background and References

Typical platforms provide baseboard HW identification data as a FRU storage
blob held in a physical EEPROM. 

[As described in the Entity-Manager documentation](https://github.com/openbmc/entity-manager/blob/master/docs/my_first_sensors.md), this blob is discovered
and read over I2C, then is decoded before the data is copied to D-Bus object
properties under the 'xyz.openbmc_project.FruDevice' name by Entity-Manager's
fru_device daemon. 

Once the data is made available on D-Bus, it can be referenced by the
Entity-Manager configuration 'probe' statements that are used for
hardware/configuration detection.

HPE platforms in Gen 10 and earlier provided HW identification data through
a standard I2C-acessible EEPROM, but that avenue is no longer available on
newer HPE systems.

GXP-based Gen 11 HPE platforms do not include a BMC-accessible EEPROM to
provide hardware ID data. Instead, baseboard HW data is stored in a secure
element with no direct BMC access. A proprietary HPE process retrieves the
data at BMC boot, then copies specific hardware-identifying fields to a
pre-determined memory location in RAM, where it is then retrieved by u-Boot
using an open source (but HPE-specific) process.

From there, the HW identification data must be made available through D-Bus,
or there will be no way for Entity-Manager to reference it during the probe
evaluations used in detecting supported hardware configurations. 

This proposal aims to leverage [phosphor-u-boot-enviornmental-manager](https://github.com/openbmc/phosphor-u-boot-env-mgr)'s
u-Boot -> D-Bus data transfer functionality in order to bridge the gap
from u-Boot to make GXP HW data available on D-Bus, where it can be
acted upon by Entity-Manager processes.


## Requirements	

During BMC boot, before Entity-Manager probe statements are evaluated:
* Should be able to detect when dealing with HPE's GXP baseboard HW

* Must discover and retrieve GXP HW identification data from u-Boot
  environmental vars  

* Must transfer the HW identification data from u-Boot to D-Bus 

* One or more Entity-Manager config files' probe statement should reference
  the appropriate bus and property name(s) to access the new data during
  probe evaluations.

## Proposed Design

High Level Flow: 

* At BMC boot -before u-Boot comes into play- a proprietary HPE process
  retrieves data from a secure element. Specific hardware identifying
  information is transferred from the data into a pre-determined memory
  location in RAM.

* u-Boot code (via an open source, but HPE-specific process) moves HW
  ID information from RAM into u-Boot environmental variables.

* HPE-specific code in [phosphor-u-boot-env-mgr](https://github.com/openbmc/phosphor-u-boot-env-mgr)
  (only activated if GXP-specific markers are found to exist) moves the
  data onto D-Bus under an HPE-specific name.
  
* Entity-Manager probes that reference property names from the HPE
  device namespace on D-Bus can key off GXP hardware data and react
  accordingly.
		
## Alternatives Considered

Instead of having u-Boot transfer HW data to D-Bus using
phosphor-u-boot-env-mgr, u-Boot could instead copy the data onto the
local filesystem, where it can be discovered and parsed to D-Bus by
Entity-Manager daemons.

An advantage to this method would be that any platform could decide
to use that filesystem -> D-Bus daemon to trigger
Entity-Manager configs based on arbitrary data on the filesystem.

Downsides to this method are that it relies on the OS filesystem for
IPC, and that putting HPE-specific code in Entity-Manager daemons will
mean that every system using Entity-Manager (likely all platforms)
would need to check for GXP-specific file(s) at Entity-Manager startup.

## Impacts

No major impacts expected. At the time of this writing, no public
projects actually include phosphor-u-boot-env-mgr in a recipe. 

Aditionally, non-HPE platforms that make use of the
phosphor-u-boot-env-mgr subproject will only encounter a single
file-existence check that's used to determine if the system requires
HPE-specific handling.

## Organizational:

No new repository should be needed.

## Testing

Can be tested on a GXP system by setting an Entity-Manager config's 
probe to trigger on match with a known GXP-HW identifying property, 
then by confirming 'busctl tree xyz.openbmc_project.EntityManager' 
shows a listing of objects that matches what's described in the 
modified EM config file.
