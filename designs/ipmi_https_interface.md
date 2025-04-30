# IPMI HTTPS interface

Author: Lei Yu (LeiYU)

Other contributors: Zhang Jian (JianZhang3032)

Created: Apr 28, 2025

## Problem Description

IPMI lanplus is widely used in data center to support remote management of
servers. However, it has known vulnerabilities that is requires the BMC to store
the password by design. The password is encrypted and stored in BMC's persistent
storage, but it could be decrypted if file is extracted, causing data leakage
from data center.

IPMI lanplus uses udp protocol to communicate with BMC, and it has no QoS if the
traffic goes outside of the data center. Thus the packets could be dropped or
delayed in such environments, which makes IPMI communication unreliable.

In this design, a new HTTPS interface is proposed to solve the above problems.

## Background and References

IPMI lanplus protocol[1] involves RAKP, which is vulnerable in the cases below:

- CVE-2013-4786[2]: RAKP Message 2 introduces a vulnerability that allows an
  attacker to extract the HMAC and perform offline attacks to recover the
  plaintext password.
- The nature of RAKP requires the BMC to store the plaintext password or at
  least a reversible equivalent. In most cases, it's encrypted by a symmetric
  key, but the key is still stored in BMC. So an attacker who gains the rootfs
  of the BMC can recover the plaintext password by decrypting the key.

BMC network is usually isolated in datacenter to mitigate the risk of attack.
However, there are cases that the traffic goes outside of the data center, and
such vulnerability is still possible.

## Requirements

In this design, HTTPS interface is introduced to replace the lanplus protocol.
ipmitool could establish the HTTPS connection and send IPMI commands to the BMC.

### Benefits

1. It does not require the BMC to store the password anymore.
2. It uses HTTPS (and underlying tcp) protocol to communicate with BMC, which is
   more reliable.
3. It re-uses the Redfish session, so it is possible to establish the fully
   "no-password" session, with mTLS, SSO or other authentication methods.

## Proposed Design

`ipmitool` establishes a HTTPS connection to the `bmcweb`, upgrades to
websocket, sends and receives IPMI commands over the websocket. `bmcweb`
forwards the IPMI requests to `ipmid` via DBus, get the response, and send it
back to the ipmitool.

```mermaid
flowchart LR
  subgraph BMC["BMC"]
    IPMID["ipmid"]
    BMCWEB["bmcweb"]
  end
  IPMITOOL["ipmitool"]

IPMID ---|DBus| BMCWEB
BMCWEB ---|HTTPS| IPMITOOL
```

As the diagram shows, this design mainly consists of the changes to ipmitool,
bmcweb, and ipmid.

### Protocol

The `/ipmi` URI is implemented in bmcweb, that could be upgraded to a websocket.
Then the data protocol between the ipmitool and bmcweb is defined as below as C
structure.

- URI

  ```http
  wss /ipmi
  ```

- Request

  ```c
  struct ipmi_rq_header {
      uint8_t netfn:6;
      uint8_t lun:2;
      uint8_t cmd;
      uint8_t target_cmd;
      uint16_t data_len;
      uint8_t *data;
  };
  ```

- Response

  ```c
  struct ipmi_rs {
  uint8_t ccode;
  uint8_t data[IPMI_BUF_SIZE];
  };
  ```

### ipmitool

#### HTTPS interface

ipmitool has a plugin mechanism to support different IPMI interfaces. HTTPS
interface will be added as a new plugin, implementing below functions:

```c
struct ipmi_intf ipmi_https_intf = {
        .name = "https",
        .desc = "OpenBMC HTTPS interface",
        .setup = ipmi_https_setup,
        .open = ipmi_https_open,
        .close = ipmi_https_close,
        .sendrecv = ipmi_https_sendrecv,
        .target_addr = IPMI_BMC_SLAVE_ADDR,
};
```

- `ipmi_https_open`: Construct the HTTPS websocket URI including optional port,
  and use libcurl to connect to the BMC.
- `ipmi_https_close`: Cleanup the curl context.
- `ipmi_https_sendrecv`: Construct the IPMI request, send the request and
  receive the response.

The connection shall use the Redfish session token for authentication.

#### libcurl

To support the HTTPS connection and to upgrade to websocket, libcurl is needed.
However, libcurl is built without websocket support by default, and it needs to
be enabled by building with `--with-websockets`, so it is better to link libcurl
statically.

#### SOL support

SOL is special in IPMI lanplus. ipmitool not only sends the "sol activate"
command to the BMC, but also starts a loop to enter the "raw mode" to use the
SOL session.

In the HTTPS interface, only the sol activate command is supported. When
invoked, it is redirected to establish a WebSocket connection to the
`/console/default` URI, and then starts the same interactive loop as used in the
lanplus implementation.

`sol deactivate` is not supported, user will have to enter `Enter ~ .` to quit
the session.

### bmcweb

#### `/ipmi` URI

bmcweb has existing support of websocket, e.g. the `/console/default` URI is
implemented as a websocket.

In bmcweb, `include/ipmi_https.hpp` is implemented to serve the `/ipmi` URI.

```c++
inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/ipmi")
        .privileges({{"Login"}})
        .websocket()
        .onopen([&](crow::websocket::Connection& conn) {
        sessions.try_emplace(&conn);
    })
        .onclose([&](crow::websocket::Connection& conn, const std::string&) {
        sessions.erase(&conn);
    }).onmessage(onMessage);
}
```

The `onMessage()` function implements the logic:

- Parse the request
- Make DBus call to ipmid
- Send the response

#### Authentication

The authentication is fully re-used from the Redfish role. The HTTPS session is
established by Redfish, and the user role of the session becomes the user role
to the ipmid.

In the `onMessage()` function, it fetches the role and forward the info to
ipmid.

```c++
    std::string userRole = conn.getUserRole();
    int privilege = 0;
    if (userRole == "priv-admin")
    {
        privilege = 4;
    }
    else if (userRole == "priv-operator")
    {
        privilege = 3;
    }
    else if (userRole == "priv-user")
    {
        privilege = 2;
    }
```

### ipmid

In ipmid, it has existing DBus method:

```c++
auto executionEntry(boost::asio::yield_context yield, sdbusplus::message_t& m,
                    NetFn netFn, uint8_t lun, Cmd cmd, ipmi::SecureBuffer& data,
                    std::map<std::string, ipmi::Value>& options);
iface->register_method("execute", ipmi::executionEntry);
```

Where it has the `options` that could be used to pass various information.

The `options` is used for `bmcweb` to carry the HTTPS interface related
information.

- ipmiHttps: Indicate it's call from HTTPS interface
- privilege: The privilege of the user
- currentSessionId: The session id of the HTTPS interface

Then it could handle the IPMI request and response to bmcweb, the same as
net-ipmid.

## Alternatives Considered

In the HTTPS connection, it is possible to use the json format to communicate
between ipmitool and bmcweb. However, a ipmitool command could involve quite a
lot of interactions (e.g. SEL/SDR commands), and this will result in a lot of
HTTPS requests and responses, which will cause a lot of overhead. We did
implement this json protocol, the test shows the significant performance
degradation comparing to the lanplus interface.

By using the binary protocol on a websocket, we see the similar performance
compared to the lanplus interface.

## Impacts

Below repos are impacted by this design:

- ipmitool: The new feature will be implemented, roughly 400 lines of code for
  the HTTPS interface and 300 lines of code for SOL over HTTPS.
- bmcweb: The `/ipmi` URI will be implemented, and the `SOL` support is re-used.
- ipmid: Minor changes to handle the HTTPS interface related information.

### Organizational

The above repos will be modified to support the HTTPS interface:

- ipmitool is maintained at codeberg.org[3], MRs will be sent to the upstream
  repo.
- bmcweb and ipmid changes will be sent to OpenBMC gerrit for review.

## Testing

With a system and ipmitool implemented this design, user could test the feature
by `ipmitool -I https` argument.

```bash
# Regular usage
ipmitool -I https -H $bmc -P $token mc info

# Optional port
ipmitool -I https -H $bmc -p 8443 -P $token mc info

# SOL
ipmitool -I https -H $bmc -P $token sol activate

# Deactivate SOL
Enter ~.
```

[1]:
  https://www.intel.com/content/dam/www/public/us/en/documents/product-briefs/ipmi-second-gen-interface-spec-v2-rev1-1.pdf
[2]: https://nvd.nist.gov/vuln/detail/cve-2013-4786
[3]: https://codeberg.org/IPMITool/ipmitool.git
