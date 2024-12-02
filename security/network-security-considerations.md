# Network Security Considerations

This describes network services provided by OpenBMC-based systems, some threats
the BMC faces from its network interfaces, and steps OpenBMC takes to address
these threats.

This is only intended to be a guide; security is ultimately the responsibility
of projects which choose to incorporate OpenBMC into their project. If you find
a security vulnerability, please consider [how to report a security
vulnerability][].

[how to report a security vulnerability]:
  https://github.com/openbmc/docs/blob/master/security/how-to-report-a-security-vulnerability.md

Threats to the BMC are classified using the [CIA triad][]. All threat types are
significant; here is an example of each:

- Confidentiality: If an attacker can get data from the BMC, they may be able to
  chain other vulnerabilities to establish a covert information channel to get
  sensitive information from the host.
- Integrity: If an attacker can modify BMC settings or data, they may be able to
  gain additional access, and launch more attacks.
- Availability: If an agent can overwhelm the BMC's resources, either by
  accident or on purpose, the BMC will not be available to service its host
  (denial of service).

[cia triad]: https://en.wikipedia.org/wiki/Information_security#Key_concepts

This document is organized by how OpenBMC services connect to the network. The
general flow is:

- The BMC is presumed to have a network adapter. The security considerations of
  the NIC are important to the BMC security, but are outside the scope of this
  document.
- Network traffic then flows through the kernel, detailed below.
- Finally, connections flow to various OpenBMC services.

OpenBMC provides services on TCP and UDP ports. For example, the HTTPS protocol
on port 443 is used to provide REST APIs and serve Web applications. These
services are detailed below. Implicit is that all other ports are inactive.

OpenBMC also initiates network communications, for example, NTP, LDAP, etc.
These are covered with their associated functions.

## Kernel and ICMP messages

Network traffic is handled by the Linux kernel. The exact kernel and device
driver have security considerations which are important to BMC security, but are
better addressed by the Linux kernel community. You can learn which kernel and
patches are used from the kernel recipes typically found in the board support
packages for the BMC referenced by your machine's configuration. For example, in
the `https://github.com/openbmc/meta-aspeed` repository under
`recipes-kernel/linux/linux-aspeed_git.bb`.

Per [CVE 1999-0524][], responding to certain ICMP packets can give an attacker
more information about the BMC's clock or subnet, which can help with subsequent
attacks. OpenBMC responds to all ICMP requests.

[cve 1999-0524]: https://nvd.nist.gov/vuln/detail/CVE-1999-0524

General considerations for ICMP messages apply. For example, packet
fragmentation and packet flooding vulnerabilities.

It is sometimes useful to filter and log network messages for debug and other
diagnostic purposes. OpenBMC provides no support for this.

## General considerations for services

Several services perform user identification and authentication:

- Phosphor REST APIs
- Redfish REST API SessionService
- Network IPMI
- SSH secure shell

OpenBMC's [phosphor-user-manager][] provides the underlying authentication and
authorization functions and ties into IPMI, Linux PAM, LDAP, and logging. Some
of OpenBMC services use phosphor-user-manager.

[phosphor-user-manager]:
  https://github.com/openbmc/docs/blob/master/architecture/user-management.md

Transport layer security (TLS) protocols are configured for each service at
compile time, become part of the image, and cannot be changed dynamically. The
protocols which use TLS include:

- RAKP for IPMI.
- SSH for ssh and scp.
- HTTPS for Web and REST APIs.

Automated network agents (such as hardware management consoles) may malfunction
in a way that the BMC continuously gets authentication failures, which may lead
to denial of service. For example, a brief delay before reporting the failure,
for example, of one second, may help prevent this problem or lessen its
severity. See [OWASP Blocking Brute Force Attacks][].

[owasp blocking brute force attacks]:
  https://www.owasp.org/index.php/Blocking_Brute_Force_Attacks

Network agents may fail to end a session properly, which causes the service to
use resources to keep track of orphaned sessions. To help prevent this, services
may limit the maximum number of concurrent sessions, or have a session
inactivity timeout.

Services which are not required should be disabled to limit the BMC's attack
surface. For example, a large scale data center may not need a Web interface.
Services can be disabled in several ways:

1.  Configure OpenBMC recipes to build the unwanted feature out of the BMC's
    firmware image. This gives the BMC the advantage of a smaller attack
    surface.
2.  Implement something like the [Redfish ManagerNetworkProtocol][] properties
    for IPMI, SSH, and other BMC services, possibly by using shell commands like
    'systemctl disable ipmid' and 'systemctl stop ipmid'.

[redfish managernetworkprotocol]:
  https://redfish.dmtf.org/schemas/ManagerNetworkProtocol.v1_4_0.json

Network services should log all authentication attempts with their outcomes to
satisfy basic monitoring and forensic analysis requirements. For example, as
part of a real-time monitoring service, or to answer who accessed which services
at what times.

OpenBMC does not have a firewall.

Laws may require products built on OpenBMC to have reasonable security built
into them, for example, by not having a default password. See, for example, [CA
Law SB-327].

[ca law sb-327]:
  https://leginfo.legislature.ca.gov/faces/billTextClient.xhtml?bill_id=201720180SB327

## Services provided on TCP and UDP ports

### TCP port 22 - Secure Shell (SSH) access to the BMC

The Secure Shell (SSH) protocol is provided, including secure shell (ssh
command) access to the BMC's shell, and secure copy (scp command) to the BMC's
file system.

The default SSH server implementation is provided by Dropbear. All configuration
is at compile-time with defaults for:

- Authentication provided by Linux PAM, where methods include username and
  password, and SSH certificates (the `ssh-keygen` command).
- Transport layer security (TLS) protocols offered.

SSH access to the BMC's shell is not the intended way to operate the BMC, gives
the operator more privilege than is needed, and may not be allowed on BMCs which
service hosts that process sensitive data. However, BMC shell access may be
needed to provision the BMC or to help diagnose problems during its operation.

### TCP port 443 - HTTPS REST APIs and Web application

BMCWeb is the Web server for:

- The Redfish REST APIs.
- The webui-vue Web interface.
- The Phosphor D-Bus REST interface. And initiates WebSockets for:
- Host KVM.
- Virtual media.
- Host serial console.

The [BMCWeb configuration][] controls which services are provided.

General security considerations for HTTP servers apply such as given by [OWASP
Application Security][].

BMCWeb controls which HTTPS transport layer security (TLS) ciphers it offers via
compile-time header file `include/ssl_key_handler.hpp` in the
https://github.com/openbmc/bmcweb repository. The implementation is provided by
OpenSSL.

BMCWeb provides appropriate HTTP response headers, for example, in header file
`include/security_headers_middleware.hpp` and `crow/include/crow/websocket.h` in
the https://github.com/openbmc/bmcweb repository.

[bmcweb configuration]: https://github.com/openbmc/bmcweb#configuration
[owasp application security]:
  https://www.owasp.org/index.php/Category:OWASP_Application_Security_Verification_Standard_Project

#### REST APIs

BMCWeb offers three authentication methods:

1.  The Redfish SessionService, which takes a username and password and provides
    an X-Auth token.
2.  The Phosphor D-Bus REST interface '/login' URI, which takes a username and
    password and provides a session cookie. This method is deprecated in
    OpenBMC.
3.  Basic Access Authentication, which takes a username and password (often URL
    encoded like https://user:pass@host/...) in an "Authorization" request
    header, and returns no credentials. This method is deprecated by RFC 3986.

The username and password are presented to phosphor-user-manager for
authentication.

The first two methods create the same kind of session but return different
credentials. For example, you can create a Redfish session, and use your
credentials to invoke Phosphor D-Bus REST APIs. Note, however, that the X-Auth
tokens are required to use POST, PUT, PATCH, or DELETE methods.

General security considerations for REST APIs apply:
https://github.com/OWASP/CheatSheetSeries/blob/master/cheatsheets/REST_Security_Cheat_Sheet.md

Redfish provides security considerations in the "Security Detail" section of the
"Redfish Specification" (document ID DSP0266) available from
https://www.dmtf.org/standards/redfish.

#### The webui-vue Web application

General considerations for Web applications such as given by [OWASP Web
Application Security Guidance][] apply to OpenBMC. The webui-vue uses username
and password-based authentication, and REST APIs for subsequent access.

[owasp web application security guidance]:
  https://www.owasp.org/index.php/Web_Application_Security_Guidance

The web app also provides interfaces to use the host serial console, virtual
media, and host KVM.

### TCP port 2200

Access to the BMC's [host serial console][] is provided via the SSH protocol on
port 2200.

[host serial console]: https://github.com/openbmc/docs/blob/master/console.md

This uses the same server implementation as port 22, including the same TLS
mechanisms.

How the host secures its console (for example, username and password prompts) is
outside the scope of this document.

### TCP and UDP ports 5355 - mDNS service discovery

General security considerations for service discovery apply. For example,
described here: https://attack.mitre.org/techniques/T1046/

### UDP port 427 - SLP, Avahi

General security considerations for service discovery apply.

### UDP port 623 - IPMI RCMP

The IPMI network-facing design is described here:
https://github.com/openbmc/docs/blob/master/architecture/ipmi-architecture.md
and the implementation is described here:
https://github.com/openbmc/phosphor-net-ipmid. Note that host IPMI is outside
the scope of this document.

General security considerations for IPMI apply. For example, described here:
https://www.us-cert.gov/ncas/alerts/TA13-207A

OpenBMC implements RCMP+ and IPMI 2.0. The phosphor-user-manager provides the
underlying authentication mechanism.

Supported IPMI ciphers can be found in the code, for example, by searching for
function `isAlgorithmSupported`, or from the `ipmitool` command such as
`ipmitool channel getciphers ipmi`.

OpenBMC supports IPMI "serial over LAN" (SOL) connections (via
`impitool sol activate`) which shares the host serial console socket.
