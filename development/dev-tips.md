# Tips

In this article, it introduces various tips during building, developing and
testing OpenBMC.
It assumes that the reader already has experience of OpenBMC or Bitbake and
know the related concepts, e.g. recipes.

Note: refer to [cheatsheet.md][1] for existing tips. This article is expected
to be merged into that doc in future.

### Building

#### Share downloads dir
It takes a long time for the first build of OpenBMC, it downloads various repos
from the internet.

Check `build/downloads`, you will see all the downloaded repos.

* If a repo is a single archive, it usually looks like
   * `zlib-1.2.11.tar.xz` - The repo itself
   * `zlib-1.2.11.tar.xz.done` - A flag indicating the repo is downloaded
* If a repo is managed by git, it usually looks like
   * `git2/github.com.openbmc.linux` - The git bare clone
   * `git2/github.com.openbmc.linux.done` - A flag indicating the repo is downloaded

Bitbake will extract the code to working dir during build, so the `downloads`
directory could be shared by different builds on a system, e.g. by creating a
symbol link.

So if you have cloned openbmc in a different dir and want to build it, you
could:
```
ln -sf <path>/<to>/<existing>/downalods build/downloads
```
And do the build, it will save a lot of time from downloading codes.

#### Using git proxy
If you experience extreme slow download speed during code fetch (e.g. if you
are in China), it is possible to use a git proxy to speed up the code fetch.

Google `git-proxy-wrapper` you will find various ways to setup the proxy for
git protocol.

Below is my wrapper, assuming I have a local socks5 proxy at port 9054
```
#!/bin/sh
## Use connect-proxy as git proxy wrapper which supports SOCKS5
## Install with `apt-get install connect-proxy`
## And use with `export GIT_PROXY_COMMAND=~/bin/git-proxy-wrapper`
/usr/bin/connect -S localhost:9054 "$@"
```
Then you can run `export GIT_PROXY_COMMAND=~/bin/git-proxy-wrapper` and you are
now downloading git code through your proxy.

### devtool

`devtool` is a convenient utility in Yocto to make changes in the local
directory.
Typical usage is:
```
# To create a local copy of recipe's code and build with it
devtool modify <recipe>
cd build/workspace/sources/<recipe>  # And make changes
bitbake obmc-phosphor-image  # Build with local changes

# After you have finished, reset the recipe to ignore local changes
devtool reset <recipe>
```

To use this tool, you need the build environment, e.g. `. oe-init-build-env`.
It will add `<WORKDIR>/scripts/` to your PATH and `devtool` is in it.

Below are real examples.


#### devtool on ipmi

If you want to debug or adding a new function in ipmi, you probably need to
change the code in [phosphor-host-ipmid][2].
Checking the recipes, you know this repo is in [phosphor-ipmi-host.bb][3].
So below are the steps to use devtool to modify the code locally, build and
test it.
1. Use devtool to create a local repo
   ```
   devtool modify phosphor-ipmi-host
   ```
   It clones the repo into `build/workspace/sources/phosphor-ipmi-host`, create
   and checkout branch `devtool`.
2. Make changes in the repo. E.g. adding code to handle new ipmi commands, or
   simply adding trace logs. Say you have modified `apphandler.cpp`
3. Now you can build the whole image or the ipmi recipe itself.
   ```
   bitbake obmc-phosphor-image  # Build the whole image
   bitbake phosphor-ipmi-host  # Build the recipe
   ```
4. To test your change, either flash the whole image or replace the changed
   binary. Note that the changed code is built into `libapphandler.so` and it
   is used by both host and net ipmi daemon.
   Here let's copy the changed binary to BMC because it is easier to test.
   ```
   # Replace libapphandler.so.0.0.0
   scp build/workspace/sources/phosphor-ipmi-host/oe-workdir/package/usr/lib/ipmid-providers/libapphandler.so.0.0.0 root@bmc:/usr/lib/ipmid-providers/
   systemctl restart phosphor-ipmi-host.service  # Restart the inband ipmi daemon
   # Or restart phosphor-ipmi-net.service if you want to test net ipmi
   ```
5. Now you can test your changes.


### Develop linux kernel

#### devtool on linux kernel
If you want to work on linux kernel, you can use devtool as well, with some
differences from regular repos.

**Note**: As of [ac72846][4] the linux kernel recipe name is changed to
`linux-aspeed` for Aspeed based OpenBMC builds.
So replace below `linux-obmc` to `linux-aspeed` if you are on a revision after
that.

1. It does not create `devtool` branch, instead, it checkout the branch
   specified in the recipe.
   E.g. on OpenBMC v2.2 tag, `linux-obmc_4.13.bb` specifies `dev-4.13` branch.
2. If there are patches, `devtool` applies them directly on the branch.
3. It copies the defconfig and machine specific config into `oe-workdir`.
4. It generates `.config` based on the above configs.

You can modify the code and build the kernel as usual by
```
bitbake linux-obmc -c build
```

#### Modify config
If you need to change the config and save it as defconfig for further use:
```
bitbake linux-obmc -c menuconfig
# Edit the configs and after save it generates
# .config.new as the new kernel config

bitbake linux-obmc -c savedefconfig
# It will save the new defconfig at oe-workdir/linux-obmc-<version>/defconfig
```

#### Test linux kernel
After build, you can flash the image to test the new kernel.
However, it is always slow to flash an image to the chip.

There is a faster way to load the kernel via network so you can easily test
kernel builds.

OpenBMC kernel build generates fit image, including kernel, dtb and initramfs.
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


[1]: https://github.com/openbmc/docs/blob/master/cheatsheet.md
[2]: https://github.com/openbmc/phosphor-host-ipmid
[3]: https://github.com/openbmc/openbmc/blob/c53f375a0f92f847d2aa50e19de54840e8472c8e/meta-phosphor/recipes-phosphor/ipmi/phosphor-ipmi-host_git.bb
[4]: https://github.com/openbmc/openbmc/commit/ac7284629ea572cf27d69949dc4014b3b226f14f
