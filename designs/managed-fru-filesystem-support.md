
# Managed FRU Devices

Author: Christopher Sides (citysides)

Other contributors: 

Created: August 17, 2023

## Problem Description

For situations when I2C or filesystem-based FRU blob parsing is not an option 
for retrieving data from a FRU device (physical or emulated), BMC needs a new
mechanism for reading 'Managed FRU' data from the filesystem and making it 
available on D-Bus for use in Entity Manger probe evaluations.

## Background and References

Some platforms like HPE's GXP systems do not provide direct access to a
baseboard FRU device, and instead use a manager to access data, then write
it to a filesystem. 

There is currently no known way to identify HPE platforms via Entity 
Manager probes without this change, because EM probes typically rely on FRU
data for device identification, and this data is not available on HPE 
systems through the existing FRU handling processes.

In HPE Gen 10, platforms used a 'standard' I2C accessible FRU device, and
platform identification data could be parsed in a standard way. 

For HPE Gen 11 and beyond, that I2C accessible FRU is no longer present.
All the same data fields are still being surfaced as before, but field data
is now presented as one-file-per-property on the filesystem instead of as a
FRU blob via I2C -- and the ultimate data source is now a secure element
with no direct access from the BMC. 

The 'FRU data' being surfaced on HPE Gen11 systems is a compilation by the
manager, so there is no longer an internal 'FRU data blob' that can be
directly accessed or dumped to the filesystem for blob parsing. This means
that existing functionality for reading and parsing FRU blob data from the
filesystem isn't applicable.

## Requirements

BMC needs a method of accessing FRU data written one-property-per-file to
the filesystem so that the data can be transferred to D-Bus, making it
available for matching in Entity Manager probe evaluations. 

This functionality is currently believed to be critical for HPE hardware
identification to Entity Manager, but it could also be useful to any system
where non-standard data access may be useful for Entity Manager to identify
a given configuration or piece of hardware. For example, a dip switch's
status could be written to the filesystem, picked up by this process,
then used as a trigger for an Entity Manger probe.

It is beyond the scope of this document to discuss how the data is initially
written to the filesystem before being discovered and copied to D-Bus. The
proposed process only cares that the target files exist to be read, not how
they were generated.

## Proposed Design

High Level Flow: 

- The proposed system will discover 'Managed FRU' data by doing a one-off
  check of the filesystem for the existence of specific directory paths. 

- Each supported 'Managed FRU Type' is associated with a given set of target 
  directories that will be checked for existence. 

- If a directory isn't found, move on to check for the next supported
  'Managed FRU' (if one exists).

- If an associated directory is found, data will be read from any pre-defined
  target files present, and the data will be copied to D-Bus for access by BMC
  components like Entity Manager during probe evaluations.
		
## Alternatives Considered

Adding an HPE-specific 'GXP FRU Device' daemon instead of including this as
part of existing 'FRU device' handling is one alternative considered- but I
currently believe this solution is (or can be) generalized enough that it could
be useful to others without major disruption to current FRU data handling
processes.

## Impacts

No major impacts expected. This is currently aimed as on-off process at
Entity-Manager startup, but even if checks were more frequent, the number
of directories in the filesystem scanned for this process is expected to
remain low. 

Organizational:

No new repository should be needed.

## Testing

Tested by confirming that the expected data is viewable on D-Bus after boot,
and that Entity-Manager probe evaluations can trigger from that data.
 
- Boot OBMC on a system where Managed-FRU-target filepaths exists or is
  created during the boot process (but before Entity-Manager is loaded).

- call 'busctl | grep fru -I' for a listing of FRU devices on D-Bus to
  confirm a device with the expected interface name is listed

- Call 'busctl introspect [FRU path] [FRU path of choice here]' and
  confirm the expected properties appear

