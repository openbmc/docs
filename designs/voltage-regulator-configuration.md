# Voltage Regulator Configuration

Author: Shawn McCarney (Discord ShawnMcCarney)

Other contributors: Derek Howard (Discord derekh55), Matt Spinler (Discord
mspinler)

Created: 2019-07-13

## Problem Description

[Voltage regulators][1] have many configurable properties such as output
voltage, over-current limit, and pgood thresholds. The configuration is often
dependent on the system type and rail type. Regulators have a hardware default
configuration that is defined by hardware engineers early in system design.
However, this default configuration sometimes must be modified by firmware. A
new application is needed to configure regulators. It should be data-driven to
support a variety of regulator types and to avoid hard-coded logic.

## Background and References

### Regulator Configuration Data ("Config File")

Hardware engineers must specify many low-level configuration values for a
regulator. Some simple examples include output voltage, over-current limit, and
pgood thresholds. These values often vary depending on the system type and rail
type.

Depending on the regulator type, the hardware engineer may enter the
configuration values into a tool that produces a "config file".

The regulator configuration information is sent to manufacturing and stored in
non-volatile memory on the regulator. This provides the hardware/power-on
default configuration.

The default configuration values often need to be changed later. This can be due
to factors such as the following:

- New information found during hardware testing. For example, downstream
  hardware requires a higher voltage or is drawing more current than expected.
- Hardware workarounds. Problems in the regulator hardware or related hardware
  sometimes require new configuration values to work around the issue. The
  problem may be resolved in newer versions of the hardware, but firmware still
  must support the older hardware. For this reason, hardware workarounds are
  sometimes conditional, applied only to regulators with a certain version
  register value or [Vital Product Data (VPD)][2] keyword value.
- Improve manufacturing yields. Sometimes regulator configuration values must be
  changed to allow more hardware to pass manufacturing tests. For example, the
  voltage may need to be increased to overcome minor manufacturing defects.

### OpenBMC Regulator Configuration Scripts

Regulator configuration changes are performed on some OpenBMC systems using
shell scripts. For example, the following scripts configure regulators on
Witherspoon systems:

- [vrm-control.sh][3]
- [power-workarounds.sh][4]

### Hwmon and IIO Device Driver Frameworks

The Linux [Hardware Monitoring Kernel API (hwmon)][5] and [Industrial I/O
Subsystem (IIO)][6] provide device driver frameworks for monitoring hardware and
making sensor values available to applications. They do not provide an interface
for performing low-level regulator configuration.

### Voltage and Current Regulator Device Framework

The Linux [Voltage and Current Regulator API][7] provides a device driver
framework for voltage and current regulators. It provides a mechanism for the
device drivers of "consumer" devices to request a voltage or current level from
the regulator. It does not provide an interface for performing low-level
regulator configuration.

## Requirements

- Provide ability to modify configuration of any voltage regulator with a PMBus
  or I2C connection to the BMC.
- Apply the configuration changes early in the boot before the regulators are
  enabled.
- If an error occurs trying to configure a regulator, log the error and continue
  with the next regulator.
- Read configuration changes from a data file that is read at run-time.
- Provide a method for testing new configuration changes by copying a new data
  file to the BMC.

### Non-Requirements

- Enable/disable voltage regulators and monitor their pgood signals.
  - This is handled by the power sequencer chip and related firmware.
- Modify regulator configuration after regulator has been enabled.
  - Modifying regulator configuration while the system is running is often done
    by the host using a different bus connection.
- Validate that the correct number and types of regulators are present in the
  system.
- Concurrent maintenance or hot-plugging of regulators, where a regulator is
  removed/added/replaced while the system is running.

## Proposed Design

### Application

A new application named `phosphor-regulators` will be created to configure
voltage regulators. The application will be located in the proposed new
`phosphor-power` repository.

_Statement of direction: This application will provide other regulator-based
functionality in the future, such as detecting redundant phase faults._

### Application Data File

The application will read a JSON file at runtime that defines the configuration
changes for all regulators. The JSON file will be specific to a system type, so
it will be stored in the GitHub repository for the appropriate build layer (such
as meta-ibm).

JSON file location on the BMC:

- Standard directory: `/usr/share/phosphor-regulators` (read-only)
- Test directory: `/etc/phosphor-regulators` (writable)

A new version of the JSON file can be tested by copying it to the test
directory. The application will search both directories, and the file in the
test directory will override the file in the standard directory. If the
application receives a SIGHUP signal, it will re-read the JSON file.

A document will be provided that describes the objects and properties that can
appear in the JSON file. A validation tool will also be provided to verify the
syntax and contents of the JSON file.

_Statement of direction: This JSON file will be used in the future to define
other regulator operations, such as how to detect a redundant phase fault._

### D-Bus Interfaces

The following new D-Bus interface will be created:

- Name: `xyz.openbmc_project.Regulator.Collection.Configure`
- Description: Configures all voltage regulators.
- Properties: None
- Methods: Configure()

_Statement of direction: New interfaces that apply to all regulators would use
the same xyz.openbmc_project.Regulator.Collection namespace. New interfaces that
apply to a single regulator would use the xyz.openbmc_project.Regulator
namespace._

### D-Bus Paths

The new `xyz.openbmc_project.Regulator.Collection.Configure` interface will be
implemented on the object path `/xyz/openbmc_project/regulators`.

_Statement of direction: New interfaces that apply to all regulators would be
implemented on the same object path. Individual regulators would be represented
by the object path `/xyz/openbmc_project/regulators/<regulator_name>`, and
interfaces that apply to a single regulator would be implemented on that path._

### Systemd

The application will be started using systemd when the BMC is at standby. The
service file will be named `xyz.openbmc_project.Regulators.service`.

During the boot when the chassis is being powered on, the Configure() method on
the new `xyz.openbmc_project.Regulator.Collection.Configure` interface will be
called. This must be done prior to the systemd target that enables the
regulators (powers them on).

### Regulator Device Interface

The application will communicate with the voltage regulators directly using the
[i2c-dev][8] userspace I2C interface. Direct communication will be used rather
than device drivers because the current Linux device driver frameworks do not
provide the necessary functionality. See the
[Alternatives Considered](#alternatives-considered) section for more
information.

Voltage regulators that are configured using this application will not be bound
to device drivers in the Linux device tree. When the system is powered on, the
application will obtain the regulator sensor values and store them on D-Bus in
the same format as `phosphor-hwmon`.

_Statement of direction: If a driver framework is developed in the future that
supports low-level regulator configuration, then this application will be
enhanced to utilize those drivers. The goal is for the application to support a
flexible device interface, where drivers are used if possible and i2c-dev is
used if necessary._

## Alternatives Considered

### Using Standard Linux Device Drivers to Configure Regulators

Ideally one of the following device driver frameworks could be used rather than
i2c-dev to configure the regulators:

- [Hardware Monitoring Kernel API][5]
- [Industrial I/O Subsystem][6]
- [Voltage and Current Regulator API][7]

Unfortunately none of these provide the required functionality:

- Ability to perform system-specific and rail-specific, low-level configuration.
- Ability to perform dynamic configuration based on regulator version register
  values or [VPD][2] values.
- Ability to test new configuration values iteratively and quickly without
  needing to modify a device driver or rebuild the kernel.

A meeting was held with Joel Stanley and Andrew Jeffery to discuss this issue.
They believe long term a new driver framework could be designed that would
provide this functionality and would be acceptable to the Linux upstream
community. If this occurs, this application will be modified to utilize the new
driver framework rather than performing direct I2C communication from userspace.

### Using Shell Scripts to Configure Regulators

Currently shell scripts are used to configure regulators on some OpenBMC
systems, such as Witherspoon. During boot, the shell scripts unbind device
drivers, perform `i2cset` commands to configure regulators, and then re-bind
device drivers.

Using shell scripts has the following disadvantages:

- Typically no error checking is performed to verify the `i2cset` command
  worked. If error checking were added based on return code, it would be
  difficult to log an appropriate error with all the necessary information.
- On a large system with many regulators and register updates, a shell script
  would run more slowly than an application and negatively impact boot speed.
  Each `i2cset` command requires starting a separate child process.
- Scripts are usually hard-coded to one system type and do not provide a common,
  data-driven solution.

## Impacts

- No major impacts are expected to existing APIs, security, documentation,
  performance, or upgradability.
- The new D-Bus interface is documented in the
  [Proposed Design](#proposed-design) section.
- The application should be able to configure regulators using i2c-dev as fast
  or faster than equivalent shell scripts using `i2cset`.
- The regulator sensor values will be stored on D-Bus in a way that is
  consistent with `phosphor-hwmon`.

## Testing

- Automated test cases will be written to test as much of the application code
  as possible.
- A mock device interface will be used to test that the correct I2C reads and
  writes are occurring without accessing real hardware.
- End-to-end boot testing will be performed to ensure that the correct I2C
  reads/writes occurred, that the system boots, and that no errors occur.
- CI tests that boot a system will indirectly test this application.

[1]: https://en.wikipedia.org/wiki/Voltage_regulator_module
[2]: https://en.wikipedia.org/wiki/Vital_Product_Data
[3]:
  https://github.com/openbmc/meta-ibm/blob/master/meta-witherspoon/recipes-phosphor/chassis/vrm-control/vrm-control.sh
[4]:
  https://github.com/openbmc/meta-ibm/blob/master/meta-witherspoon/recipes-phosphor/chassis/power-workarounds/witherspoon/power-workarounds.sh
[5]: https://www.kernel.org/doc/html/latest/hwmon/hwmon-kernel-api.html
[6]: https://www.kernel.org/doc/html/latest/driver-api/iio/index.html
[7]: https://www.kernel.org/doc/html/latest/driver-api/regulator.html
[8]: https://www.kernel.org/doc/Documentation/i2c/dev-interface
