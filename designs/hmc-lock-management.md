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

Hardware Management Console (HMC) is a Physical / Virtual appliance used to
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

- The locking mechanism should work for a minimum of 2 levels and a Maximum of
  6 levels
- There should be a provision to not lock all resources under current resource
  level
- There should also be a provision to Lock all resources under current resource
  level
- Provision should be present to Lock all resources under current resource level
  with same segment size.

## Proposed Interface Design

The below mentioned design states the proposed rest Interface for the HMC lock
Management.The lock management consists of 3 API's.

- AquireLock
- ReleaseLock
- GetLockStatus

## AquireLock

The following are the proposed inputs for the _AquireLock_ interface.

1. HMCID - String
2. IsPersist: true/false
3. LockType  - String - [read/write]
4. ResourceID - String
5. Segmentflags - List of Lockflags,Segmentlength(String)

### HMCID

- Field which consists of a NULL terminated ASCII string whose length shall not
  exceed 32 bytes (including the NULL) followed by any necessary NULL padding
  (0 to 3 bytes) needed to align the end of thefield on a 4-byte boundary.

### IsPersist

- While this proposal also considers the Persistant/Non-persitant locks, the
  use-cases of persistant locks are still needs to be evaluated for the current
  BMC architecture.

### LockType

- The LockType states if the current lock is a Read lock or a Write Lock.

### ResourceID

- Unique resource identifier managed by HMC. BMC uses the resource ID to perform
  lock comparisons, however it doesn’t care what the actual data means.

### Segmentflags

- The Segment falgs are used to contruct a multi level lock structure.
- Any valid Segmentflag structure contains 2 to 6 Valid segments.And each
  segment contains Lockflags and Segmentlength and each segment represents the
  corresponding level information.
- LockFlags can be any of the three types as mentioned below :
      - Dontlock (Do not lock all resources under current resource level)
      - Lockall (Lock all resources under current resource level)
	- Example :
	  - This flag is used to acquire the lock on all the children under a parent resource
	  - Lock all the children under CEC, which implicitly locks the processor, memory,
            partitions and the pysical I/O adapter of that particular CEC.
      - LockSameSegment (Lock all resources under current resource level with same segment length)
	  - This requirement is used to acquire the lock on all the children of same kind under a
            parent resource.
	  - From the above figure, this flag would be used to acquire lock on all partitions under the CEC.
- SegmentLength can be String
  
Example : [Segmentflags["lockFlags"]:Dontlock,Segmentflags["Segmentlength"]:"1"....]

### Curl API

```bash
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"data": [HMCID:"SampleString",LockType:"write",ResourceID:"String",..] }' http:/{BMC_IP}/xyz/openbmc_project/HMC/locks/Aquirelock
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

- [HMCID:"SampleString",LockType:"write",ResourceID:"String",..]
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

- ResourceID - [HMCID:"SampleString",LockType:"write",ResourceID:"String",..]

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

#### Returns

- Returns - [HMCID:"SampleString",LockType:"write",ResourceID:"String",..]
- Status - [SUCCESS/FAILED]

The Status can be SUCCESS or FAILED.

## Alternatives Considered

1. A procedure in which we maintain a one-to-one mapping between the HMC
   recources ID's in BMC , which would help reduce the complexity in conflict
   resolvement in lock managemnt. But the problem would that every time HMC
   add's a new lock, the BMC code also needs to be updated which does not seem
   to be finer solution to the give problem.
2. Another thought point was that, BMC could read the MRW to get the knowledge
   of hardware resources and can lock them based on the corresponding HMC
   request, But the problem with this approach is that, the BMC is un aware of
   the Virtual/Logical resources like parttitions.
3. Another approach was that, the level information for the lock structure can
   be embedded in the form of the rest URI which can be used for locking, but we
   should have the corresponding dbus mapping for all the resources.

## Alternative Design Proposal

### Aquire Lock Interface

Acquiring the Lock Interface :

1. SessionID - String
2. LockType  - String - [read/write]
3. ResourcePath - String(separated with '/' to include the parent/child relationship)

#### SessionID

- SessionID is a unique string which is generated by the webserver when ever a new
  session is established by the client.

- At anytime the session id can be obtained by traversing the Sessionservice/Session
  schema under Redfish.

Ex:

```bash
[whoami@dhcp-9-193-95-41 SW472269]$ curl -k -H "X-Auth-Token: $bmc_token" -X GET https://${BMC_IP}/redfish/v1/SessionService/Sessions/KgPKuBnmNI
{
  "@odata.context": "/redfish/v1/$metadata#Session.Session",
  "@odata.id": "/redfish/v1/SessionService/Sessions/KgPKuBnmNI",
  "@odata.type": "#Session.v1_0_2.Session",
  "Description": "Manager User Session",
  "Id": "KgPKuBnmNI",
  "Name": "User Session",
  "UserName": "root"
```

#### LockType

- The LockType states if the current lock is a Read lock or a Write Lock.

#### ResourcePath

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
     ResourcePath: "CEC-<ID/value>/Partition-<ID/value>"  
     LockType:"read"
  }
```

#### Curl API

```bash
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"data": [SessionID:"SampleString",LockType:"write",ResourcePath:"Sample/resource/path",..] }' http:/{BMC_IP}/xyz/openbmc_project/HMC/locks/Aquirelock
```

#### Returns: Success

- Status - [SUCCESS]

The Curl api for aquiring the lock return the Status as SUCCESS

#### Returns: Failure

The Curl api for aquiring the lock return the Status as Failure, and also
returns the list of locks that are failed , with the corresponding HMCID,
LockType and the ResourceID.

- [SessionID:"SampleString",LockType:"write",ResourcePath:"String",..]
- Status - [FAILURE]

### Release Lock

The following are the proposed inputs for the _ReleaseLock_ interface.

1. SessionID - String
2. ResourcePath - String

### CurlAPI

```bash
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"data": [[SessionID:"String",ResourcePath:"String"],...] }' http:/{BMC_IP}/xyz/openbmc_project/HMC/locks/ReleaseLock
```

The above command allows the HMC to free locks it previously obtained. If the
requesting HMC does not own all the locks in the request, the entire transaction
will fail and NO locks will be freed. HMC can issue unlocks in one of two ways.
It can can either unlock all owned locks of the particular established session,
or it can can unlock specific subset of locks(which is a list of HMCID,RequestID)
that are owned by it.

#### Returns

- [SessionID:"SampleString",LockType:"write",ResourcePath:"String",..]

BMC would return all the lock contents that cannot be released.The respective
lock contents like the SessionID,LockType and ResourcePath are returned.

- Status - [SUCCESS/FAILED]

The Status can be SUCCESS or FAILED.

### GetLockStatus

The following are the proposed inputs for the _GetLockStatus_ interface.

1. SessionID - String - [List of SessionID's]

#### Curl API

```bash
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"data": [SessionID:"String"],..}' http:/{BMC_IP}/xyz/openbmc_project/HMC/locks/GetLockStatus
```

The above command , takes the SessionID as input and returns the list of locks for
that particular session.

#### Response

- Returns - [SessionID:"SampleString",LockType:"write",ResourceID:"String",..]
- Status - [SUCCESS/FAILED]

The Status can be SUCCESS or FAILED.

## Design Points Considered

- It is also to be noted that the Sessions X-auth-token will be used to uniquely
  identify the HMC Sessions.
- Resource ID is HMC-resource ID, there is no mapping of this ID to BMC resource.

## Impacts

Development would be required to implement the lock management interface API's.
Low level design is required to implement the lock management data structure and
Such lowlevel implementation details are not included in this proposal.

## Testing

Testing can be done using simple gtest based unit test for aquiring lock, releasing
the lock and the getlock status API's.

Thorough testing should be done using multiple HMC's to see if we can hit any
performance issues.

