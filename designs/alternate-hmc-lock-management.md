# Alternate design for HMC Lock Management

Author:
Manojkiran Eda <manojkiraneda!>

Primary assignee:
Manojkiran Eda

Other contributors:
None

Created:
2019-07-11

## Problem Description

The Hardware Management Console (HMC) is a hardware appliance that can be used
to configure and control one or more managed systems. As single HMC(multiple
sessions)/Multiple HMC's can possibly control same resource, we need some sort
of locking scheme across all the HMC's so that we can avoid collision between
multiple requests.

As BMC is the central entity across multiple HMC's, the lock management feature
should be implemented with in BMC itself. This document focuses on various
design considerations that were thought off and also the dbus interface for lock
management.

## Background and References

IBM's Hardware Management Console (HMC) is a Physical / Virtual appliance used to
manage IBM Systems including IBM System i,IBM System p, IBM System z, and IBM
Power Systems. HMC supports command line (ssh) as well as web (https) user
interface and REST API. Using an HMC, the system administrator can manage many
systems and partitions. It can also be used for monitoring and servicing a
system.

More details about IBM's proprietiery Hardware management console can be found
in below links :
[link!] <https://www.ibm.com/support/knowledgecenter/TI0003N/p8hat/p8hat_partitioningwithanhmc.htm>

## Requirements on Lock

- Every lock request asked by HMC should be uniquely assigned with a ID(used by
  the management console to debug),called as RequestID .
- There should be a provision to obtain either a write lock (which is acquired
  if a resource is to be modified) or a read lock (which is acquired if the
  resource is to be read).
  - There should a provision to obtain multiple read locks on same resource.
  - Multiple write locks on same resource should not be valid.
  - Read lock on reasource which is write locked should not be valid.
  - Write lock on resource which is read locked should not be valid.
- There should be a provision to obtain multi-level locks (parent and child
  locking mechanism), so that we should be able aquire lock for all/some
  resources which are under another resource.

## Aquire Lock Interface

Acquiring the Lock Interface :

1. HMCID - String
2. LockType  - String - [read/write]
3. LockString - String(separated with '/' to include the parent/child relationship)

### HMCID

- Field which consists of a NULL terminated ASCII string whose length shall not
  exceed 32 bytes (including the NULL) followed by any necessary NULL padding
  (0 to 3 bytes) needed to align the end of thefield on a 4-byte boundary.

### LockType

- The LockType states if the current lock is a Read lock or a Write Lock.

### LockString

This is an alternative design where, the HMC would construct a string in the form
of a PATH which would indicate the parent/child relationship for acquiring the lock.
The BMC will not have any understanding on the contents in the Lock String, but
it merely parses the string to form a tree structure so that it could find the
conflicts against the new lock request.

Example:

- Request json structure for locking all the Children under a specific Partition
  which is Under CEC.

```bash
  {
     HMCID:"0x0000xxx"
     LockString: "CEC-<value>/Partition-<value>/*"  
     LockType:"read"
  }
```
  
- Request json structure for explicitly telling not to lock all the Children 
  under Partition which is Under CEC.

```bash
    {
      HMCID:"0x0000xxx"
      LockString: "CEC-<value>/Partition-<value>/!*"
      LockType:"read"
}
```

- Request json structure for locking all the Partitions Under CEC (with same type)

```bash
    {
      HMCID:"0x0000xxx"
      LockString: "CEC-<value>/Partition-<value>/~*"
      LockType:"read"
    }

### Curl API

```bash
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"data": [HMCID:"SampleString",LockType:"write",LockString:"CEC-<value>/Partition-<value>/*",..] }' http:/{BMC_IP}/xyz/openbmc_project/HMC/locks/Aquirelock
```

#### Returns: Success

- Status - [SUCCESS]:
- Returns - [RequestID:"SampleRequestID",..]

The Curl api for aquiring the lock return the Status as SUCCESS followed by a
list of request ID's, if the request is legitimate as per the design.

#### Returns: Failure

The Curl api for aquiring the lock return the Status as Failure, and also
returns the list of locks that are failed , with the corresponding HMCID,
LockType and the ResourceID.

- [HMCID:"SampleString",LockType:"write",LockString:"String",..]
- Status - [FAILURE]

## Release Lock

The following are the proposed inputs for the _ReleaseLock_ interface.

1.RequestID - String

Example :[request id1,...]

### CurlAPI

```bash
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"data": [[RequestID,..]/All"] }' http:/{BMC_IP}/xyz/openbmc_project/HMC/locks/ReleaseLock
```

The above command allows the HMC to free locks it previously obtained. If the
requesting HMC does not own all the locks in the request, the entire transaction
will fail and NO locks will be freed. HMC can issue unlocks in one of two ways.
It can can either unlock all owned locks of the perticular established session,
or it can can unlock specific subset of locks(which is a list of HMCID,RequestID)
that are owned by it.

#### Returns

- ResourceID - [HMCID:"SampleString",LockType:"write",LockString:"String",..]

BMC would return all the lock contents that cannot be released.The respective
lock contents like the HMCID,LockType and ResourceID are returned.

- Status - [SUCCESS/FAILED]

The Status can be SUCCESS or FAILED.

## GetLockStatus

The following are the proposed inputs for the _GetLockStatus_ interface.

1. HMCID - String - [List of HMC-ID's]

### Curl API

```bash
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"data": [HMCID:"String/All"],..}' http:/{BMC_IP}/xyz/openbmc_project/HMC/locks/GetLockStatus
```

The above command , takes the HMCID as input and returns the list of locks for
that particular HMC.If the rest Query input is given as `ALL` , then the curl
API returns all the locks pertaining to all the HMCS.

### Response

- Returns - [HMCID:"SampleString",LockType:"write",ResourceID:"String",..]
- Status - [SUCCESS/FAILED]

The Status can be SUCCESS or FAILED.

## Impacts

Development would be required to implement the lock management interface API's.
Low level design is required to implement the lock management data structure.
Such lowlevel implementation details are not included in this proposal.

## Testing

Testing can be done using simple gtest based unit test for aquiring lock, releasing
the lock and the getlock status API's.

Thorough testing should be done using multiple HMC's to see if we can hit any
performance issues.
