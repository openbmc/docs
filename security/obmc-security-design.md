# OpenBMC Security Design

This document says how OpenBMC addresses the security requirements of
each major security-enforcing interfaces as defined in the [OpenBMC
Protection Profile](obmc-protection-profile.md).

This explains the architecture, design, and configuration of each
interface, and says how it meets the security requirements.  Ideally:
 - it establishes a strong correspondence between the requirements and
   its design.
 - this workbook also includes a code walkthrough for major security
   functions, for example, handling certificates or passwords.
 - this workbook identifies test cases for each major security function.

The level of detail expected in a document like this is governed by
Common Criteria standards.  The idea is that more detail gives
security experts (which includes you) more information about how to
implement better security, for example, that the security function
cannot be corrupted or bypassed.  That is part of the "security
assurance" concept.  See the Common Criteria standard, Part 3,
"Evaluation Assurance Levels" (EAL) for more information.  The Common
Criteria documentation suggests that a development team using "sound
development practises" can achieve EAL3.  However, this document does
not currently meet any Common Criteria standards.

The templates below are from EAL4 which requires:
 - the basic security architecture and design to be explained.
 - code walkthrough for the parts that handle passwords, certificates,
   etc.
 - testcases identified for each security requirement.
Just follow the pattern and you'll have enough detail.

In this document, references to the Common Criteria publications
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


About the section headings
--------------------------

Each section below addresses one set of security requirements from the
OpenBMC Protection Profile.  The tags in these section headings match
the tags in the security requirements which also match the tags in the
Common Criteria publications, Part 2.  This practice maintains a tight
correspondence between the requirements and the implementation and
will save time later when questions about security come up.

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
   attempted to check your work by using code reviews and test cases.
   These practices put us in good shape for a formal security review.
   When you document this, it makes it easier for everyone else
   to find answers to their security questions.
 - Please question the requirements and make updates.
 - If you are doing any security work that is not described
   in the security requirements or security assurance requirements,
   thank you!  And please update the documentation.
 - Please answer the questions from the point of view of a BMC
   installed in a secure data center being operated in the manner
   prescribed by our guidance documentation
   (obmc-security-publications.md).
   For a primer, read the OpenBMC Protection Profile sections that
   talk about the BMC's "operational environment" or its
   "operational requirements".
 - You do not have to perform all of your testing on a secure system.
   For example, when you can justify that it does not affect the test
   results, it is okay to use an emulator such as QEMU and perform your
   test with root access.  Otherwise, you should test with minimum authority
   and just-below minimum authority.


## OpenBMC functional requirements

The OpenBMC's external interfaces are...
 - Web application
 - REST API
 - RedFish REST API
 - IPMI
 - ssh
 - IPMI SOL


Web server nginx
================

TO DO: FIX ME:
OpenBMC uses the NGINX HTTP server for requests that originate from
outside the local host.  The general approach is to comply with web
application standards from the [OWASP Application Security
Verification Standard (ASVS) Project](https://www.owasp.org/index.php/Category:OWASP_Application_Security_Verification_Standard_Project)
within the [Open Web Application Security Project
(OWASP)](https://www.owasp.org).

Mozilla offers guidance on secure TLS configurations [here](http://wiki.mozilla.org/Security/Server_Side_TLS).


The OpenBMC Web Application
===========================

This section is not yet complete.


Overview
--------

OpenBMC offers a web application for users to control the BMC.
See https://github.com/openbmc/phosphor-webui.

The OpenBMC web application allows users to point their browser to
the BMC's IP address, enter their userid and password,
and begin using BMC functions.
The server redirects http to https requests.

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

TO DO: Explain the following:
 - where in the code the userid and password are accepted.
 - show how these are passed into the REST API and then not logged or
   stored where a rogue agent can see them.
 - show how the session credentials from the REST API are handled.
 - state where the cookie is stored.
 - show how the cookie is used for subsequent access.
 - explain how a session ends via log out, or timeout.
 - If a framework is used for these activities, show how you use it.

Is there a timeout, so someone who walks away from their browser
is eventually logged off?

Test cases?


The OpenBMC REST API
====================

This section is not yet complete.

Overview
--------

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

Overview
--------

The OpenBMC responds to IPMI v2.0 protocol.
See https://www.intel.com/content/www/us/en/servers/ipmi/ipmi-second-gen-interface-spec-v2-rev1-1.html
The implementation is here???:
https://github.com/openbmc/phosphor-net-ipmid

TO DO:
We need to explain how OpenBMC implements IPMI in terms of
obsolete authentication mechanisms, limited SDR name spaces, etc.
The reasons for this should be stated in the OpenBMC Protection Profile
in terms of threats, security objectives, and security requirements.
The design should be documented here.
