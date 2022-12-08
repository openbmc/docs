# In-Band Update of BMC Firmware (and others) using OEM IPMI Blob Transport

Author: Patrick Venture <venture!>

Created: 2018-10-18

## Problem Description

The BMC needs a mechanism for receiving a new firmware image from the host
through a variety of mechanisms. This can best be served with one protocol into
which multiple approaches can be routed.

## Background and References

BMC hardware provides at a minimum some interface for sending and receiving IPMI
messages. This hardware may also provide regions that can be memory mapped for
higher speed communication between the BMC and the host. Certain infrastructures
do not provide network access to the BMC, therefore it is required to provide an
update mechanism that can be done in-band between the host and the BMC.

In-band here refers to a communications channel that is directly connected
between the host and BMC.

1.  Serial
1.  IPMI over LPC
1.  IPMI over i2c
1.  LPC Memory-Mapped Region
1.  P2A bridge

## Primer

Please read the IPMI BLOB protocol design as primer
[here](https://github.com/openbmc/phosphor-ipmi-blobs/blob/master/README.md).

## Requirements

The following statements are reflective of the initial requirements.

- Any update mechanism must provide support for UBI tarballs and legacy (static
  layout) flash images. Leveraging the BLOB protocol allows a system to provide
  support for any image type simply by implementing a mechanism for handling it.

- Any update mechanism must allow for triggering an image verification step
  before the image is used.

- Any update mechanism must allow implementing the data staging via different
  in-band mechanisms.

- Any update mechanism must provide a handshake or equivalent protocol for
  coordinating the data transfer. For instance, whether the BMC should enable
  the P2A bridge and what region to use or whether to turn on the LPC memory map
  bridge.

- Any update mechanism must attempt to maintain security, insomuch as not
  leaving a memory region open by default. For example, before starting the
  verification step, access to the staged firmware image must not be still
  accessible from the host.

## Proposed Design

OpenBMC supports a BLOB protocol that provides primitives. These primitives
allow a variety of different "handlers" to exist that implement those primitives
for specific "blobs." A blob in this context is a file path that is strictly
unique.

Sending the firmware image over the BLOB protocol will be done via routing the
[phosphor-ipmi-flash design](https://github.com/openbmc/phosphor-ipmi-flash/blob/master/README.md)
through a BLOB handler. This is meant to supplant `phosphor-ipmi-flash`'s
current approach to centralize on one flexible handler.

### Sequencing Control

To enforce sequencing control, the design requires that only one blob be open at
a time. If the verification blob is open, the other blobs cannot be opened, and
likewise if a client has a data blob open, the verification blob cannot be
opened.

### Defining Blobs

The BLOB protocol allows a handler to specify a list of blob ids. This list will
be leveraged to specify whether the platform supports either the legacy (static
layout) or the UBI mechanism, or both. The flags provided to the open command
identify the mechanism selected by the client-side. The stat command will return
the list of supported mechanisms for the blob.

The blob ids for the mechanisms will be as follows:

| Flash Blob Id    | Type            |
| ---------------- | --------------- |
| `/flash/image`   | Static Layout   |
| `/flash/tarball` | UBI             |
| `/flash/bios`    | Host BIOS image |

The flash handler will determine what commands it should expect to receive and
responses it will return given the blob opened, based on the flags provided to
open.

The flash handler will only allow one of the above blobs to be opened for a
sequence of commands, such that you cannot open `/flash/image` and then open
`/flash/bios` without completing (or later aborting) the first update process
started.

The following blob ids are defined for storing the hash for the image:

| Hash Blob     | Id Mechanism                    |
| ------------- | ------------------------------- |
| `/flash/hash` | Whichever flash blob was opened |

The flash handler will only allow one open file at a time, such that if the host
attempts to send a firmware image down over IPMI BlockTransfer, it won't allow
the host to start a PCI send until the BlockTransfer file is closed.

There is only one hash "file" mechanism. The exact hash used will only be
important to your verification service. The value provided will be written to a
known place.

When a transfer is active, it'll create a blob_id of `/flash/active/image` and
`/flash/active/hash`.

#### Verification Blob

The following blob id is defined once the image or hash upload has started. Its
purpose is to trigger and monitor the firmware verification process. Therefore,
the BmcBlobOpen command will fail until both the hash and image file are closed.
Further on the ideal command sequence below.

| Trigger Blob    | Note                     |
| --------------- | ------------------------ |
| `/flash/verify` | Verify Trigger Mechanism |

When the verification file is closed, if verification was completed
successfully, it'll add an update blob id, defined below.

The verification process used is not defined by this design.

#### Update Blob

The update blob id is available once `/flash/verify` is closed with a valid
image or tarball. The update blob needs to be opened and commit() called on that
blob id to trigger the update mechanism.

The update process can be checked periodically by calling stat() on the update
blob id.

| Update Blob     | Note                     |
| --------------- | ------------------------ |
| `/flash/update` | Trigger Update Mechanism |

The update process used is not defined by this design.

#### Cleanup Blob

The cleanup blob id is always present. The goal of this blob is to handle
deletion of update artifacts on failure, or success. It can be implemented to do
any manner of cleanup required, but for systems under memory pressure, it is a
convenient cleanup mechanism.

The cleanup blob has no state or knowledge and is meant to provide a simple
system cleanup mechanism. This could also be accomplished by warm rebooting the
BMC. The cleanup blob will delete a list of files. The cleanup blob has no state
recognition for the update process, and therefore can interfere with an update
process. The host tool will only use it on failure cases. Any other tool
developed should respect this and not employ it unless the goal is to cleanup
artifacts.

To trigger the cleanup, simply open the blob, commit, and close. It has no
knowledge of the update process. This simplification is done through the design
of a convenience mechanism instead of a required mechanism.

| Cleanup Blob     | Note                      |
| ---------------- | ------------------------- |
| `/flash/cleanup` | Trigger Cleanup Mechanism |

### Caching Images

Similarly to the OEM IPMI Flash protocol, the flash image will be staged in a
compile-time configured location.

Other mechanisms can readily be added by adding more blob ids or flags to the
handler.

### Commands

The update mechanism will expect a specific sequence of commands depending on
the transport mechanism selected. Some mechanisms require a handshake.

#### BlockTransfer Sequence

1.  Open (for Image or tarball)
1.  Write
1.  Close
1.  Open (`/flash/hash`)
1.  Write
1.  Close
1.  Open (`/flash/verify`)
1.  Commit
1.  SessionStat (to read back verification status)
1.  Close
1.  Open (`/flash/update`)
1.  Commit
1.  SessionStat (to read back update status)
1.  Close

#### P2A Sequence

1.  Open (for Image or tarball)
1.  SessionStat (P2A Region for P2A mapping)
1.  Write
1.  Close
1.  Open (`/flash/hash`)
1.  SessionStat (P2A Region)
1.  Write
1.  Close
1.  Open (`/flash/verify`)
1.  Commit
1.  SessionStat (to read back verification status)
1.  Close
1.  Open (`/flash/update`)
1.  Commit
1.  SessionStat (to read back update status)
1.  Close

#### LPC Sequence

1.  Open (for image or tarball)
1.  WriteMeta (specify region information from host for LPC)
1.  SessionStat (verify the contents from the above)
1.  Write
1.  Close
1.  Open (`/flash/hash`)
1.  WriteMeta (LPC Region)
1.  SessionStat (verify LPC config)
1.  Write
1.  Close
1.  Open (`/flash/verify`)
1.  Commit
1.  SessionStat (to read back verification status)
1.  Close
1.  Open (`/flash/update`)
1.  Commit
1.  SessionStat (to read back update status)
1.  Close

### Stale Images

If an image update process is started but goes stale there are multiple
mechanisms in place to ensure cleanup. If a session is left open after the blob
timeout period it'll be closed. Because expiration is not the same action as
closing, the cache will be flushed and any staged pieces deleted.

The image itself, in legacy (static layout) mode will be placed and named in
such a way that it will disappear if the BMC reboots. In the UBI case, the file
will be stored in `/tmp` and deleted accordingly.

At any point during the upload process, one can abort by closing the open blobs
and deleting them by name.

### Blob Primitives

The update mechanism will implement the Blob primitives as follows.

#### BmcBlobOpen

The blob open primitive allows supplying blob specific flags. These flags are
used for specifying the transport mechanism. To obtain the list of supported
mechanisms on a platform, see the `Stat` command below.

```
enum OpenFlags
{
    read = (1 << 0),
    write = (1 << 1),
};

/* These bits start in the blob specific range of the flags. */
enum FirmwareUpdateFlags
{
    bt = (1 << 8),   /* Expect to send contents over IPMI BlockTransfer. */
    p2a = (1 << 9),  /* Expect to send contents over P2A bridge. */
    lpc = (1 << 10), /* Expect to send contents over LPC bridge. */
};
```

An open request must specify that it is opening for writing and one transport
mechanism, otherwise it is rejected. If the request is also set for reading,
this is not rejected but currently provides no additional value.

Once opened a new file will appear in the blob_id list (for both the image and
hash) indicating they are in progress. The name will be `flash/active/image` and
`flash/active/hash` which has no meaning beyond representing the current update
in progress. Closing the file does not delete the staged images. Only delete
will.

**_Note_** The active image blob_ids cannot be opened. This can be reconsidered
later.

#### BmcBlobRead

This will initially not perform any function and will return success with 0
bytes.

#### BmcBlobWrite

The write command's contents will depend on the transport mechanism. This
command must not return until it has copied the data out of the mapped region
into either a staging buffer or written down to a staging file. How the command
reads from the mapped region is beyond the scope of this design.

##### If BT

The data section of the payload is only data.

##### If P2A

The data section of the payload is the following structure:

```
struct ExtChunkHdr
{
    uint32_t length; /* Length of the data queued (little endian). */
};
```

##### If LPC

The data section of the payload is the following structure:

```
struct ExtChunkHdr
{
    uint32_t length; /* Length of the data queued (little endian). */
};
```

#### BmcBlobCommit

If this command is called on the session of the firmware image itself, nothing
will happen at present. It will return a no-op success.

If this command is called on the session for the hash image, nothing will happen
at present. It will return a no-op success.

If this command is called on the session for the verify blob id, it'll trigger a
systemd service `verify_image.service` to attempt to verify the image. Before
doing this, if the transport mechanism is not IPMI BT, it'll shut down the
mechanism used for transport preventing the host from updating anything.

When this is started, only the BmcBlobSessionStat command will respond. Details
on that response are below under BmcBlobSessionStat.

#### BmcBlobClose

Close must be called on the firmware image and the hash file before opening the
verify blob.

If the `verify_image.service` returned success, closing the verify file will
have a specific behavior depending on the update. If it's UBI, it'll perform the
install. If it's legacy (static layout), it'll do nothing. The verify_image
service in the legacy case is responsible for placing the file in the correct
staging position. A BMC warm reset command will initiate the firmware update
process.

If the image verification fails, it will automatically delete any files
associated with the update.

**_Note:_** During development testing, a developer will want to upload files
that are not signed. Therefore, an additional bit will be added to the flags to
change this behavior.

#### BmcBlobDelete

Aborts any update that's in progress:

1.  Stops the verify_image.service if started.
1.  Deletes any staged files.

In the event the update is already in progress, such as the tarball mechanism is
used and in the middle of updating the files, it cannot be aborted.

#### BmcBlobStat

Blob stat on a blob_id (not SessionStat) will return the capabilities of the
blob_id handler.

```
struct BmcBlobStatRx {
    uint16_t crc16;
    /* This will have the bits set from the FirmwareUpdateFlags enum. */
    uint16_t blob_state;
    uint32_t size; /* 0 - it's set to zero when there's no session */
    uint8_t  metadata_len; /* 0 */
};
```

#### BmcBlobSessionStat

If called pre-commit, it'll return the following information:

```
struct BmcBlobStatRx {
    uint16_t crc16;
    uint16_t blob_state; /* OpenFlags::write | (one of the interfaces) */
    uint32_t size; /* Size in bytes so far written */
    uint8_t  metadata_len; /* 0. */
};
```

If it's called and the data transport mechanism is P2A, it'll return a 32-bit
address for use to configure the P2A region as part of the metadata portion of
the `BmcBlobStatRx`.

```
struct BmcBlobStatRx {
    uint16_t crc16;
    uint16_t blob_state; /* OpenFlags::write | (one of the interfaces) */
    uint32_t size; /* Size in bytes so far written */
    uint8_t  metadata_len = sizeof(struct P2ARegion);
    struct P2ARegion {
        uint32_t address;
    };
};
```

If called post-commit on the verify file session, it'll return:

```
struct BmcBlobStatRx {
    uint16_t crc16;
    uint16_t blob_state; /* OPEN_W | (one of the interfaces) */
    uint32_t size; /* Size in bytes so far written */
    uint8_t  metadata_len; /* 1. */
    uint8_t  verify_response; /* one byte from the below enum */
};

enum VerifyCheckResponses
{
    VerifyRunning = 0x00,
    VerifySuccess = 0x01,
    VerifyFailed  = 0x02,
    VerifyOther   = 0x03,
};
```

If called post-commit on the update file session, it'll return:

```
struct BmcBlobStatRx {
    uint16_t crc16;
    uint16_t blob_state; /* OPEN_W | (one of the interfaces) */
    uint32_t size; /* Size in bytes so far written */
    uint8_t  metadata_len; /* 1. */
    uint8_t  update_response; /* one by from the below enum */
};

enum UpdateStatus
{
    UpdateRunning = 0x00,
    UpdateSuccessful = 0x01,
    UpdateFailed = 0x02,
    UpdateStatusUnknown = 0x03
};
```

The `UpdateStatus` and `VerifyCheckResponses` are currently identical, but this
may change over time.

#### BmcBlobWriteMeta

The write metadata command is meant to allow the host to provide specific
configuration data to the BMC for the in-band update. Currently that is only
aimed at LPC which needs to be told the memory address so it can configure the
window.

The write meta command's blob will be this structure:

```
struct LpcRegion
{
    uint32_t address; /* Host LPC address where the chunk is to be mapped. */
    uint32_t length; /* Size of the chunk to be mapped. */
};
```

## Alternatives Considered

There is currently another implementation in-use by Google that leverages the
same mechanisms, however, it's not as flexible because every command is a custom
piece. Mapping it into blobs primitives allows for easier future modification
while maintaining backwards compatibility (without simply adding a separate OEM
library to handle a new process, etc).

## Impacts

This impacts security because it can leverage the memory mapped windows. There
is not an expected performance impact, as the blob handler existing only
generates a couple extra entries during the blob enumerate command's response.

## Testing

Where possible (nearly everywhere), mockable interfaces will be used such that
the entire process has individual unit-tests that verify flags are checked, as
well as states and sequences.

### Scenarios

#### Sending an image with a bad hash

A required functional test is one whereby an image is sent down to the BMC,
however the signature is invalid for that image. The expected result is that the
verification step will return failure and the files will be deleted from the BMC
without user intervention.

#### Sending an image with a good hash

A required functional test is one whereby an image is sent down to the BMC with
a valid signature. The expected result is that the verification step will return
success.

## Configuration

See the configuration section of
[Secure Flash Update Mechanism](https://github.com/openbmc/phosphor-ipmi-flash/blob/master/README.md)
