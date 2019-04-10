# MCTP daemon on OpenBMC

Author: Deepak Kodihalli <dkodihal@linux.vnet.ibm.com> <dkodihal>

Primary assignee: Deepak Kodihalli

Created: 09-April-2019

## Problem Description
The proposed design for [MCTP on OpenBMC](https://github.com/openbmc/docs/blob/master/designs/mctp.md)
describes an "MCTP+applications" daemon. This document details the requirements
and design of said daemon.

## Background and References
Please read the [MCTP design document](https://github.com/openbmc/docs/blob/master/designs/mctp.md).

## Requirements

- A single instance of the daemon running on a terminus should represent all the
  network connections of the MCTP network that this terminus is part of.

- It should be possible to configure the daemon with static EIDs of all MCTP
  endpoints in the network.

- It should be possible to configure the daemon with the physical binding
  information about the various connections that exist between the endpoints in
  MCTP network.

- It should be possible to plug-in handlers for specific MCTP types (for eg
  PLDM, NVMe-MI, etc) to the daemon. This implies handler must be implemented
  as, or have a component which is, a library which can be loaded by the daemon.

- It should be possible for multiple MCTP handlers to co-exist, and any IO being
  performed by a handler shouldn't block other handlers or the daemon.

- The daemon should route MCTP RX packets to appropriate handlers, and should be
  able to TX MCTP packets as well.

## Proposed Design

### Daemon configuration

The following JSON file represents configuration information needed by the
daemon.
```
{
    "endpoints" : [{
        "id" : 8,
        "bus" : "self"
    },
    {
        "id" : 9,
        "bus" : "serial",
        "address" : "/dev/foo/bar",
        "handlers" : [{
            "type" : "pldm",
            "lib" : "libpldm.so"
         },
         {
            "type" : "nvme",
            "lib" : "libnvme.so"
         }]
    },
    {
        "id" : 10,
        "bus" : "pcievdm",
        "handlers" : [{
            "type" : "pldm",
            "lib" : "libpldm.so"
         }]
    }
}
```

After parsing such a configuration file, the daemon would do the following:

- Based on the list of physical busses, init corresponding bindings.
- Dynamically load handler libraries.
- Call a registration API that would have to be implemented by the hander.
  plug-ins. The handlers would have to provide an entry point to the code
  handling a specific MCTP type.
- Poll each of the physical busses for incoming traffic.
- Route incoming MCTP packets to the appropriate callbacks via in-process
  callbacks.

### Asynchronous IO
The handler libraries, the MCTP daemon and the physical binding implementations
would all have to perform IO. To prevent them from blocking each other, it needs
to be ensured that the IO calls are asynchronous. This will be achieved via the
Boost ASIO framework. Boost allows for dynamic registration of event sources (eg
file descriptors, sockets) to an event loop.

### Source code and relation with libmctp
The daemon will link with libmctp, although given the direction to implement it
in boost/c++, the daemon itself may not be provided by libmctp. It could instead
be housed in a separate repository, for eg openbmc/mctpd.

## Impacts
Low level design and development is required to implement parsing and processing
of the daemon configuraion file. The Boost ASIO framework needs to be employed.

PLDM, NVMe-MI and other protocol implementations that may run over MCTP would
have to implement a registration API to get plugged-in to the MCTP daemon. The
low level details of the API will be published in this document and relevant
repository README files.

## Testing
Parsing and processing the configuration file has to be unit tested. MCTP type
handlers can be mocked for this purpose. The asynchronous nature of IO performed
by the daeamon has to be unit tested as well.
