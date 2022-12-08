# IPMI Architecture

IPMI is made up of commands and channels. Commands are provided by providers and
channels are processes called bridges. Commands are received from external
sources (system admin users) via the bridges and routed to the providers'
command handlers via the main IPMI daemon.

## High-Level Overview

IPMI is all about commands and responses. Channels provide a mechanism for
transporting the data, each with a slightly different protocol and transport
layer, but ultimately, the highest level data is a raw IPMI command consisting
of a NetFn/LUN, Command, and optional data. Each response is likewise a
Completion Code and optional data. So the first step is to break apart channels
and the IPMI queue.

```
                                                     /------------------\
    /----------------------------\                   |                  |
    |      KCS/BT - Host         | <-All IPMI cmds-> |                  |
    |                            |                   |                  |
    \----------------------------/                   |   IPMI Daemon    |
                                                     |    (ipmid)       |
                                                     |                  |
   /-----------------------------\                   |                  |
   |            LAN - RMCP+      |                   |                  |
   | /--------------------------\|                   |                  |
   | |*Process the Session and  || <-All IPMI cmds-> |                  |
   | | SOL commands.            ||  except session   |                  |
   | |*Create Session Objs      ||  and SOL cmds     |                  |
   | \--------------------------/|                   |                  |
   \-----------------------------/                   \------------------/
                :                                             ^     ^
                :                                             |     |
                :                                             |     |
     /-------------------------\                              |     |
     | Active session/SOL Objs | <---------Query the session-/      |
     | - Properties            |           and SOL data via Dbus    |
     \-------------------------/                                    |
                                                                    V
                                                     ---------------------\
                                                     | D-Bus services     |
                                                     | ------------------ |
                                                     | Do work on behalf  |
                                                     | of IPMI commands   |
                                                     ---------------------/
```

The IPMI messages that get passed to and from the IPMI daemon (ipmid) are
basically the equivalent of ipmitool's "raw" commands with a little more
information about the message.

```less
Message Data:
  byte:LUN - LUN from netFn/LUN pair (0-3, as per the IPMI spec)
  byte:netFn - netFn from netFn/LUN pair (as per the IPMI spec)
  byte:cmd - IPMI command ID (as per the IPMI spec)
  array<byte>:data - optional command data (as per the IPMI spec)
  dict<string:variant>:options - optional additional meta-data
    "userId":int - IPMI user ID of caller (0 for session-less channels)
    "privilege": enum:privilege - ADMIN, USER, OPERATOR, CALLBACK;
      must be less than or equal to the privilege of the user and less
      than or equal to the max privilege of this channel
```

```less
Response Data:
  byte:CC - IPMI completion code
  array<byte>:data - optional response data
```

A channel bridge, upon receiving a new IPMI command, will extract the necessary
fields from whatever transport format (RMCP+, IPMB, KCS, etc.) For session-based
channels (RMCP+) the bridge is responsible for establishing the session with
credentials and determining the maximum privilege available for this session.
The bridge then takes the extracted command, data, and possibly user and
privilge information, and encodes them in a D-Bus method call to send to the
IPMI daemon, ipmid. The daemon will take the message, and attempt to find an
appropriate handler in its handler tables. If a handler is found, it will
attempt to extract the required parameters for the handler and pass them along.
The handler will return a tuple of response parameters, which will get packed
back into a D-Bus response message and sent back to the calling channel's
bridge. The bridge will then re-package the response into its transport protocol
and send it off.

The next part is to provide a higher-level, strongly-typed, modern C++ mechanism
for registering handlers. Each handler will specify exactly what arguments are
needed for the command and what types will be returned in the response. This
way, the ipmid queue can unpack requests and pack responses in a safe manner.
Because the handler packing and unpacking code is templated, it is written
mostly in headers.

## Details and Implementation

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
hook is ACCEPT, where the command just passes onto the execution phase. But OEMs
and providers can register hooks that would ultimately block IPMI commands from
executing, much like the IPMI 2.0 Spec's Firmware Firewall. The hook would be
passed in the context of the IPMI call and the raw content of the call and has
the opportunity to return any valid IPMI completion code. Any non-zero
completion code would prevent the command from executing and would be returned
to the caller.

The network channel bridges (netipmid), executing one process per interface,
handle session (RMCP+) and SOL commands and responses to those commands. Get/Set
SOL configuration, Get session info, close session commands can also be
requested through KCS/BT interface, ipmid daemon must also need details about
session and SOL. In order to maintain sync between ipmid and netipmid daemon,
session and SOL are exposed in D-Bus, which ipmid can query and respond to
commands issued through host interface (KCS/BT).

The next phase is parameter unpacking and validation. This is done by
compiler-generated code with variadic templates at handler registration time.
The registration function is a templated function that allows any type of
handler to be passed in so that the types of the handler can be extracted and
unpacked.

This is done in the core IPMI library (from a high level) like this:

```cpp
template <typename Handler>
class IpmiHandler
{
  public:
    explicit IpmiHandler(Handler&& handler) :
        handler_(std::forward<Handler>(handler))
    {
    }
    message::Response::ptr call(message::Request::ptr request)
    {
        message::Response::ptr response = request->makeResponse();
        // < code to deduce UnpackArgsType and ResultType from Handler >
        UnpackArgsType unpackArgs;
        ipmi::Cc unpackError = request->unpack(unpackArgs);
        if (unpackError != ipmi::ccSuccess)
        {
            response->cc = unpackError;
            return response;
        }
        ResultType result = std::apply(handler_, unpackArgs);
        response->cc = std::get<0>(result);
        auto payload = std::get<1>(result);
        response->pack(*payload);
        return response;
    }
  private:
    Handler handler_;
};
 ...
 namespace ipmi {
   constexpr NetFn netFnApp = 0x06;
 namespace app {
   constexpr Cmd cmdSetUserAccessCommand = 0x43;
 }
  class Context {
     NetFn netFn;
     Cmd cmd;
     int channel;
     int userId;
     Privilege priv;
     boost::asio::yield_context* yield; // for async operations
   };
 }
```

While some IPMI handlers would look like this:

```cpp
ipmi::RspType<> setUserAccess(
    std::shared_ptr<ipmi::Context> context,
    uint4_t channelNumber,
    bool ipmiEnable,
    bool linkAuth,
    bool callbackRestricted,
    bool changeBit,
    uint6_t userId,
    uint2_t reserved1,
    uint4_t privLimit,
    uint4_t reserved2,
    std::optional<uint4_t> userSessionLimit,
    std::optional<uint4_t> reserved3) {
  ...
  return ipmi::ResponseSuccess();
}

ipmi::RspType<uint8_t, // max user IDs
              uint6_t, // count of enabled user IDs
              uint2_t, // user ID enable status
              uint8_t, // count of fixed user IDs
              uint4_t  // user privilege for given channel
              bool,    // ipmi messaging enabled
              bool,    // link authentication enabled
              bool,    // callback access
              bool,    // reserved bit
              >
    getUserAccess(std::shared_ptr<ipmi::Context> context, uint8_t channelNumber,
                  uint8_t userId)
{
    if (<some error condition>)
    {
        return ipmi::response(ipmi::ccParmOutOfRange);
    }
    // code to get
    const auto& [usersMax, status, usersEnabled, usersFixed, access, priv] =
        getSdBus()->yield_method_call(*context->yield, ...);
    return ipmi::responseSuccess(usersMax, usersEnabled, status, usersFixed,
                                 priv, access.messaging, access.linkAuth,
                                 access.callback, false);
}

void providerInitFn()
{
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnApp,
                          ipmi::app::cmdSetUserAccessCommand,
                          ipmi::Privilege::Admin,
                          setUserAccess);
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnApp,
                          ipmi::app::cmdGetUserAccessCommand,
                          ipmi::Privilege::Operator,
                          getUserAccess);
}
```

Ipmid providers are all executed as boost::asio::coroutines. This means that
they can take advantage of any of the boost::asio async method calls in a way
that looks like synchronous code, but will execute asynchronously by yielding
control of the processor to the run loop via the yield_context object. Use the
yield_context object by passing it as a 'callback' which will then cause the
calling coroutine to yield the processor until its IO is ready, at which point
it will 'return' much like a synchronous call.

Ideally, all handlers would take advantage of the asynchronous capabilities of
ipmid via the boost::asio::yield_context. This means that the queue could have
multiple in-flight calls that are waiting on another D-Bus call to return. With
asynchronous calls, this will not block the rest of the queue's operation, The
sdbusplus::asio::connection that is available to all providers via the
getSdBus() function provides yield_method_call() which is an asynchronous D-Bus
call mechanism that 'looks' like a synchronous call. It is important that any
global data that an asynchronous handler uses is protected as if the handler is
multi-threaded. Since many of the resources used in IPMI handlers are actually
D-Bus objects, this is not likely a common issue because of the serialization
that happens via the D-Bus calls.

Using templates, it is possible to extract the return type and argument types of
the handlers and use that to unpack (and validate) the arguments from the
incoming request and then pack the result back into a vector of bytes to send
back to the caller. The deserializer will keep track of the number of bits it
has unpacked and then compare it with the total number of bits that the method
is requesting. In the example, we are assuming that the non-full-byte integers
are packed bits in the message in most-significant-bit first order (same order
the specification describes them in). Optional arguments can be used easily with
C++17's std::optional.

Some types that are supported are as follows:

- standard integer types (uint8_t, uint16_t, uint32_t, int, long, etc.)
- bool (extracts a single bit, same as uint1_t)
- multi-precision integers (uint<N>)
- std::bitset<N>
- std::optional<T> - extracts T if there are enough remaining bits/bytes
- std::array<T, N> - extracts N elements of T if possible
- std::vector<T> - extracts elements of T from the remaining bytes
- any additional types can be created by making a pack/unpack template

For partial byte types, the least-significant bits of the next full byte are
extracted first. While this is opposite of the order the IPMI specification is
written, it makes the code simple. Multi-byte fields are extracted as LSByte
first (little-endian) to match the IPMI specification.

When returning partial byte types, they are also packed into the reply in the
least-significant bit first order. As each byte fills up, the full byte gets
pushed onto the byte stream. For examples of how the packing and unpacking is
used, see the unit test cases in test/message/pack.cpp and
test/message/unpack.cpp.

As an example this is how a bitset is unpacked

```cpp
/** @brief Specialization of UnpackSingle for std::bitset<N>
 */
template <size_t N>
struct UnpackSingle<std::bitset<N>>
{
    static int op(Payload& p, std::bitset<N>& t)
    {
        static_assert(N <= (details::bitStreamSize - CHAR_BIT));
        size_t count = N;
        // acquire enough bits in the stream to fulfill the Payload
        if (p.fillBits(count))
        {
            return -1;
        }
        fixed_uint_t<details::bitStreamSize> bitmask = ((1 << count) - 1);
        t |= (p.bitStream & bitmask).convert_to<unsigned long long>();
        p.bitStream >>= count;
        p.bitCount -= count;
        return 0;
    }
};
```

If a handler needs to unpack a variable payload, it is possible to request the
Payload parameter. When the Payload parameter is present, any remaining,
unpacked bytes are available for inspection. Using the same unpacking calls that
were used to unpack the other parameters, the remaining parameters can be
manually unpacked. This is helpful for multi-function commands like Set LAN
Configuration Parameters, where the first two bytes are fixed, but the remaining
3:N bytes vary based on the selected parameter.
