# OpenBMC security documentation

OpenBMC security documentation starts here.
This is the root of the OpenBMC security documentation.

Preamble
--------

The OpenBMC project is not seeking security certification but
wants to be in a position for projects built from OpenBMC technology
to be able to pursue formal security certification using the
[Common Criteria](https://www.commoncriteriaportal.org) model.
That means two things for the OpenBMC team:
1. We want OpenBMC to comply with BMC security standards
   (which presupposes there are BMC security standards).
   Doing so makes it possible for folks who deploy an off-the-shelf
   OpenBMC to secure their installation with little effort.
2. We want basic high-level design information about
   how OpenBMC complies with the security standards
   (to the extent it does so).
   Doing so gives assurance that the OpenBMC team gave 
   security considerations the attention it deserves.
   
To this end, we propose a set of BMC security protection profiles
and a workbook for documenting the high level security designs.


Documentation
-------------

 - [BMC Protection Profile](obmc-protection-profile.md)

   This defines the BMC security requirements.
   The document describes what a BMC is,
   the security problems it faces,
   and defines security objectives and requirements.
   The intent is to develop this into an industry standard, so
   the document is in the form of a Common Criteria Protection Profile.

   This strawman draft focuses on a relatively smaller security problem,
   but still a meaningful one, focusing on basic user authentication
   and transport level security mechanisms.
   This is intentional to garner industry support.
   We can always enhance the standard in a later revision.

   The OpenBMC project should meet the requirements
   specified in the BMC Protection Profile and
   may address additional security as described in
   the protection profile modules below.

 - BMC Protection Profile Modules:

   This section extends the basic BMC Protection Profile
   to address various additional security problems.
   The idea is that all BMCs should meet the basic requirements, then
   meet zero or more of the standards listed in this section.

   Each document in this list is a BMC Protection Profile Module
   that references the basic BMC Protection Profile.
   It further develops the security problem and
   provides extended objectives and requirements.
   These requirements can be used either 
   as a basis for making formal security claims or 
   as input to BMC design process.

   Including a module in this list does not mean
   OpenBMC complies with its security requirements, 
   only that the security problem is known to us.

    + obmc-protection-profile-auditing.md

      Requires the BMC to audit access to its functions.

    + obmc-protection-profile-authorization.md

      Requires the BMC to allow the admin to restrict user authority
      to three basic roles: admin, update, and read only.

    + obmc-protection-profile-authentication.md

      Requires the BMC to allow the admin to restrict the types of 
      authentication the BMC will accept.  For example, it can
      disallow weak passwords or require certificate-based authentication.

    + obmc-protection-profile-tls.md

      Requires the BMC to reject communication attempts based on
      weak transport-level security protocols.
      For example, IPMI version 1.0.

    + obmc-protection-profile-internals.md

      Requires the BMC to have its internal controls secured to prevent
      accidental changes from the admin, defects, or a rogue agent.

 - [OpenBMC Security Design](obmc-security-design.md)

   This says how OpenBMC meets the BMC security requirements.
   The document references the objectives and requirements
   listed in the BMC Protection Profile and
   talks about how the OpenBMC project addresses each item.


How Common Criteria documentation is structured
-----------------------------------------------

The OpenBMC documentation is structured for
a formal Common Criteria security evaluation.
The information security community has a particular way of doing things,
and its easier if we just follow along.

It starts with a "Protection Profile" which
introduces what a BMC is,
talks about the security problems it faces,
defines what the BMC should do to protect itself, and
gives formal requirements for BMC security.
A typical Protection Propfile document
belongs to the overall BMC community and not to any particular vendor.
As you can imagine, this document has a wide range of audiences,
so we have to follow the expected format.
See the handy guide below.

As of 1H1018 the BMC Protection Profile is just being written,
so we have this opportunity to influence the BMC industry.

The next document is the "Security Target" which
makes a formal claim that OpenBMC meets security requirements.
The sections are:
 - Exactly what device and software is being tested, including
   which version, how it is configured,
   which features are enabled.
   Sorry, we cannot claim that "OpenBMC meets the standards"
   but have to make a separate claim
   for each separate board and each separate release.
 - Exactly what security claims are being made, and referring
   to specific security standards and Protection Profiles.
 - Sections for the security problem, security objectives, and
   security requirements (same as in the Protection Profile),
   stating how the product handles each item.
 - An overview of how you demonstrate
   (like test cases and design documentation)
   that your product meets the security requirements.

A Security Target *would be* needed if we were making a formal claim.
We're not going to make a security target document.
Until we start a formal evaluation,
the development will use an informal definition,
typically using a dirty build running on the qemu emulator
or whichever OpenPower model they happen to use.
That will get us a very long way toward meeting security requirements.

The final pieces of documentation talk about how OpenBMC
addresses each security requirement.
It's a workbook for us developers to keep track of
the basic security decisions we've made and
find out what still needs to be done.
The idea is that the workbook gets updated as part of
each security relevant commit.
It is yours to structure as you please.


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
   ptotect against all of the threats identified above.
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

There's a lot more detail in the document.
If you are new to formal security documentation,
do yourself a favor: start at the beginning and
don't skip around.
The material builds from one section to the next.

Note it is standard industry practice to write
a Protection Profile such as the
BMC Protection Profile as a generic specification,
not specific to any particular BMC vendor.
This is so all BMC manufacturers use the same standards and
makes it easier to compare security between similar devices.
