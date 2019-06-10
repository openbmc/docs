# Redfish TLS User Authentication

Author:
  Kamil Kowalski <kamil.kowalski@intel.com>

Primary assignee:
  Kamil Kowalski

Other contributors:
  None

Created:
  June 7, 2019

## Problem Description

Redfish API presented by [BMCWeb](https://github.com/openbmc/bmcweb) allows user
to authenticate using quite a few methods, eg. BasicAuth, Sessions, etc. In
addition to those user can gain access to nodes by providing certificate upon
negotiating HTTPS connection for identifications. The design and principles
behind this solution are described below.

## Background and References

Redfish currently lacks support for modern authentication methods. Certificate
based auth would allow for more secure and controllable access control. Using
SSL certificates provides validity periods, ability to revoke access from CA
level, and many other security features.

Reference documents:
- [Certificate Schema Definition](https://redfish.dmtf.org/schemas/v1/Certificate_v1.xml)
- [CertificateLocations Schema Definition](https://redfish.dmtf.org/schemas/v1/CertificateLocations_v1.xml)
- [CertificateService Schema Definition](https://redfish.dmtf.org/schemas/v1/CertificateService_v1.xml)
- [DSP-IS0008 DMTF's Redfish Certificate Management Document](https://www.dmtf.org/dsp/DSP-IS0008)


## Requirements

Adding this would benefit WebUI's and Redfish API's security greatly, and would
push it towards modern security standards compliance.

## Proposed Design

### Process overview

Whenever ``CA``'s certificate changes ``User`` shall provide ``Redfish`` with
it. After that is completed, user should request a **CSR** (**C**ertificate
**S**igning **R**equest) from ``Redfish`` to get a request allowing to generate
proper ``user``'s certificate from ``CA``. After this
certificate is acquired, ``User`` can use this certificate when initializing
HTTPS sessions.

```
┌──┐                           ┌────┐                                 ┌───────┐
│CA│                           │User│                                 │Redfish│
└┬─┘                           └─┬──┘                                 └───┬───┘
 │    Request CA's certificate   │                                        │
 │ <──────────────────────────────                                        │
 │                               │                                        │
 │ Return CA's public certificate│                                        │
 │  ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─>                                        │
 │                               │                                        │
 │                               │          Upload CA Certificate         │
 │                               │ ───────────────────────────────────────>
 │                               │                                        │
 │                               │              Generate CSR              │
 │                               │ ───────────────────────────────────────>
 │                               │                                        │
 │                               │               Return CSR               │
 │                               │ <─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─
 │                               │                                        │
 │ Request certificate using CSR │                                        │
 │ <──────────────────────────────                                        │
 │                               │                                        │
 │   Return User's certificate   │                                        │
 │  ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─>                                        │
 │                               │                                        │
 │                               │                                        │
 │                   ╔═══════╤═══╪════════════════════════════════════════╪════╗
 │                   ║ LOOP  │  Typical runtime                           │    ║
 │                   ╟───────┘   │                                        │    ║
 │                   ║           │ Initiate HTTPS session with certificate│    ║
 │                   ║           │ ───────────────────────────────────────>    ║
 │                   ║           │                                        │    ║
 │                   ║           │          Return requested data         │    ║
 │                   ║           │ <─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─    ║
 │                   ╚═══════════╪════════════════════════════════════════╪════╝
┌┴─┐                           ┌─┴──┐                                 ┌───┴───┐
│CA│                           │User│                                 │Redfish│
└──┘                           └────┘                                 └───────┘
```

### BMCWeb / Redfish API

#### Uploading CA Certificate

CA's certificates for user authentication are kept at
``/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates``. There can be
more than one, so user must use certificate that is signed by **any CA** that
have theirs valid certificate stored there. New certificates can be uploaded
by *POST*ing new certificate object on CertificateCollection.

Example POST payload:
```json
{
    "CertificateString": "... <Certificate String> ...",
    "CertificateType": "PEM"
}
```

Should CA certificate gets invalid (compromised, out-of-date, etc.) it is
recommended to use ``#CertificateService.ReplaceCertificate`` action at
``/redfish/v1/CertificateService``, to avoid wasting space and performance
unnecessarily for processing invalid certificates.

Example ``#CertificateService.ReplaceCertificate`` action payload executed on
``/redfish/v1/CertificateService/Actions/CertificateService.ReplaceCertificate``:

```json
{
    "CertificateUri": "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/1",
    "CertificateString": "... <Certificate String> ...",
    "CertificateType": "PEM"
}
```

#### Generating CSR

It is recommended for user to have different certificate for each individual
platform to prevent situations where one user certificate leak would compromise
security of multiple platforms. This is unwanted in the same way as having the
same password for multiple services is strongly discouraged.

Generating user certificate is based on a **CSR**, which contains user data can
be supplied to CA that generates a certificate for user matching all the
requirements. To get such CSR a Redfish action
``#CertificateService.GenerateCSR`` should be used from
``/redfish/v1/CertificateService``. This will return a CSR String that User can
send to CA (or Sub-CA, but ensuring that certificate chain is maintained to be
able to verify that the Root CA is the one that Redfish has a certificate for.
With guaranteed support for up to 5 CAs in chain).

Only required field describing user credentials in
``#CertificateService.GenerateCSR`` action is *CommonName* where user should
provide username existing on system as it is used for identification. Other
fields are not used for the authentication process.

User needs at least ``ConfigureSelf`` Redfish privilege to be able to generate
CSR, but will be allowed to input only his own username into *CommonName*. To
generate CSR for users other than self, user need ``ConfigureUsers`` privilege.

Example ``#CertificateService.GenerateCSR`` action payload executed on
``/redfish/v1/CertificateService/Actions/CertificateService.GenerateCSR``:
```json
{
    "CommonName": "testuser",
    "Country": "US",
    "City": "Chicago",
    "State": "Illinois",
    "Organization": "Testing Inc."
}
```
Which will should return a CSR to provide to CA:
```json
{
    "CSRString": "... <Certificate Request String> ...",
    "CertificateCollection": "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates"
}
```

#### Authentication Process

```
                                    +-+
                                    +++
                                     |
                                     V
                        +------------+------------+
                  Yes   |                         |
              +---------+  Is certificate valid   +
              |         | and signed by known CA? |
              |         |                         |
              |         +------------+------------+
              |                      |
              |                      | No
              |                      V
              |          +-----------+-----------+
              |    Yes   |                       |
              +----------+  Is URI whitelisted?  |
              |          |                       |
              |          +-----------+-----------+
              |                      |
              |                      | No
              |                      V
              |          +-----------+-----------+
              |    Yes   |                       |
              +----------+ Is X-Token provided?  |
              |          |                       |
              |          +-----------+-----------+
              |                      |
              |                      | No
              |                      V
              |          +-----------+-----------+
              |    Yes   |                       |
              +----------+  Is cookie provided?  |
              |          |                       |
              |          +-----------+-----------+
              |                      |
              |                      | No
              |                      V
              |          +-----------+-----------+
              |    Yes   |                       |
              +----------+   Is Token provided?  |
              |          |                       |
              |          +-----------+-----------+
              |                      |
              |                      | No
              |                      V
              |      +---------------+--------------+
              | Yes  |                              |  No
              +------+ Is Basic auth data provided? +------+
              |      |                              |      |
              |      +------------------------------+      |
              V                                            V
+-------------+--------------+               +-------------+--------------+
|                            |               |                            |
|       Create session       |               | Return authorization error |
|                            |               |                            |
+-------------+--------------+               +-------------+--------------+
              |                                            |
              |                     +-+                    |
              +--------------------->*<--------------------+
                                    +-+
```

Certificate based authentication has the highest priority, because of the design
of *Boost.Beast/Boost.ASIO/OpenSSL* as the certificate verification is being
done at the very beginning of HTTPS request processing. It tries to match
received certificate against known CA certificates utilizing built-in library
function, which invokes a callback providing result of user<->CA matching
success. There, in case of success Redfish extracts username from ``CommonName``
and verifies if user does exist in the system.
As can be seen on the flow diagram, Redfish will use **the first valid**
credentials according to processing sequence. It is recommended for user to use
only one set of credentials/authentication data in a single request to be sure
what will be used, otherwise there is no certainty which credential are used
during operation. There is no timeout specified for the sessions, so they can be
kept alive as long as possible regardless of used authentication method.

User can configure which methods are available in ``/redfish/v1/AccountService``
OEM schema. The sequence of credential verification stays the same regardless
of configuration. Whitelist verification is always-on, because of Redfish
specification and other accessibility requirements.

User certificate does not have to be signed by the exact CAs whose certificates
are stored, but instead it can be done in a chain (Redfish guarantees support
for chain depth up to 5, but greater ones may work as well). It is recommended
to use at least 2048bit RSA or 256/384bit elliptic curve keys. Certificate has
to be in its validity period in the moment of session initialization.

#### Authorization

User identified by any of methods described above, goes through process of
examining whether user actually exists, and what privileges, groups, etc. should
be provided. Current base is BasicAuth as it should be used for creating
sessions which can be used in following connections, and it is executed by
BMCWeb through PAM library usage. Other auth methods have access only to user's
login credentials without password, so verification of user existence cannot be
directly done through classic PAM flow, and should be done in other way. This
also applies for certificate based auth, so all non BasicAuth methods should
verify whether user exists and is not locked out of the system on any login
attempt.

## Alternatives Considered

None.

## Impacts

This change should not impact current implementation in any way. It will only
add additional auth method.

## Testing

Additional tests for auth process should be added. Tests should also confirm,
that current implementation did not deteriorate, and did not suffer any
regression. Additionally security testing should be conducted on this.
