# OpenBMC Protection Profile Modules

This document defines BMC security features that extend
the base [OpenBMC Protection Profile](obmc-protection-profile.md).

These modules are (mostly) independent so that once you satisfy the 
requirements in the base BMC Protection Profile, you can
can mix-and-match the security functions listed here.
Some of these may be incorporated into the base BMC Protection Profile.

This document follows the Common Criteria model
but does not adhere to its formality.
In particular, each module described here
further develops the security problem, 
articulates the security threats, and
provides new objectives and requirements.

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

Security Threats:
 - The BMC may become damaged if an authorized agent is not careful.

Security Objective:
The BMC shall protect access to its internal controls.

Examples of internal controls are:
 - read/write vs read permission on a file system
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

The audit log shall be viewable by the admin.
Only the admin shall be able to modify or delete the log.

Security Requirements: tbd

------------------------------------------------------------------------

Name: Secure firmware updates

Requires: BMC or payload host has updatable firmware

Security Problem:
An attacker may tamper with the firmware image destined for 
either the BMC or its payload host.
Tampering may result in them gaining control of the devices.

Security Threats:
 - undetected changes to the firmware of the BMC or its payload host
   may allow an attacker to gain control of the device

Security Objective:
The BMC shall authenticate the firmware for itself and
its payload host (as applicable)
as part of a request to update the firmware.
Examples of authentication include use of digital signatures.
If the firmware is not authentic, the BMC shall not perform the update.
Only the administrator shall be able to determine the criteria
for which firmware images are authentic

Security Requirements: tbd


------------------------------------------------------------------------

Name: Periodic firmware updates

Requires: Secure firmware images

Security Problem:
As firmware updates become available for the BMC and
the payload host (as applicable), they should be be applied
relatively more sooner to guard against
attacks that are based on the newly discoverd vulnerability.

Security Threats:
 - An attacker uses knowledge of a new vulnerability to launch
   an attack against the BMC before the BMC is secured against the attack.

Security Objective:
The BMC shall periodically check for and install secure firmware images
to get the the latest level.

Security Requirements: tbd


------------------------------------------------------------------------

Name: Advanced authentication

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

Note that this is not a problem for
communication between the BMC and its payload host.
Such communication uses private channels,
for example, wires on the motherboard.
Such comunicatin is not required to use TLS and
may send signals with no encryption.

Security Threats:
An attacked who has gained access to the BMC network can
crack a weaker TLS protocol, and use that ability to
take over a session, control the system, or get information.

Security Objective:
The BMC shall require all operator-facing interfaces
to use relatively strong TLS protocols
and not allow communication using weaker protocols.

Security Requirements: tbd


------------------------------------------------------------------------

Name: Secure data at rest

Requires: n/a

Security Problem:
The BMC collects data from its payload device and stores it
on nonvolatile storage.
For example, on the flash drive alongside its firmware.
Such data may includes information about
the payload device's configuration, events, or other operations.
If this data is not encrypted, it is subject to unwanted disclosure
if the BMC's firmware is copied or physically removed.

Security Threats:
A threat agent may gain access to the information if they get access
to the storage.

Security Objective:
The BMC shall encrypt the data it receives from the payload device.
(Maybe it should encrypt some of its operationl data too?  Logs?)
Only the admins shall be able to determine the encryption used.
Only users with read access or better shall be able to decrypt the data.

Security Requirements: tbd

------------------------------------------------------------------------

Name: Self test

Requires: n/a

Security Problem:
The BMC may encounter an unexpected defect that causes its
security functions to malfunction which would
allowing access by an unauthorized user.

Security Threats:
 - Malfunction causes loss of security

Security Objective:
The BMC shall perform a self-test for each of its major security functions.
The BMC shall disable the function if the test fails.

For example, when the REST API server starts running,
testing can attempt to
authenticate with an unknown user,
authenticate with a known user with bad credentials,
authorize an authenticated user to a function they don't have access to.


Security Requirements: tbd

------------------------------------------------------------------------
