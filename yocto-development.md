# Yocto in OpenBMC #

The Yocto Project is an open source collaboration project that provides
templates, tools and methods to help you create custom Linux-based systems for
embedded products regardless of the hardware architecture

OpenBMC uses the Yocto tools to manage configuration and creation of BMC
images.

## Developing with Yocto ##

There are two main use-cases for Yocto in OpenBMC:

1. Building from master or existing tags
2. Developing changes for submission to master

The first is the easy case, and largely involves picking the system
configuration to build before invoking `bitbake`. Examples for
[Palmetto](cheatsheet.md#building-for-palmetto) and
[Barreleye](cheatsheet.md#building-for-barreleye) are in the
[cheatsheet](cheatsheet.md).

The second case can be helped with Yocto's `devtool`. After running
`.  openbmc-env`, a tool called `devtool` will be in your path, and can be
applied in several ways.

If you have an existing source tree you'd like to integrate, running
`devtool modify -n ${PACKAGE} ${SRCTREE}` first creates a new Yocto layer in
your build directory where devtool stores recipe modifications. It then
constructs a `.bbappend` for the the package recipe and uses the
`externalsource` class to replace the download, fetch, and patch steps with
no-ops. The result is that when you build the package, it will use the local
source directory as is. Keep in mind that the package recipe may not perform a
clean and depending on what you are doing, you may need to run `${PACKAGE}`
build system's clean command in `${SRCTREE}` to clear any built objects. Also
if you change the source, you may need to run
`bitbake -c cleansstate ${PACKAGE}` to clear BitBake's caches.

Alternatively if you don't already have a local source tree but would still
like to modify the package, invoking `devtool modify ${PACKAGE}` will handle
the fetch, unpack and patch phases for you and drop a source tree into your
default workspace location.

When you are all done, run `devtool reset ${PACKAGE}` to remove the `.bbappend`
from the devtool Yocto layer.

Further information on [devtool][0] can be found in the [Yocto Mega Manual][1].

[0]: (http://www.yoctoproject.org/docs/2.1/mega-manual/mega-manual.html#devtool-use-devtool-modify-to-enable-work-on-code-associated-with-an-existing-recipe) "devtool"
[1]: (http://www.yoctoproject.org/docs/2.1/mega-manual/mega-manual.html) "Yocto Mega Manual"
