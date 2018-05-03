# Strawman BMC Security Protection Profile

Authors: OpenBMC project - contact IRC room #openbmc_security

Version: 2018-05-03

Acknowledgements:
Many ideas were taken from NIAP approved Protection Profiles:
 - Protection Profile for Hardcopy Devices
   (https://www.niap-ccevs.org/MMO/PP/pp_hcd_v1.0.pdf)
 - BIOS Update for PC Client Devices Protection Profile
   (https://www.niap-ccevs.org/MMO/PP/pp_bios_v1.0.pdf)
 - Protection Profile for General Purpose Operating Systems
   (https://www.niap-ccevs.org/MMO/PP/-400-/)


########################################################################
## Table of Contents
########################################################################

 - Protection Profile Introduction
   - Purpose
   - Identification of the Protection Profile
   - Overview of the BMC
   - Target of Evaluation
   - Operational Environment
   - Security Use Cases
   - Major Security Functions
 - Security Problem Definition
   - Users
   - Assets
   - Threats
   - Organizational Security Policies
   - Assumptions
 - Security Objectives
   - Objectives for the BMC device
   - Objectives for the BMC's Operational Environment
 - Security Functional Requirements


########################################################################
## Protection Profile Introduction
########################################################################

This is a straw-man draft of a security Protection Profile
for a baseboard management controller (BMC).
It was developed specifically for the OpenBMC project,
https://github.com/openbmc,
but is intended to be generally applicable to any BMC.

Readers are assumed to have basic knowledge of computer operating systems
with no specialized knowledge of computer server implementation or
information security concepts.


### Purpose

The purpose of this straw-man protection profile is for
IT developers, vendors, and the security community
to understand the information security problems faced by the BMC.
It is expected for this protection profile to evolve
with the intention of becoming NIAP approved.


### Identification of the Protection Profile

This protection profile is incomplete and should not be used.
It does yet not claim conformance to the Common Criteria.


### Overview of the BMC

A baseboard management controller (BMC) is
also known as a service processor.
It is a small computer physically close to and
dedicated to servicing a larger computer (the "payload system")
[??? use the term "payload device"???]
in tasks such as powering up and monitoring critical elements
like overheating and over voltage.
It gives an operator the ability to manage the payload system
by performing tasks like
loading the BIOS, rebooting, and
getting error logs needed for service calls.
The BMC typically provides management access in the form of
web applications, REST APIs, and supports the IPMI protocol.

The BMC is typically a system on a chip (SoC) together with its firmware.
A BMC can be implemented by a general-purpose operating system
such as embedded Linux pre-loaded with
specialized applications that communicate with either
the payload system or the system operator.
The physical hardware typically includes an ethernet connection
so the operator can access the BMC over the network.
The physical hardware also typically includes pins
to communicate with the payload host
using specialized protocols such as IPMI over i2c.
The BMC's firmware is typically on a flash storage device
which may or may not be removable,
but can be updated via BMC operations.

The BMC hardware and its firmware
are generally deployed and operate together with
the payload server hardware and its firmware (aka BIOS).
However, they can be updated independently.


### Target of Evaluation

This section defines the hardware and software boundaries of the BMC
for purposes of defining its security profile.
The boundaries are critical to understand
the exposed surface available to attackers.
This is referred to as the Target of Evaluation (TOE)
by the information security community.

The TOE consists of the BMC's physical hardware
together with its firmware.
Note that emulated BMCs (such as qemu)
may be useful for development and testing,
but are not useful for servicing a payload system,
so they are outside the scope of this document.
The concept of virtualized BMCs is outside the scope of this document.

The TOE boundary includes the following:
 - Pins exposed to the payload host system;
   this includes IPMI commands from the payload system's BIOS.
 - The attached console.
 - The ethernet jack, including Internet Protocol traffic
   using protocols such as tcp, http, and ipmi.

The TOE boundary does not include
access to any platform underlying the BMC.
For example, if the BMC is implemented as a Linux system,
he TOE does not include access via ssh.
[We'll have to state that ssh is disabled in compliant implementations?
Otherwise, we'll have the protection profile of an operating system,
which we don't want.  How do we service the BMC itself???]

The hardware platform on which the BMC runs
is outside the scope of this document.  (??? or is it?)


### Operational Environment

The operational environment of the BMC is
typically in a physically secure area,
connected to an internal monitored network,
and connected to a physically close payload device.
Note that the payload device can be connected to the internet,
but that is outside the scope of this document.


### Security Use Cases

The use cases are with the BMC's physical hardware with its firmware
located close to the payload server system.
An unsupported use case is
the BMC firmware running on an emulator,
which may be useful to demonstrate some aspects of security compliance.
Specifically, you can use an emulator (such as qemu)
to perform security tests provided you explain why it is valid to do so.

Note:
In this document the term "administrator" refers to
the agent who authorizes other users to the BMC.
The term "operator" or "user" refers to an agent who
is authorized (by an administrator) to use the BMC's controls.
An agent can typically be a person or software
accessing the BMC through its network interfaces.
Neither role would typically have access to the internals of the BMC
(such as ssh access or the BMC root password).
[Who does have such access?  BMC service agent???  terminology to use???]

The use cases are:
 - An operator uses BMC interfaces
   such as the web app, REST API, or IMPI
   to perform a system administration function
   [should each interface be separated here???]
   to get information about the payload host system such as
   event logs and hardware configuration,
   to update the payload host system configuration, or
   to update the BMC itself
 - The system administrator uses BMC interfaces to
   authorize users to specific BMC functions
 - More???


### Major Security Functions

The BMC provides the following security functions:
 - Identification, authentication, and authorization to use the BMC function
 - Access control - controls who can access BMC function
 - Transport Level Security - encrypts network traffic
 - Administrative roles - controls who is authorized to specific BMC function
 - Service roles - controls who is authorized to service the BMC itself
 - Auditing - logs access to the BMC function

Note that access to the BMC may imply some access to the payload system
because the BMC has functions to control the payload system
(for example, power on and off) and get data from it
(for example, hardware configuration and operational performance data).
To clarify, the BMC has some access to
the payload system's hardware and firmware,
but does not have normally have access to the device's function, such as
the operation of the processors or any software the payload system is running.

The BMC does not encrypt data at rest on the BMC device.


########################################################################
## Security Problem Definition
########################################################################

The primary asset to be protected is the payload host system.
Specific assets for the OpenPower servers include:
the payload system's firmware,
the firmware's operational data (e.g., SELs, GARD data)
which is stored alongside the firmware
on nonvolatile storage or sent to the BMC,
access to the payload host's circuitry.

The secondary asset to be protected is the BMC device
which services the payload system and has direct access
to the payload system's firmware and
control of the payload host circuits.
Example:
If the BMC is compromised, an attacker may be able to
gain full control of the payload device by altering its firmware.


### Users

There are two roles: Administrator and Operator.
Both are identified (typically by username) and authenticated.
The administrator role determines which user can access which assets.


### Assets

The assets to be protected are in these categories:
 - The BMC
 - The BMC's functions
 - Payload host configuration data.  For example, the number of processor cores.
 - Payload host operational data.  For example, how fast the fans are running.
 - Payload host event logs.  For example, when a fan fails.
 - Payload host operational controls.  Do we need sub-categories???:
   - Powering off the payload host
   - Updating the host's firmware
   - Modifying operational controls, e.g., running fans at maximum speed
   - Modifying operational parameters, e.g., temperature/fan limits
   - More???
 - More???  [This area needs work.]


### Threats

This section lists threats to the BMC's security.

Security threats to the BMC come from
physical attacks,
network based attacks, and
attacks from the payload server.

Unauthorized access to the BMC's underlying implementation.
For example, for BMC's implemented as imbedded Linux systems,
attempting to gain root access.

Unauthorized physical access to the BMC.
For example, replacing its firmware or
electronically listening to the physical pins connected to the payload host.

Unauthorized access to the BMC's data or payload system controls.
Access to the BMC's data could be used for
a side channel attack of the payload host system.

Access to the BMC's controls of the payload host system
can be used to disable the host (for example, power it off) or
could be used to damage the host's hardware.

Attackers can try to use the BMC's network interface
to gain unauthorized access to it.


### Organizational Security Policies

The BMC should be installed and managed by an organization
that adheres to these policies.

The administrator must only allow trained users
to access the BMC's function.

Users must protect their credentials.

The network the BMC uses must be controlled and managed.

Only digitally signed firmware images may be used for both the BMC and the payload host firmware.


### Assumptions

This section lists assumptions made in identifying the security threats.

Physical protection.
The BMC and its attached console is assumed to
be in a physically protected space.
Threats from physically tampering with the firmware,
snooping electronically, etc.
are not considered.

Limited functionality.
The BMC is assumed to be limited to its designed function.
For example, if someone configures it to start an unrelated web server,
those threats are not considered.

Trusted administrator.
The person trusted to administer the BMC is
expected to follow all security procedures.
For example, if they direct the BMC to visit a malicious website,
those threats are not considered.

Admin credentials are secure.
The administrator's credentials are expected to be secured.

Trusted hardware.
The hardware that implements the BMC is expected
to be secure and obtained in a secure manner.

Trusted open source components, obtained securely,
and updated with regular security updates.  ???


########################################################################
## Security Objectives
########################################################################

This section defines the security objectives for
the BMC and its operational environment.
These objectives form the basis for each of the security requirements.

to do -- this entire section needs work.
It need to reference external security standards.
It needs to define codes for the security requirements to reference.


### Objectives for the BMC device

User authorization.
The BMC shall perform authorization of users
in accordance with security policies.

User Identification and Authentication.
The BMC shall identify and authenticate users for each interface it supports.
For example, direct login, the web application, the REST API, IPMI, etc.

Access control.
The BMC shall only allow users access to the functions
for which they are authorized.

Administrator roles.
The BMC shall only allow administrators to perform administrator functions.
Specifically, to authorize other users to access BMC functions.

Firmware update.
The BMC shall provide a way to verify that
firmware updates for itself and for the payload system are authentic,
typically using digital signatures.

Self test.
The BMC shall perform a self-test, at least every time it is powered on,
to detect errors in its security function.

Network Communications.
The BMC shall have a mechanism to prevent
unauthorized access, replay, and spoofing.
For example, transport level security prevents unauthorized access.

Auditing.
The BMC shall keep audit records of access to its interfaces.
It may keep more detailed records of what functions were invoked.

Protect data at rest.
The BMC encrypt the data it collects from the payload host
when stored on nonvolatile media.
For example, on flash stroage.
For BMC that service computer systems, this typicvally applies to
part inventory (such as FRUs) and event logs (such as SELs), etc.


### Objectives for the BMC's Operational Environment

Physical protection.
The BMC should be physically accessible only to
trained and trusted data center personnel.

Network protection.
The BMC shall be on an isolated private network
that is monitored for threats.

Trusted administrator and users.
The BMC administrator and users shall be
trained, trusted to follow all security policies, and not be malevolent.


########################################################################
## Security Requirements
########################################################################

To do -- after the security objectives are better defined.

This section talks about all the supported interfaces and
all kinds of things you can do with them.
It states the BMC's security requirements and
shows how they address the security objectives.

For example, using the web application, REST APIs, and IPMI
you can update the firmware, get system data, and control system operation.
The security requirements of each item are addressed in detail.


### Security Functional Requirements

to do

This should be organized as a matrix.
List the major function (like web app, IPMI, REST API)
and go over the security requirements for each.

An item would be how to identify, authenticate, and authorize a user
using the REST API to attempting to upload a new host system PNOR.
What are the requirements?
What about transport level security?
Auditing?


### Security Assurance Requirements

To do

