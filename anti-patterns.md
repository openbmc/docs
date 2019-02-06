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

## Explict listing of shared library packages in RDEPENDS in bitbake metadata

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

### Background
The initial bitbake metadata author for OpenBMC was not aware that bitbake
added these dependencies automatically.  Initial bitbake metadata therefore
listed shared library dependencies explictly, and was subsequently copy pasted.

### Resolution
Do not list shared library packages in RDEPENDS.  This eliminates the
possibility of installing unnecessary shared library packages due to
unmaintained library dependency lists in bitbake metadata.
