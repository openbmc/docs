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

The content and depth of the information presented here is intended to
support formal security evaluation programs such as [Common Criteria](https://www.commoncriteriaportal.org/).


## Security documentation stories

The documentation begins with stories that parallel OpenBMC
development activities.  The stories capture the essence of each
activity, identify security risks, and explain how OpenBMC addresses
those risks.

 - TO DO: Functional security.  This describes OpenBMC's security
features, that is, what OpenBMC does to protect the BMC against
security threats.  Examples: secure boot, user authentication, web
server security, transport level security.

 - Development team security practices.  This highlights specific
practices the development team uses to help ensure only reliable,
reviewed, tested code goes into the OpenBMC project.

 - Using OpenBMC in higher security projects.  This talks about how to
protect the BMC from security threats that develop over its lifecycle
spanning firmware development, building the install image,
provisioning, operation, and decommissioning.

 - TO DO: Security implementation.  This highlights the design and
implementation of OpenBMC's user interfaces to help assure that each
is properly implemented and tested.
