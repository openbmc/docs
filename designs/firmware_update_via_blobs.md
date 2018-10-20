# BMC Firmware Update using OEM IPMI Blob Transport

Author: Patrick Venture <venture!>

Primary assignee: Patrick Venture

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

## Requirements

The following statements are reflective of the initial requirements.

*   Any update mechanism must provide support for UBI tarballs and legacy flash
    images. Leveraging the BLOB protocol allows a system to provide support for
    any image type simply by implementing a mechansim for handling it.

*   Any update mechanism must allow for triggering an image verification step
    before the image is used.

*   Any update mechanism must allow implementing the data staging via different
    in-band mechanisms.

*   Any update mechansim must provide a handshake or equivalent protocol for
    coordinating the data transfer. For instance, whether the BMC should enable
    the P2A bridge and what region to use or whether to turn on the LPC memory
    map bridge.

*   Any update mechanism must attempt to maintain security, insomuch as not
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

Defining Blobs The BLOB protocol allows a handler to specify a list of blob ids.
This will be leveraged to specify whether the platform supports legacy and or
UBI. The flags provided to open list identify the mechanism selected by the
client-side. The stat command will return the list of supported mechanisms for
the blob.

The blob ids for the mechanisms will be as follows:

Flash Blob Id  | Type
-------------- | ------
/flash/image   | Legacy
/flash/tarball | UBI

The flash handler will determine what commands it should expect to receive and
responses it will return given the blob opened, based on the flags provided to
open.

The following blob ids are defined for storing the hash for the image:

Hash Blob           | Id Mechanism
------------------- | ------------
/flash/hash         | Legacy
/flash/hash/tarhash | UBI

The flash handler will only allow one open file at a time, such that if the host
attempts to send a firmware image down over BlockTransfer, it won't allow the
host to start a PCI send until the BlockTransfer file is closed. Caching Images
Similarly to the OEM IPMI Flash protocol, the flash image will be staged in a
compile-time configured location.

Other mechanisms can readily be added by adding more blob_ids or flags to the
handler.

### Commands

The update mechanism will expect a specific sequence of command depending on the
transport mechanism selected. Some mechanisms require a handshake.

#### BlockTransfer Sequence

1.  Open (for Image or tarball)
1.  Write
1.  Close
1.  Open (for Hash)
1.  Write
1.  Commit
1.  Close

#### P2A Sequence

1.  Open (for Image or tarball)
1.  SessionStat (Request Region for P2A mapping)
1.  Write
1.  Close
1.  Open (for hash)
1.  SessionStat
1.  Write
1.  Commit
1.  Close

#### LPC Sequence

1.  Open (for image or tarball)
1.  WriteMeta (specify region information from host for LPC)
1.  SessionStat (verify the contents from the above)
1.  Write
1.  Close
1.  Open (for hash)
1.  WriteMeta (...)
1.  SessionStat (...)
1.  Write
1.  Commit
1.  Close

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

enum FirmwareUpdateFlags
{
    bt = (1 << 8),   /* Expect to send contents over IPMI BlockTransfer. */
    p2c = (1 << 9),  /* Expect to send contents over P2A bridge. */
    lpc = (1 << 10), /* Expect to send contents over LPC bridge. */
};
```

An open request must specify that it is opening for writing and one transport
mechanism, otherwise it is rejected. If the request is also set for reading,
this is not rejected but currently provides no additional value.

#### BmcBlobRead

This will initially not perform any function and will return success with 0
bytes.

#### BmcBlobWrite

The write command's contents will depend on the transport mechanism. This
command must not return until it has copied the data out of the mapped region
into either a staging buffer or written down to a staging file.

##### If BT

The data section of the payload is purely data.

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

If this command is called on the session for the hash image, it'll trigger a
systemd service `verify_image.service` to attempt to verify the image. Before
doing this, if the transport mechanism is not IPMI BT, it'll shut down the
mechanism used for transport preventing the host from updating anything.

When this is started, only the SessionStat command will respond and this command
will in this state, return the status of image verification.

#### BmcBlobClose

Close must be called on the firmware image before triggering verification via
commit. Once the verification is complete, you can then close the hash file.

If the `verify_image.service` returned success, closing the hash file will have
a specific behavior depending on the update. If it's UBI, it'll perform the
install. If it's legacy, it'll do nothing. The verify_image service in the
legacy case is responsible for placing the file in the correct staging position.
A BMC warm reset command will initial the firmware update process.

#### BmcBlobDelete

Aborts any update that's in progress, provided it's possible.

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
    uint16_t blob_state; /* OPEN_W | (one of the interfaces) */
    uint32_t size; /* Size in bytes so far written */
    uint8_t  metadata_len; /* 0. */
};
```

If called post-commit on the hash file session, it'll return:

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
while maintaing backwards compatibility (without simply adding a separate OEM
library to handle a new process, etc).

## Impacts

This impacts security because it can leverage the memory mapped windows. There
is not an expected performance impact, as the blob handler existing only
generates a couple extra entries during the blob enumerate command's response.

## Testing

Where possible (nearly everywhere), mockable interfaces will be used such that
the entire process has individual unit-tests that verify flags are checked, as
well as states and sequences.

### Sending an image with a bad hash

A required functional test is one whereby an image is sent down to the BMC,
however the signature is invalid for that image. The expected result is that the
verification step will return failure.

### Sending an image with a good hash

A required functional test is one whereby an image is sent down to the BMC with
a valid signature. The expected result is that the verifciation step will return
success.

