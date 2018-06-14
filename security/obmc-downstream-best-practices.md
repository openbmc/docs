# OpenBMC downstream security best practices

This describes how to use OpenBMC in projects intended for higher
security applications.  Specifically, this addresses security risks
that can be introduced throughout the project's lifecycle spanning
development, build, provisioning, operation, and decommissioning.

Readers are assumed to be familiar with the tasks of developing and
using OpenBMC, described in [Setting up your OpenBMC
project](https://github.com/openbmc/openbmc/blob/master/README.md) and
[OpenBMC Documentation](https://github.com/openbmc/docs).

The [OpenBMC development team security practices](obmc-development-practices.md)
document describes security practices specific to OpenBMC that also
may apply to downstream development teams and should be followed.


## Development team practices

The [OpenBMC team development security practices](obmc-development-practices.md)
are used to develop OpenBMC, and similar practices should be used by
teams who use OpenBMC.  In particular, you should have secure
environments for development and at the BMC operational site.


## Obtain source code

You should securely retrieve all the source code you need to build
OpenBMC, including various openbmc repositories and its upstream
projects.

A typical development environment would fork stable branches and
populate private mirror sites.

Consider that the set of packages you need is controlled by your exact
configuration, including the TEMPLATECONF variable and how your Bitbake
layers and Bitbake recipes are set up.


## Modify and Configure

Here are some considerations when adapting OpenBMC code for your
application.

Ensure the Bitbake build process retrieves code only from your secure
environment (repositories and mirrors).

Ensure the Bitbake build process retrieves the code you modified (and
not an older version) by setting the SRC_URI and SRCREV variables in
your Bitbake recipes to the versions you intend.

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

Consider the security advice for every upstream project OpenBMC uses.

Configure OpenBMC recipes to use only the elements you need.

Review OpenBMC configuration items, including:
 - The systemd configuration.
 - The nginx reverse proxy server
 - The ipmid IPMI server
 - The net-impid server
 - The sshd server
 - The REST API server

The BMC hardware may have features with security related implications
that should be taken into consideration when creating a firmware
distribution because they increase the attack surface of the BMC.
These are identified in the [OpenBMC Server Security Architecture](obmc-securityarchitecture.md)
and are specific to the hardware model, for example ASPEED AST2500 or
Nuvoton W83795 series.  Example items include:
 - disable unneeded bridges between the BMC and host server
 - disable debug or trace modes

Configure the build process to digitally sign firmware images.  Note
that a build may result in multiple firmware images (UBI and static)
in different formats (compressed, etc.).

Consider configuring the build to use compiler security hardening
flags to protect against some kinds of vulnerabilities.


## Build

Consider performing the build process in a restricted environment to
protect the firmware image's integrity before it is signed and to
protect access to your signing capability.

Have a way to identify the exact version of the code you are building
so you can re-build from that same version plus fixes.

Consider that the Yocto project does not guarantee 100% reproducible
images.  That means images built from identical source may differ.

Ensure the source tree is clean before building.  For example,
validate that the `git status` command shows no modified files.  One
way to do this is to start from a clean directory and clone the
openbmc repository.

Consider that the Yocto project does not guarantee 100% reproducible
images.  That means images built from identical source may differ.

Digitally sign the firmware images.


## Provisioning

Provisioning includes obtaining hardware and preparing it for
operation, including installing firmware, post install configuration
(if needed), and testing.

Have a way for the installer to validate that the BMC install image
has not been modified.  For example, digitally sign the install image
and publish the public key in a secure area such as a website using
https, then establish a process to validate the install image
signature.

BMC hardware should be obtained from reputable sources and prepared
according to the manufacturer's instructions.  Especially consider
disabling unused functions and applying security measures.

If the host is not trusted (for example, when it is configured as a
bare-metal server and allowed to have untrusted software or BIOS),
consider configuring the BMC to limit communication between the host
and BMC.  For example, by only allowing white-listed IPMI commands.

OpenBMC offers several ways to update its firmware.  These are
described in the [OpenBMC code-update docs](https://github.com/openbmc/docs/tree/master/code-update).

OpenBMC offers two build image layouts: UBI and static.  With either
layout, an update can be performed from the OpenBMC's Linux command
line or from its REST API at URI `/xyz/openbmc_project/software/`.


## Operate

Many aspects of operational security are covered by the [OpenBMC
Server Security Architecture](obmc-securityarchitecture.md) and are
not repeated here.  Example topics include authentication,
authorization, and accountability such as auditing.  Example
interfaces include REST APIs, web application, and SSH signons.

The security architecture document also makes explicit security
assumptions in the form of non-goals.  Carefully consider each of
them.  To provide security, the operating environment must address
these.  Examples:
 - The OpenBMC code is not designed to withstand attacks from the
   internet.  The recommendation is to use OpenBMC on private, secure,
   monitored networks for higher security applications.
 - OpenBMC offers limited physical security features.  Physical security
   can be provided by some other means, for example, by being in a
   physically secured data center with controlled access.
 - Access the BMC's function may provided by IPMI over SMBus (IPMB) by
   unauthenticated agents.


## Repair

Repair refers to the BMC's ability to update its firmware to either
repair a corrupted image or to install a new version that corrects
flaws.  It does not refer to correcting the firmware of its host
computer: that is covered by the BMC's operation.

OpenBMC offers ways to update its overall firmware image as discussed
in the provisioning topic above.  OpenBMC does not offer a way to
patch its firmware image.  Operations such as modifying a
configuration file or replacing a Linux program are not supported.

Diagnostic data.  OpenBMC collects data about its operation in the
form of log files written by various Linux services.  These may be
requested by repair personnel performing diagnostic services.  Note
that they may contain sensitive information.


## Decommission

The OpenBMC firmware may store data that should not be disclosed after
the BMC is removed from service.  For example, access credentials and
information about the host.

The OpenBMC REST API has interfaces to erase some of this data, for
example, logs, dumps, and firmware images.
