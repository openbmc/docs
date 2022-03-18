# BIOS->BMC SMM Error Logging Queue Daemon

Author:
*  Brandon Kim / brandonkim@google.com / @brandonk

Primary assignee:
*  Brandon Kim / brandonkim@google.com / @brandonk

Other contributors:
*  Marco Cruz-Heredia / mcruzheredia@google.com

Created:
  Mar 15, 2022

## Problem Description

We've identified use cases where the BIOS will go into SMM to provide error
logs to the BMC, requiring messages to be sent as quickly as possible without a
handshake / ack back from the BMC due to the time constraint that it's under.
The goal of this daemon we are proposing is to implement a circular buffer over
a shared BIOS->BMC buffer that the BIOS can fire-and-forget.

## Background and References

There are various ways of communicating between the BMC and the BIOS, but there
are only a few that don't require a handshake and lets the data persist in
shared memory. These will be listed in the "Alternatives Considered" section.

Different BMC vendors support different methods such as Shared Memory (SHM, via
LPC / eSPI) and P2A or PCI Mailbox, but the existing daemon that utilizes them
do it over IPMI blob to communicate where and how much data has been
transferred (see [phosphor-ipmi-flash](https://github.com/openbmc/phosphor-ipmi-flash)
and [libmctp/astlpc](https://github.com/openbmc/libmctp/blob/master/docs/bindings/vendor-ibm-astlpc.md))

## Requirements

The fundamental requirements for this daemon are listed as follows:

1. The BMC shall initialize the shared buffer in a way that the BIOS can
   recognize when it can write to the buffer
2. After initialization, the BIOS shall not have to wait for an ack back from
   the BMC before any writes to the shared buffer (**no synchronization**)
3. The BIOS shall be the main writer to the shared buffer, with the BMC mainly
   reading the payloads, only writing to the buffer to update the header
4. The BMC shall read new payloads from the shared buffer for further processing
5. The BIOS must be able to write a payload (~1KB) to the buffer within 50µs

The shared buffer will be as big as the protocol allows for a given BMC platform
(for Nuvoton's PCI Mailbox for NPCM 7xx as an example, 16KB) and each of the
payloads is estimated to be less than 1KB.

This daemon assumes that no other traffic will communicate through the given
protocol. The circular buffer and its header will provide some protection
against corruption, but it should not be relied upon.

## Proposed Design

The implementation of interfacing with the shared buffer will very closely
follow [phosphor-ipmi-flash](https://github.com/openbmc/phosphor-ipmi-flash). In
the future, it may be wise to extract out the PCI Mailbox, P2A and LPC as
separate libraries shared between `phosphor-ipmi-flash` and this daemon to
reduce duplication of code.

Taken from Marco's (mcruzheredia@google.com) internal design document for the
circular buffer, the data structure of its header will look like the following:

| Name       | Size        | Offset      | Written by   | Description       |
| ---------- | ----------- | ----------- | ------------ | ----------------- |
| BMC Interface Version | 4 bytes | 0x0 | BMC at init | Allows the BIOS to determine if it is compatible with the BMC |
| BIOS Interface Version | 4 bytes | 0x4 | BIOS at init  | Allows the BMC to determine if it is compatible with the BIOS |
| Magic Number | 16 bytes | 0x8 | BMC at init | Magic number to set the state of the queue as described below.  Written by BMC once the memory region is ready to be used. Must be checked by BIOS before logging. BMC can change this number when it suspects data corruption to prevent BIOS from writing anything during reinitialization |
| Queue size | 2 bytes | 0x18 | BMC at init | Indicates the size of the region allocated for the circular queue. Written by BMC on init only, should not change during runtime |
| Uncorrectable Error region size | 2 bytes | 0x1a | BMC at init | Indicates the size of the region reserved for Uncorrectable Error (UE) logs. Written by BMC on init only, should not change during runtime |
| BMC flags | 4 bytes | 0x1c | BMC | <ul><li>BIT0 - BMC UE reserved region “switch”<ul><li>Toggled when BMC reads a UE from the reserved region.</li></ul><li>BIT1 - Overflow<ul><li>Let's BIOS know BMC has seen the overflow incident</li><li>Toggled when BMC acks the overflow incident</li></ul> |
| BMC read pointer | 2 bytes | 0x20 | BMC | Used to allow the BIOS to detect when the BMC was unable to read the previous error logs in time to prevent the circular buffer from overflowing. |
| Reserved | 6 bytes | 0x22 | BMC | May need to leave some reserved bits to ensure BMC is not performing read-update-write on BIOS regions. 6 bytes for 8 byte alignment of the BMC writable sections of this structure. |
| BIOS flags | 4 bytes | 0x28 | BIOS | <ul><li>BIT0 - BIOS UE reserved region “switch”<ul><li> Toggled when BIOS writes a UE to the reserved region.</li></ul><li>BIT1 - Overflow<ul><li>Let's the BMC know that it missed an error log</li><li>Toggled when BIOS sees overflow and not already overflowed</li></ul>|
| BIOS write pointer | 2 bytes | 0x2c | BIOS | Indicates where the next log will be written by BIOS. Used to tell BMC when it should read a new log |
| Padding | 2bytes | 0x2e | Reserved | Padding for 8 byte alignment. Must be 0 |
| Uncorrectable Error reserved region | TBD1 | 0x30 | BIOS | Reserved region only for UE logs. This region is only used if the rest of the buffer is going to overflow and there is no unread UE log already in the region.  |
| Error Logs from BIOS | Size of the Buffer - 0x30 - TBD1 | 0x30 + TBD1 | BIOS | Logs vary by type, so each log will self-describe with a header. This region will fill up the rest of the buffer |

This daemon will first initialize the shared buffer by writing zero to the whole
buffer, then initializing the header's `BMC at init` fields before writing the
`Magic Number`. Once the `Magic Number` is written to, the BIOS will assume that
the shared buffer has been properly initialized, and will be able to start
writing entries to it.

This daemon will poll the buffer at a set interval (the exact number will be
configurable as the processing time and performance may allow for different
polling rate) and once a new payload is detected, the payload will be processed
by a library that can also be chosen and configured at compile-time.

An example of a processing library (and something we would like to push in our
initial version of this daemon) would be an RDE for processing a subset of
Redfish Device Enablement (RDE) commands, and decoding its attached Binary
Encoded JSON (BEJ) payloads.

## Alternatives Considered

* IPMI was considered, did not meet our speed requirement of writing 1KB entry
  in about 10 microseconds.
* LPC / eSPI was also considered but our BMC's SHM buffer was limited to 4KB
  which was not enough for our use case.
* `libmctp` and MCTP PCIe VDM were considered.
  * `libmctp`'s current implementation relies on LPC as the transport binding
    and IPMI KCS for synchronization. LPC as discussed, does not fit our current
    need and synchronization does not work.
  * We may use MCTP PCIe VDM on our future platforms once we have more resources
    with expertise both from the BMC and the BIOS side (which we currently lack)
    for our current project timeline.

## Impacts

Reading from the buffer and processing it may hinder performance of the BMC,
especially if the polling rate is set too high.

## Testing

Unit tests will cover each parts of the daemon, mainly:

*  Initialization
*  Circular buffer processing
*  Decoding / Processing library

