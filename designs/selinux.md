____

# Integrate Security-Enhanced Linux into OpenBMC

Author: Ruud Haring <RuudHaring#2662>

Other contributors: Yutaka Sugawara <YutakaSugawara#4715>
                    Russell Wilson

Created: April 21, 2022
Updated: April 26, 2022; July 23, 2022

## Problem Description
Motivation: OpenBMC currently has a security exposure.
Given that the service daemons runs as root, they have more
authority than they need. If a daemon is compromised, the attacker can
exploit this: install malicious userIDs (with root privileges), affect
critical files for other applications, affect journald and systemd, ...
Although an obvious solution is to run these services as non-root users,
using SELinux will provide even finer-grained access control, in a
systematic manner, to limit or mitigate any attacks.

[Security-Enhanced Linux]
(https://en.wikipedia.org/wiki/Security-Enhanced_Linux) (SELinux) is a
[Linux kernel security module]
(https://en.wikipedia.org/wiki/Linux_Security_Modules) (LSM) that can
limit the authority of users, processes, and daemons, and block any
unintended/unnecessary actions (e.g. bmcweb accessing critical files
under /etc). In this way, when a process is compromised, the resulting
impact can be eliminated or minimized. In other words, SELinux will add
an additional security barrier on top of existing security and
authentication mechanisms.

This will add value to OpenBMC for security-sensitive users (financial
institutions, government users), and help meet
compliance requirements.

From a technical perspective, if OpenBMC is compromised,
it will allow an attacker to conduct low-level attacks to the
host CPU as well (e.g. alter UEFI firmware FLASH via BMC, alter BIOS
configuration, access host physical memory via the PCIe link). The
impacts of these attacks are serious, and are difficult to protect by
host hypervisor or operating system. In this respect, OpenBMC needs to
be protected with the same priority as the host operating system. Given
the reality that many CVE issues are reported in various service daemons
every year, adding one more security barrier (SELinux) would be a
natural choice for use cases where uncontrolled damage by a compromised
process is prohibitive.

## Background and References
History and attributes of SELinux are well-described in
[this Wikipedia article](https://en.wikipedia.org/wiki/Security-Enhanced_Linux).
Many other references, YouTube videos of conference presentations etc. can be
found on the internet.

SELinux is available as a meta-selinux layer in Yocto
[https://git.yoctoproject.org/meta-selinux](https://git.yoctoproject.org/meta-selinux).

Manojkiran Eda discussed SELinux use cases specific to OpenBMC in
[April 2020](https://lists.ozlabs.org/pipermail/openbmc/2020-April/021477.html),
with further discussion in the
[May 2020](https://lists.ozlabs.org/pipermail/openbmc/2020-May/)
mailing list archives.

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
with minimal changes to recipes/config files.
The goal is to develop SELinux to the point that it works in "permissive" mode,
with a template policy that enables bmcweb, IPMI, and SSH activities
in typical OpenBMC use scenarios.

As a prerequisite, an end user needs to obtain meta-selinux from the yocto
project git repo, and place it under their local openbmc/ top level
directory:

>     cd openbmc/
>     git clone git://git.yoctoproject.org/meta-selinux

Next, we propose to add a new layer 'meta-phosphor-selinux' to
correctly enable meta-selinux for OpenBMC. Users who
want to enable SELinux can add this to their bblayer.conf.samples.
This new layer contains bbappend files to apply the following changes:
* recipes-core/coreutils/coreutils: Include chcon in base_bindir_progs
  for SELinux
* recipes-core/packagegroup/packagegroup-base: Add RDEPENDS for
  packagegroup-selinux-minimal
* recipes-extended/pam/libpam : Apply a patch to
  pam_selinux.c to fix compilation errors.
* recipes-security : Apply changes to meta-selinux, to make it work for
  OpenBMC. For example, existing meta-selinux recipes assume commands
  exist in /usr/bin. This needs to be changed to /bin for OpenBMC.

In addition, end users need to apply changes on their target-specific
recipes. We will provide instructions and example recipes for the
evb-ast2600 target.
The following is a brief summary of the proposed example
evb-ast2600 recipes that will reside under
meta-evb/meta-evb-aspeed/meta-evb-ast2600-selinux/ :
* conf/bblayers.conf.sample: Need to add ##OEROOT##/meta-selinux
  and ##OEROOT##/meta-phosphor-selinux
* conf/local.conf.sample: Add "acl xattr pam selinux" to
  DISTRO_FEATURES, ensure to set systemd as the RUNTIME_init_manager, add
  "packagegroup-core-selinux" to IMAGE_INSTALL
* conf/machine/evb-ast2600-selinux.conf : The main machine config file,
  specify SELinux policy (ours is refpolicy-minimum)
* recipes-kernel/linux/linux-aspeed : Set of bbappend and patch files to
  modify kernel parameters and FLASH layout to enable SELinux for AST2600
  kernel build

As the kernel parameters and FLASH layout are highly dependent on the
target machine, these should be placed in target-specific recipe
directories, rather than in central places. End-user's recipes
can determine whether SELinux is enabled or not, by checking whether
DISTRO_FEATURE contains "selinux".

End-users will probably need to fine-tune the policy for their specific use
cases, before choosing to switch to "enforcing" mode.

This design focuses on enabling SELinux in permissive mode. Development
and management of policies should be covered in another design.
Our current outlook is that policies can be maintained as follows:
* meta-selinux contains the baseline policies that work in generic poky
* meta-phosphor-selinux will contain OpenBMC-specific generic policies,
that will cover typical OpenBMC systems. Specifically, these policies
cover common OpenBMC services such as bmcweb, dbus, ipmi, and sshd.
* Any target-dependent policies (i.e. valid only for a particular build
target or non-generic hardware devices) should be stored in the target
recipe folder. For example, policies that are specific to the evb-ast2600
target should be placed under meta-evb/meta-evb-aspeed/meta-evb-ast2600-selinux.

The meta-selinux layer provides baseline recipes to populate the basic
policies. The meta-phosphor-selinux or a target-specfic layer can add
or change policy files by using .bbappend files.

## Alternatives Considered
SELinux and [AppArmor](https://en.wikipedia.org/wiki/AppArmor) are both independent
[Linux security module](https://en.wikipedia.org/wiki/Linux_Security_Modules) (LSM)
implementations. Implementation of AppArmor in OpenBMC was tried in 2021
by Ratan Gupta <ratankgupta31@gmail.com>.
[Reference to the e-mail thread](https://lists.ozlabs.org/pipermail/openbmc/2022-April/030167.html).

From the various discussions on the web (see also the
[SELinux Wikipedia article](https://en.wikipedia.org/wiki/Security-Enhanced_Linux)),
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
Note that the SELinux type enforcement system is separate from, and
orthogonal to, the DBus ACLs. The goal of the present design is to
enable SELinux in permissive mode, so that we can figure out the
policies to (selectively) allow all required DBus transactions.

## Impacts
- API impact? None

- Security impact? Greatly enhanced :-)

- Documentation impact?
  Standard SELinux documentation should cover most documentation needs.
  For example:
  - Books:
    - "SELinux - NSA's Open Source Security Enhanced Linux" by Bill McCarty
    - "SELinux by Example: Using Security Enhanced Linux" by F Mayer,
       K MacMillan, D Caplan; online: https://flylib.com/books/en/2.803.1/
  - Online guides:
    - https://www.linuxtopia.org/online_books/getting_started_with_SELinux/index.html
    - https://www.linuxtopia.org/online_books/writing_SELinux_policy_guide/index.html

  If additional OpenBMC-specific documentation is required, we will provide.

- Performance impact? Reportedly minimal.

- Developer impact?
  - The meta-selinux layer should be stable, and invokes the baseline
    SELinux policies that work in generic poky.
    Once SELinux is enabled in OpenBMC (in permissive mode) it is our
    next task to formulate the OpenBMC-specific generic SELinux policies
    in the meta-phosphor-selinux layer, and
    ensure that these policies will allow all normal OpenBMC operations.
    (SSHD, IPMI, bmcweb, and dbus). This is testable by running the
    openbmc-test-automation suite in SELinux permissive mode and checking
    the resulting logs.
  - Each end-user can define further policy rules in their
    target-specific meta directory. We will maintain an example in
    meta-evb/meta-evb-aspeed/meta-evb-ast2600-selinux/
  - A bitbake process will compile these policies into a combined
    policy.conf file.
  - New OpenBMC applications will need to be developed in SELinux
    permissive mode. If additional policy rules are required, the
    developer would see SELinux denial messages in the system log,
    which, via the SELinux "audit2allow" process can be (judiciously)
    converted into new policy rules.
    audit2allow and other log analysis tools are available on any
    SELinux-enabled Linux system, e.g any system which responds to 'sestatus'.
    Our team would be glad to help formulate such new policy rules,
    to ensure that the new or updated application will also work in
    SELinux-enabled OpenBMC instances.
  - SELinux supports changing the mode on-demand (e.g. permissive vs.
    enforcing) using setenforce commands or /etc/sysconfig/selinux. For
    example, an administrative user can change from enforcing to permissive
    mode to temporarily enable debug tools. Also, SELinux can be turned off
    by rebooting the kernel with the selinux=0 option.

- Upgradability impact? SELinux is considered quite stable and updates
  should keep pace with Yocto.

- Footprint impact? In our current build for evb-ast2600 the mtd image
  fits in a 64MB SPI FLASH, without using eMMC, but not in 32MB. The
  actual used space is 42MB plus extra RWfs space consumed during
  runtime.
  - Without SELinux, the image would fit in 32MB SPI FLASH, with
    actual used space of 22MB plus runtime RWfs space consumption.
    Thus, SELinux will require 20MB of extra space with the current design.
  - For now we would propose to document a 64MB FLASH size requirement.
    In continued development, we will explore size reduction optimizations.
    - For example, our busybox build for evb-ast2600 is successful,
      but we have not yet switched over from coreutils, and thus
      we have not removed coreutils yet.
    - Also, the default SELinux configuration builds log analysis tools
      into the BMC image. However, the anticipated use case is to offload
      the logs and analyze them on another system, so SELinux will be
      configured to not include the log analysis tools.

- Logging capacity:
  - When SELinux is enabled in permissive mode with missing rules,
    policy denial messages will be reported in the system log
    (once per specific event). This may introduce concerns on the RWfs
    capacity, if the log size is not capped.
  - When the log size is capped, a concern is that these denial messages
    may wipe out existing important log messages. A possible option is
    to use rsyslog where possible, to store these
    messages in an external server with plenty of storage space.

## Organizational
Does this project require a new repository? No

## Testing
SELinux introduces
[a new set of commands](https://fedoraproject.org/wiki/SELinux/Commands)
like `sestatus`, `setenforce`, `getenforce`, ...
and a new option (-Z) integrated with commands such as `ls`, `ps`, `id`.
These will all be tested.

In testcases for OpenBMC applications, we will navigate between the SELinux
operation modes:
- In disabled mode, OpenBMC should behave as before. The
  openbmc-test-automation suite should run unchanged.
- In permissive mode, OpenBMC should behave as before, but produce logging
  events. Testcases can be written to check the log for expected/unexpected
  denials.
- While enforcing mode is an end-user choice, we can use it in test cases to
  check for known allowals and denials.
- "Bad path" testing:
  Note that SELinux will deny an operation by default, unless a policy rule
  specifically allows it.
  SELinux is supposed to be a 2nd line of defense: with a complete policy set,
  all normal OpenBMC operations are allowed, and only compromised/misbehaving
  processes should run into denials.
  Thus, bad path testing is akin to error injection.

  Two bad path categories:
  - False denials: an operation is denied that should have been allowed.
    Most likely points to a bug in policy (e.g. missing/mis-formed rule).
  - False allowal: an operation is allowed that should have been denied.
    These can be due to over-broad allow-rules (broader than intended
    by the end-user) -- we cannot generically test for end-user intent.
    Specific end-user testcases can be formulated, however,
    such as intentionally omitting an allowal rule,
    or intentionally testing denied operations which OpenBMC allows,
    but the end-user's SELinux rules do not
    -- e.g. denying certain in-band IPMI operations.

    False allowals can also be due to a bug in the SELinux machinery itself.
    Then the space is huge, but out-of-scope for the current goal:
    merely enabling SELinux and (next) getting the generic policies straight.

All new and modified tests will be uploaded to openbmc-test-automation.
