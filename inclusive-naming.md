# Inclusive Naming

## Background
OpenBMC relies on the Linux community to provide guidelines on inclusive
naming.  In addition to below, the most recent Linux kernel guidelines can be
found here:

https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/process/coding-style.rst

For symbol names and documentation, avoid introducing new usage of
'master / slave' (or 'slave' independent of 'master') and 'blacklist /
whitelist'.

Recommended replacements for 'master / slave' are:
    '{primary,main} / {secondary,replica,subordinate}'
    '{initiator,requester} / {target,responder}'
    '{controller,host} / {device,worker,proxy}'
    'leader / follower'
    'director / performer'

Recommended replacements for 'blacklist/whitelist' are:
    'denylist / allowlist'
    'blocklist / passlist'

Exceptions for introducing new usage is to maintain a userspace ABI/API,
or when updating code for an existing (as of 2020) hardware or protocol
specification that mandates those terms. For new specifications
translate specification usage of the terminology to the kernel coding
standard where possible.
