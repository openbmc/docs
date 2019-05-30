# Network Security Assurance Guide

This describes network services provided by OpenBMC-based systems,
threats to the BMC from its network interfaces, and how OpenBMC
answers each threat.

Threats to the BMC are classified using the [CIA triad][].  All threat
types are significant; here is an example of each:
 - Confidentiality: If an attacker can get data from the BMC, they may
   be able to chain other vulnerabilities to establish a covert
   information channel to get sensitive information from the host.
 - Integrity: If an attacker can modify BMC settings or data, they may
   be able to gain additional access, and launch more attacks.
 - Availability: If an agent can overwhelm the BMC's resources, either
   by accident or on purpose, the BMC will not be available to service
   its host (denial of service).

[CIA triad]: https://en.wikipedia.org/wiki/Information_security#Key_concepts

This document is organized by how OpenBMC services connect to the
network.  The general flow is:
 - The BMC is presumed to have a network adapter.  The security
   considerations of the NIC are important to the BMC security, but
   are outside the scope of this document.
 - Network traffic then flows through the kernel, detailed below.
 - Finally, connections flow to various OpenBMC services.

OpenBMC provides services on TCP and UDP ports.  For example, the
HTTPS protocol on port 443 is used to provide REST APIs and serve Web
applications.  These services are detailed below.  Implicit is that
all other ports are inactive.

OpenBMC also initiates network communications, for example, NTP, LDAP,
etc.  These are covered with their associated functions.


## Kernel and ICMP messages

Network traffic is handled by the Linux kernel.  The exact kernel and
device driver have security considerations which are important to BMC
security, but are better adddressed by the Linux kernel community.
You can learn which kernel and patches are used from the kernel
recipes typically found in the board support packages for the BMC
referenced by your machine's configuration.  For example, in the
`https://github.com/openbmc/meta-aspeed` repository under
`recipes-kernel/linux/linux-aspeed_git.bb`.

Per [CVE 1999-0524][], responding to certain ICMP packets can give an
attacker more information about the BMC's clock or subnet, which can
help with subsequent attacks.  OpenBMC responds to all ICMP requests.
It would be possible to drop ICMP packets of type 13 or 14 and/or code
0 either with an internal or external firewall, or a kernel patch.

[CVE 1999-0524]: https://nvd.nist.gov/vuln/detail/CVE-1999-0524

General considerations for ICMP messages apply.  For example, packet
fragmentation and packet flooding vulnerabilities.

It is sometimes useful to filter and log network messages for debug and
other diagnostic purposes.  OpenBMC provides no support for this.


## General considerations for services

Several services perform user identification and authentication:
 - Phosphor REST APIs
 - Redfish REST API SessionService
 - Network IPMI
 - SSH secure shell

OpenBMC's [phosphor-user-manager][] provides the underyling
authentication and authorization functions and ties into IPMI, Linux
PAM, LDAP, and logging.  Some of OpenBMC services use phosphor-user-manager.

[phosphor-user-manager]: https://github.com/openbmc/docs/blob/master/user_management.md

Automated network agents (such as hardware management consoles) may
malfunction in a way that the BMC continuously gets authentication
failures, which may lead to denial of service.  Having a brief delay
before reporting the failure, for example, of one second, may help
prevent this problem or lessen its severity.  TODO: Is there a CVE or
CWE for this?

Network agents may fail to end a session properly, which causes the
service to use resources to keep track of orphaned sessions.  To help
prevent this, services may limit the maximum number of concurrent
sessions, or have a session inactivity timeout.

Services which are not required should be disabled to limit the BMC's
attack surface.  For example, a large scale data center may not need a
Web interface.  Services can be disabled in several ways:
 1. Configure OpenBMC recipes to build the unwanted feature out of the
    BMC's firmware image.
 2. Use shell commands like 'systemctl disable ipmid' and 'systemctl
    stop ipmid' when the BMC is being provisioned.  The effectiveness
    of these approaches has not been evaluated.
 3. Implement something like the [Redfish ManagerNetworkProtocol][]
    properties for IPMI, SSH, and other BMC services.

[Redfish ManagerNetworkProtocol]: https://redfish.dmtf.org/schemas/ManagerNetworkProtocol.v1_4_0.json

Network services should log all authentication attempts with their
outcomes to satisfy basic monitoring and forensic analysis
requirements.  For example, as part of a real-time monitoring service,
or to answer who accessed which services at what times.

### Firewall

OpenBMC does not use an integral or kernel-based firewall.

Blocking unused ports, such as with a firewall, would provide a
defense in depth against an attacker attempting to start an
unauthorized network service.

Firewalls have been used to disable features such as manufacturing or
debug functions.  However, there are better ways to disable these
features, discussed above.


## TCP port 22 - Secure Shell (SSH) access to the BMC

Secure shell (SSH) access to the BMC's login shell is provided.
OpenBMC's default shell is `bash`.

The default SSH server implementation is provided by Dropbear.
All configuration is at compile-time and defaults to:
 - Authentication methods include username and password, and SSH
   certificates (the `ssh-keygen` command).  Handled by Linux PAM.
 - Transport-level security (TLS) protocols offered.

SSH access to the BMC's shell is not the intended way to operate the
BMC, gives the operator more priviledge than is needed, and may not be
allowed on BMCs which service hosts that process sensitive data.
However, BMC shell access may be needed to provision the BMC or to
help diagnose problems during its operation.


## TCP port 443 - HTTPS REST APIs and Web application

BMCWeb is the HTTP server for:
 - The Redfish REST APIs.
 - Web applications such as phosphor-webui.
 - The Phosphor REST APIs (deprecated).
And initiates WebSockets for:
 - Host KVM video.
 - Virtual media.
 - Host serial console.

The [BMCWeb configuration][] controls which services are provided.

[BMCWeb configuration]: https://github.com/openbmc/bmcweb#configuration

General security considerations for HTTP servers apply.

BMCWeb controls which HTTPS transport-level security (TLS) ciphers it
offers via compile-time header file `include/ssl_key_handler.hpp` in
the https://github.com/openbmc/bmcweb repository.  The implementation
is provided by OpenSSL.

BMCWeb provides appropriate HTTP response headers, for example, in
header file `include/security_headers_middleware.hpp` in the
https://github.com/openbmc/bmcweb repository.

### REST APIs

BMCWeb offers two authentication methods:

1. The Redfish SessionService APIs accept username and password and
provides an X-Auth token.

2. The phosphor-rest '/login' URI accepts username and password and
provides a session cookie.  This method is deprecated in OpenBMC.

The username and password are presented to phosphor-user-manager for
authentication.

Both methods create the same session but return different credentials.
For example, you can create a Redfish session, and use your
credentials to invoke phosphor-rest APIs.  Note, however, that the
X-Auth tokens are required to use APIs which modify the BMC's state.

General security considerations for REST APIs apply:
https://github.com/OWASP/CheatSheetSeries/blob/master/cheatsheets/REST_Security_Cheat_Sheet.md

Redfish provides security considerations in the "Security Detail"
section of the "Redfish Specification" (document ID DSP0266) available
from https://www.dmtf.org/standards/redfish.

### The phosphor-webui Web application

General considerations for Web applications such as given by [OWASP
Web Application Security Guidance][] and other organizations apply to
OpenBMC.  The phosphor-webui uses username and password-based
authentication, which it forwards to the Redfish SessionService, and
all subsequent access is via X-Auth tokens.

[OWASP Web Application Security Guidance]: https://www.owasp.org/index.php/Web_Application_Security_Guidance

The web app also provides interfaces to use the host serial console,
virtual media, and host KVM.

## TCP port 2200

OpenBMC provides access to the host serial console via the SSH
protocol on port 2200.  For example, `ssh -p 2200` creates a secure
connection to the console.

This uses the same server implementation as port 22, including the
same TLS mechanisms.

How the host secures its console (for example, username and password
prompts) is outside the scope of this document.

## TCP and UDP ports 5355 - mDNS service discovery

General security considerations for service discovery apply.

## UDP port 427 - SLP, Avahi

General security considerations for service discovery apply.

## UDP port 623 - IPMI RCMP

General security considerations for IPMI RCMP apply.

OpenBMC implements RCMP+ and IPMI 2.0.  The phosphor-user-manager
provides the underlying authentication mechanism.

OpenBMC supports IPMI "serial over lan" (SOL) connections (via
`impitool sol activate`) which shares the host serial console socket.
