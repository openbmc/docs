____

# Integrate Security-Enhanced Linux into OpenBMC

Author: Ruud Haring   -- Discord ID: RuudHaring#2662

Primary assignee: Yutaka Sugawara -- Discord ID: YutakaSugawara#4715

Other contributors: Russell Wilson -- Discord ID:

Created: April 21, 2022
Updated: April 26, 2022

## Problem Description
Motivation: OpenBMC currently has several security exposures. For example:
 1. In OpenBMC, every application runs as root.  A compromised application
     can do anything: install malicious userIDs (with root privileges), affect
     critical files for other applications, affect journald and systemd, ...
 2. Every application can send/receive messages for any DBus service.

[Security-Enhanced Linux](https://en.wikipedia.org/wiki/Security-Enhanced_Linux) (SELinux) is a [Linux kernel security module](https://en.wikipedia.org/wiki/Linux_Security_Modules) (LSM)
that can control which activities a system will allow for each user, process
and daemon, with very precise specifications. This limits the potential harm
from a process that becomes compromised.

We propose that integrating SELinux will bring value to OpenBMC.
It will enhance OpenBMC's security posture, addressing end-user security and
compliance requirements.

## Background and References
History and attributes of SELinux are well-described in
[this Wikipedia article](https://en.wikipedia.org/wiki/Security-Enhanced_Linux).
Many other references, YouTube videos of conference presentations etc. can be
found on the internet.

SELinux is available as a meta-selinux layer in Yocto
[https://git.yoctoproject.org/meta-selinux](https://git.yoctoproject.org/meta-selinux).

Manojkiran Eda discussed SELinux use cases specific to OpenBMC in [April 2020](https://lists.ozlabs.org/pipermail/openbmc/2020-April/021477.html),
with further discussion in the [May 2020](https://lists.ozlabs.org/pipermail/openbmc/2020-May/) mailing list archives.

## Requirements
Having analyzed Manojkiran's use cases for risk (probability * impact) we are
particularly interested in locking down permissions for Linux processes that
provide network services, and in securing systemd and Dbus permissions.

The scope of this work is:
 - Build and boot OpenBMC with SELinux, based on the existing
   meta-selinux layer in the Yocto project. First requirement is that all
   OpenBMC CI/CD tests should work unchanged when SELinux mode=disabled.
 - When switching to SELinux mode=permissive, requirement is that all OpenBMC
   CI/CD tests should continue to work unchanged. But, given a reference policy,
   denial events should be logged.
 - Extract SELinux logs for analysis off-line (enabling policy refinement).
 - Formulate "malicious" testcases that should lead to denial log events when
   SELinux mode=permissive.

## Proposed Design
We plan to offer SELinux as an "opt-in" feature to the OpenBMC build process,
with minimal changes to recipes/config files:
initially just adding meta-selinux and specifying a reference policy.

In the final design, additional changes in config files may be required,
possibly in multiple places -- this is not known at this point.
Documentation will be provided on how to modify config files to
enable SELinux, for a selected set of machine configurations.

The goal is to develop SELinux to the point that it works in "permissive" mode,
with a template policy that enables bmcweb, IPMI,  and SSH activities
in typical OpenBMC use scenarios.

End-users will probably need to fine-tune the policy for their specific use
cases, before choosing to switch to "enforcing" mode.

## Alternatives Considered
SELinux and [AppArmor](https://en.wikipedia.org/wiki/AppArmor) are both independent [Linux security module](https://en.wikipedia.org/wiki/Linux_Security_Modules) (LSM)
implementations. Implementation  of AppArmor in OpenBMC was tried in 2021
by Ratan Gupta <ratankgupta31@gmail.com>.  Ratan [reported that they were not
(yet) successful](https://lists.ozlabs.org/pipermail/openbmc/2022-April/030167.html). The effort is currently blocked by an issue that is apparently
non-trivial and which is still pending with the AppArmor developers.
From the various discussions on the web (see also the [SELinux Wikipedia article](https://en.wikipedia.org/wiki/Security-Enhanced_Linux)),
AppArmor offers a less capable solution based on path names, whereas
SELinux is a more general type-enforcement system that can be applied to a
larger set of objects (inodes, ports, ...).

As an opt-in feature, SELinux is not mandatory or exclusive. If someone else
prefers AppArmor, it can be developed and added in separately.

A prior [design](https://gerrit.openbmc-project.xyz/c/openbmc/docs/+/49100/1/designs/deprivileging.md)
to make daemons run as non-root users (with reduced privileges) and to
introduce ACLs for D-Bus communication was [started but not merged](https://gerrit.openbmc-project.xyz/c/openbmc/openbmc/+/42748 ).
Since D-Bus has built-in support for SELinux, we intend to use the SELinux
approach to secure D-Bus.

## Impacts
 - API impact? None
 - Security impact?  Greatly enhanced :-)
 - Documentation impact? Pointers to standard SELinux documentation.
   Policy documentation is especially of importance when governance of policies
   is transferred to an end-user.
 - Performance impact? Reportedly minimal
 - Developer impact?  Getting it to work, defining policies.
   Newly deployed OpenBMC applications will need corresponding SELinux policies.
   This should be caught in regression testing.
 - Upgradability impact?  SELinux is considered quite stable and updates
   should keep pace with Yocto.
 - Footprint impact?  1st attempt (Manojkiran Eda, 2020) took about 20 MB.
   However, it comes with log analysis tools.  These should be stripped out to be
   used off-line. Final footprint impact should be less.

## Organizational
Does this project require a new repository?  No

## Testing
SELinux introduces [a new set of commands](https://fedoraproject.org/wiki/SELinux/Commands)
like  `sestatus`, `setenforce`, `getenforce`, ...
and a new option (-Z) integrated with commands such as `ls`, `ps`, `id`.
These will all be tested.

In testcases for OpenBMC applications, we will navigate between the SELinux
operation modes:
- In disabled mode, OpenBMC should behave as before.
- In permissive mode, OpenBMC should behave as before, but produce logging
  events.  Testcases can be written to check the log for expected/unexpected
  denials.
- While enforcing mode is an end-user choice, we can use it in test cases to
  check for known allowals and denials.

All new and modified tests will be uploaded to openbmc-test-automation.
