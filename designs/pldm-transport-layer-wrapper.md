# PLDM transport wrapper on OpenBMC

  Naveen Moses S (naveen.moses), [naveen.mosess@hcl.com](mailto:naveen.mosess@hcl.com)
  Velumani T (velu),  [velumanit@hcl](mailto:velumanit@hcl.com)

Primary assignee: Naveen Moses S

Created: October 6, 2021

## Problem Description
The PLDM stack in OpenBMC currently supports PLDM over MCTP transport layer.
There are some platforms which support other transport layers.
(ex : PLDM over NCSI based on RBT). so PLDM library needs to extend
support for other transport layers.

This design proposes common PLDM request apis under libpldm for any
suppoorted transport layer and provides abstraction based on
terminus id .

The proposed PLDM request apis accepts terminus id
and PLDM parameters for supported transport layers(MCTP,NCSI etc).
This common api calls then internally maps the transport layer
specific PLDM apis based on the given pldm_tid_t terminus id.

## Assumptions
- The additional transport layer is considered as the NCSI over RBT.

## Background and References

#### PLDM stack design :
https://github.com/openbmc/docs/blob/master/designs/pldm-stack.md

### libpldm pldm requester apis
https://github.com/openbmc/pldm/blob/master/libpldm/requester/pldm.c

### PLDM request class
https://github.com/openbmc/pldm/blob/master/requester/request.hpp

### PLDM handler class
https://github.com/openbmc/pldm/blob/master/requester/handler.hpp

## Requirements

- The PLDM request api should have common interfaces to send / receive
 PLDM packets for supported transport layers based on PLDM terminus id.

- The PLDM request api should abstract transport layer specific parameters.

- The discovery and mapping of PLDM terminus id with the underlying transport
  layer endpoints shall be done dynamically or done as static config based on the
  transport layer / device capability.

- If dynamic PLDM endpoint discovery is not supported for an OEM platform then
 the transport layer endpoint configs (such as NCSI package,
 channel,opcode etc) of respective PLDM enpoint will be stored as
 entity manager config.

- The transport layer specific PLDM request libraries (NCSI, MCTP) should
 be included in the PLDM repository.

- The initialization of the transport layer and drivers should be handled
 separately and not part of this wrapper.

## Transport layer identification

## Terminus id discovery and EID mapping

    In order to make the transport layer abstracted from the PLDM client app
the PLDM request api should accept PLDM terminus id as parameter along
with the PLDM request message. The mapping of the terminus id to
the transport layer specific endpoints (EID or RBT UID) should be done
in the discovery phase which is done by the PLDM wrapper when the pldmd
service is started.

    When  calling the PLDM requester api, the terminus id
(pldm_tid_t) is passed as one of the parameters.
 The PLDM wrapper requester api deducts mapped transport layer endpoints
from this terminus id parameter and calls corresponding transport layer
PLDM request api.

### mctp terminus id mapping
- The transport layer triggers the mctp EID discovery call
    and gathers the available MCTP endpoints.

- Based on the each EID is polled for the PLDM device terminus id
     and a one to one mapping is created for the terminus id and eid.

- Incase if the mctp does not have terminus id supported then
    a virtual terminus id is created and mapped to the device with the eid.

### NCSI terminus id mapping
- Terminus id mapping for NCSI transport layer terminus id is done by
    mapping to ncsi parameters such as package id, channel id and interface.

- The NCSI parametrs are read from the entity manager config and
      a virtual terminus id is set for each individual ncsi device parameter.

## PLDM  transport wrapper design

- In order to support multiple transport layers, the transport wrapper api should
provide request api with terminus id as parameter.

- The new PLDM request apis accepts the terminus id and PLDM parameters.

- It should provide api to discovery of transport layer based device
endpoints(EID or RBT UID).

- If the PLDM is supported assigning terminus id to supported PLDM endpoints
create a map of terminus id with for corresponding endpoint.

- If set Terminus id is not supported then the PLDM wrapper can create a
virtual terminus id and map it with the transport layer
specific endpoints(EID or RBT UID).

- The PLDM wrapper requester should have linkage to the ncsi based pldm
   and mctp based PLDM apis.


```

discoverTIDs(vector<pldm_tid_t> tids)

pldm_requester_rc_t pldm_send(pldm_tid_t tid,
			      const uint8_t *pldm_req_msg, size_t req_msg_len)

pldm_requester_rc_t pldm_send_recv(pldm_tid_t tid,
				   const uint8_t *pldm_req_msg,
				   size_t req_msg_len, uint8_t **pldm_resp_msg,
				   size_t *resp_msg_len)

pldm_requester_rc_t pldm_recv(pldm_tid_t tid, uint8_t instance_id,
			      uint8_t **pldm_resp_msg, size_t *resp_msg_len)

pldm_requester_rc_t pldm_recv_any(pldm_tid_t tid,
				  uint8_t **pldm_resp_msg, size_t *resp_msg_len)
```

## Proposed PLDMD requester and handler changes

 PLDMD requester code consists of the following two classes
 1. request - calls lippldm api to send PLDM request
 2. handler - creates object of request class with PLDM request msg and
  initiates  PLDM request when **registerRequest** member method is called.

 Currently the pldmd request and handler classes are also has mctp association.

 The PLDM requester exposes registerRequest() method
 to send and receive pldmd messages asynchronously which currently accepts mctp parameters.

```
    int registerRequest(mctp_eid_t eid, uint8_t instanceId, uint8_t type,
                        uint8_t command, pldm::Request&& requestMsg,
                        ResponseHandler&& responseHandler)
```

 This api shall be modified to accept terminus id and PLDM request message as
 parameters.

```
    int registerRequest(pldm_tid_t tid, uint8_t instanceId, uint8_t type,
                        uint8_t command, pldm::Request&& requestMsg,
                        ResponseHandler&& responseHandler)

```
 The registerrequest internally calls the PLDM transport wrapper with the
 provided terminus id and PLDM request message.

 The PLDM wrapper calls the corresponding TID - endpoint id map to
 and calls the transport layer specific call with the mapped endpoint parameters.

## Proposed PLDM message transaction process flow

- when the PLDM service is called , the PLDM wrapper's discoverTIDs() api
is called so that the transport layer specific endpoints(EID or RBT UID) are
 discovered and mapped to the TID or virtual TID.

- The PLDM client packs the PLDM message using libpldm's encode apis.

- The PLDM client apps which calls the PLDMrequester with terminus id
 and PLDM request as parameter using **registerRequest** api

- The PLDM requester calls the PLDM wrapper api with
 terminus id and PLDM request message as parameters.

- The PLDM wrapper internally checks the TID and endpoint id
map and calls the respective transport layer PLDM request api

 - When the PLDM message is received the callback function
 provided in the **registerRequest** api is called and the
 PLDM client app process the received PLDM message.

## Impacts
The current apps using  mctp based socket and EID will need to use the
terminus ID and PLDM request message as parameter.
## Testing
MCTP transport - existing features can be validated using supported platforms
NCSI RBT - Testing can be done on the supported yosemitev2 platform.

