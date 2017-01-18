
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
$ TEMPLATECONF=meta-openbmc-machines/meta-openpower/meta-ibm/meta-palmetto/conf . openbmc-env
$ bitbake obmc-phosphor-image
```

## Building for Barreleye

The Barreleye target is `barreleye`.

If you are starting from scratch without a `build/conf` directory you can just:
```
$ cd openbmc
$ TEMPLATECONF=meta-openbmc-machines/meta-openpower/meta-rackspace/meta-barreleye/conf . openbmc-env
$ bitbake obmc-phosphor-image
```

## Building the OpenBMC SDK
Looking for a way to compile your programs for 'ARM' but you happen to be running on a 'PPC' or 'x86' system?  You can build the sdk receive a fakeroot environment.  
```
$ bitbake -c populate_sdk obmc-phosphor-image
$ ./tmp/deploy/sdk/openbmc-phosphor-glibc-x86_64-obmc-phosphor-image-armv5e-toolchain-2.1.sh
```
Follow the prompts.  After it has been installed the default to setup your env will be similar to this command
```
. /opt/openbmc-phosphor/2.1/environment-setup-armv5e-openbmc-linux-gnueabi
```

## Rebuilds & Reconfiguration

You can reconfigure your build by removing the build/conf dir:
```
rm -rf build/conf
```
and running `openbmc-env` again (possibly with `TEMPLATECONF` set).

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

```
qemu-system-arm -m 256 -M palmetto-bmc -nographic \
-drive file=<path>/flash-palmetto,format=raw,if=mtd \
-net nic \
-net user,hostfwd=:127.0.0.1:2222-:22,hostfwd=:127.0.0.1:2443-:443,hostname=qemu \
```
If you get an error you likely need to build QEMU (see the section in this document).   If no error and QEMU starts up just change the port when interacting with the BMC...

```
curl -c cjar -b cjar -k -H "Content-Type: application/json" \
-X POST https://localhost:2443/login -d "{\"data\": [ \"root\", \"0penBmc\" ] }"
```
or

```
ssh -p 2222 root@localhost
```

To quit, type `Ctrl-a c` to switch to the QEMU monitor, and then `quit` to exit.

## Building QEMU

```
git clone https://github.com/openbmc/qemu.git
cd qemu
git submodule update --init dtc
mkdir build
cd build
../configure --target-list=arm-softmmu
make
```
Built file will be located at: ```arm-softmmu/qemu-system-arm```

### Use a bridge device
Using a bridge device requires a bit of root access to set it up.  The benefit
is your qemu session runs in the bridges subnet so no port forwarding is needed.
There are packages needed to yourself a virbr0 such as...

```
apt-get install libvirt libvirt-bin bridge-utils uml-utilities qemu-system-common

qemu-system-arm -m 256 -M palmetto-bmc -nographic \
-drive file=<path>/flash-palmetto,format=raw,if=mtd \
-net nic,macaddr=C0:FF:EE:00:00:02,model=ftgmac100  \
-net bridge,id=net0,helper=/usr/lib/qemu-bridge-helper,br=virbr0
```

There are some other useful parms like that can redirect the console to another
window.  This results in having an easily accessible qemu command session.
```-monitor stdio -serial pty -nodefaults```


## Booting the host

Login:
```
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"data": [ "root", "0penBmc" ] }' https://palm5-bmc/login
```

Connect to host console:
```
ssh -p 2200 root@bmc
```

Power on:
```
curl -c cjar -b cjar -k -H "Content-Type: application/json" -X POST     -d '{"data": []}'  https://palm5-bmc/org/openbmc/control/chassis0/action/powerOn
```

## GDB

[SDK build](#building-the-openbmc-sdk) provides GDB and debug symbols:

* `$GDB` is available to use once SDK environment is setup
* Debug symbols are located in `.debug/` directory of each executable

To use GDB:

1. Setup SDK environment;
2. Run below GDB commands:
   ```
   cd <sysroot_of_sdk_build>
   $GDB <relative_path_to_exeutable> <path_to_core_file>
   ```

## Coredump

By default coredump is disabled in OpenBMC. To enable coredump:
```
echo '/tmp/core_%e.%p' | tee /proc/sys/kernel/core_pattern
```


## Yocto Tips ##

### Adding a file to your image ###
Once you have a recipe built with `devtool` (as described [here] (yocto-development.md#Yocto-in-OpenBMC)) you are going to have to make a couple of edits to 1) tell the recipe what to do and 2) tell bitbake why  to build with your new package.  For the example we can just copy a file from your local tmp dir.

```
touch /tmp/hello
devtool add welcome file:///tmp/hello
```

That will create a welcome.bb file; open it up.  Tell the recipe what to do by adding commands to the do_install() function and tell bitbake what the bb file provides.

```
vim workspace/recipes/welcome/welcome.bb

RPROVIDES_${PN} = "welcome"
do_install () {
        # Specify install commands here
        install -d ${D}/home/root
        install -m 0644 ${S}/hello ${D}/home/root
}
FILES_${PN} = "/home/root/hello"
```
Final Step is to tell bitbake... what the recipe is cooking!

```
vim conf/local.conf
IMAGE_INSTALL_append = " welcome"
```

That's it, recompile and boot your system, the file `hello` will be in `/home/root`.

There is another neat way to find if your package made it in to the image without the labourous effort of flashing and booting a system.  NXP wrote a few examples of [useful] (https://community.nxp.com/docs/DOC-94953) commands.

```
bitbake -g obmc-phosphor-image && cat pn-depends.dot |grep welcome
```
