# OpenBMC Security Design

This document is a stub.
I plan to document some real BMC Security requirements
in the BMC Protection Profile, then
document how the OpenBMC prject addresses those requirements.


This document says how the OpenBMC handles each
Security Functional Requirement listed in the
[BMC Security Protection Profile](obmc-protection-profile.md).

Where the BMC Protection Profile is generic,
this is specific to OpenBMC.
So be specific.

OpenBMC developers, this document is for you!
Read about BMC security issues in the Protection Profile,
determine if the profile needs to be updated,
decide how OpenBMC will implement security, and
document the design here.


About the section headings
--------------------------

Each section below addresses one set of security requirements
from the BMC Protection Profile.
The tags in these section headings should match the tags
in the security requirements.
This feels like extra bookwork now, but doing so will save time
when questions about security come up.


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
 - What part of the project


Yes, you really have to dig into the Security Requirements section.
The requirements all have a tag.
Find the same tag in this file, and answers question like
which security products are used,
how they are configured, and
what code implements them.
If everyone does this,
we'll be ready to address any questions about OpenBMC security.

Developers: 
We know you really use an emulator with root access, but
please provide explanations from the point of view of
a BMC installed in a data center.
For a primer, read the BMC Protection Profile sections that
talk about the BMC's operational requirements.


Example section headings

FIA_UAU.5.1 for the REST API on port 80 URI /v1

FIA_UAU.5.1 for the Web application on port 80 URI /

FIA_UAU.5.1 for the IPMI ACCESS via ???



The OpenBMC Web Application
===========================

This section is not yet complete.


Overview.

OpenBMC offweerrs a web application for users to control the OBMC.
See https://github.com/openbmc/phosphor-webui.

The OpenBMC web application allows users to point their browser to
the OBMC's IP address, port 80, enter their userid and password,
and begin using BMC functions.
This requires https and does not allow http.

TO DO: How does someone enable or disable the web application?

TO DO: What web server technology serves the application?  nginx?  apache?

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


The OpenBMC REST API
====================

This section is not yet complete.

Overview.

The OpenBMC provides a REST API described here:
https://github.com/openbmc/phosphor-rest-server

....

Note that this is used by the OpenBMC web application.

TO DO: Provide a high-level overview of
how the userid and password are handled,
and how the resulting credentials are handled.


The OpenBMC IPMI server
=======================

This section is not yet complete.

Overview.

The OpenBMC responds to IPMI protocol.
See https://www.intel.com/content/www/us/en/servers/ipmi/ipmi-second-gen-interface-spec-v2-rev1-1.html
The implementation is here???:
https://github.com/openbmc/phosphor-net-ipmid

