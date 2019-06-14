# u-boot environment manager

Author: Richard Marian Thomaiyar (rthomaiy123)

Primary assignee: Richard Marian Thomaiyar

Other contributors: Vernon Mauery

Created: June 5, 2019

## Problem Description
Some OpenBMC features require to access u-boot environmental variable. As there
are no D-Bus interface to access the same OpenBMC applications are
forced to read or write the variables using fw_printenv or fw_setenv
tools.

## Background and References
During firmware update or to preserve some data under reset to defaults
u-boot environmental variables are used. Unfortunately, no D-Bus interface
for the same exists forcing OpenBMc applications which needs to read or write
u-boot variables to call the fw_printenv or fw_setenv tools directly.
This requires each application to handle the output and update the same,
even though the same can be performed directly in single place and exposed
through D-Bus interfaces.

## Requirements
1. Expose D-Bus interface method to Read All u-boot environment variable
which will read all the variables, and return it in dictionary
form.
2. Provide method to Write u-boot environment variable, so that applications
in OpenBMC can write the same using a D-Bus method
3. In future it will be easier to implement any redundancy and checksum
Features or storing the variables in different places based on variable names.

## Proposed Design
Expose a D-Bus interface `xyz.openbmc_project.u_boot.Variable` which will
expose 2 D-Bus method 1. `ReadAllVariables` which will return
DICT[string, string] containing variable name and value. 2. `WriteVariable`
which will accept [string, string] containing variable name and value and
update the u-boot environment area.
Daemon which offers this can re-use the fw-utils source code and call
the fw_setenv() and fw_printenv() library function internally.

## Alternatives Considered
Application must directly call fork and execute the fw_printenv or fw_setenv
tool and parse the data on its own. This requires each application to do
this activity even though it will be easy to do D-Bus call.

## Impacts
This requires interface to defined and a daemon to be implemented and rest
of the code to access the same.

## Testing
Can be tested by executing commands through CI system.
