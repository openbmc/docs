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

## Explicit AC_MSG_ERROR on PKG_CHECK_MODULES failure

### Identification
```
PKG_CHECK_MODULES(
    [PHOSPHOR_LOGGING],
    [phosphor-logging],
    [],
    [AC_MSG_ERROR([Could not find phosphor-logging...openbmc/phosphor-logging package required])])
```

### Description

The PKG_CHECK_MODULES macro provides the ability to specify an "if found" and
"if not found" behavior.  By default, the "if not found" behavior will list the
package not found.  In many cases, this is sufficient to a developer to know
what package is missing.  In most cases, it's another OpenBMC package.

If the library sought's name isn't related to the package providing it, then
the failure message should be set to something more useful to the developer.

### Resolution

Use the default macro behavior when it is clear that the missing package is
another OpenBMC package.

```
PKG_CHECK_MODULES([PHOSPHOR_LOGGING], [phosphor-logging])
```

## Explicit listing of shared library packages in RDEPENDS in bitbake metadata

### Identification
```
RDEPENDS_${PN} = "libsystemd"
```

### Description
Out of the box bitbake examines built applications, automatically adds runtime
dependencies and thus ensures any library packages dependencies are
automatically added to images, sdks, etc.  There is no need to list them
explicitly in a recipe.

Dependencies change over time, and listing them explicitly is likely prone to
errors - the net effect being unnecessary shared library packages being
installed into images.

Consult
https://www.yoctoproject.org/docs/latest/mega-manual/mega-manual.html#var-RDEPENDS
for information on when to use explicit runtime dependencies.

### Background
The initial bitbake metadata author for OpenBMC was not aware that bitbake
added these dependencies automatically.  Initial bitbake metadata therefore
listed shared library dependencies explicitly, and was subsequently copy pasted.

### Resolution
Do not list shared library packages in RDEPENDS.  This eliminates the
possibility of installing unnecessary shared library packages due to
unmaintained library dependency lists in bitbake metadata.

## Use of /usr/bin/env in systemd service files

### Identification
In systemd unit files:
```
[Service]

ExecStart=/usr/bin/env some-application
```

### Description
Outside of OpenBMC, most applications that provide systemd unit files don't
launch applications in this way.  So if nothing else, this just looks strange
and violates the [princple of least
astonishment](https://en.wikipedia.org/wiki/Principle_of_least_astonishment).

### Background
This anti-pattern exists because a requirement exists to enable live patching of
applications on read-only filesystems.  Launching applications in this way was
part of the implementation that satisfied the live patch requirement.  For
example:

```
/usr/bin/phosphor-hwmon
```

on a read-only filesystem becomes:

```
/usr/local/bin/phosphor-hwmon`
```

on a writeable /usr/local filesystem.

### Resolution
The /usr/bin/env method only enables live patching of applications.  A method
that supports live patching of any file in the read-only filesystem has emerged.
Assuming a writeable filesystem exists _somewhere_ on the bmc, something like:

```
mkdir -p /var/persist/usr
mkdir -p /var/persist/work/usr
mount -t overlay -o lowerdir=/usr,upperdir=/var/persist/usr,workdir=/var/persist/work/usr overlay /usr
```
can enable live system patching without any additional requirements on how
applications are launched from systemd service files.  This is the preferred
method for enabling live system patching as it allows OpenBMC developers to
write systemd service files in the same way as most other projects.

To undo existing instances of this anti-pattern remove /usr/bin/env from systemd
service files and replace with the fully qualified path to the application being
launched.  For example, given an application in /usr/bin:

```
sed -i s,/usr/bin/env ,/usr/bin/, foo.service
```
