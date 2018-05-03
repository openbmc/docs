# OpenBMC Security Design

This document is a stub.
I plan to document some real BMC Security requirements
in the BMC Protection Profile, then
document how the OpenBMC prject addresses those requirements.


This document says how the OpenBMC handles each
Security Functional Requirement listed in the 
[BMC Security Protection Profile](obmc-protection-profile.md).

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

to do

Example section headings

FIA_UAU.5.1 for the REST API on port 80 URI /v1

FIA_UAU.5.1 for the Web application on port 80 URI /

FIA_UAU.5.1 for the IPMI ACCESS via ???
