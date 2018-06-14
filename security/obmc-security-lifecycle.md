# DRAFT OpenBMC lifecycle security

This describes security topics related to OpenBMC's lifecycle spanning
development, build, test, provisioning, operation, repair, and
decommissioning.  The main idea is to identify all places where
security risks can enter these activities and what OpenBMC is doing to
address those risks.

The primary use of this document is to identify and improve practices
and processes over time.

This document contains information about OpenBMC the OpenBMC
development team.  Guidance is provided for downstream development
teams who intend to use OpenBMC in higher security applications.


## OpenBMC Development

### Project identification

The OpenBMC home page at https://www.openbmc.org identifies project
page https://github.com/openbmc.  In this document, the project refers
to all git repositories under https://github.com/openbmc, contributors
to those repositories, participants in project meetings, and related
support.  This does not include upstream projects (that OpenBMC uses),
and does not include downstream development projects (that use
OpenBMC).

There are multiple source code repositories under
https://github.com/openbmc.  The primary repository is
https://github.com/openbmc/openbmc which contains the top-level build
instructions.  It must be configured for a specific target
architecture before it can be used to create an install image.  Based
on this configuration, the build process pulls in various repositories
from https://github.com/openbmc and from other open source projects,
explained later in this section.

This document refers to repositories under https://github.com/openbmc
as "OpenBMC repositories".

### Overview

OpenBMC is open source and changes can be submitted by anyone.
However, all OpenBMC repositories are under source code control.  The
code review, continuous testing, and source code maintainer processes
accept only high-quality changes for merging into the project.  The
OpenBMC project's [contributing guidelines](https://github.com/openbmc/docs/blob/master/contributing.md)
page explains this.

### Code review process

As described in the contributing guidelines, anyone can develop
material (code, documentation, etc.) intended for OpenBMC repositories
in their private repositories.  When ready, that material is submitted
for review as a git commit pushed to the OpenBMC Gerrit server.

The commit is expected be a logically complete change and have a
commit message which contains:
 - a short descriptive title,
 - an explanation of changed items,
 - a "tested" tag that explains how the change was tested (when
   applicable),
 - a "signed-off-by" tag identifying the responsible contributor,
 - a "resolves" tag which identifyes which issue it resolves (when
   applicable).

The Gerrit code review server is at
https://gerrit.openbmc-project.xyz.  Its configuration is controlled
by the Gerrit maintainer.

A Jenkins server builds and tests each commit and reports its finding
back to the Gerrit review process.  See the test topic for more
information.

Gerrit validates the quality of C++ and Python code (formatting rules,
etc.) via `clang` and `pycodestyle` and reports its finding back to
the code review process.

The list of reviewers is initially assigned by the agent who pushed
the change to Gerrit, usually the committer.  Each OpenBMC repository
has a `MAINTAINERS` file which lists required reviewers who are
subject matter experts.  Those reviewers add additional reviews as
appropriate.

The OpenBMC development team conducts the review.

The Gerrit review tool allows reviewers to comment specific lines of
code, for comments to be seen by all reviewers in context, and for
re-worked material to be seen in context with previous versions.

The review results in the proposed change being accepted, rejected for
reworking, or abandoned.  Each repository has a small list of
maintainers who accept only high-quality material and merge it into
the respective OpenBMC repository.

An alternate code review process is offered for those who cannot use
Gerrit.  Typically this is used for Linux kernel updates and not for
general use.  Changes can be submitted in the form of Linux patches to
the team's email list: openbmc@lists.ozlabs.org.  The email list
allows discussions of the proposed change.  TO DO: If the patch is
accepted, one of the maintainers pushes it into the OpenBMC
repository.

### Development team education

The development team consists of anyone who submits material to the
project (whether that material is accepted or not).  There are no
education requirements.  The OpenBMC team maintains contributing
guidelines which gives best practices including input checking, source
code formatting, etc.

The development review team is rooted in the `MAINTAINERS` file in the
OpenBMC repositories.  That list is controlled by each repository's
maintainer.  Those reviewers are highly-skilled software engineers who
are aware of software and hardware security practices.

### OpenBMC repository updates

The OpenBMC repositories are configured to allow accept changes (in
the form of git commits) only from a small list of maintainers.

The maintainers accept only high-quality, reviewed, and approved
commits from the Gerrit server.  They also accept approved patches
from the openBMC mailing list.

The Gerrit commits are merged into the OpenBMC repositories using
git's fast-forward merge strategy which avoids merge conflicts.  Any
commits with merge conflicts must be reworked (`git rebase`) before
they are merged.

Gerrit commit messages can include a "resolves" directive.  When used,
this mechanism closes the corresponding github issue when the gerrit
commit is merged into the github repository.

### Use of open source projects and toolchains

The OpenBMC build process pulls in other open source projects,
including Yocto, OpenEmbedded, and many others.  The exact list
depends on how the downstream developer configured OpenBMC to build.
(The exact list can be found by using the `bitbake -e` command).

The quality of these projects matters because they can introduce
security vulnerabilities into the OpenBMC project.  The open source
packages are from well-known sources, specific releases are used, and
the cryptographic hashes are obtained from trusted sources and
validated during the build process.  All such hashes are stored in the
OpenBMC build repository (https://github.com/openbmc/openbmc, or as
cloned) which means the exact set of packages OpenBMC picks up can be
controlled.

The OpenBMC development team reviews the packages it uses for known
security vulnerabilities, and updates accordingly.

### External development tools

OpenBMC relies on web-based tools such as Gerrit, Jenkins, and Github
to provide their own security.  The OpenBMC developers who have
administration access to these sites protect that access and rely on
the tools to enforce their security requirements.

### Secure development environment

OpenBMC contributors are not expected to have a secure development
environment.  See the code review process.

OpenBMC maintainers are authorized to make changes to the source code
and various code review and testing processes.  They generally have
secure development environments.  TO DO: Can we strengthen this
statement?

### Work items

Work items are recorded in https://github.com/openbmc/openbmc/issues.
These generally include defects, features, and wishes.  Anybody can
read, create, and comments on issue.  The OpenBMC maintainers can
assign owners and close issues.

The gerrit merge process also closes issues when it merges a commit
that contains a "resolves" directive.


## Downstream Development

Downstream development refers to any project that uses OpenBMC as part
of its function.  A simple application would be building a firmware
image from the example OpenBMC configuration and loading it onto a
simulated BMC device such as QEMU.

Although downstream development is outside the control of the OpenBMC
team, various aspects relate to security assurance and so are
addressed here.  Note that the OpenBMC team necessarily performs some
of the functions of a downstream development team as part of its
testing activity.

The primary use case for a downstream development team is to:
 - Create a secure development environment which means controlled
   access to the hardware, network, storage, command-line interfaces,
   etc.
 - Copy (`git clone` aka fork) an OpenBMC release and place that under
   source code control in a private repository into the secure
   development environment.  Then only use that copy for validation and
   further changes.
 - Optionally create private mirror sites within the secure
   development environment to contain all the packages pulled in by
   OpenBMC.
 - Modify OpenBMC code and configure its behavior.  For example:
   - Userids: OpenBMC defaults to a Linux image that has the root
     userid set to a well-known password.
   - Web server: OpenBMC uses Nginx as a reverse proxy to serve web
     apps and REST APIs.
   - Digitally sign firmware images during the build process.

Many of the subsequent activities such as testing amd building are
performed by both the OpenBMC development team and by the downstream
development team.  These are ddescribed in later sections.


## Test

OpenBMC functional test cases are part of each repository.  In
addition, a Jenkins server at https://openpower.xyz runs a continuous
integration test which compiles, starts up a QEMU-based BMC, and
runs a test suite against Gerrit-based changes.

The Jenkins build and test server has a maintainer who controls its
configuration.

The OpenBMC team has an active community including downstream
development teams that use OpenBMC and submit fixes back into the
OpenBMC project and its upstream projects (such as the Linux kernel).

Downstream development teams are expected to have their own testing
agenda.


## Release

The OpenBMC project is planning regular releases to begin November
2018, and continuing twice per year following the [Yocto project
release schedule](https://wiki.yoctoproject.org/wiki/Planning).

TO DO: Items for consideration include:
 - How are OpenBMC releases identifies?  Web page?
 - Will upstream projects and release levels be identified?  (Yocto
   helps provide that list.)
 - Will the security flaws fixed in each release be identified?

See the repair topic for details about compatability between releases.

Downstream development teams are free to have their own release
schedule which may involve more frequent security updates.


## Build

OpenBMC is a toolkit for creating firmware.  It provides a build
process, but does not offer pre-built firmware images.  The
information in this section is provided as guidance for downstream
development teams.

OpenBMC's build process is based on [Yocto](https://wiki.yoctoproject.org)
which uses OpenEmbedded and BitBake to produce Linux install images.

### Secure build environment

A secure build environment should be established for building the
installation image (similar to the secure development environment).  A
secure build environment is needed to help ensure the integrity of the
source code and build artifacts, and to restrict access to the private
keys for digital signatures.

Secure repository: The git repositories should be on secure, backed up
storage.  The build environment should have secure access to that
repository.

Secure mirror sites: Optional private mirror sites should be secured
the same way as the git repositories.

The OpenBMC Jenkins server uses a secure build environment.

### Build process

This is not intended to be a complete description of the build
process, only to highlight its security-relevant aspects.

The build process should be performed in the secure build environment
using a clean source tree (`git status` shows no modified files) using
a known git commit id from the secure repository.  The build process
retrives other code (described above) from other OpenBMC repositories,
the internet, or from secure mirror sites.

The build process directs the native compiler to build a new gcc-based
tool chain, called the software development kit (SDK).  The SDK then
builds the remaining artifacts from source.

A digital signing step should be added to the automated build process
to sign the install images.  The public key should be published in a
secure place and associate with the install image.


## Provisioning

This section is intended as guidance for downstream development teams.
This refers to provisioning the BMC.  Using the BMC to provision the
host server is part of the BMC's operation.

BMC devices should be provisioned according the manufacturer's
specifications.

TO DO: Valid digital signature.  There should be a way to validate
the digital signature of the BMC's bootable firmware image.

TO DO: Secure boot: There should be a way to boot the BMC so that if
the digital signature is not valid, the BMC detect this and responds
appropriately.

For re-provisioning, which means updating the firmware image on a
working BMC, OpenBMC offers several ways to update the firmware.
These are described in the [OpenBMC code-update docs](https://github.com/openbmc/docs/tree/master/code-update).

OpenBMC offers two build image layouts: UBI and static.  With either
layout, an update can be performed from the OpenBMC's Linux command
line or from its REST API at URI `/xyz/openbmc_project/software/`.

TO DO: Discuss how trust is established with devices such as
hardware management consoles (HMC) and LDAP servers.


## Operation

This section is intended as guidance for downstream development teams.

Most aspects of operational security are covered by the functional
security documentation and are not repeated here.  Example functional
security topics include authentication, authorization, and
accountability such as auditing.  Example interfaces include REST
APIs, web application, and SSH signons.

The use case for a higher security installation includes the BMC
installed on a private, secured, and monitored network.


## Repair

This section is intended as guidance for downstream development teams.

OpenBMC offers ways to update its overall firmware image.  These are
discussed in the re-provisioning topic.

TO DO: Check this: OpenBMC firmware updates preserve the data stored
about BMC operation and about the host.  Generally, the updates are
forward compatable which means newer version of OpenBMC work with
older versions of the data.

OpenBMC does not offer a way to patch its firmware image.  Operations
such as modifying a configuration file or replacing a Linux program
are not supported.  Ways to update the firmware image are described in
the provisioning section of this document.

TO DO: OpenBMC developers will want to diagnose OpenBMC or OpenPOWER
firmware problems by getting access to the BMC and entering Linux
shell commands.  How can we limit access while giving them the tools
they need?

TO DO: OpenBMC developers will want to provide test-fixes and
data-collection instrumentation by installing custom-built programs to
replace functionality, or re-configuring.  What provisions does
OpenBMC have in this area?  For example, such changes on a locked down
system may cause the BMC to fail to secure boot, resulting in an
inability to boot the host server.


## Decommissioning

This section is intended as guidance for downstream development teams.

The OpenBMC firmware may store data that should not be disclosed even
after the BMC is removed from service.  This data may include
information about:
 - how to authenticate to the BMC, such as:
    - Linux passwords
    - ssh keys
 - how to authenticate to services the BMC uses such as:
    - LDAP servers
    - the BMC's host server
 - the host server:
    - list of parts
    - event logs

OpenBMC has interfaces to erase some of this data, for example, data
stored on partitions accessible to the BMC.
TO DO: Enumerate those interfaces.

TO DO: Is there a way to fully erase the firmware image or the entire
storage device?
