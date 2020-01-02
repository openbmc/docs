# Applicable security standards

Purpose: This identifies security standards and best practices which apply
directly to OpenBMC and documents aspects of OpenBMC security which may be
needed to use OpenBMC in higher-security applications.

Audience: OpenBMC developers seeking standards and security reviewers seeking
answers to security assurance questions.


## Standards, best practices, and related work

This section lists standards, best practices, and related work which applies
directly to BMCs or which the OpenBMC project has referenced in the past.
Including an item here does not imply that OpenBMC complies with it.
- [Yocto security][].  Yocto security efforts flow directly into the OpenBMC
  project.
- [Open Web Application Security Project (OWASP)][].  This gives best
  practices for various aspects of OpenBMC's web server.
- [Redfish security standards][] (in publication DSP0266).  This gives
  standards for OpenBMC's Redfish implementation in BMCWeb.
- [CSIS Secure Firmware Development Best Practices][].  This describes
  multiple aspects of firmware security which apply directly to OpenBMC
  features.
- [OCP Security Project][].  This anchors work on multiple aspects of BMC
  security such as threat models, firmware update, and secure boot.

[CSIS Secure Firmware Development Best Practices]: https://github.com/opencomputeproject/Security/blob/master/SecureFirmwareDevelopmentBestPractices.md
[OCP Security Project]: https://www.opencompute.org/wiki/Security
[Open Web Application Security Project (OWASP)]: https://www.owasp.org/
[Redfish security standards]: https://www.dmtf.org/standards/redfish
[Yocto security]: https://wiki.yoctoproject.org/wiki/Security


## Security aspects

This section lists aspects of the OpenBMC project in terms of its development
process, security functions, security controls, testing, monitoring, and
auditing relevant to [information security][]
 - The [OpenBMC development process][].  Introduces aspects of OpenBMC's
   [software development security][].
 - [OpenBMC Interfaces][].  Introduces OpenBMC's interfaces which are input to
   most [Threat Model][] processes.
 - [Security response team][].  Introduces OpenBMC's security response team.

[information security]: https://en.wikipedia.org/wiki/Information_security
[OpenBMC development process]: https://github.com/openbmc/docs/blob/master/CONTRIBUTING.md
[software development security]: https://en.wikipedia.org/wiki/Software_development_security
[OpenBMC Interfaces]: https://gerrit.openbmc-project.xyz/c/openbmc/docs/+/27969
[Threat Model]: https://en.wikipedia.org/wiki/Threat_model
[Security response team]: https://github.com/openbmc/docs/blob/master/security/obmc-security-response-team.md
