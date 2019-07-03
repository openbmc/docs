# Power Hardware Abstraction Layer (pHAL)

Author: Jayanth Othayoth <ojayanth@in.ibm.com>

Contributor: Alistair Popple <apopple@in.ibm.com>

Created 2019-07-01

## Problem Description
The openBMC requires more additional capabilities for the CEC hardware such as
 - A common mechanism to access and control the CEC hardware from the BMC.
 - A storage mechanism to store the data that can be used in to initialize,
   control and access the CEC hardware.
 - An infrastructure to initialize the hardware using the hardware procedures
   provided by the hardware team.
 - Common mechanism to handle the hardware failures for the host hardware.
These above features are essential for the BMC to provide better RAS capability,
manufacturing, and lab debug for the enterprise class systems. 

The proposed pHAL aims to provide a common layout such that the openpower
application can use this infrastructure to boot, enable RAS features like dump,
diagnostics, and lab debug tooling purpose.

## Glossary
Boot: The time between when power is applied to the platform hardware and when
the payload is fully functional
CEC: Central Electronics Complex, which includes Host processor, buses and memory
subsystems.
Device Tree: Data structure describing the CEC hardware components and properties.
[Device Tree documentation Link]("https://elinux.org/Device_Tree_Reference")
eCMD: POWER hardware access interface tool.
[eCMD documentation Link]("https://github.com/open-power/eCMD")
Hostboot firmware: Refers to firmware that runs on the host processors to
initialize the memory and processor bus during boot and at runtime.
[Hostboot documentation Link]("https://github.com/open-power/docs/blob/master/hostboot/HostBoot_PG.md")
HWP: Hardware procedure. A “black box” code module supplied by the hardware
team that initializes CEC hardware in a platform-independent fashion.
HWPF: A code layer that executes black box hardware procedures for different
platforms and environments.
libfdt: pHAL uses to construct the in-memory tree structure of all targets.
[Reference link]("https://github.com/dgibson/dtc")
pdbg: Application to allow debugging of the host POWER processors from the BMC.
[Reference link]("https://github.com/open-power/pdbg")
MRW: Machine readable workbook. An XML description of a machine as specified
by the system owner.
RAS: Reliability, availability, and serviceability.
SBE: Self-boot engine. Specialized PORE that initializes the processor chip,
and then loads or invokes the hostboot boot firmware base image
[Reference link]("https://github.com/open-power/sbe")

## Background and References
The pHAL is a combination of open source packages that are tailored/customized
to use in openBMC. For instance POWER hardware access and control is managed
through pdbg and CEC data modeling is done via Device tree.

pHAL is group of libraries running in BMC. These libraries are used by openpower
specific application for CEC chip interactions, Hostboot and SBE initialization,
diagnostics and debugging.

Existing openBMC only allows minimum infrastructure to boot the POWER systems
There is no uniform mechanism to initialize host with custom configured data
(Eg. policies ,Hardware configuration) for manufacturing and lab debug.

This document scope is limited to discuss the pHAL high level layout.
Refer the Application specific design documents for functions specific details.

A key value proposition of this infrastructure is its extendability to other
platform architecture by replacing architecture specific libraries.

## Requirements

### libpdbg library
- Provide interfaces to get and put scoms using SBE chip-ops.
- Provide interfaces to get and put scoms  BMC natively for the lab debug.
- Provide mechanism to dynamically enable full trace of all SCOM operations
  to the hardware.
- Provide SCAN trace support for debugging.
- Provide interfaces to read/write data starting at a specified address from
  the mainstore memory (ADU/PBA) using SBE chip-ops.
- Provide interfaces which allow read/write access to POWER processor general
  purpose register(GPR), Floating point register(FPR) and Special purpose
  register(SPR) using SBE chip-ops.
- Provide interfaces to read/write CFAM  operation on a specified address.
- Provide asynchronous external interface (d-bus) for Processor Stop and SReset
  using SBE chip-ops
- Provide interface to retrieve state of the cores in the system using SBE
  chip-ops for the lab debug
- Provide interface to start instruction on the specified functional unit for
  the lab debug.
- Provide interface for the SBE to enter a Memory Preserving boot using the SBE
  chip-ops.
- Provide interface to inform SBE to continue Memory Preserving boot.

### libekb library
- Must allow BMC to execute arbitrary hardware procedures supplied by the
  hardware team.
- HWPF BMC platform code must translate native BMC errors and targets to HWPF
  platform neutral versions, and back.
- Provide support for  Plat functions get/put scom and put scom under mask.

### Device tree library
- Provide interface to get/set the given attributes, determine relationships.
- Allows clients to iterate over every Device Nodes in the system blueprint.
- Allows  using predicates/range filters etc.

### General requirements
- Load the device tree data prior to bmc standby to enable ecmd application at
  standby.
- opeBMC should not update device tree data if hostboot is active.

### pHAL Error logging library
- Provide common pHAL error logging infrastructure, to convert BMC style error
  logs reported by phal libraries.

## Proposed Layout Design
The infrastructure(pHAL) design is open source based in its entirety. For instance
POWER hardware access and control is managed through pdbg and CEC data modeling
is done via  Device tree.

pHAL is group of libraries running in BMC. These libraries are used by OPENPOWER
specific application for CEC chip interactions, Hostboot and SBE initialization
diagnostics and debugging.

### pHAL layout block diagram
![pHAL Layout_V4](https://user-images.githubusercontent.com/12947151/61534256-bdcbd780-aa4c-11e9-8a1b-7ddf9e7c0a37.png)

## pdbg repository
This repository provides a libpdbg library, that performs the abstractions for
accessing and updating CEC hardware like Host Processor registers, instruction
control and memory). Additionally, the library provides interfaces to execute
all SBE chip-ops that are mentioned in the requirement section.

## libpkb repository 
This repository contains copy of hardware procedures provided by the hardware
team and openBMC specific platform code. The platform code is essentially
required for the hardware procedure execution. Library for this reposiotory
will provide interfaces for executing the HWP from the openBMC.

Hardware procedures(HWP) are C++ code that does all of the hardware accesses
required to test and initialize the hardware.

## pdata repository
pHAL uses device tree-based data structure to manage CEC hardware information.
Device tree data modeling mainly includes the hardware topology and attributes,
which includes the configuration data.

Device Tree data is constructed during the build time, based on the System
specific MRW and packaged as part of Hostboot image.

The libfdt library, an open source library,  which act as a backend, that is
used to access and manipulate APIs of Device tree data. Additionally, custom
APIs are provided to lock the device tree data updates to avoid data coherency
issue.

## pHAL logging library
This library provides common pHAL error logging infrastructure. which includes
converting errors from openBMC external library errors to openBMC error logging
format. Also provide error handling abstraction when power faults occur to the  
system and/or FRUs.

## Assumptions
- HWPs are provided by respective Hardware team.
- Hostboot will be updating common data (used by both openBMC and Hostboot)
  during the boot window.
- SBE is responsible for doing special wake-up during hardware access.

## Alternatives Considered
NA

## Impacts
Development would be required to implement the error handling and device tree
management, platform specfic code for HWP.

## Testing
APIs provided in the pHAL libraries can be tested via fuzzing.
Hardware specific functionalities need to be validated on the CI infrastructure.
