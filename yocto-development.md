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
[Barreleye](#building-for-barreleye) are in the [cheatsheet](cheatsheet.md).

The second case can be helped with Yocto's `devtool`. After running `.
openbmc-env`, a tool called `devtool` will be in your path. Running `devtool
modify -n ${PACKAGE}` first creates a new Yocto layer in your build directory
where devtool stores recipe modifications. It then constructs a `.bbappend` for
the the package recipe and uses the `externalsource` class to replace the
download, fetch, and patch steps with no-ops. The result is that when you build
the package, it will use the local source directory as is. Keep in mind that
the package recipe does not do a clean nor does the externalsource class know
when the source has been changed. If you change the source, you'll need to run
`bitbake -c cleansstate ${PACKAGE}` to clear BitBake's caches otherwise BitBake
will just used the cached result. Depending on what you are doing, you may need
to run 'make mrproper' in the ${PACKAGE} source directory to clear any built
objects. When you are all done, run `devtool reset ${PACKAGE}` to remove the
`.bbappend` from the devtool Yocto layer. The source will be untouched but
BitBake will go back to using the version specified in the recipe.

Further information can be found in the [Yocto Mega Manual][1].

[1]: (http://www.yoctoproject.org/docs/2.1/mega-manual/mega-manual.html) "Yocto Mega Manual"
