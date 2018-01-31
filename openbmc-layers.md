Adding a new system
================

1) Create a new directory following the
meta-openbmc-machines/meta-[architecture]/meta-[company]/meta-[system]
structure.

2) Create the following files and directories in the newly created directory.
Many examples can be found under
meta-openbmc-machines/meta-[architecture]/meta-[company]/meta-[system](https://github.com/openbmc/openbmc/tree/master/meta-openbmc-machines)

 * `conf/bblayers.conf.sample` which will define the BitBake layers for the
system. This should include the layers mentioned below: Yocto, OpenBMC, BSP,
architecture, and machine layers.

 * `conf/conf-notes.txt` will define the common targets of the system.
`conf-notes.txt` should include `obmc-phosphor-image`, the OpenBMC image.
The `conf-notes.txt` file might look like:

```
Common targets are:
      obmc-phosphor-image
```

 * `conf/layer.conf` which allows recipes in the layer to be located.
An example of layer.conf from the Zaius system:

```
# We have a conf and classes directory, add to BBPATH
BBPATH .= ":${LAYERDIR}"

# We have recipes-* directories, add to BBFILES
BBFILES += "${LAYERDIR}/recipes-*/*/*.bb \
            ${LAYERDIR}/recipes-*/*/*.bbappend"

BBFILE_COLLECTIONS += "zaius"
BBFILE_PATTERN_zaius = ""
```

 * `conf/local.conf.sample` is where all local user settings are placed. A
sample is located
[here](https://git.yoctoproject.org/cgit.cgi/poky/plain/meta-poky/conf/local.conf.sample).

 * `conf/machine/<system>.conf` Since we are creating a new machine

 * `recipes.txt` which documents any recipe directories in the layer.
A system that includes the `recipes-phosphor` (OpenBMC user space application
and configuration recipes) and `recipes-kernel` directories would look like:

```
recipes-kernel       - The kernel and generic applications/libraries withstrong kernel dependencies
recipes-phosphor     - Phosphor OpenBMC applications and configuration

```

TODO: recipes-kernel and recipes-phosphor

TODO: kernel config


 * Any recipes directories (e.g. recipes-kernel and recipes-phosphor)

 * A README with information regarding what's contained in the layer and any
dependencies


OpenBMC BitBake layers
================

The Yocto Project uses the OpenEmbedded Build System which uses BitBake as its
build tool. A BitBake layer is a collection of recipes and/or configuration.

This example and the write up that follows detail the BitBake layers of the
Witherspoon system.

```
$ bitbake-layers show-layers
layer                 path                                      priority
==========================================================================
meta                  /esw/san5/gmills/obmc/openbmc/meta        5
meta-poky             /esw/san5/gmills/obmc/openbmc/meta-poky   5
meta-oe               /esw/san5/gmills/obmc/openbmc/import-layers/meta-openembedded/meta-oe  6
meta-networking       /esw/san5/gmills/obmc/openbmc/import-layers/meta-openembedded/meta-networking  5
meta-perl             /esw/san5/gmills/obmc/openbmc/import-layers/meta-openembedded/meta-perl  6
meta-python           /esw/san5/gmills/obmc/openbmc/import-layers/meta-openembedded/meta-python  7
meta-virtualization   /esw/san5/gmills/obmc/openbmc/import-layers/meta-virtualization  8
meta-phosphor         /esw/san5/gmills/obmc/openbmc/meta-phosphor  6
meta-aspeed           /esw/san5/gmills/obmc/openbmc/meta-openbmc-bsp/meta-aspeed  6
meta-ast2500          /esw/san5/gmills/obmc/openbmc/meta-openbmc-bsp/meta-aspeed/meta-ast2500  6
meta-openpower        /esw/san5/gmills/obmc/openbmc/meta-openbmc-machines/meta-openpower  6
meta-ibm              /esw/san5/gmills/obmc/openbmc/meta-openbmc-machines/meta-openpower/meta-ibm  6
meta-witherspoon      /esw/san5/gmills/obmc/openbmc/meta-openbmc-machines/meta-openpower/meta-ibm/meta-witherspoon  6
```

Layer Priority: Each layer has a priority, which is used by BitBake to decide
which layer takes precedence if there are recipe files with the same name in
multiple layers. A higher numeric value represents a higher priority.

The Witherspoon layers are defined
[here](https://github.com/openbmc/openbmc/blob/master/meta-openbmc-machines/meta-openpower/meta-ibm/meta-witherspoon/conf/bblayers.conf.sample).

### Yocto Layers
The meta, meta-poky, meta-oe, meta-networking, meta-perl, meta-python,
and meta-virtualization layers all come from the Yocto Project and are updated
when Yocto is updated.

 * meta-poky contains the OpenEmbedded Build System (BitBake and OpenEmbedded
Core) as well as a set of metadata to get you started building.

 * meta-oe contains additional OpenEmbedded (OE) metadata.

 * meta-networking is the central point for networking-related packages and
configuration.

 * meta-perl and meta-python are home to OpenEmbedded Perl and python modules
respectfully.

 * meta-virtualization provides support for building Xen, KVM, Libvirt, and
associated packages necessary for constructing OpenEmbedded-based virtualized
solutions.

More about meta, meta-poky, meta-oe, meta-networking, meta-perl, meta-python,
and meta-virtualization can be found
[here](https://layers.openembedded.org/layerindex/branch/master/layers/)

### OpenBMC layer
meta-phosphor is the OpenBMC layer. This layer should be included for all
OpenBMC systems. The OpenBMC layer contains content which is shared between all
OpenBMC systems.

### Board Support Package (BSP) layers
meta-aspeed and meta-ast2500 are  BSP layers.
A BSP is a collection of information that defines how to support a particular
hardware device. The OpenBMC BSP layer includes the service management SOCs,
such as ASPEED's AST2500 in this case.

### Architecture Layer
meta-openpower is the OpenPOWER layer and should be included for all OpenPOWER
systems. This layer encompasses content that is intended for all OpenPOWER
systems. Similar layers are available for other architectures.

### Machine Layer
meta-witherspoon is the machine layer. This layer is intended for content
specific to the Witherspoon system.

Learn more about Yocto Project by visiting the
[Yocto Project Manual](http://www.yoctoproject.org/docs/current/ref-manual/ref-manual.html).
