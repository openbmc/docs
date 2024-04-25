# Platform Secure Boot Config Information

Author: DelphineCCChiu (<Delphine_CC_Chiu@wiwynn.com>)

Created: 04/24/2024

## Problem Description

AMD PSB provides a hardware to authenticate the initial Firmware during boot
process. PSB config in CPU EEprom stores the information and status of PSB which
is helpful for debugging and status checking. This design provides the solution
of getting PSB config via BMC over PLDM protocol with PLDM get FRU record
command which is a standard format in PLDM specification. Other applications can
also store PSB config through different protocols.

## Background and References:

PLDM for FRU Data transfer: Platform Level Data Model (PLDM) for FRU Data 5
Specification Section 9
<https://www.dmtf.org/sites/default/files/standards/documents/DSP0257_1.0.0.pdf>

## Requirements

This feature supports AMD platform. This feature requires an endpoint
device which stores PSB config and also supports PLDM GetFRURecordTable and
GetFRUTableMetadata command.

## Proposed Design

### Interface

This design introduces a new interface for PSB config: Service:
"xyz.openbmc_project.PLDM" Object:
"/xyz/openbmc_project/inventory/Item/CPU/PSB/CPU_ENDPOINT_ID" Interface:
"xyz.openbmc_project.Inventory.Item.CPU.PSB"

Follows are the properties in xyz.openbmc_project.Inventory.Item.CPU.PSB
interface.

Properties:

PlatformVendorID PlatformModelID BIosKeyRevisionID RootKeySelect
PlatformSecureBootEn DisableBiosKeyAntiRollback DisableAmdKeyUsage
DisableSecureDebugUnlock CustomerKeyLock PSBStatus PSBFusingReadiness
HSTIStatePSPSecureEn HSTIStatePSPPlatformSecureEn HSTIStatePSPDebugLockOn

Using standard PLDM command: GetFRURecordTableMetadata (0x01) to get the
description of PSB FRU record set. Using standard PLDM command:
GetFRURecordTable (0x02) to get the PSB config record.
 
## Alternatives Considered

None.

## Impacts

None.

## Testing

Using PLDM command GetFRURecordTableMetadata and GetFRURecordTable for dumping
and processing the PSB record.
