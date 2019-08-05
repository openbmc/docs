____
# Design Guidelines - *Delete this section*

* Not all new features need a design document. If a feature can be
  contributed in a single reasonably small patchset that has little impact
  to any other areas, it doesn't need a design discussion and documentation.
* The focus of the design is to define the problem we need to solve and how it
  will be implemented.
* This is not intended to be extensive documentation for a new feature.
* You should get your design reviewed and merged before writing your code.
  However you are free to prototype the implementation, but remember that
  you may learn of new requirements during the design review process that
  could result in a very different solution.
* Your spec should be in Markdown format, like this template.
* Please wrap text at 79 columns.
* Please do not delete any of the sections in this template.  If you have
  nothing to say for a whole section, just write: None
* To view your .md file, see: https://stackedit.io/
* If you would like to provide a diagram with your spec, ASCII diagrams are
  required.  http://asciiflow.com/ is a very nice tool to assist with making
  ASCII diagrams.  Plain text will allow the review to proceed without
  having to look at additional files which can not be viewed in Gerrit.  It
  will also allow inline feedback on the diagram itself.
* Once ready for review, submit to gerrit and set the topic of the review
  to "design"
____

# Redirect HTTP to HTTPS
Author:
  Joseph Reynolds <josephreynolds1>

Primary assignee:
  None

Other contributors:
  None

Created:
  2019-08-05

## Problem Description
Web users are used to typing the IP address of the BMC into the
browser to load the web application.  The browser applies the HTTP
transport protocol, resulting in an URL like `http://192.168.10.20/`.
However, the BMCWeb web server is not listening to HTTP (port 80)
traffic, so the browser tells the Web user the system cannot be
reached.

BMCWeb should redirect Web Application users to HTTPS.

## Background and References
HTTP redirection is described here:
 - https://tools.ietf.org/html/rfc3986
 - https://tools.ietf.org/html/rfc7538
 - https://cheatsheetseries.owasp.org/cheatsheets/Unvalidated_Redirects_and_Forwards_Cheat_Sheet.html

BMCWeb options are described here: https://github.com/openbmc/bmcweb/blob/master/CMakeLists.txt.

## Requirements

Create a new option BMCWEB_ENABLE_HTTP_REDIRECT, ON by default.  When
this option and BMCWEB_ENABLE_STATIC_HOSTING are both ON, BMCWeb
listens at port 80 and redirects GET requests for URI=`/` and
URI=`/index.html` to HTTPS.  Other requests get HTTP status code 400
("Bad Request") with a response that indicates HTTPS is required.

## Proposed Design
When HTTP redirect is called for, listen at port 80 and respond
appropriately.

## Alternatives Considered
None.

## Impacts
None.

## Testing
With both options ON, ensure good and bad redirects work as expected.

