# OpenBMC Hello World in SDK

**Document Purpose:** Walk through compiling and running an OpenBMC application
in QEMU.

**Prerequisites:** Completed Development Environment Setup [Document](https://github.com/openbmc/docs/blob/master/development/dev-environment.md)

### Clone and Build a Repo

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

  This is an automake based repository so it will have a bootstrap.sh script
  for doing the basic build setups.
  ```
  ./bootstrap.sh
  ./configure ${CONFIGURE_FLAGS}
  make
  ```

### Load the Application Into QEMU

  1. Strip the binary you generated

  OpenBMC is an embedded environment so always best to load on the smallest size
  application/library
  ```
  arm-openbmc-linux-gnueabi-strip phosphor-bmc-state-manager
  ```

  2. Create the directory in your QEMU session
  for you to copy your binary too

  OpenBMC overrides the PATH variable to always look in /usr/local/bin/ first so
  that's where we put patches for testing. From your QEMU session:
  ```
  mkdir -p /usr/local/bin
  ```

  3. scp this binary onto your QEMU instance

  If you used the default ports when starting QEMU then here is the scp command
  to run from your phosphor-state-manager directory. If you chose your own port
  then substitute that here for the 2222.
  ```
  scp -P 2222 phosphor-bmc-state-manager root@127.0.0.1:/usr/local/bin/
  ```

### Run the Application in QEMU

  1. Run the application in your QEMU session:
  ```
  phosphor-bmc-state-manager
  ```

  You'll see your "Hello World" message displayed.  Ctrl^C to end that
  application. In general, this is not how you will test new applications.
  Instead, you'll be using systemd services.

  2. Start application via systemd service

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
