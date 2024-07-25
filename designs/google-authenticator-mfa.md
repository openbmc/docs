# Google Authenticator multi-factor authentication

Author: Raviteja

Created: 10/06/2024

## Problem Description

This design is to enable Time-based One-time Password (TOTP) based google
authenticator two factor authentication to BMC as part of multi-factor
authentication

DMTF has already defined the Google Authenticator multi-factor authentication
model and schemas. This design is to adapt google-authenticator-libpam module
into openbmc system, implement D-bus interfaces and BMCWeb Redfish interfaces so
that OpenBMC systems supports interfaces to configure Google Authenticator and
login to BMC systems with two factor (Password and TOTP) authentication.

## Background and References

### Google Authenticator

Google Authenticator provides 'google-authenticator-libpam' PAM module which
supports Time-based One-time Password (TOTP) two-factor authentication for
logging into servers specified in RFC 6238.

References:

1. http://redfish.dmtf.org/schemas/v1/AccountService.v1_15_1.json#/definitions/GoogleAuthenticator
2. http://redfish.dmtf.org/schemas/v1/AccountService.yaml#/components/schemas/AccountService_MFABypass
3. https://redfish.dmtf.org/schemas/v1/Session.v1_7_2.yaml
4. https://github.com/google/google-authenticator-libpam/blob/master/README.md
5. https://datatracker.ietf.org/doc/html/rfc6238

### Phosphor-user-manager

Phosphor-user-manager is an OpenBMC daemon that does user management.

It exposes DBus APIs to dynamically add users, manage users' attributes (e.g.,
group, privileges, status, and account policies). It has a hardcoded list of
user groups (SSH, IPMI, Redfish, Web) and a hardcoded list of privileges
("priv-admin", "priv-operator", "priv-user", "priv-noaccess"). These privileges
are implemented as Linux secondary groups.

References:

1. https://github.com/openbmc/docs/blob/master/architecture/user-management.md
2. https://github.com/openbmc/phosphor-user-manager
3. https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/yaml/xyz/openbmc_project/User

### BMCWeb

BMCWeb is an OpenBMC Daemon which implements the Redfish service (it implements
other management interfaces as well besides Redfish).

BMCWeb currently supports PAM based username/password style of authentication,
on successful authentication BMCWeb creates redfish session.

References:

1. https://github.com/openbmc/bmcweb

## Requirements

1. Support GoogleAuthenticator Time-based One-time Password (TOTP) two factor
   authentication on OpenBMC.
2. GoogleAuthenticator MFA configuration supported at AccountService
   configuration. This is disabled by default.
3. MFA bypass configuration is supported on ManagerAccounts where MFA can be
   bypassed for specific users.
4. When MFA enabled, User needs to generate google-authenticator secret key
5. User expected to configure google authenticator secret key in Google
   authenticator mobile application or use oauth tool to generate TOTP token.
6. Enable TOTP MFA for local BMC users, this design does not cover LDAP users
   TOTP MFA support which can be extended later in future.
6. Support redfish session create with password and MFA token
7. Only Admistrator privileged users will be allowed to configure MFA
   configuration
8. Time and date on the BMC should match with client device date and time
9. There is no two factor authentication support for ssh.
10. No two factor authentication support for IPMI user

## Proposed Design

The google authenticator is designed to protect user authentication with a
second factor Time-based One-time Password (TOTP). Prior logging in, the user
will be asked for both password and one-time passcode. Such one-time codes can
be generated with the Google Authenticator application, installed on the user's
Android device oruse oauth tools. To respectively generate and verify those
one-time codes, a secret key (randomly generated) must be shared between the
device on which one-time codes are generated and the system on which Google
authenticator MFA is enabled.

1. Google authenticator PAM module will be pulled into OpenBMC and configured as
   required.
2. UserManager implements these new D-bus interfaces
   - MultiFactorAuthentication D-bus interface to enable/disable MFA at
     user-manager object
   - MFABypass D-bus interface to support user MFA bypass configuration
   - GoogleAuthenticator D-bus interface to support CreateSecretKey method to
     generate secret for the user.
   - IsSecretKeySet propery to indicate whether secret key or not.
3. UserManager persists GoogleAuthenticator MFA D-bus configuration across
   reboots.
4. BMCWeb implements google authenticator multi-factor authentication
   configuration as per redfish schema
5. BMCWeb implements enable/disable GoogleAuthenticator at redfish
   AccountService.
6. BMCWeb supports multi-factor authentication bypass configuration for each
   user account.
7. When GoogleAuthenticator MFA enabled, next redfish user session create
   request recommends user to setup google authenticator secret key for the user
   account unless user explicitly bypass google authenticator MFA.
8. BMCWeb supports "GenerateSecretKey" action which invokes user-manager D-bus
   call to create user specific secret key and shares secret key in redfish
   response property.
9. User needs to configure secret key to generate TOTP token in
   google-authenticator mobile app or use oauth tool.
10. Users expected to pass both Password and MFA TOTP token for further redfish
    session create or login.
11. BMCWeb supports MFA token parameter along with username and password for two
    factor authentication.
12. BMCWeb needs to pass this MFA TOTP token to pam_authenticate() API along
    with password. google-authenticator PAM module verifies given TOTP token
    against user's secret key to pass authentication successfully.

```
 +-----------------------+   +-------------------+
 |Redfish session request|   |GoogleAuthenticator|
 |  with password + TOTP |   |  Enable/Disable   |
 |                       |   |MFA Bypass config  |
 |                       |   |GenerateSceretKey  |
 +--------|--------------+   +--------|----------+
          |                           |
          |      +--------------------+
          |      |
 +-----------------+                         +---------------------+
 |      BMCWeb     |DBus:GoogleAutheticator  |phosphor-user-manager|
 |                 ------------------------->|                     |
 | AccountService  |<------------------------|  MFABypass config   |
 |   Accounts      |                         |  GenerateSecretKey  |
 |                 |                         |                     |
 |                 |   Linux PAM Stack       |  Enable/Disable     |
 |                 |   +------------------+  |GoogleAuthenticator  |
 +-------+---------+   |pam_unix.so       |  +---------------------+
         |             |pam_google_auth.so|           | Generate secret key
         |             |                  |           |
         +------------>|pam_ldap.so       |    +------+--------------------------+
                       +------------------+    | Secret key store                |
                                |              |                                 |
                                +------------> |/home/$user/.google-authenticator|
                                               +---------------------------------+

```

## BMCWeb / Redfish API

### Google Authenticator MFA Enable

Google authenticator configuration supported by "GoogleAuthenticator" redfish
property at '/redfish/v1/AccountService' By default this Enabled property set to
false.

Example PATCH payload:

```json
{
  "GoogleAuthenticator": {
    "Enabled": true
  }
}
```

### Multi-Factor authentication bypass configuration

Multi-Factor Authentication bypass configuration 'MFABypass' property supported
at '/redfish/v1/AccountService/Accounts/<user>' and by default this MFABypass
property set to null for each user account.

Example PATCH payload:

```json
{
  "MFABypass": {
    "BypassTypes": "GoogleAuthenticator"
  }
}
```

### GenerateSecretKeyRequired MessageRegistry

When MFA is enabled and in next redfish session create request throws
'GenerateSecretKeyRequired' redfish message response and directs user to perform
'GenerateSecretKey' operation to setup google authenticator secret key.

Example redfish session create request response when MFA enabled and no secret
key setup for the user.

```json
{
  "@Message.ExtendedInfo": [
    {
      "@odata.type": "/redfish/v1/$metadata#Message.v1_5_0.Message",
      "Message": "Google authenticator secret key needs to be setup for this account before access is granted. POST on '/redfish/v1/AccountService/Accounts/<user>/Actions/ManagerAccount.GenerateSecretKey' to complete this process.",
      "MessageArgs": ["/redfish/v1/AccountService/Accounts/<user>"],
      "MessageId": "Base.1.8.1.GenerateSecretKeyRequired",
      "MessageSeverity": "Critical",
      "Resolution": "Generate secret key for this account using a POST to perform redfish action 'GenerateSecretKey' at the URI provided."
    }
  ]
}
```

### GenerateSecretKey

- Generate secret key redfish action supported at
  '/redfish/v1/AccountService/Accounts/<user>'
- Perform POST operation on
  '/redfish/v1/AccountService/Accounts/<user>/Actions/ManagerAccount.GenerateSecretKey'

Example redfish response:

```json
{
  "GenerateSecretKeyResponse": {
    "SecretKey": "RCWOKVVTE77VMPIKMJC44H2HC4"
  }
}
```

#### GenerateSecretKey Workflow

```
 +-------------------------+
 |    User's first login   |        BMCWeb                    User-Manager
 | with GoogleAuthenticator|         |                              |
 |      MFA Enabled        |         |                              |
 +-------------------------+         |                              |
         |                           |                              |
         |   Create redfish session  |                              |
         |-------------------------->|                              |
         |                           |  Creates redfish session     |
         |                           |  with ConfigureSelf and      |
         |GenerateSecretKeyRequired MessageRegistry to setup secretkey
         |<--------------------------|                              |
         |                           |                              |
         |   PATCH GenerateSecretKey |                              |
         |-------------------------->|   DBus: CreateSecretKey      |
         |                           |----------------------------->|
         |                           |                              |
         |                           |                 +-----------------------+
         |                           |                 |  setup secret key file|
         |                           |                 |  in user's home space |
         |                           |                 +-----------------------+
         |                           |                              |
         |                           |                              |
         | GenerateSecretKeyResponse | Read secret key from file    |
         |       Secretkey           |<-----------------------------|
         |<--------------------------|                              |
         |                           |                              |

```

User expected to make a note of this secret key and configure this secret key to
generate TOTP token in Google TOTP mobile app or use oauth tool. Next redfish
login expects user to pass both "Password" and "Token" MFA token parameters
otherwise user gets `ResourceAtUriUnauthorized` error

### Create redfish session with two factor authentication

POST on /redfish/v1/SessionService/Sessions

Example redfish payload:

```json
{
  "UserName": "newadminuser",
  "Password": "0penBmc0",
  "Token": "654321"
}
```

#### Create redfish session workflow with MFA

```

            User                BMCWeb                     PAM Stack
              |                   |                            |
              |                   |                            |
              | redfish session   |                            |
              |------------------>|  Invoke pam auth calls     |
              |     request       | with password + TOTP token |
              |                   |--------------------------->|
  Create      |                   |                            |
Redfish sesion|                   |                            |
              |                   |            +----------------------+
              |                   |            | pam_google_auth.so   |
              |                   |            |                      |
              |                   |            |Verfies 6 digit TOTP  |
              |                   |            |with user's secret key|
              |                   |            +----------------------+
              |                   |                            |
              |                   |            +---------------------+
              |                   |            |  pam_unix.so        |
              |                   |            |Verfies user password|
              |                   |            +---------------------+
              |                   |                            |
              |                   |      okay                  |
              |  session created  |<---------------------------|
              |<------------------|                            |
              |       response    |                            |
              |                   |                            |
              v                   v                            v
```

## Alternatives Considered

## Impacts

1. New DBus interfaces on phosphor-user-manager
2. GoogleAuthenticator MFA redfish properties implemented by BMCWeb
3. Multifactor Authentication bypass properties implemented in BMCWeb
4. Add support MFA "Token" parameter in sessions implementation.

## Organizational

No new repository is required. Phosphor-user-manager and BMCWeb will be modified
to implement the design.

## Testing

New Robot Framework test can be added to test MultiFactor authentication

Test cases should include:

- Verify two factor authentication for a specific user work without impacting
  other user authentication.
- Verify MFA enable/disable and bypass configurations.
- Verify Generate google-authenticator secret key flow.
- Redfish session create/login tests with or without 'Token' parameter.
- Verify MFA configuration persistency
