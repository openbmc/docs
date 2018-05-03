# OpenBMC and the Common Criteria Security Framework

This is a comprehensive overview of all OpenBMC security topics
organized under the Common Criteria framework.

Common Critera (CC) provides a comprehensive framework that covers all
areas of security including product design, development, deployment,
and operation.  For more information, see [Common Criteria](https://www.commoncriteriaportal.org).

References to the Common Criteria publications look like this, "(see
CC part 1, ASE_SPD)," and refer to the [Common Criteria for
Information Technology Security Evaluation](https://www.commoncriteriaportal.org/cc) publications, parts 1-3,
Version 3.1 Revision 5.  When it is clear from the context, the class
and family (see CC part 1, "Terms and definitions") are used without
explicit references to the CC publications.  For example, ASE_SPD
refers to "CC part 1, ASE_SPD."

TO DO: Common Criteria uses tags that include underscores, for
example, ASE_SPD.  This use conflicts with [Markdown](https://daringfireball.net/projects/markdown/) formatting
which interprets the underscore to mean italics.  We could fix this by
using a backslash to escape the underscore, but that is ugly.  We
could fix this by using a dash or dot instead of the underscope, for
example like ASE-SPD.

Common Criteria documents referenced here include:
- [Part 1: Introduction and general model](https://www.commoncriteriaportal.org/files/ccfiles/CCPART1V3.1R5.pdf)
- [Part 2: Security functional requirements](https://www.commoncriteriaportal.org/files/ccfiles/CCPART2V3.1R5.pdf)
- [Part 3: Security assurance requirements](https://www.commoncriteriaportal.org/files/ccfiles/CCPART3V3.1R5.pdf)

For a 20-minute overview of the benefits Common Critera can bring to a
security-aware development team, begin with:
 - CC Part 1, Chapter 6 ("Overview").
 - CC Part 1, Chapter 7 ("General Model").
 - CC Part 2, Chapter 6 ("Functional requirements paradigm")
 - CC Part 3, Chapter 6 ("Assurance paradigm")

The structure of this document follows the outline in the Common
Criteria publications.  The primary focus is to help the OpenBMC
development team produce a secure product.  A secondary focus is to
inform other groups who use OpenBMC in their highly-secure product
(whether they intend to pursue a Common Criteria evaluation or not).

When reading the Common Criteria publications, you'll need to know
these commonly-used acronyms (see CC Part 1):
 - TOE (Target of Evaluation)
   Refers to the OpenBMC code and device it runs on,
   described in detail in the OpenBMC Protection Profile.
 - SFR (Security Functional Requirements)
   Refers to the security requirements in the OpenBMC Protection
   Profile that talk about what the OpenBMC code must do: check
   passwords, use secure communications channels, etc.  Contrast with
   security assurance requirements which do not appear in the code.
 - TSF (TOE Security Function)
   Refers to the OpenBMC's implementation of the SFR security
   functions: checking passwords, using secure communications, etc.


## CC Part 1: Introduction and General Model

### Target of Evaluation (see CC Part 1, TOE)
OpenBMC is not pursuing a security evaluation and does not define a
Target of Evaluation (TOE).  For most development purposes, your TOE
can be your most recent OpenBMC build loaded onto any old device.

### OpenBMC Protection Profile
The OpenBMC development team defines an OpenBMC Protection Profile
intended for use by the development team, downstream developers, and
security evaluators.

The Protection Profile talks about what security problems BMCs face
and defines security objectives and requirements.  Specifically, it
addresses the following (see CC Part 1) topics:
 - ASE_INT Introduction
 - ASE_CCL Conformance Claims (none yet)
 - ASE_SPD Problem Definition
 - ASE_OBJ Security Objectives
 - ASE_ECD Extended Component Definition
 - ASE_REQ Security Requirements

The protection profile in currently a straw-man and has not been
evaluated against the CC standards.


## CC Part 2: Security Functional Components

OpenBMCs security functional requirements are expressed in the OpenBMC
Protection Profile.  These are the items the BMC must provide, for
example checking passwords, denying access, and using secure sockets.

The OpenBMC project addresses the following major areas.  This list
and the tags it uses are from Common Criteria, Part 2.

### FAU - security audit
Security auditing involves recognising, recording, storing, and
analysing information related to security relevant activities
(activities controlled by the BMC).

### FCO - communication
This covers non-repudiation of the origin and receipt of messages.
The BMC has no such concerns.

### FCS - cryptographic support

The BMC has cryptographic support for user authentication and for
transport level security.
TO DO: Mention that OpenBMC uses Linux user ids (???with PAM?).
TO DO: Explaoin how OpenBMC creates a sshd key pair.
more?

### FDP - user data protection
This specifies requirements for protecting data such as event logs,
part data, etc.

#### FDP_ACC access control policy
#### FDP_ACF access control functions
#### FDP_DAU data authentication
#### FDP_ETC export from the BMC
#### FDP_IFC information flow control policy
#### FDP_IFF information flow control functions
#### FDP_ITC import from outside the BMC
#### FDP_ITT internal BMC transfer
#### FDP_RIP residual information protection
#### FDP_ROL rollback
#### FDP_SDI stored data integrity
#### FDP_UTC inter-BMC data confidentiality transfer protection
#### FDP_TSF inter-BMC data integrity transfer protection

### FIA - identification and authentication
Ensures that users are associated with the proper security
attributes (e.g. identity, groups, roles, security or integrity levels

#### FIA_AFL Authentication failures
Deals with timeouts, repeated login failures, and disabling user ids.

#### FIA_ATD User attribute definition
Deals with how each user can have a different set of authorizations.

Each OpenBMC user in either a regular user, root, or has sudo priviledge.

#### FIA_SOS Specification of secrets
This talks about the quality of secrets the BMC creates for itself.

TO DO: I think sshd creates a certificate on the BMC the first time it
is used.

TO DO: Does the BMC have a good source of randomness it can use to
generate sessioon keys?

#### FIA_UAU User authentication
This talks about the types of user authentication OpenBMC supports.
This also covers what information can be given away to
non-authenticated users, for example, at the login prompt, or at the
login screen on a web app, or to http requests with no session info.

The BMC supports username and password.
TO DO: For ssh, it supports certificate-based authentication.
TO DO: For REST API use, it supports session credentials.

#### FIA_UID User identification
This talks about when you have to give your userid to use the BMC.
TO DO: talk about ssh, login to rthe web app, and the REST APIs.

#### FIA_USB User-subject binding
This talks about what you can access within the BMC.

TO DO: There are currently only two levels of access: root or sudo,
and regular users.  Regular users can access all functions and data,
including controlling the payload host.  Root can also perform
administrative functions on the BMC such as changing user accounts,
and reconfigururing the BMC's behavior.

FUTURE: One idea is to have role-based bindings: admin, operator,
and read-only.

### FMT - security management
This talks about how the BMC admininstrator can authorize other
users to various BMC functions.

TO DO: This is currently a very simple model.  The admin can
create, disable, or removfe user accounts, and can grant sudo access
to create another administrator.

### FPR - privacy
This protects users against discovery and misuse of identity by
other users.

The BMC has no concerns on this area.  Anopnymity, pseudonymity,
unlinkability, and unobservability do not apply.

### FPT - protection of the BMC
This focuses on protecting the BMC from changes that affect how it
enforces its own security functions.  (Compare with user data protection.)

TO DO: Regular users are prevented from changing BMC controls via
the usual Linux mechanisms, e.g., read only permissions on system files,
no access to the systemctl function, etc.
TO DO: If there is a read-only file system, the BMC is protected
against certain changes even from an admin/root.

The term "TSF data" is used in several of the assurance families.
It refers to the "BMC's security-related data" such as:
 - BMC firmware images
 - BMC digital certificates
 - BMC audit data

It does not include other important data that the BMC stores such as
FRUs and SELs.

#### FPT_FLS Fail secure
There are no types of BMC failures that would cause it to not enforce
its security function.

TO DO: What if PAM fails, or we cannot connect?

#### FPT_ITA Availability of exported TSF data
This refers to the BMC exporting its security-related data to another
system.

TO DO:

#### FPT_ITC Confidentiality of exported TSF data
This refers to protecting the BMC's security-related data from
disclosure while it is in transit our of the BMC.

TO DO: The only such supported function is transferring firmware
images for the PNOR???

TO DO: Most use cases are protected by using ssh or https.

TO DO: The firmware image for the payload device travels over wires(?)
from the BMC to the payload device.  What other devices have
visibility to this space, and why do we trust them?

#### FPT_ITI Integrity of exported TSF data
This refers to protecting the BMC's security-related data from being
modified while it is in transit.

TO DO: Same ideas as above.

#### FPT_ITT Internal BMC TSF data transfer
This refers to protecting the BMC's security-related data when it is
transferred between separate parts of the BMC across an internal
channel.

This does not apply: the BMC is a system on a chip.
TO DO: What about the flash device that stores the BMC's code?

#### FPT_PHP BMC TSF physical protection
Says how the BMC detects physical tampering.

TO DO: The BMC is enclosed in a case.  As far as I know, BMC detects
when the case is opened and reports this fact, and the condition can
only be reset by ?? authorized users.  ??? No detection when the BMC
is powered off.

#### FPT_RCV Trusted recovery
Says how the BMC ensures it security functions are working when it
starts up.

TO DO: What does Linux do?  Self checking?

#### FPT_RPL Replay detection
Says how the BMC detects and corrects replayed events.  For example,
someone could record PNOR being loaded onto the payload host and play
that back 3 years later when there are known exploits to flaws in its
authentication mechanisms.

TO DO: Does ssh or https help prevent this?

#### FPT_SSP State synchrony protocol
Does not apply.  The BMC is not distributed.

#### FPT_STM Time stamps
The BMC needs reliable time stamps.

The BMC has uses a real-time clock either on board or as an
external device.
TO DO: If external, why do we trust it?
TO DO: Does it sync using network time protocol (NTP) servers?
TO DO: Does the BMC ever synchronize with the payload host's clock?

#### FPT_TDC Inter-TSF TSF data consistency
This refers to the BMC exchanging its security-related data either
with a distributed part of itself, or another trusted device.  The
data interpretation must consistent.

Does not apply. (?)

#### FPT_TEE Testing of external entities
This refers to the BMC testing the integrity of its firmware image,
the platform it is running on, the payload host, an external PAM
server, etc.

TO DO: The BMC does not do this.

#### FPT_TRC Internal TOE TSF data replication consistency
This refers to keeping the BMC's security-related data consistent
when there are more than one copies of it.

TO DO: Are there?  Or is this N/A?

#### FPT_TST BMC self test
This refers to the BMC testing its security function, including
the integrity of its code.

TO DO: Does it?

### FRU - resource utilization
This refers to the availabilty of BMC function, for example, if the
HMC or payload host goes haywire and launches a DOS attack on the BMC.
NOTE: The term FRU also refers to Field Replaceable Unit.

### FTA - BMC access
TO DO

### FTP - trusted access channels
TO DO


## CC Part 3: Security Assurance Components

These are things the OpenBMC development team must do to assure
that due care was used the following areas:
 - the development team implementing security features
 - security features were tested
 - the security architecture and design was examined
 - security features were tested
 - the handoff from development to operations was secure
 - security guidance for operations is correct

These items are the combined responsibility of the OpenBMC development
team, the downstream team using the OpenBMC project, and upstream
projects that OpenBMC uses.


### Evaluation Assurance Level
The Evaluation Assurance Level (see CC part 3, EAL) expresses how
**well** the development team implemented security-relevant processes
like code reviews and source code control, how cleanly they
implemented security functions, and how well they tested the security
functions.

TO DO: Create templates to support Common Criteria assurance level 4
(see CC Part 3, EAL4).  This level requires:
 - a clean architecture which OpenBMC provides by using module design
   including separate web servers, systemd, and dbus.  The basic
   architecture and modular design must be documented.
 - a controlled development process, including source code control
   using git and github, and code reviews using gerrit.
 - secure delivery, provided in part by digitally signed firmware

Note that having an open source means that anyone can find and report
flaws to the development community (and that attackers can more easily
find flaws to exploit).

### Assurance Classes
The OpenBMC project addresses the following major assurance areas.

The acronyms used in this section are in Common Criteria, Part 3, as
referenced at the top of this document.

TO DO: Using a bulleted list doesn't feel like the right way to anchor
these topics.  Find a better way to present them.

TO DO: Provide a one-sentence introduction to each topic drawn from
the CC Part 3.  The idea is for developers to get maximum information
from this outline, only referring to Common Criteria manuals for
deeper study.

### APE - Protection Profile evaluation.
The OpenBMC protection profile has not been evaluated to CC standards.

### ASE - Security target evaluation
The OpenBMC does not define a security target and instead defines a
protection profile.

### ADV - Development

#### ADV_ARC Security architecture
The development team documents the architecture of security-relevant
interfaces.  File: obmc-security-design.md

#### ADV_FSP Functional spec
#### ADV_IMP Implementation representation
#### ADV_INT BMC security internals
#### ADV_SPM Security policy modeling
#### ADV_TDS BMC internal security design evaluation

### AGD - Guidance

#### AGD_OPE operational user guidance

#### AGD_PRE preparative (installation) procedures

The OpenBMC team is collecting security guidance, that is, all of the
warning and requirements intended for OpenBMC installation and
operations personnel.  File: obmc-security-publications.md

### ALC - lifecycle support

#### ALC_CMC Configuration Management (CM) capabilities
This talks about controlling changes to the source code.

OpenBMC uses code repositories under https://github.com/openbmc.  Each
repo has a small list of responsible maintainers.

#### ALC_CMS Configuration Management (CM) scope
This talks about what is controlled.

The following repositiories are included:
 - github.com/openbmc/openbmc
 - yocto and bitbake, including all tools in the build toolchain
 - various repos under github.com/openbmc
 - various other open source projects

The OpenBMC openbmc repo uses yocto, which pulls in other repos.  It
has a mechanism to select which version of each repo to retrieve,
typically using a git commit id.

TO DO: provide a way to enumerate the repos, for example, by using
bitbake commands (bitbake -e?).

TO DO: We'll have to explain that we are relying on the security of
the various open source projects openBMC is using and why we think
that is a good idea.  The open source community has some advice here.
The build environment is also technically included in CM, e.g.,
bitbake runs on RHEL or Ubuntu when making released binaries.

#### ALC_DEL Delivery
This has two parts:
 1. OpenBMC delivers source code under github.com/under openbmc repos.
    That security story is well-known.  TO DO - state it here
 2. Any downstream project is expected to fork openbmc, including
    the openbmc repos identified under ALC_CMS.
    Then things like digitally signatures can and digital
    fingerprints be used to secure the OpenBMC install images.

#### ALC_DVS development security
The physical, procedural, personnel, and other security measures
used in the development environment.

The OpenBMC team restricts updates to the code by requiring the
"maintainer" of each code repository to approve the change.  Any such
code updates either go through a peer review process on gerrit, or
come from a "git pull request" where they are reviewed and go into the
code only after being reviewed and approved.

Example: https://github.com/openbmc/docs/MAINTAINERS or
https://github.com/openbmc/docs/blob/master/MAINTAINERS

Ideally have way to ensure developers (especially required reviewers)
have read and understand basic secure engineering education.  Does
github or gerrit offer any help here?

Note that OpenBMC is an open source project, so there is no
confidential code in the shipped code.

TO DO: Some of this material may be mis-categorized and needs to be
moved between the ALC_DVS and ALC_TAT classes.

#### ALC_FLR flaw remediation
This requires a system to track security flaws.

The OpenBMC team uses an issues database to track problems.

We should periodically (annually or release boundary) evaluate getting
newer versions of our upstream open source projects.  Ideally, we
would monitor for (or be notified of) critical patches and quickly
evaluate if we need to apply them and re-release OpenBMC.  We also
need a way to address flaws in OpenBMC itself (github issues) and
provide critical security fixes.

A mature system would have:
 - The OpenBMC team proactively monitoring for flaws announced by our
   upstream projects
 - a way to take flaw reports for OpenBMC
 - a system to categorize and prioritize flaws
 - developers mitigate flaws (workaround, configuration, bug fix)
 - the OpenBMC team announces flaws to its downstream users

#### ALC_LCD lifecycle definition
This talks about how to ensure basic quality of the OpenBMC code.

Some OpenBMC contributors track "epics" and "stories", including much
of the security work items.  However, other contributors may add their
own items ad-hoc or from their private management system.
TO DO: How does the open source community maintain control of the
release life-cycle when they don't have full control of integrations
into the code?

One aspect is that OpenBMC incorporates upstream open source
projects as part of its function.  This talks about how we ensure
their quality and they are correctly integrated into OpenBMC.

#### ALC_TAT tools and techniques
This addresses things like C++ coding standards, the review process

### ASE - security target evaluation
TO DO

### ATE - tests

#### ATE_COV coverage
#### ATE_DPT depth
#### ATE_FUN functional tests
#### ATE_IND independent testing

### AVA - vulnerability assessment

#### AVA_VAN - vulnerability analysis

Note that the results of a vulnerability analysis many include flaws
that we do not want to be made publicly available until we address
them.  These would typically be published in the form of work items.

### ACO
This says how the OpenBMC is "composed" with other services
such as PAM servers.

TO DO: The OpenBMC is going to use a PAM server hosted on an external
server?  Details about how that is configure would be stored here.

TO DO: This might be the right place to talk about how the BMC is
"composed" with its host system.


## Assorted and uncategorized topics
Please add topics here!

Investigate https://bestpractices.coreinfrastructure.org/en which
links to: http://web.mit.edu/Saltzer/www/publications/protection/

See also: https://www.ibm.com/security/secure-engineering/
