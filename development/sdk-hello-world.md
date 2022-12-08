# OpenBMC Hello World in SDK

**Document Purpose:** Walk through compiling and running an OpenBMC application
in QEMU.

**Prerequisites:** Completed Development Environment Setup
[Document](https://github.com/openbmc/docs/blob/master/development/dev-environment.md)

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
The OpenBMC overrides the PATH variable to always look in /usr/local/bin/ first
so you could simply scp your application in to /usr/local/bin, run it and be
done. That works fine for command line tests, but when you start launching your
applications via systemd services you will hit problems since application paths
are hardcoded in the service files. Let's start our journey doing what the
professionals do and create an overlay file system. Overlays will save you time
and grief. No more renaming and recovering original files, no more forgetting
which files you messed with during debug, and of course, no more bricking the
system. Log in to the QEMU instance and run these commands.

```
mkdir -p /tmp/persist/usr
mkdir -p /tmp/persist/work/usr
mount -t overlay -o lowerdir=/usr,upperdir=/tmp/persist/usr,workdir=/tmp/persist/work/usr overlay /usr
```

3. scp this binary onto your QEMU instance

If you used the default ports when starting QEMU then here is the scp command to
run from your phosphor-state-manager directory. If you chose your own port then
substitute that here for the 2222.

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

You'll see your "Hello World" message displayed. Ctrl^C to end that application.
In general, this is not how you will test new applications. Instead, you'll be
using systemd services.

3. Start application via systemd service

OpenBMC uses systemd to manage its applications. There will be later tutorials
on this, but for now just run the following to restart the BMC state service and
have it pick up your new application:

```
systemctl restart xyz.openbmc_project.State.BMC.service
```

Since systemd started your service, the "Hello World" will not be output to the
console, but it will be in the journal. Later tutorials will discuss the journal
but for now just run:

```
journalctl | tail
```

You should see something like this in one of the journal entries:

```
<date> romulus phosphor-bmc-state-manager[1089]: Hello World
```

That's it! You customized an existing BMC application, built it using the SDK,
and ran it within QEMU!
