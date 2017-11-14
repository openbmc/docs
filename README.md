# OpenBMC documentation

This repository contains documentation for OpenBMC as a whole. There may
be component-specific documentation in the repository for each component.

OpenBMC Usage
-------------

These documents describe how to use OpenBMC, including using the programmatic
interfaces to an OpenBMC system.

 - [rest-api.md](rest-api.md): Introduction to using the OpenBMC REST API

 - [console.md](console.md): Using the host console

 - [host-management.md](host-management.md): Performing host management tasks
   with OpenBMC

 - [host-code-update.md](host-code-update.md): Updating host platform firmware


OpenBMC Development
-------------------

These documents contain details on developing OpenBMC code itself

 - [cheatsheet.md](cheatsheet.md): Quick reference for some common
   development tasks

 - [contributing.md](contributing.md): Guidelines for contributing to
   OpenBMC

 - [kernel-development.md](kernel-development.md): Reference for common
   kernel development tasks


OpenBMC Goals
-------------

The OpenBMC project's aim is to create a highly extensible framework for BMC
software and implement for data-center computer systems.

We have a few high-level objectives:

 * The OpenBMC framework must be extensible, easy to learn, and usable in a
   variety of programming languages.

 * Provide a REST API for external management, and allow for "pluggable"
   interfaces for other types of management interactions.

 * Provide a remote host console, accessible over the network

 * Persist network configuration settable from REST interface and host

 * Provide a robust solution for RTC management, exposed to the host.

 * Compatible with host firmware implementations for basic IPMI communication
   between host and BMC

 * Provide a flexible and hierarchical inventory tracking component

 * Maintain a sensor database and track thresholds
