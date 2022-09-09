# Dump Manager Design

Author:
  Dhruvaraj Subhashchandran <dhruvaraj@in.ibm.com>

Other contributors:

Created: 12/12/2019

## Problem Description
During a crash or a host failure, an event monitor mechanism generates an error
log, but the size of the error log is limited to few kilobytes, so all the data
from the crash or failure may not fit into an error log. The additional data
required for the debugging needs to be collected as a dump.
The existing OpenBMC dump interfaces support only the dumps generated on
the BMC and dump manager doesn't support download operations.

## Glossary

- **System Dump**: A dump of the Host's main memory and processor registers.
    [Read More](https://en.wikipedia.org/wiki/Core_dump)
- **Memory Preserving Reboot(MPR)**: A method of reboot with preserving the
    contents of the volatile memory
- **PLDM**: An interface and data model to access low-level platform inventory,
    monitoring, control, event, and data/parameters transfer functions.
    [ReadMore](https://github.com/openbmc/docs/blob/master/designs/pldm-stack.md)
- **Machine Check Exception**: A severe error inside a processor core that
    causes a processor core to stop all processing activities.
- **BMCWeb**: An embedded webserver for OpenBMC. [More Info](https://github.com/openbmc/bmcweb/blob/master/README.md)

## Background and References

### Type of dumps supported.
These are some of the dumps supported by dump manager.

## Requirements

## Proposed Design

## Alternatives Considered

## Impacts

## Testing
