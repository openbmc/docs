OpenBMC OpenEmbedded layers
================

The Yocto Project uses the OpenEmbedded Build System. An OpenEmbedded layer is a
collection of recipes and/or configuration.

This example and the write up that follows detail the OpenEmbedded layers of the
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

### OpenPOWER Layer
meta-openpower is the OpenPOWER layer and should be included for all OpenPOWER
systems. This layer encompasses content that is intended for all OpenPOWER
systems. Similar layers are available for other architectures.

### Machine Layer
meta-witherspoon is the machine layer. This layer is intended for content
specific to the Witherspoon system.

Learn more about Yocto Project by visiting the
[Yocto Project Manual](http://www.yoctoproject.org/docs/current/ref-manual/ref-manual.html).
