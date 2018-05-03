# Strawman BMC Security Protection Profile

Authors: OpenBMC project - https://www.github.com/openbmc

Contact: IRC room #openbmc - jrey@us.ibm.com

Version: 2018-05-07

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
to understand the information security problems faced by the BMC
and what can be done about them.
It is expected for this protection profile to evolve
with the hope of becoming NIAP approved.


### Identification of the Protection Profile

This protection profile is incomplete and should not be used.
This does yet not claim conformance to the Common Criteria.

When ready, references to this protection profile 
should refer to the "BMC Protection Profile"
along with its version.


### Overview of the BMC

A baseboard management controller (BMC) is a specialized service processr 
physically connected and dedicated to serving a device
such as a powerful computer system.
For purposes of this document, we'll use a broad definition of a BMC
and refer to a "payload device".

A BMC has two required attributes.
1. The BMC gives an operator the ability to
   monitor and control the payload device.
   For example, powering it on
   monitoring internal temperatures,
   loading the BIOS, rebooting, and
   getting error logs needed for service calls.
2. The BMC performs critical tasks for the payload device.
   For example, the BMC powers on the payload device
   in the correct sequence, and
   cuts power to the payload device when it detects
   an over voltage condition that threatens to damage the device.

The BMC typically provides management access in the form of
web applications, REST APIs, and supports the IPMI protocol
(???reference needed).

The BMC is typically implemented as
a system on a chip (SoC) together with its firmware.
A BMC can be implemented by a general-purpose operating system
such as embedded Linux pre-loaded with
specialized applications that communicate with
the payload device or the operator.
The physical hardware typically includes an ethernet connection
so the operator can access the BMC over the network.
The physical hardware also typically includes pins
to communicate with the payload device
using specialized protocols such as IPMI over i2c.
The BMC's firmware is typically on a flash storage device
which may or may not be removable,
but can be updated via BMC operations.

The BMC hardware and its firmware
are generally deployed and operate together with
the payload device and its firmware (aka BIOS).
However, they can be updated independently.


### Target of Evaluation

This section defines the hardware and software boundaries of the BMC
for purposes of defining its security profile.
The boundaries are critical to understand
the exposed surface available to attackers.
The boundary or components inside the boundary
are referred to as the Target of Evaluation (TOE)
by the information security community.

The BMC's TOE consists of its physical hardware
together with its firmware.

Note that emulated BMCs (such as qemu)
may be useful for development and testing,
but are not useful for servicing a payload system,
so they are outside the scope of this document.
The concept of virtualized BMCs is outside the scope of this document.

The TOE boundary includes the following:
 - The BMC's firmware, including any internal or external connectors.
 - Any pins exposed to the payload device used for communication.
   For example, server hosts send IPMI commands to the BMC.
 - The attached console.
 - The ethernet jack, including Internet Protocol traffic.
   using protocols such as tcp, http, and ipmi.
 - Any Universal Serial Bus (USB) connections connected to the BMC.
 - Any additional communication channels such as a phone jack
   or communication lines an operator can use.

The TOE boundary must include at least the following items:
firmware,
communication with the payload device,
and communication with an operator.

The TOE boundary does not necessarily include
access to any platform underlying the BMC.
For example, for BMCs implemented as a Linux system with IP access,
the TOE includes TCP port 22 ("the SSH server"), but
operational procedures can specify that
the ssh server not be configured.
In such configurations, there must be some other way for
the operator to monitor and control the BMC which is part of the TOE.

The hardware platform on which the BMC runs is part of the TOE.
For example, a BMC may run on the AST2500 system on a chip.
Compliant configurations will have to identify the hardware and
independently assure its security requirements are met.
However, that is outside the scope of this document.


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

This section defines security problems addressed by the BMC.
Every security problem addressed here becomes an input to the 
security objectives which are defined in the next section.

The BMC faces additional security problems not address here;
these may be addressed by BMC Protection Profile Modules, but
that is outside the scope of this document.
  ???????????????????????????????????????? list here ????????????????????

Each security problem defines security threats in terms of
agents (such as authorized users and attackers), 
assets to be protected (such as data and controls), and
bad things that can happen.

Some would say there is no security problem because
the BMC is expected to be in a physically controlled data center and
connected only to its secure internal network.
To negate this idea, let's assume that an agent has penetrated the
data center's firewall and gained access to its secure internal network.


### Users

There are two roles: Administrator and Operator.
Both are identified (typically by username) and authenticated.
The administrator role determines which user can access which assets.


### Assets

The primary asset to be protected is the payload device.
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

Security threats to the BMC come from three categories:
physical attacks,
network based attacks, and
attacks from the payload device.

There are no known threats to the BMC from the payload device.
However, an attacker can rent a "cloud computing" service
which they use to gain control over the server, then
use the server to attack that system's BIOS, and finally
use that to attack the BMC.
The BMC does have a two-way communication channel with the BMC, 
so it is an attack vector.

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
That training must include maintaining secure passwords.
And yes, the admin must change the default password.

Users must protect their credentials from disclosure.

The network the BMC uses must be controlled and managed.
That is, the network is responsible for any threats based on
uncontrolled access the BMC.
BMC is not expected to withstand sustained attacks of such kind.

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

This section defines the security objectives
for the BMC and its operational environment.
These objectives state how the BMC itends to protect itself
from the security threats which were identified in the previous section.
Some of these relate to the BMC itself, 
and others relate to the environment in which the BMC operates.
In total, they must address all of the threats.

The objectives for the BMC itself form the basis
for the security requirements which are given in the next section.

The objectives for the environment in which the BMC operates
are an important part of how the BMC is protected,
but do not form the basis for any BMC security requirements.
Instead, they form the basis for operational procedures
that BMC installers and operators must follow.
Such procedures are outside the scope of this document.
When the BMC is evaluated against these standards,
it is assumed that all such objectives are satisfied.

to do -- the next two subsections need work.
It needs to reference external security standards.
It needs to define codes for the security requirements to reference.


### Objectives for the BMC device

The primary security objectives for the BMC are
to protect itself against use by unauthenticated agents
and to use transport level security measures to guard against
disclosing information to unauthorized agents.
In computer speak, all interfaces must
require passwords or something stronger, and
all IP traffic must be secured with something like https or ssl.

In this protection profile, the following are specifically not
security objectives.
A BMC may provide features for these non-objectives
and still be compliant with this protection profile,
but such features are not required.
The intent is to provide various BMC Protection Profile Modules
to address these objectives,
but that is outside the scope of this document.

 - Not required: Multiple users.
   The BMC is not required offer the administrator a way to
   create credentials for another user.
   That is, it may offer only one userid.
 
 - Not required: User-level authorization 
   The BMC is not required to offer the administrator a way to
   limit the authorization of users.  
   For example, the only way to add another BMC user may
   give them administrator-level access.

 - Not required: Secure internals.
   The BMC is not required to restrict access to its internal controls
   from administrators.
   For example, the administrator can be root or a user with sudo, and
   configuration files can be read+write.
   
 - Not required: Auditing.
   The BMC is not required to maintain a log of security events.

 - Not required: Periodic updates.
   The BMC is not required to have a function that
   periodically updates its firmware or its payload host's firmware.

 - Not required: Advanced transport level security.
   Although the BMC is REQUIRED to use transport level security (TLS)
   for communications with the operator,
   is NOT required to use any specific protocol, and
   may use obsolete protocols.

   The BMC is not required to use TLS for communication with
   the payload device or attached console.

 - Not required: Secure data at rest.
   The BMC is not required to encrypt any data it collects
   from the payload device;
   it may store such data on non-volatile and removable media
   such as flash memory.
   Note that such data may includes information about
   the payload device's configuration, events, or other operations.

 - Not required: Secure firmware updates.
   The BMC is not required to validate any firmware updates for it
   or its payload system are authentic, for example by using 
   digital signatures.

 - Not required: Self test.
   The BMC is not required to perform a self-test
   of its security function during start up or when requested.

Supported interfaces:
The BMC must support at least one interface that
can be used by an operator to manage and control the payload device.
Examples of typical interfaces include:
the attached console, login via ssh, the web application, the REST API,
IPMI, including IPMI access from the payload device.
If the BMC offers more than one interface,
each such interface must be identified and either:
 - specifically excluded from the security claim AND 
   shut off from access
   (such as via a BMC firewall or by stopping its daemon), or
 - specified separately

Objective: User Identification and Authentication.
The BMC shall identify and authenticate users for each supported interface.
In computer speak, each interface must require a userid and password
or something stronger.

Objective: Authentication changes
The BMC shall have a way to change the authentication credentials
of its users.
In computer speak, a way to change the password.

Objective: Access control.
The only way to use a BMC function shall be
via an authenticated user authorized to use that function.
However, note that the BMC Protection Profile 
does not require any specific authorization model,
so it may be that any user with a correct password
is authorized to all functions.

Objective: Secure Network Communications.
The BMC shall have a mechanism to prevent
unauthorized access, replay, and spoofing.
For example, transport level security prevents unauthorized access.


### Objectives for the BMC's Operational Environment

The objectives in this subsection relate to the operational environment
in which the BMC operates.
They form the basis for operational documentation
such as used during installation and operation.
From the BMC Protection Profile's point of view,
it is assumed that these objectives are satisfied,
so they for the basis for any BMC requirements.

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

