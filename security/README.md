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
   It has value to team leaders, developers, testers, security
   experts, and downstream projects.

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


External specs for OpenBMC
--------------------------

Here are some standards that could be applied to OpenBMC:

1. NIST [Platform Firmware Resiliency Guidelines]
(https://csrc.nist.gov/publications/detail/sp/800-193/final)
"provides technical guidelines and recommendations supporting
resiliency of platform firmware and data against potentially
destructive attacks."  BMCs are explicitly mentioned.

2. NIAP [Protection Profile for Virtualization]
(https://www.niap-ccevs.org/Profile/Info.cfm?PPID=408&id=408) is to
"describe the security functionality of virtualization technologies in
terms of Common Criteria and to define security functional and
assurance requirements for such products."  In the document,
references to the "management subsystem" include BMCs.

3. The Open Web Application Security Project (OWASP)
[Application Security Verification Standard (ASVS) Project](https://www.owasp.org/index.php/Category:OWASP_Application_Security_Verification_Standard_Project)
"provides a basis for testing web application technical security
controls and also provides developers with a list of requirements for
secure development."


Common Criteria Primer for OpenBMC developers
---------------------------------------------

Common Criteria is a mature standard that can lead to internationally
recognized security certification.  It also offers benefits to the
development team in the form of a comprehensive framework and
catalogue of security-relevant areas.  But at is heart, CC defines a
model for your product's security story.  A highly detailed,
fact-based model that developers, customers, and security experts can
check and improve over time.

The format of the Common Criteria story begins by talking about a
product and its assets.  It then identifies threats to the assets and
talks about objectives to address those threats.  Finally, it defines
the product security requirements in terms of the objectives.

For example, a BMC (product) is a small computer that controls a
larger such as a powerful computer server.  The BMC can be used to
check errors on a server (asset) and reboot it (asset).  However,
someone can reboot the server by accident (threat) or deliberately to
cause problems (threat).  Users should have to sign in before they can
use the BMC (objective).  So, the BMC must protect access to its
function with a userid and password (requirement).  This part of the
story is reusable across all OpenBMC releases, so it is factored out
and formulated as the OpenBMC Protection Profile.

Like many products, OpenBMC exposes several user interfaces: http,
web, and command-line interfaces.  The story is told for each of these
pieces.  Note that CC expects us to refer to additional national
security standards or best practices as appropriate, for example, web
security.

The second part of the story provides assurance that the product's
security functions were implemented correctly.  It begins by
describing the product's internal architecture and design,
articulating how each security function works.  Details for each major
security function include:
 - what function this provides,
 - names of source code modules,
 - code walkthroughs, configurations,
 - how lower-level security functions are called,
 - etc.
The testing story is also told; details include: How each security
function is tested, names of test cases, evidence the tests were run
and were successful.

For example, OpenBMC anchors functional assurance details in its
security design workbook.  Each major interface (http server, web
server, command-line, etc.) has its own section and unique story to
tell.  Although Common Criteria requires documentation of specific
areas (as above), it doesn't prescribe how to present the material.
Finally, note that Common Criteria allows for different levels of
detail (the "evaluation assurance level (EAL)" concept) in providing
this material; OpenBMC is just beginning to organize the work here.

The third part of the story provides assurance that security flaws
were not introduced at any point in the development process.  It reads
like a catalog of topics: developer training, code review process,
source code management tools, tool chains, secure physical
environment, etc.  Any one of which could be exploited by an attacker.

The OpenBMC security framework lists these areas, and
explains the effort for each (or why the item is not applicable).

The overall main idea is that once the product documents how it meets
its functional security requirements (which it advertises to
customers), and the development team meets the security assurance
requirements (which is normally a private matter), an independent
security firm evaluates the evidence, and a national agency grants a
security certification which is recognized internationally.

So Common Criteria is geared toward certifying a product
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

The documentation here is a balance between formality and effectiveness.
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
   the information it stores, and controls it provides.
   These requirements are given in the OpenBMC Protection Profile.
   This topic is fully addressed in Common Criteria part 2.

 - Security Assurance Requirements (see CC Part 1, SAR):
   Says everything the OpenBMC development team must do to assure
   that their intention to deliver OpenBMC security features
   was carried out properly.
   This consists of team-wide and component-specific elements.
   The team-wide elements are addressed by team leadership, and include
   developer education, mandatory code reviews, source control systems.
   These are addressed in the security-framework.
   The component-specific elements include
   design documents, code walkthroughs, and reasons why.
   These are addressed in the security-design-workbook.
   This topic is fully addressed in Common Criteria part 3.


Use cases for this documentation
--------------------------------

This documentation addresses a wide audience.  The use cases described
here are intended to help you find what you are looking for.

**Use case:** As a project leader, I want to learn about the overall
security stance of the OpenBMC project.

Study the README file.  It states:
 - that OpenBMC uses the Common Criteria framework,
 - where to find more information about Common Criteria, and
 - where to find the security framework that gives details about how
   OpenBMC addresses each Common Criteria topic.

The README mentions other items of interest:
 - external standards that may apply to OpenBMC.
 - where to find the Protection Profile which defines the OpenBMC's
   security functional requirements.
 - where to find the security design workbook which gives details
   needed for the security assurance requirements.
 - where to find the requirements for other security assurance
   requirements, such as coding standards, required reviews, etc.

The main idea is that OpenBMC is applying Common Criteria standards to
improve its security stance.  This is a work in progress, including
fleshing out the framework, developing the Protection Profile, and
recording details in the security design workbook.

Teams using OpenBMC as part of their highly-secure project are
expected to:
 - fork the OpenBMC repositories
 - securely build their version of OpenBMC
 - securely incorporate their OpenBMC image into their project
 - evaluate their security needs

Projects who seek Common Criteria certification would also:
 - define a Target of Evaluation (see CC, part 1, TOE) which includes
   their OpenBMC build.
 - define a Security Target (see CC part 1, ST), which includes
   OpenBMC security function.
 - use documentation provided by the OpenBMC project as part of their
   security evaluation.

The OpenBMC team welcomes feedback in this area.


**Use case:** As a technical leader, I want to find specific security
requirements or implementations.

Your requirements are defined in the OpenBMC Protection Profile
section titled "security functional requirements".  The security
design workbook addresses security assurance requirements and has
details about how each security function is designed, coded, and
configured.

The remaining security assurance requirements relate to development
practices and publications intended for users, both of which you
should review.  These are detailed in the README file and the
security-framework.

The security design workbook is a work in progress and is yours to
contribute to.  Please help make it a joy to read.


**Use case:** As a technical contributor, I need to learn about basic
security requirements required by all contributors.

TO DO: The main ideas are that OpenBMC has coding standards, a review
process, repo maintainers, etc. to help ensure no security flaws are
introduced.  Also that developers understand their code runs in a
priviledged environment, so they need to avoid accidentally creating
security flaws.


Roadmap
-------

The OpenBMC security documentation work is in three phases:

1. Complete the framework.  The framework consists of three main pieces:
    1. The outline of sections from the Common Criteria Parts 1-3.
    2. The OpenBMC Protection Profile.
    3. The OpenBMC security design workbook.

   For each section:
    1. Establish correspondence with the Common Criteria framework
       with a link into the Common Criteria documents.
    2. Motivate and establish relevance for each section.
    3. Begin to fill in details (in phase 2).

2. Fill in the framework with details about OpenBMC.  This documents
   the current state of the OpenBMC project.  This is necessarily a
   collaborative and iterative process.  We'll start with the basics and
   try to address all the sections.  More detailed informaton can be
   provided as needed (in phase 3).

3. Use the framework to make more informed decisions about security.
   This includes upgrading the content of the framework to higher
   assurance levels (back to phase 2).


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
