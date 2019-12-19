# OpenBMC documentation

The OpenBMC project is a Linux Foundation project whose goal is to produce an
open-source Baseboard Management Controllers (BMC) customizable firmware
stack. This repository contains documentation for OpenBMC as a whole. There may
be component-specific documentation in the repository for each component.

The [features](features.md) document lists the project's major features
with links to more information.

## Contact

- Mail: openbmc@lists.ozlabs.org [https://lists.ozlabs.org/listinfo/openbmc](https://lists.ozlabs.org/listinfo/openbmc)
- IRC: #openbmc on freenode.net
- Riot: [#openbmc:matrix.org](https://riot.im/app/#/room/#openbmc:matrix.org)

## OpenBMC Goals

The OpenBMC project aims to create a highly extensible framework for BMC
firmware for data-center computer systems.

We have a few high-level objectives:

 * The OpenBMC framework must be extensible, easy to learn, and usable

 * Provide a REST API for external management, and allow for "pluggable"
   interfaces for other types of management interactions

 * Provide a remote host console, accessible over the network

 * Persist network configuration settable from REST interface and host

 * Provide a robust solution for RTC management, exposed to the host

 * Compatible with host firmware implementations for basic communication between
   host and BMC

 * Provide a flexible and hierarchical inventory tracking component

 * Maintain a sensor database and track thresholds

## Technical Steering Committee

The Technical Steering Committee (TSC) guides the project. Members are:

 * Brad Bishop (chair), IBM
 * Nancy Yuen, Google
 * Sai Dasari, Facebook
 * James Mihm, Intel
 * Sagar Dharia, Microsoft
 * Supreeth Venkatesh, Arm

## OpenBMC Development

These documents contain details on developing OpenBMC code itself

 - [cheatsheet.md](cheatsheet.md): Quick reference for some common
   development tasks

 - [CONTRIBUTING.md](CONTRIBUTING.md): Guidelines for contributing to
   OpenBMC

-  [development tutorials](development/README.md): Tutorials for getting up to
   speed on OpenBMC development

 - [kernel-development.md](kernel-development.md): Reference for common
   kernel development tasks

## OpenBMC Usage

These documents describe how to use OpenBMC, including using the programmatic
interfaces to an OpenBMC system.

 - [code-update](code-update): Updating OpenBMC and host platform firmware

 - [console.md](console.md): Using the host console

 - [host-management.md](host-management.md): Performing host management tasks
   with OpenBMC

 - [rest-api.md](rest-api.md): Introduction to using the OpenBMC REST API

 - [REDFISH-cheatsheet.md](REDFISH-cheatsheet.md): Quick reference for some
   common OpenBMC Redfish commands

 - [REST-cheatsheet.md](REST-cheatsheet.md): Quick reference for some common
   OpenBMC REST API commands
