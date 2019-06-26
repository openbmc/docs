# Methods to handle OpenPOWER System boot sequences

  Author: Dhruvaraj S <dhruvaraj@gmail.com>

  Primary Assignee: Dhruvaraj S

  Created: 06/26/2019

## Problem Description

OpenBMC phosphor-state-manager currently supports booting, powering off and
rebooting a system. POWER based servers also require the ability to control
each individual aspect of the system boot. Including, such as the  ability
to execute each boot steps individually. It also has some special reboot loops
for handling boot time hardware failures and host debug data collection which
is not supported in the phosphor-state-manager. The BMC needs to support
additional steps which executes hardware procedures and SBE chip ops to
handle the special sequences needed for OpenPOWER systems.

## Glossary

- Boot: The process of initializing hardware components in a computer system
  and loading the operating system.
- Hostboot: The firmware runs on the host processors and perform all processor,
  bus and memory initialization on OpenPOWER based servers. [read more](https://github.com/open-power/docs/blob/master/hostboot/HostBoot_PG.md)
- Self Boot Engine(SBE): A microcontroller built into the host processors of
  OpenPOWER systems to assist in initializing the processor during the boot.
  It also acts as an entry point for several hardware access operations for the
  processor. [read more](https://sched.co/SPZP)
- Master Processor: The processor which get initialized first to execute boot
  firmware.
- POWER Hardware Abstraction Layer(PHAL): A software component on the BMC
  providing access to the OpenPOWER hardware
- Device Tree: A software representation of the hardware units in a tree unit,
  provided as part of the PHAL. [read more](https://www.devicetree.org/)
- Hardware Procedure(HWP): A set of procedures provided to initialize, configure
  or execute a specific operation on a hardware unit.

## Background and References

The booting of systems starts with powering on the BMC. Once the BMC is powered
on it does necessary steps to initialize the host processor and firmware to boot
the system. The host processor is initialized by Self Boot Engine(SBE), Once the
power on is applied, the service processor power on the SBE on the primary
booting processor. The master processor is initialized to start the hostboot
firmware. The hostboot firmware initializes rest of the system to start the
runtime firmware.

The application to handle Power System specific  boot sequence will be
responsible for initiating the SBE, doing necessary initialization before
executing the sequences, initiating hardware procedures as part of steps
executed on the BMC and handling of errors.

## Requirements

***Normal Boot***
-   Provide an infrastructure to execute various steps to boot.
-   Initiate required hardware procedures for initialization
-   Handle errors encountered during BMC steps

***Step by Step Boot - lab usecase***
-   Provide a command line option to user to execute any arbitrary boot step.
-   Should configure SBE and hostboot to boot in step by step mode by waiting
    for next command after completion of each step.
-   BMC steps will be executed directly by calling the implementation in BMC.
-   Trigger SBE chipop for executing the SBE steps.
-   Initiate a communication to hostboot to execute a hostboot boot step.
-   Handle errors encountered during SBE or hostboot steps.

***Assumptions***
- PHAL layer provides the infrastructure for executing the Hardware procedures.
- Hostboot to provide a hardware procedure to communicate to hostboot about the
  step to execute.


## Proposed Design

-   The invocation of the specific boot sequence will be based on systemd
    targets, new systemd targets will be created for OpenPOWER specific
    normal boot sequences like MPIPL.
-   There will at least one service file for each boot sequence to initiate the
    steps to boot in sequence.
-   An OpenPOWER specific boot control application will be called from the
    service files to execute the steps.
-   OpenPOWER Boot control application does following operations
    - Implements each of the steps to be executed on BMC..
    - Call the HWP execution interface provided by the PHAL to execute the
      HWP needed for BMC steps.
    - Initialization needed for executing the step.
    - Do necessary error handling after a boot step failure like creating
      errors in BMC specific format for HWP failures or errors from PHAL.
-   For the host or SBE steps, boot control application should inform the
    respective subsystems to execute the step.
![](https://user-images.githubusercontent.com/16666879/64525188-e7eb9880-d31d-11e9-90c0-b32369bcc152.png)

***Steps to be executed on BMC***

-  poweron: Apply power to the system.
-  start_ipl: Gets SP into a state ready to start boot.
-  set_ref_clock: Step to adjust reference clock frequencies.
-  proc_clock_test: Check to see whether reference clock is valid.
-  proc_select_boot_master: Select the SEEPROM to boot.
-  sbe_config_update: Updating scratch register for SBE.
-  sbe_start: Starting SBE.

***Example Boot Flow:***

    Power On:
     -> Chassis power on (target) --> Chassis power on operations
      -> Host power on (target)
        -> Start ipl (service) --> OP Boot Application --> PHAL
        -> set reference clock (service) --> OP Boot Application -->PHAL
        -> Update host configuration (service) --> OP Boot Application
           --> PHAL
        -> Update SBE configuration (service) --> OP Boot Application
           --> PHAL

***Commandline interface***
  OpenPOWER Boot control application is a command line interface which takes
  step numbers, boot type, boot mode as inputs and execute the step in the
  specified mode.

            Options:
                   --help            Print this menu
                   --major           Step Major Number
                   --minor           Step Minor Number
                   --step            Single step or a valid range of
                                     steps to execute. Valid range should
                                     be separated by '..'.
                                     Step mode will be the default boot mode.
                   --exec            Execute single step or a valid
                                     range of steps by step name. The range
                                     should be separated by '..'.
                                     Step mode will be the default boot mode.
                   --mode            Boot mode
                                     Valid modes normal,step
                   --type            Boot type
                                     Valid types: on,off,reboot

## Alternatives Considered
- OpenPower Boot manager daemon, which runs always and listen for the boot or
  step request with keeping track of the boot mode and boot type. The in the
  decision the design meeting is not to make a general daemon  but develope
  command line application and pass mode and type each time. And if there is any
  usecase in future for a daemon, create a smaller daemon just needs to run that
  specific usecase.

## Impacts
- Existing phosphor state manager application is not touched.
- Impact on chassis power on services, since that needs to be initiated as part
  of step by step booting.
- Impact to existing open power boot applications which starts SBE.

## Testing
   ***Test Plan:***
   - Unit tests:
     - Execute each step on BMC, SBE and host
     - Execute a range of steps
     - Test for invalid range
     - Execute all steps to load the hypervisor
     - Execute power on using obmcutil
     - Execute power off using obmcutil
