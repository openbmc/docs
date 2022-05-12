# OpenBMC Development Environment

**Document Purpose:** How to set up an OpenBMC development environment

**Audience:** Programmer familiar with Linux and BMCs

**Prerequisites:** Current Linux, Mac, or Windows system

## Overview

OpenBMC uses the [Yocto](https://www.yoctoproject.org/) Project as its
underlying building and distribution generation framework. The main
OpenBMC [README](https://github.com/openbmc/openbmc/blob/master/README.md)
provides information on getting up and going with Yocto and OpenBMC.
There are mechanisms to use this process to build your changes but it can be
slow and cumbersome for initial debug and validation of your software. This
guide focuses on how to test new changes quickly using [QEMU](https://www.qemu.org/).

QEMU is a software emulator that can be used to run OpenBMC images.

This doc walks through the recommended steps for setting up an OpenBMC
development environment.

## Install Linux Environment

If you are running Linux, and are ok with installing some additional packages,
then you can skip to step 3.

The recommended OpenBMC development environment is the latest Ubuntu LTS
release. Other versions of Linux may work but you are using that at your own
risk. If you have Windows or Mac OS then VirtualBox is the recommended
virtualization tool to run the development environment.

1. Install either [VirtualBox](https://www.virtualbox.org/wiki/Downloads) or
[VMware](https://www.vmware.com/products/workstation-player/workstation-player-evaluation.html)
onto your computer (Mac, Windows, Linux)

  Both have free versions available for what you need. VirtualBox is what the
  majority of core OpenBMC development is using. **Note:** If you want to use
  this VM to BitBake a full OpenBMC image, you'll want to allocate as many
  resources as possible. Ideal minimum resources are 8 threads, 16GB memory,
  200GB hard drive. Just using for SDK builds and QEMU should work fine with the
  normal defaults on a VM.

2. Install the latest Ubuntu LTS release

  The majority of OpenBMC development community uses Ubuntu.  The qemu below
  is built on [18.04](http://releases.ubuntu.com/18.04/) but whatever is most
  recent *should* work. The same goes for other Linux distributions like
  Fedora but again, these are not tested nearly as much by the core OpenBMC
  team as Ubuntu.

  **VirtualBox Tips** - You'll want copy/paste working between your VM and Host.
  To do that, once you have your VM up and running:
  - Devices -> Insert Guest Additions CD Image (install)
  - Devices -> Shared Clipboard -> Bidirectional
  - reboot (the VM)

3. Install required packages

  Refer to
  [Prerequisite](https://github.com/openbmc/openbmc/blob/master/README.md#1-prerequisite)
  link.

  **Note** - In Ubuntu, a "sudo apt-get update" will probably be needed before
  installing the packages.

## Initialize OpenBMC repo & Start QEMU Session

1. Pick a place for OpenBMC to live and clone the repo

  ```
  mkdir ~/bmc
  cd ~/bmc
  git clone git@github.com:openbmc/openbmc.git
  ```

2. From your new OpenBMC repo, source the Qemu enviroment

  ```
  source oe-init-build-env build && umask 022
  ```

3. Build the image

  ```
  bitbake obmc-phosphor-image
  ```

4. Start QEMU with the built image

  ```
  runqemu nographic 
  ```

5. Wait for your QEMU-based BMC to boot

  Login using default root/0penBmc login (Note the 0 is a zero).

6. (Optional) Setup network access

  ```
  ip a a 192.168.7.2/24 dev eth0
  ```

7. Check the system state

  You'll see a lot of services starting in the console, you can start running
  the obmcutil tool to check the state of the OpenBMC state services. When you
  see the following then you have successfully booted to "Ready" state.

  ```
  root@openbmc:~# obmcutil state
  CurrentBMCState     : xyz.openbmc_project.State.BMC.BMCState.Ready
  CurrentPowerState   : xyz.openbmc_project.State.Chassis.PowerState.Off
  CurrentHostState    : xyz.openbmc_project.State.Host.HostState.Off
  ```

  **Note** To exit (and kill) your QEMU session run: `ctrl+a x`
