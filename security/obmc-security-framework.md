# OpenBMC and the Common Criteria Security Framework

This is a comprehensive overview of all OpenBMC security topics
organized under the Common Criteria framework.

Common Critera (CC) provides a comprehensive framework
that covers all areas of security including
product design, development, deployment, and operation.
For more information, see
[Common Criteria for Information Technology Security Evaluation]
(https://www.commoncriteriaportal.org).
This document uses CC Version 3.1 Revision 5.

The reader is assumed to have basic familiarity with
 - the OpenBMC project
   (read the OpenBMC protection profile) and
 - the Common Criteria framework
   (Common Criteria pubs, Part 1, Chapter 7, ("General Model")).
In this document, references to the CC are like this: (CC:ASE_INT).

------------------------------------------------

## CC Part 1: Introduction and General Model


### Target of Evaluation (CC:TOE)

OpenBMC does not define a Target of Evaluation (TOE)
as defined by the Common Criteria framework and
is not pursuing a security evaluation.
The idea is that some other group uses OpenBMC
to build a BMC intended for use a secure application.
That group would define a Security Target (which includes OpenBMC)
and perform a security evaluation.
Such an evaluation will have many questions for the OpenBMC team, and
the material here is intended to help answer questions.


### OpenBMC Protection Profile (CC:PP)

The OpenBMC development team defines an OpenBMC Protection Profile
intended for use by the development team, downstream developers,
and security evaluators.

The Protection Profile talks about what security problems BMCs face
and defines security objectives and requirements.
Specifically, it addresses the following topics:

 - ASE_INT Introduction
 - ASE_CCL Conformance Claims - none yet
 - ASE_SPD Problem Definition
 - ASE_OBJ Security Objectives
 - ASE_ECD Extended Component Definition
 - ASE_REQ Security Requirements

The profile in currently a straw-man and 
has not been evaluated against the CC standards.


### Guidance for Operations

The OpenBMC team is collecting
security guidance for installation and operations.
File: obmc-security-publications.md


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

The Evaluation Assurance Level (EAL) expresses how well
the development team
implemented security-relevant processes
like code reviews and source code control, 
how cleanly they implemented security functions, and
how well they tested the security functions.

The OpenBMC team should target being able to support
security evaluations at assurance level 4 (CC:EAL4).
This level requires:
 - a clean architecture which
   OpenBMC provides by using module design including
   separate web servers, systemd, and dbus.
   The basic architecture and modular design must be documented.
 - a controlled development process, including source code control
   using git and github, and code reviews using gerrit.
 - secure delivery, provided in part by digitally signed firmware

Additionally, having an open source means that
anyone can find and report flaws to the development community
(and that attackers can more easily find flaws to exploit).


### Assurance Classes

The OpenBMC project addresses the following major assurance areas:

 - APE - Protection Profile evaluation.
   The OpenBMC protection profile has not been evaluated to CC standards.

 - ASE - Security target evaluation
   The OpenBMC does not define a security target and
   instead defines a protection profile.

 - ADV - Development
 - ADV_ARC Security architecture
   Example: https://github.com/openbmc/docs/MAINTAINERS
        https://github.com/openbmc/docs/blob/master/MAINTAINERS
 - ADV_FSP Functional spec
 - ADV_IMP Implementation representation
 - ADV_INT BMC security internals
 - ADV_SPM Security policy modeling
 - ADV_TDS BMC internal security design evaluation

 - AGD - Guidance
 - AGD_OPE operational user guidance
 - AGD_PRE preparative (installation) procedures

 - ALC - lifecycle support
 - ALC_CMC Configuration Management (CM) capabilities
 - ALC_CMS Configuration Management (CM) scope
 - ALC_DEL Delivery
 - ALC_DVS development security
 - ALC_FLR flaw remediation
 - ALC_LCD lifecycle definition
 - ALC_TAT tools and techniques

 - ASE - security target evaluation

 - ATE - tests
 - ATE_COV coverage
 - ATE_DPT depth
 - ATE_FUN functional tests
 - ATE_IND independent testing

 - AVA - vulnerability assessment
 - AVA_VAN - vulnerability analysis


This section is for topics you are not sure where to put
========================================================


Developer education
-------------------

This section lists topics for educating the OpenBMC development team.
These are required by the security assurance requirements.

Note: We need a way to ensure every developer
has read and understands the education.
Does github or gerrit offer any help here?
