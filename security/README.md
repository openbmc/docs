# OpenBMC security documentation

This is the root of OpenBMC security documentation.

The audience is the OpenBMC development team, projects that use
OpenBMC, and IT security professionals.  Readers are assumed to have
general familiarity with the firmware development process and with
OpenBMC's capabilities.

Security is broadly defined to mean avoiding negative impacts to the
confidentiality, integrity, and availability of the BMC's resources.
Resources include information stored by the BMC and its capability to
control itself and its host server.


## Security documentation stories

The documentation begins with stories that parallel OpenBMC
development activities.  The stories capture the essence of each
activity, identify security risks, and explain how OpenBMC addresses
those risks.  Conceptually, all other security work is rooted in these
stories.

The content and depth of the information presented here is primarily
intended to foster the development of secure OpenBMC implementations.
It may also be useful in formal security evaluation processes such as
[Common Criteria](https://www.commoncriteriaportal.org/).

 - [OpenBMC Server Security Architecture](obmc-securityarchitecture.md).
   This describes OpenBMC's security features including what OpenBMC does
   to protect the BMC against security threats.  Features include user
   authentication, transport level security, and secure boot.  Example
   components include:
    - the BMC and devices such as switches, sensors, and the host
    - IPMI and the IPMI server
    - network interfaces, web, and REST API servers

 - [OpenBMC development team security practices](obmc-development-practices.md).
   This highlights specific practices the development team uses to help
   ensure only reliable, reviewed, tested code goes into the OpenBMC
   project.  Example topics include:
    - work items and work flows
    - repository maintainers and the code review process
    - use of external servers such as GitHub, Gerrit, and Jenkins

 - [OpenBMC downstream security best practices](obmc-downstream-best-practices.md).
   This talks about how to protect the BMC from security threats that
   develop over its lifecycle spanning firmware development, building the
   install image, provisioning, operation, and decommissioning.  Example
   topics include:
    - retrieving and configuring OpenBMC and its upstream projects
    - building an OpenBMC install image
    - provisioning and operating the BMC


## Security work flow

The OpenBMC team has established the following processes:

 - [OpenBMC release planning security checklist](obmc-release-checklist.md).
   This is a checklist of security work items for OpenBMC release
   planning.
