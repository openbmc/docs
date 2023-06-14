# OpenBMC Hello World using devtool

**Document Purpose:** Walk through compiling and running an OpenBMC application
in QEMU.

**Prerequisites:** Completed Development Environment Setup
[Document](https://github.com/openbmc/docs/blob/master/development/dev-environment.md)

## Clone and Build a Repo

devtool is a command line tool provided by the yocto distribution that allows
you to extract a targeted repositories source code into your local bitbake
environment and modify it.

Today we'll be extracting the
[phosphor-state-manager](https://github.com/openbmc/phosphor-state-manager.git)
repository using devtool and modifying it.

This assumes you have followed the first tutorial and are in the bitbake
environment, right after doing the ". setup" command.

1. Use devtool to extract source code

   ```
   devtool modify phosphor-state-manager
   ```

   The above command will output the directory the source code is contained in.

2. Modify a source file and add a cout

   ```
   vi workspace/sources/phosphor-state-manager/bmc_state_manager_main.cpp
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

3. Rebuild the flash image which will now include your change

   This will be a much faster build as bitbake will utilize all of the cache
   from your previous build, only building what is new.

   ```
   bitbake obmc-phosphor-image
   ```

   Follow the steps in the first tutorial to load your new image into a QEMU
   session and boot it up.

4. Confirm your "Hello World" made it into the new image

   After you login to your QEMU session, verify the message is in the journal

   ```
   journalctl | grep "Hello World"
   ```

   You should see something like this:

   ```
   <date> romulus phosphor-bmc-state-manager[1089]: Hello World
   ```

You made a change, rebuilt the flash image to contain that change, and then
booted that image up and verified the change made it in!

## Loading an Application Directly Into a Running QEMU

In this section we're going to modify the same source file, but instead of fully
re-generating the flash image and booting QEMU again, we're going to just build
the required binary and copy it into the running QEMU session and launch it.

1. Modify your hello world

   ```
   vi workspace/sources/phosphor-state-manager/bmc_state_manager_main.cpp
   ```

   Change your cout to "Hello World Again"

2. Bitbake only the phosphor-state-manager repository

   Within bitbake, you can build only the repository (and it's dependencies)
   that you are interested in. In this case, we'll just rebuild the
   phosphor-state-manager repo to pick up your new hello world change.

   ```
   bitbake phosphor-state-manager
   ```

   Your new binary will be located at the following location relative to your
   bitbake directory:
   `./tmp/work/arm1176jzs-openbmc-linux-gnueabi/phosphor-state-manager/1.0+git999-r1/package/usr/bin/phosphor-bmc-state-manager`

3. Create a safe file system for your application

   Now is time to load your Hello World application in to QEMU virtual hardware.
   The OpenBMC overrides the PATH variable to always look in /usr/local/bin/
   first so you could simply scp your application in to /usr/local/bin, run it
   and be done. That works fine for command line tests, but when you start
   launching your applications via systemd services you will hit problems since
   application paths are hardcoded in the service files. Let's start our journey
   doing what the professionals do and create an overlay file system. Overlays
   will save you time and grief. No more renaming and recovering original files,
   no more forgetting which files you messed with during debug, and of course,
   no more bricking the system. Log in to the QEMU instance and run these
   commands.

   ```
   mkdir -p /tmp/persist/usr
   mkdir -p /tmp/persist/work/usr
   mount -t overlay -o lowerdir=/usr,upperdir=/tmp/persist/usr,workdir=/tmp/persist/work/usr overlay /usr
   ```

4. scp this binary onto your QEMU instance

   If you used the default ports when starting QEMU then here is the scp command
   to run from your phosphor-state-manager directory. If you chose your own port
   then substitute that here for the 2222.

   ```
   scp -P 2222 ./tmp/work/arm1176jzs-openbmc-linux-gnueabi/phosphor-state-manager/1.0+git999-r1/package/usr/bin/phosphor-bmc-state-manager root@127.0.0.1:/usr/bin/
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

   You'll see your "Hello World Again" message displayed. Ctrl^C to end that
   application. In general, this is not how you will test new applications.
   Instead, you'll be using systemd services.

3. Start application via systemd service

   OpenBMC uses systemd to manage its applications. There will be later
   tutorials on this, but for now just run the following to restart the BMC
   state service and have it pick up your new application:

   ```
   systemctl restart xyz.openbmc_project.State.BMC.service
   ```

   Since systemd started your service, the "Hello World Again" will not be
   output to the console, but it will be in the journal. Later tutorials will
   discuss the journal but for now just run:

   ```
   journalctl | tail
   ```

   You should see something like this in one of the journal entries:

   ```
   <date> romulus phosphor-bmc-state-manager[1089]: Hello World Again
   ```

That's it! You customized an existing BMC application, built it using devtool,
and ran it within QEMU by both regenerating the entire flash image and patching
it on via scp!

## Advanced Workflow

Once you've got a good handle on the above, there are some more advanced tools
and wrappers you can utilize to optimize this flow. See this
[link](https://amboar.github.io/notes/2022/01/13/openbmc-development-workflow.html)
for more information.
