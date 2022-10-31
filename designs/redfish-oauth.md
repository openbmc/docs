# Redfish OAuth 2.0

Author: Nan Zhou (nanzhoumails@gmail.com)

Created: Oct 31, 2022

## Problem Description

The Redfish authorization subsystem controls which authenticated users have
access to resources and the type of access that users have.

One of the option for a Redfish service implementation as per Redfish
Specification is to integrate the RFC6749-defined OAuth 2.0 authorization
framework. This document proposes an OAuth architecture in OpenBMC.

## Background and References

### OAuth 2.0

OAuth 2.0 is the industry-standard protocol for authorization.

OAuth defines four different roles.
1. Resource owner: An entity capable of granting access to a protected
   resource.
2. Resource server: the server hosting the protected resources, capable of
   accepting and responding to protected resource requests using access tokens.
3. Client: an application making protected resource requests
4. Authorization server: the server issuing access tokens to the client after
   successfully authenticating the resource owner and obtaining authorization.

A typical workflow is mapped out below.

```
     +--------+                               +---------------+
     |        |--(A)- Authorization Request ->|   Resource    |
     |        |                               |     Owner     |
     |        |<-(B)-- Authorization Grant ---|               |
     |        |                               +---------------+
     |        |
     |        |                               +---------------+
     |        |--(C)-- Authorization Grant -->| Authorization |
     | Client |                               |     Server    |
     |        |<-(D)----- Access Token -------|               |
     |        |                               +---------------+
     |        |
     |        |                               +---------------+
     |        |--(E)----- Access Token ------>|    Resource   |
     |        |                               |     Server    |
     |        |<-(F)--- Protected Resource ---|               |
     +--------+                               +---------------+
```

References:
1. https://www.rfc-editor.org/rfc/rfc6749

### Native Redfish Authorization

The native Redfish authorization model defined in the Redfish spec consists of
the privilege model and the operation-to-privilege mapping, where the Redfish
service authenticates the peer and maps the peer to a Redfish role. In OpenBMC,
the proposal Dynamic Redfish Authorization [1] proposed an implementation for
this workflow. Please refer to this proposal for other background about BMCWeb
(the Redfish service) and phosphor-user-manager (the backend of user
management).

References:
1. https://github.com/openbmc/docs/blob/master/designs/redfish-authorization.md

### Redfish Authorization Delegation

A Redfish service can also delegate authorization using the OAuth 2.0
framework. "Delegation" in this context means the Redfish service doesn't need
to performs user to Redfish role mapping. It asks other services for such work
and consumes the result (as authorization tokens) directly.

To support delegation, the Redfish service must be able to receive an
RFC7519-defined JSON Web Token (JWT) in the `Authorization` request header.
JWTs are a compressed JSON structure that contain a JOSE Header, a set of
claims that describe the type of access that is granted to a client, and a
signature.

The JOSE Header consists of type of the token and the algorithm of the
signature of the token which is used for verification.

The set of claims must contain the following items: identifier, subject,
issuer, audience, valid time, valid time, and most importantly, the scope,
which specifies the Redfish Roles or Redfish Privileges granted by the
authorization server to the client.

In Redfish world, the Redfish service is considered as the resource server. In
OpenBMC, BMCWeb is the corresponding implementation. Redfish clients who send
request to access Redfish resources are considered OAuth clients.

1. https://redfish.dmtf.org/schemas/DSP0266_1.15.1.html#delegated-authorization-with-oauth-20
2. https://www.rfc-editor.org/rfc/rfc7519
3. https://github.com/openbmc/bmcweb

### Gaps

As far as the proposal was written, there are no existing services in OpenBMC
which implements authorization server and resource owner of the OAuth
framework.

However, modern systems have use cases where the user (authenticated
identities) to Redfish role mapping need to happen outside the Redfish server:
a client in a separate authentication system (e.g., SSO, Google's LOAS, etc)
accesses a Redfish server that doesn't understand the client's credentials; on
the other hand, the resource owner can authenticate the client. The OAuth
framework fits in naturally for this use case.

## Requirements

BMC implements the OAuth framework and integrates it with the existing Redfish
architecture.

1. BMCWeb shall support OAuth 2.0 authorization delegation
     * it shall accept access tokens in Authorization request header as bearer
       tokens;
     * it shall accept JWT as the token `typ`; it shall support `RS256` as the
       `alg` and must not accept `none` as the `alg`; it might support other
       token types and signature algorithms
     * it must verify any authorization token: signature, expiration, and
       audience; it shall use a public key to verify signature; it might use
       mTLS to verify subject of the token matches the client
     * it shall support Redfish roles and privileges in the `scope` claim. When
       processing Redfish roles, it shall interact with phosphor-user-manager
       to get the corresponding Redfish privileges
2. BMC can optionally implement an authorization server as a native daemon with
   the Redfish server
     * the authorization server must be able to verify authorization grant from
       the resource owner or the authorization request is from the resource
       owner itself; e.g., it can use mTLS to verify the peer that requests
       authorization tokens is the resource owner
     * the authorization server must provide ways for the Redfish server to
       verify the signature of a token; e.g., it delivers the public key to the
       Redfish server to verify the signature of the tokens from clients

Designs of the interactions between clients and resource owners are unspecified
and out of the scope of this proposal. However, we provide an example
architecture where resource owner acts as a Redfish proxy.

## Proposed Design

The high level diagram of the whole design is mapped out below.

```
  +-------------------+
  |                   |
  |      Client       |
  |                   |
  +----^---------|----+
       |  LOAS   |
 200 OK|         |GET /Redfish/v1/Chassis
       |         |
       |         |                                                        +-----------------------------------------+
  +----|---------v----+                    GetToken                       |  +------------------------------------+ |
  |                   |------------------------------------------------------>                +-----------+       | |
  |  Resource Owner   |  mTLS                                             |  |  Authz Server  |Private Key|       | |
  |                   |<------------------------------------------------------                +-----------+       | |
  +----^---------|----+                     Token                         |  +------------------|-----------------+ |
       |         |                                                        |                     |   +------------+  |
       |         |                                                        |                     +---> Public Key |  |
       |         | GET /redfish/v1/Chassis; Authorization: Bearer $Token  |  +--------------+       +--|---------+  |
       |         +----------------------------------------------------------->              |          |            |
       |                                                                  |  |Redfish Server|          |            |
       +----------------------------------------------------------------------              <----------+            |
                                      200 OK                              |  +--------------+                  BMC  |
                                                                          +-----------------------------------------+
```

A typical workflow is mapped out below,
1. the authorization server on BMC starts and runs, it generates a random
   private and public key pair; it keeps the private in memory and dump the
   public key in disk
2. the client asks for resource grant or, as the example shows, the resource
   itself directly, from the resource owner
3. the resource owner authenticates the client and authorize its access by
   giving it certain Redfish privileges; in the example diagram, the
   authentication method is LOAS (Google's cluster authentication system)
4. the resource owner asks the authorization server to mint an authorization
   token for this client in an mTLS channel; within the request, th resource
   owner specifies which Redfish privileges (or which Redfish role) to grant;
5. the authorization server receives the token request; it verifies the peer is
   the resource owner via a mTLS channel; it generates a token with proper
   claims, and signs the token with the private key; it sends back the token to
   the resource owner as a JWT object
6. the resource owner then attaches the returned token to the Redfish request
   as a bearer token and forwards it to the Redfish server
7. the Redfish server verifies the authorization token, performs authorization
   (check if the granted privileges in the request is a super set of the
   required privileges for the requested resource and operation), and returns
   the response
8. the response is sent back to the client by the resource owner

### phosphor-authz-server

The authorization server exposes the following interface to mint authorization
tokens. The reference implementation will use mTLS gRPC. Thus, the interface is
described in protocol-buffers.

```
enum TokenType {
  JWT = 1;
}

enum SignatureAlgorithm {
  NONE = 1;
  RS256 = 2;
}

message MintTokenRequest {
  string client = 1;
  string redfish_role = 2;
  Duration valid_for = 3;
  TokenType = 4;
  SignatureAlgorithm alg = 5;
}

message MintTokenResponse {
  string token = 1;
}

service AuthorizationService {
  rpc MintToken(MintTokenRequest) returns (MintTokenResponse);
}
```

We mapped out the other important design aspects of the authorization server
below,
1. the authorization server generates a random public/private key at every
   daemon start up using OpenSSL APIs, and dumps the public key to a fixed
   location. It notifies the Redfish server (BMCWeb) via SIGHUB so that BMCWeb
   can reload the public key it uses for token verification
2. the authorization server performs Mutual TLS (mTLS) and expects the peer to
   be in a compile-time fixed resource owner list; otherwise, it rejects the
   MintToken RPC
3. the returned token string consists of three "." delimited Base64URL-encoded
   components: a JOSE Header, a set of claims, and a signature; all three
   components are specified by the Redfish Specification
     * `typ`: JWT
     * `alg`: RS256
     * `iss`: phosphor-authz-server
     * `sub`: the SAN of the peer (the resource owner)'s X509 certificates
     * `aud`: the hostname of BMC that the Redfish server runs on
     * `exp`: the current time + the `valid_for` attribute in the request
     * `nbf`: the current time
     * `iat`: the current time
     * `jti`: a counter of issued tokens since the service starts
     * `scope`: a Redfish role

### BMCWeb

BMCWeb supports the OAuth 2.0 authorization delegation,
1. it still performs the normal authentication: username/password based or mTLS
   based; the peer must provide a valid username/password pair or a client
   certificate that's respected by the root certificates
2. after authentication, when determine the Redfish privileges of the peer, if
   the request has a bearer token in the authorization header, instead of
   sending DBus request to phosphor-user-manager with the user name of the
   peer, it decodes the JWT, performs token verification, and extract the
   Redfish role or privileges from the `scope` claim
3. when doing token verification, BMCWeb performs the following asserts
     * in the header, `typ` is `JWT` and `alg` is `RS256`
     * the signature is signed by the local authorization server: it can be
       decrypted by the public key and the decryption matches the digest of the
       whole token
     * the `iss` claim is the local authorization server
     * the token is still valid by checking the `nbf` and `iat` claims
     * the `sub` claim matches the peer's authenticated identity (e.g., Subject
       Alternative Name in the X509 certificate)
     * the `aud` matches the hostname of BMC that the Redfish server runs on
4. when decoding the `scope` claim, if it's a Redfish role, BMCWeb queries
   phosphor-user-manager for the corresponding Redfish privileges; on the other
   hand, if they're a list of Redfish privileges, BMCWeb uses them directly.

New `typ` or `alg` can be added in the future.

This feature can be enabled as an optional Meson feature, together with all
existing Authx options in BMCWeb.

## Alternatives Considered

### Remote Authorization Server

We can implement the authorization server as a micro service outside BMC.
However, this alternative needs to handle the the complexity of distributing
public keys or BMCWeb makes queries to verify the authorization tokens.

### Authorization Server as a Route in BMCWeb

We can implement a new HTTP handler in BMCWeb, e.g., accept POST requests to
`/tokens` and returns the generated tokens. Public/private key management will
be the verify similar to the local authorization server.

This can be implemented if there are other use cases of this architecture.

### Private Key Generation

The authorization server can potentially ask the existing service
(phosphor-certificate-manager) to generate a private/public key pair instead of
generating the key pair in its own process. However, this involves extra
complexity of involving another daemon.

## Impacts

BMCWeb will accept authorization tokens when the feature "oauth" is enabled,
which already follows the Redfish specification.

A reference implementation of the authorization server is added into OpenBMC.

### Organizational

- A new repository `phosphor-authz-server` needs to be created for the
  authorization server. The initial maintainer will be Nan Zhou.
- BMCWeb will be modified.

## Testing

1. The new authorization server will have unit tests as part of the CI
2. New unit tests will be added for codes added to BMCWeb
3. New Robot Framework test cases can be added for integration tests; test
   cases should include:
     * in BMCWeb, clients with proper authorization tokens are assigned
     corresponding Redfish roles or privileges, so that certain resources can
     be accessed
     * in BMCWeb, clients with invalid authorization tokens are rejected: e.g.,
       signature verification failure, expired, wrong subject or audience.
     * in authorization server, only resource owners are allowed to get
       authorization tokens and other clients are rejected