# **IPMI Architecture**

IPMI is a hard problem to solve. New bits bolted onto legacy stuff makes for
quite the Frankensteinian monster. If we break it apart again and look at each
piece, maybe we can sleep with fewer nightmares.


## **High Level Overview**

IPMI is all about commands and responses. Channels provide a mechanism for
transporting the data, each with a slightly different protocol and transport
layer, but ultimately, the highest level data is a raw IPMI command consisting
of a NetFn/LUN, Command, and optional data. Each response is likewise a
Completion Code, and optional data. So the first step is to break apart
channels and the IPMI queue.


```
___________          ___________________
| KCS/BT  |          |                 |
| Channel | <------> |                 |
----------/          | IPMI Daemon     |
-----------          |   (ipmid)       |
| RMCP+   |          |                 |
| Channel | <------> |                 |
----------/          ------------------/
```


The IPMI messages that get passed to and from the IPMI daemon (ipmid) are
basically the equivalent of ipmitool's "raw" commands with a little more
information about the message.

```less
Message Data:
  byte:userID - IPMI user ID of caller (0 for session-less channels)
  enum:privilege - ADMIN, USER, OPERATOR, CALLBACK; will be less than or
    equal to the privilege of the user and less than or equal to the max
    privilege of this channel
  enum:channel - what channel the request came in on (LAN0, LAN1, KCS/BT,
    IPMB0, etc.), also used to route the response back to the caller.
  integer:msgID - identifier for message to match with response
  byte:LUN - LUN from netFn/LUN pair (0-3, as per the IPMI spec)
  byte:netFn - netFn from netFn/LUN pair (as per the IPMI spec)
  byte:cmd - IPMI command ID (as per the IPMI spec)
  array<byte>:data - optional command data (as per the IPMI spec)
```

```less
Response Data:
  enum:channel - what channel the request came in on
  integer:msgID - what request this response matches
  byte:CC - IPMI completion code
  array<byte>:data - optional response data
```

The next part is to provide a higher-level, strongly-typed, modern c++
mechanism for registering handlers. Each handler will specify exactly what
arguments are needed for the command and what types will be returned in the
response. This way, the ipmid queue can unpack requests and pack responses in a
safe manner.

Instead of using a bunch of dynamically-loaded shared object files, ipmid will
simply be statically linked with all the handlers to be registered statically
linked in. The hope here would be that we can find some Yocto magic to link
things in at compile time rather than run time.


## **Details and Implementation**

For session-less channels (like BT, KCS, and IPMB), the only privilege check
will be to see that the requested privilege is less than or equal to the
channel's maximum privilege. If the channel has a session and authenticates
users, the privilege must be less than or equal to the channel's maximum
privilege and the user's maximum privilege.

Ipmid takes the LUN/netFN/Cmd tuple and looks up the corresponding handler
function. If the requested privilege is less than or equal to the required
privilege for the given registered command, the request may proceed. If any of
these checks fail, ipmid returns with _Insufficient Privilege_.

At this point, the IPMI command is run through the filter hooks. The default
hook is ACCEPT, where the command just passes onto the execution phase. But
OEMs and providers can register hooks that would ultimately block IPMI commands
from executing, much like the IPMI 2.0 Spec's Firmware Firewall. The hook would
be passed in the context of the IPMI call and the raw content of the call and
has the opportunity to return any valid IPMI completion code. Any non-zero
completion code would prevent the command from executing and would be returned
to the caller.

The next phase is parameter unpacking and validation. This is done by
compiler-generated code with variadic templates at handler registration time.
The registration function is a templated function that allows any type of
handler to be passed in so that the types of the handler can be extracted and
unpacked.

This can be done with something along these lines:

```cpp
class ipmiQueue {
  template <typename MessageHandler, typename... InputArgs>
  auto register_ipmi_handler(
      const std::vector<enum ipmiChannel>& channelList,
      uint8_t lun, uint8_t netFn, uint8_t cmd,
      MessageHandler handler) {
    ...
  }
  template <typename MessageHandler, typename... InputArgs>
  auto register_ipmi_handler_async(
      const std::vector<enum ipmiChannel>& channelList,
      uint8_t lun, uint8_t netFn, uint8_t cmd,
      MessageHandler handler) {
    ...
  }
  template <typename MessageHandler, typename... ReplyArgs>
  auto async_reply(ipmi::handlerContext&, ReplyArgs) {
    ...
  }
};
 ...
 namespace ipmi {
   constexpr uint8_t appNetFn = 6;
   class handlerContext {
     enum ipmiChannel channel;
     uint32_t msgId;
     uint8_t userId;
     enum ipmiPriv privilege;
   };
   namespace app {
     constexpr uint8_t setUserAccessCmd = 0x43;
   }
 }

std::tuple<ipmi::compCode> setUserAccess(
    ipmi::handlerContext& context,
    uint1_t changeBit, // one bit integer type
    uint1_t callbackRestricted,
    uint1_t linkAuth,
    uint1_t ipmiEnable,
    uint4_t channelNumber,
    uint2_t reserved1,
    uint6_t userId,
    uint4_t reserved2,
    uint4_t privLimit,
    std::optional<uint4_t> reserved3,
    std::optional<uint4_t> userSessionLimit) {
  ...
  return std::tuple<ipmi::compCodeNormal>;
}

ipmi::register_ipmi_handler(ipmi::lun0, ipmi::appNetFn,
                            ipmi::app::setUserAccessCmd,
                            setUserAccess);
void getUserAccess(
    ipmi::handlerContext& context,
    uint4_t reserved1,
    uint4_t channelNumber,
    uint2_t reserved2,
    uint6_t userId) {
  ...
    auto reply = std::make_shared<std::tuple<ipmi::compCode,
      uint2_t, uint6_t, uint2_t, uint6_t, uint2_t, uint6_t, uint1_t,
      uint1_t, uint4_t>>();
    async_call([&]() {
      std::get<0>(*reply) = ipmi::compCodeNormal;
      std::get<2>(*reply) = ...;
      ipmi::async_reply(context, *reply);
    });
 ...
 }

ipmi::register_ipmi_handler_async(ipmi::lun0, ipmi::appNetFn,
                            ipmi::app::setUserAccessCmd,
                            setUserAccess);
```


Ideally we would have support for asynchronous handling of IPMI calls. This
means that the queue could have multiple in-flight calls that are waiting on
another dbus call to return. With asynchronous calls, this will not block the
rest of the queue's operation, allowing for maximum throughput and minimum
delay. Synchronous calls would emit warnings if they hold up the queue for too
long. If it is possible to do, it would be nice to abstract the dbus interface
call interface so that it could put off returning the result to a function and
handle other stuff while waiting. Then if the IPMI method only had dbus calls,
it could be written in a synchronous method but still allow the rest of the
queue to behave as if it was written as an asynchronous callback.

Passing the reply tuple in as a shared pointer allows for multiple levels of
nested lamdas so that the owner is never destroyed and the lifetime of the
object is preserved. This is helpful if multiple dbus calls need to be made to
gather information. Alternately, it might only need to be generated in the last
stage when something of value is generated. Either way, the tuple is ultimately
passed into the templated async_reply function that packs the parameters back
into a vector to send back to the requester. The context is guaranteed to be
valid until either a synchronous call returns or async_reply is called.

Using templates, it is possible to extract the return type and argument types
of the handlers and use that to unpack (and validate) the arguments from the
incoming request and then pack the result back into a vector of bytes to send
back to the caller. The deserializer will keep track of the number of bits it
has unpacked and then compare it with the total number of bits that the method
is requesting. In the example, we are assuming that the non-full-byte integers
are packed bits in the message in most-significant-bit first order (same order
the specification describes them in). Optional arguments can be used easily
with c++17's std::optional (or using boost::optional for earlier c++). Actually
calling the handler with the extracted tuple of arguments is easy with c++17's
std::apply (or can be written by hand if necessary). The moral of the story
here is that we should use c++17 since it is available with yocto 2.4.

For multi-byte parameters, endianness matters, so we should define some types
that can denote that: be_int32_t be_uint32_t, le_int32_t, le_uint32_t.
Alternately, we could only specify big-endian variants because most of the IPMI
spec uses little-endian representations.

To start with, we can implement the templated registration scheme, but still
allow for a legacy registration method so that all the currently implemented
IPMI handlers can still work until they have been rewritten to use the new
mechanism. When all the current commands have been rewritten, we can remove the
legacy interface. All commands registering with the legacy interface will get
logged with a message saying that interface is deprecated.

Things that would be nice to have are as follows:



*   nested types for arguments: e.g., std::array<std::tuple<uint1_t, uint1_t,
    uint2_t, uint4_t>, 4>
*   a nested callback mechanism (the one that comes to mind is set/get lan
    parameters) where the handler is really ultimately split into subhandlers
    with different trailing parameters by examining the first few bytes. In
    this case, you read a few common bytes and then need to re-interpret the
    large trailing buffer. If we can provide the message parser to the
    handlers, then they can re-parse the big buffer using compiler-generated
    code rather than re-writing their own sub-parser.
*   c++17 (as noted above for std::apply and std::optional (and possibly other
    shiny goodness)


