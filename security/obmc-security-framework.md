# OpenBMC and the Common Criteria Security Framework

This is a comprehensive overview of all OpenBMC security topics
organized under the Common Criteria framework.

Common Critera (CC) provides a comprehensive framework
that covers all areas of security including
product design, development, deployment, and operation.
For more information, see
[Common Criteria](https://www.commoncriteriaportal.org).

References to the Common Criteria publications
look like this, "(see CC part 1, ASE_SPD)," and refer to the
[Common Criteria for Information Technology Security Evaluation]
(https://www.commoncriteriaportal.org/cc)
publications, parts 1-3, Version 3.1 Revision 5.
When it is clear from the context, the class and family
(see CC part 1, "Terms and definitions") are used
without explicit references to the CC publications.
For example, ASE_SPD refers to "CC part 1, ASE_SPD."

Common Criteria documents referenced here include:
- [Part 1: Introduction and general model]
  (https://www.commoncriteriaportal.org/files/ccfiles/CCPART1V3.1R5.pdf)
- [Part 2: Security functional requirements]
  (https://www.commoncriteriaportal.org/files/ccfiles/CCPART2V3.1R5.pdf)
- [Part 3: Security assurance requirements]
  (https://www.commoncriteriaportal.org/files/ccfiles/CCPART3V3.1R5.pdf)

For a 20-minute overview of the benefits Common Critera
can bring to a security-aware development team, begin with:
 - CC Part 1, Chapter 6 ("Overview").
 - CC Part 1, Chapter 7 ("General Model").
 - CC Part 2, Chapter 6 ("Functional requirements paradigm")
 - CC Part 3, Chapter 6 ("Assurance paradigm")

The structure of this document follows the outline
in the Common Criteria publications.
The primary focus is to
help the OpenBMC development team produce a secure product.
A secondary focus is to inform other groups
who use OpenBMC in their highly-secure product
(whether they intend to pursue a Common Criteria evaluation or not).

When reading the Common Criteria publications,
you'll need to know these commonly-used acronyms (see CC Part 1):

 - TOE (Target of Evaluation)
   Refers to the OpenBMC code and device it runs on,
   described in detail in the OpenBMC Protection Profile.

 - SFR (Security Functional Requirements)
   Refers to the security requirements in
   the OpenBMC Protection Profile
   that talk about what the OpenBMC code must do:
   check passwords, use secure communications channels, etc.

 - TSF (TOE Security Function)
   Refers to the OpenBMC security function, for example:
   checking passwords, using secure communications, etc.

------------------------------------------------

## CC Part 1: Introduction and General Model


### Target of Evaluation (see CC Part 1, TOE)

OpenBMC is not pursuing a security evaluation and
does not define a Target of Evaluation (TOE).
For most development purposes, your TOE can be
your most recent OpenBMC build loaded onto any old device.


### OpenBMC Protection Profile

The OpenBMC development team defines an OpenBMC Protection Profile
intended for use by the development team, downstream developers,
and security evaluators.

The Protection Profile talks about what security problems BMCs face
and defines security objectives and requirements.
Specifically, it addresses the following (see CC Part 1) topics:

 - ASE_INT Introduction
 - ASE_CCL Conformance Claims (none yet)
 - ASE_SPD Problem Definition
 - ASE_OBJ Security Objectives
 - ASE_ECD Extended Component Definition
 - ASE_REQ Security Requirements

The protection profile in currently a straw-man and
has not been evaluated against the CC standards.


## CC Part 2: Security Functional Components

OpenBMCs security functional requirements are expressed in the
OpenBMC Protection Profile.
These are the items the BMC must provide, for example
checking passwords, denying access, and using secure sockets.

The OpenBMC project addresses the following major areas:

 - FAU - security audit

 - FCO - communication

 - FCS - cryptographic support

 - FDP - user data protection
   - FDP_ACC access control policy
   - FDP_ACF access control functions
   - FDP_DAU data authentication
   - FDP_ETC export from the BMC
   - FDP_IFC information flow control policy
   - FDP_IFF information flow control functions
   - FDP_ITC import from outside the BMC
   - FDP_ITT internal BMC transfer
   - FDP_RIP residual information protection
   - FDP_ROL rollback
   - FDP_SDI stored data integrity
   - FDP_UTC inter-BMC data confidentiality transfer protection
   - FDP_TSF inter-BMC data integrity transfer protection

 - FIA - identification and authentication
   - more...

 - FMT - security management

 - FPR - privacy

 - FPT - protection of the BMC

 - FRU - resource utilization

 - FTA - BMC access

 - FTP - trusted access channels



## CC Part 3: Security Assurance Components

These are things the OpenBMC development team must do to assure
that due care was used the following areas:
 - the development team implementing security features
 - security features were tested
 - the security architecture and design was examined
 - security features were tested
 - the handoff from development to operations was secure
 - security guidance for operations is correct

These items are the combined responsibility of
the OpenBMC development team,
the downstream team using the OpenBMC project,
and upstream projects that OpenBMC uses.


### Evaluation Assurance Level

The Evaluation Assurance Level (see CC part 3, EAL) expresses
how **well** the development team
implemented security-relevant processes
like code reviews and source code control,
how cleanly they implemented security functions, and
how well they tested the security functions.

TO DO: The OpenBMC team should target being able to support
security evaluations at assurance level 4 (see CC Part 3, EAL4).
This level requires:
 - a clean architecture which
   OpenBMC provides by using module design including
   separate web servers, systemd, and dbus.
   The basic architecture and modular design must be documented.
 - a controlled development process, including source code control
   using git and github, and code reviews using gerrit.
 - secure delivery, provided in part by digitally signed firmware

Note that having an open source means that
anyone can find and report flaws to the development community
(and that attackers can more easily find flaws to exploit).


### Assurance Classes

The OpenBMC project addresses the following major assurance areas.

The acronyms used in this section are in Common Criteria, Part 3,
as referenced at the top of this document.

TO DO: Using a bulleted list doesn't feel like the right way to anchor
these topics.  Find a better way to present them.

TO DO: Provide a one-sentence introduction to each topic drawn from
the CC Part 3.  The idea is for developers to get maximum information
from this outline, only referring to Common Criteria manuals for
deeper study.

 - APE - Protection Profile evaluation.
   The OpenBMC protection profile has not been evaluated to CC standards.

 - ASE - Security target evaluation
   The OpenBMC does not define a security target and
   instead defines a protection profile.

 - ADV - Development
 - ADV_ARC Security architecture
   The development team documents the
   architecture of security-relevant interfaces.
   File: obmc-security-design.md
 - ADV_FSP Functional spec
 - ADV_IMP Implementation representation
 - ADV_INT BMC security internals
 - ADV_SPM Security policy modeling
 - ADV_TDS BMC internal security design evaluation

 - AGD - Guidance
 - AGD_OPE operational user guidance
 - AGD_PRE preparative (installation) procedures
   The OpenBMC team is collecting security guidance,
   that is, all of the warning and requirements intended for
   OpenBMC installation and operations personnel.
   File: obmc-security-publications.md

 - ALC - lifecycle support
 - ALC_CMC Configuration Management (CM) capabilities
 - ALC_CMS Configuration Management (CM) scope
 - ALC_DEL Delivery
   e.g., digitally signed OpenBMC install images.
 - ALC_DVS development security
   The physical, procedural, personnel, and other security measures
   used in the development environment.

   Example: https://github.com/openbmc/docs/MAINTAINERS
        https://github.com/openbmc/docs/blob/master/MAINTAINERS

   Ideally have way to ensure developers (especially required reviewers)
   have read and understand basic secure engineering education.
   Does github or gerrit offer any help here?

   Note that OpenBMC is an open source project, so
   there is no confidential code in the shipped code.

   TO DO: Some of this material may be mis-categorized and needs to be
   moved between the ALC_DVS and ALC_TAT classes.

 - ALC_FLR flaw remediation
   We should periodically (annually or release boundary)
   evaluate getting newer versions of our upstream open source projects.
   Ideally, we would monitor for (or be notified of) critical patches and
   quickly evaluate if we need to apply them and re-release OpenBMC.
   We also need a way to address flaws in OpenBMC itself (github issues)
   and provide critical security fixes.

   A mature system would have:
    - The OpenBMC team proactively monitoring for flaws
      announced by our upstream projects
    - a way to take flaw reports for OpenBMC
    - a system to categorize and prioritize flaws
    - developers mitigate flaws (workaround, configuration, bug fix)
    - the OpenBMC team announces flaws to its downstream users

 - ALC_LCD lifecycle definition
 - ALC_TAT tools and techniques
   This addresses things like C++ coding standards, the review process




 - ASE - security target evaluation

 - ATE - tests
 - ATE_COV coverage
 - ATE_DPT depth
 - ATE_FUN functional tests
 - ATE_IND independent testing

 - AVA - vulnerability assessment
 - AVA_VAN - vulnerability analysis
   Note that the results of a vulnerability analysis many include
   flaws that we do not want to be made publicly available until
   we address them.
   These would typically be published in the form of work items.


Assorted and uncategorized topics
=================================

Please add topics here!

Investigate https://bestpractices.coreinfrastructure.org/en
 - This links to:
   http://web.mit.edu/Saltzer/www/publications/protection/

See also: https://www.ibm.com/security/secure-engineering/
