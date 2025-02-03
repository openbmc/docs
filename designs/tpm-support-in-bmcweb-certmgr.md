# TPM support for bmcweb & phosphor-certificate-manager

Author: [Manojkiran Eda][manoj-email] `<manojkiran>`

[manoj-email]: manojkiran.eda@gmail.com

Created: Feb 3, 2025

## Problem Description

Modern web servers increasingly rely on cryptographic operations for secure
communications, data protection, and identity management. Performing
cryptographic operations directly on the server can lead to scalability
challenges, performance bottlenecks, and potential exposure of sensitive keys
and operations to vulnerabilities.

This document proposes a solution to offload cryptographic operations to
hardware providers like [Trusted Platform Module][TPM-Link], [EdgeLock SE050
secure element][Edgelock-link] or [software providers][software-prov-link] like
`default`, `fips`, `base`, `null`, [oqs-provider][oqs-link] & also use the
providers as a secure certificate store by leveraging the exentensible [openssl
provider][provider-link] API's, enhancing security, scalability, and efficiency.

**NOTE** : While this document talks extensively about TPM which is considered
as a hardware provider, the proposed solution largely is valid to any hardware
or software providers.

[TPM-Link]: https://en.wikipedia.org/wiki/Trusted_Platform_Module
[Edgelock-link]: https://www.nxp.com/products/SE050
[software-prov-link]:
  https://github.com/openssl/openssl/blob/master/README-PROVIDERS.md
[oqs-link]: https://github.com/open-quantum-safe/oqs-provider
[provider-link]: https://docs.openssl.org/3.0/man7/provider

## Background and References

In the current state upon initialization, **bmcweb** verifies the existence of a
self-signed certificate in the certificate store (`/etc/ssl/certs`). If no
certificate is found, it generates a new cryptographic key pair, which is
subsequently used to create a self-signed certificate. This certificate is
securely stored in the filesystem-based certificate store and loaded into the
SSL context, enabling the server to present a server-side certificate for secure
communication and authentication. Users could also replace the self signed
certificate with a CA signed certificate using the API's exposed by
[phosphor-certificate-manager][cert-mgr-pdi].

It is worth noting that while **bmcweb** manages server-side TLS, browsers
typically employ client-side TLS to establish secure connections.

Following are the issues with having the key pair and certificates in the
filesystem, which can be addressed by offloading key generation and certificate
storage to a hardware provider like TPM2:

- Keys stored in the filesystem are vulnerable to theft or compromise through
  malware, unauthorized access, or misconfiguration. TPM2 ensures keys are
  securely generated and stored within tamper-resistant hardware.
- Filesystem-based keys are susceptible to attacks from rogue processes or
  system vulnerabilities. TPM2 isolates keys in hardware, mitigating such risks.
- Privileged users or administrators may accidentally or maliciously access keys
  stored in the filesystem. TPM2 enforces non-exportable keys, ensuring they
  remain inaccessible outside the hardware.
- Filesystem storage may not meet strict regulatory standards such as [FIPS
  140-3][fips-link]. TPM2, being a compliant hardware solution, addresses this
  requirement.
- Filesystem-based keys are portable and can be misused on other platforms. TPM2
  binds keys to the specific hardware, preventing unauthorized use.

One of the leading open-source web servers, **NGINX**, also supports offloading
cryptographic operations to hardware devices like TPM2, as detailed in its
[documentation][nginx-ssl-engine]. While it's current implementation relies on
the deprecated OpenSSL Engine APIs, there are ongoing plans to migrate this
functionality to the more modern provider APIs, as discussed in this [ticket]
[nginx-provider-api] This highlights the growing adoption of hardware-backed
cryptographic solutions in high-performance web servers.

[cert-mgr-pdi]:
  https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/yaml/xyz/openbmc_project/Certs
[fips-link]: https://en.wikipedia.org/wiki/FIPS_140-3
[nginx-ssl-engine]: http://nginx.org/en/docs/ngx_core_module.html#ssl_engine
[nginx-provider-api]: https://trac.nginx.org/nginx/ticket/2449

## Requirements

- The system must use TPM2 to generate private keys securely within the TPM,
  ensuring the keys never leave the hardware boundary.
- Support the generation of RSA and ECC key pairs to align with common
  cryptographic algorithms used in server-side certificates.
- Enable the use of TPM-generated keys for creating self-signed certificates or
  Certificate Signing Requests (CSRs).
- Store the generated certificates or their references (e.g., certificate
  handles) securely within the TPM, reducing reliance on the filesystem.
- Provide mechanisms to retrieve and use the certificates for server-side TLS
  operations without exposing the private key outside the TPM.
- Ensure that bmcweb can load and use TPM-stored certificates for establishing
  HTTPS connections.
- Implement configuration mechanisms so that user/companies could configure
  bmcweb and certificate-manager to work with a given provider.
- Extend phosphor-certificate-manager to manage TPM-based certificates,
  including install, replace certificates stored in the TPM.
- Ensure that Redfish APIs can expose TPM-related certificate management
  capabilities, such as uploading CSRs or obtaining TPM-stored certificates.
- Ensure that TPM-based cryptographic operations (e.g., signing, CSR generation)
  do not introduce significant latency compared to filesystem-based key usage.
- Support multiple certificates and keys stored in the TPM, ensuring the design
  scales to accommodate various use cases.
- Ensure compatibility with existing components (bmcweb,
  phosphor-certificate-manager) without requiring significant redesign or
  disruption to current functionality.
- Ensure the solution adheres to cryptographic standards (e.g., FIPS 140-3, NIST
  guidelines) and TPM specifications (e.g., TCG standards).
- Support widely accepted certificate formats (e.g., X.509) and key formats
  compatible with TPM operations.

## Proposed Design

To enable secure key generation and certificate storage using TPM2, the current
proposal extends bmcweb with a TPM2 provider option. This design leverages
OpenSSL's provider framework and TPM2 libraries to seamlessly integrate
TPM-backed cryptographic operations while retaining compatibility with existing
workflows & code.

### Prerequisites

- Enable TPM2 Distro Feature
  - Ensure that the build environment includes the TPM2 distro feature, enabling
    access to TPM-related packages and tools.
- Required Packages:
  - Include the following packages in the build system:
    - `tpm2-tools`(optional): Contains set of tools for interfacing with the
      TPM.
    - `tpm2-openssl`: For integrating TPM as an OpenSSL provider.
    - `tpm2-tss`: The TPM2 Software Stack libraries.
    - `libtss2-tcti-device`: For communication with the TPM device.

### bmcweb & certificate-manager

#### Build options

Introduce a providers build option to specify the cryptographic provider for
bmcweb & certificate-manager. This option determines which backend will be used
for key generation and certificate storage.

```text
option(
  'providers',
  type: 'array',
  value: ['default'],
  choices: ['default', 'fips', 'null', 'tpm2'],
  description: '''Select the cryptographic providers to enable.
                  Options include default, fips, null, and tpm2.'''
)
```

### bmcweb certificate & key lookup configuration

Currently, both **bmcweb** and **phosphor-certificate-manager** have the
certificate store path hardcoded to `/etc/ssl/certs/https`. To enable greater
flexibility, this path should be configurable via a meson build option. This
modification would leverage the [URI scheme][URI-scheme-rfc] based approach in
accessing the resource (certificate or key) and the way to access the resource
is via the openssl's [OSSL_STORE][ossl-store-link] API's.

The proposed Meson options are defined as follows:

```text
option(
  'uri-cert',
  type: 'string',
  value: ['file:/etc/ssl/certs/https/server.pem'],
  description: 'URI specifying the storage location for the server certificate'
)

option(
  'uri-key',
  type: 'string',
  value: ['file:/etc/ssl/certs/https/privkey.pem'],
  description: 'URI specifying the storage location for the server private key'
)
```

In scenarios where a provider such as a Trusted Platform Module (TPM) is
utilized, objects like certificates and keys are referenced using a 32-bit
handle instead of file paths. When the TPM hardware provider is selected through
the `providers` option, it is necessary to specify the TPM2 handle values (e.g.,
`0x81000000`) for accessing these objects. In such cases, the Meson options
would be changed to `handle:0x81000000`, where `handle:` is the URI scheme to
reference TPM objects.

**NOTE** : While the proposed design talks about the TPM2 URI scheme to use
`handle:`, it is [currently being debated][tpm2-uri-debate] and could change to
something else in future.

[URI-scheme-rfc]: https://www.iana.org/assignments/uri-schemes/uri-schemes.xhtml
[ossl-store-link]: https://docs.openssl.org/3.0/man7/ossl_store
[tpm2-uri-debate]: https://github.com/tpm2-software/tpm2-openssl/issues/71

### certificate manager certificate & key lookup configuration

Currently certificate-manager takes `--path` command line argument which would
point to the certificate or key location in the filesystem.

For example:

```text
# https certificate
phosphor-certificate-manager --type=server --endpoint=https \
    --path=/etc/ssl/certs/https/server.pem --unit=bmcweb.service

# ca certificate
./phosphor-certificate-manager --type=authority --endpoint=truststore \
    --path=/etc/ssl/certs/authority --unit=bmcweb.service

#ldap client certificate
./phosphor-certificate-manager --type=client --endpoint=ldap \
    --path=/etc/nslcd/certs/cert.pem
```

The `path` command-line argument should be deprecated and instead two new
command line arguments `uri-cert` & `uri-key` should be added with support to
take a `URI scheme` instead of a traditional file system path. This change
enables the use of the `OSSL_STORE APIs`, allowing certificates and keys to be
stored either in the TPM or the filesystem, depending on the providers loaded
via the `providers` meson option.

Most of the existing functionality/code for key pair and certificate generation
in both bmcweb and phosphor-certificate-manager will remain unchanged. However,
the following enhancements are required:

1. Provider Library Integration – Mechanisms must be introduced to load the
   appropriate cryptographic provider.
2. Standardized Lookup via OSSL APIs – File system lookups will be replaced with
   `OSSL_STORE API` calls that utilize `URI schemes` for certificate and key
   retrieval.

## Alternatives Considered

An alternative approach to integrating TPM support into both BMCWeb and the
certificate Manager involved leveraging the APIs provided by the TPM TSS library
for all cryptographic operations. While this method would have offered precise
control over TPM functionalities, the resulting implementation would lack
scalability across different hardware and software providers.

## Impacts

### API Impact

- Possible modifications to existing bmcweb & certificate management APIs to
  accommodate TPM-based keys.

### Security Impact

- Enhanced security through hardware-based key storage and attestation.
- Potential risks associated with TPM misuse, requiring strict access controls
  and policies.

### Documentation Impact

- Updates required for API documentation to detail TPM integration.
- Additional guidance for developers and administrators on configuring and using
  TPM.

### Performance Impact

- Potential increase in cryptographic operation latency due to TPM hardware
  interaction.
- Improved security benefits outweigh minor performance trade-offs.

### Developer Impact

- Developers will need to familiarize themselves with TPM standards and
  integration techniques.
- Changes in authentication and key management workflows will require training
  and adaptation.

## Organizational

- **Does this proposal require a new repository?**
  - No
- **Who will be the initial maintainer(s) of this repository?**
  - N/A
- **Which repositories are expected to be modified to execute this design?**
  - `bmcweb`
  - `phosphor-certificate-manager`
- **Maintainers to be added to the Gerrit review:**
  - `@bmcweb-maintainers`
  - `@phosphor-certificate-manager-maintainers`

## Testing

- Unit tests will be implemented to verify TPM interactions in both bmcweb and
  certificate Manager.
- Integration tests will ensure seamless operation between services and TPM
  hardware or software TPM hooked on to qemu running BMC firmware.
