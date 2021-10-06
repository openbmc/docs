# PLDM transport wrapper on OpenBMC

  Naveen Moses S (naveen.moses), [naveen.mosess@hcl.com](mailto:naveen.mosess@hcl.com)
  Velumani T (velu),  [velumanit@hcl](mailto:velumanit@hcl.com)

Primary assignee: Naveen Moses S

Created: October 6, 2021

## Problem Description
The PLDM stack in OpenBMC currently supports pldm over MCTP transport layer.
There are some platforms which support other transport layers.
(ex : PLDM over NCSI based on RBT). so PLDM library needs to extend
support for other transport layers.

This design proposes common pldm request apis under libpldm for any
suppoorted transport layer and provides abstraction for the transport
layer specific parameters.
The commoon pldm request apis accepts abstracted transport
parameters for supported transport layers(MCTP and NCSI).
This common api calls then internally maps the transport layer
specific pldm apis based on the given pldm_tid_t parameter.

## Assumptions
- The additional transport layer is considered as the NCSI over RBT.

## Background and References

#### pldm stack design :
https://github.com/openbmc/docs/blob/master/designs/pldm-stack.md

### libpldm pldm requester apis
https://github.com/openbmc/pldm/blob/master/libpldm/requester/pldm.c

### pldm request class
https://github.com/openbmc/pldm/blob/master/requester/request.hpp

### pldm handler class
https://github.com/openbmc/pldm/blob/master/requester/handler.hpp

## Requirements

- The transport layer configs (such as NCSI package,channel,opcode etc)
 of respective FRU device will be stored as entity manager config.

- The libpldm request api should have common interfaces to send / receive
 PLDM packets for supported transport layers.

- the pldm request api should abstract transport layer specific parameters.

- The transport layer specific pldm request libraries (NCSI, MCTP) should
 be included in the PLDM repository.

- Initialization of the transport layer and drivers should be handled
 separatively and not part of this wrapper.

## Transport layer identification

- The default transport layer is considered as mctp. In case the platform
supported transport layer is different than mctp then the
transport layer id(pldm_tid_t) along with other transport layer configs
are stored part of the entity manager.

```
ex : NCSI transport layer config in entity manager
  pldm_tid_t tid
  ncsi package id
  ncsi channel id
```
- while  calling the pldm requester api call this transport layer
 capabilities(tid_t) are passed as one of the parameters.
 the pldm requester api deducts the transport layer from this capability
 parameter and calls corresponding transport layer pldm request api.

## Changes in libpldm request apis

the current libpldm has all the pldm request / receive apis with a MCTP
socket association.

In order to support multiple transport layers, the libpldm apis should be adapted
to accept abstracted socket type and transport layer parameters as argument.

### Existing libpldm apis

The following are the libpldm request apis which
are by default accepting mctp socket and mctp eid

```
int pldm_open()

pldm_requester_rc_t pldm_send(mctp_eid_t eid, int mctp_fd,
			      const uint8_t *pldm_req_msg, size_t req_msg_len)

pldm_requester_rc_t pldm_send_recv(mctp_eid_t eid, int mctp_fd,
				   const uint8_t *pldm_req_msg,
				   size_t req_msg_len, uint8_t **pldm_resp_msg,
				   size_t *resp_msg_len)

pldm_requester_rc_t pldm_recv(mctp_eid_t eid, int mctp_fd, uint8_t instance_id,
			      uint8_t **pldm_resp_msg, size_t *resp_msg_len)

pldm_requester_rc_t pldm_recv_any(mctp_eid_t eid, int mctp_fd,
				  uint8_t **pldm_resp_msg, size_t *resp_msg_len)
```

### new generic transport parameters

The following generic transport layer parameters are introduced
so that the  common pldm request apis can accept any supported
transport layer parameters.

This newly introduces parameters provides abstraction for the transport layer
related parameters.

 **1. pldm_tid_t tid** - pldm transport layer id enum type mctp=0, ncsi=1.

 **2. std::variant<mctpSocket,ncsiSocket> transportSocket**

  This variant holds different socket types specific to transport layer.

   *mctpSocket* - this is a int based socket fd.

   *ncsiSocket*  - ncsi socket is netlink based nl_sock type.

 **3. std::variant<mctpConfig,ncsiConfig>  transportPacketConfig**

  mctpConfig - this is a struct consists of mctp parameters
     *mctp_eid_t eid* - mctp endpoint id

  ncsiConfig - this is a struct consists of ncsi related parameters
     *package_id* - ncsi package id
     *channel_id* - ncsi channel id

### proposed libpldm apis with generic transport parameters

- The new pldm request apis accepts the generic transport parameters.

- The tid is read first to identify the transport layer.

- based on the transport layer id the specific transport layer config is
  extracted from the transportSocket variant and corresponding transport layer
  request apis are mapped accordingly.
  tid = 0 - mctp based pldm request api is called
  tid = 1 - ncsi based pldm request api is called

 - The libpldm requester should have linkage to the ncsi based pldm
   and mctp based pldm apis.

```

std::variant<mctpSocket,ncsiSocket> transportSocket pldm_open(pldm_tid_t tid)

pldm_requester_rc_t pldm_send(pldm_tid_t tid,
           std::variant<mctpSocket,ncsiSocket> transportSocket,
			      const uint8_t *pldm_req_msg, size_t req_msg_len)

pldm_requester_rc_t pldm_send_recv(pldm_tid_t tid,
          std::variant<mctpSocket,ncsiSocket> transportSocket,
				   const uint8_t *pldm_req_msg,
				   size_t req_msg_len, uint8_t **pldm_resp_msg,
				   size_t *resp_msg_len)

pldm_requester_rc_t pldm_recv(pldm_tid_t tid,
            std::variant<mctpSocket,ncsiSocket> transportSocketd,
            uint8_t instance_id,
			      uint8_t **pldm_resp_msg, size_t *resp_msg_len)

pldm_requester_rc_t pldm_recv_any(pldm_tid_t tid,
           std::variant<mctpSocket,ncsiSocket> transportSocket,
				   uint8_t **pldm_resp_msg, size_t *resp_msg_len)
```


## PLDMD requester changes

 PLDMD requester code consists of the following two classes
 1. request - calls lippldm api to send pldm request
 2. handler - creates object of request class with pldm request msg and
  initiates  pldm request when registerRequest member method is called.

 Currently the pldmd request and handler classes are also has mctp association.
 similar to the libldm requester changes with abstracted transport layer changes,
 the pldmd request and handler classes should be also adapted to accept
 abstracted transport layer parameters such as tid_t,
 std::variant<mctpSocket,ncsiSocket> transportSocket
 and std::variant<mctpConfig,ncsiConfig>  transportPacketConfig.


### existing request class
```
      /** @brief Constructor
     *
     *  @param[in] fd - fd of the MCTP communication socket
     *  @param[in] eid - endpoint ID of the remote MCTP endpoint
     *  @param[in] event - reference to PLDM daemon's main event loop
     *  @param[in] requestMsg - PLDM request message
     *  @param[in] numRetries - number of request retries
     *  @param[in] timeout - time to wait between each retry in milliseconds
     */
    explicit Request(int fd, mctp_eid_t eid, sdeventplus::Event& event,
                     pldm::Request&& requestMsg, uint8_t numRetries,
                     std::chrono::milliseconds timeout) :
        RequestRetryTimer(event, numRetries, timeout),
        fd(fd), eid(eid), requestMsg(std::move(requestMsg))
    {}

```

### proposed common transport request class
```
      /** @brief Constructor
     *  @param[in] tid_t tid - transport layer id
     *  @param[in] std::variant<mctpSocket,ncsiSocket> transportSocket
          - common communication socket
     *  @param[in] std::variant<mctpConfig,ncsiConfig>  transportPacketConfig
           - common transport config
     *  @param[in] event - reference to PLDM daemon's main event loop
     *  @param[in] requestMsg - PLDM request message
     *  @param[in] numRetries - number of request retries
     *  @param[in] timeout - time to wait between each retry in milliseconds
     */
    explicit Request(tid_t tid,
                      std::variant<mctpSocket,ncsiSocket> transportSocket
                     std::variant<mctpConfig,ncsiConfig>  transportPacketConfig
                     , sdeventplus::Event& event,
                     pldm::Request&& requestMsg, uint8_t numRetries,
                     std::chrono::milliseconds timeout) :
        RequestRetryTimer(event, numRetries, timeout),
        fd(fd), eid(eid), requestMsg(std::move(requestMsg))
    {}

```
### existing handler class

```
    /** @brief Constructor
     *
     *  @param[in] fd - fd of MCTP communications socket
     *  @param[in] event - reference to PLDM daemon's main event loop
     *  @param[in] requester - reference to Requester object
     *  @param[in] verbose - verbose tracing flag
     *  @param[in] instanceIdExpiryInterval - instance ID expiration interval
     *  @param[in] numRetries - number of request retries
     *  @param[in] responseTimeOut - time to wait between each retry
     */
    explicit Handler(
        int fd, sdeventplus::Event& event, pldm::dbus_api::Requester& requester,
        bool verbose,
        std::chrono::seconds instanceIdExpiryInterval =
            std::chrono::seconds(INSTANCE_ID_EXPIRATION_INTERVAL),
        uint8_t numRetries = static_cast<uint8_t>(NUMBER_OF_REQUEST_RETRIES),
        std::chrono::milliseconds responseTimeOut =
            std::chrono::milliseconds(RESPONSE_TIME_OUT)) :
        fd(fd),
        event(event), requester(requester), verbose(verbose),
        instanceIdExpiryInterval(instanceIdExpiryInterval),
        numRetries(numRetries), responseTimeOut(responseTimeOut)
    {}
```

### proposed common trasport Handler class
```
        /** @brief Constructor
     *
     *  @param[in] tid_t tid - transport layer id
     *  @param[in] std::variant<mctpSocket,ncsiSocket> transportSocket
          - common communication socket
     *  @param[in] event - reference to PLDM daemon's main event loop
     *  @param[in] requester - reference to Requester object
     *  @param[in] verbose - verbose tracing flag
     *  @param[in] instanceIdExpiryInterval - instance ID expiration interval
     *  @param[in] numRetries - number of request retries
     *  @param[in] responseTimeOut - time to wait between each retry
     */
    explicit Handler(
        tid_t tid, sdeventplus::Event& event, pldm::dbus_api::Requester& requester,
        bool verbose,
        std::chrono::seconds instanceIdExpiryInterval =
            std::chrono::seconds(INSTANCE_ID_EXPIRATION_INTERVAL),
        uint8_t numRetries = static_cast<uint8_t>(NUMBER_OF_REQUEST_RETRIES),
        std::chrono::milliseconds responseTimeOut =
            std::chrono::milliseconds(RESPONSE_TIME_OUT)) :
        sock(std::variant<mctpSocket,ncsiSocket> transportSocket),
        event(event), requester(requester), verbose(verbose),
        instanceIdExpiryInterval(instanceIdExpiryInterval),
        numRetries(numRetries), responseTimeOut(responseTimeOut)
    {}

```
## Impacts
The current apps using  mctp based socket and config will need to use the
common communication socket type and initialize the
common communication socket with mctp socket.

## Testing
MCTP transport - existing features can be validated using supported platforms
NCSI RBT - Testing can be done on the supported yosemitev2 platform.

