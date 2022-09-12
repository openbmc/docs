# Impact of Aggregation on openpower-hw-diags

Author:
  Zane Shelley (zane131)

Other contributors:

Created:
  September 6th, 2022

## Problem Description

The impacts of [BMC aggregation][aggr-design] on OpenBMC software will have
significant impacts to some areas of the code base. The goal of this document
is to highlight the challenges (and solutions) that openpower-hw-diags will
have.

Please review the referenced aggregation design for a general overview of the
software design and terms used by this document.

Issues with the System Control Aggregator (SCA) failing and potential fail over
scenarios are outside the scope of this document as there will need to be some
larger system architecture in this area.

[aggr-design]: https://gerrit.openbmc.org/c/openbmc/docs/+/44547/2/designs/redfish-aggregation.md

## Background and References

The openpower-hw-diags application responds to the following hardware
attentions and initiates analysis accordingly:

- System checkstop
- PHYP initiated Terminate Immediate (PHYP TI)
- Hostboot initiated Terminate Immediate (HB TI)
- SBE vital
- Breakpoint

The above attentions will be reported to a sled's BMC via a GPIO pin.

Both checkstop and TI attentions can be caused by a FIR attentions that
originated on another sled (e.g a memory UE consumed by a core).

Checkstop attentions will propagate to all sleds in a multi-sled system over
the SMP links (once configured). Therefore, all sleds within a composed system
will report checkstop attentions to their local BMC.

## Requirements

- Hardware SCOM access must be issued from the local BMCs and can only access
  the connected sled.

- Device tree information will reside on the local BMCs and only scoped to the
  hardware on the connected sled.

- Guard records are scoped to the local BMCs because they are only needed by
  Hostboot during the IPL of a sled.

- Hardware callout location codes will ultimately need to be fully extended
  with rack and sled information. Today, phosphor-logging will add rack/sled
  information when the PEL is committed. However, that will only work if the
  PEL is committed on the BMC connected to the sled needing the hardware
  replacement.

- The SCA will know the SMP topology of all sleds in a composed system.

- Hostboot will IPL sleds independently. During this time, attentions cannot
  propagate to other sleds so isolation, analysis, dumps, reIPLs, etc. should
  scoped to a single sled.

- At the end of the IPL, Hostboot will stitch all the sleds together using the
  SMP links. At this point, the system is considered "SMP coherent" and
  attentions can propagate to other sleds.

## Proposed Design

FIR analysis is issued for checkstop and TI analysis to find any active FIR
attentions in hardware that can be blamed as the root cause of the failure.
Because the root cause may have originated from any sled in a composed system
and not necessarily the sled that reported the failure, FIR analysis must be
initiated on all BMCs in the system from the SCA.

Similarly, post analysis operations like hardware dumps, MP-IPL, and system
reboots will need to be issued to all BMCs in the system from the SCA.

### General FIR Analysis Flow

1. The SCA tells all BMCs in the composed system to do FIR isolation (Redfish
   tasks).

2. Each BMC will query hardware on their connected sled (via SCOM
   operations) to find all active attentions. Then, the BMC will return the
   following back to the SCA (as response to Redfish tasks):

    - A list of all active attentions on the sled.

    - Additional FFDC gathered during isolation (e.g. register capture
      data).

3. Once data has been received from all BMCs, the SCA will look at the complete
   list of active attentions and determine the root cause attention.

4. The SCA sends the root cause attention signature and all FFDC to **only**
   the BMC that reported the root cause attention (Redfish POST OEM).

5. The BMC containing the root cause attention will determine any required
   callouts/service actions and submits a PEL with all FFDC.

### System Checkstop Analysis Flow

1. One or more BMCs detect an attention.

2. The SCA is notified (via Redfish events) of the attention(s).

    - Note that because of checkstop attention propagation, more than one BMC
      may report an attention, but the SCA should only initiate FIR analysis
      once.

3. The SCA initiates FIR Analysis.

4. The SCA requests a hardware dump.

5. The SCA reboots all sleds in the system.

### PHYP TI Analysis Flow

1. A BMC detects an attention.

2. The BMC issues chip-op to get TI information and generates a PEL.

3. The SCA is notified (via Redfish events) of the attention.

4. The SCA initiates FIR Analysis.

5. The SCA requests an MP-IPL.

### Hostboot TI Analysis Flow

1. A BMC detect an attention.

2. The BMC issues chip-op to get TI information and generates a PEL.

3. The SCA is notified (via Redfish events) of the attention.

4. The SCA initiates FIR Analysis.

5. The SCA requests a Hostboot dump.

6. The SCA reboots the sled.

### SBE Vital Analysis Flow

1. A BMC detect an attention.

2. The SCA is notified (via Redfish events) of the attention.

3. The SCA requests an SBE dump.

4. The SCA reboots all sleds in the system.

### Breakpoint Analysis Flow

1. A BMC detect an attention.

2. The attention will be reported to the `croserver` application on the local
   BMC, which will handle its own aggregation. 

## Alternatives Considered

## Impacts

The current openpower-hw-diags application, which currently contains
everything, will need to be split into multiple parts:

    - BMC attention handling service

    - BMC FIR isolation

    - BMC FIR analysis (for applying service actions and submitting PEL)

Then, there will be all new code for the FIR analysis aggregation. Fortunately,
it can re-use the current root cause filtering, but that is only a small
portion of the work.

## Organizational

- Does this repository require a new repository? No

- Which repositories are expected to be modified to execute this design?
    
    - openpower-hw-diags

    - bmcweb, but only if this is used to issue the Redfish commands (TBD). 

## Testing

TBD

