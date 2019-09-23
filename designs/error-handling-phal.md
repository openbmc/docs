# Error handling for PHAL

Author: Marri Devender Rao devenrao@in.ibm.com
Primary assignee: Marri Devender Rao
Other contributors: Jayanth
Created: 20/09/2019

## Problem Description
Develop a wrapper library around pdbg, ekb, pdata libraries to cater for PEL
logging, SBE dump, SBE recovery based on system state for SBE chip-op failures.

Interfaces defined in pdbg, ekb, pdata return error codes that are analyzed and
corresponding errors/PEL's are logged with openpower-logging framework.

## Background and References
None.

## Requirements
libpdbg is a generic library written in C to cater for debugging of host POWER
processors. Because of its generic nature it is not considered for adding extra
requirements like
1.Logging of journal logs
2.Creating PEL logs with the FFDC data.
3.Creating PEL logs for intermittent errors in request/reply messages to the
SBE.
4)Catering for SBE recovery during SBE communication failure.

Facilitate in parsing of return codes returned by he hardware procedures
defined in libekb and create PEL logs.

Applications to do hardware access will call into the wrapper library and
commits the error/PEL logs for failure cases using the interfaces provided by
the wrapper library.

## Glossary
SBE: Self-boot engine. Specialized PORE that initializes the processor chip and
then loads or invokes the hostboot IPL firmware base image
( [SBE](https://github.com/open-power/sbe))

pHAL: Power Hardware Abstraction Layer. [Reference link]
("https://gerrit.openbmc-project.xyz/#/c/openbmc/docs/+/23287/")[Under Review]

libekb: Contains HW procedures. A "black box" code module supplied by the HW
team that initializes CEC hardware in a platform-dependent fashion.

pdbg: Application to allow debugging of the host POWER processors from the BMC.
[pdbg]("https://github.com/open-power/pdbg")

pldm: To send requests to host for recovery of SBE
[PLDM](https://github.com/openbmc/pldm)


## Proposed Design

+------------+
| APPLCATIONS|
+----+-------+   +--------+
     |    +----->+  PDBG  |
     |    |      +--------+
     |    |
     |    |      +---------+
   +-v----+-+--->> PDBG    |
   |  PHAL  |    +---------+
   +-----+--+
         |       +----------+
         +-------> LOGGING  |
                 +----------+

1. Applications will call into PHAL for hardware access
2. PHAL library calls into PDBG for hardware access functions
3. PHAL calls into PDBG for recovery of SBE during communication failure.
4. PHAL using D-Bus interfaces calls into openpower-logging/phosphor-logging
to create and commit PEL logs.
5. Applications will call into PHAL for parsing the FFDC data and logging PEL's
6. The library will be implemented in modern C++

+------------+ +-------+  +-------+   +-------------+ +--------------+
| application| | phal  |  |  pdbg |   | logging     | | chiop|failure|
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

## Alternatives Considered
Considered extending the exisging pdbg library to cater for error handling and
recovery but due it's generic nature and impact on existing users proposing
wrapper library.

## Impacts
Applications need to call into phal library for error handling and not directly
calling into pdbg.

## Testing
Unit test will be added by mocking the API'S provided by PHAL and ensuring
corresponding error logs created and to check SBE recovery actions if any
initiated.
