# OpenBMC SDK

The SDK is a group of packages that are built during a BitBake operation.
BitBake is the tool used to build Yocto based distributions. The SDK provides
all required libraries and cross compilers to build OpenBMC applications. The
SDK is not used to build entire OpenBMC flash images, it provides a mechanism to
compile OpenBMC applications and libraries that you can then copy onto a running
system for testing.

For testing purposes, this guide uses the Romulus system as the default because
this is the system tested for each CI job, which means it's the most stable.

## Download and Install SDK

The OpenBMC Software Development Kit (SDK) contains a cross-toolchain and a set
libraries for working on OpenBMC applications. The SDK is installed on the
machine you will use to develop applications for OpenBMC and not on the BMC
itself.

Generally, SDKs for one BMC cannot be used for developing software for other
BMCs. This can be due to platform ABI, libc or kernel differences, or any other
number of choices made in the configuration of the firmware.

Romulus is the BMC platform used for the purpose of this walk-through.

To begin working with the SDK:

1. Download the latest SDK to your system. It's recommended that you create a
  directory to store your SDK scripts and installation directories to keep your
  workspace organised.

  ```
  mkdir -p ~/sdk
  cd ~/sdk

  wget https://jenkins.openbmc.org/job/latest-master-sdk/label=docker-builder,target=romulus/lastSuccessfulBuild/artifact/deploy/sdk/oecore-x86_64-arm1176jzs-toolchain-nodistro.0.sh
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

## Download and Start QEMU Session

1. Download latest openbmc/qemu fork of QEMU application

  ```
  wget https://jenkins.openbmc.org/job/latest-qemu-x86/lastSuccessfulBuild/artifact/qemu/build/qemu-system-arm

  chmod u+x qemu-system-arm
  ```

2. Download the Romulus image.

  ```
  wget https://jenkins.openbmc.org/job/latest-master/label=docker-builder,target=romulus/lastSuccessfulBuild/artifact/openbmc/build/tmp/deploy/images/romulus/obmc-phosphor-image-romulus.static.mtd
  ```

3. Start QEMU with downloaded Romulus image

  **Note** - For REST, SSH and IPMI to work into your QEMU session, you must connect
  up some host ports to the REST, SSH and IPMI ports in your QEMU session. In this
  example, it just uses 2222, 2443, 2623. You can use whatever you prefer.
  ```
  ./qemu-system-arm -m 256 -M romulus-bmc -nographic \
      -drive file=./obmc-phosphor-image-romulus.static.mtd,format=raw,if=mtd \
      -net nic \
      -net user,hostfwd=:127.0.0.1:2222-:22,hostfwd=:127.0.0.1:2443-:443,hostfwd=udp:127.0.0.1:2623-:623,hostname=qemu
  ```

   **Note** - By default, Jenkins and openbmc-test-automation use SSH and HTTPS
   ports 22 and 443, respectively. For the IPMI port 623 is used. SSH connection
   to use a user-defined port 2222 might not be successful. To use SSH port 22,
   HTTPS port 443 and IPMI port 623:
   ```
   ./qemu-system-arm -m 256 -machine romulus-bmc -nographic \
       -drive file=./obmc-phosphor-image-romulus.static.mtd,format=raw,if=mtd \
       -net nic \
       -net user,hostfwd=:127.0.0.1:22-:22,hostfwd=:127.0.0.1:443-:443,hostfwd=tcp:127.0.0.1:80-:80,hostfwd=tcp:127.0.0.1:2200-:2200,hostfwd=udp:127.0.0.1:623-:623,hostfwd=udp:127.0.0.1:664-:664,hostname=qemu
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

## Alternative yocto QEMU

  yocto has tools for building and running qemu. These tools avoid some of the
  configuration issues that come from downloading a prebuilt image, and
  modifying binaries. Using yocto qemu also uses the [TAP
  interface](https://www.kernel.org/doc/Documentation/networking/tuntap.txt)
  which some find be more stable. This is particularly useful when debugging
  at the application level.

  - set up a bmc build environment
  ```
  source setup romulus myBuild/build
  ```
  - add the qemu x86 open embedded machine for testing
  ```
  MACHINE ??= "qemux86"
  ```
  - Make the changes to the build (ie devtool modify bmcweb, devtool add gdb)
  ```
  devtool modify bmcweb myNewLocalbmcweb/
  ```
  - build open bmc for the qemu x86 machine
  ```
  MACHINE=qemux86 bitbake obmc-phosphor-image
  ```
  - run qemu they way yocto provides
  ```
  runqemu myBuild/build/tmp/deploy/images/qemux86/ nographic \
      qemuparams="-m 2048"
  ```
  - after that the all the a TAP network interface is added, and protocol like
  ssh, scp, http work well.

# OpenBMC Hello World in SDK

## Clone and Build a Repo

This lesson uses
[openbmc/phosphor-state-manager](https://github.com/openbmc/phosphor-state-manager)
repo. To keep your repos organized, it's a good idea to keep them all under some
common directory like ~/Code/.

1. Clone the Repository
  ```
  git clone https://github.com/openbmc/phosphor-state-manager.git
  ```

2. Add code to print out a Hello World
  ```
  cd phosphor-state-manager
  vi bmc_state_manager_main.cpp
  ```

  Your diff should look something like this:
  ```
  +#include <iostream>

   int main(int argc, char**)
   {
  @@ -17,6 +18,8 @@ int main(int argc, char**)

       bus.request_name(BMC_BUSNAME);

  +    std::cout<<"Hello World" <<std::endl;
  +
       while (true)
       {
  ```

3. Build the Repository

  This is a meson based repository
  ```
  meson build
  ninja -C build
  ```

## Load the Application Into QEMU

  1. Strip the binary you generated

  OpenBMC is an embedded environment so always best to load on the smallest size
  application/library
  ```
  arm-openbmc-linux-gnueabi-strip phosphor-bmc-state-manager
  ```

  2. Create a safe file system for your application

  Now is time to load your Hello World application in to QEMU virtual hardware.
  The OpenBMC overrides the PATH variable to always look in /usr/local/bin/
  first so you could simply scp your application in to /usr/local/bin, run it
  and be done.  That works fine for command line tests, but when you start
  launching your applications via systemd services you will hit problems since
  application paths are hardcoded in the service files. Let's start our journey
  doing what the professionals do and create an overlay file system.  Overlays
  will save you time and grief.  No more renaming and recovering original files,
  no more forgetting which files you messed with during debug, and of course, no
  more bricking the system.  Log in to the QEMU instance and run these commands.
  ```
  mkdir -p /tmp/persist/usr
  mkdir -p /tmp/persist/work/usr
  mount -t overlay -o lowerdir=/usr,upperdir=/tmp/persist/usr,workdir=/tmp/persist/work/usr overlay /usr
  ```

  3. scp this binary onto your QEMU instance

  If you used the default ports when starting QEMU then here is the scp command
  to run from your phosphor-state-manager directory. If you chose your own port
  then substitute that here for the 2222.
  ```
  scp -P 2222 phosphor-bmc-state-manager root@127.0.0.1:/usr/bin/
  ```

## Run the Application in QEMU

  1. Stop the BMC state manager service
  ```
  systemctl stop xyz.openbmc_project.State.BMC.service
  ```

  2. Run the application in your QEMU session:
  ```
  phosphor-bmc-state-manager
  ```

  You'll see your "Hello World" message displayed.  Ctrl^C to end that
  application. In general, this is not how you will test new applications.
  Instead, you'll be using systemd services.

  3. Start application via systemd service

  OpenBMC uses systemd to manage its applications. There will be later tutorials
  on this, but for now just run the following to restart the BMC state service
  and have it pick up your new application:
  ```
  systemctl restart xyz.openbmc_project.State.BMC.service
  ```

  Since systemd started your service, the
  "Hello World" will not be output to the console, but it will be in the
  journal. Later tutorials will discuss the journal but for now just run:
  ```
  journalctl | tail
  ```

  You should see something like this in one of the journal
  entries:
  ```
  <date> romulus phosphor-bmc-state-manager[1089]: Hello World
  ```

That's it! You customized an existing BMC application, built it using the SDK,
and ran it within QEMU!
