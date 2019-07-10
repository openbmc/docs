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

- There should be a provision to obtain either a write lock (which is acquired
  if a resource is to be modified) or a read lock (which is acquired if the
  resource is to be read).
- There should a provision to obtain multiple read locks on same resource.
- Multiple write locks on same resource should not be valid.
- Read lock on reasource which is write locked should not be valid.
- Write lock on resource which is read locked should not be valid.
- There should be a provision to obtain multi-level locks (parent and child
  locking mechanism), so that we should be able acquire lock for all/some
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

- The locking mechanism should work for a minimum of 2 levels and a Maximum of 6 levels.
- There should be a provision to not lock all resources under current resource level.
- There should also be a provision to Lock all resources under current resource level.
  This flag is used to acquire the lock on all the children under a parent resource
  Example:
  - Lock all the children under CEC, which implicitly locks the processor, memory,
    partitions and the pysical I/O adapter of that particular CEC.
- Provision should be present to Lock all resources under current resource level with
  identical type.
  Example:
  - This requirement would be used to acquire lock on all partitions under the CEC.

## Design Points Considered

- It is to be noted that the Sessions X-auth-token will be used to uniquely
  identify the HMC Sessions.
- It should also be noted that the HMC-ID and Session-ID are directly obtained
  by the Session header.
- Any session which is expired due to time out, would result in removing those
  session's lock entries from the lock table.

## Proposed Interface Design

The below mentioned design states the proposed rest Interface for the HMC lock
Management.The lock management consists of 3 API's.

- AcquireLock
- ReleaseLock
- GetLockList

### Acquire Lock Interface

This Interface would facilitate the REST client to send a request to lock a particular
resource of its interest.

#### Curl API

```bash
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"data":[LockType:"write",ResourcePath:"Sample/resource/path",..] }' 
http:/{BMC_IP}/xyz/openbmc_project/HMC/locks/Acquirelock
```

#### Inputs

1. LockType  - String - [read/write]
2. ResourcePath - String(separated with '/' to include the parent/child relationship)

#### LockType

- The LockType states if the current lock is a Read lock or a Write Lock.

#### ResourcePath

The HMC would construct a string in the form of a PATH which would indicate the
parent/child relationship for acquiring the lock.The BMC will not have any
understanding on the contents in the Lock String, but it merely parses the string
and forms a data structure so that it could find the conflicts against the new
lock request.

Example:

- Request json structure for locking all the Children under a specific Partition
  which is Under CEC.

```bash
  {
     ResourcePath: "CEC-<ID/value>/Partition-<ID/value>"  
     LockType:"read"
  }
```

#### Returns: Success

- Status - [SUCCESS]

The Curl Api returns the Status as SUCCESS if it was able to acquire the lock
successfully.

#### Returns: Failure

The Curl api for acquiring the lock returns the Status as Failure, and also
returns the list of locks that are failed , with the corresponding HMCID,Session ID,
LockType and the ResourcePath.

Returns- [SessionID:"SampleString",LockType:"write",ResourcePath:"String",HMCID:"String",..]
Status - [FAILURE]

### Sample Lock Table Structure

The following figure dipicts how the BMC stores the lock records.

+--------------------------------------------------------------------------+
|                                                                          |
|                             Sample Lock Table                            |
+----------------+----------------+----------------------+-----------------+
|                |                |                      |                 |
|  Session ID    |     HMC ID     |   Resource Path      |  Lock Type      |
+--------------------------------------------------------------------------+
|                |                |                      |                 |
|  KgPKuBnmNI    |    0xYYYYYY    |  "CEC-X/Partition-Y" |     Write       |
+--------------------------------------------------------------------------+
|                |                |                      |                 |
|  MpRRHuymiy    |    0xZZZZZZ    |  "CEC-Y/Processor-X" |     Read        |
+--------------------------------------------------------------------------+
|                |                |                      |                 |
|   +-------+    |    +-------+   | +-----+      +---+   |    +-------+    |
|                |                |                      |                 |
|    +-----+     |    +--------+  | +-----+    +------+  |    +--------+   |
|                |                |                      |                 |
+----------------+----------------+----------------------+-----------------+

### Release Lock Interface

This interface facilitates the REST client to send a request to release the lock
which was previously obtained by it.

### CurlAPI

```bash
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"data": [[ResourcePath:"String"],...] }'
http:/{BMC_IP}/xyz/openbmc_project/HMC/locks/ReleaseLock
```

The following are the proposed inputs for the _ReleaseLock_ interface.

1. ResourcePath(optional) - String

The above command allows the HMC to free locks it previously obtained. If the
requesting HMC does not own all the locks in the request, the entire transaction
will fail and NO locks will be freed. HMC can issue unlocks in one of two ways.

- It can can either unlock all owned locks of the particular established session.
  (In this case the ResourcePath should be given as an EMPTY string)
  
- It can can unlock specific subset of locks(which is a list of ResourcePaths)
  that are owned by it.

#### Returns

- [SessionID:"SampleString",LockType:"write",ResourcePath:"String",..]

BMC would return all the lock contents that cannot be released in case of FAILURE.
The respective lock contents like the SessionID,LockType,HMCID and ResourcePath
are returned.

- Status - [SUCCESS/FAILED]

The Status can be either SUCCESS or FAILED.

### GetLockList

This interface would facilitate the REST client(HMC) to query all the locks that
are owned by a particular session.

#### Curl API

```bash
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"data": [SessionID:"String"],..}'
http:/{BMC_IP}/xyz/openbmc_project/HMC/locks/GetLockList
```

The following are the proposed inputs for the _GetLockList_ interface.

1. SessionID - String - [List of SessionID's]

The above command , takes the SessionID as input and returns the list of locks which
are obtained by that particular session.

To obtain the locks owned by a particular session, the HMC would first query for the
list of sessionsID's via GET request on the Session service schema, and then send the
sessionID/(List of SessionID's) as an input to the _GetLockList_ interface.

#### SessionID

- SessionID is a unique string which is generated by the webserver when ever a new
  session is established by the client.

- At anytime the session id can be obtained by traversing the Sessionservice/Session
  schema under Redfish.

Ex:

```bash
$ curl -k -H "X-Auth-Token: $bmc_token" -X GET https://${BMC_IP}/redfish/v1/SessionService/Sessions/KgPKuBnmNI
{
  "@odata.context": "/redfish/v1/$metadata#Session.Session",
  "@odata.id": "/redfish/v1/SessionService/Sessions/KgPKuBnmNI",
  "@odata.type": "#Session.v1_0_2.Session",
  "Description": "Manager User Session",
  "Id": "KgPKuBnmNI",
  "Name": "User Session",
  "UserName": "root"
```

#### Response

- Returns - [SessionID:"SampleString",LockType:"write",ResourceID:"String",..]
- Status - [SUCCESS/FAILED]

The Status can be SUCCESS or FAILED.

## Alternatives Considered

1. A procedure in which we maintain a one-to-one mapping between the HMC
   recources ID's in BMC , which would help reduce the complexity in conflict
   resolvement in lock managemnt. But the problem would be that every time HMC
   add's a new lock, the BMC code also needs to be updated which does not seem
   to be finer solution to the given problem.
2. Another thought point was that, BMC could read the MRW to get the knowledge
   of hardware resources and can lock them based on the corresponding HMC
   request, But the problem with this approach is that, the BMC is un aware of
   the Virtual/Logical resources like parttitions.

## Impacts

Development would be required to implement the lock management interface API's.
Low level design is required to implement the lock management data structure and
Such lowlevel implementation details are not included in this proposal.

## Testing

Testing can be done using simple gtest based unit tests for acquiring lock, releasing
the lock and the getlock status API's.

Thorough testing should be done using multiple HMC's to see if we can hit any
performance issues.
