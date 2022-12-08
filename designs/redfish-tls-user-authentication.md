# Redfish TLS User Authentication

Author: Kamil Kowalski <kamil.kowalski@intel.com>

Other contributors: None

Created: June 7, 2019

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
- [RFC 5246 - TLS 1.2 Specification](https://tools.ietf.org/html/rfc5246)
- [RFC 8446 - TLS 1.3 Specification](https://tools.ietf.org/html/rfc8446)

### Dictionary

**Redfish API** - Redfish API as defined by DMTF **Redfish** - Redfish API
implementation in BMCWeb

## Requirements

Adding this would benefit WebUI's and Redfish API's security greatly, and would
push it towards modern security standards compliance.

## Proposed Design

### Process overview

Whenever `CA`'s certificate changes `User` shall provide `Redfish` with it.
After that is completed, user should request a **CSR** (**C**ertificate
**S**igning **R**equest) from `Redfish` to get a request allowing to generate
proper `user`'s certificate from `CA`. After this certificate is acquired,
`User` can use this certificate when initializing HTTPS sessions.

```
┌──┐                           ┌────┐                                 ┌───────┐
│CA│                           │User│                                 │Redfish│
└┬─┘                           └─┬──┘                                 └───┬───┘
 │    Request CA's certificate   │                                        │
 │ <──────────────────────────────                                        │
 │                               │                                        │
 │    Return CA's certificate    │                                        │
 │  ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─>                                        │
 │                               │                                        │
 │                               │          Upload CA Certificate         │
 │                               │ ───────────────────────────────────────>
 │                               │                                        │
 │                               ──────────┐                              │
 │                                         │ Generate CSR                 │
 │                               <─────────┘                              │
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
 │                   ║           │         Initiate HTTPS Session         │    ║
 │                   ║           │ ───────────────────────────────────────>    ║
 │                   ║           │                                        │    ║
 │                   ║           │   Request TLS client authentication    │    ║
 │                   ║           │ <─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─    ║
 │                   ║           │                                        │    ║
 │                   ║           │          Provide certificate           │    ║
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
`/redfish/v1/AccountService/TLSAuth/Certificates`. There can be more than one,
so user must use certificate that is signed by **any CA** that have their valid
certificate stored there. New certificates can be uploaded by *POST*ing new
certificate object on CertificateCollection.

Example POST payload:

```json
{
  "CertificateString": "... <Certificate String> ...",
  "CertificateType": "PEM"
}
```

Should CA certificate get invalid (compromised, out-of-date, etc.) it is
recommended to use `#CertificateService.ReplaceCertificate` action at
`/redfish/v1/CertificateService`, to avoid wasting space and performance
unnecessarily for processing invalid certificates.

Example `#CertificateService.ReplaceCertificate` action payload executed on
`/redfish/v1/CertificateService/Actions/CertificateService.ReplaceCertificate`:

```json
{
  "CertificateUri": "/redfish/v1/AccountService/TLSAuth/Certificates/1",
  "CertificateString": "... <Certificate String> ...",
  "CertificateType": "PEM"
}
```

#### Generating CSR

User can generate CSR in any way that is convenient to him.

#### Authentication Process

```
                                    +-+
                                    +++
                                     |
                                     V
                        +------------+------------+
                  Yes   |                         |
              +---------+  Is certificate valid   |
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
of _Boost.Beast/Boost.ASIO/OpenSSL_ as the certificate verification is being
done at the very beginning of HTTPS request processing. _OpenSSL_ library is
responsible for determining whether certificate is valid or not. For certificate
to be marked as valid, it (and every certificate in chain) has to meet these
conditions:

- does KeyUsage contain required data ("digitalSignature" and "keyAgreement")
- does ExtendedKeyUsage contain required data (contains "clientAuth")
- public key meets minimal bit length requirement
- certificate has to be in it's validity period
- notBefore and notAfter fields have to contain valid time
- has to be properly signed by certificate authority
- certificate cannot be revoked
- certificate is well-formed according to X.509
- certificate cannot be self-signed
- issuer name has to match CA's subject name

After these checks a callback is invoked providing result of user<->CA matching
status. There, in case of success Redfish extracts username from `CommonName`
and verifies if user does exist in the system.

As can be seen on the flow diagram, Redfish will use **the first valid**
credentials according to processing sequence. It is recommended for user to use
only one set of credentials/authentication data in a single request to be sure
what will be used, otherwise there is no certainty which credential are used
during operation.

User can configure which methods are available in `/redfish/v1/AccountService`
OEM schema. The sequence of credential verification stays the same regardless of
configuration. Whitelist verification is always-on, because of Redfish
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

Current auth methods will not be impacted. This proposition is based on locally
stored CA certificates, so it does not guarantee automated measures against
situations where certificates have been revoked, and user/admin has not yet
updated certificates on BMC.

## Testing

Testing should be conducted on currently supported auth methods beside TLS, to
confirm that their behavior did not change, and did not suffer any regression.

As for TLS auth itself:

1. Flow described in [Process overview](###process-overview) should be tested,
   to confirm that after going through it, everything works as expected.
2. Validity period tests - to confirm that certificates that are not-yet-valid
   and expired ones are not accepted, by both - changing validity periods in
   certificates themselves, as well as modifying time on BMC itself
3. Removing CA certificate and confirming that user will not be granted access
   after that when using certificate that worked before removal.
4. Chain certificates verification - checking that chained certificates are
   accepted as required.
5. Negative tests for breaking user's certificate - invalid username, invalid
   validity period, invalid CA, binary broken certificate, etc.
