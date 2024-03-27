# Using Meta mTLS client certificates with bmcweb

Author: Marco Kawajiri (kawmarco)

Other contributors: None

Created: March 27, 2024

## Problem Description

Meta Inc. uses an internal TLS client certificate format that follows a specific
subject common name format that doesn’t adhere to any known standard:

```
Subject: CN = <type>:<entity>/<hostname>
```

`type` being one of:

1. `user`: a Meta Inc. employee (e.g: `user:kawmarco/dev123.facebook.com`,
   already supported by bmcweb)
2. `host`: a machine in Meta Inc’s network (e.g: `host:/tw456.facebook.com` )
3. `svc`: a Meta Inc. service (`svc:openbmc_configure`)

These certificates are used throughout Meta’s infrastructure to identify
employees, machines, and services (e.g. containerised processes).

bmcweb
[already supports Meta Inc. certificates with type=”user”](https://gerrit.openbmc.org/c/openbmc/bmcweb/+/67477).
Goal is to add support for “host” and “svc” types as it’s a hard requirement to
have a functional usage of client certificate authentication in Meta
infrastructure.

## Background and References

Inside Meta Inc’s infrastructure, most requests to
[BMCs running facebook/openbmc](https://github.com/facebook/openbmc) are not
made directly by users/employees but by _services_ or from BMCs’ _managed
hosts_.

For example, most requests to power cycle machines come from a service serving
as frontend to BMCs. This frontend service receives requests from users (e.g.
operation engineers) and other services (e.g. provisioning workflow services)
instead of individual employees.

Having BMC accesses managed by these frontend services means **we only maintain
a very small set of identities that need direct access to the BMC** in
deployments of our [facebook/openbmc](https://github.com/facebook/openbmc) fork
by periodically deploying a configuration file with a list of identities and
their associated privileges (ACLs) using an privileged internal service
(_Configure Service_).

### Glossary

_Configure Service_: A privileged service that is responsible for bootstrapping
and periodically maintaining BMCs configured across Meta Inc’s fleet

_managed host_: a host (e.g. a x86 compute server) managed by a BMC.

_service_: A Meta Inc distributed service,
[usually containerised](https://engineering.fb.com/2019/06/06/data-center-engineering/twine/)
and one that is identified by a type=“svc” client certificate.

## Requirements

1. On bmcweb, map Meta Inc client certificates of different types (“user”,
   “svc”, “host”) to local BMC users (as shown in
   redfish/v1/AccountService/Accounts)
2. Ensure entities of different types never collide with the same local BMC user
   1. For example, the local BMC user for “user:zeus” MUST NOT be the same for
      the local BMC user of “svc:zeus”

## Proposed Design

1. Extend
   [the current logic that parses client certificates of type=“user”](https://gerrit.openbmc.org/c/openbmc/bmcweb/+/67477)
   to also allow type=”svc” and type=”host” (Requirement 1)

   1. https://gerrit.openbmc.org/c/openbmc/bmcweb/+/70274

2. Prefix local usernames with the certificate type so different entities never
   match to the same local user (Requirement 2), examples: 2. A client cert with
   Subject CN=“user:zeus” should match a local user “user_zeus” 3. A client cert
   with Subject CN=“svc:zeus” should match a local user “svc_zeus” 4. A client
   cert with Subject CN=”host:/hostname” should match a local user
   “host_hostname”

3. A privileged internal service (Configure Service) will ensure
   users/services/hosts that should have access to the BMC have entries in
   redfish/v1/AccountService/Accounts with appropriate roles 5. The source of
   truth is an internal permissions database 6. Entries in
   AccountService/Accounts will be modified using regular REST requests
   (POST/DELETE/etc) by Configure Service

## Alternatives Considered

- LDAP
  - LDAP is not used to authenticate access to Meta services or hosts
  - Maintaining independent LDAP infrastructure is not feasible for the small
    teams that operate OpenBMC in Meta’s fleet
- Password authentication
  - Accesses to the BMC would require queries to a password database
  - Meta Inc’s security guidelines mandate usage of passwords only for emergency
    access (certificate auth should be used for regular access)
- PAM
  - AFAICT I don’t see a user name translation mechanism for PAM (e.g. even if
    we manage to forward the full client subject CN to PAM, I’m not sure it’ll
    be able to handle a user name with characters such as `:` or `/` as present
    in the CN)

## Impacts

- Feature in bmcweb is gated by a build flag
  (`-Dmutual-tls-common-name-parsing='meta'`) meaning no runtime burden on
  regular builds
- Certificate Subject CN parsing will be kept as simple as possible to reduce
  impact on community developer attention and resources

### Organiszational

- Does this repository require a new repository? No
- Which repositories are expected to be modified to execute this design? bmcweb

## Testing

- All parsing code will have thorough unit tests exemplifying common usage and
  edge cases ([example](https://gerrit.openbmc.org/c/openbmc/bmcweb/+/67477))
- Will add flags to
  [generate_auth_certificates.py](https://github.com/openbmc/bmcweb/blob/master/scripts/generate_auth_certificates.py)
  so community developers can simulate the certificate environment at Meta Inc.
  using self-signed certificates
