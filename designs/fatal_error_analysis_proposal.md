# System Fatal Error Diagnostics for POWER Systems

Author: Zane Shelley

Primary assignee: Zane Shelley

Other contributors: Dean Sanner, Daniel Crowell

Created: 2019-02-14

## Problem Description
In the event of a system fatal error, POWER Systems need the ability to
diagnose the root cause of the failure and perform any service action needed to
avoid repeated system failures.

## Background and References
POWER processor-based systems contain specialized hardware detection circuitry
that is used to detect erroneous hardware operations. The error checker signals
are captured and stored in hardware fault isolation registers (FIRs).
Ultimately, isolation to these FIRs will require service actions which are
dependent on the type of error reported by the FIRs.

Service actions may include:
- Requesting the end user to replace one or more parts in the system.
- Preventing the system from using the the faulty hardware on subsequent boots
  to avoid further outages. On POWER Systems, this is known as a GARD action.

## Requirements
OpenBMC currently does not have the support to directly access registers on
POWER Systems chips. Instead, the register contents will be gathered via the
SBE micro-controller on each of the POWER processors.

Any information regarding details of the POWER chips (register addresses, FIR
hierarchy, etc.), required service actions, part location IDs, and GARD
information will be data driven.

After analysis, the analyzer will need the ability to:
- Communicate to the end user which parts need to be replaced.
- Write GARD information to the POWER processor PNOR flash.
- Log a dump of the register contents related to the failure, and other
  pertinent data, for debugging purposes.

## Proposed Design
**Data File definitions:**
- **Chip Data Files**: These contain the chip register addresses and isolation
  hierarchy. Stored in PNOR, consumed by BMC. Data format TBD.
- **Service Data Files**: This is a map containing service actions required for
  each active error found in the registers. Stored in PNOR, consumed by BMC.
  Data format TBD.
- **Chipset Blueprint**: Defines the location codes and GARD information for
  each serviceable target in the system. Stored in PNOR, consumed by BMC. Will
  use Device Tree format.
- **Register Data Files**: Defines all of the registers the SBE must gather for
  analysis. Built into the SBE image. Data format TBD.

**Diagnostics components on BMC:**
The diagnostics code will be split into two components:
- **Isolator**: The Isolator uses the Chip Data Files as a definition of the
  hardware being analyzed. It will traverse the FIR hierarchy and find all
  active errors reported by the FIRs. It addition, it will collect a dump all
  register data associated with the active errors for debugging purposes.
  **NOTE:** The Isolator will be used in multiple applications within POWER
  systems, not just on OpenBMC. Therefore, this will require a separate
  source repository, which will be used by each of the projects. In addition,
  the Isolator will be written using the **C++11** standard.
- **Analyzer**: This will analyze all of the active errors found by the
  Isolator. It will use the Service Data Files and Chipset Blueprint to perform
  service action specific to the OpenBMC environment. This will require its own
  OpenBMC repository.

**High Level Flow:**
1. A component of OpenBMC initiates diagnostics. **NOTE:** This design does not
   cover how fatal errors are detected by the BMC, nor does it cover when
   diagnostics is initiated. That will be covered in another design proposal.
2. The Analyzer will issue a chip op (via FSI) to all of the SBEs.
3. Each SBE will query and store the values of each register defined by the
   Register Data Files and send the data back to the BMC for processing.
4. The Analyzer will set up necessary structures so that any “hardware reads”
   from the Isolator will actually be from the register data.
5. The Analyzer will tell the Isolator to isolate.
6. Using the Chip Data Files, the Isolator will traverse the register hierarchy
   and return a list of all active errors along with the register dumps
   pertinent to the active errors.
7. The Analyzer will compare the list of active errors against the RAS Data
   Files to determine the most likely root cause of the fatal error.
8. Again referencing the RAS Data Files, the Analyzer will perform any
   necessary service actions for the root cause attention.
9. If a GARD action is required, the Analyzer writes the GARD information
   (sourced from the Chiplet Blueprint) to the GARD partition in the PNOR.
10. The Analyzer will log the error and, if necessary, report the location code
    of the failing part (sourced from the Chiplet Blueprint) to the customer.
11. The Analyzer will then return back to the caller.

## Alternatives Considered
We have considered moving the Isolator logic into the SBE image. This would
eliminate the Register Data Files (which are a redundant subset of the Chip
Data Files) and the associated processing code on the SBE. This would also
reduce the number of registers the SBE will need to access, thus improving
performance and reducing memory usage. Unfortunately, the SBE has very limited
resources and since we have not implemented the Isolator yet, it is unclear if
the Isolator will ever fit in that environment. This could be a possible
enhancement in the future.

## Impacts
No impacts at this time.

## Testing
Still investigating the OpenBMC development environment. It seems possible
with a little bit of tools that we may be able to build a custom simulator in
the QEMU environment. This may be able to allow us to run test cases on
thousands of error scenarios very quickly. In addition, we should add at least
one test case to CI by injecting a fatal error on real hardware so that we can
see the process work end-to-end.

