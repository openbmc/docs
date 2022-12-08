# Error handling for power Hardware Abstraction Layer (pHAL)

Author: Devender Rao <devenrao@in.ibm.com> <devenrao>

Other contributors: None

Created: 14/01/2020

## Problem Description

Proposal to provide a mechanism to convert the failure data captured as part of
power Hardware Abstraction Layer(pHAL) library calls to [Platform Event Log][1]
(PEL) format.

## Background and References

OpenBmc Applications use the pHAL layer for hardware access and hardware
initialization, any software/hardware error returned by the pHAL layer need to
be converted to PEL format for logging the error entry. PEL helps to improve the
firmware and platform serviceability during product development, manufacturing
and in customer environment.

Error data includes register data, targets to [guard][2] and callout. Guard
refers to the action of "guarding" faulty hardware from impacting future system
operation. Callout points to a specific hardware with in the server that relates
to the identified error.

[Phosphor-logging][3] [Create][4] interface is used for creating PELs.

pHAL layer constitutes below libraries and and these libraries return different
return codes.

1. libipl used for initial program load
2. libfdt for device tree access
3. libekb for hardware procedure execution
4. libpdbg for hardware access

Proposal is to structure the return data to a standard return code format so
that the caller can just handle the single return code format for conversion to
PEL.

### Glossary

pHAL: power Hardware Abstraction Layer. pHAL is group of libraries running in
BMC. These libraries are used by Open Power specific application for hardware
complex interactions, hostboot and Self Boot Engine initialization, diagnostics
and debugging.

libfdt: pHAL uses to construct the in-memory tree structure of all targets.
[Reference][5]

libpdbg: library to allow debugging of the host POWER processors from the BMC
[Reference][6]

MRW: Machine readable workbook. An XML description of a machine as specified by
the system owner.

HWP: Hardware procedure. A "black box" code module supplied by the hardware team
that initializes host processor and memory subsystems in a platform -independent
fashion.

Device Tree: A device tree is a data structure describing the hardware
components of a particular computer so that the operating system's kernel can
use and manage those components, including the CPU or CPUs, the memory, the
buses and the peripherals. [Reference][7]

EKB: EKB library contains all the hardware procedures (HWP) for the specific
platform and corresponding error XML files for each hardware procedure.

PEL: [Platform Entity Log][1]

## Requirement

### libekb

EKB library contains hardware procedures for the specific platform and the
corresponding error xml files for each hardware procedure. Error XML specifies
attribute data, targets to callout, targets to guard, and targets to deconfigure
for a specific error. Parsers in EKB library parse the error XML file and
generate a c++ header file which is used by the hardware procedure in capturing
the failure data.

Add parser in libekb to parse the error XML file and provide methods that can
parse the failure data returned by the hardware procedure methods and return
data in key, value pairs so that the same can be used in the creation of PEL.

### libipl

Initial program load library used for booting the system. Library internally
calls hardware procedures (HWP) of EKB library. Hardware procedure execution
status need to be returned to the caller so that caller can create PEL on
hardware procedure execution failure.

### libpdbg

libpdbg library is used for hardware access, any hardware access errors need to
be captured as part of the PEL.

### Message Registry Entries

For errors to be raised in pHal corresponding error message registry entries
need to be created in the [message registry][8].

## Proposed design

### Hardware procedure failure

Add parser in libekb to parse the error XML file and provide methods that can
parse the failure data returned by the hardware procedure methods and return
data in key, value pairs so that the same can be used in the [Create][4]
interface for the creation of PEL.

Inventory strings for the targets to Callout/Guard/Deconfig need to be added to
the additional data section of the Create interface.

Applications need to register callback methods in libekb library to get back the
error logging traces.

Debug traces returned through the callback method will be added to the PEL.

### libipl internal failure

Applications need to register callback methods in libipl library to get back the
error logging traces.

Debug traces returned through the callback method will be added to the PEL.

### libpdbg internal failure

Applications need to register callback methods to get the debug traces from
libpdbg library.

Debug traces returned through the callback method will be added to the PEL.

## Sequence diagrams

### Register for debug traces and boot errors

![image](https://user-images.githubusercontent.com/26330444/76838214-e4e7dc80-6859-11ea-818c-031bf5a191d6.png)

### Process debug traces

![image](https://user-images.githubusercontent.com/26330444/76838355-152f7b00-685a-11ea-9975-4091ae1064cc.png)

### Process boot failures

![image](https://user-images.githubusercontent.com/26330444/76838503-3a23ee00-685a-11ea-9f2a-559e233b408f.png)

## Alternatives Considered

None

## Impacts

None

## Future changes

At present using [Create][4] by providing the data in std::map format the same
will be changed to JSON format when the corresponding support to pass JSON files
to the Create interface is added.

## Testing

1. Simulate hardware procedure failure and check if PEL is created.

[1]:
  (https://github.com/openbmc/phosphor-logging/blob/master/extensions/openpower-pels/README.md)
[2]:
  (https://gerrit.openbmc.org/#/c/openbmc/docs/+/27804/2/designs/gard_on_bmc.md)
[3]: (https://github.com/openbmc/phosphor-logging)
[4]:
  (https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Logging/Create.interface.yaml)
[5]: (https://github.com/dgibson/dtc)
[6]: (https://github.com/open-power/pdbg)
[7]: (https://elinux.org/Device_Tree_Reference)
[8]:
  (https://github.com/openbmc/phosphor-logging/blob/master/extensions/openpower-pels/registry/message_registry.json)
