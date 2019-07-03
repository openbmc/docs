# Power Hardware Abstraction Layer (pHAL)

Author: Jayanth Othayoth <ojayanth@in.ibm.com>

Contributor: Alistair Popple <apopple@in.ibm.com>

Created 2019-07-01

## Problem Description
The reliability, availability, and serviceability (aka. "RAS") is one of the key
design point for Enterprise class systems. Existing openBMC code doesn’t have 
infrastructure to meet all the key RAS requirements. Some of the key RAS 
requirements are diagnostics, fault isolation, redundancy and support for
debugging Host firmware and hardware failures.

## Glossary
CEC: Central Electronics Complex, which includes Host processor and memory
subsystems.
Device Tree: Data structure describing the CEC hardware components and properties.
[Device Tree documentation Link]("https://elinux.org/Device_Tree_Reference")
eCMD: POWER hardware access interface tool.
[eCMD documentation Link]("https://github.com/open-power/eCMD")
Hostboot firmware: Refers to firmware that runs on the host processors to
initialize the memory and processor bus during IPL and at runtime.
[Hostboot documentation Link]("https://github.com/open-power/docs/blob/master/hostboot/HostBoot_PG.md")
HWP: Hardware procedure. A “black box” code module supplied by the hardware
team that initializes CEC hardware in a platform-independent fashion.
HWPF: A code layer that executes black box hardware procedures for different
platforms and environments.
IPL: Initial program load. The time between when power is applied to the
platform hardware and when the payload is fully functional.
libfdt: pHAL uses to construct the in-memory tree structure of all targets.
[Reference link]("https://github.com/dgibson/dtc")
pdbg: Application to allow debugging of the host POWER processors from the BMC.
[Reference link]("https://github.com/open-power/pdbg")
MRW: Machine readable workbook. An XML description of a machine as specified
by the system owner.
RAS: Reliability, availability, and serviceability. 
SBE: Self-boot engine. Specialized PORE that initializes the processor chip,
and then loads or invokes the hostboot IPL firmware base image
[Reference link]("https://github.com/open-power/sbe")

## Background and References
Existing openBMC only allows minimum infrastructure to IPL the POWER systems
There is no uniform mechanism to initialize host with custom configured data
(Eg. policies ,Hardware configuration) for manufacturing and lab debug. 

The plan is to provide common infrastructure code to run openPOWER specific
applications to enable better RAS support in openBMC. This required POWER
specific hardware knowledge and access from the openBMC. Also need to provide
better error isolation procedure for host specific failures.

This document scope is limited to discuss the pHAL high level layout.
Refer the Application specific design documents for functions specific details. 

A key value proposition of this infrastructure is its extendability to other 
platform architecture by replacing architecture specific libraries.

## Requirements
- POWER Hardware Abstraction and Access.
- HWPF Infrastructure support 
- SBE/Hostboot Control and FFDC. 
- CEC IPL co-ordination and Diagnostics Infrastructure support 
- Manufacturing Support 
- POWER Hardware Debug and Bringup Support 

## Proposed Layout Design
The infrastructure(pHAL) design is open source based in its entirety. For instance
POWER hardware access and control is managed through pdbg and CEC data modeling
is done via  Device tree.

pHAL is group of libraries running in BMC. These libraries are used by OPENPOWER
specific application for CEC chip interactions, Hostboot and SBE initialization
diagnostics and debugging.

![image](https://user-images.githubusercontent.com/12947151/60520075-61558200-9d02-11e9-9f43-d81364115388.png)

## libpdbg library
This library provides abstractions for accessing and updating CEC hardware like
Host Processor registers, instruction control and memory). Also provides support
execute all chip-ops supported by SBE. This is the library is part of pdbg tool 
an application to allow debugging of the host POWER processors from the BMC.

## Hardware Procedure Framework(HWPF) library
The CEC chip( Processor, Memory) initialization and debug is driven by the
hardware procedures. Hardware procedures(HWP) are C++ code that does all of the
hardware accesses required to test and initialize the hardware. This library
provides a functions to execute hardware procedure from BMC.

## Device Tree based CEC Information management
pHAL uses device tree based data structure to manage CEC hardware information.
Device tree data modeling mainly includes the hardware topology and attributes,
which includes the configuration data.

Device Tree data is constructed during Hostboot build time based on the System
specific MRW and packaged as part of Hostboot  image.

### libfdt library
This is an open source library provides access and manipulation APIs of Device
tree data. Custom APIs, like Target locking during data updates and low level
APIs to update the data to Host data area.

## Error Plugins library:
This library provides common pHAL error logging infrastructre. which includes
converting errors from openBMC external library errors to openBMC error logging
format. Also provide error handling abstraction when power faults occur to the
system and/or FRUs. 

## Assumptions                                                               
- HWPs are provided by respective Hardware team.                                
- Use Hostboot flash as a primary store for CEC Hardware configuration.

## Alternatives Considered
NA

## Impacts
Development would be required to implement the error handling and device tree
management, platform specfic code for HWP.

## Testing
APIs provided in the pHAL libraries can be tested via fuzzing.
Hardware specific functionalities need to be validated on the CI infrastructure. 
