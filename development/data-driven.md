# Data Driven Applications in OpenBMC

**Purpose:** Describe techniques for finding data in OpenBMC.

**Intended Audience:** System integrators, system engineers, developers porting
systems to OpenBMC.

**Prerequisites:** None

If you already know the techniques for finding data in OpenBMC, skip ahead to
[the list of OpenBMC applications with
data](#list-of-openbmc-applications-with-data).

## Techniques for finding data
### Runtime configuration files in bitbake metadata
Some applications consume data in the form of runtime configuration files.
Runtime means the application loads the configuration data when it is invoked,
on the target (on the BMC).  Configuration files are `/etc/<packagename>.conf`
or in `/etc/<packagename>/` in the target's root filesystem.  The format of a
configuration file is application specific.

To identify the location of a configuration file in bitbake metadata, first
identify which bitbake layer will host or already hosts the support for your
target.  Often this is `meta-<vendor>/meta-<target>` or just `meta-<vendor>`.
The former is used in one to one layer to target configurations, the latter in
one to many layer to target configurations.

#### Anatomy of a path in a bitbake layer
Most bitbake layers adhere to a de-facto standard directory hierarchy:

`recipes-<group>/<subgroup>/<package>/<target-group>`

The `<group>` is almost always phosphor, which refers to programs that provide
BMC specific functions and the `<subgroup>` refers to even more specific
functions like fan control or inventory management.  The `<package>` is the
program that makes use of the configuration file.  The `<target-group>` may just
be the target or it may refer to a group of targets if multiple targets share
the same configuration.

Configuration files may be found in the `<target-group>` directory or in the
`<package>` directory.  For example the location of the host processor console
configuration for the [Mihawk][2] server would be:

[`recipes-phosphor/console/obmc-console/mihawk/server.ttyVUART0.conf`][1]

### Build time configuration files in bitbake metadata
Other applications consume data in the form of build time configuration files.
Build time means the data is embedded directly into the application in some
fashion.  Typically build time configuration files in OpenBMC are written in
YAML, but the schemas are application specific.

The location of build time configuration files can be found in the same manner
as runtime configuration files.

### Application specific hardware databases
Another class of applications maintain databases of supported hardware.
Hardware databases are often just a hierarchy of JSON files in
`/usr/share/<packagename>/` in the target root filesystem.  Different
applications have different schema for their databases.  Applications with
hardware databases have some level of support for discovering out what kinds of
hardware exist and then load the correct data from the database.

Like other data, hardware databases can be found in the bitbake metadata.  Or
hardware databases are often found in the source repository for the application
itself.

## List of OpenBMC applications with data
When adding new applications to the list, be sure to indicate whether the
application uses [runtime](#runtime-configuration-files-in-bitbake-metadata) or
[build time](#build-time-configuration-files-in-bitbake-metadata) configuration
files, or a [hardware database](#application-specific-hardware-databases).  Also
list briefly what the application data describes and provide links to
application specific documentation that outline the data schema.

### obmc-console
obmc-console uses runtime configuration files in bitbake metadata.  The
obmc-console configuration describes the physical console (UART or VUART)
parameters.  Additional documentation for the obmc-console configuration can be
found [here][3].

### phosphor-regulators
phosphor-regulators uses a hardware database.  The database describes
information required to configure voltage regulators in servers, or any other
system with a voltage regulator.  Additional information about data in
phosphor-regulators can be found [here][4].

### phosphor-dbus-monitor
phosphor-dbus-monitor uses build time configuration files in bitbake metadata.
The phosphor-dbus-monitor configuration can describe anything, but is typically
used to express power or thermal system policies.  Additional informationi about
data in phosphor-dbus-monitor can be found [here][5].

[1]: https://github.com/openbmc/meta-ibm/tree/master/recipes-phosphor/console/obmc-console/mihawk/server.ttyVUART0.conf
[2]: https://github.com/openbmc/meta-ibm/tree/master/conf/machine/mihawk.conf
[3]: https://github.com/openbmc/obmc-console/tree/master/conf/server.ttyVUART0.conf.in
[4]: https://github.com/openbmc/phosphor-power/tree/master/phosphor-regulators/README.md
[5]: https://github.com/openbmc/phosphor-dbus-monitor/tree/master/src/example/example.yaml
