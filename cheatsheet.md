
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
$ bitbake obmc-phosphor-image
```

## Building for Barreleye

The Barreleye target is `barreleye`.

If you are starting from scratch without a `build/conf` directory you can just:
```
$ cd openbmc
$ TEMPLATECONF=meta-openbmc-machines/meta-openpower/meta-rackspace/meta-barreleye/conf . oe-init-build-env
$ bitbake obmc-phosphor-image
```

## Building the OpenBMC SDK
Looking for a way to compile your programs for 'ARM' but you happen to be running on a 'PPC' or 'x86' system?  You can build the sdk receive a fakeroot environment.  
```
$ bitbake -c populate_sdk obmc-phosphor-image
$ ./tmp/deploy/sdk/openbmc-phosphor-glibc-x86_64-obmc-phosphor-image-armv5e-toolchain-1.8+snapshot.sh
```
Follow the prompts.  After it has been installed the default to setup your env will be similar to this command
```
. /opt/openbmc-phosphor/1.8+snapshot/environment-setup-armv5e-openbmc-linux-gnueabi
```

## Rebuilds & Reconfiguration

You can reconfigure your build by removing the build/conf dir:
```
rm -rf build/conf
```
and running `oe-init-build-env` again (possibly with `TEMPLATECONF` set).

## Useful dbus CLI tools

## `busctl`

http://www.freedesktop.org/software/systemd/man/busctl.html

Great tool to issue dbus commands via cli. That way you don't have to wait for
the code to hit the path on the system. Great for running commands with QEMU
too!

Run as:

```
busctl call <path> <interface> <object> <method> <parameters>
```

* \<parameters\> example : sssay "t1" "t2" "t3" 2 2 3

## Using QEMU

QEMU has a palmetto-bmc machine (as of v2.6.0) which implements the core
devices to boot a Linux kernel. OpenBMC also [maintains a
tree](https://github.com/openbmc/qemu) with patches on their way upstream or
temporary work-arounds that add to QEMU's capabilities where appropriate.

QEMU's wiki has instructions for [building from
source](http://wiki.qemu.org/Documentation/GettingStartedDevelopers).

Assuming the CWD is the root of the openbmc tree, a palmetto-bmc machine can be
invoked with:

```
qemu-system-arm \
    -M palmetto-bmc \
    -m 256 \
    -append "console=ttyS4" \
    -nographic \
    -kernel build/tmp/deploy/images/palmetto/cuImage-palmetto.bin \
    -dtb build/tmp/deploy/images/palmetto/cuImage-aspeed-bmc-opp-palmetto.dtb \
    -initrd build/tmp/deploy/images/palmetto/obmc-phosphor-image-palmetto.cpio.gz
```

To quit, type `Ctrl-a c` to switch to the QEMU monitor, and then `quit` to exit.
