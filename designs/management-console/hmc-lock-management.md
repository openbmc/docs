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
design considerations and also the REST interface for lock management.

## Glossary

- BMC - Baseboard Management Controller
- HMC - Hardware Management Console

## Background and References

HMC is used to manage IBM Power Systems. Using a HMC, the system administrator
can manage many systems and partitions. It can also be used for monitoring and
servicing a system.

More details about IBM's proprietary Hardware management console can be found
in the following link:
<https://www.ibm.com/support/knowledgecenter/TI0003N/p8hat/p8hat_partitioningwithanhmc.htm>

## Requirements on Lock

- Every lock request issued by the HMC must be assigned a unique ID called as
  Transaction ID (used by the management console to debug).
- There must be a provision to obtain either a write lock (which is acquired
  if a resource is to be modified) or a read lock (which is acquired if the
  resource is to be read).
  - There must a provision to obtain multiple read locks on same resource.
  - Multiple write locks on same resource are prohibited.
  - Read lock on resource which is write locked must not be valid.
  - Write lock on resource which is read locked must not be valid.
- There must be a provision to obtain multi-level locks (parent and child
  locking mechanism), so that we must be able acquire lock for all/some
  resources which are under another resource.
- There must be a provision to release all the locks that are obtained by a
  particular session.
- There must be a provision to also release only specific locks based on the
  transaction ID
- There must be a provision to query to obtain list of all the locks that a
  particular session has acquired.


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

## Requirements on Multi-Level Locking

- There must be a provision to not lock all resources under current resource level.
- There must also be a provision to lock all resources under current resource level.
  - Example:
      Lock all the children under system resource, which implicitly locks the processor, memory,
    partitions and the physical I/O adapter of that particular System Resource.
- Provision must be present to Lock all resources under current resource level with
  identical type.
  - Example:
      This requirement would be used to acquire lock on all partitions under the Server
      system resource.

## Design Points Considered

- It is to be noted that the Sessions X-auth-token will be used to uniquely
  identify the HMC Sessions.
- The proposed API's would be specific for *hmcuser* and would be unauthorized for
  all the other users.But with that said ,these API's would also be accessible for
  all the Admin users as well (even when they are not HMC Sessions).
- It should also be noted that the HMC-ID(client identifier) and Session-ID are
  directly obtained by the Session header.
- Any session which is expired due to time out, would result in removing those
  session's lock entries from the lock table.
- Any session , that is logged out would also result in removing those session's
  lock entries from the lock table.

## Proposed Interface Design

The below mentioned design states the proposed REST Interface for the HMC lock
Management. The lock management consists of 3 API's.

- AcquireLock
- ReleaseLock
- GetLockList

## Introspect Lock Service

A GET request on the following API will get the supported actions on the lock service.

```bash
$ curl -k -H "X-Auth-Token: $bmc_tokens" -X GET https://<BMC_IP>/ibm/v1/HMC/LockService
```

The output of the above GET call would return the following response.

```bash

{
  "@odata.id": "/ibm/v1/HMC/LockService/",
  "@odata.type": "#LockService.v1_0_0.LockService",
  "Actions": {
    "#LockService.AcquireLock": {
      "target": "/ibm/v1/HMC/LockService/Actions/LockService.AcquireLock"
    },
    "#LockService.GetLockList": {
      "target": "/ibm/v1/HMC/LockService/Actions/LockService.GetLockList"
    },
    "#LockService.ReleaseLock": {
      "target": "/ibm/v1/HMC/LockService/Actions/LockService.ReleaseLock"
    }
  },
  "Id": "LockService",
  "Name": "LockService"
}
```

*NOTE* : There are 3 targets, that a client can fire POST request at, namely
AcquireLock, GetLockList, ReleaseLock.

### AcquireLock

This interface would facilitate the REST client to send a request to lock a particular
resource of its interest.


```bash
curl -k -H "X-Auth-Token: <token>" -X POST -H "Content-type: application/json" -d '{
  “Request" :[
                { "LockType":"Read/Write",
                  "SegmentFlags": [
                                      {"LockFlag":"<LockAll/DontLock/LockSame>","SegmentLength":<length>},
                                      {"LockFlag":"<LockAll/DontLock/LockSame>","SegmentLength":<length>}
                                  ],
                  "ResourceID": <id1/uint64_t>
                }
             ]
  }' https://<BMC_IP>/ibm/v1/HMC/LockService/Actions/LockService.AcquireLock
```

The following are the proposed inputs for the _AcquireLock_ interface.

1. LockType  - String - [Read/Write]
2. Segmentflags - List of Lockflags, Segmentlength(uint32_t)
3. ResourceID - uint64_t

#### LockType

- The LockType states if the current lock is a Read lock or a Write Lock.

#### Segmentflags

- The Segment flags are used to construct a multi level lock structure.
- Any valid Segmentflag structure contains 2 to 6 Valid segments and each segment
  contains Lockflags and Segmentlength and each segment represents the corresponding
  level information as per the resource hierarchy.
- LockFlags can be any of the three types as mentioned below :
  - *Dontlock* (Do not lock all resources under current resource level)
  - *Lockall* (Lock all resources under current resource level)
    Example :
    - This flag is used to acquire the lock on all the children under a parent resource
    - Lock all the children under system resource, which implicitly locks the processor,
      memory,partitions and the physical I/O adapter of that particular system resource.
  - *LockSameSegment* (Lock all resources under current resource level with same segment length)
    - This requirement is used to acquire the lock on all the children of same kind under a
      parent resource.
    - From the above figure, this flag would be used to acquire lock on all partitions under the
      system resource.

#### SegmentLength

  - This is a uint32_t number ranging from 1 to 4, which indicates the size (in bytes)
  of the particular segment.

#### ResourceID

- Unique resource identifier managed by HMC. BMC uses the resource ID to perform
  lock comparisons, however it doesn’t care what the actual data means.

#### Returns: Success

The Curl command for acquiring the lock return the HTTP return code *200 ok* followed
by a JSON containing the Transaction ID for the corresponding lock request, if the
request is legitimate as per the design.

```bash
{
  "TransactionID": <TransactionID>
}
```
*NOTE*: <TransactionID> is 32-bit number generated by the BMC after the success
of this API.This number gets incremented after each successful transaction, irrespective
of the client and session.

#### Returns: Failure

1. The Curl command for acquiring the lock returns  HTTP return code 409, when the
   BMC is not able to lock the corresponding resource as per the design,and conversely
   even a single lock conflict(in the request) will cause the BMC to deny all the
   locks asked in the request and BMC would return the first conflicting lock
   record to the HMC, with the corresponding HMC-ID, LockType, ResourceID, SessionID
   and TransactionID.

```bash
{
  "Record": {
    "HMCID": "hmc-id",
    "LockType": "Read/Write",
    "ResourceID": <uint64_t>,
    "SegmentFlags": [
      {
        "LockFlag": "<LockAll/DontLock/LockSame>",
        "SegmentLength": <uint32_t>
      },
      {
        "LockFlag": "<LockAll/DontLock/LockSame>",
        "SegmentLength": <uint32_t>
      }
    ],
    "SessionID": "ZkJQPC8cFI",
    "TransactionID": <uint32_t>
  }
}
```

2. The Curl command for acquiring the lock returns  HTTP return code 400, when the
   provided input JSON is not as per the Spec mentioned or a malformed JSON.

3. The Curl command for acquiring the lock returns HTTP return code 400, when the
   provided input JSON has multiple lock requests that are having conflicting
   requirements.

### Release Lock

This interface facilitates the REST client to send a request to release the lock
which was previously obtained by it.

HMC can release the locks in any of the below mentioned two ways:

- HMC can release all the locks that are obtained by the requesting session.
- HMC can release only locks that are previously obtained by providing the respective
  transaction ID’s.

*NOTE*:

- When the session gets terminated all the locks held by the session will be
  released automatically.
- Locks can be released by the same session which owns them.

```bash
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '
  { “Type”: “Session/Transaction”,
    "TransactionIDs": ["id 1", "id 2", ...]
  }' http:/{BMC_IP}/ibm/v1/HMC/LockService/Actions/LockService.ReleaseLock
```

The following are the proposed inputs for the _ReleaseLock_ interface.

1.Type: String

- This is a string parameter which could be either *Session* or *Transaction*. If
  mentioned as *Session*, the ReleaseLock API would release all the locks obtained
  by the requesting session. In this case the TransactionIDs parameter is ignored.

- If mentioned  as *Transaction*, the ReleaseLock API  would release  only certain
  set  of locks that are mentioned in the TransactionIDs parameter.

2.TransactionID - uint64_t

The above command allows the HMC to free locks it previously obtained. If the
requesting HMC does not own all the locks in the request, the entire transaction
will fail and NO locks will be freed.HMC would ask the BMC to release a particular
locked resource by sending a list of HMCID, TransactionID's pertaining to the lock
record of interest.

#### Returns : Success

In case of Success, the curl command returns HTTP return code - 200 OK.


#### Returns : Failure

1. BMC would return HTTP return code 401(unauthorized) with all the lock contents that cannot be released.The respective lock contents like the HMCID, LockType, ResourceID, SegementFlags, SessionID & TransactionID.

```bash
{
  "Record": {
    "HMCID": "hmc-id",
    "LockType": "Read/Write",
    "ResourceID": <uint64_t>,
    "SegmentFlags": [
      {
        "LockFlag": "<LockAll/DontLock/LockSame>",
        "SegmentLength": <uint32_t>
      },
      {
        "LockFlag": "<LockAll/DontLock/LockSame>",
        "SegmentLength": <uint32_t>
      }
    ],
    "SessionID": "ZkJQPC8cFI",
    "TransactionID": <uint32_t>
  }
}
```

2. BMC would return HTTP return code 400(Bad_request), in case if the requested
   transaction ID's are not valid(not present in the lock table).

### GetLockList

This interface would facilitate the REST client(HMC) to query all the locks that
are owned by a particular session.The below command ,takes the SessionID as input
and returns the list of locks which are obtained by that particular session.

```bash
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"SessionIDs”: [ "id 1", "id 2", ... ]}' http:/{BMC_IP}/ibm/v1/HMC/LockService/Actions/LockService.GetLockList
```

The following are the proposed inputs for the _GetLockList_ interface.

1. SessionIDs - String - [List of SessionID's]

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
}
```

To obtain the locks owned by a particular session, the HMC would first query for the
list of sessionsID's via GET request on the Session service schema, and then send the
sessionID/(List of SessionID's) as an input to the _GetLockList_ interface.

#### Response : Success

The Curl command for GetLockList API returns HTTP return code 200 ok, when provided
with a valid SessionID and also returns all the locks that are acquired by the mentioned
session.

```bash
{
  "Records": [{
    "HMCID": "hmc-id",
    "LockType": "Read/Write",
    "ResourceID": <uint64_t>,
    "SegmentFlags": [
      {
        "LockFlag": "<LockAll/DontLock/LockSame>",
        "SegmentLength": <uint32_t>
      },
      {
        "LockFlag": "<LockAll/DontLock/LockSame>",
        "SegmentLength": <uint32_t>
      }
    ],
    "SessionID": "ZkJQPC8cFI",
    "TransactionID": <uint32_t>
  }]
}
```

#### Response : Failure

The Curl command for GetLockList API returns HTTP return code 200 ok, when provided
with an un-valid SessionID but with an empty list of lock records.

```bash
{
  "Records": []
}
```

## Alternatives Considered

1. A procedure in which we maintain a one-to-one mapping between the HMC resources
   ID's in BMC , which would help reduce the complexity in conflict resolvement
   in lock management. But the problem would that every time HMC add's a new lock,
   the BMC code also needs to be updated which does not seem to be finer solution
   to the give problem.
2. Another thought point was that, BMC could read the MRW to get the knowledge
   of hardware resources and can lock them based on the corresponding HMC request,
   But the problem with this approach is that, the BMC is unaware of the Virtual/Logical
   resources like partitions.

## Implementation of HMC Lock

1. Implement the lock API's inside *bmcweb* repository and write a custom route(s)
   for handling the lock management.

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

## Impacts

Development would be required to implement the lock management interface API's.
Low level design is required to implement the lock management data structure and
Such low level implementation details are not included in this proposal.

Impacted Repo : bmcweb

## Testing

Testing can be done using simple gtest based unit tests for acquiring lock, releasing
the lock and the getlock status API's.

Thorough testing should be done using multiple HMCs to see if we can hit any
performance issues.
