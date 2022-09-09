
# OpenBMC cheatsheet

This document is intended to provide a set of recipes for common OpenBMC
customisation tasks, without having to know the full yocto build process.

## Using a local kernel build

The kernel recipe is in:

```
 meta-phosphor/common/recipes-kernel/linux/linux-obmc_X.Y.bb
```

To use a local git tree, change the `SRC_URI` to a git:// URL without
a hostname, and remove the `protocol=git` parameter. For example:

```
SRC_URI = "git:///home/jk/devel/linux;branch=${KBRANCH}"
```

The `SRCREV` variable can be used to set an explicit git commit, or
set to `"${AUTOREV}"` to use the latest commit in `KBRANCH`.

## Building for Palmetto

The Palmetto target is `palmetto`.

```
$ cd openbmc
$ . setup palmetto
$ bitbake obmc-phosphor-image
```

## Building for Zaius

The Zaius target is `zaius`.

```
$ cd openbmc
$ . setup zaius
$ bitbake obmc-phosphor-image
```

## Building a specific machine configuration

If the system you want to build contains different machine configurations:

```
meta-<layer>/meta-<system>/conf/machine/machineA.conf
meta-<layer>/meta-<system>/conf/machine/machineB.conf
```

You can specify the machine configuration you want to build by passing the
name to the `setup`.

```
$ cd openbmc
$ . setup machineB
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
and running `setup` again (possibly with `TEMPLATECONF` set).

## Useful D-Bus CLI tools

## `busctl`

http://www.freedesktop.org/software/systemd/man/busctl.html

Great tool to issue D-Bus commands via cli. That way you don't have to wait for
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
-net user,hostfwd=:127.0.0.1:2222-:22,hostfwd=:127.0.0.1:2443-:443,hostname=qemu
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
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"data": [ "root", "0penBmc" ] }' https://${bmc}/login
```

Connect to host console:
```
ssh -p 2200 root@bmc
```

Power on:
```
curl -c cjar -b cjar -k -H "Content-Type: application/json" -X PUT \
  -d '{"data": "xyz.openbmc_project.State.Host.Transition.On"}' \
  https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
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
ulimit -c unlimited
```

## Cleaning up read-write file system changes

You may want to investigate which file(s) are persisting through the overlay
rwfs.  To do this, you can list this path and then remove those files which
you'd prefer the originals or remove the deletion overlay to restore files.

```
/run/initramfs/rw/cow/
```

## Building

### Share downloads directory
It takes a long time for the first build of OpenBMC. It downloads various repos
from the internet.

Check `build/downloads` to see all the downloaded repos.

* If a repo is a single archive, it usually looks like this:
   * `zlib-1.2.11.tar.xz` - The repo itself
   * `zlib-1.2.11.tar.xz.done` - A flag indicating the repo is downloaded
* If a repo is managed by git, it usually looks like this:
   * `git2/github.com.openbmc.linux` - The git bare clone
   * `git2/github.com.openbmc.linux.done` - A flag indicating the repo is downloaded

Bitbake will extract the code to the working directory during build, so the
`downloads` directory could be shared by different builds on a system:

* Set `DL_DIR` Bitbake environment variable to the location of your shared
   downloads directory by editing the `build/conf/local.conf` file:
   ```
   DL_DIR ?= "<path>/<to>/<existing>/downloads"
   ```
* Or create a symbol link:
   ```
   ln -sf <path>/<to>/<existing>/downloads build/downloads
   ```
Then do the build.  It will save a lot of time from downloading codes.

## Using git proxy
If you experience extremely slow download speed during code fetch (e.g. if you
are in China), it is possible to use a git proxy to speed up the code fetch.

Google `git-proxy-wrapper` will find various ways to setup the proxy for the
git protocol.

Below is an example wrapper in `~/bin` assuming a socks5 proxy at port 9054:
```
#!/bin/sh
## Use connect-proxy as git proxy wrapper which supports SOCKS5
## Install with `apt-get install connect-proxy`
## Use with `export GIT_PROXY_COMMAND=~/bin/git-proxy-wrapper`
/usr/bin/connect -S localhost:9054 "$@"
```
Then you can run `export GIT_PROXY_COMMAND=~/bin/git-proxy-wrapper` and you are
now downloading git code through your proxy.

## devtool

`devtool` is a convenient utility in Yocto to make changes in the local
directory.
Typical usage is:
```
# To create a local copy of recipe's code and build with it:
devtool modify <recipe>
cd build/workspace/sources/<recipe>  # And make changes
bitbake obmc-phosphor-image  # Build with local changes

# After you have finished, reset the recipe to ignore local changes:
devtool reset <recipe>
```

To use this tool, you need the build environment, e.g. `. oe-init-build-env`.
The above script will add `<WORKDIR>/scripts/` to your `PATH` env and
`devtool` is in the path.

Below are real examples.


### devtool on ipmi

If you want to debug or add a new function in ipmi, you probably need to
change the code in [phosphor-host-ipmid][1].
Checking the recipes, you know this repo is in [phosphor-ipmi-host.bb][2].
Below are the steps to use devtool to modify the code locally, build and test
it.
1. Use devtool to create a local repo:
   ```
   devtool modify phosphor-ipmi-host
   ```
   devtool clones the repo into `build/workspace/sources/phosphor-ipmi-host`,
   creates and checkout branch `devtool`.
2. Make changes in the repo, e.g. adding code to handle new ipmi commands or
   simply adding trace logs.
3. Now you can build the whole image or the ipmi recipe itself:
   ```
   bitbake obmc-phosphor-image  # Build the whole image
   bitbake phosphor-ipmi-host  # Build the recipe
   ```
4. To test your change, either flash the whole image or replace the changed
   binary. Note that the changed code is built into `libapphandler.so` and it
   is used by both host and net ipmi daemon.
   It is recommended that you copy the changed binary to BMC because it is
   easier to test:
   ```
   # Replace libapphandler.so.0.0.0
   scp build/workspace/sources/phosphor-ipmi-host/oe-workdir/package/usr/lib/ipmid-providers/libapphandler.so.0.0.0 root@bmc:/usr/lib/ipmid-providers/
   systemctl restart phosphor-ipmi-host.service  # Restart the inband ipmi daemon
   # Or restart phosphor-ipmi-net.service if you want to test net ipmi.
   ```
5. Now you can test your changes.


## Develop linux kernel

### devtool on linux kernel
If you want to work on linux kernel, you can use devtool as well, with some
differences from regular repos.

**Note**: As of [ac72846][3] the linux kernel recipe name is changed to
`linux-aspeed` for Aspeed based OpenBMC builds.
In the following examples, replace `linux-obmc` with `linux-aspeed` if you are
on a revision later than [ac72846][3].

1. devtool does not create the 'devtool' branch. Instead, it checkout the
   branch specified in the recipe.
   For example, on the OpenBMC v2.2 tag, `linux-obmc_4.13.bb` specifies
   `dev-4.13` branch.
2. If there are patches, `devtool` applies them directly on the branch.
3. devtool copies the defconfig and machine-specific config into `oe-workdir`.
4. devtool generates the `.config` file based on the above configs.

You can modify the code and build the kernel as usual as follows:
```
bitbake linux-obmc -c build
```

### Modify config
If you need to change the config and save it as defconfig for further use:
```
bitbake linux-obmc -c menuconfig
# Edit the configs and after save it generates
# .config.new as the new kernel config

bitbake linux-obmc -c savedefconfig
# It will save the new defconfig at oe-workdir/linux-obmc-<version>/defconfig
```

### Test linux kernel
After build, you can flash the image to test the new kernel.
However, it is always slow to flash an image to the chip.

There is a faster way to load the kernel via network so you can easily test
kernel builds.

OpenBMC kernel build generates `fit` image, including `kernel`, `dtb` and
`initramfs`.
Typically we can load it via tftp, taking Romulus as an example:
1. Put `build/tmp/deploy/images/romulus/fitImage-obmc-phosphor-initramfs-romulus.bin`
   to a tftp server, name it to `fitImage`
2. Reboot BMC and press keys to enter uboot shell;
3. In uboot:
   ```
   setenv ethaddr <mac:addr>  # Set mac address if there it is unavailable
   setenv ipaddr 192.168.0.80  # Set BMC IP
   setenv serverip 192.168.0.11  # Set tftp server IP
   tftp 0x83000000 fitImage  # Load fit image to ram. Use 0x43000000 on AST2400
   bootm 0x83000000  # Boot from fit image
   ```
Then you are running an OpenBMC with your updated kernel.


[1]: https://github.com/openbmc/phosphor-host-ipmid
[2]: https://github.com/openbmc/openbmc/blob/c53f375a0f92f847d2aa50e19de54840e8472c8e/meta-phosphor/recipes-phosphor/ipmi/phosphor-ipmi-host_git.bb
[3]: https://github.com/openbmc/openbmc/commit/ac7284629ea572cf27d69949dc4014b3b226f14f
