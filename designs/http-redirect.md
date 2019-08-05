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

