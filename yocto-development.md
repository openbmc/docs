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


### Adding a file to your image ###

There are a lot of examples of working with bitbake out there the [recipe example] (http://docs.openembedded.org/usermanual/html/recipes_examples.html) from openembedded is a great one and the premise of this OpenBMC tailored section.

So you wrote some code.  You've been scp'ing the compiled binary on to the openbmc system for a while and you know there is a better way.  Have it built as part of your flash image.  

Run the devtool command to add your repo to the workspace.  In my example I have a repo out on github that contains my code.

```
devtool add welcome https://github.com/causten/hello.git
```

Now edit the the bb file it created for you.  You can just use `vim` but `devtool` can also edit the recipe `devtool edit-recipe welcome` without having to type the complete path.

Add/Modify these lines.  

```
RPROVIDES_${PN} = "welcome"
do_install () {
        install -m 0755 -d ${D}${bindir} ${D}${datadir}/welcome
        install -m 0644 ${S}/hello ${D}${bindir}
        install -m 0644 ${S}/README.md ${D}${datadir}/welcome/
}
```

The `RPROVIDES_${PN}` is used by bitbake to know what the recipe creates.  The install directives create directories and then copies the files in to them.  Now bitbake will pick them up from the traditional `/usr/bin` and `/usr/shared/doc/hello/README.md`. 


The Final Step is to tell bitbake that you need the `welcome` recipe

```
vim conf/local.conf
IMAGE_INSTALL_append = " welcome"
```

That's it, recompile and boot your system, the binary `hello` will be in `/usr/bin` and the `README.md` will be in `/usr/shared/doc/welcome`.


There is another neat way to find if your package made it in to the image without the labourous effort of flashing and booting a system.  NXP wrote a few examples of [useful] (https://community.nxp.com/docs/DOC-94953) commands.

Further information on [devtool][0] can be found in the [Yocto Mega Manual][1].

[0]: (http://www.yoctoproject.org/docs/2.1/mega-manual/mega-manual.html#devtool-use-devtool-modify-to-enable-work-on-code-associated-with-an-existing-recipe) "devtool"
[1]: (http://www.yoctoproject.org/docs/2.1/mega-manual/mega-manual.html) "Yocto Mega Manual"
