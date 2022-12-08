# OpenBMC kernel development

The OpenBMC project maintains a kernel tree for use by the project. The tree's
general development policy is that code must be upstream first. This is a strong
requirement, not a hard requirement, and exceptions will be made on a
case-by-case basis. If in doubt, start a discussion on the mailing list.

The OpenBMC kernel tree is hosted at https://github.com/openbmc/linux and
contains the set of patches that we carry. Ideally there would be no patches
carried, as everything should be upstream.

Your code will make it into the OpenBMC tree in these ways, from most to least
desirable:

1. When the OpenBMC kernel moves to a new upstream release
2. By backporting upstream commits from a newer kernel version to the OpenBMC
   kernel
3. Patches included in the OpenBMC tree temporarily

## TL;DR

If you require a patch added to the tree, follow these steps:

1. Submit your patch upstream. It doesn't need to be upstream, but it should be
   on it's way
2. Use
   `git format-patch --subject-prefix="PATCH linux dev-4.7" --to=openbmc@lists.ozlabs.org --to=joel@jms.id.au`
   to create a formatted patch

## Developing a new driver

When developing a new driver, your goal is to have the code accepted upstream.
The first step should be to check that there is no existing driver for the
hardware you wish to support. Check the OpenBMC `-dev` tree, check upstream, and
if you do not find support there ask on the mailing list.

Once you are sure a driver needs to be written, you should develop and test the
driver, before sending it upstream to the relevant maintainers. You should feel
welcome to cc the OpenBMC list when sending upstream, so other kernel developers
can provide input where appropriate. Be sure to follow the (upstream development
process)[https://www.kernel.org/doc/Documentation/process/submitting-patches.rst].

In the past patches underwent 'pre-review' on the OpenBMC mailing list. While
this is useful for developers who are not familiar with writing kernel code, it
has lead to confusion about the upstreaming process, so now we do all of our
development in the community.

Once the driver has been accepted upstream, send the good news to the OpenBMC
list with a reference to the upstream tree. This may be Linus' tree, or it might
be one of the subsystem maintainers. From there the OpenBMC kernel team can
decide how best to include your code in the OpenBMC tree.

### Exceptions

There are cases where waiting for upstream acceptance will delay the bring-up of
a new system. This should be avoided through careful planning and early
development of the features upstream, but where this has not happened we can
chose to carry the patches in the OpenBMC tree while the upstream development is
ongoing.

Another exception to the upstream first rule is where patches are modifying
files that are not upstream. This currently includes the aspeed board file
`arch/arm/mach-aspeed/aspeed.c`, and the device tree source files `dts`. The
board file should go away when we get drivers written for all of the
functionality; for now it contains some hacks relating to LPC and early init.

If you find yourself adding to `arch/arm/mach-aspeed/aspeed.c`, first send an
email to the OpenBMC list to get the opinion of the kernel developers. Patches
to `aspeed.c` will be treated with some prejudice as the file will be removed
once we have drivers for all of the Aspeed peripherals.

## Getting existing code in the tree

The OpenBMC kernel is currently based on the 4.7 series. If there is upstream
code you would like backported, send it to the list. Be sure to include the
upstream commit SHA in the commit message.

## Testing

When modifying the tree we currently test on the following platforms:

- Palmetto, an OpenPower Power8 box containing an ast2400 with NCSI networking
- ast2500-evb, the Aspeed dev board with two PHYs
- Witherspoon, an OpenPower Power9 box containing an ast2500 with NCSI
  networking
- qemu-plametto and qemu-romulus

Before submitting patches it is recommended you boot test on at least the Qemu
platforms, and whatever hardware you have available.

# Tips and Tricks

Some general hints for kernel development

## Out-of-tree builds

You can build a kernel out of the yocto environment, by using the initramfs
(from a pre-existing yocto build) directly:

```
make ARCH=arm \
    O=obj \
    CROSS_COMPILE=arm-linux-gnueabihf- \
    CONFIG_INITRAMFS_SOURCE=/path/tp/obmc-phosphor-image-palmetto.cpio.gz
```

(adjust `O` and `CROSS_COMPILE` parameters as appropriate).

You'll need to use `aspeed_g4_defconfig` or `aspeed_g5_defconfig` as your base
kernel configuration.

The cpio can be found in the following yocto output directory:

```
 build/tmp/deploy/images/palmetto/
```

## Building a uImage

To build a uImage (for example, to netboot):

```
# build a zImage using the obmc rootfs
make ARCH=arm \
    O=obj \
    CROSS_COMPILE=arm-linux-gnueabihf- \
    CONFIG_INITRAMFS_SOURCE=/path/tp/obmc-phosphor-image-palmetto.cpio.gz

# create a combined zImage + DTB image
cat obj/arch/arm/boot/zImage \
    obj/arch/arm/boot/dts/aspeed-bmc-opp-palmetto.dtb \
        > obj/aspeed-zimage

# create a uImage
./scripts/mkuboot.sh -A arm -O linux -C none -T kernel \
    -a 0x40008000 -e 0x40008000 -n $USER-`date +%Y%m%d%H%M` \
    -d obj/aspeed-zimage obj/uImage
```

Note that some systems may have upgraded to a FIT-based u-boot, where the old
uImage format is no longer accepted.
