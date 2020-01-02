# Applicable security standards

OpenBMC firmware is intended to be suitable for use in systems which process
sensitive information (like personal or financial data).  Such systems are
typically subject to various information security laws, regulations, and
standards.  This shows how such security schemes relate to OpenBMC.

OpenBMC developers (including projects built on OpenBMC) can use this to learn
about which standards may apply, and begin to learn about OpenBMC's compliance
efforts in terms of the development process, security requirements, security
functions, security controls, testing, monitoring, and auditing.  OpenBMC does
not claim compliance with items listed here.

This lists representative laws, regulations, standards, best practices, and
guides which may apply to OpenBMC and shows how each item relates to the
OpenBMC project.  In general, each item applies to one or more of
 - The [OpenBMC development process][].
 - OpenBMC functions (for example, REST API security).
 - The host system's management system (which includes the BMC) or firmware
   (which includes BMC firmware).

Items which relate to the host system:
- [PCI DSS][]
- [HIPAA security rule][]
- [FISMA][]
- [FedRAMP][]

General items:
- [NIST Computer Security][]
- [Open Compute Platform (OCP)][]
- [IBM Secure Engineering][]
- [Microsoft Security Engineering][]
- [Common Criteria][]

Items for the BMC:
- [General Data Protection Regulation (GDPR)][]
- [Open Web Application Security Project (OWASP)][]
- [Redfish security standards][] (in publication DSP0266)
- [CSIS Secure Firmware Development Best Practices][]
- [Yocto security][]
- [Linux Foundation Core Infrastructure Initiative (CII)][]


[OpenBMC development process]: https://github.com/openbmc/docs/blob/master/CONTRIBUTING.md
[PCI DSS]: https://www.pcisecuritystandards.org/
[HIPAA security rule]: https://www.hhs.gov/hipaa/for-professionals/security/index.html
[FISMA]: https://csrc.nist.gov/Projects/risk-management
[FedRAMP]: https://www.fedramp.gov/
[NIST Computer Security]: https://csrc.nist.gov/publications/sp
[Open Compute Platform (OCP)]: https://www.opencompute.org/wiki/Security
[IBM Secure Engineering]: https://www.ibm.com/security/secure-engineering/
[Microsoft Security Engineering]: https://www.microsoft.com/en-us/securityengineering
[Common Criteria]: https://www.commoncriteriaportal.org/
[General Data Protection Regulation (GDPR)]: https://gdpr.eu/
[Open Web Application Security Project (OWASP)]: https://www.owasp.org/
[Redfish security standards]: https://www.dmtf.org/standards/redfish
[CSIS Secure Firmware Development Best Practices]: https://www.opencompute.org/documents/csis-firmware-security-best-practices-position-paper-version-1-0-pdf
[Yocto security]: https://wiki.yoctoproject.org/wiki/Security
[Linux Foundation Core Infrastructure Initiative (CII)]: https://www.coreinfrastructure.org/
