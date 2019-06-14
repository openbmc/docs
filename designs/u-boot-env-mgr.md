# U-Boot environment manager

Author: Richard Marian Thomaiyar (rthomaiy123)

Primary assignee: Richard Marian Thomaiyar

Other contributors: Vernon Mauery

Created: June 5, 2019

## Problem Description
Some OpenBMC applications require access to U-Boot environmental variables.
Since there is no D-Bus interface to access U-Boot environmental variables,
OpenBMC applications are forced to read or write the variables using the
fw_printenv or fw_setenv tools or by including U-Boot environment library.

## Background and References
U-Boot environment variables are used during firmware update and to preserve
data during a factory reset. Since, no D-Bus interface exists these
applications must call the fw_printenv or fw_setenv tools directly and handle
the output even though the same can be performed directly in single place and
exposed through D-Bus interfaces.

## Requirements
1. Expose a D-Bus interface method to read all U-Boot environment variables and
return them in a dictionary form.
2. Provide a method to write U-Boot environment variables, so that applications
in OpenBMC can write the same using a D-Bus method
3. In future it will be easier to implement any redundancy and checksum
features or storing the variables in different places based on variable names.

## Proposed Design
Expose a D-Bus interface `xyz.openbmc_project.UBoot.Environment` which will
expose 3 D-Bus methods
1. `ReadAllVariables` which will return DICT [string, string] containing
variable name and value.
2. `ReadVariable` which will accept the variable as a string, and return the
 data as a string.
3. `WriteVariable` which will accept [string, string] containing variable name
and value to update the U-Boot environment variable.
Daemon which offers this can re-use the fw-utils source code and call
the fw_setenv() and fw_printenv() library function internally. Apart from this
inotify on /dev/mtdX (U-Boot environment mtd section) can be used to
effectively cache the variables.

## Alternatives Considered
Application must directly call fork and execute the fw_printenv or fw_setenv
tool and parse the data on its own or can use U-Boot environment library.
Both requires application to execute in root privilege as these tools must
access the /dev/mtdx to read / write the variable.

## Impacts
This requires an interface to be defined, a daemon to be implemented, and the
applications currently using the U-Boot environment variables to move over to
this new interface.

## Testing
Can be tested by executing commands through CI system.
