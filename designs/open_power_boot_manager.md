# Methods to handle Open Power System boot sequences.

  Author: Dhruvaraj S <dhruvaraj@gmail.com>
  Primary Assignee: Dhruvaraj S
  Created: 06/26/2019

## Problem Description

OpenBMC phosphor-state-manager currently supports booting a system, powering off
a system, and rebooting a system. POWER based servers also require the ability
to control each individual aspect of the system boot. It also has some special
reboot loops for handling hardware failures and host debug data collection.

## Glossary

- Boot: The process of initializing hardware components in a computer system
  and loading the operating system.
- Hostboot: The firmware runs on the host processors to initialize planner
  components on an Open Power System.
- SBE: A microcontroller built into the host processors of Open Power systems
  to assist in initializing the processor during the boot. It also acts as an
  entry point for several hardware access operations for the processor.
- PHAL: A software level on the BMC providing access to the Open Power hardware
- Device Tree: A software representation of the hardware units in a tree unit,
  provided as part of the PHAL.
- Hardware Procedure(HWP): A set of procedures provided to initialize, configure
  or execute a specific operation on a hardware unit.

## Background and References

The booting of systems starts with powering on the BMC. Once the BMC is powered
on it does necessary steps to initialize the host processor and the booting
firmware. The host processor is initialized by Self Boot Engine(SBE), a
micro-controller contained inside the host processor. Once the power on is
applied, the service processor power on the SBE on the primary booting
processor. The primary booting processor is initialized to start the hostboot
firmware. The host boot firmware initialized rest of the system to start the
virtualization firmware or an operating system.

The application to handle Power System specific  boot sequence will be
responsible for initiating the SBE, doing necessary initialization before
executing the sequences, initiating hardware procedures as part of steps
executed on the BMC and handling errors.

## Requirements

***Normal Boot***
-   Provide an infrastructure to execute various steps to boot.
-   Initiate required hardware procedures for initialization
-   Handle errors encountered during BMC steps

***Step by Step Boot***
-   Provide a method to manually execute each step.
-   Initiate required SBE chip-ops
-   Handle errors encountered during SBE or host steps.

***Assumptions***
- PHAL layer provides the infrastructure for executing the Hardware procedures.
- PHAL provides mechanism to execute the step in host during step by step
  execution

## Proposed Design

-   A set of systemd targets will be created for each type of boot sequence.
-   Each execution step will be a service or part of an existing service and
    invoking a command line application to execute the step.
-   Open Power Boot Manager an Open Power specific application will implement
    functions to call the underlying hardware procecures as part of PHAL
-   Open Power Boot control Application does following operations
    - Implements entry point for each steps.
    - Call the Hardware Procedure execution interface provided by
      the PHAL to execute the BMC steps.
    - Initialization needed for executing the step.
    - Do necessary error handling after a boot step failure.
-   For the host or SBE steps, boot manager application should inform the
    respective subsystems to execute the step.

***Example Boot Flow:***

    Power On:
            -> Chasis power on (target) --> Chasis poweron operations
            -> Host power on (target)
              -> Start ipl (service) --> OP Boot Application --> PHAL
              -> set reference clock (service) --> OP Boot Application -->PHAL
              -> Update host configuration (service) --> OP Boot Application
                 --> PHAL
              -> Update SBE configuration (service) --> OP Boot Application
                 --> PHAL
***Commandline interface***
  Open Power Boot control application is a command line interface which takes
  step numbers, boot type, boot mode as inputs and execute the step in the
  specified mode.
            Options:
                   --help            Print this menu
                   --major           Step Major Number
                   --minor           Step Minor Number
                   --mode            Boot mode
                                     Valid modes Normal,step
                   --type            Boot type
                                     Valid types: on,off,reboot

## Alternatives Considered

## Impacts

## Testing
   Test using:
   - Command line tool to run different type of boot
   - command line tool to execute a single step
