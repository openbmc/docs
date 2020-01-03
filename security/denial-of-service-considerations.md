# Denial of Service Considerations

Purpose: The BMC's goal is to be continuously available to its host and
management agents except during brief scheduled time windows.  Despite the
BMC's protected place within the server and data center, it is subject to
[denial of service][] (DoS).  The BMC has limited resources (CPU, memory,
storage) which makes it easy to overwhelm even by legitimate users.
Protecting against DoS is important, requires a variety of techniques, and
defences will never be complete.  This discusses considerations for the BMC's
defence against DoS scenarios.

Goals for OpenBMC DoS protection may include include:
 - Prevent DoS from naive attacks and from accidents from authenticated users.
 - Mitigate focused DoS attacks.  For example, use rate limiting to address
   volume-based attacks, apply security patches to prevent protocol-based
   attacks.
 - Detect and recover from DoS incidents.  For example, provide audit logs for
   incident response teams.

The BMC's threats come from its environment.  To frame the DoS discussion,
assume access as follows:
 - Management network.  Assume an attacker has unathenticated acccess to the
   BMC, or has hijacked an authenticated session.  Also, a legitimate user
   misusing authenticated access may accidentally overwhelm the BMC's
   resources.
 - Host system.  Assume the host may not be trusted, or the host may be
   compromised.
 - Physical access.  Assume the BMC is in a physically controlled environment.
   However, that access does not necessarily allow administrative control over
   the BMC, and OpenBMC may have features to detect physical access and
   prevent unauthorized modification.
 - Supply chain.  Assume newly discovered security bugs present in the BMC's
   operational firmware image.


##  Applicable guidance

This section references existing guidelines for DoS.  The general guidance
about preventing, mitigating, and recovering from DoS attacks applies to BMCs.
Much of the material for distributed DoS (DDoS) attacks also applies.  The
material about network-based agents may also apply to the BMC's host facing
interfaces.

General references:
 - Wikipedia entry for [denial of service][].  Introduces DoS attack and
   defense techniques.
 - [CERT-EU DDoS Overview and Incident Response Guide][].  Suggest that DoS
   protection is as important as other defences, for example, DoS as part of a
   coordinated attack.
 - [NIST 800-63b Digital Identity Guidelines][].  Suggests rate limiting as an
   effective defence against brute force attacks.
 - [NIST 800-160v2 Developing Cyber Resilient Systems][].  Stressed the
   importance of DoS defence, and suggests monitoring for DoS attacks and
   operating at diminished capacity when needed.
 - [OWASP DoS cheat sheet][].  Outlines topics such as input validation,
   access controls, rate limiting, etc.

[denial of service]: https://en.wikipedia.org/wiki/Denial-of-service_attack
[CERT-EU DDoS Overview and Incident Response Guide]: https://cert.europa.eu/static/WhitePapers/CERT-EU-SWP_14_09_DDoS_final.pdf
[NIST 800-63b Digital Identity Guidelines]: https://pages.nist.gov/800-63-3/sp800-63b.html
[NIST 800-160v2 Developing Cyber Resilient Systems]: https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-160v2.pdf
[OWASP DoS cheat sheet]: https://cheatsheetseries.owasp.org/cheatsheets/Denial_of_Service_Cheat_Sheet.html

OpenBMC specific references:
 - [OpenBMC Service Management][].  Can enable, disable, and configure which
   services are available.
 - [Dropbear SSH server options][].  Provides default behavior for
   rate-limiting options.
 - [BMCWeb developer information][].  Topics include performance, secure
   coding, and web security (OWASP).
 - Redfish implementation in BMCWeb
    - [Redfish spec][], document DSP0266, "Security" section.  Describes
      topics including: allowed unauthenticated access, session life cycle
      management, and session lifetime.
    - [Redfish school sessions][].  Mentions DOS attacks.
    - [Redfish spec][], dicuments DSP2014 and DSP0226.  Briefly discuss policy
      coordination needed to prevent accidental DoS.
    - Redfish Role and Privilege model (in DSP0266).  Assigning less powerful
      roles to authorized users can help prevent DoS by limiting access.
 - [OpenBMC User Management][].  Describes OpenBMC privilege roles (which map
   to Redfish Roles) and group roles (which control access to interfaces such
   as SSH, IPMI, Redfish, and Webserver), user creation and deletion,
   authentication and authorization flows.

[OpenBMC Service Management]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Control/Service/README.md.
[Dropbear SSH server options]: https://github.com/mkj/dropbear/blob/master/default_options.h
[BMCWeb developer information]: https://github.com/openbmc/bmcweb/blob/master/DEVELOPING.md
[Redfish spec]: https://www.dmtf.org/standards/redfish
[Redfish school sessions]: https://www.dmtf.org/sites/default/files/Redfish_School-Sessions.pdf
[OpenBMC User Management]: https://github.com/openbmc/docs/blob/master/architecture/user_management.md

References for similar and related services:
- [NGINX mitigating DoS attacks][].
- [OpenSSL vulnerabilities][].
- [OpenSSH security][].

[NGINX mitigating DoS attacks]: https://www.nginx.com/blog/mitigating-ddos-attacks-with-nginx-and-nginx-plus/
[OpenSSL vulnerabilities]: https://www.openssl.org/news/vulnerabilities.html
[OpenSSH security]: https://www.openssh.com/security.html


## Protection provided

Here are the OpenBMC areas which need DoS protection, together with a
discussion of the protection provided.  This begins by following [OpenBMC
interfaces currently in review][].

[OpenBMC interfaces currently in review]: https://gerrit.openbmc-project.xyz/c/openbmc/docs/+/27969.

### Management network

General considerations apply to the NIC, network services, and the operating
system.  For example, to keep the system updated with security fixes.

Discovery service.  General considerations apply.  For example, disable the
service when not needed.

BMCWeb HTTP server (port 443).  This a primary interface to operate and manage
the BMC.  It provides the HTTPS transport protocol and an authentication
service.  It is fully subject to volume-based, protocol-based, and
application-based attacks.  Rate limiting considerations:
 1. The Redfish MaxConcurrentSessions property is not implemented in BMCWeb.
    There is no limit to concurrent sessions.
 2. The Redfish SessionService SessionTimeout (for inactivity) is implemented
    as read-only with a value of 60.
 3. Redfish allows active sessions to remain indefinitely; there is no maximum
    session duration.  BMCWeb allows sessions to live indefinitely.  NIST
    Digital Identity Guidelines (NIST Special Publication 800-63B) recommands
    a maximum session duration of 30 days.

Network IPMI.  Can be used to perform DoS attacks, and can also provide
emergency access to the BMC during a DoS attack.

SSH: Port 22 accesses the BMC's shell, and port 2200 accesses the
serial-over-lan (SoL) connection.  Both require authentication to the BMC.
Access to the BMC's shell is not needed in a typical production environment
and can be disabled, SSH access to the host serial console may still be used.
OpenBMC uses the [Dropbear SSH server options][] which provides the following
limits by default:
 - MAX_UNAUTH_PER_IP 5
 - MAX_UNAUTH_CLIENTS 30
 - MAX_AUTH_TRIES 10
 - No limits for maximum concurrent authenticated sessions.

### Host-facing interfaces

The BMC's host interfaces are privileged, meaning only those with root access
to the host can use these interfaces.  Many of the interface are not
authenticated, meaning the BMC does not require additional credentials.

### Physical access

A controlled access environment is explicitly assumed where only trained and
trusted agents have access.  This access is needed to service the device (for
example, to replace a FRU).  Such service agents may need to the BMC's
function as part of their work (for example, to access and modify FRU data).

However, accidents can happen and malicious acts can be performed.  OpenBMC
has no defence against physical acts like powering off, damaging, or tampering
with (modifying) the BMC.  If equipped, the BMC may be able to detect physical
access while it is operational.  Technology such as TPMs or SecureBoot may be
able to detect when the BMC is running with unauthorized firmware.

### Supply chain

The BMC supply chain includes software and hardware components.  For example,
when a new DoS vulnerability is discovered, it may be important to quickly
learn about it, build the fix into a firmware image, and upodate the BMC.

### Logging and auditing

Logging and auditing may help incident response teams analyze DoS incidents.
In addition to the normal Linux and systemd logs, [BMCWeb Redfish logging][]
is implemented.

[BMCWeb Redfish logging]: https://github.com/openbmc/docs/blob/master/architecture/redfish-logging-in-bmcweb.md

### Unauthenticated access

OpenBMC allows unauthenticated and unprivileged access.  This access is the
typical vector to launch volume-based, protocol-based, and application-based
attacks to cause DoS.  The unauthenticated access to the BMC includes:

 - Any network traffic not intended to create or use an authenticated session.
 - Discovery service.
 - HTTP GET requests which do not require authentication, for example,
   `GET /redfish/v1/schema`.
 - Login attempts to BMCWeb.
 - Attempts to use BasicAuth with BMCWeb.
 - Login attempts to SSH (port 22 or 2200).
 - Login attempts to use the BMC's serial console.

For example, a malfunctioning aggregation service or management console may
repeatedly access the BMC, accidentally causing DoS the BMC.

### Authenticated access

Authenticated access includes both agents with legitimate authority such as
the sytem operator, and attackers who gain authenticated access.  In either
case, authentication allows for more detailed audit entries which can be used
to detect or prevent DoS conditions, and may help investigators understand
what happened.

Authorized agents have a legitimate need to consume resources such as network
bandwidth, BMC memory, flash storage, and CPU time.  However, such use may
cause DoS.  For example, a second concurrent KVMIP (IP-KVM) session may cause
DoS.  The Redfish MaxConcurrentSessions Manager Property may help, but it is
not fully implemented in BMCWeb.

User roles should be carefully considered, and the minimum access granted.

Access to the BMC's shell is very powerful, making all of the BMC's facilities
available, and represents a great DoS risk.  Such access is needed during
development and may be needed for advanced debug operations.  Access to the
shell should be carefully controlled.
