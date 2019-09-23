# PowerPC FSI Debugger wrapper and error log library

Author: Marri Devender Rao<br/>
Created: 20/09/2019

## Problem Description
PowerPC FSI Debugger(PDBG) library provides interfaces for hardware access and
initialization. 

The generic nature of the library written in C cannot be extended to cater for
pre/post access operations, parsing failure data, catering for deconfig/guard
of the failed components, creating Platform Event Log(PEL), and performing
recovery operations.

The aim of this proposal is to develop PowerPC FSI Debugger(PDBG) wrapper
library to overcome the limitations of PowerPC FSI Debugger(PDBG).

Add Power Hardware Abstraction Layer error log library for handling of errors
returned by  PowerPC FSI Debugger(PDBG) wrapper library and hardware procedure
execution.


## Background and References
None.

## Requirements
Add PowerPC FSI Debugger(PDBG) wrapper library and Power Hardware Abstraction
Layer(PHAL) error log library.

### PowerPC FSI Debugger(PDBG) wrapper library
Interfaces for hardware access, read/write registers, read/write system
memory, and instruction control.

Conversion of the error return codes, failure data returned by the PowerPC
FSI Debugger(PDBG) into error handles.

Capturing callout/deconfig/guard data if any as part of the error handle.

Perform recovery of self boot engine for non responsive state of the self
boot engine.

Captures self boot engine debug data for self boot engine operation failures.

### Power Hardware Abstraction Layer(PHAL) error log library
Defines error handles which the PowerPC FSI Debugger(PDBG) wrapper library
can return.

Define error handles for the errors returned by the hardware procedures.

Conversion of error handles to platform event logs.

Interface with logging library through D-Bus to create platform event logs.

## Glossary
SBE: Self-boot engine. Specialized PORE that initializes the processor chip and
then loads or invokes the hostboot boot firmware base image.
( [SBE](https://github.com/open-power/sbe))

PHAL: Power Hardware Abstraction Layer.
[PHAL]("https://gerrit.openbmc-project.xyz/#/c/openbmc/docs/+/23287/")

libekb: A "black box" code module supplied by the hardware team that initialize
hardware in a platform-dependent fashion.

PDBG: PowerPC FSI Debugger (PDBG), application to allow debugging of the host
POWER processors from the BMC. It works in a similar way to JTAG programmers for
embedded system development in that it allows you to access GPRs, SPRs and
system memory.
[pdbg]("https://github.com/open-power/pdbg")

PEL: Platform Event Log

## Proposed Design
### Component diagram
```
                     +--------------+
       +-------------> PDBG wrapper +----------+
       |             +------+-------+          |
+------+------+             |                  v
| Application |             |               +--+---+
+--+----------+  ++---------+               | PDBG |
   |              |                         +------+
   |              |
   |   +----------v-------+          +-----------------+
   +--->Error Log Library +--------->+phosphor logging |
       +------------------+          +-----------------+

```
### Sequence diagram

```
 +-------------+  +---------+   +-------+   +-------------+  +---------------+   +-----------------+
 | Application |  |Wrapper  |   | pdbg  |   | Error handle|  | phal error log|   | phosphor|logging|
 +-----+-------+  +---+-----+   +---+---+   +-------+-----+  +----+----------+   +-------+---------+
       |              |             |               |             |                      |
       |   getSPR     |             |               |             |                      |
       +------------->+             |               |             |                      |
       |              |  pib_read   |               |             |                      |
       |              +------------>+               |             |                      |
       |              |             |               |             |                      |
+------+-----------------------------------------------------------------------------------+
| operation failed    |             |               |             |                      | |
|      |              |  get failure data           |             |                      | |
|      |              +------------>|               |             |                      | |
|      |              |             |               |             +                      | |
|      |              |         create(rc, failure, deconfig, guard data                 | |
|      |              +-------------+--------------->             |                      | |
|      | error handle |             |               |             |                      | |
|      +<-------------+             |               |             |                      | |
|      |              |  reportError(Errorhandle, severity)       |                      | |
|      +----------------------------+---------------------------->+                      | |
|      |              |             |               |             |   create             | |
|      |              |             |               |            +---------------------->+ |
|      |              |             |               |             |                      | |
|      |              |             |               |             |                      | |
|      |              |             |               |             |                      | |
+------------------------------------------------------------------------------------------+
       |              |             |               |             +-+                    +

```

## Alternatives Considered
Considered extending the existing PowerPC FSI Debugger (PDBG) library to cater
for error handling and recovery but due it's generic nature and impact on 
existing users proposing wrapper library.

## Impacts
None

## Testing
A test application built  will be used for testing API.
Add unit test cases by separating out generic logic not dependent on hardware.
