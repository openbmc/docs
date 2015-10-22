# OpenBMC kernel development

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

You'll need to use `aspeed_defconfig` as your base kernel configuration.

The cpio can be found in the following yocto output directory:

```
 build/tmp/deploy/images/palmetto/
```

# Building a uImage

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
