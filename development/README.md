# OpenBMC Developer Documentation

This directory is focused on providing OpenBMC developers with the instructions
they need to get up and going with OpenBMC development. This can be reviewed in
any order you like, but the recommended flow can be found below.

1. [Development Environment Setup](dev-environment.md)

   Start here. This shows how to setup an OpenBMC development environment using
   its bitbake build process and how to start the software emulator, QEMU.

2. [Hello World](devtool-hello-world.md)

   This shows how to use the yocto tool, devtool, to extract an OpenBMC source
   repository, build, and test your changes within QEMU.

3. [Web UI Development](web-ui.md)

   This shows how to modify the phosphor-webui web application and test your
   changes within QEMU.

4. [Code Reviews Using Gerrit](gerrit-setup.md)

   This shows how to setup your environment to utilize Gerrit for submitting
   your changes for code review.
