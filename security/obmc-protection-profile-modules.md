# OpenBMC Protection Profile Modules

This document defines BMC security features that extend
the base [OpenBMC Protection Profile](obmc-protection-profile.md).

These modules are (mostly) independent so that once you satisfy the
requirements in the base BMC Protection Profile, you can
can mix-and-match the security functions listed here.
Some of these may be incorporated into the OpenBMC Protection Profile.

This document follows the Common Criteria model
but does not adhere to its formality.
In particular, each module described here
further develops the security problem,
articulates the security threats, and
provides new objectives and requirements.

These are NOT requirements for the OpenBMC team.  This is meant as a
place to record ideas.

Please note that these may be in various stages of development.  Some
may need considerable design and implementation work.

Example of how the information in this document can used is:
 - record ideas of what security features to provide
 - basis for making a formal security claim


BMC Protection Profile Modules
==============================

This section describes each security module.
Each module describes the following:
 - its name
 - requirements for the BMC or other modules listed here
 - the enhanced security problem
 - the new threats
 - the new security objectives
 - the new security requirements


------------------------------------------------------------------------

Name: Multiple Users

Requires:
This requires a BMC that supports mutiple userids,
each of which can have its own unique password.
For example, the useradd command on a Linux system.

Security Problem:
This solves several security problems.
The first is that it allow you to identify individual users.
(Most organizations have a "no sharing passwords" policy.)
Second, it allows you to remove access from one user while
keeping access for another user.
Third, it allow you to think about having user-based access controls.

Security Threats:
 - The BMC administrator will have no idea
   who has access to the BMC system.
 - The BMC admin can be locked out of the system if one of the
   users changes the password.

Security Objectives:
The BMC shall allow the administrator to create, modify, and delete
multiple userids each of which can have an unique password.

Security Requirements:
tbd


------------------------------------------------------------------------

Name: User-level Authorization

Requires:
This requires a BMC that implements the Multiple Users module.

Security Problem:
We need to lock down user access, and
not give users more access then they need.

Security Threats:
 - The BMC admin can be locked out of the system if one of the
   users changes the password.

Security Objective:
The BMC shall allow the administrator to set or change
the authorization level for each user.
There shall be at minumum three categories:
 - read only - allowed to read all data on the system
 - operator - same as read only, plus
   allowed to operate all controls on the system
 - admin - same as operator, plus
   allowed to create and delete users and change their access level

TO DO: Roles should be a requirement separate from user ids so that in
cases where user ids are not implemented we can still have a reasoned
discussion about roles.  For example, AAA services grants a user (not
in BMC scope) a signed token allowing operator access to a BMC. The
lifetime of the token is defined by operations policy. All of the
entities involved are automated.

OpenBMC should offer a role-based authorization model like [RedFish](redfish.dmtf.org/schemas/DSP0266_1.1.html).
See CC part 2, "FDP User data protection."

Security Requirements: tbd


------------------------------------------------------------------------

Name: Secure internals

Requires:
This assumes the BMC is built from some kind of internal platform,
for example, a Linux-based system.

Security problem:
Various agents may have acccess to the internal platform,
for example, access to the Linux shell.
Such agents may include the administrator and
agents perfoming service on the BMC itself.
They may have too much power and
if they mistype something the BMC may become damaged.

For example, root access should not be used which will require use of
the sudo command to perform commands that affect the system
configuration.  For another example, having a read-only file system
will prevent accidental erasure of files.

Security Threats:
 - The BMC may become damaged if an authorized agent is not careful.

Security Objective:
The BMC shall protect access to its internal controls.

Examples of internal controls are:
 - The /etc file system mounted as read only
 - access to sudo

Example of things it can do are:
 - restrict write access to files
 - disallow signing on by the root user
 - only give sudo access to administrators
   from administrators.

Security Requirements: tbd


------------------------------------------------------------------------

Name: Auditing

Requires: User-level Authorization

Security Problem:
The administrator is required to understand changes to
the BMC and its payload device,
and to limit access to the information stored on the BMC.

Security Threats:
 - undetected changes to the BMC or its payload device
 - undetected and undesired disclosure of information

Security Objective:
The BMC shall keep a record (an audit log) of access to the
system which shall include at least the following:
 - the time of access
 - the user who accessed the information
 - the nature of the information they accessed, for example,
   part inventory, failure data, sensor data
 - if the access was allowed or disallowed

The audit log shall be viewable only those users who have access
to read the logs (typically the admin).
Likewise, specific authority is needed to modify or delete the log.

Security Requirements: tbd

------------------------------------------------------------------------

Name: Secure BMC boot

Requires: BMC hardware has support for secure boot

Security problem:
If the BMC firmware is tampered with, its security function may not
operate correctly.

Security threats:
Altered BMC firmware may allow unauthorized access.

Security Objective:
The BMC should have a way to be configured to require a secure boot.
For example, by setting a jumper on GPIO pins.  When configured for
secure boot, the BMC should have a way to check the integrity of its
firmware image.  For example, by using a digital signature.  If this
check fails, the BMC must not function with full service.  It could
report the error and shut down, or function with limited privileges
such as not interacting with its host server or disclosing its stored
data, but it can reinstall its firmware and reboot itself.

TO DO: concerns??

Security Requirements: tbd

------------------------------------------------------------------------

Name: Secure BMC firmware images

Requires: BMC has updatable firmware

Security Problem:
An attacker may tamper with the firmware image destined for the BMC.
Tampering may result in them gaining control of the device.

Security Threats:
 - undetected changes to the firmware of the BMC
   may allow an attacker to gain control of the device

Security Objective:

The BMC shall have a way for the administrator to configure the BMC to
require secure BMC firmware.  When enabled, the BMC will authenticates
its firmware as part of a request to update its firmware.  Examples of
authentication include use of digital signatures.  If the firmware is
not authentic, the BMC shall not perform the update.  Only the
administrator shall be able to determine the criteria for which
firmware images are authentic.

Security Requirements: tbd


------------------------------------------------------------------------

Name: Secure payload host firmware images

Requires: payload device (typically a host computer) has updatable
firmware

Security Problem:
An attacker may tamper with the firmware image destined for the
payload device.  Tampering may result in them gaining control of the
devices.

TO DO: Reconcile this with secure and trusted boot ideas.

Security Threats:
 - undetected changes to the firmware of the payload device
   may allow an attacker to gain control of the device

Security Objective:
The BMC shall have a way for the administrator to configure the BMC to
require secure firmware for its payload host.  When enabled, the BMC
will authenticates they payload host's firmware as part of a request
to update its firmware.  Examples of authentication include use of
digital signatures.  If the firmware is not authentic, the BMC shall
not perform the update.  Only the administrator shall be able to
determine the criteria for which firmware images are authentic.

Security Requirements: tbd


------------------------------------------------------------------------

Name: Periodic firmware updates

Requires: Secure firmware images

Security Problem:
As firmware updates become available for the BMC and
the payload device (as applicable), they should be be applied
relatively more sooner to guard against
attacks that are based on the newly discoverd vulnerabilities.

Security Threats:
 - An attacker uses knowledge of a new vulnerability to launch
   an attack against the BMC before the BMC is secured against the attack.

Security Objective:
The BMC shall have a mechanism that can be enabled to
periodically check for and install secure firmware images
to get the the latest level.

Security Requirements: tbd


------------------------------------------------------------------------

Name: Advanced authentication (various topics)

Requires: n/a

Security Problem:
Some types of authentication is weaker than others.
For example, password protection is weak, and
certificate-based authentication is stronger.
For example, using ssh certificates instead of password.
Weak authentication may allow an attacker to gain access to the BMC.

Security Threats:
 - An attacked may break weak authentication protocols to gain access
   to the BMC to steal data or access its controls.

Security Objective:
The BMC shall allow the administrator a way to disallow weak forms
of authentication (such as passwords) and
require the use of stronger authentication
for all supported operator interfaces.

Security Requirements: tbd

TO DO: The implementation should allow provisioning a BMC prior to
deployment such that only permitted AAA mechanisms and policies can be
used.  The intention is to have a way to provision the BMC so its
authentication mechanisms cannot be changed after it is deployed.

------------------------------------------------------------------------

Name: Advanced transport level security

Requires: n/a

Security Problem:
The BMC is required to use transport level security (TLS)
for communications with the operator.
Example of TLS include the ssl, https, and ssh protocols.
However, some protocols are weaker than others.
If an attacker can break the protocol, they can, for example,
steal password and use that to gain access to the BMC.

Security Threats:
An attacked who has gained access to the BMC network can
crack a weaker TLS protocol, and use that ability to
take over a session, control the system, or get information.

Security Objective: The BMC shall have a way for a user with the
administrative role to configure all operator-facing interfaces to use
relatively stronger TLS protocols and not allow communication using
weaker protocols.  (TO DO: We may want to have a way to provision the
BMC so it's communication protocols cannot be changed.)

Security Requirements: tbd

------------------------------------------------------------------------

Name: Security between the BMC and payload host

Requires: n/a

Security Problem: In the case of bare-metal hosting in a data center,
the host may be considered hostile and have very restricted access to
the BMC, if any. TLS or other AAA solutions may be required, even on
the hardwired communications channel between host and BMC.

This speaks to the general question of trust between connected systems
and highlights the need for the BMC and host to be wary of each other,
understanding the basis for their mutual trust and limits on access.

Security Threats: The host computer gaining access to the BMC and into
the data center.

Security Objective:
The BMC shall not trust the payload host.

Security Requirements: tbd

------------------------------------------------------------------------

Name: Secure data at rest

Requires: n/a

Security Problem:
The BMC collects data from its payload device and stores it
on nonvolatile storage.
For example, on the flash device alongside its firmware.
Such data may includes information about
the payload device's configuration, events, or other operations.
If this data is not encrypted, it is subject to unwanted disclosure
if the BMC's firmware is copied or physically removed.

TO DO: adddress these questions: The entire BMC/Host lifecycle has to
be considered from manufacturing and test, provisioning, updates,
repairs, and decommissioning. In some of those phases, these
requirements will be able to provide solutions or guidance. However,
it's not practical to solve all of these problems in the scope of the
BMC. It seems that key management will be an issue in this example. In
the case of secure logging, one can encrypt the logs with the public
key of a log service; the BMC having no access to the private key. The
BMC should probably never have access to personally identifying
information. What mechanisms should be made available to operations
staff for use in their policies?

Security Threats:
A threat agent may gain access to the information if they get access
to the storage.
TO DO: Personnel will have access to the BMC for update and repairs.
We should ideally have a way to protect the data from disclosure to
them.

Security Objective:
The BMC shall encrypt the data it receives from the payload device.
TO DO: Determine exactly which data.
TO DO: Determine related security policies, such as:
Only the admins shall be able to determine the encryption used.
Only users with read access or better shall be able to decrypt the data.

Security Requirements: tbd

------------------------------------------------------------------------

Name: Secure decommisioning

Requires: n/a

Security Problem:
The BMC may be removed from its secure environment when it is
decommissioned, yet it may still have sensitive data.

Security Threats:
 - Disclosure of information.

Security Objective:
The BMC shall have a way to remove all data related to its operation
including all data from the payload host and all user credentials.
TO DO: Possibly as separate options.

Security Requirements: tbd

------------------------------------------------------------------------

Name: Self test

Requires: n/a

Security Problem:
The BMC may encounter an unexpected defect that causes its
security functions to malfunction which would
allow access by an unauthorized user.

Security Threats:
 - Malfunction causes loss of security function.

Security Objective:
The BMC shall perform a self-test for each of its major security functions.
The BMC shall disable its function if the test fails.

For example, when the REST API server starts running,
testing can attempt to
authenticate with an unknown user,
authenticate with a known user with bad credentials,
authorize an authenticated user to a function they don't have access to.

Security Requirements: tbd

------------------------------------------------------------------------

Name: AAA Management service

Requires: n/a

Security Problem:
This addresses several related problems:
 - The BMC device has limited storage, not suited to storing large
amounts of audit data.
 - Large data centers have hundreds of identical BMCs, and maintaining
the same user authentication data becomes a management problem.  For
example, consider removing a user's access from all BMCs when more
BMCs are non-functional.
 - The BMC should not store user authentication data on it.

Security Threats:
The problems are:
 - Audit data may be lost because of small buffer sizes on the BMC.
 - Access credentials may be unevenly distributed, either denying
access to authorized agents or approving access to agents whose access
was supposed to be removed.
 - Agents who gain access to the BMC (such as service personnel) may
be able to gain access to access credentials stored on the BMC.

Security Objective:

The BMC shall provide a AAA management service, which when enabled,
will provide Authentication, Authorization, and Accountability
services for its major security interfaces.  For example, http
requests, web applications, IPMI access, etc.

What that means for the BMC is the user's credentials and request are
passed to the AAA service which either allows or disallows access.
The AAA service itself is rooted in the BMC and would take pluggable
modules to provide the service.

A simple AAA service provider would accept linux passwords, check them
against an local passwords or an LDAP server, and if the password is
valid to always grant access.

A full AAA service provider would contact an AAA server on the
internal data center network.  That server would perform the AAA
function and return an answer to the BMC.

Security Requirements: tbd

------------------------------------------------------------------------

Name: Multiple BMCs cooperate to perform a task

Requires: Multiple BMCs on the same network

Security Problem:

This addresses security issues when two or more independent BMCs
cooperate to accomplish a task that necessarily requires multiple
BMCs.  For example, two BMCs can be used to move a virtual machine
(VM) from one host server to another host server.

From CC's perspective, the VM is "user data and the "operation" is the
BMCs transferring the VM between their host servers.  There are three
data flows:
 1. from the origin BMC's host server to the origin BMC,
 2. from the origin BMC to the destination BMC,
 3. from the destination BMC to the destination BMCs host server.

The security aspects of transferring the VM between the BMC are
addressed by CC part 2, FDP security family ("User data protection").
More specifically, the origin BMC is exporting user data and the
destination BMC is importing user data.  Many of the FDP following
security classes apply:
 - Access control policy (FDP_ACC).  Access control function
   (FDP_ACF).  Is the transfer allowed?
 - Data authentication (FDP_DAU).  Do we need to digitally sign the data?
 - Export from the BMC (FDP_ETC).
 - Import from outside of the BMC (FDP_ITC).
 - Residual data protection (FDP_RIP).  The BMC should erase the VM
   data when done.
 - Rollback (FDP_ROL).  The BMCs should rollback the operation, for
   example, when the VM cannot be transferred successfully because the
   destination host server's hypervisor is not functioning.
 - Inter-BMC data confidentiality transfer protection (FDP_UCT). Does
   the VM need to be protected from unauthorized disclosure, for example,
   by being encrypted?

TO DO: Describe security aspects of flows 1 and 3 (between BMC and its
host server).

Security Threats:
 - transferring the VM to a hostile BMC
 - losing the VM or duplicating it (like a failed rollback)
 - disclosing the content of the VM either on the network or as
   residual data on the BMC

Security Objective:

Security Requirements:

