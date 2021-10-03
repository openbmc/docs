# CMIS-compliant module management  daemon
Author: Jianqiao Liu (jianqiaoliu@google.com)

Primary assignee: Jianqiao Liu (jianqiaoliu@google.com)

Other contributors: Jason Ling (jasonling@google.com)

## Problem Description
The Common Management Interface Specification (CMIS, Latest [Rev 5.0](http://www.qsfp-dd.com/wp-content/uploads/2021/05/CMIS5p0.pdf)) is an open
industry standard for pluggable or on-board modules with host to module
communication based on a Two-Wire Interface (TWI, aka, I2C), such as QSFP Double
Density (QSFP-DD), OSFP, COBO, QSFP, SFP-DD as well as future modules . The CMIS
is developed to allow the host and the module vendor to utilize a common code
base across a variety of form factors. It provides a set of core functionalities
that all modules must implement and a set of optional features whose
implementation is advertised in the module memory map. The below figure is a
typical case that CMIS-compliant module is deployed in the real system. The
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
(e.g. telemetry reporting and firmware upgrade). The deamon interact with
modules through two interfaces: the TWI/I2C interface is for the data
communicaton and the GPIO pins are for the module presence detection, hard reset
and low/high power selection. A typical system diagram looks like below:

```ascii
 ┌────────────────────────────────────────────────────────────────┐
 │Host                                                            │
 │                                                                │
 │             ┌──────────────────────────────────────┐           │
 │             │   Link Quality Monitoring Service    │           │
 │             └──────────────────▲───────────────────┘           │
 │                                │                               │
 └────────────────────────────────┼───────────────────────────────┘
                                  │
 ┌────────────────────────────────┼───────────────────────────────┐
 │BMC                             │                               │
 │             ┌──────────────────▼───────────────────┐           │
 │             │                BMCWeb                │           │
 │             └──────────────────▲───────────────────┘           │
 │                                │                               │
 │ ┌────────────────────────┐     │     ┌───────────────────────┐ │
 │ │  phosphor-pid-control  │     │     │     Debugging CLI     │ │
 │ └────────────────▲───────┘     │     └───────▲───────────────┘ │
 │                  │             │             │                 │
 │                  │dbus         │dbus         │dbus             │
 │                  │             │             │                 │
 │           ┌──────▼─────────────▼─────────────▼──────┐          │
 │           │ CMIS-Compliant module management daemon │          │
 │           └──▲─▲─────────▲─▲──────────▲─▲───────────┘          │
 │              │ │         │ │          │ │                      │
 └──────────────┼─┼─────────┼─┼──────────┼─┼──────────────────────┘
             I2C│ │GPIO  I2C│ │GPIO   I2C│ │GPIO
                │ │         │ │          │ │
           ┌────▼─▼───┐ ┌───▼─▼────┐ ┌───▼─▼────┐
           │ Module 1 │ │ Module 2 │ │ Module 3 │  ***
           └──────────┘ └──────────┘ └──────────┘

```

## Background and Reference
This CMIS-compliant module management daemon follows the Common Management
Interface Specification ([Rev 3.0](http://www.qsfp-dd.com/wp-content/uploads/2018/09/QSFP-DD-mgmt-rev3p0-final-9-18-18-Default-clean.pdf), [Rev 4.0](http://www.qsfp-dd.com/wp-content/uploads/2019/05/QSFP-DD-CMIS-rev4p0.pdf), [Rev 5.0](http://www.qsfp-dd.com/wp-content/uploads/2021/05/CMIS5p0.pdf)).

### Requirements
1. The daemon should detect the CMIS-compliant module through GPIO pin
   interrupts. It works as the source-of-the-truth for the module presence. CMIS
   module should be dynamically configured using Entity-manager reactor models
   to identify the location of GPIO pins.
2. For a CMIS-compliant module already plugged into the system, we also want to
   differentiate its work states. For example, when a module is under firmware
   upgrade or debug mode, it may not be able to report identification or
   telemetry data. The management daemon should report the module status as
   present but unusable.
3. All CMIS-compliant modules should make their static identifier readable
   through the TWI. Some active modules (mostly optics) utilize Digital Signal
   Processing technology and complex signal modulation schemes to achieve high
   data rate and longer transmission range. Those modules usually carry a
   micro-controller and are able to dynamically report some sensor readings. The
   proposed daemon should collect both the static identifier and dynamic sensor
   data (if available), and expose them on the D-Bus for other usages.
4. The daemon should be able conduct firmware upgrade through Command Data Block
   based approach, which is defined in [CMIS-4.0](http://www.qsfp-dd.com/wp-content/uploads/2019/05/QSFP-DD-CMIS-rev4p0.pdf) (Section 7.2.2) and above,
   on modules that support this feature.
5. There are some chances in development phase that huaman want to take over the
   full control of the module for debugging purpose. When requested, the
   management daemon should stop its access to the module and export the raw i2c
   read/write access to the debugging CLI, which is vendor specific and not
   upstreamed.

### Other references:
SFF Module Management Reference Code Tables ([Rev 4.9](https://www.snia.org/technology-communities/sff/specifications))

## Proposed Design

### CMIS module identifier interface
The management daemon should extract a number of identifers from the module and
report on the D-Bus. Some of those identifers have already been well-modelled by
existing interfaces, so we'd like to reuse them:

| Field Name      | Default | Type    | Description                                       | Reused interface                                                     |
|-----------------|---------|---------|---------------------------------------------------|----------------------------------------------------------------------|
| Vendor Name     | ""      | string  | The module vendor name.                           | xyz/openbmc_project/Inventory/Decorator/Asset.interface.yaml         |
| Part Number     | ""      | string  | The module part number provided by the vendor.    | xyz/openbmc_project/Inventory/Decorator/Asset.interface.yaml         |
| Serial Number   | ""      | string  | The module vendor serial number.                  | xyz/openbmc_project/Inventory/Decorator/Asset.interface.yaml         |
| Vendor Revision | ""      | string  | The module revision level provided by the vendor. | xyz/openbmc_project/Inventory/Decorator/Revision.interface.yaml      |
| Presence        | false   | boolean | The availabilty status of the module.             | xyz/openbmc_project/State/Decorator/Availability.interface.yaml      |

For identifers that haven't been studied before, we would like to group them
into a new dbus interface
`xyz/openbmc_project/Inventory/Item/CmisModule.interface.yaml` like this:

```
/xyz/openbmc_project/Inventory/Item/CmisModule/Module.interface.yaml

description:
  Implement to describe CMIS-compliant module specific identifer information.

properties:
    - name: Identifier
      default: ""
      type: enum[self.ModuleIdentifierType]
      description: Identifier type of the module (defined by SFF-8024 Table 4-1).
    - name: VendorOUI
      default: ""
      type: string
      description: The module vendor IEEE company ID.

enumerations:
    - name: ModuleIdentifierType
      description: Description of Module, with memory map type if not unique.
      values:
        - name: Unknown
          description: Type unknow or unspecified
        - name: GBIC
          description: Giga Bit Interface Converter
        - name: ModuleSslderedToMotherboard
          description: Module/connector soldered to motherboard (using SFF-8472)
        - name: SFP
          description: SFP(Small Form Factor Pluggable)/SFP+/SFP28 and later
        - name: XBI
          description: 300 pin XBI
        - name: XENPAK
          description: 10Gbit Ethernet transceiver Package
        - name: XFP
          description: 10G Form Factor Pluggable
        - name: XFF
          description: (Obsolete)
        - name: XFP-E
          description: (Obsolete)
        - name: XPAK
          description: 10G Form Factor pluggable transceiver package
        - name: X2
          description: 10G Form Factor pluggable
        - name: DWDM-SFP
          description: DWDM-SFP/SFP+ (not using SFF-8472)
        - name: QSFP
          description: QSFP (INF-8438)
        - name: QSFP+withSFF-8636
          description: QSFP+ or later with SFF-8636 or SFF-8436 management interface (SFF-8436, SFF-8635, SFF-8665, SFF-8685 et al.)
        - name: CXP
          description: CXP (100G 12 lane Pluggable Module) or later
        - name: ShieldedMiniMultilaneHD4X
          description: Shielded Mini Multilane HD 4X
        - name: ShieldedMiniMultilaneHD8X
          description: Shielded Mini Multilane HD 8X
        - name: QSFP28withSFF-8636
          description: QSFP28 or later with SFF-8636 management interface (SFF-8665 et al.)
        - name: CXP2
          description: CXP2 (100G 12 lane Pluggable Module 2, aka CXP28) or later
        - name: CDFPStyle1Or2
          description: CDFP (Style 1/Style2)
        - name: ShieldedMiniMultilaneHD4XFanoutCable
          description: Shielded Mini Multilane HD 4X Fanout Cable
        - name: ShieldedMiniMultilaneHD8XFanoutCable
          description: Shielded Mini Multilane HD 8X Fanout Cable
        - name: CDFPStyle3
          description: CDFP (Style 3)
        - name: microQSFP
          description: microQSFP
        - name: QSFP-DD
          description: QSFP-DD Double Density 8X Pluggable Transceiver (INF-8628)
        - name: OSFP8X
          description: OSFP 8X Pluggable Transceiver
        - name: SFP-DD
          description: SFP-DD Double Density 2X Pluggable Transceiver
        - name: DSFP
          description: DSFP Dual Small Form Factor Pluggable Transceiver
        - name: x4MiniLink
          description: x4 MiniLink/OcuLink
        - name: x8MiniLink
          description: x8 MiniLink
        - name: QSFP+withCMIS
          description: QSFP+ or later with Common Management Interface Specification (CMIS)
        - name: Reserved
          description: Reserved for future use.
        - name: Vendor specific
          description: Vendor specific connector.

```

### Sensor reading report (optics module only)
The propsed management daemon will serve as the module detector and reactor. It
should periodically collect sensor data from the module and publish on the dbus.
In OpenBMC D-Bus producer-consumer module, this daemon should be the producer,
and other daemon like bmcweb and phosphor-pid-control would be the consumer.
There are two levels of sensor data to be exposed:

1. The module-level temperature and supply voltage
2. The lane-specific monitors, such as Tx/Rx power, and lane-specific flags,
   e.g. Tx/Rx Loss of Signal (LOS) and Tx/Rx CDR Loss of Lock (LoL).

We'd like to reuse the existing
`xyz/openbmc_project/Sensor/Value.interface.yaml` to expose those
temperature/voltage/power data.

| Name           | Type   | Description                                                                    |
|----------------|--------|--------------------------------------------------------------------------------|
| Temperature    | DOUBLE | The measured module temperature in degrees celsius. One instance per module.   |
| Supply Voltage | DOUBLE | The measured voltage supplied to the module in volts. One instance per module. |
| Tx Power       | DOUBLE | The measured Tx output optical power in dBm. One instance per lane.            |
| Rx Power       | DOUBLE | The measured Rx input optical power in dBm. One instance per lane.             |

But those lane-specific flags are mostly boolean variables and can not be
categorized as "sensors" per OpenBMC design, so we propose a new LaneStatus
interface for them:

```
/xyz/openbmc_project/Inventory/Item/CmisModule/LaneStatus.interface.yaml

description:
  Implement to describe lane-specific flags/monitors per CMIS.

properties:
    - name: TxLossOfLock
      type: boolean
      default: false
      description: Lane Tx Loss of Lock flag.
    - name: RxLossOfLock
      type: boolean
      default: false
      description: Lane Rx Loss of Lock flag.
    - name: TxLossOfSignal
      type: boolean
      default: false
      description: Lane Tx Loss of Signal flag.
    - name: RxLossOfSignal
      type: boolean
      default: false
      description: Lane Rx Loss of Signal flag.
```

### Firmware upgrading (optics module only)
[CMIS-4.0](http://www.qsfp-dd.com/wp-content/uploads/2019/05/QSFP-DD-CMIS-rev4p0.pdf) has already defined the Command Data Block based firmware upgrade approach
(Section 7.2.2 Firmware Upgrade Commands in CDB). This management daemon follows
this public specification and accepts a series of firmware images as the input.

### Module initialization & Configuration
The CMIS-compliant module is usually plugged in some slots on the electrical
board or tray. Each slot has its own bus id/address, and allows a single module
at a time. Depending on system design or logistics considerations, one
electrical board may connect with modules produced by different vendors, and
even with different media types (copper/optics).

When a module is plugged in the slot, the management daemon will take it out of
reset through either direct GPIO pins or other configurables (IO expander or
controller), and load corresponding configuration according to the module’s type
and identification data (vendor name, part number, etc).

## Alternatives considered
We may also implement a kernel driver for the CMIS-compliant module, but the
driver would be hard to support features like firmware upgrade and debugging.
Those features require the raw i2c access, thus the kernel driver has to
maintain a lock to gate the module access. Considering that the kernel space is
usually more difficult to debug and unit test, the user space daemon is an
easier approach and serves the same purpose.

## Testing
Testing can be done by monitoring the module identification and telemetry data
from the dbus and comparing with other sensors reading.


