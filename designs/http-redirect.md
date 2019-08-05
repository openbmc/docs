# Redirect HTTP to HTTPS
Author:
  Joseph Reynolds <josephreynolds1>

Primary assignee:
  None

Other contributors:
  None

Created:
  2020-01-10

## Problem Description
Web users are used to typing the IP address or hostname of the BMC into the
browser to load the web application.  The browser applies the `http` scheme,
resulting in an URL like `http://192.168.10.20/` or
`http://mybmc.example.com`.  However, no service is listening to the `http`
service port (`80`), so the browser tells the Web user the system cannot be
reached.  The user is confusued if they mistyped the URL or if the BMC is
running.

## Background and References

The nginx server was used as a reverse proxy by the meta-ibm layer and
redirected requests using the `http` scheme to the `https` scheme before use
of nginx< was discontinued around 2/2019 for the OpenBMC 2.7 release.  Although
this support is no longer part of the project and was not intended to meet
security recommendations or requirements, the [latest version of the nginx
configuration file][nginx config] shows how this was configured: see the
`server` section containing `listen 80`.

HTTP redirection is described here:
 - HTTP redirect is described in [RFC 6797][] section titled "HTTP Request
   Type".
 - [RFC 7538][] describes HTTP Status Code 308 (Permanent Redirect).
 - The [OWASP TLS Cheatsheet][] gives best practices for using HTTP and HTTPS,
   including redirection.  See also the [OWASP Unvalidated Redirects and
   Forwards Cheat Sheet][].
 - BMCWeb implements HSTS and [BMCWeb follows OWASP recommendations][]
 - See also [BMCWeb options][].

[nginx config]: https://github.com/openbmc/openbmc/blob/837851ae37a67b84f0f8c0fd310d33b5a731cc80/meta-ibm/recipes-httpd/nginx/files/nginx.conf#L49..L55
[RFC 6797]: https://tools.ietf.org/html/rfc6797#page-18
[RFC 7538]: https://tools.ietf.org/html/rfc7538
[OWASP TLS Cheatsheet]: https://cheatsheetseries.owasp.org/cheatsheets/Transport_Layer_Protection_Cheat_Sheet.html
[BMCWeb follows OWASP recommendations]: https://github.com/openbmc/bmcweb/blob/master/DEVELOPING.md#web-security
[OWASP Unvalidated Redirects and Forwards Cheat Sheet]: https://cheatsheetseries.owasp.org/cheatsheets/Unvalidated_Redirects_and_Forwards_Cheat_Sheet.html
[Redfish DSP0266]: https://www.dmtf.org/sites/default/files/standards/documents/DSP0266_1.10.0.pdf
[ManagerNetworkProtocol]: https://redfish.dmtf.org/schemas/ManagerNetworkProtocol.v1_6_0.json
[BMCWeb]: https://github.com/openbmc/bmcweb

## Requirements

When redirection is enabled, the BMC listens on port 80 and redirects GET and
HEAD requests for URI=`/` and URI=`/index.html` to the `https` scheme.
Appropriate HTTP response headers (see the OWASP reference) are sent.  Other
requests to port 80 may get HTTP status code 404 (`Not Found`) ideally with a
HTTP response body that indicates HTTPS is required or other HTTP status codes
as appropriate.

The [Redfish DSP0266][] spec (Chapter 13 "Security Details", section
"Authentication", subsection "HTTP redirect") requires HTTP status 404 (`Not
Found`) for Redfish URIs which require authentication.  Such URIs must not
redirect.

A requirement is that the BMC administrator can disable redirection.  There
need not be a supported interface; for example, shell commands run on the BMC
to disable redirection would satisfy this requirement.

## Proposed Design

The HTTP redirect feature shall not be present in the default image and shall
require an opt-in from the BMC system integrator (or similar configuration in
meta-data layers).  If the feature is present, the HTTP redirect service
should be enabled by default.

When HTTP redirect feature is enabled, start a new `systemd` service to listen
at port 80 and respond per the requirements above.

If it is desired to allow the BMC admininstrator to enable and disable the
HTTP redirect service, enhance the `[ManagerNetworkProtocol][]` `HTTP` property
`ProtocolEnabled` to allow PATCHing.

## Alternatives Considered

Initially this option was to be enabled by default.  The consensus is that the
complexity and risk outweighs the benefit for a few web users.

Initially, the design was to implement this as part of BMCWeb.  This design
was seen as difficult to implement, and harder to satisfy the requirement to
allow the BMC admin to disable.


## Impacts

1. There will be an extra port open in the BMC.
2. There will need to be additions to any test plan.
3. The BMC will now be hosting an unencrypted interface.

Investigate attacks based on application protocol, for example, injection
attacks based on the implementation (shell or `awk`, for example).

## Testing

Test with multiple browser implementations.

When the redirect feature is disabled, ensure requests using the `http` scheme
are dropped.

When the redirect feature is enabled:
 - Ensure correct redirects work as expected: `GET` and `HEAD` requests with
   different paths.  Validate response headers are as expected.
 - Ensure incorrect redirects work as expected: `POST` and `PUT` and
   unexpected methods, non-implemented URIs.
 - Attempt volume-based attacks with many requests in a short time, with large
   header content or large body content.  Ensure BMC recovers.

Perform white-box testing for security vulnerabilities (like script injection).

Ensure the HTTP redirection service can be enabled and disabled.
