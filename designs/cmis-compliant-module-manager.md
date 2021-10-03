# CMIS-compliant module management  daemon
Author: Jianqiao Liu (jianqiaoliu@google.com)

Primary assignee: Jianqiao Liu (jianqiaoliu@google.com)

Other contributors: Jason Ling (jasonling@google.com)

## Problem Description
The Common Management Interface Specification (CMIS, Latest [Rev
5.0](http://www.qsfp-dd.com/wp-content/uploads/2021/05/CMIS5p0.pdf)) is an open
industry standard for pluggable or on-board modules with host to module
communication based on a Two-Wire Interface (TWI, aka, I2C), such as QSFP Double
Density (QSFP-DD), OSFP, COBO, QSFP, SFP-DD as well as future modules . The CMIS
is developed to allow the host and the module vendor to utilize a common code
base across a variety of form factors. It provides a set of core functionalities
that all modules must implement and a set of optional features whose
implementation is advertised in the module memory map. The below figure is a
typical case when a CMIS-compliant module is deployed in the real system. The
module usually transmits and receives data at 10 ~ 200 GB/s on the high speed
path.

```ascii
                                           ┌─────────────────┐
                                           │ Management chip │
                                           └────────▲────────┘
                                                    │
                                                    │Management interface (I2C)
                                                    │
                                                    │
 ┌──────────────────────┐ High speed data path ┌────▼───┐ Copper / Optical media  ┌─────────┐
 │ Data processing unit ◄──────────────────────► Module ◄─────────────────────────► Network │
 └──────────────────────┘                      └────────┘                         └─────────┘

```

This design doc is to propose a management daemon to detect the CMIS-compliant
modules in the system, enable their features and facilitate the management work
(e.g. telemetry reporting and firmware upgrade). The daemon interacts with
modules through two interfaces: the I2C interface is for the data communication
and the GPIO pins are for the module presence detection, hard reset and low/high
power selection. A typical system diagram looks like below:

```ascii
 ┌──────────────────────────────────────────────────────────────────────┐
 │Host                                                                  │
 │                                                                      │
 │             ┌──────────────────────────────────────┐                 │
 │             │   Link Quality Monitoring Service    │                 │
 │             └──────────────────▲───────────────────┘                 │
 │                                │                                     │
 └────────────────────────────────┼─────────────────────────────────────┘
                                  │
 ┌────────────────────────────────┼─────────────────────────────────────┐
 │BMC                             │                                     │
 │             ┌──────────────────▼───────────────────┐                 │
 │             │                BMCWeb                │                 │
 │             └──────────────────▲───────────────────┘                 │
 │                                │                                     │
 │ ┌────────────────────────┐     │     ┌─────────────────────────────┐ │
 │ │  phosphor-pid-control  │     │     │  Diagnosis/Debugging tools  │ │
 │ └────────────────▲───────┘     │     └───────▲─────────────────────┘ │
 │                  │             │             │                       │
 │                  │dbus         │dbus         │dbus                   │
 │                  │             │             │                       │
 │           ┌──────▼─────────────▼─────────────▼──────┐       ┌─────┐  │
 │           │ CMIS-Compliant module management daemon ◄───────► CLI │  │
 │           └──▲─▲─────────▲─▲──────────▲─▲───────────┘       └─────┘  │
 │              │ │         │ │          │ │                            │
 └──────────────┼─┼─────────┼─┼──────────┼─┼────────────────────────────┘
             I2C│ │GPIO  I2C│ │GPIO   I2C│ │GPIO
                │ │         │ │          │ │
           ┌────▼─▼───┐ ┌───▼─▼────┐ ┌───▼─▼────┐
           │ Module 1 │ │ Module 2 │ │ Module 3 │  ***
           └──────────┘ └──────────┘ └──────────┘

```

## Background and Reference
This CMIS-compliant module management daemon follows the **Common Management
Interface Specification**
([Rev3.0](http://www.qsfp-dd.com/wp-content/uploads/2018/09/QSFP-DD-mgmt-rev3p0-final-9-18-18-Default-clean.pdf),
[Rev4.0](http://www.qsfp-dd.com/wp-content/uploads/2019/05/QSFP-DD-CMIS-rev4p0.pdf),
[Rev5.0](http://www.qsfp-dd.com/wp-content/uploads/2021/05/CMIS5p0.pdf)).

### Requirements
1. The daemon should use less than 1% BMC CPU when idle and CMIS-compliant
   modules should be detected with <30ms of latency. It works as the
   source-of-the-truth for the module presence. CMIS modules should be
   dynamically configured using Entity-manager reactor models to identify the
   location of GPIO pins.
2. For a CMIS-compliant module already plugged into the system, we also want to
   differentiate its work states. For example, when a module is under firmware
   upgrade or debug mode, it may not be able to report identification or
   telemetry data. The management daemon should report the module status as
   present but unusable.
3. All CMIS-compliant modules make their static identifier readable through the
   I2C interface. Some active modules (mostly optics) utilize Digital Signal
   Processing technology and complex signal modulation schemes to achieve high
   data rate and longer transmission range. Those modules usually carry a
   micro-controller and are able to dynamically report some sensor readings. The
   proposed daemon should collect both the static identifier and dynamic sensor
   data (if available), and expose them on the D-Bus for other usages.
4. The daemon should be able to conduct firmware upgrades through Command Data
   Block based approach, which is defined in
   [CMIS-4.0](http://www.qsfp-dd.com/wp-content/uploads/2019/05/QSFP-DD-CMIS-rev4p0.pdf)
   (Section 7.2.2) and above, on modules that support this feature.
5. Other system daemons (not upstreamable) may need to configure the module for
   diagnostic purposes, e.g. entering low power mode or disable Tx laser to
   measure signal return loss. There may be scenarios during initial platform
   bring up where full manul control of the module will be helpful for debugging
   purposes. When requested, the management daemon should stop its access to the
   module and export the raw i2c read/write access (or some APIs) to diagnostic
   or debugging tools.

### Other references:
**SFF Module Management Reference Code Tables** ([Rev
4.9](https://www.snia.org/technology-communities/sff/specifications))

## Proposed Design

We propose the mangement daemon to detect the module presence through GPIO pin
interrupts to save CPU usage and reduce latency.

### CMIS module identifier interface
The management daemon should extract a number of identifiers from the module and
report on the D-Bus. Some of those identifiers have already been well-modeled by
existing interfaces, so we'd like to reuse them:

| Field Name      | Default | Type    | Description                                       | Reused interface                                                     |
|-----------------|---------|---------|---------------------------------------------------|----------------------------------------------------------------------|
| Vendor Name     | ""      | string  | The module vendor name.                           | xyz/openbmc_project/Inventory/Decorator/Asset.interface.yaml         |
| Part Number     | ""      | string  | The module part number provided by the vendor.    | xyz/openbmc_project/Inventory/Decorator/Asset.interface.yaml         |
| Serial Number   | ""      | string  | The module vendor serial number.                  | xyz/openbmc_project/Inventory/Decorator/Asset.interface.yaml         |
| Vendor Revision | ""      | string  | The module revision level provided by the vendor. | xyz/openbmc_project/Inventory/Decorator/Revision.interface.yaml      |
| Presence        | false   | boolean | The availability status of the module.            | xyz/openbmc_project/State/Decorator/Availability.interface.yaml      |


For identifiers that haven't been studied before, we would like to propose two
new dbus interfaces like below:

```
/xyz/openbmc_project/Inventory/Item/Module.interface.yaml

description:
  Implement to describe the identifier information for a module connected to the system.

properties:
    - name: Identifier
      default: ""
      type: string
      description: Identifier type of the module.
```

```
/xyz/openbmc_project/Inventory/Decorator/VendorOUI.interface.yaml

description:
  Implement to describe the vendor IEEE company ID for an inventory.

properties:
    - name: VendorOUI
      default: 0xffffffff
      type: int
      description:
        A 24-bit number which uniquely identifies the module vendor
        IEEE company ID of a specific inventory item.
```

### Sensor reading report (optics module only)
The proposed management daemon will serve as the module detector and reactor. It
should periodically collect sensor data from the module and publish on the dbus.
In OpenBMC D-Bus producer-consumer module, this daemon should be the producer,
and other daemon like bmcweb and phosphor-pid-control would be the consumer.
There are two levels of sensor data to be exposed:

1. The module-level temperature and supply voltage
2. The lane-specific monitors, such as Tx/Rx power, and lane-specific flags,
   e.g. Tx/Rx Loss of Signal (LOS) and Tx/Rx CDR Loss of Lock (LoL).

`xyz/openbmc_project/Sensor/Value.interface.yaml` will be used to expose those
temperature/voltage/power data.

| Name           | Description                                                                    |
|----------------|--------------------------------------------------------------------------------|
| Temperature    | The measured module temperature in degrees celsius. One instance per module.   |
| Supply Voltage | The measured voltage supplied to the module in volts. One instance per module. |
| Tx Power       | The measured Tx output optical power in watts. One instance per lane.            |
| Rx Power       | The measured Rx input optical power in watts. One instance per lane.             |

These lane-specific flags are mostly boolean variables and can not be
categorized as "sensors" per OpenBMC design, so we propose a new LaneStatus
interface for them:

```
/xyz/openbmc_project/State/Decorator/LanesStatus.interface.yaml

description:
  Implement to describe lane-specific flags/monitors for network devices. One instance per device and each device may carry multiple lanes. The overall number of lanes can be inferred from module Identifier.

properties:
    - name: LanesWithTxLossOfLock
      type: set[uint32]
      description: The array of lanes having the Tx Loss of Lock flag. Lane id starts with 1.
    - name: LanesWithRxLossOfLock
      type: set[uint32]
      description: The array of lanes having the Rx Loss of Lock flag. Lane id starts with 1.
    - name: LanesWithTxLossOfSignal
      type: set[uint32]
      description: The array of lanes having the Tx Loss of Signal flag. Lane id starts with 1.
    - name: LanesWithRxLossOfSignal
      type: set[uint32]
      description: The array of lanes having the Rx Loss of Signal flag. Lane id starts with 1.
```

### Firmware upgrading (optics module only)
The proposed management daemon will implement OpenBMC firmware upgrade methods
(openbmc_project/Software/). Note that
[CMIS-4.0](http://www.qsfp-dd.com/wp-content/uploads/2019/05/QSFP-DD-CMIS-rev4p0.pdf)
defines the Command Data Block (CDB) based firmware upgrade approach (Section
7.2.2 Firmware Upgrade Commands in CDB), but it is completely CMIS specific. The
CMIS/CDB specific control logic should be handled by another repository which
resides outside of OpenBMC.

### Module initialization & Configuration
The CMIS-compliant module is usually plugged in some slots on the electrical
board or tray. Each slot has its own bus id/address, and allows a single module
at a time. Such information should be stored in an BMC entity manager
configuration file like below. Depending on system design or logistics
considerations, one electrical board may connect with modules produced by
different vendors, and even with different media types (copper/optics).

```
{
    "Exposes": [
        {
            "Address": "0x50",
            "Bus": "21",
            "Name": "osfp1",
            "Type": "CMISModule",
            "PresenceGpioLine": "osfp1_prsnt_l",
            "PresencePolarity": "active_low",
            "ResetGpioLine": "osfp1_reset_l",
            "ResetPolarity": "active_low",
            "LowPowerGpioLine": "osfp1_low_power_l",
            "LowPowerPolarity": "active_low"
        }
    ],
    "Name": "accelerator_cmis_modules",
    "Probe": "xyz.openbmc_project.FruDevice({'BOARD_PRODUCT_NAME': 'Accelerator Board'})",
    "Type": "Board",
}
```

When a module is plugged in the slot, the management daemon will take it out of
reset through either direct GPIO pins or other configurables (IO expander or
controller), and load corresponding entity-manager configuration according to
the module’s type and identification data (vendor name, part number, etc).

## Alternatives considered
We may also implement this as a kernel driver. There are pros and cons:
**Pros:** we can utilize existing kernel subsystems, hwmon, iio, etc, and avoid
many edge cases that kernel subsystems already handled.
**Cons:**
1. in-kernel firmware upgrade is a rough edge. See our previous efforts:
   https://lkml.org/lkml/2020/8/7/565, https://lkml.org/lkml/2020/6/24/830
2. And hwmon doesn't have buckets that fit some fields, such as firmware
   revision, lane based loss-of-lock/signal and module descriptor, natively and
   nicely into.

Both the user-space daemon and the kernel driver can serve this proposal's
purpose with some workarounds, but we also realize that neither of them is
perfect. After considering the ease of debugging and unit testing, we decided to
implement it as a user space daemon.

## Testing
Testing can be done by monitoring the module identification and telemetry data
from the dbus and comparing with other sensors reading.