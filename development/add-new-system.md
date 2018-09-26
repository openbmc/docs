# Add a New System to OpenBMC

**Document Purpose:** How to add a new system to the OpenBMC distribution

**Audience:** Programmer familiar with BitBake

**Prerequisites:** Completed Development Environment Setup [Document](https://github.com/openbmc/docs/blob/master/development/dev-environment.md)

### Overview

The OpenBMC distribution is based on [Yocto](https://www.yoctoproject.org/).
Yocto is a project that allows developers to create custom Linux distributions.
OpenBMC uses Yocto to create their embedded Linux distribution to run on
a variety of devices.

Yocto has a concept of hierarchical layers. When you build a Yocto-based
distribution, you define a set of layers for that distribution. OpenBMC uses
many common layers from Yocto as well as some of its own layers. The layers
defined within OpenBMC can be found with the meta-* directories in OpenBMC
[GitHub](https://github.com/openbmc/openbmc).

Yocto layers are a combination of different files that define packages to
incorporate in that layer. One of the key file types used in these layers are
[BitBake](https://github.com/openembedded/bitbake/blob/master/README) recipes.

BitBake is a fully functional language in itself. For this lesson, we will
focus on only the aspects of BitBake required to understand the process of
adding a new system.

### Start the Initial BitBake

For this work, you will need to have allocated at least 100GB of space to your
development environment and as much memory and CPU as possible. The initial
build of an OpenBMC distribution can take hours. Once that first build is done
though, future builds will use cached data from the first build, speeding the
process up by orders of magnitude.

So before we do anything else, let's get that first build going.

Follow the direction on the OpenBMC Github [page](https://github.com/openbmc/openbmc/blob/master/README.md#2-download-the-source)
for the Romulus system (steps 2-4).

### Create a New System

While the BitBake operation is going above, let's start creating our new system.
Similar to previous lessons, we'll be using Romulus as our reference. Our new
system will be called romulus-prime.

From your openbmc repository you cloned above, the Romulus layer is defined within ``meta-ibm/meta-romulus/``.  The Romulus layer is defined within the ``conf`` subdirectory. Within there you will see a layout like this:

```
meta-ibm/meta-romulus/conf/
├── bblayers.conf.sample
├── conf-notes.txt
├── layer.conf
├── local.conf.sample
└── machine
    └── romulus.conf
```

To create our new romulus-prime system we are going to start out by copying
our romulus layer.
```
cp -R meta-ibm/meta-romulus meta-ibm/meta-romulus-prime
```

Let's review and modify as needed each file in our new layer

1. meta-ibm/meta-romulus-prime/conf/bblayers.conf.sample

  This file defines the layers to pull into the meta-romulus-prime distribution.
  You can see in it a variety of yocto layers (meta, meta-poky,
  meta-openembedded/meta-oe, ...). It also has OpenBMC layers like
  meta-phosphor, meta-openpower, meta-ibm, and meta-ibm/meta-romulus.

  The only change you need in this file is to change the two instances of
  meta-romulus to meta-romulus-prime. This will ensure your new layer is used
  when building your new system.

2. meta-ibm/meta-romulus-prime/conf/conf-notes.txt

  This file simply states the default target the user will build when working
  with your new layer. This remains the same as it is common for all OpenBMC
  systems.

3. meta-ibm/meta-romulus-prime/conf/layer.conf

  The main purpose of this file is to tell BitBake where to look for recipes
  (\*.bb files). Recipe files end with a ``.bb`` extension and are what contain
  all of the packaging logic for the different layers. ``.bbapend`` files are
  also recipe files but provide a way to append onto ``.bb`` files.
  ``.bbappend`` files are commonly used to add or remove something from a
  corresponding ``.bb`` file in a different layer.

  The only change you need in here is to find/replace the "romulus-layer" to
  "romulus-prime-layer"

4. meta-ibm/meta-romulus-prime/conf/local.conf.sample

  This file is where all local configuration settings go for your layer. The
  documentation in it is well done and it's worth a read.

  The only change required in here is to change the ``MACHINE`` to
  ``romulus-prime``.

5. meta-ibm/meta-romulus-prime/conf/machine/romulus.conf

  This file describes the specifics for your machine. You define the kernel
  device tree to use, any overrides to specific features you will be pulling
  in, and other system specific pointers. This file is a good reference for
  the different things you need to change when creating a new system (kernel
  device tree, MRW, LED settings, inventory access, ...)

  The first thing you need to do is rename the file to romulus-prime.conf.

  **Note** If our new system really was just a variant of Romulus,
  with the same hardware configuration, then we could have just created a
  new machine in the Romulus layer. Any customizations for that system
  could be included in the corresponding .conf file for that new machine. For
  the purposes of this exercise we are assuming our romulus-prime system has
  at least a few hardware changes requiring us to create this new layer.

## Build New System

This will not initially compile but it's good to verify a few things form the
initial setup are done correctly.

Do not start this step until the build we started at the beginning of this
lesson has completed.

1. Modify the conf for your current build

  Within the shell you did the initial "bitbake" operation you need to reset
  the conf for your build. You can manually copy in the new files or just
  remove it and let bitbake do it for you:
  ```
  cd ..
  rm -r ./build/conf
  export TEMPLATECONF=meta-ibm/meta-romulus-prime/conf
  . openbmc-env
  ```

  Run your bitbake command.

2. Nothing RPROVIDES 'romulus-prime-config'

  This will be your first error after running "bitbake obmc-phosphor-image"
  against your new system.

  The openbmc/skeleton repository was used for initial prototyping of OpenBMC.
  Within this repository is a [configs](https://github.com/openbmc/skeleton/tree/master/configs) directory.

  The majority of this config data is no longer used but until it is all
  completely removed, you need to provide it.

  Since this repository and file are on there way out, we'll simply do a quick
  workaround for this issue.
  ```
  cp meta-ibm/meta-romulus-prime/recipes-phosphor/workbook/romulus-config.bb meta-ibm/meta-romulus-prime/recipes-phosphor/workbook/romulus-prime-config.bb

  vi meta-ibm/meta-romulus-prime/recipes-phosphor/workbook/romulus-prime-config.bb

  SUMMARY = "Romulus board wiring"
  DESCRIPTION = "Board wiring information for the Romulus OpenPOWER system."
  PR = "r1"

  inherit config-in-skeleton

  #Use Romulus config
  do_make_setup() {
          cp ${S}/Romulus.py \
                  ${S}/obmc_system_config.py
          cat <<EOF > ${S}/setup.py
  from distutils.core import setup

  setup(name='${BPN}',
      version='${PR}',
      py_modules=['obmc_system_config'],
      )
  EOF
  }

  ```

  Re-run your bitbake command.

3. Fetcher failure for URL: file://romulus.cfg

  This is the config file required by the kernel. It's where you can put some
  additional kernel config parameters. In this case, we will just stick with
  what Romulus uses. We just need to add the -prime to the prepend path.
  ```
  vi ./meta-ibm/meta-romulus-prime/recipes-kernel/linux/linux-aspeed_%.bbappend

  FILESEXTRAPATHS_prepend_romulus-prime := "${THISDIR}/${PN}:"
  SRC_URI += "file://romulus.cfg"
  ```

  Re-run your bitbake command.

4. No rule to make target arch/arm/boot/dts/aspeed-bmc-opp-romulus-prime.dtb

  The .dtb file is a device tree blob file. It is generated during the Linux
  kernel build based on its corresponding .dts file. When you introduce a
  new OpenBMC system, you need to send these kernel updates upstream. The
  linked email [thread](https://lists.ozlabs.org/pipermail/openbmc/2018-September/013260.html)
  is an example of this process. Upstreaming to the kernel is a lesson in
  itself. For this lesson, we will simply use the Romulus kernel config files.
  ```
  vi ./meta-ibm/meta-romulus-prime/conf/machine/romulus-prime.conf
  # Replace the ${MACHINE} variable in the KERNEL_DEVICETREE

  # Use romulus device tree
  KERNEL_DEVICETREE = "${KMACHINE}-bmc-opp-romulus.dtb"
  ```

  Re-run your bitbake command.

### Boot New System

  And you've finally built your new systems image! There is more customizations
  to be done but lets first verify what you have boots.

  Your new image will be in the following location from where you ran your
  bitbake command:
  ```
  ./tmp/deploy/images/romulus-prime/obmc-phosphor-image-romulus-prime.static.mtd
  ```
  Copy this image to where you've set up your QEMU session and re-run the
  command to start QEMU (qemu-system-arm command from dev-environment.md),
  giving your new file as input.

  Once booted, you should see the following for the login:
  ```
  romulus-prime login:
  ```

  There you go! You've done the basics of creating, booting, and building a new
  system. This is by no means a complete system but you now have the base for
  the customizations you'll need to do for your new system.

### Further Customizations

  There are a lot of other areas to customize when creating a new system.
  We'll dig into more detail with these (IPMI, HWMON, LED) in future
  development guides.

  Although not in the same format as these guides, [Porting_Guide](https://github.com/mine260309/openbmc-intro/blob/master/Porting_Guide.md)
  provides a lot of very useful information as well on adding a new system.
