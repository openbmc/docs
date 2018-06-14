# Using OpenBMC in higher security projects

This describes how to use OpenBMC in projects intended for higher
security applications.  Specifically, this addresses security
assurance topics concerned with security vulnerabilities that can be
introduced throughout the project's lifecycle spanning development,
build, provisioning, operation, and decommissioning.

Readers are assumed to be familiar with the tasks of developing and
using OpenBMC, described in [Setting up your OpenBMC
project](https://github.com/openbmc/openbmc/blob/master/README.md) and
[OpenBMC Documentation](https://github.com/openbmc/docs).


## Development team practices

The [OpenBMC team development security practices](obmc-development-practices.md)
are used to develop OpenBMC, and similar practices should be used by
teams who use OpenBMC.  In particular, you should have secure
environments for development and the BMC deployment site.


## Obtain source code

This section builds on topics introduced in [Setting up your OpenBMC
project](https://github.com/openbmc/openbmc/blob/master/README.md).

The reader is assumed to understand the basic structure of OpenBMC
repositories, Yocto and OpenEmbedded layers, and how Bitbake recipes
retrieve projects from the network.  This understanding is needed to
avoid introducing harmful code into your project.

Consider retrieving all the source code you may need to modify.  For
example, clone various repositories under `github.com/openbmc`.

Consider retrieving all source code, for example, so you can have a
stable version to inspect and build from.  Consider creating a mirror
site in your development environment so your build process will not
have to retrieve packages from the internet.

Retrieve source code only via secure communications.  The repositories
under `github.com/openbmc` can only be accessed by the git and https
protocols, but of which are secure.  Use similar measures when
retrieving other open source projects.

Determine which projects you need only after you configure Yocto to
build for a specific handware because that affects what recipes and
packages it retrieves.  Note that any configuration of Bitbake layers
and recipes may result in a different set of packages.  Note that
Bitbake does not use all of the recipes in each layer.  Use the
`bitbake -e` command in your development environment to determine what
recipes and packages Bitbake is using.  If you are building for
multiple configurations, perform these steps for each of them.


## Modify and Configure

Here are some considerations when adapting OpenBMC code.

Ensure the Bitbake build process retrievesthe code you modified (and
not an older version) by setting the SRC_URI and SRVREV variables in
your Bitbake recipe to the version you intend.

Use secure engineering practices.  Consider adopting practices
described in the [OpenBMC Development Team Practices](to do:reference).

Implement security advice from the Yocto project, including:
 - the [Yocto security page](https://wiki.yoctoproject.org/wiki/Security)
 - the [Yocto meta-security layer](https://git.yoctoproject.org/cgit/cgit.cgi/meta-security/about/)
 - section [Making Images More Secure](https://www.yoctoproject.org/docs/current/dev-manual/dev-manual.html#making-images-more-secure)
   in the [Yocto Project Development Tasks Manual](https://www.yoctoproject.org/docs/current/dev-manual/dev-manual.html).

The Yocto security topics include:
 - setting root passwords and creating additional users
 - turning off debug modes
 - turning off unneeded services
 - tightening Linux configuration settings
 - configuring SELinux Mandatory Access Control (MAC)

Learn about the security consideratons for each project Bitbake
retrieves, and consider its security advice about configuration, etc.

Configure OpenBMC recipes to use only the elements you need.

Review OpenBMC configuration items, including:
 - The systemd configuration.
 - The nginx web server reverse proxy
 - The ipmid IPMI server
 - The REST API server

Configure the build process to digitally sign firmware images.
TO DO: As a post-build step in a recipe?

Control which version of packages Bitbake retrieves ans build from by
using the SRC_URI and SRCREV variables in each recipe, or some other
means.


## Build

Consider perfoming the build process in a more secure environment
to protect access to the capability to digitally sign firmware images.

Determine which version of the code to build.  For example, by using a
git commit identifier or a git tag.  The version of packages the build
process pulls in for that version should similarly be controlled as
described in the section above.

Have a way to archive the version of the code being built so you can
inspect it later and use it to build patches.  This includes all
packages Bitbake retrieves.

Ensure the source tree is clean before building.  For example, the
`git status` command shows no modified files.  One way to do this is
to start from a clean directory and clone the openbmc repository.

The build step should digitally sign the firmware image.  Note that
OpenBMC offers multiple build image layouts (UBI and static) in
different formats (compressed, etc.).


## Provisioning

Have a way for the installer to validate that the BMC install image
has not been modified.  For example, digitally sign the install image
and publish the public key in a secure area such as a website using
https, then establish a process to validate the install image
signature.

TO DO: Does OpenBMC have a way to validate its firmware images'
digital signatures?  Using what keys?

TO DO: Secure boot: There should be a way to boot the BMC so that if
the digital signature is not valid, the BMC detects this and responds
appropriately.

OpenBMC offers several ways to update its firmware.  These are
described in the [OpenBMC code-update docs](https://github.com/openbmc/docs/tree/master/code-update).

OpenBMC offers two build image layouts: UBI and static.  With either
layout, an update can be performed from the OpenBMC's Linux command
line or from its REST API at URI `/xyz/openbmc_project/software/`.

TO DO: Discuss how trust is established with devices such as
hardware management consoles (HMC) and LDAP servers.


## Operate

Most aspects of operational security are covered by the functional
security documentation and are not repeated here.  Example functional
security topics include authentication, authorization, and
accountability such as auditing.  Example interfaces include REST
APIs, web application, and SSH signons.

The OpenBMC code is not designed to withstand attacks from the
internet.  The recommendation is to use OpenBMC on private, secure,
monitored networks for higher security applications.

OpenBMC offers limited physical security features.  Physical security
can be provided by some other means, for example, by being in a
physically secured data center with controlled access.

TO DO: Check this: OpenBMC has a security feature to help detect
physical tampering.  It offers an LED that lights up when the case is
opened, and can only be turned off by administrator action.

TO DO: check this: OpenBMC has a secure boot feature which is
activated by a jumper on it circuit board.  ??? What happens when the
jumper is physically moved back to the "not secure boot" position?


## Repair

Repair refers to the BMC's ability to update its firmware to either
repair a corrupted image or to install a new version that corrects
flaws.  It does not refer to correcting the firmware of its host
computer; that is covered by the BMC's operation.

OpenBMC offers ways to update its overall firmware image.  These are
discussed in the provisioning topic.

TO DO: Check this: OpenBMC firmware updates preserve the data stored
about BMC operation and about the host.

TO DO: Forward compatability.  Check this: Will new OpenBMC releases
be forward compatable?  Meaning, a newer version of OpenBMC works with
older versions of the BMC's data.

OpenBMC does not offer a way to patch its firmware image.  Operations
such as modifying a configuration file or replacing a Linux program
are not supported.  Ways to update the firmware image are described in
the provisioning section of this document.

Diagnostic data.  OpenBMC collects data about its operation in the
form of various log files written by Linux services.  These may be
requested by repair personnel performing diagnostic services.  Note
that they may contain sensitive information.

TO DO: OpenBMC developers will want to diagnose OpenBMC or OpenPOWER
firmware problems by getting access to the BMC, for example by using
the ssh command, and entering Linux shell commands.  How can we limit
access while giving them the tools they need?

TO DO: OpenBMC developers will want to provide test-fixes and
data-collection instrumentation by installing custom-built programs to
replace functionality, or re-configuring.  What provisions does
OpenBMC have in this area?  For example, such changes on a locked down
system may cause the BMC to fail to secure boot, resulting in an
inability to boot the host server.


## Decommission

The OpenBMC firmware may store data that should not be disclosed after
the BMC is removed from service.  For example, access credentials and
information about the host.

OpenBMC has interfaces to erase some of this data, for example, data
stored on partitions accessible to the BMC.
TO DO: Enumerate those interfaces.

TO DO: Is there a way to fully erase the firmware image or the entire
storage area?  What is the recommendation?
