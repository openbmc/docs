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

The primary use of this document is to identify all OpenBMC security
topics to help improve OpenBMC security over time.


## Organization

The documentation begins with stories that parallel OpenBMC
development activities.  The stories capture the essence of each
activity, identify security risks, and explain how OpenBMC addresses
those risks.  The result is a systematic approach to security and a
platform to address what is being done.

 - The [OpenBMC life cycle story](obmc-security-lifecycle.md)
   describes security topics related to OpenBMC's lifecycle spanning
   development, build, test, provisioning, operation, repair, and
   decommissioning.  The main idea is to identify all places where
   security risks can enter these activities and what OpenBMC is doing to
   address those risks.  Functional security items are described in a
   separate story.

 - TO DO: Functional security describes what the BMC does to protect
   itself against security threats.

 - TO DO: Security implementation stories describe the design and
   implementation of the major user interfaces and security features.
   This documentattion helps answer questions about the implementation
   and how it was tested.
