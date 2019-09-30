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
guide focuses on how to test new changes quickly using the OpenBMC Software
Development Kit (SDK) and [QEMU](https://www.qemu.org/).

The SDK is a group of packages that are built during a BitBake operation.
BitBake is the tool used to build Yocto based distributions. The SDK provides
all required libraries and cross compilers to build OpenBMC applications. The
SDK is not used to build entire OpenBMC flash images, it provides a mechanism to
compile OpenBMC applications and libraries that you can then copy onto a running
system for testing.

QEMU is a software emulator that can be used to run OpenBMC images.

This doc walks through the recommended steps for setting up an OpenBMC
development environment and installing the needed SDK.

For testing purposes, this guide uses the Romulus system as the default because
this is the system tested for each CI job, which means it's the most stable.

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

  The majority of OpenBMC dev is on [16.04](http://releases.ubuntu.com/16.04/)
  but whatever is most recent *should* work. The same goes for other Linux
  distributions like Fedora but again, these are not tested nearly as much by
  the core OpenBMC team as Ubuntu.

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

## Download and Start QEMU Session

1. Download latest openbmc/qemu fork of QEMU application

  ```
  wget https://openpower.xyz/job/openbmc-qemu-build-merge-x86/lastSuccessfulBuild/artifact/qemu/arm-softmmu/qemu-system-arm

  chmod u+x qemu-system-arm
  ```

2. Download the Romulus image.

  ```
  wget https://openpower.xyz/job/openbmc-build/distro=ubuntu,label=builder,target=romulus/lastSuccessfulBuild/artifact/deploy/images/romulus/obmc-phosphor-image-romulus.static.mtd
  ```

3. Start QEMU with downloaded Romulus image

  **Note** - For REST and SSH to work into your QEMU session, you must connect
  up some host ports to the REST and SSH ports in your QEMU session. In this
  example, it just uses 2222 and 2443. You can use whatever you prefer.
  ```
  ./qemu-system-arm -m 256 -M romulus-bmc -nographic -drive file=./obmc-phosphor-image-romulus.static.mtd,format=raw,if=mtd -net nic -net user,hostfwd=:127.0.0.1:2222-:22,hostfwd=:127.0.0.1:2443-:443,hostname=qemu
  ```

   **Note** - By default, Jenkins and openbmc-test-automation use SSH and HTTPS
   ports 22 and 443, respectively. SSH connection to use a user-defined port 2222
   might not be successful. To use SSH port 22 and HTTPS port 443:
   ```
   ./qemu-system-arm -m 256 -machine romulus-bmc -nographic -drive file=./obmc-phosphor-image-romulus.static.mtd,format=raw,if=mtd -net nic -net user,hostfwd=:127.0.0.1:22-:22,hostfwd=:127.0.0.1:443-:443,hostfwd=tcp:127.0.0.1:80-:80,hostfwd=tcp:127.0.0.1:2200-:2200,hostfwd=udp:127.0.0.1:623-:623,hostfwd=udp:127.0.0.1:664-:664,hostname=qemu
   ```

4. Wait for your QEMU-based BMC to boot

  Login using default root/0penBmc login (Note the 0 is a zero).

5. Check the system state

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

## Download and Install SDK

To keep things organized, create a directory to store your SDK install scripts
and directories.

1. Download the latest SDK to your system (using Romulus for this walk through)

  ```
  mkdir -p ~/sdk
  cd ~/sdk

  wget https://openpower.xyz/job/openbmc-build-sdk/distro=ubuntu,target=romulus/lastSuccessfulBuild/artifact/deploy/sdk/oecore-x86_64-arm1176jzs-toolchain-nodistro.0.sh

  chmod u+x oecore-x86_64-arm1176jzs-toolchain-nodistro.0.sh
  ```

2. Install the SDK

  Choose an appropriate location and name. It's a good idea to include the date
  and system supported by that SDK in the directory name. For example:

  ```
  mkdir -p ~/sdk/romulus-`date +%F`
  ```

  Run the following command to install the SDK.  When command  asks you to
  "Enter target directory for SDK", enter the directory you created in the
  previous step.

  ```
  ./oecore-x86_64-arm1176jzs-toolchain-nodistro.0.sh
  ```

  The installation script will indicate progress and give completion messages
  like this:
  ```
  SDK has been successfully set up and is ready to be used.
  Each time you wish to use the SDK in a new shell session, you need to source
  the environment setup script e.g. $ . /...path-to-sdk.../environment-setup-arm1176jzs-openbmc-linux-gnueabi
  ```

3. Source yourself into the SDK

  Ensure no errors. The command to do this will be provided at the end of
  installation. To make your shell use the new SDK environment, you must source
  its `environment-setup` script which was created in the previous step.  You
  may wish to save the required command, for example, cut/paste the text above
  into a README.

That's it, you now have a working development environment for OpenBMC!
