Design proposal for eCMD (External Command) based CLI infrastructure for OpenBMC

Author: Lakshminarayana Kammath, <lkammath@in.ibm.com>

Other contributors: Jason Albert, Jayanth Othayoth

Created: 2019-07-04

## Problem Description

Traditionally on POWER based servers debug, manufacturing tooling, system and
and functional test tools, etc. all heavily depend on eCMD for hw access use
cases. Currently, servers that use OpenBMC/non-OpenBMC stack does have a CLI
way of doing scoms(read/write registers), read/write cfam registers, CPU control
instruction, Host memory operations, etc. using pdbg but, replacing all
the above tools which are using eCMD would be complicated and time-consuming.

## Glossary

* eCMD: External Command. A POWER hardware access interface tool.
  [eCMD documentation Link]("https://github.com/open-power/eCMD")

* CEC: Central Electronics Complex, which includes Host processor, main bus,
  and memory subsystems.

* cfam: Common FRU Access Macro. A common set of chip logic that allows the
  service processor to access/identifies types of chips in the CEC hardware.

* HWP: HW procedure. A "black box" code module supplied by the HW team that
  initializes CEC hardware in a platform-dependent fashion.

* Host Boot: Refers to a firmware that runs on the host processors to initialize
  the memory and processor bus during boot and at runtime.
  [Host Boot documentation link]("https://github.com/open-power/docs/blob/master/hostboot/HostBoot_PG.md")

* pHAL: Power Hardware Abstraction Layer.
  [Reference link]("https://gerrit.openbmc-project.xyz/#/c/openbmc/docs/+/23287/")[Under Review]

* libpdbg: Library that provides the APIs to build custom applications.
  Currently, the eCMD-pdbg glue layer code uses libpdbg APIs for hw access.

* pdbg: Application to allow debugging of the host POWER processors from the BMC.
  [Reference link]("https://github.com/open-power/pdbg")


## Background and References

eCMD is the hardware access API for POWER Systems.  It provides the user with
a consistent way to address hardware between system types and even across
processor generations. It is used by hardware bring-up teams, simulation,
debug, manufacturing, and test teams.

More details can be found in the link
[eCMD documentation Link]("https://github.com/open-power/eCMD/tree/master/docs")

eCMD-pdbg is the glue code layer necessary for pdbg to be used as an eCMD plugin.

More details can be found in the link
[eCMD-pdbg documentation Link]("https://github.com/open-power/ecmd-pdbg/blob/master/README.md")

## Requirements

Provide a set of eCMD utilities as a standalone application on OpenBMC to:
   * Execute HW operations like scom, scan, cfam, Host memory read/write
     access(ADU/PBA), instruction control, Architected state register
     access, etc.
   * Execute any Arbitrary Istep to exercise HWP that will be
     extended to exercise the HWPs across different Isteps to boot and also
     support handling of error during each Istep execution.
   * Query functional state of the chips.

## Proposed Design for servers that use OpenBMC

eCMD will be tightly coupled with libpdbg to perform CEC chip interactions required
or requested by its user.  eCMD will use pdbg library, an open power-based debug
utility as well, that provides the low-level interfaces to access host hardware
from the BMC. For instance, POWER hardware access and control is managed through
pdbg.

For Istep/boot step native execution eCMD utility will invoke a D-Bus call that,
will execute Istep application which implements each step to be executed on BMC.
The application will be the entry point for each of the steps needed to execute
during the boot. This application will provide DBUS interfaces corresponds to the
steps and do required actions specific to Istep. The required support needs to be
added in the eCMD glue layer that also provides an interface to call pdbg as well
D-Bus target.

### High level flow

https://user-images.githubusercontent.com/27765459/63280969-6c23a080-c2c9-11e9-82be-189ecc4ac9cc.png

### pHAL Block diagram

https://user-images.githubusercontent.com/12947151/60520075-61558200-9d02-11e9-9f43-d81364115388.png


## BMC Support

Enable the eCMD-pdbg as a standalone application running on BMC that, will be
invoked as a CLI utility for hw access specific use cases. It is accessible
only via BMC CLI since the usage of the eCMD utility is mostly for debugging
purpose and there is no real use case for it to be given as an external
interface like Redfish call or GUI.


## Alternatives Considered

Use pdbg itself as the CLI utility to perform the set of hw access operations
defined above but the caveat here is that, pdbg is just yesteryear tool which
came into light which can be alternative for eCMD but, eCMD has been established
as one of the go-to command-line tool from the very beginning of the POWER
existence and a large number of tools and infrastructure are built around it.
It will be easier for someone to consume eCMD as it is than working with a new
tool like pdbg or any other.


## Impacts

All the development work here involves making the hw access tools available on
BMC as a set of utilities which, uses eCMD-pdbg plugin layer. It has minimal
impact on the rest of the system.


## Testing

The testing required to make sure eCMD utilities should be able to exercise
hw access like scom, scan, cfam, host memory read/write, instruction control,
read architected register states, etc. All the debugging, manufacturing
and test specific tools that internally use these eCMD tools should work
seamlessly.

