# OpenBMC Security Design

This document says how the OpenBMC development team
addresses security requirements in two areas:

 1. How the OpenBMC code satisfies the security requirements defined
    in the [OpenBMC Protection Profile](obmc-protection-profile.md).
    For each external interface (REST API, web server, IMPI, ssh, etc),
    this describes what the OpenBMC code does in response to requests,
    specifically the security architecture and design,
    It answers questions like: how can the security function be
    corrupted or bypassed.

 2. How the development team assures the security features are
    correctly coded, tested, and delivered.
    This addresses questions raised in the Common Criteria Part 3
    classes ADV (development), ALC (lifecycle support), and ATE (tests).

The level of detail in this document is specified by
the Common Criteria framework.
The OpenBMC project is working toward EAL4 (see CC Part 3) which
requires the basic security architecture and design
to be explained together with references into the C++ code.
Just follow the pattern and you'll have enough detail.

References to the Common Criteria publications
look like this, "(see CC part 1, ASE_SPD)," and refer to the
[Common Criteria for Information Technology Security Evaluation]
(https://www.commoncriteriaportal.org/cc)
publications, parts 1-3, Version 3.1 Revision 5.
When it is clear from the context, the class and family
(see CC part 1, "Terms and definitions") are used
without explicit references to the CC publications.
For example, ASE_SPD refers to "CC part 1, ASE_SPD."

Where the OpenBMC Protection Profile is generic,
this is specific to OpenBMC.
So be specific.

OpenBMC developers, this document is for you!
Read about BMC security issues in the Protection Profile,
determine if the profile needs to be updated,
decide how OpenBMC will implement security, and
document the design here.
Also, please correct my misconceptions and errors.


Team-wide requirements
----------------------
This section talks about security requirements
that are satisifed at the team level.
The section names come from various
Common Criteria assurance classes (see CC part 3).

ALC_DVS development security
The OpenBMC team restricts updates to the code by requiring
the "maintainer" of each code repository to approve the change.
Any such code updates either go through a peer review process
on gerrit, or come from a "git pull request" where
they are reviewed and go into the code
only after being reviewed and approved.


ALC_FLR flaw remediation.
The OpenBMC team uses an issues database to track problems.

ALC_LCD life cycle definition.
The OpenBMC team leadership tracks "epics" and "stories"





About the section headings
--------------------------

Each section below addresses one set of security requirements
from the OpenBMC Protection Profile.
The tags in these section headings should match the tags
in the security requirements.
Most of the section headings originate in CC part 2.
This feels like extra bookwork now, but doing so will save time later
when questions about security come up.

Example section headings
 - FIA_UAU.5.1 for the REST API on port 80 URI /v1
 - FIA_UAU.5.1 for the Web application on port 80 URI /
 - FIA_UAU.5.1 for the IPMI ACCESS via ???

You would find details about FIA_UAU ("User authentication") in CC part 2.

About the section content
-------------------------

Each section below should answer the following questions:
 - Which security requirement is being addressed?
   Please use the tag that matches the protection profile document
 - What is the basic approach?
   Are we using native Linux support?  PAM?
   A specific open source security product?  If so, what version?
 - How is the security configured?
   For example, what version of security protocols?
   Is password access allowed or are certificates required?
   Are these considered secure?
 - What module and function implement the security?
 - Where are the test cases for each security function?

Notes to developers:
 - Answering these questions shows that you've
   read and understood the security requirements,
   thought about how to implement them, and
   attempted to check your work.
   These practices put us miles ahead of other groups.
   And when you document this, it makes it easier for everyone else
   to find answers to their security questions.
 - Please question the requirements and make updates.
 - If you are doing any security work that is not described
   in the security requirements or security assurance requirements,
   thank you!  And please let us know.
 - We know you really use an emulator with root access, but
   please provide explanations from the point of view of
   a BMC installed in a data center.
   For a primer, read the OpenBMC Protection Profile sections that
   talk about the BMC's "operational environment" or its
   "operational requirements".


## OpenBMC functional requirements

The OpenBMC's external interfaces are...
 - Web application
 - REST API
 - RedFish REST API
 - IPMI
 - ssh
 - IPMI SOL


The OpenBMC Web Application
===========================

This section is not yet complete.


Overview.

OpenBMC offers a web application for users to control the OBMC.
See https://github.com/openbmc/phosphor-webui.

The OpenBMC web application allows users to point their browser to
the OBMC's IP address, port 80, enter their userid and password,
and begin using BMC functions.
This requires https and does not allow http.

TO DO: How does someone enable or disable the web application?

TO DO: What web server technology serves the application?  nginx?  apache?
What version, and how is it configured?

Basic mechanisms for user identification, authentication, and authorization
and how transport level security is accomplished.

Under the covers, the username and password are passed into the
OBMC's REST API which results in some kind of credential ???
that gets passed into every subsequent REST API request.
All of these requests are via https.
So the REST APIs provide the underlying mechanisms of
identifying users,
authenticating them,
and authorizing them.

TO DO: Explain where in the code the userid and password are
accepted, show how these are passed into the REST API and
then not logged or stored where a rogue agent can see them.
Then show how the credentials from the REST API are handled,
specifically that they are not stored where a rogue agent can get them.
How is logging off handled?

Is there a timeout, so someone who walks away from their browser
is eventually logged off?

Test cases?


The OpenBMC REST API
====================

This section is not yet complete.

Overview.

The OpenBMC provides a REST API described here:
https://github.com/openbmc/phosphor-rest-server

TLS:
The REST API accepts http requests on port 80, but
redirects to https on port 433.

Here is a high-level overview of
how the userid and password are handled,
and how the resulting credentials are handled.

User identification and authentication:
The REST API /login URI accepts POST requests
with JSON formatted username and password.
If not successful, an error message is returned
and no credentials are created.
If authentication is successful, it returns
credentials which are used for subsequent requests.

Here is an example of a successful login to the REST API.
This invokes the /login URI with
JSON formatted username/password credentials.
It returns a JSON formatted success message and
credentials, which are stored in the cjar file.
A more typical use would be from a web browser based application
and would NOT store the credentials in a file.

```
curl -L -c cjar -b cjar -k \
   -H "Content-Type: application/json" \
   -X POST https://$BMC_IP/login \
   -d "{\"data\": [ \"root\", \"0penBmc\" ] }"
{
  "data": "User 'root' logged in",
  "message": "200 OK",
  "status": "ok"
}
```


These REST APIs are used by the OpenBMC web application.



The OpenBMC IPMI server
=======================

This section is not yet complete.

Overview.

The OpenBMC responds to IPMI protocol.
See https://www.intel.com/content/www/us/en/servers/ipmi/ipmi-second-gen-interface-spec-v2-rev1-1.html
The implementation is here???:
https://github.com/openbmc/phosphor-net-ipmid

