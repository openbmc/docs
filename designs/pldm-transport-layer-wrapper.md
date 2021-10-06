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

This design proposes a simple wrapper which enables pldb requests based on
the FRU device supported transport layer(NCSI or MCTP). So that PLDM request
calls from other processes are done via abstracted common pldm send/receive
requests and the wrapper routes the respective pldm api
call based on the detected transport layer for the FRU.

## Assumptions
- The transport layer is considered as the NCSI layer.
- The FRU may or may not support PDR.

## Background and References

#### pldm stack design :
https://github.com/openbmc/docs/blob/master/designs/pldm-stack.md

## Requirements

- The FRU device sensor configurations should be part of the BMC PDR or
  entity manager config.

- The transport layer configs(such as NCSI package,channel,opcode etc)
of each FRU device identified and stored which will be used by the wrapper.

- The wrapper should have common interfaces to send / receive PLDM packets.

- The transport layer libraries (NCSI MCTP) should be included in the PLDM repository.

- Initialization of the transport layer and drivers should be handled
 separatively and not part of this wrapper.

## Transport layer identification

### a) FRU device NCSI capability as entity manager config

- If the FRU does not support PDR, then pldm sensor config is stored and
read from the entity manager.

- Along with the FRU device specific pldm sensor configuration,
if the transport layer is not MCTP then the sensors corresponding
transport layer related config( NCSI) should be part of the pldm sensor config.

- while reading the sensor config the corresponding  FRU Device transport layer
config are also read.

- while  calling the pldm wrapper api call this transport layer
 capabilities are passed as one of the parameters.
 the wrapper deducts the transport layer from this capability
 parameter and calls corresponding transport layer pldm request api.

### b) Separarate process to identify the FRU devices with NCSI capability.

  - To identify which of the FRU supports NCSI interface a separate transport
  layer identifier daemon is started and monitors for supported FRU devices based
   on NCSI layer and then strores their parameters as dbus interfaces which is later
   used by the wrapper.

  - The PDR config for the FRU device is generated and stored in pldm repo during
  build process. This pdr config is available as bmc pdr config without requesting
  from the PLDM FRU itself.

    ##### The PLDM sensor is initialized and configured as below :
    - The NCSI parameters for the specific FRU (such as NCSI package,channel,opcode etc)
      is provided by the NCSI identification daemon.
    - the sensor capabilities for the corresponding  FRU is provided by the BMC PDR config.


## Proposed wrapper design


- The wrapper library is linked with the supported transport layer libraries(libpldm and ncsi-util).

- The wrapper  has common api for sending and receiving pldm packets which
internaly calls the transport layer specific library calls(libldm for MCTP and NCSI util for NCSI).

- The application which uses the wrapper should read the transport layer config
 for the specific FRU first from entity manager or from the transport layer
 identifier daemon. while calling the wrapper wit pldm request api,
 the transport layer config is passed as one of the parameter
 along with the pldm related parameters.

- Based on the transport layer config  parameter the corresponding transport layer
pldm request api is called (MCTP or NCSI).

##### Proposed wrapper design - flow diagram

```
+-------------------+     +----------------+      +---------------+    +----------------+
|                   |     |                |      |               |    |                |
| bmc requester /   |     |   transport    |      |     PLDM      |    |   transport    |
|                   |     |    layer       |      |    wrapper    |    |   layer api    |
|   client app      |     |   identifier   |      |               |    |                |
|                   |     |                |      |               |    |( NCSI or MCTP) |
+--------+----------+     +-------+--------+      +-------+-------+    +--------+-------+
         |                        |                       |                     |
         |                        |                       |                     |
         |  get transport layer   |                       |                     |
         +----------------------->|                       |                     |
         |    for FRU device      |                       |                     |
         |                        |                       |                     |
         |                        |                       |                     |
         |    return  conf        |                       |                     |
         |<-----------------------+                       |                     |
         |                        |  pldm_send(req,conf)  |                     |
         +------------------------+---------------------->|                     |
         |                        |                       |                     |
         |                        |                       |    pldm api based   |
         |                        |                       +-------------------->|
         |                        |                       |    on transport     |
         |                        |                       |      layer config   |
         |                        |                       |                     |
         |                        |                       |                     |
         |                        |                       |                     |
         |                        |                       |    pldm response    |
         |                        |                       <---------------------+
         |                        |                       |                     |
         |                        |                       |                     |
         |                        |  pldm response        |                     |
         <------------------------+-----------------------+                     |
         |                        |                       |                     |
         |                        |                       |                     |
         |                        |                       |                     |
         v                        v                       v                     v
```
## Alternative to the proposed wrapper design

### PLDM requester as wrapper
- Instead of the speparate wrapper, use the pldmd requester to
send / receive pldm messages for FRU device's supported transport layer.

- The requester part of the pldmd code should be extended to support
the wrapper logic. It should support additional parameter for
transport layer config.

- The requester should call the corresponding transport layer api(MCTP or NCSI)
 based on the transport layer config parameter.


## Impacts
The applications which are using mctp specific pldm request apis
should have to use the wrapper's common api.

## Testing
Testing can be done on the platforms with respective transport layers.

