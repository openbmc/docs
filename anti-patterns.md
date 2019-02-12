# OpenBMC Anti-patterns

From [Wikipedia](https://en.wikipedia.org/wiki/Anti-pattern):


"An anti-pattern is a common response to a recurring problem that is usually
ineffective and risks being highly counterproductive."


The developers of OpenBMC do not get 100% of decisions right 100% of the time.
That, combined with the fact that software development is often an exercise in
copying and pasting, results in mistakes happening over and over again.


This page aims to document some of the anti-patterns that exist in OpenBMC to
ease the job of those reviewing code.  If an anti-pattern is spotted, rather
that repeating the same explanations over and over, a link to this document can
be provided.


<!-- begin copy/paste on next line -->

## Anti-pattern template [one line description]

### Identification
(1 paragraph) Describe how to spot the anti-pattern.

### Description
(1 paragraph) Describe the negative effects of the anti-pattern.

### Background
(1 paragraph) Describe why the anti-pattern exists.  If you don't know, try
running git blame and look at who wrote the code originally, and ask them on the
mailing list or in IRC what their original intent was, so it can be documented
here (and you may possibly discover it isn't as much of an anti-pattern as you
thought).  If you are unable to determine why the anti-pattern exists, put:
"Unknown" here.

### Resolution
(1 paragraph) Describe the preferred way to solve the problem solved by the
anti-pattern and the positive effects of solving it in the manner described.

<!-- end copy/paste on previous line -->

## Placement of applications in /sbin or /usr/sbin

### Identification
OpenBMC applications that are installed in /usr/sbin.  $sbindir in bitbake
metadata, makefiles or configure scripts.

### Description
Installing OpenBMC applications in /usr/sbin violates the [princple of least
astonishment](https://en.wikipedia.org/wiki/Principle_of_least_astonishment) and
more importantly violates the
[FHS](https://en.wikipedia.org/wiki/Filesystem_Hierarchy_Standard).

### Background
The sbin anti-pattern exists because the FHS was misinterpreted at an early point
in OpenBMC project history, and has proliferated ever since.

From the hier(7) man page:

```
/usr/bin This is the primary directory for executable programs.  Most programs
executed by normal users which are not needed for booting or for repairing the
system and which are not installed locally should be placed in this directory.

/usr/sbin This directory contains program binaries for system administration
which are not essential for the boot process, for mounting /usr, or for system
repair.
```
The FHS description for /usr/sbin refers to "system administration" but the
de-facto interpretation of the system being administered refers to the OS
installation and not a system in the OpenBMC sense of managed system.  As such
OpenBMC applications should be installed in /usr/bin.

### Resolution
Install OpenBMC applications in /usr/bin/.
