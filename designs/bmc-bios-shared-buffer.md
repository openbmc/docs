# BMC-BIOS Shared Buffer Daemon

Author:
*  Brandon Kim / brandonkim@google.com / @brandonk

Primary assignee:
*  Brandon Kim / brandonkim@google.com / @brandonk

Other contributors:
*  Marco Cruz-Heredia / mcruzheredia@google.com

Created:
  Mar 15, 2022

## Problem Description
We've identified usecases where the BIOS requires sending messages as quikcly as
possible to the BMC without a handshake / ack back from the BMC due to the time
constraint that it's under. The goal of this daemon we are proposing is to
implement a circular buffer over a shared BMC-BIOS buffer that the BIOS can fire
and forget.

## Background and References
There are various ways of communicating between the BMC and the BIOS, but there
are only a few that don't require a handshake from both sides by letting the
data persist in shared memory.

Different BMC vendors support different methods such as Shared Memory (SHM, via
LPC / eSPI) and P2A or PCI Mailbox, but the existing daemon that utilize them
do it over IPMI blob to communicate where and how much data has been
transferred (see [phosphor-ipmi-flash](https://github.com/openbmc/phosphor-ipmi-flash)).

## Requirements
The fundamental requirements for this daemon are listed as follows:

1. The BMC shall initialize the shared buffer in a way that the BIOS can
   recognize when it can write to the buffer
2. After initialization, the BIOS shall not have to wait for an ack back from
   the BMC before any writes to the shared buffer.
3. The BIOS shall be the main writer to the shared buffer, with the BMC mainly
   reading the payloads, only writing to the buffer to update the header
4. The BMC shall read new payloads from the shared buffer and store it in its
   tmpfs after any processing
5. The BMC shall provide a hook for where a configurable library can be utilized to
   process and or decode the payload

The shared buffer will be as big as the protocol allows for a given BMC platform
(for Nuvoton's PCI Mailbox for NPCM 7xx as an exmaple, 16KB) and each of the
payloads are estimated to be less than 1KB.

This daemon assumes that no other traffic will communicate through the given
protocol. The circular buffer and its header will provide some protection
against corruption, but it should not be relied upon.

## Proposed Design

The implementation of interfacing with the shared buffer will very closely
follow [phosphor-ipmi-flash](https://github.com/openbmc/phosphor-ipmi-flash). In
the future, it may be wise to extract out the PCI Mailbox, P2A and LPC as a
separate libraries shared between `phosphor-ipmi-flash` and this daemon to
reduce duplication of code.

Taken from Marco's (mcruzheredia@google.com) internal design document for the
circular buffer, the data structure of its header will look like the following:

| Name       | Size        | Offset      | Written by   | Description       |
| ---------- | ----------- | ----------- | ------------ | ----------------- |
| BMC Interface Version | 4 bytes | 0x0 | BMC at init | Allows the BIOS to determine if it is compatible with the BMC |
| BIOS Interface Version | 4 bytes | 0x4 | BIOS at init  | Allows the BMC to determine if it is compatible with the BIOS |
| Magic Number | 16 bytes | 0x8 | BMC at init | Magic number to set the state of the queue as described below.  Written by BMC once the memory region is ready to be used. Must be checked by BIOS before logging. BMC can change this number when it suspects data corruption to prevent BIOS from writing anything during reinitialization |
| Normal Queue size | 2 bytes | 0x18 | BMC at init | Indicates the size of the region allocated for the circular queue. Written by BMC on init only, should not change during runtime |
| Reserved Queue size | 2 bytes | 0x1a | BMC at init | Indicates the size of the region reserved for important entries. Written by BMC on init only, should not change during runtime |
| BMC flags | 4 bytes | 0x1c | BMC | <ul><li>BIT0 - BMC reserved queue “switch”<ul><li>Toggled when BMC reads an entry from reserved region.</li></ul><li>BIT1 - Overflow<ul><li>Let's BIOS know BMC has seen the overflow incident</li><li>Toggled when BMC acks the overflow incident</li></ul> |
| BMC read pointer | 2 bytes | 0x20 | BMC | Used to allow the BIOS to detect when the BMC was unable to read a normal queue entry in time to prevent the circular buffer from overflowing. |
| Reserved | 6 bytes | 0x22 | BMC | May need to leave some reserved bits to ensure BMC is not performing read-update-write on BIOS regions. 6 bytes for 8 byte alignment of the BMC writable sections of this structure. |
| BIOS flags | 4 bytes | 0x28 | BIOS | <ul><li>BIT0 - BIOS reserved queue “switch”<ul><li> Toggled when BIOS writes an entry from reserved region.</li></ul><li>BIT1 - Overflow<ul><li>Let's the BMC know that it missed an entry</li><li>Toggledwhen BIOS sees overflow and not already overflowed</li></ul>|
| BIOS write pointer | 2 bytes | 0x2c | BIOS | Indicates where the next entry will be written by BIOS. Used to tell BMC when it should read a new entry |
| Padding | 2bytes | 0x2e | Reserved | Padding for 8 byte alignment. Must be 0 |
| Reserved queue region | TBD1 | 0x30 | BIOS | Reserved region only for important entries that should not be overwritten. The only time a normal entry can be written here is if the rest of the buffer is going to overflow and there is no unread important entries already in the region  |
| Normal queue region | Size of the Buffer - 0x30 - TBD1 | 0x30 + TBD1 | BIOS | Entries vary by type, so each entry will self-describe with a header. This region will fill up the rest of the buffer |

This daemon will first initialize the shared buffer by writing zero to the whole
buffer, then initializing the header's `BMC at init` fields before writing the
`Magic Number`. Once the `Magic Number` is written to, the BIOS will assume that the
shared buffer has been properly initialized, and will be able to start writing
entries to it.

This daemon will poll the buffer at a set interval (the exact number will be
configurable as the processing time and performance may allow for different
polling rate) and once a new payload is detected, the payload will be processed
by a library that can also be chosen and configured at compile-time.

An example of a processing library (and something we would like to push in our
initial version of this daemon) would be an RDE for processing a subset of
Redfish Device Enablement (RDE) commands, and decoding its attached Binary
Encoded JSON (BEJ) payloads.

## Alternatives Considered
* IPMI was considered, did not meet our speed requirement of writing 1KB entry in
about 10 microseconds.

* LPC / eSPI was also considered but our BMC's SHM buffer was limited to 4KB which
was not enough for our use case.

## Impacts
Reading from the buffer and processing it may hinder performance of the BMC,
especially if the polling rate is set too high.

## Testing
Unit tests will cover each parts of the daemon, mainly:
*  Initialization
*  Circular buffer processing
*  Decoding / Processing library

