# OpenBMC refactor static configs

Author: John Broadbent (jebr) < jebr@google.com >

Primary assignee: John Broadbent (jebr)

Other contributors:

Created: October 14, 2020

## Problem Description

Specific static configuration related to sensors and inventory are saved in a
yaml file, and they are converted into a C++ at library at build time.  Several
different repositories require the static configuration. Each repository
contains duplicate logic to generate the C++ file.

## Background and References

Reorganizing the static configuration will allow us to restructure other
reposisties in a way that will be easy to maintain. The following files are all
the same. (with very minor changes)

 * https://github.com/openbmc/ipmi-fru-parser/blob/0968237b479d649ecaac7561cf07fbacf241d98c/scripts/fru_gen.py
 * https://github.com/openbmc/phosphor-host-ipmid/blob/be4ffa8720adfe03e00da0e4c16b1769f8540a72/scripts/inventory-sensor.py
 * https://github.com/openbmc/openpower-host-ipmi-oem/blob/9975ae919e4cb8968639d9f953912e6c73205c44/scripts/inventory-sensor.py

Rather than copying and pasting the code again, we believe it would be better
to create a separate repository for handling this conversion. All similar yaml
code can be integrated into one repository

## Requirements

1. Make the static configurations easier to manage

3. Prevent the duplication of code

2. Not disrupt any other features or workflows

## Proposed Design

We propose putting this code into its own repository, and pulling it into
configurations with recipes.  This will decrease the redundant code, and
provide a method for other repositories to get this information without
resoruting the duplicating this logic.

## Alternatives Considered

It is possible to continue the current method in which we continue to duplicate
the C++ generation code, but delaying this clean up will only make it
eventually more difficult.

## Impacts


## Testing

Testing will be handled by the existing presubmit infrastructure




