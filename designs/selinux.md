____

# Integrate Security-Enhanced Linux into OpenBMC

Author: Ruud Haring   -- Discord ID: RuudHaring#2662

Primary assignee: Yutaka Sugawara -- Discord ID: YutakaSugawara#4715

Other contributors: Russell Wilson -- Discord ID:

Created: April 21, 2022

## Problem Description
Motivation: OpenBMC currently has several security exposures. For example:
 1. In OpenBMC, every application runs as root.  A compromised application
     can do anything: install malicious userIDs (with root privileges), 
     affect critical files for other applications, affect journald and systemd,
     ...
 2. Every application can send/receive messages for any DBus service.

[Security-Enhanced Linux](https://en.wikipedia.org/wiki/Security-Enhanced_Linux) (SELinux) is a [Linux kernel security module](https://en.wikipedia.org/wiki/Linux_Security_Modules) (LSM)
that provides a mechanism for supporting access control security policies,
including mandatory access controls (MAC).  
SELinux can control which activities a system will allow for each user, 
process, and daemon, with very precise specifications. This limits the 
potential harm from a process that becomes compromised.

We propose that integrating SELinux will bring value to OpenBMC in terms of
enhancing its security posture, addressing customer security and compliance
requirements. As SELinux is “well-known” to management and security-conscious
customers, it is relatively easy to get stakeholders informed and on-board  
-- as opposed to any alternative or “home brew” solution.

## Background and References
History and attributes of SELinux are well-described in 
[this Wikipedia article](https://en.wikipedia.org/wiki/Security-Enhanced_Linux).
Many other references, YouTube videos of conference presentations etc. can be
found on the internet. SELinux is available as a meta-selinux layer in Yocto
[https://git.yoctoproject.org/meta-selinux](https://git.yoctoproject.org/meta-selinux).

Manojkiran Eda discussed a set of SELinux use cases specific to OpenBMC in
the [April 2020](https://lists.ozlabs.org/pipermail/openbmc/2020-April/) and [May 2020](https://lists.ozlabs.org/pipermail/openbmc/2020-May/) mailing list archives.

The current plan was discussed in the [OpenBMC Security Working Group](https://github.com/openbmc/openbmc/wiki/Security-working-group) 
-- see [04/13/2021 minutes](https://docs.google.com/document/d/1b7x9BaxsfcukQDqbvZsU2ehMq4xoJRQvLxxsDUWmAOI/edit)

## Requirements
Having analyzed Manojkiran's use cases for risk (probability * impact) we are
particularly interested in locking down permissions for  network processes
and in securing systemd and Dbus permissions.

The scope of this work is: 
 - Build and boot OpenBMC with SELinux, based on the existing 
 meta-selinux layer in the Yocto project.  Requirement is that all usual 
 OpenBMC CI/CD tests should work unchanged when SELinux mode=disabled.
 
 - Work on SELinux mode=permissive. Requirement is that all usual
 OpenBMC CI/CD tests should work unchanged when SELinux mode=permissive.
 Get SELinux logging  to work in such a way that its log can be extracted and
 analyzed off-line. 
 
- Use the log analysis capability to iteratively get a set
 of policies in place appropriate to general OpenBMC use.  
 Specifically, formulate polices that enable all the usual OpenBMC CI/CD tests 
 (and thus OpenBMC applications) when SELinux mode=enforcing.

- Formulate "malicious" testcases that should lead to denial log events when
SELinux mode=permissive and to actual denials when SELinux mode=enforcing.

 - In a later phase, we aim to fine-tune the policies to the client's
 environment. 

## Proposed Design
The plan is to offer SELinux as an "opt-in" feature to the OpenBMC build
process.  We will design recipes/config files so that SELinux can be "opt-in"
with minimal changes: initially adding meta-selinux and specifying a refpolicy.
In the final design, additional changes in config files may be required, 
possibly in multiple places -- this is not known at this point. 
Documentation will be provided on how to modify config files to
enable SELinux, for a selected set of machine configurations.

The hardest part of SELinux is to get the correct set of permissions in place.
However, for OpenBMC, an embedded system running a finite set of
processes, this task should be manageable. SELinux offers a mechanism to
iteratively fine-tune its permissions by running it in "permissive mode".
In this mode, it will log all its denials, but won't enforce them. SELinux offers
a tool, [audit2allow](https://linux.die.net/man/1/audit2allow), to analyze the 
logs and (judiciously) convert denials into "allow rules".
Prerequisite is, of course,  to make sure that the logging works
adequately for this purpose.  

This methodology will be used to fine-tune the permissions in a 
pre-deployment phase (subject to all regular OpenBMC testing) 
and in an early real-world deployment phase.

We will upstream a set of basic policies to enable bmcweb, IPMI, and SSH
activities in typical OpenBMC use scenarios, to act as a template policy. 
We recognize that different use cases will have different policy requirements,
and we are open to discuss policy management. 

Only once enough confidence is obtained that false positives will be
manageable, will SELinux be switched to "enforcing mode".

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
to make daemons run as non-root users (reduced privileges) and to introduce
ACLs for D-Bus communication was [started but not merged](https://gerrit.openbmc-project.xyz/c/openbmc/openbmc/+/42748 ).  
Since D-Bus has built-in support for SELinux, we intend to use the SELinux 
approach to secure D-Bus.


## Impacts
 - API impact? None
 - Security impact?  Greatly enhanced :-)
 - Documentation impact? Pointers to standard SELinux documentation.
 Policy documentation is especially of importance when governance of policies 
 is transferred to a client.
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
- In enforcing mode, we can test for known allowals and denials.

I.e. as the SELinux policies mature, we will probably move testcases for the 
targeted applications from disabled to permissive to enforcing mode.

All new and modified tests will be uploaded to openbmc-test-automation.

