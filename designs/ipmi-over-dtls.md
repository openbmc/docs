# IPMI over DTLS

Author: Vernon Mauery <vernon.mauery@linux.intel.com>

## Problem Description
RMCP+ is showing its age, with vulnerabilities at every corner. IPMI is still a
very popular remote administration protocol, even though Redfish is available.
The purpose of this is not to debate the merits or issues with Redfish or IPMI,
but offer a possible solution to the IPMI network protocol (RMCP+) issues. In
the end, RMCP and RMCP+ need to be replaced by something secure. Given that the
IPMI standards group does not plan on releasing a new version of the IPMI
specification, this is an opportunity for the BMC community to come together to
find a solution that can save themselves. RMCP+ is a big black eye on BMCs
faces everywhere; a cold compress won't help here.

## Background and References

The RMCP+ protocol, including Remote Authentication and Key-Exchange Protocol
(RAKP), is described  in detail in the IPMI v2 Specification
[https://www.intel.com/content/www/us/en/products/docs/servers/ipmi/ipmi-second-gen-interface-spec-v2-rev1-1.html](https://www.intel.com/content/www/us/en/products/docs/servers/ipmi/ipmi-second-gen-interface-spec-v2-rev1-1.html)
Specifically, sections 13.15 (IPMI v2.0/RMCP+ Session Activation) through 13.34
(Random Number Generation) are relevant.

Datagram Transport Layer Security (DTLS) is a well-established method of running TLS over UDP.
 - [DTLS Wikipedia page](https://en.wikipedia.org/wiki/Datagram_Transport_Layer_Security)
 - [DTLS 1.0 RFC](https://tools.ietf.org/html/rfc4347)
 - [DTLS 1.2 RFC](https://tools.ietf.org/html/rfc6347)

Some of the issues that are driving this are:
1) RMCP+ requires the BMC to store the user passwords in a manner so that the
   passwords can be used as HMAC keys as part of the authentication process.
2) RMCP+ uses weak ciphers and algorithms. While cipher suite 17 does offer
   SHA256-HMAC, it then truncates it, reducing the security. AES-128 is no
   longer recommended, and that is the best RMCP+ has to offer in terms of
   symmetric encryption. The key creation mechanisms for ephemeral session keys
   do not use modern standards, and are considered to be weak.
3) RAKP, used in session creation is vulnerable to various attacks such as
   MITM, password sniffing, privilege escalation, and other attacks.

## Requirements

1) Use modern crypto standards instead of custom protocols.
2) Be able to move forward as crypto ciphers and protocols change.
3) Be able to co-exist with current RMCP+.
4) Use as much of the existing IPMI machinery, only throwing out the bits that
   are insecure.
5) Remove the need for passwords to be stored unencrypted on the BMC.

## Proposed Design

Listen on UDP port 623 with a packet multiplexer that would be able to direct
the incoming packets to the correct session, which could be either an RMCP+
session or a RMCP+DTLS session, based on the first incoming message from that
client. When a client first connects to the BMC, it can either send an RMCP+
session initiation series or it can send the DTLS handshake (ClientHello) to
initiate a secure session.

### Implementation Details

1) Wait on UDP port 623 for incoming messages
2) For each incoming packet:
  1) Look in current connection table for matching client (IP/socket pair)
  2) If connection exists, push packet into the client's queue
  3) If no connection exists, check to see if this 'first packet' is a
     ClientHello or an RMCP+ packet
  4) If packet is RMCP+, create a new RMCP+ connection
  5) If the packet is ClientHello, create a new RMCP+DTLS connection
  6) Push the packet onto the new connection's queue

For RMCP+ connections, nothing really changes. For RMCP+DTLS sessions, they
begin with the first packet even though that is prior to the actual IPMI-level
session. Because the TLS state must be tracked from very early on, it is
necessary to keep track of each RMCP+DTLS session from the first packet. This
also means that the session manager will need to be very dilligent at culling
stale sessions to limit resource utilization.

#### RMCP+DTLS Session Initiation

The RMCP+DTLS server will use the same private key and certificate that the
bmcweb/redfish server uses. This will allow the client to trust that it is
communicating with a known host during the DTLS session initiation. The DTLS
session will be initiated using cipher suites that are configured.

The session initiation will be built on top of the DTLS layer using either
certificate-based mutual authentication or Secure Remote Password (SRP)
protocol. SRP is defined in [RFC 2945](https://tools.ietf.org/html/rfc2945) and
TLS-SRP is defined in [RFC 5054](https://tools.ietf.org/html/rfc5054). SRP
allows for secure password authentication without the need to store passwords
in a plain-text format.

The initial Client Hello will let the BMC know which path it is to move
along for authentication. If the client provides a username, SRP will be
used, otherwise, mutual certificate authentication will be used.

DTLS-SRP authentication:
```sequence
Client->BMC: Client Hello (SRP option with username)
BMC->Client: Server Hello
BMC->Client: Certificate (optional if server has self-signed certificate)
BMC->Client: Server Key Exchange (SRP key options)
BMC->Client: Server Hello Done
```

DTLS certificate mutual authentication:
```sequence
Client->BMC: Client Hello
BMC->Client: Server Hello
BMC->Client: Certificate
BMC->Client: Server Key Exchange
BMC->Client: Certificate Request
Client->BMC: Certificate
BMC->Client: Server Hello Done
```

By the time the BMC reaches Server Hello Done, it will have a decision on
whether or the user has successfully authenticated.

In the successful authentication case, the session setup would be considered
complete and all further traffic on the channel would be framed as RMCP+ with
cipher suite 0 (no authentication, integrity, or security) because the
underlying DTLS transport would provide those.

A fully established session should make "Set Privilege Level" the first
authenticated call to set the session privilege at the minimum privilege
required for the session. The BMC will reject any requests to assume a
privilege that is higher than the maximum privilege for that user or channel.

##### Notes about SRP
1) SRP was originally written to work around a patent for secure remote
   authentication, but since then, the patent has expired anyway.
3) Because of the way SRP does the authentication, the passwords are not just
   sent in plain text over the channel and cannot be used in the current PAM
   plugins because the encrypted passwords are stored differently.
2) SRP has a PAM plugin that could also be used, but doing so might require
   that the shadow file is changed or might possibly cause incompatibilities
   with existing passwords.

##### Alternatives to SRP
If password authentication is required, an alternative to SRP might be a simple
challenge/response interchange once the TLS session is complete. This could be
accomplished in a variety of methods. One would be to simply initiate an RMCP
session using the standard protocol, but inside the TLS session. The advantages
of this over simply a challenge are that a new interchange doesn't need to be
created and that the password does not ever really cross the network.

#### RMCP+DTLS Management

- Add an IPMI command to enable/disable RMCP+ and RMCP+DTLS
- Add an IPMI command to configure RMCP+DTLS authentication methods
- Add an IPMI command to configure DTLS cipher suites
- Add a build flag to phosphor-net-ipmi to enable RMCP+DTLS
- Add a build flag to phosphor-net-ipmi to disable RMCP+
- Add code to disable RMCP+ or RMCP+DTLS at runtime


### Existing Issues with DTLS
1) Multi-client support is not yet implemented within the OpenSSL library. It
   is trivially implemented with a custom BIO that can multiplex the incoming
   packets to the proper TLS session.
2) The custom BIO solution breaks the cookie mechanism that allows DTLS to be
   stateless until a session is fully formed. This means that care must be
   taken to ensure that incomplete sessions are culled and do not take extra
   resources. The current solution is to have a limit on the number of sessions
   and cull sessions as needed: idle incomplete sessions over a given age; idle
   sessions over a given age; and a random incomplete session if no session
   slots are available for a new incoming session.

## Alternatives Considered
### Add a New OEM Cipher Suite
If all the current standard cipher suites are bad, why not just add a new one
that uses modern algorithms? The flaw in this proposal is that the algorithms
used are only part of the problem. The cipher suites of algorithms are used as
a part of a larger security protocol (RAKP/RMCP+), which is also deeply flawed
with previously discovered and as of yet undiscovered issues such as
side-channel attacks, replay attacks, and offline brute-force attacks. Sure,
the algorithms are showing age, with only cipher suite 17 even coming close to
security. But the issues are deeper than just the algorithms. Using existing
standards such as TLS avoid this problem because the application uses the
interfaces high above the algorithm level, allowing the implementation of the
security protocol to live and change as issues come up and get resolved,
without impacting the API that the application is using. As a secondary
consideration, that is only of minor interest after that first issue, an OEM
cipher suite would need to be integrated into all the available tools to be of
any use. If this work is to be done, it may as well be done right and move to
TLS.

### Remove IPMI Completely or in Part
This is a popular choice for some; there are alternatives out there that can do
much of what IPMI does. For example, Redfish, PLDM, and MCTP all have some
overlap with IPMI and might be able to replace some or all of it with some
effort. The reality is that many IT shops will continue to use IPMI and desire
better security.

### TLS on TCP port 623
This would guarantee that all traffic could be encrypted. The port would be
able to be tunneled, which might be considered good or bad. TCP 623 is
currently not currently officially reserved for anything, but also, it is not
reserved for IPMI. Special firewall rules (besides standard rules for RMCP)
would need to be made, given that RMCP currently runs on UDP 623.  Obviously,
the start session protocol would be slightly different, probably something
like:
1) Init TLS session
2) Get Channel Authentication Capabilities
3) Activate Session

### DTLS on UDP 6230
The port here does not matter so much, because any port would have the same
issues. The same issues and benefits would be similar to TCP 623 with the
exception that UDP cannot be tunneled as trivially as TCP.

### OpenVPN on BMC to Tunnel RMCP+
OpenVPN is a fairly lightweight, secure VPN solution that could run on the BMC.
Then it would just be a matter of sending the RMCP+ packets over the secure
tunnel. This method would address the insecure RMCP+ cipher suites, but does
not address the plain-text password storage problem.

### DTLS Upgrade with STARTTLS
During the start of session, offer a mechanism for DTLS session negotiation to
begin. This will provide solutions to the issues above and still allow IPMI to
be used in a secure manner.
1) Because the authentication and authorization can be done over a secure
   channel (post-DTLS session initiation), standard PAM authentication
   mechanisms could be used, allowing the BMC to only be required to store
   salted-encrypted hashes of users' passwords. Also, it would be possible to
   offer mutual DTLS authentication, much like the OpenBMC Web interface
   offers, uploading trusted user certificates to the BMC and then using those
   to authenticate and authorize users instead of passwords.
2) DTLS ciphers move forward with modern cryptographic invention. Much like web
   traffic started with SSL1 and is now on TLS1.3, DLTS uses similar cipher
   suites, but runs over UDP instead of TCP. As old algorithms and protocols
   get deprecated, DTLS can move forward with newer standards without affecting
   the higher-level IPMI traffic.
3) DTLS key exchange works along the same lines as TLS key exchange and was
   created by crypto experts in an open manner. This helps provide some sort of
   confidence in the resulting connection that RMCP+ cannot expect to have.

RMCP and RMCP+ have a well-established port number and protocol. This new
solution would be very similar to RMCP+ over DTLS, with some minor variations
to allow tools to gracefully fail if they do not support this new protocol. The
session initiation will follow RMCP more closely, since the RMCP+ session
initiation is mostly the RAKP exchange, which is not needed with TLS.

#### Session Initiation
Some IPMI commands are available outside of a session. These session-less
commands will be available prior to the DTLS session initiation. Some of these
commands will need minor changes to be able to serve legacy RMCP+ and RMCP+
over DTLS.  For a small amount of brevity, this new session type will be called
RMCP+DTLS. These commands are outlined below:

##### Get Channel Authentication Capabilities
This command will be modified as follows:
Request byte 1, bit 6 will be changed from reserved to mean "fetch DTLS
parameters" if set. If clear, there is no behavior change if both RMCP+ and
RMCP+DTLS are supported.

Response byte 2, bit 6 will be changed from reserved to mean "RMCP+DTLS is
supported". If the command returns with an error completion code or byte 2, bit
6 is not set, RMCP+DTLS is not supported by this BMC.

Bytes 6-9 will specify an 'OEM' authentication type:
Bytes 6-8: OpenBMC IANA PEN 49871
Byte 9 is defined as follows:
* bit 0 - password authentication is available
* bit 1 - certificate-based authentication is available
* bit 2-7 - reserved

##### Get Session Challenge
If *Get Channel Authentication Capabilities* provides byte 2, bit 6 set, the
*Get Session Challenge* command can proceed with the following changes to
initiate a DTLS session instead of the RAKP key exchange and a legacy RMCP+
session.

The *Get Session Challenge* request size will be the same as a normal RMCP
request with:
Byte 1 set as either 4-password, or 5-OEM proprietary authentication
Bytes 2:17 will only contain { 'S', 'T', 'A', 'R', 'T', 'T', 'L', 'S', 0,...}
instead of the username.

This will be a valid *Get Session Challenge* request for a BMC that only
supports RMCP, but does not support RMCP+DTLS, but would return an error if
there was no user named STARTTLS .

A successful RMCP+DTLS *Get Session Challenge* response will be the same from
bytes 1:5. Bytes 6:21 will only contain { 'S', 'T', 'A', 'R', 'T', 'T', 'L',
'S', ' ', 'O', 'K', 0,...}

When a client gets a "STARTTLS OK" response to an *Get Session Challenge*
request, the next packet to the BMC must be the DTLS session initiator.

**Errors**
A BMC that only supports RMCP+DTLS that did not get a "STARTTLS" *Get Session
Challenge* request will respond with status code 70h (STARTTLS missing)
The BMC should also make clear at this point if the client is requesting an
authentication method (password or OEM) that it does not support, it will
return an appropriate error 71h (Invalid Authentication Mechanism).

##### DTLS Initiation
The DTLS session initiation starts when the client sends the first packet after
it receives the "STARTTLS OK" response. After the underlying transport is
initiated, the user authentication and authorization takes place.
**Password Authentication**
If password authentication was requested in *Get Session Challenge* request,
the authentication will be done during the *Activate Session* command.
**Certificate Authentication**
If certificate authentication was requested in the *Get Session Challenge*
request, the client will follow the standard TLS mutual authentication
mechanism to authenticate the user.

##### RAKP 1 / RAKP 3
If legacy RMCP+ is not supported, these will both return error codes 12h
(Illegal or unrecognized parameter), corresponding to RMCP+ not supported.
