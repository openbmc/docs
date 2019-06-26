# Methods to handle IBM&reg Power&reg System boot sequences. 

Author: Dhruvaraj S <dhruvaraj@gmail.com>
Primary Assignee: Dhruvaraj S
Other Contributors: 
Created: 06/26/2019

## Problem Description 

IBM&reg Power&reg Based Systems need to execute various types of initialization
sequences in order to boot the system. Apart from booting there are additional
state transitions needed to handle certain scenarios. Few examples of system
state transitions are normal power on, power off, warm reboot, cold reboot,
memory preserving reboot, reconfigure loop. Proposing to create a specialized
open power application suitable to handle the IBM&reg Power&reg based enterprise
system requirements.

## Background and References

The booting of systems starts with powering on the service processor. Once the
service processor is powered on it does necessary steps to initialize the host
processor and the booting firmware. The host processor is initialized by
Self Boot Engine(SBE), a micro controller contained inside the host processor.
Once the power on is started, the service processor power on the SBE on the 
primary booting processor. The primary booting processor is initialized to start
the host booting firmware. The host booting firmware initialized rest of the
system to start the virtualization firmware or an operating system.

The application to handle boot sequence will be responsible for initiating
the SBE, doing necessary initialization before executing the sequences,
initiating hardware procedures as part of steps executed on the BMC and handling
errors.

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

**-   General Requirements**

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
-   Each execution step will be a service and this service will make a dbus call
    to the boot manager application
-   Open Power Boot Manager Application implements each steps to be executed on
    BMC.
-   Open Power specific Boot Manager Application: An open power specific 
    application will be the entry point to the each of the steps needed to
    execute during the boot. This application will provide DBUS interfaces c
    orresponds to the steps and do following operations
-   Initialization needed for executing the step
-   Call needed hardware procedures or initialization functions.
-   Do necessary error handling after an IPL step failure
-   For the host or SBE steps, boot manager application should inform the
    respective subsystems to execute the step

## Alternatives Considered

## Impacts

## Testing
