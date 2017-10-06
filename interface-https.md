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
available (web pages, redfish, etc) and has implemented an architecture that
allows for new services to be added as machine features.  The machine feature
technique was chosen to not require those machines without the need for
additional web services to be burden by them.

The OpenBMC now maintains a reverse proxy server on the frontend to allow
tailoring at the machine/company level of web services.


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


External Interface
------------------
This interface is accessible via HTTPS. Authentication is performed via cookies.
Cookie expiration is configurable.  There is no direct access to the servers
proxied by Nginx since they will run with unsecured HTTP.

###Reserved URIs

- `/login` , `/logoff` Authentication URIs.
- `/` 	Current path to D-Bus/REST JSON interface, and future path to webserver
- `/xyz` Path to D-Bus/REST JSON interface
- `/org` Deprecated D-Bus/REST JSON interface
- `/redfish` Future path to any Redfish interface
- `/api` Future path for D-Bus/REST JSON interface

*NOTE: Standard URI expectations are to expose a web interface (if there is one)
off of the root URI.  The OpenBMC Project will be moving to that model and it is
recommended to change management tools to the new /api URI now*
 
### SSL Certificates
The BMC will generate a self-signed certificate if one does not exist already.

### Size Management
The D-Bus/REST interface is the recommended way to perform a firmware code
update.  The server needs to reserve enough space to allow for the maximum code
load possible.


Internal Interfaces
-------------------

All configuration of the reverse proxy should follow the requirements of the
software.  Nginx adds static pages at `/var/www/localhost/html`


### Logging

There are two types of logs; Error and Access.  Error logs are to be logged to
the journal via stderr.  Access logs are optional and disabled by default due to
the fact that the size can easily exceed traditional embedded flash file systems.  See the
[Nginx Logging guide](https://www.nginx.com/resources/admin-guide/logging-and-monitoring/)
for details.  Note, Nginx does not provide log management therefore consider using
a tool such as `logrotate` would be advised.


### D-Bus

The reverse proxy server does not contain any D-Bus interfaces.  The ability to
add an SSL certificate would be a future candidate.


Implementation details
----------------------

Interfaces are defined in `meta-phosphor/common/meta-recipies/interfaces`.
Configuration of routes are defined in `meta-phosphor/common/meta-httpd/nginx/files/nginx.conf`.

### SSL Certificates
If the self-signed certificate is not desired simply replace the one located at
`/etc/ssl/certs/nginx/cert.pem` at compile or runtime.  Runtime requires logging
in to the BMC as no D-Bus interface today replaces it.

### Conditional web services

Add location sections in `meta-phosphor/common/meta-httpd/nginx/files/nginx.conf`.

### Testing

Minimum:

- [test\_rest\_interface.robot](https://github.com/openbmc/openbmc-test-automation/blob/master/tests/test_rest_interfaces.robot)
- FW update

### Future Features

- Nginx supports the ability to include other files in the configuration.  It is
possible to add a directive to the `nginx.conf` file to point to a location to
pick up URIs to proxy.  This would allow individual web services to not have to
patch the `nginx.conf` directly.  This could be done by replacing the existing
location section in the `nginx.conf` with `include /path/to/locations/443_*.conf`.
Then each new service would add its own location paths in its own file i.e.
`443_redfish.conf` or `443_rest_dbus.conf`.

- Adding more gevent services.  The gevent systemd service file would need to be
enhanced to support multiple instances.


Additional Reading
------------------
[RFC Adding Interfaces to OpenBMC](https://lists.ozlabs.org/pipermail/openbmc/2017-September/009036.html)
