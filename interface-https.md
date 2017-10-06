OpenBMC HTTPS Interface
==============================
Target audience:  administrators, OpenBMC developers

Overview
--------
Access to the BMC via HTTPS is one of the primary supported interfaces.  The
HTTPS interface provides all REST and Web-related interactions.  At the
beginning of the project, only the D-Bus/REST interface was exposed.  This was a
direct representation via REST/JSON for all D-Bus objects with a `/xyz` path.
The OpenBMC Project recognized that other web processes could also be made
available (web pages, redfish, etc.) and has implemented an architecture that
allows for new services to be added as machine features.  The machine feature
technique was chosen to avoid burdening machines that have no need of
additional web services.

The supported interfaces include the Redfish REST APIs and the web
application.  Note that the D-Bus/REST interface is internal and
backwards compatibility is *not* assured.

OpenBMC offers nginx and BMCWeb as externally-facing web servers.

The BMCWeb web server handles Redfish REST APIs, web applications, and
various other interfaces such as the D-Bus/REST APIs.  The set of
interfaces it offers can be configured by each distribution.

OpenBMC uses nginx as a reverse proxy server on the frontend to allow
web services to be tailored at the machine/company level.  The
following diagram shows HTTPS requests passing through such a proxy
and being handled by web servers internal to OpenBMC.

```
Request - https ---> Nginx (Reverse-Proxy)
                  |
                  |http
                  |
                 /|\
                | | `-> rest_dbus  127.0.0.1:8081
                |  `--> <app2>     127.0.0.1:8082
                  ...
                `-----> <appn>     127.0.0.1:808n
```

Note that maintaining separate processes for a reverse proxy, web
server, and downstream web services is more expensive than a single
process model.

External Interface
------------------
This interface is accessible via HTTPS. Authentication is performed via cookies.
Cookie expiration is configurable.  There is no direct access to the servers
proxied by Nginx since they will run with unsecured HTTP.

TODO: Document authentication for other types (such as basic token
oauth).  A good example is redfish has a session interface that takes
a post request to start a session. Would this be handled with a
sideband for shared session creation?  How would the whitelist be
managed?

###Reserved URIs

Supported URIs vary between OpenBMC distributions.

The default OpenBMC distribution supports the following URIs:
- `/login` , `/logoff` Authentication URIs.
- `/` path to web server files under rootfs /usr/share/www/**
- `/xyz` Path to D-Bus/REST JSON interface
- `/org` Deprecated D-Bus/REST JSON interface
- `/redfish` Path to Redfish interface
- `/api` Future path for D-Bus/REST JSON interface

Note that the /usr/share/www directory holds "static" web pages.  For
example, /usr/share/www/index.html can be served by a request for
https://bmc.ip.address/.  This directory can be populated by various
repos, including:
 - bmcweb (for the redfish REST interface)
 - phosphor-webui (for the web application)

The meta-openbmc-machines/meta-openpower/meta-ibm distributions
supports the following URIs:
- /**, as files in the BMC's root file system under /usr/share/www/**.
- /kvmws
- /redfish/
- /redfish/v1/AccountService/Accounts/
- /redfish/v1/AccountService/Accounts/<str>/
- /bus/
- /bus/system/
- /list/
- /xyz/<path>
- /bus/system/<str>
- /download/dump/<str>
- /bus/system/<str>/<path>

### Size Management
The D-Bus/REST interface is the recommended way to perform a firmware code
update.  The server needs to reserve enough space to allow for the maximum code
load possible.


Internal Interfaces
-------------------

All configuration of the reverse proxy should follow the requirements of the
software.  Nginx adds static pages at `/var/www/localhost/html`


### Logging

There are two types of logs: Error and Access.  Error logs are to be logged to
the journal via stderr.  Access logs are optional and disabled by default due to
the fact that the size can easily exceed traditional embedded flash file systems.  See the
[Nginx Logging guide](https://www.nginx.com/resources/admin-guide/logging-and-monitoring/)
for details.

Note: Nginx does not provide log management. Therefore, consider using
a tool such as `logrotate`.

### D-Bus

The reverse proxy server does not contain any D-Bus interfaces.  The ability to
add an SSL certificate would be a future candidate.


Implementation details
----------------------

Web services are provided by either [nginx](https://nginx.org/en) or
[bmcweb](https://github.com/openbmc/bmcweb).

Interfaces are defined in `meta-phosphor/common/meta-recipies/interfaces`.
Configuration of routes are partially defined in
`meta-phosphor/common/meta-httpd/nginx/files/nginx.conf`.
Nginx supports the ability to include other files in the configuration, allowing
individual web services to avoid patching `nginx.conf` directly.  Each new service
adds its own location paths in its own file in `/etc/nginx/sites-enabled/` i.e.
`443_redfish.conf` or `443_rest_dbus.conf`.

The exact list of URIs bmcweb supports are configured by C++
preprocessor symbols used by `src/webserver_main.cpp`.
The following diagram shows the mapping from the symbol name to the header file
that adds the URI.
```
   BMCWEB_ENABLE_STATIC_ASSETS -- webassets.hpp
     /**, as files in the BMC's root file system under /usr/share/www/**.
   BMCWEB_ENABLE_KVM -- web_kvm.hpp
     /kvmws
   BMCWEB_ENABLE_REDFISH -- redfish_v1.hpp
     /redfish/
     /redfish/v1/AccountService/Accounts/
     /redfish/v1/AccountService/Accounts/<str>/
   BMCWEB_ENABLE_DBUS_REST -- openbmc_dbus_rest.hpp
     /bus/
     /bus/system/
     /list/
     /xyz/<path>
     /bus/system/<str>
     /download/dump/<str>
     /bus/system/<str>/<path>
```

Note that the /usr/share/www directory is referenced by bitbake
recipes as `${D}${datadir}/www`.

### SSL Certificates

The BMC will generate a self-signed certificate if one does not exist already.
If the self-signed certificate is not desired, simply replace the one located at
`/etc/ssl/certs/nginx/cert.pem` when provisioning the BMC

To replace the certificate, you must `ssh` or `scp` into the BMC.
TODO: Update this section per
https://gerrit.openbmc-project.xyz/#/c/11840 (proposed REST API to
replace the certificate).

### Conditional web services

Add location sections in `meta-phosphor/common/meta-httpd/nginx/files/nginx.conf`.

### Testing

Minimum:

- [test\_rest\_interface.robot](https://github.com/openbmc/openbmc-test-automation/blob/master/tests/test_rest_interfaces.robot)
- FW update


### Future Features

- Adding more gevent services.  The gevent systemd service file would need to be
enhanced to support multiple instances.


Additional Reading
------------------
[RFC Adding Interfaces to OpenBMC](https://lists.ozlabs.org/pipermail/openbmc/2017-September/009036.html)
