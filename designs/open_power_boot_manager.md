# Methods to handle IBM&reg Power&reg System boot sequences.

  Author: Dhruvaraj S <dhruvaraj@gmail.com>
  Primary Assignee: Dhruvaraj S
  Other Contributors:
  Created: 06/26/2019

## Problem Description

OpenBMC phosphor-state-manager currently supports booting a system, powering off
a system, and rebooting a system. POWER based servers also require the ability
to control each individual aspect of the system boot. It also has some special
reboot loops for handling hardware failures and host debug collection.

## Background and References

The booting of systems starts with powering on the BMC. Once the
BMC is powered on it does necessary steps to initialize the host
processor and the booting firmware. The host processor is initialized by
Self Boot Engine(SBE), a micro controller contained inside the host processor.
Once the power on is started, the service processor power on the SBE on the
primary booting processor. The primary booting processor is initialized to start
the host booting firmware. The host booting firmware initialized rest of the
system to start the virtualization firmware or an operating system.

The application to handle Power System specific  boot sequence will be
responsible for initiating the SBE, doing necessary initialization before
executing the sequences, initiating hardware procedures as part of steps
executed on the BMC and handling errors.

***Different types of boot commands/sequences***
-   Power on Reset: A normal power on of the system.
-   Power off: Powering off a running system.
-   Warm reboot: Reinitialize the system without stopping power to the system.
-   Memory Preserving boot: A warm reboot with preserving memory contents.
-   Reconfigure loop: A special reboot if a fatal error is encountered while
    booting
-   Initialization by steps: Executing single booting steps for debug or
    development.

## Requirements

-   Provide an infrastructure to execute various steps to boot.
-   Support various types of boot sequences.
-   Provide interface to invoke various types of boot sequences
-   Initiate required hardware procedures for initialisation
-   Initiate required SBE chip-ops
-   Handle errors encountered during execution
-   Handle errors encountered during SBE or host booting.
-   Provide a method to manually execute each step.

## Proposed Design

-   A set of systemd targets will be created for each type of boot sequence.
-   Each execution step will be a service or part of an existing service and a
    dbus call needs to be made to the boot manager application.
-   Open Power Boot Manager Application implements each steps to be executed on
    BMC.
-   Open Power specific Boot Manager Application: An open power specific
    application will be the entry point to each of the steps needed to
    execute during the boot. This application will provide DBUS interfaces
    corresponds to the steps and do following operations
-   Initialization needed for executing the step
-   Call needed hardware procedures or initialization functions.
-   Do necessary error handling after an IPL step failure
-   For the host or SBE steps, boot manager application should inform the
    respective subsystems to execute the step

    Example Boot Flow:

    Power On:
            -> Chasis power on (target) --> Chasis poweron operations
            -> Host power on (target)
              -> Start ipl (service) --> OP Boot Application
              -> set reference clock (service) --> OP Boot Application
              -> Update host configuration (service) --> OP Boot Application
              -> Update SBE configuration (service) --> OP Boot Application
              -> Start SBE (service) --> OP Boot Application

    External interfaces

    - Set boot parameters
       - Mode: execute step by step or complete ipl
       - type: Type of state transition
    - DoIpl : Execute the specified step

## Alternatives Considered

## Impacts

## Testing
   Test using:
   - Command line tool to run different type of boot
   - command line tool to execute a single step
