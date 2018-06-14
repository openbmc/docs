# OpenBMC development team security practices

This document identifies practices and procedures used by the OpenBMC
development team to help ensure only reliable, reviewed, tested code
goes into the OpenBMC project and prevent security vulnerabilities
from being introduced during OpenBMC development.


## Open source contributions

OpenBMC is open source and changes can be submitted by anyone.  There
are no security requirements.  The OpenBMC repositories, however, are
under source code control, and can only be updated by the maintainers.

The OpenBMC project's [contributing
guidelines](https://github.com/openbmc/docs/blob/master/contributing.md)
page explains the guidelines for coding, testing, and reviewing changes
to merge into the project.


## Maintainers

The project maintainers hold the keys to the OpenBMC repositories and
the external tools like Git, Gerrit, and Jenkins resources.  They are
responsible for maintaining the quality and integrity of the project
and therefore its security.

Maintainers are identified by the project home page and a
`MAINTAINERS` file in each OpenBMC repository.

Maintainers are expected to have secure development environments.


## Secure development environments

A secure development environment is broadly defined as one where only
the intended devlopers have access to resources including hardware,
network, storage, command-line and web interfaces.  The idea is to
protect the confidentiality and integrity of the environment.

Secure environments are the basis for securely performing tasks such
as code reviews, merging code into the master repositories, building
and testing code.


## Code review process

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

A Jenkins server builds and tests each commit and patch level and
reports its finding back to the Gerrit review process.  See the test
section for details.

The list of reviewers is initially assigned by the developer who
pushed the change to Gerrit, usually the committer.  Each OpenBMC
repository has a `MAINTAINERS` file which lists required reviewers who
are subject matter experts.  Those reviewers add additional reviews as
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
allows discussions of the proposed change.  TO DO: Check this: If the
patch is accepted, one of the maintainers pushes it into the OpenBMC
repository.


## Development team education

The development team consists of anyone who submits material to the
project (whether that material is accepted or not).  There are no
education requirements.  The OpenBMC team maintains contributing
guidelines which gives best practices including input checking, source
code formatting, etc.

The code review process is educational, as experts from each area are
invited to join and comment.

The maintainers generally also have formal education in software or
firmware engineering and keep up with security practices.

There is specific education for [Yocto security](https://wiki.yoctoproject.org/wiki/Security).


## OpenBMC repository updates

The OpenBMC repositories are configured to allow accept changes (in
the form of git commits) only from a small list of maintainers.

The maintainers accept only high-quality, reviewed, and approved
commits from the Gerrit server.  They also accept approved patches
from the OpenBMC mailing list.

The Gerrit commits are merged into the OpenBMC repositories using
git's fast-forward merge strategy which avoids merge conflicts.  Any
commits with merge conflicts must be reworked (`git rebase`) before
they are merged.

Gerrit commit messages can include a "resolves" line which identifies
an OpenBMC GitHub issue.  When used, this mechanism closes the
corresponding GitHub issue when the Gerrit commit is merged.


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
to provide their own security.  The OpenBMC developers who have
administration access to these sites protect that access and rely on
the tools to enforce their security requirements.

OpenBMC relies on the security of the open source gcc-based toolchain,
including a large set of utility programs.  This is also known as the
Software Development Kit (SDK).  These projects are retrieved as
securely as any other open source project


## Work items

Work items are recorded in https://github.com/openbmc/openbmc/issues.
These generally include defects, features, and wishes.  Anybody can
read, create, and comments on issue.  The OpenBMC maintainers can
assign owners and close issues.  TO DO: check this: Do other repos use
issues?

Project management uses work items to track features and defects to
help ensure OpenBMC code is the way it was intended.

The gerrit merge process also closes issues when it merges a commit
that contains a "resolves" directive.


## Downstream Development

The OpenBMC team produces BMC source code.  Apart from a basic test
suite, it does not build images, provision, or maintain any BMC.

OpenBMC team members, however, do coordinate with teams that consume
OpenBMC code and perform these activities.  OpenBMC provides guidance
for such teams in the [Using OpenBMC in higher security projects](to
do: reference) guide.


## Test

OpenBMC functional test cases are part of each repository.  In
addition, a Jenkins server at https://openpower.xyz runs a continuous
integration test which compiles, starts up a QEMU-based BMC, and
runs a test suite against Gerrit-based changes.

The Gerrit code review process invokes a Jenkins testing process which
validates the quality of C++ and Python code (formatting rules, etc.)
via `clang` and `pycodestyle` and reports its finding back to the code
review process.

A simple test would be building a firmware image from the example
OpenBMC configuration and loading it onto a simulated BMC device such
as QEMU.

The Jenkins build and test server has a maintainer who controls its
configuration and ensures its security.

The OpenBMC team has an active community including downstream
development teams that use OpenBMC and submit fixes back into the
OpenBMC project and its upstream projects (such as the Linux kernel).

Selected OpenBMC configurations are tested.


## Release

The OpenBMC project is planning regular releases to begin November
2018, and continuing twice per year following the [Yocto project
release schedule](https://wiki.yoctoproject.org/wiki/Planning).

TO DO: Items for consideration include:
 - How are OpenBMC releases identifies?  Web page?
 - Will upstream projects and release levels be identified?  (Yocto
   helps provide that list.)
 - Will the security flaws fixed in each release be identified?

See the repair topic (in the "using OpenBMC" guide) for details about
compatability between releases.
