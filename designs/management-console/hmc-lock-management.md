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
to configure and control one or more managed systems. Since multiple HMCs can
control the same system, a locking mechanism is needed.

As BMC is the central entity across multiple HMCs, the lock management feature
should be implemented within the BMC itself. This document focuses on various
design considerations and also the rest interface for lock management.

## Glossary

- BMC - Baseboard Management Controller
- HMC - Hardware Management Console

## Background and References

Hardware Management Console (HMC) is a Physical / Virtual appliance used to
manage IBM Power Systems. HMC supports command line (ssh) as well as web (https)
user interface and REST API. Using a HMC, the system administrator can manage
many systems and partitions. It can also be used for monitoring and servicing a
system.

More details about IBM's proprietary Hardware management console can be found
in the following links:
<https://www.ibm.com/support/knowledgecenter/TI0003N/p8hat/p8hat_partitioningwithanhmc.htm>

## Requirements on Lock

- Every lock request issued by the HMC should be assigned a unique RequestID
  (used by the management console to debug).
- There should be a provision to obtain either a write lock (which is acquired
  if a resource is to be modified) or a read lock (which is acquired if the
  resource is to be read).
  - There should a provision to obtain multiple read locks on same resource.
  - Multiple write locks on same resource are prohibited.
  - Read lock on resource which is write locked should not be valid.
  - Write lock on resource which is read locked should not be valid.
- There should be a provision to obtain multi-level locks (parent and child
  locking mechanism), so that we should be able acquire lock for all/some
  resources which are under another resource.

## Example of Multi level structure for Locking

```ascii
                                        +-----------+
                                        |           |
                                        | System    |  +---------> Level 1
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

- There should be a provision to not lock all resources under current resource level.
- There should also be a provision to Lock all resources under current resource level.
  This flag is used to acquire the lock on all the children under a parent resource
  Example:
  - Lock all the children under CEC, which implicitly locks the processor, memory,
    partitions and the physical I/O adapter of that particular CEC.
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
Management. The lock management consists of 3 API's.

- AcquireLock
- ReleaseLock
- GetLockList

### AcquireLock

This Interface would facilitate the REST client to send a request to lock a particular
resource of its interest.

```bash
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"data": [LockType:"<write/read>",Segmentflags["lockFlags":<Lockall/Dontlock/LockSameSegement>,"Segmentlength":"1"....],ResourceID:"String",..] }' http:/{BMC_IP}/ibm/v1/HMC/locks/Acquirelock
```

The following are the proposed inputs for the _AcquireLock_ interface.

1. LockType  - String - [read/write]
2. Segmentflags - List of Lockflags,Segmentlength(String)
3. ResourceID - String

#### LockType

- The LockType states if the current lock is a Read lock or a Write Lock.

#### Segmentflags

- The Segment flags are used to construct a multi level lock structure.
- Any valid Segmentflag structure contains 2 to 6 Valid segments.And each segment
  contains Lockflags and Segmentlength and each segment represents the
  corresponding level information.
- LockFlags can be any of the three types as mentioned below :
  - *Dontlock* (Do not lock all resources under current resource level)
  - *Lockall* (Lock all resources under current resource level)
    Example :
    - This flag is used to acquire the lock on all the children under a parent resource
    - Lock all the children under CEC, which implicitly locks the processor, memory,
      partitions and the physical I/O adapter of that particular CEC.
  - *LockSameSegment* (Lock all resources under current resource level with same segment length)
    - This requirement is used to acquire the lock on all the children of same kind under a
      parent resource.
    - From the above figure, this flag would be used to acquire lock on all partitions under the CEC.

Example : [Segmentflags["lockFlags"]:Dontlock,Segmentflags["Segmentlength"]:"1"....]

#### ResourceID

- Unique resource identifier managed by HMC. BMC uses the resource ID to perform
  lock comparisons, however it doesn’t care what the actual data means.

#### Returns: Success

The Curl api for acquiring the lock return the Status as SUCCESS followed by a
list of request ID's, if the request is legitimate as per the design.

```bash{
"data": {["RequestID1",..]},
"message": "200 OK",
"status": "ok"
}
```

#### Returns: Failure

The Curl api for acquiring the lock returns the Status as Failure when the BMC is
not able to lock the corresponding resource as per the design,and conversely even
a single lock conflict(in the request) will cause the BMC to deny all the locks
asked in the request and BMC would return the first conflicting lock record to
the HMC, with the corresponding HMCID, LockType and the ResourceID.

```bash
{
"data": {
[HMCID:"SampleString",LockType:"write",ResourceID:"String",..]},
"message": "4XX Bad Request",
"status": "Error"
}
```

### Release Lock

This interface facilitates the REST client to send a request to release the lock
which was previously obtained by it.

```bash
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"data": [[RequestID,..]"] }' http:/{BMC_IP}/ibm/v1/HMC/locks/ReleaseLock
```

The following are the proposed inputs for the _ReleaseLock_ interface.

1.RequestID - String

Example :[RequestID1, ...]

The above command allows the HMC to free locks it previously obtained. If the
requesting HMC does not own all the locks in the request, the entire transaction
will fail and NO locks will be freed.HMC would ask the BMC to release a particular
locked resource by sending a list of HMCID,RequestID's pertaining to the lock record
of interest.

#### Returns : Success

In case of Success, the curl API returns a 200 OK.

```bash
{
"data": { “description”: “Released lock”},,
"message": "200 OK",
"status": "ok"
}
```

#### Returns : Failure

BMC would return all the lock contents that cannot be released.The respective
lock contents like the HMCID,LockType and ResourceID are returned.

```bash{
"data": {
[HMCID:"SampleString",LockType:"write",ResourceID:"String",..]},
"message": "4XX/5XX",
"status": "error"
}
```

### GetLockList

This interface would facilitate the REST client(HMC) to query all the locks that
are owned by a particular session.

```bash
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"data": [SessionID],..}' http:/{BMC_IP}/ibm/v1/HMC/locks/GetLockList
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

#### Response : Success

```bash
{
"data": { - [SessionID:"SampleString",LockType:"write",ResourceID:"String",..]},,
"message": "200 OK",
"status": "ok"
}
```

#### Response : Failure

```bash
{
"data": {
"description": "No JSON object could be decoded"
},
"message": "4XX Bad Request",
"status": "error"
}
```

## Alternatives Considered

1. A procedure in which we maintain a one-to-one mapping between the HMC
   resources ID's in BMC , which would help reduce the complexity in conflict
   resolvement in lock management. But the problem would that every time HMC
   add's a new lock, the BMC code also needs to be updated which does not seem
   to be finer solution to the give problem.
2. Another thought point was that, BMC could read the MRW to get the knowledge
   of hardware resources and can lock them based on the corresponding HMC
   request, But the problem with this approach is that, the BMC is un aware of
   the Virtual/Logical resources like partitions.

## Implementation of HMC Lock

1. Implement the lock API's inside *bmcweb*(include/openbmc_dbus_rest.hpp)
   repository and write a custom route(s) for handling the lock management.

```ascii
                                              +-------------------+-----------+
                                              |            BMC    |           |
                                              |                  DBUS         |
                                              |                   |           |
                                              |                   |           |
+--------------+                              +---------------+   |           |
|              |                              |     BMCWEB    |   |           |
|              |                              | REST  + REDFISH   |           |
|              |                              |       |       |   |           |
|              +------------------------------->      |       |   |           |
|              |                              |Implement      |   |           |
|     HMC      |                              |  HMC  |       |   |           |
|              |                              |related|       |   |           |
|             <-------------------------------+  API's|       |   |           |
|              |                              |       |       |   |           |
|              |                              |       |       |   |           |
|              |                              |       |       |   |           |
+--------------+                              +-------+-------+   |           |
                                              |                   |           |
                                              |                   |           |
                                              |                   |           |
                                              +-------------------+-----------+

```

2. Implement custom routes for lock management in *bmcweb*, where the custom
   routes would merely call the respective dbus-methods defined for lock(s).The
   actual implementation for these API's would be maintained in a separate repo.
   This hmc features can be enabled/disabled using the distro enbale/disable
   feature while bitbaking the flash image.

```ascii
                                              +-------------------+-----------+
                                              |            BMC    |           |
                                              |                  DBUS         |
                                              |  +--------------^ +------+    |
                                              |  |              | |      |    |
+--------------+                              +---------------+ | | +----v----+
|              |                              |  v  BMCWEB    | | | |  HMCD   |
|              |                              | REST  + REDFISH | | |         |
|              |                              |       |       | | | |         |
|              +-------------------------------> Call |       | | | |         |
|              |                              |   to  |       | | | |Implement|
|     HMC      |                              |  DBUS |       | +-+ |   ALL   |
|              |                              | Methods       |   | |   HMC   |
|             <-------------------------------+       |       |   | | Related |
|              |                              |       |       |   | |  API's  |
|              |                              |       |       |   <-+         |
|              |                              |       |       |   | |         |
+--------------+                              +---+---+-------+   | +---------+
                                              |   |               |           |
                                              |   +--------------->           |
                                              |                   |           |
                                              +-------------------+-----------+
```

3. Define a new OEM schema for HMC management, and use the redfish model to implement
   the proposed HMC rest API's. This change will go inside bmcweb , and the HMC features
   can be disabled/enabled using a custom flag setting during bmcweb compilation.

```ascii
                                              +-------------------+-----------+
                                              |            BMC    |           |
                                              |                  DBUS         |
                                              |                   +           |
                                              |                   |           |
+--------------+                              +-----------------+ |           |
|              |                              |     BMCWEB      | |           |
|              |                              | REST  + REDFISH | |           |
|              |                              |       |         | |           |
|              +------------------------------->      |         | |           |
|              |                              |       | Implemnt| |           |
|     HMC      |                              |       | All HMC | |           |
|              |                              |       |  API's  | |           |
|             <-------------------------------+       | using an| |           |
|              |                              |       |   OEM   | |           |
|              |                              |       |         | |           |
|              |                              |       |         | |           |
+--------------+                              +-------+---------+ |           |
                                              |                   |           |
                                              |                   |           |
                                              |                   |           |
                                              +-------------------+-----------+
```

## Alternative Design Proposal

### Acquire Lock Interface

This Interface would facilitate the REST client to send a request to lock a particular
resource of its interest.

1. LockType  - String - [read/write]
2. ResourcePath - String(separated with '/' to include the parent/child relationship)

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
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"data":[LockType:"write",ResourcePath:"Sample/resource/path",..] }' 
http:/{BMC_IP}/ibm/v1/HMC/locks/Acquirelock
```

#### Returns: Success

- Status - [SUCCESS]

The Curl api for acquiring the lock return the Status as SUCCESS

#### Returns: Failure

The Curl api for acquiring the lock return the Status as Failure, and also
returns the list of locks that are failed , with the corresponding HMCID,
LockType and the ResourceID.

- [SessionID:"SampleString",LockType:"write",ResourcePath:"String",..]
- Status - [FAILURE]

### Release Lock

This interface facilitates the REST client to send a request to release
the lock which was previously obtained by it.

The following are the proposed inputs for the _ReleaseLock_ interface.

1. ResourcePath - String

### CurlAPI

```bash
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"data": [[ResourcePath:"String"],...] }' https:/{BMC_IP}/ibm/v1/HMC/locks/ReleaseLock
```

The above command allows the HMC to free locks it previously obtained. If the
requesting HMC does not own all the locks in the request, the entire transaction
will fail and NO locks will be freed. HMC can issue unlocks in one of two ways.
It can either unlock all owned locks of the particular established session,
or it can unlock specific subset of locks(which is a list of HMCID, RequestID)
that are owned by it.

#### Returns

- [SessionID:"SampleString",LockType:"write",ResourcePath:"String",..]

BMC would return all the lock contents that cannot be released.The respective
lock contents like the SessionID,LockType and ResourcePath are returned.

- Status - [SUCCESS/FAILED]

The Status can be SUCCESS or FAILED.

### GetLockList

This interface would facilitate the REST client(HMC) to query all the locks that are
owned by a particular session.

To obtain the locks owned by a particular session, the HMC would first query for the
list of sessionsID's via GET request on the Session service schema, and then send the
sessionID/(List of SessionID's) as an input to the _GetLockList_ interface.

The following are the proposed inputs for the _GetLockList_ interface.

1. SessionID - String - [List of SessionID's]

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

#### Curl API

```bash
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"data": [SessionID:"String"],..}' http:/{BMC_IP}/ibm/v1/HMC/locks/GetLockStatus
```

The above command , takes the SessionID as input and returns the list of locks for
that particular session.

#### Response

- Returns - [SessionID:"SampleString",LockType:"write",ResourceID:"String",..]
- Status - [SUCCESS/FAILED]

The Status can be SUCCESS or FAILED.

### Sample Lock Table Structure (Alternate design)

The following figure depicts how the BMC stores the lock records.

- It should also be noted that the HMC-ID and Session-ID are directly obtained
  by the Session header.
- Any session which is expired due to time out, would result in removing those
  session's lock entries from the lock table.

```ascii
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
```

## Impacts

Development would be required to implement the lock management interface API's.
Low level design is required to implement the lock management data structure and
Such lowlevel implementation details are not included in this proposal.

## Testing

Testing can be done using simple gtest based unit tests for acquiring lock, releasing
the lock and the getlock status API's.

Thorough testing should be done using multiple HMCs to see if we can hit any
performance issues.
