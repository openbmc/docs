# BMC Generic Binary Blob Store via OEM IPMI Blob Transport

Author: Kun Yi (kunyi@google.com, kunyi!)

Created: 2018-12-07

## Problem Description

Server platform host OS often needs to store and/or access data coupled with the
machine itself, which in most cases has a 1:1 mapping with the motherboard.
Traditionally, this is achieved by directly accessing the FRU EEPROM, or going
through BMC using IPMI FRU commands. However, it may not always be viable or
desirable due to:

- The FRU data may be programmed by MFG and treated as read-only
- The FRU data may not be in IPMI FRU format
- The data to store may not fit in the data types defined in IPMI FRU spec
- Host may want to store multiple copies in e.g. BMC EEPROM

The BMC generic IPMI blobs binary store, or "binary store" in short, serves a
simple purpose: provide a read/write/serialize abstraction layer through IPMI
blobs transport layer to allow users to store binary data on persistent
locations accessible to the BMC.

Despite its name, the binary blob store cannot be used for everything.

- It is not a host/BMC mailbox. In general, BMC should reserve the space for
  blob store and not try to write it due to concurrency concerns. It is expected
  the only accessors are the IPMI blob store commands.
- It is not a key store. Because the data stored is accessible to IPMI users and
  is not encrypted in anyway, extra caution should be used. It is not meant for
  storing data containing any keys or secrets.
- The data to be stored should not be too large. Since each IPMI packet is
  limited in size, trying to send an overly large binary is going to take too
  long, and the overhead of the IPMI transport layer might make it not
  worthwhile.

## Background and References

Please read the IPMI Blob protocol design as primer
[here](https://github.com/openbmc/phosphor-ipmi-blobs/blob/master/README.md).

Under the hood, the binary blobs are stored as a binary
[protocol buffer](https://github.com/protocolbuffers/protobuf), or "protobuf" in
short.

## Requirements

1. BMC image should have `phosphor-ipmi-blobs` installed.
1. The host should know the specific blob base id that it intends to operate on.
   For this design the discovery operations are limited.
1. The host should only store binary data that is suitable using this transfer
   mechanism. As mentioned it is not meant for mailbox communication, key store,
   or large binary data.

## Proposed Design

This section describes how the handler `phosphor-ipmi-blobs-binarystore` defines
each handler of the IPMI Blob protocol.

### Blob ID Definition

A "blob id" is a unique string that identifies a blob. Binary Store handler may
show two kinds of blob ids: "base id" and "file id". They should only contain
ASCII alphanumerical characters and forward slashes ('/').

A "base id" begins and ends with a forward slash. It is analogous to a Unix
directory path. The binary store handler will assign each storage location a
unique base id (See next section for details).

A "file id" begins with a forward slash but must not have a slash at the end,
and is analogous to a Unix file path. Any file id with a valid base id as its
longest matching prefix is considered reserved as a binary blob in the storage
space.

For example, if `/test/` and `/test/blob0` are the initial blob ids, we know
there is one binary store location with one blob already created. To create a
new blob named `/test/blob1`, one simply calls open with the id and write/commit
with the returned session id. Opening invalid ids such as `/foo/bar` or
`/test/nested/dir` will fail.

### Platform Configuration

For the binary store handler, a configuration file provides the base id, which
file and which offset in the file to store the data. Optionally a "max_size"
param can be specified to indicate the total size of such binary storage should
not exceed the limitation. If "max_size" is specified as -1 or not specified,
the storage could grow up to what the physical media allows.

```none
base_id: /bmc_store/
sysfile_path: /sys/class/i2c-dev/i2c-1/device/1-0050/eeprom
offset: 256
max_size: 1024
```

[1] Example Configuration

### Binary Store Protobuf Definition

The data is stored as a binary protobuf containing a variable number of binary
blobs, each having a unique blob_id string with the base id as a common prefix.

```none
message BinaryBlob {
  optional string blob_id = 1;
  optional bytes data = 2;
}

message BinaryBlobStore {
  optional string blob_base_id = 1;
  repeated BinaryBlob blob = 2;
  optional uint32 max_size = 3;
}
```

Storing data as a protobuf makes the format more flexible and expandable, and
allows future modifications to the storage format.

### IPMI Blob Transfer Command Primitives

The binary store handler will implement the following primitives:

#### BmcBlobGetCount/BmcBlobEnumerate

Initially only the base id will appear when enumerating the existing blobs. Once
a valid binary has successfully been committed, its blob id will appear in the
list.

#### BmcBlobOpen

`flags` can be `READ` for read-only access or `READ|WRITE`. `blob_id` can be any
string with a matching prefix. If there is not already a valid binary stored
with supplied `blob_id`, the handler treats it as a request to create such a
blob.

The `session_id` returned should be used by the rest of the session based
commands to operate on the blob. If there is already an open session, this call
will fail.

NOTE: the newly created blob is not serialized and stored until `BmcBlobCommit`
is called.

#### BmcBlobRead

Returns bytes with the requested offset and size. If there are not enough bytes
the handler will return the bytes that are available.

Note this operation reads from memory. Make sure the stat is 'COMMITTED' which
indicates that the memory content matches the data serialized to storage.

#### BmcBlobWrite

Writes bytes to the requested offset. Return number of bytes written if success,
zero if failed.

#### BmcBlobCommit

Store the serialized BinaryBlobStore to associated system file.

#### BmcBlobClose

Mark the session as closed. Any uncommitted changes to the blob state is lost.

#### BmcBlobDelete

Delete the binary data associated with `blob_id`. Deleting the base_id (the
'directory' level) will fail harmlessly.

#### BmcBlobStat

`size` returned equals to length of the `data` bytes in the protobuf.
`blob_state` will be set with `OPEN_R`, `OPEN_W`, and/or `COMMITTED` as
appropriate.

#### BmcBlobSessionStat/BmcBlobWriteMeta

Not supported.

### Example Host Command Flow

#### No binary data yet, write data

1. `BmcBlobGetCount` followed by `BmcBlobEnumerate`. Since there is no valid
   blob with binary data stored, BMC handler only populates the `base_id` per
   platform configuration. e.g. `/bmc_store/`.
1. `BmcBlobOpen` with `blob_id = /bmc_store/blob0`, BMC honors the request and
   returns `session_id = 0`.
1. `BmcBlobWrite` multiple times to write the data into the blob.
1. `BmcBlobCommit`. BMC writes data into configured path, e.g. to EEPROM.
1. `BmcBlobClose`

#### Read existing data

1. `BmcBlobGetCount` followed by `BmcBlobEnumerate` shows `/bmc_store/` and
   `/bmc_store/blob0`.
1. `BmcBlobStat` on `/bmc_store/blob0` shows non-zero size and `COMMITTED`
   state.
1. `BmcBlobOpen` with `blob_id = /bmc_store/blob0`.
1. `BmcBlobRead` multiple times to read the data.
1. `BmcBlobClose`.

## Alternatives Considered

The first alternative considered was to store the data via IPMI FRU commands; as
mentioned in the problem description, it is not always viable.

There is a Google OEM I2C-over-IPMI driver that allows the host to read/write
I2C devices attached to the BMC. In comparison, the blob store approach proposed
offer more abstraction and is more flexible in where to store the data.

## Impacts

**_Security_**: As mentioned, the binary store transfer mechanism doesn't offer
encryption. Entities storing the data through this mechanism should be aware and
either not to transfer security-critical data, or supply their own security
mechanism on top.

**_BMC performance_**: Since the handler requires protobuf package, it may
increase BMC image size if the package wasn't previously installed.

**_Host compatibility_**: If data has been stored using IPMI blob binary store,
then the host would need a handler that understands the blob transfer semantics
to read the data.

## Testing

Where possible mockable interfaces will be used to unit test the logic of the
code.
