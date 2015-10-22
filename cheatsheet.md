
# OpenBMC cheatsheet

This document is intended to provide a set of recipes for common OpenBMC
customisation tasks, without having to know the full yocto build process.

## Using a local kernel build

The kernel recipe is in:

```
 meta-phosphor/common/recipes-kernel/linux/linux-obmc_4.2.bb
```

To use a local git tree, change the `SRC_URI` to a git:// URL without
a hostname. For example:

```
SRC_URI = "git:///home/jk/devel/linux;protocol=git;branch=${KBRANCH}"
```

The `SRCREV` variable can be used to set an explicit git commit. The
default (`${AUTOREV}`) will use the latest commit in `KBRANCH`.

## Building for Palmetto

The Palmetto target is `palmetto`.

If you are starting from scratch without a `build/conf` directory you can just:
```
$ cd openbmc
$ TEMPLATECONF=meta-openbmc-machines/meta-openpower/meta-ibm/meta-palmetto/conf . oe-init-build-env
$ obmc-phosphor-image
```

## Rebuilds & Reconfiguration

You can reconfigure your build by removing the build/conf dir:
```
rm -rf build/conf
```
and running `oe-init-build-env` again (possibly with `TEMPLATE_CONF` set).
