# HMC Lock Management REST interface

Author:
Manojkiran Eda <manojkiraneda!>

Primary assignee:
Manojkiran Eda

Other contributors:
None

Created:
2019-07-11

## Problem Description

The Hardware Management Console (HMC) is a hardware appliance that can be used to configure and 
control one or more managed systems. As single HMC(multiple sessions)/Multiple HMC's can possibly
control same resource, we need some sort of locking scheme across all the HMC's so that we can 
avoid collision between multiple requests.

As BMC is the central entity across multiple HMC's, the lock management feature should be implemented 
with in BMC itself.This document focuses on various design considerations that were thought off and 
also the dbus interface for lock management.

## Background and References

Hardware Management Console (HMC) is a Physical / Virtual appliance used to manage IBM Systems including
IBM System i, IBM System p, IBM System z, and IBM Power Systems. HMC supports command line (ssh) as well
as web (https) user interfaces and REST API. Using an HMC, the system administrator can manage many systems
and partitions. It can also be used for monitoring and servicing a system.

More details about IBM's proprietiery Hardware management console can be found in below links :
[link!] <https://www.ibm.com/support/knowledgecenter/TI0003N/p8hat/p8hat_partitioningwithanhmc.htm>

## Requirements on Lock

- Every lock request asked by HMC should be uniquely assigned with a ID(used by the management console to 
  debug), called as RequestID.
- There should be a provision to obtain either a write lock (which is acquired if a resource is to be modified)
  or a read lock(which is acquired if the resource is to be read).
  - There should a provision to obtain multiple read locks on same resource.
  - Multiple write locks on same resource should not be valid.
  - Read lock on reasource which is write locked should not be valid.
  - Write lock on resource which is read locked should not be valid.
- There should be a provision to obtain multi-level locks (parent and child locking mechanism), so that we should
  be able aquire lock for all/some resources which are under another resource.

## Example of Multi level structure for Locking

```ascii
                                        +-----------+
                                        |           |
                                        |    CEC    |  +---------> Level 1
                                        |           |
                                        +-----------+
                                              |
                                              |
                                              |
              +-----------------------------------------------------------------+
              |                     |                  |                        |
              |                     |                  |                        |
     +------------------+  +----------------+  +------------------+ +----------------------+
     |                  |  |                |  |                  | |                      |
     |    Processor     |  |      Memory    |  |    Partitions    | | Physical IO Adapters | +------->  Level 2
     |                  |  |                |  |                  | |                      |
     +------------------+  +----------------+  +------------------+ +----------------------+
                                                       |
                                                       |
                                         +-------------------------------+
                                         |                               |
                                         |      Virtual IO Adapters      |  +-------->  Level 3
                                         |                               |
                                         +-------------------------------+

```

## Requirements on Multi Level Locking

- The locking mechanism should work for a minimum of 2 levels and a Maximum of 6 levels
- Do not lock all resources under current resource level
- Lock all resources under current resource level
- Lock all resources under current resource level with same segment size.

## Proposed Interface Design

The below mentioned design states the proposed rest Interface for the HMC lock Management.The management consists of threeAPI's.

## AquireLock

The following are the proposed inputs for the _AquireLock_ interface.

1. HMCID - String
2. IsPersist: true/false
3. LockType  - String - [read/write]
4. ResourceID - String

### HMCID

- Field which consists of a NULL terminated ASCII string whose length shall not exceed 32 bytes (including the NULL) followed by any necessary NULL padding (0 to 3 bytes) needed to align the end of thefield on a 4-byte boundary.

### IsPersist

- While this proposal also considers the Persistant/Non-persitant locks, the use-cases of persistant locks are still needs to be evaluated for the current BMC architecture.

### LockType

- The LockType states if the current lock is a Read lock or a Write Lock.

### ResourceID

- Unique resource identifier managed by HMC. BMC uses the resource ID to perform lock comparisons, however it doesn’t care what the actual data means.

### Curl API

```bash
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"data": [HMCID:"SampleString",LockType:"write",ResourceID:"String",..] }' http:/{BMC_IP}/xyz/openbmc_project/HMC/locks/Aquirelock
```

#### Returns: Success

- Status - [SUCCESS]:
- Returns - [RequestID:"SampleRequestID",..]

The Curl api for aquiring the lock return the Status as SUCCESS followed by a list of request ID's, if the request is legitimate as per the design.

#### Returns: Failure

The Curl api for aquiring the lock return the Status as Failure, and also returns the list of locks that are failed , with the corresponding HMCID,LockType and the ResourceID.

* [HMCID:"SampleString",LockType:"write",ResourceID:"String",..]
* Status - [FAILURE]

## Release Lock

The following are the proposed inputs for the _ReleaseLock_ interface.

1.ResourceID - String - [List of Resource ID's]

### CurlAPI

```bash
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"data": ["ResourceID/All"] }' http:/{BMC_IP}/xyz/openbmc_project/HMC/locks/ReleaseLock
```

The above command allows the HMC to free locks it previously obtained. If the requesting HMC does not own all the locks in the request, the entire transaction will fail and NO locks will be freed. HMC can issue unlocks in one of two ways. It can can either unlock all owned locks of the perticular established session, or it can can unlock specific subset of locks(which is a list of ResourceID's) that are owned by it.

#### Returns

* ResourceID - [HMCID:"SampleString",LockType:"write",ResourceID:"String",..]

BMC would return all the lock contents that cannot be released.The respective lock contents like the HMCID,LockType and ResourceID are returned.

* Status - [SUCCESS/HMC_LOCK_RELEASE_FAILED]

The Status can be SUCCESS or HMC_LOCK_RELEASE_FAILED.

## GetLockStatus

The following are the proposed inputs for the _GetLockStatus_ interface.

1. HMCID - String - [List of HMC-ID's]

### Curl API


```bash
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"data": [HMCID:"String/All"],..}' http:/{BMC_IP}/xyz/openbmc_project/HMC/locks/GetLockStatus
```

The above command , takes the HMCID as input and returns the list of locks for that particular HMC.If the rest Query input is given as `ALL` , then the curl API returns all the locks pertaining to all the HMCS.

#### Returns

- ResourceID - [HMCID:"SampleString",LockType:"write",ResourceID:"String",..]
- Status - [SUCCESS/HMC_GET_STATUS_FAILED]

The Status can be SUCCESS or HMC_GET_STATUS_FAILED.

## Design Points

- It is also to be noted that the Sessions X-auth-token will be used to uniquely identify the HMC Sessions.
- AquireLock Api will fail if resource is write locked, then further read/write lock on the same resource will not be allowed.
- Multiple read locks would be allowed on the same resource.
- Resource ID is HMC-resource ID, there is no mapping of this ID to BMC resource.
