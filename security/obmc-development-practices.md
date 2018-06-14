# OpenBMC development team security practices

This document addresses security risks that can be introduced during
the OpenBMC development process.

The reader is assumed to be familiar with the OpenBMC [contributing
guidelines](https://github.com/openbmc/docs/blob/master/contributing.md).
which gives guidelines for coding, testing, and reviewing changes
to merge into the project.


## Open source contributions

OpenBMC is open source.  All source code is available to the public
and anyone can submit changes to a review process which results in
accepting or abandoning the change.


## Code review process

Code reviews are conducted as described in the contributing guidelines
and are only accepted (merged) when a maintainer is satisfied with the
quality of the change.

The Gerrit review tool helps provide high quality reviews in the form
of comments to be seen by all reviewers in context, for re-worked
material to be seen in context with previous versions, etc.


## Maintainers

As described in the [contributing guidelines](https://github.com/openbmc/docs/blob/master/contributing.md),
the project maintainers are responsible for maintaining the quality
and integrity and therefore the security of the project.

The [maintainer workflow](../maintainer-workflow.md) describes the
steps maintainers are expected to take before accepting changes.

Maintainers are expected to maintain secure development environments.


## Secure development environments

A secure development environment is broadly defined as one where only
the intended developers have access to resources including hardware,
network, storage, command-line and web interfaces.  The idea is to
protect the confidentiality and integrity of the environment.

Secure environments are the basis for securely performing tasks such
as code reviews, merging code into the master repositories, building,
and testing code.  These tasks are performed by the OpenBMC team as
part of its quality assurance work.


## Development team education

Developers are expected to have basic software development skills
including open source participation skills.  OpenBMC-specific
education is provided in the docs repository and continues in the form
of code reviews and communication channels.

The maintainers generally also have formal education in software or
firmware engineering and keep up with evolving security practices.

Here is security specific education for OpenBMC and upstream projects:
 - [OpenBMC security](https://github.com/openbmc/openbmc/security)
 - [Yocto security](https://wiki.yoctoproject.org/wiki/Security)
 - [OpenSSH security](https://www.openssh.com/security.html)


## OpenBMC repository updates

Each OpenBMC repository accepts changes (in the form of git commits)
only from the maintainers it is configured to accept, and that list is
controlled by the repository maintainer.

The Gerrit commits are merged into the OpenBMC repositories using
git's fast-forward merge strategy which avoids merge conflicts.  Any
commits with merge conflicts must be reworked (`git rebase`) before
they are merged.

Gerrit commit messages can include a "resolves" line which identifies
an OpenBMC GitHub issue to close when the Gerrit commit is merged.

See the [maintainer workflow](../maintainer-workflow.md) for other
ways openbmc repositories can be updated.


## Use of open source projects and toolchains

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
cloned) as Bitbake SRCREV variables.  This mechanism can control the
exact set of packages OpenBMC retrieves.

The OpenBMC development team reviews the packages it uses for known
security vulnerabilities, and updates accordingly.


## External development tools

OpenBMC relies on web-based tools such as Gerrit, Jenkins, and Github
to provide their own security.  OpenBMC developers have administration
access to these sites protect that access, control the site's
configuration, and rely on the tools to enforce their security
requirements.

GitHub offers 2-factor authentication (2FA) for user authentication.
GitHub also offers a way for project owners to require 2FA as a
prerequisite to edit the project.

OpenBMC relies on the security of the open source gcc-based toolchain,
including a large set of utility programs.  This is also known as the
Software Development Kit (SDK).  These projects are retrieved as
securely as any other open source project


## Work items

Project management uses work items to track features and defects to
help ensure OpenBMC code is the way it was intended.  Specifically,
work items help identify which security items are merged.

Work items are recorded as Git issues in
https://github.com/openbmc/openbmc/issues and in other openbmc
repositories.  These generally include defects, features, and wishes.
Anybody can read, create, and comment on issues.  The OpenBMC
maintainers can assign owners, tag, and close issues.

The responsible disclosure model is to not create public work items
for high-impact vulnerabilities (easier to reproduce and have larger
impacts) until a mitigation is available, for example, a tested and
merged fix.

The Gerrit code review tool also closes github issues when it merges a
commit that contains a "resolves" line.


## Downstream Development

The OpenBMC team produces BMC source code.  Apart from a basic test
suite, it does not build images and it does not provision or maintain
any BMC.

OpenBMC team members, however, do coordinate with teams that consume
OpenBMC code and perform these activities.  OpenBMC provides guidance
for such teams in the [OpenBMC downstream security best practices](obmc-downstream-best-practices.md)
guide.


## Test

Testing is a core part of security assurance.

OpenBMC functional test cases are part of the "getting started" guide
an in each repository.  In addition, a Jenkins server at
https://openpower.xyz runs a continuous integration test which
compiles, starts up a QEMU-based BMC, and runs a test suite against
Gerrit-based changes.

The Gerrit code review process invokes a Jenkins-based [continuous
integration](https://github.com/openbmc/openbmc/wiki/Continuous-Integration)
server which validates the quality of C++ and Python code (formatting
rules, etc.)  via `clang` and `pycodestyle` and reports its finding
back to the code review process.

The OpenBMC team has an active community including downstream
development teams that use OpenBMC and submit work items and fixes
back into the OpenBMC project, including security items.

Not all OpenBMC configurations are tested.


## Release

The OpenBMC project is planning regular releases to begin in 2019 and
continuing twice per year following the [Yocto project release
schedule](https://wiki.yoctoproject.org/wiki/Planning).

The [OpenBMC release planning security checklist](obmc-release-checklist.md)
has work items that should be performed each release.
