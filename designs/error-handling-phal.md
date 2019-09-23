# Power Hardware Abstraction Layer wrapper library for hardware access
Author: Marri Devender Rao
Created: 20/09/2019

## Problem Description
PowerPC FSI Debugger(PDBG) library provides interfaces for hardware access
and initialization. The generic nature of the library written in C cannot
be extended to cater for pre/post access operations, parsing First-failure
data capture(FFDC), creating Platform Event Log(PEL), system state aware
for any recovery operations.

The aim of this proposal is to propose a new wrapper library around libpdbg
that caters for performing pre/post hardware access operations, parsing the
errors returned by the libpdbg, capturing FFDC and creation of event logs.

The wrapper library will cater for  recovery  of Self Boot Engine (SBE) in
case of non responsive state of the SBE.
Detailed design and requirements for SBE recovery will be captured in another
design document 

## Background and References
None.

## Requirements

PHAL wrapper library provides interfaces for following hardware access functions
1. Get/Put/Modify scom
2. Putscom under mask
3. Get/Put/Modify CFAM
4. Get/Put memory
5. Get/Put register
6. Start/Stop/Step instructions

PHAL wrapper library converts the error return codes, FFDC data returned by the
PDBG library into an exception and return to the caller.
Provides interfaces to create PEL's out of the exceptions defined in the library.
Initiate SBE recovery for non-reponsive state of the SBE.
Collect SBE dump for any hardware access failure.

## Glossary
SBE: Self-boot engine. Specialized PORE that initializes the processor chip and
then loads or invokes the hostboot IPL firmware base image

( [SBE](https://github.com/open-power/sbe))
PHAL: Power Hardware Abstraction Layer. [Reference link]
("https://gerrit.openbmc-project.xyz/#/c/openbmc/docs/+/23287/")[Under Review]

libekb: Contains Hardware procedures. A "black box" code module supplied by
the hardware team that initialize hardware in a platform-dependent fashion.

pdbg: Application to allow debugging of the host POWER processors from the BMC.
[pdbg]("https://github.com/open-power/pdbg")
PEL: Platform Event Log
FFDC: First-failure data capture.

## Proposed Design
+------------+
| APPLICATIONS|
+----+-------+   +--------+
     |    +----->+  PDBG  |
     |    |      +--------+
     |    |
     |    |    
   +-v----+-+
   |  PHAL  |
   +-----+--+
         |       +----------+
         +-------> LOGGING  |
                 +----------+

1. Applications will call into PHAL for hardware access
2. PHAL library calls into PDBG for hardware access functions
3. Applications will call into PHAL by passing the exception for parsing the
FFDC data and logging PEL's
4. The library will be implemented in modern C++

+------------+ +-------+  +-------+   +-------------+ +--------------+
| application| | phal  |  |  pdbg |   | logging     | | exception|
+-----+------+ +---+---+  +---+---+   +-----+-------+ +-----+--------+
      |  getScom   |          |             |               |
      +----------> |  pib_read|             |               |
      |            +--------->+             |               |
      |            |          |             |               |
    +-+------------+-----------------------------+          |
    |operation_failed         |             |    |          |
    | |            |  get_ffdc|             |    |          |
    | |            +--------->+             |    |          |
    | |            |          |ffdc, return code |          |
    | |            +------------------------+-------------->+
    | |            +----+     |             |    |          |
    | |            |    |   throw chop_failure exception    |
    | |            +<---+     |             |    |          |
    | |            |          |             |    |          |
    +--------------------------------------------+          |
      |            |          |             |               |
    +-+------------+-----------------------------------------+
    |chip_op_failure exception|             |               ||
    | |            |          |             |               ||
    | | createError|          |             |               ||
    | +----------->+          |create       |               ||
    | |            +------------------------+               ||
    +--------------------------------------------------------+
      |            |         |              |               |
      +            +         +              +               +

### API
sample phal.hpp that provides interfaces for applications to use
namespace openpower
{
namespace phal
{
namespace helper
{
   uint64_t pibRead(struct pdbg_target& target, uint64_t addr);
} //helper

namespace sbe
{
    uint64_t getScom(struct pdbg_target *target, uint64_t addr) throw SbeChipOpError;
} //sbe

namespace error
{
    void createError(const SbeChipOpError& chipOpError);
} //error
} // namespace phal
} // namespace openpower



## Alternatives Considered
Considered extending the existing pdbg library to cater for error handling
and recovery but due it's generic nature and impact on existing users proposing
wrapper library.

## Impacts
None

## Testing
A test application built  will be used for testing API's
Add unit test cases by separating out generic logic not dependent on hardware access.
