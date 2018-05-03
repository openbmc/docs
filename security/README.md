# OpenBMC security documentation

This is the root of all OpenBMC security documentation.

Preamble
--------

The OpenBMC security documentation is organized under the
[Common Criteria](https://www.commoncriteriaportal.org) framework.

References to the Common Criteria publications
look like this (see CC part 1, ASE_SPD) and refer to the
[Common Criteria for Information Technology Security Evaluation]
(https://www.commoncriteriaportal.org/cc)
publications, parts 1-3, Version 3.1 Revision 5.

Using Common Criteria provides the following benefits:
 - It's a path to internationally recognized certification.
 - It provides value to the development team by organizing
   all security-related information within a consistent framework.
 - It assures users that OpenBMC was developed with security built in.

The general flow of the OpenBMC security documentation is:

1. The OpenBMC Security Framework.
   This is the Common Criteria security framework, filled in with
   details about OpenBMC.
   Its primary value is having a single place for all
   of our security documentation.

2. The OpenBMC Protection Profile.
   This describes security issues BMCs face,
   says how OpenBMC addresses them, and
   defines the security requirements.

3. The OpenBMC Security Design Workbook.
   This describes how each OpenBMC interface
   addresses its security requirements.
   It states the design and basic code flow, and talks about
   how passwords and certificates are handled,
   how to find out which ciphers are accepted, etc.


Documentation
-------------

 - [OpenBMC Security Framework](obmc-security-framework.md)

   This addresses every OpenBMC security topic
   organized under the Common Criteria framework.
   Use this document to learn about OpenBMC security topics and
   as a starting point for specific security topics.

   Example topics include:
    - what are the security requirements
    - what security features OpenBMC provides
    - how they were designed and implemented
    - coding conventions to aid review and analysis
    - how developers perform code reviews and update the code base
    - what level of testing is performed
    - how the code is turned over to operations
    - what guidance is provided to operators

   The OpenBMC team should keep this material up to date
   for its own security analysis and
   for any groups who wish to perform a security evaluation.

 - [BMC Protection Profile](obmc-protection-profile.md)

   Everyone should read the OpenBMC Protection Profile.

   The document introduces BMCs and the security problems they face,
   works these into objectives, then
   works the objectives into security requirements.
   (The structure is from the Common Criteria framework.)

   The OpenBMC project should meet the requirements
   specified in the OpenBMC Protection Profile.

 - [BMC Protection Profile Modules](obmc-protection-profile-modules.md)

   This extends the OpenBMC Protection Profile
   to address various additional security problems.
   The idea is this document records proposed standards.
   Over time, some of these modules may be incorporated
   into the OpenBMC Protection Profile.

   Including a module in this list does not mean
   OpenBMC complies with its security requirements,
   only that the security problem is known to us.

   Some examples of BMC protection profile modules are:
    - Requires the BMC to audit access to its functions.
    - Requires the BMC to allow the admin to restrict user authority
      to three basic roles: admin, update, and read only.
    - Requires the BMC to allow the admin to restrict the types of
      authentication the BMC will accept.  For example, it can
      disallow weak passwords or require certificate-based authentication.
    - Requires the BMC to reject communication attempts based on
      weak transport-level security protocols.
      For example, IPMI version 1.0.
    - Requires the BMC to have its internal controls secured to prevent
      accidental changes from the admin, defects, or a rogue agent.

 - [OpenBMC Security Publications](obmc-security-publications.md)

   This contains all the security-related information
   that must be addressed in the OpenBMC publications so that
   installers, administrators, and operators will
   know how to use the OpenBMC system in a secure manner.

 - [OpenBMC Security Design](obmc-security-design.md)

   This says how OpenBMC meets the security requirements.
   The document references the objectives and requirements
   listed in the BMC Protection Profile and
   talks about how the OpenBMC project addresses each item.


Common Criteria Primer for OpenBMC developers
---------------------------------------------

Common Criteria is geared toward certifying a product
by evaluating it against a set of security standards.
It defines a comprehensive framework which talks about:
 - what product we are protecting,
 - what assets there are to protect,
 - what agents act on those assets,
 - what threats we need protect against,
 - what harm can result,
 - how the product protects itself in terms of its architecture and design,
 - why we believe the protection is correct
   in terms of its implementation and test cases,
 - how we protect the code against tampering,
 - how we deliver the product to customers,
 - what guidance we provide customers,
 - and other topics.
Each topic is thoroughly discussed
then methodically combined to produce evidence
that your product is secure.
The evidence is evaluated, and the product is certified.

This documentation is a balance between formality and effectiveness.
Like 20% of the effort for 80% of the benefit.
The approach OpenBMC uses for security documentation is:
 - Use the Common Criteria framework, but
   not structure it for a product evaluation.
   This addresses the most relevant topics
   without the burden of a formal assessment.
 - Interpret the abstract Common Criteria terminology
   into language more familiar to software engineers.
 - Interpret Common Criteria requirements into more specific
   requirements that relate to the OpenBMC team.
   References into the Common Criteria documentation are provided
   which give more details about why the requirements are needed
   and how to fulfil them.
 - Start small by creating a framework in which to place specific
   information, and give examples thay say:
    - what information is needed,
    - how to structure it,
    - what level of detail to provide.
 - Make it easy for subject matter experts to learn about
   OpenBMC security topics and contribute.

**What's not covered here, and what the Common Criteria standard
does not cover, is that we have the correct set of BMC features
or the correct security requirements.
So please help out here where you can.**

Here is terminology for developers new to formal security analysis:

 - Protection Profile (see CC Part 1, PP):
   A generic description of a BMC that
   introduces the security problem, and
   defines security objectives and functional requirements
   in a nice clean reusable package.
   A protection profile has a wide audience.

 - Security Problem (see CC Part 1, ASE_SPD):
   Defines the security problems the BMC is expected to address
   in terms of agents who pose threats to the BMC's assets.
   It also discusses assumptions made in identifying threats and
   organizational policies the BMC must conform to.
   Contast this term with "security vulnerability" which is
   a known security weakness.

 - Security Functional Requirements (see CC Part 1, SFR):
   Says what the BMC hardware and software must do to protect itself,
   the information, and controls it provides.
   These requirements are given in the OpenBMC Protection Profile.
   This topic is fully addressed in Common Criteria part 2.

 - Security Assurance Requirements (see CC Part 1, SAR):
   Says everything the OpenBMC development team must do to assure
   that their intention to deliver OpenBMC security features
   was carried out properly.
   This consists of team-wide and component-specific elements.
   The team-wide elements are addressed by team leadership, and include
   developer education, mandatory code reviews, source control systems.
   The component-specific elements include
   design documents, code walkthroughs, and reasons why.
   This topic is fully addressed in Common Criteria part 3 and
   fleshed out in the OpenBMC Security Design Workbook.


Notes to Reviewers
------------------

Please review the entire set of documents and
give special consideration to the following areas:

OpenBMC team leadership:
 - The use of the Common Criteria framework.
 - The overall structure of the documentation.
 - The level of detail of design documentation expected from developers.
 - Team-wide security requirements.
   These are addressed in the obmc-security-framework.  Examples:
    - coding conventions reduce cuding errors and
      make it easier to understand code,
    - code reviews make it easier to discover security flaws,
    - source code control such as git provides a way to ensure that only
      reviewed and approved code goes into the project, and
    - developer training helps developers understand the
      importance of secure engineering practices, and
      how they can prevent inadventently introducing security flaws.

OpenBMC technical leaders:
 - The OpenBMC Protection Profile.
   This document is yours to define what assets to protect
   how to protect them, and
   exactly what the requirements are.

OpenBMC user interface owners:
 - The OpenBMC Protection Profile, for areas you own.
 - The security design for areas you own

OpenBMC developers:
 - OpenBMC developer education



How to read the BMC Protection Profile
--------------------------------------

Everyone should read the BMC Protection Profile.

All designers, testers, coders, writers, managers,
customizers, and curious onlookers.
At least the first two sections.

The Protection Profile document is in the style of a
[NIAP Protection Profile](https://www.niap-ccevs.org/Profile/PP.cfm).
This style is curious blend of
formal and informal, different points of view, and level of detail.
The first two sections are informative and
really are meant for a wide range of audiences.
The second two sections are normative and
provide a basis for a formal security review.
With that is mind, here is an idea of what to expect.

The sections are:

1. Introduction.
   This uses uses plain language to introduce the BMC.
   It talks about the BMC's place in the data center,
   introduces user ids, passwords, and access controls,
   what kinds of things you can do with the BMC,
   and bad things that can happen without security.

2. The security problem.
   This more clearly defines user roles,
   defines exactly what are we trying to protect, and
   lists all of the threats are we trying to protect against.
   Example:
     Users log in with their password, then invoke the function
     to update the payload host system's firmware.  If they are
     authorized and the firmware images passes inspection,
     it gets loaded.  Otherwise a security event is logged.
     Without this, someone could add a virus to the payload
     system's BIOS.

3. Security objectives.
   This section defines all of the BMC security objectives to
   protect against all of the threats identified above.
   It states things like
   how the BMC must be installed and maintained,
   how users gets authorized, and
   what the BMC must do to maintain security.
   These objectives form the basis for the security requirements
   in the next section, so they are important.
   The language in this section becomes more formal,
   changes to the BMC's point of view,
   and we begin to see references to the security requirements.
   Example:
     O.ACCOUNTABILITY:
     The BMC requires a userid and password.
     The credentials are checked and the result is logged.
     Only if the credentials are good, a menu of options is
     presented.
     If they try to update the host system's
     firmware, their authority to do so is checked and the results
     are logged.  If authorized, ... etc.

4. Security Requirements.
   This section defines the requirements for BMC security.
   It addresses all of the security objectives from the previous section
   and re-states them in the form of requirements.
   The language in this section is in standardized security-speak
   to make it easier for security experts to
   compare these requirements with other protection profiles and
   between various BMC implementations.
   Example:
     Identification and Authentication (FIA):
     FIA_UAU.5 Multiple Authentication Mechanisms
     FIA_UAU.5.1
     The BMC shall provide the following authentication mechanisms
      - authentication based on user name and password,
      - authentication based on X.509 certificates
     to support user authentication.

Each section builds on material from the previous section.

Note it is standard industry practice to write
a Protection Profile such as the
BMC Protection Profile as a generic specification,
not specific to any particular BMC vendor.
This is so all BMC manufacturers use the same standards and
makes it easier to compare security between similar devices.
For our strawman draft, we're specific to OpenBMC.


Where are the security requirements exactly?
--------------------------------------------

The security requirements are defined in the
OpenBMC Protection Profile.
Within the protection profile,
the security requirements are in three sections:

1. Security Objectives / Objectives for the BMC's Operational Environment.
   This defines the requirements that govern
   what type of environment the BMC can be installed and
   how it must be operated.
   These requirements go into the operational manuals.

2. Security Requirements / Security Functional Requirements
   This defines requirements for the BMC's external interfaces
   and governs what the developers must deliver.

3. Security Requirements / Security Assurance Requirements
   This defines aspects of how the OpenBMC development team
   provided the "security functional requirements", for example
   that they held code reviews and ran test cases.

Please note that to understand the security requirements
you should also read the "security problem" and "security objectives"
sections, which motivate the requirements.


Balance openness versus security
--------------------------------

This section addresses question about
how open the OpenBMC team should be
about discussing security topics.
We what to be as transparent as possible but
not give attackers any more information
about how to defeat OpenBMC security measures
before we have a solution for our users.

In general, it is okay to openly discuss
the security measures OpenBMC implements
as well as the security measures OpenBMC does not take.
Any attacker can read our code on github or clone it for further study.
Specifically, it is okay to talk about which
security protocols OpenBMC accepts, for example
obsolete IPMI protocols, or older transport level security, etc.
We can also discuss security features that we intend to implement,
knowing that we are advertising to an attacker that
we do not currently offer such features.

We should be relatively more circumspect when discussing
flaws in the design or code that would
allow someone with a relatively low level of sophistication
armed with knowledge of the vulnerability to gain access to the BMC.
The main idea is to protect security conscious installations
by suppressing information about the flaw until the team has
addressed the problem in the form of a workaround or available fix.
TO DO: How to report security-related bugs to the OpenBMC team
is an open topic.  How does the open source community do it?
