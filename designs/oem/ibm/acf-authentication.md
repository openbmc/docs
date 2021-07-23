# IBM ACF authentication

Author: Joseph Reynolds <jrey@linux.ibm.com>

Primary assignee: Joseph Reynolds <jrey@linux.ibm.com>

Other contributors: None

Created: July 23, 2021

## Problem description

This presents a novel authentication problem: how to authenticate to the BMC
via password, but only when the BMC manufacturer, BMC administrator, and BMC
user all agree.  The intended use case is for a service agent authorized by
the manufacturer to access the BMC, but only when the BMC admin allows it.
The service agent is allowed to use BMC functions which the BMC admin must not
be allowed to access.  See the background and references section for details.

Use of this design is optional to support a specific use case:
- This design shall not affect any OpenBMC implementation which does not
  explicitly opt-in.
- Use of this design is intended to be used for specific users and is not
  intended to replace the BMC's existing authentication mechanisms for other
  users.

This design describes only the core authentication functions.  A "service"
account is mentioned, but it is not part of this design.

## Background and Reference

This background needed to understand the design is given in subsections:
- A brief overview of OpenBMC's existing authentication functions.
- A trust model for IBM enterprise class servers, its users, and service
  agents.
- The operating environment in which some systems are installed.
- General trust considerations for manufacturing agents accessing customer
  systems.

Use of the term "ACF" (access control file) pre-dates this design.  It is
defined in the "proposed design" section below.

### Existing authentication functions

OpenBMC provides user interfaces including HTTPS (including Redfish and REST
APIs) and SSH which support password-based authentication and use Linux-PAM as
the underlying mechanism.  When the BMC has local users, the `pam_unix.so`
Linux-PAM module is used to check the entered password with the BMC's
`/etc/shadow` file.  In contrast, this ACF design creates a new Linux-PAM
authentication module and a new BMC file to check the password against.

Note that OpenBMC provides other interfaces which use password-based
authentication (such as IPMI) and other authentication methods (such as mTLS
and IPMI-based authentication).  The only intended limitation for ACF
authentication is it cannot be used with IPMI or with non-password-based
authentication.

To illustrate one possible BMC configuration, it is possible to have different
authentication methods for different users:
- The admin user uses local password-based authentication.
- The service user uses ACF authentication.
- Other users use LDAP authentication.

### Trust model

IBM enterprise class servers have a 3-way trust relationship.  The actors are:
1. The manufacturer (as the firmware provider).  Delivers the system to its
   owner together with a service warranty.
2. The system owner (as the BMC administrator).  Takes delivery of the server
   and operates it.  They may secure the system to process sensitive or
   personal information.  Service access may not be allowed when this data is
   in the server.
3. The manufacturer's service and support representative (SSR).  Provides
   service to the system when agreed by both the owner and the manufacturer.
   A service call may involve using BMC functions which are not available to
   the system owner.

In this model:
1. The manufacturer trusts the system owner to comply with the warranty
   agreement.  For example, it may be possible to overclock the processor to
   get more work from the system, but that may subject the system to premature
   wear or known conditions which would lead to service calls, part
   replacement, etc.
2. The manufacturer does not trust the system owner with access to functions
   intended only for manufacturing test, configuring manufacturing parameters,
   and providing access to system internals.  The admin account is not allowed
   to use these functions.
3. The system owner trusts the manufacturer to provide security features which
   can lock out the manufacturer's access to the system without the owner's
   cooperation.  This is required when the server has sensitive or personal
   information.
4. The system owner trusts the manufacturer's service organization to allow
   only authorized SSRs to generate credentials needed to access the BMC's
   service function.
5. The system owner trusts the SSR to perform service calls in the manner
   consistent with their training.

### Operating environment

Systems owners may operate their servers in environments which have physical
and electronic access restrictions.  Specifically:
- No internet.  The BMC may be connected to a network, but all data must be
  hand-carried to and from this network because there are no bridges to the
  internet.
- No electronics.  The SSR is not allowed to carry any electronics (including
  storage media) to the work site.

This may limit the available authentication techniques, including credentials
the SSR can present to the system, and may limit the BMC's access to
authentication servers.

### General trust considerations

Service access gives the SSR extraordinary access to the BMC's internal
functions and crosses security boundaries between the manufacturer and the
system owner.  This justifies extraordinary precautions when granting this
access.

The items relevant to the authentication steps are:
1. Only agents authorized by the service organization are allowed to create
   service access credentials (for example, by controlling access to the
   private key used to create the credentials).
2. Each set of credentials is valid only for the machine being serviced (via
   system serial number).
3. Each set of credentials is valid only for a specific time window.

## Requirements

The basic requirement is for an authentication function which satisfies all of
the restrictions described in the background and references section.
Specifically:
1. Requires the manufacturer's service organization to explicitly authorize
   each service access.
2. Requires the BMC administrator to take explicit action to allow the SSR
   access to their BMC.
3. Allows the SSR to use a password to authenticate to the BMC's HTTPS and SSH
   interfaces.

## Proposed Design

An access control file (ACF) tells the BMC to accept a service account login.
This file has details of the intended service access including system serial
number, access dates, and a secure hash of the password needed to login.  The
ACF is digitally signed by the service organization.  The overview presents a
high-level view and is followed by details in the sections below.

### Overview

The basic flow to get access to the BMC service account is:
1. The system owner calls for service and identifies the system serial number.
2. The SSR providing the system serial number as input to an ACF generator
   tool.  This server creates a random password and an ACF and gives the ACF
   and cleartext password to the SSR.
3. The SSR provides the ACF to the BMC admin who then uploads it to the BMC.
   The BMC validates the ACF including its digital signature (against the
   public key stored in BMC firmware), system serial number, access dates.
   Successfully uploading an ACF allows the SSR to log in to the service
   account.
4. The SSR uses the password associated with the ACF to log into the service
   account.  The BMC authenticates the ACF and the password.
5. The SSR performs service and logs out.
6. The BMC admin deletes the ACF from the BMC or allows it to expire.

BMC functions included in this design are:
1. Admin function to upload an ACF to the BMC.
2. Function to validate the ACF (including signature, system serial number,
   valid dates).
3. Function for password-based authentication using an ACF.
4. Admin function to delete the ACF from the BMC.

The service organization has additional infrastructure including:
1. An ACF generation tool.  This takes input from the SSR and outputs an ACF
   together with its cleartext password.
2. The manufacturer generates an ACF key pair.  The public key is stored in
   the BMC's firmware image; it is used to validate that ACF files were signed
   by the manufacturer.  The private key is used by the ACF generator to sign
   ACF files.

Service organization functions included in this design include tools to
generate an ACF.  See details in sections below.

The following diagram shows the above items in context.  Infrastructure
elements are at the top of the diagram.  They include:

- Generating the key pair.
- Putting the public key into the firmware image.
- Using the private key to sign ACFs.
- The ACF generator tool.

The BMC and its functions are the bottom of the diagram.  They include:
- The BMC firmware which contains the ACF public key
- The ACF upload and validation functions.
- The ACF authentication function for service account logins.
- The ACF delete function is omitted from the diagram.

```ascii
 ┌──────────────┐
 │Generate keys │
 └──┬──┬────────┘
    │  │private    ┌──────────┐
    │  └──────────►│ACF       │
    │public        │generator ◄── ACF request
    │              │tool      │
    │              └──┬──┬────┘
    │                 │  │
    │                 ▼  ▼
    │               ACF  password
    │                 │  │
  ┌─▼──────┐          │  │
  │firmware│          │  │
 ┌┴────────┴────────┐ │  │
 │BMC               │ │  │
 │     admin        ◄─┘  │
 │       upload ACF │    │
 │     service      ◄────┘
 │       login via  │
 │       pam_ibmacf │
 └──────────────────┘
```

Implicit to make this design work is a special "service" account pre-created
on the BMC.
- The service account has all the authority needed for the SSR to perform
  their work.
- Authentication to the service account is via password which is validated
  with an ACF (see below).  If a valid ACF is not present, the service user
  cannot authenticate.
- The admin shall be locked out of the service account so they cannot escalate
  their privileges such as by changing the service account password or
  configuring an LDAP service account.

Providing a service account is outside the scope of this design.

### ACF details

An access control file (ACF) controls the service user's access to the system.
- An ACF identifies:
   - The system (by serial number).
   - The expiration date.
   - The password needed to authenticate (hashed via PBKDF2 sha512 at 100000
     iterations, with 128 bit random salt).
- The ACF is digitally signed by the service organization and the signature is
  validated by the BMC.
- For each service call, the service agent generates the ACF and provides it
  to the BMC administrator who then uploads it to the BMC.
- Successfully uploading a valid ACF to the BMC allows password-based
  authentication to the service account. Deleting the ACF from the BMC
  disables service access.
- The ACF together with its password forms a two a part authentication scheme:
   - The ACF itself has no secrets (outside of the password hash) and is readable
     by the BMC administrator.
  - The password which goes with the ACF.

An ACF is an ASN.1 file which contains a JSON file together with a digital
signature over the JSON data.

The JSON file has:
- The machine type of the intended system.
- The serial number of the intended system.
- The password hash.
- The password hash salt.
- The expiration date of this ACF.
- The ACF request ID and serial number.

### Security functions

There are three primary functions (described in the following sections):
1. Tool to generate an ACF.
2. Library function to validate an ACF on a BMC.
3. Linux-PAM module to validate a password-based service user login against an ACF.

#### ACF generation tool

The tool to generate an ACF has two parts: a back end which produces the ACF,
and a production interface.  This design describes only the back end.  The
production interface is described only to illustrate the design.

This tool is intended to be used on a machine secured by the service
organization.  It is not intended to run on a BMC.

The back end:
- Direct use of the back end is intended for testing purposes and for use by
  the production interface.
- Takes four parameters: machine serial number, expiration date, password, and
  private key.
- Algorithm:
   - Hashes the password.
   - Creates the ACF from the hashed password, serial number, and expiration date.
   - Digitally signs the ACF using the private key.
   - Returns the ACF to the caller.

The ACF is now ready to be uploaded to the BMC.

The production interface is invoked directly by the SSRs:
- Takes parameters: machine serial number
- Generates a cryptographically random password.
- Sets an expiration date (such as midnight of the next day).
- Invokes the backend ACF generator tool (including serial number, generated
  parameters, and private key for signing).
- Returns the ACF together with its password to the caller.
- Additionally, it stores the ACF request for auditing purposes.

There should be strong access and audit controls on use of this interface
because it creates production-signed ACFs.

#### ACF validation

The library function to validate an ACF first checks the digital signature and
then checks fields such as the system serial number and expiration date.  It
does not check the password: see the authentication function for that.  One
intended use is by the BMC function to upload the ACF.

The function to validate an ACF has the following use cases:
1. The function for the BMC administrator to upload an ACF must first validate
   the ACF before accepting it.  Invalid ACFs shall be rejected.
2. The function to display an ACF shall check the digital signature, machine
   serial number, and expiration date.
3. The function to authenticate the service user's ACF password must first
   validate the ACF before checking the password.  Invalid ACFs shall result
   in authentication failure.

#### ACF password authentication

A new Linux-PAM module pam_ibmacf.so handles PAM-related functions which run
on the BMC.  It handles only specific users and ignores others.  This module:
- Performs ACF authentication (including validate the ACF and checking the
  password)
- Handles other PAM functions such as rejecting all password change requests
  (again, only for the service user).

Configuring the new module into the Linux-PAM stack for use with a "service"
account is outside the scope of this design.

## Alternatives considered

Other two factor authentication (2FA) schemes were considered.  These were not
suitable in all server operating environments (see section above).
Specifically:
1. The second factor (such as the password) were longer than 30 characters or
   require electronic delivery.
2. The BMC reaches out to the network as part of the authentication scheme.

## Impacts

Only firmware versions which explicitly configure this feature are impacted.
This feature increases the BMC's attack surface, specifically a new ACF upload
function and new authentication function.

## Testing

Set up a BMC with a service user configured to use ACF authentication.

Test the authentication good path:
- Create an ACF and upload it to the BMC.
- Login the service user.

Test authentication bad paths:
1. Attempt to login the service user when there is no ACF present.
2. Attempt to login the service user with a bad password.
3. Ensure attempts to login to the "service" account via LDAP authentication
are ineffective.

Test bad paths:
1. Ensure attempts to change the service user's password always fail.
2. Attempt to upload an invalid ACF.  Variations include bad ACF file format
   (in various ways), bad signature, incorrect system serial number,
   expiration date in the past.
