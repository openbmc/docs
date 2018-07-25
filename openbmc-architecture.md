# OpenBMC architecture

DRAFT: This is a draft version of the document for review purposes.

Purpose: This describes the primary hardware and software elements of
OpenBMC's architecture.  This is intended as reference material for
detailed architecture and design documents and formal security
analysis.

The audience is assumed to be familiar with baseboard management
controllers (BMCs) and the Linux operating system.

## Overview

OpenBMC is a Linux distribution with added features for baseboard
management control (BMC) including:
 - Definitions for the BMC to communicate with devices on its host
   board.  Examples: I2C, LPC.
 - Applications for the BMC to control elements on the host board.
   Examples: fan control, collecting error logs.
 - Interfaces for external agents to interact with the BMC and operate
   the host.  Example interfaces: REST APIs, web applications, network
   interfaces.  Example tasks: Uploading new firmware, powering on the
   system, collecting error logs.

OpenBMC is based on the Yocto distribution.  The OpenBMC source code
is in various repositories in the https://github.com/openbmc
organization.  Each repository holds one element of OpenBMC, for
example, board definitions, application programs, and web servers.

OpenBMC's software architecture consists of layers of programs which
communicate via Linux device files, D-Bus objects, REST APIs, and
externally-facing interfaces such as IPMI and web servers.  One such
layer populates D-Bus objects which represent all the BMCs controls
including host devices and BMC resources.  Another layer uses D-Bus
objects for applications such as automated thermal control and to
export user controls such as REST APIs.

## Hardware architecture

OpenBMC is intended for use on a board which hosts interconnected
hardware elements including the BMC.  The BMC is expected to have a
network connection for interaction with its users.  The BMC's
operating system must be an OpenBMC Linux image.

OpenBMC supports various BMCs and boards.  Note that although OpenBMC
terminology assumes the BMC is embedded (aka hosted) in the board of a
machine (aka computer), OpenBMC is not limited to that application.

[Copied from gerrit review 11164]: The typical BMC is a
System-On-a-Chip (SOC) that integrates three functional components
consisting of an ARM embedded controller, video controller, and a
Super I/O (SIO) controller.

[Copied from gerrit review 11164]: OpenBMC provides support for server
computers in the form of an alternate interface to control power &
reset to the host processor independent of the host operating system,
thermal management, health monitoring, remote access, and management
of Field Replaceable Units (FRUs).  FRUs typically consist of power
supplies, fan controllers, network switches, storage controllers,
storage devices, and Add-In-Cards (AICs).

Question for reviewers: See the current review in
https://gerrit.openbmc-project.xyz/#/c/openbmc/docs/+/11164/2/security/obmc-securityarchitecture.md
... Should we talk about common hardware element such as USB 2.0,
PCIe, I2C, LPC, USB, UART, ethernet controllers, etc., in this
document, or split out those details in separate documents
hierarchically under this one?

The openbmc/openbmc repository contains the catalog of supported
hardware and is the starting point of the firmware build process.
Within this repository:
 - The collection of boards is under the openbmc/meta-openbmc-machines
   directory.  Each board defines the hardware environment in which the
   BMC operates (including definitions for the BMC hardware itself) and
   defines recipes for BMC application programs that interact with the
   host machine.  The directory is structured for access to specific
   board layers and for access to common elements shared by board
   families.
 - The collection of BMCs is under the openbmc/meta-openbmc-bsp
   directory.  Each BMC definition gives details needed to create the
   Linux image, for example, the compiler's target architecture.

Each board-specific application is in its own repository.  For
example, the openbmc/openpower-occ-control repository contains the
control application for the OpenPOWER On-Chip-Controller and is
included in recipes for the OpenPOWER family of machines.

## Software architecture

OpenBMC uses the Yocto/OpenEmbedded project which uses the Bitbake
tool to create custom Linux distributions in layers.  OpenBMC adds two
layers to the base Yocto layers: a board layer and the phosphor layer.
The board layers are described by the hardware architecture section
above.  The phosphor layer contains applications intended to be common
to all OpenBMC images.  The naming convention is that each
openbmc/phosphor-* repository contains a phosphor application.

The primary element of an OpenBMC firmware image the Linux operating
system which serves as a platform for all other elements.  Within the
Linux image, the architecture is characterized by four interface
layers: device files, D-Bus objects, URIs, and external interfaces.
 - Device files directly represent hardware devices such as fans and
   temperature sensors; they provide a way to monitor and control the
   hardware the BMC uses as part of its operation.
 - D-Bus objects provide access to all the resources the BMC monitors
   and controls.  Some of the objects are direct mappings of devices,
   others are for resources stored on the BMC such as firmware
   images. The D-Bus objects are intended to expose all the BMCs
   controls.  D-Bus objects are not guaranteed to be forward compatible.
 - URIs (uniform resource identifiers) are provided by REST API
   servers.  These typically serve as a translation layer for D-Bus
   objects and provide access via HTTP.  The OpenBMC REST APIs are
   intended to be forward compatible.  An example REST API is the Redfish
   specification.
 - The external interfaces include network servers (such as web
   applications, REST APIs, and IPMI) and access from the host (for
   example: error logging, or host-initiated reboot).  Other OpenBMC
   elements are characterized in terms of these interfaces.
