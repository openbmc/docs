# CMIS-compliant module management  daemon
Author: Jianqiao Liu (jianqiaoliu@google.com)

Primary assignee: Jianqiao Liu (jianqiaoliu@google.com)

Other contributors: Jason Ling (jasonling@google.com)

## Problem Description
The Common Management Interface Specification (CMIS, [Rev 5.0](http://www.qsfp-dd.com/wp-content/uploads/2021/05/CMIS5p0.pdf)) is an open industry standard for pluggable or on-board modules with host to module communication based on a Two-Wire Interface (TWI), such as QSFP Double Density (QSFP-DD), OSFP, COBO, QSFP, SFP-DD as well as future modules . The CMIS is developed to allow the host and the module vendor to utilize a common code base across a variety of form factors. It provides a set of core functionalities that all modules must implement and a set of optional features whose implementation is advertised in the module memory map.

This design doc is to propose a management daemon to detect the CMIS-compliant modules in the system, enable their features and facilitate the management work (e.g. telemetry reporting and firmware upgrade).

## Background and Reference
This CMIS-compliant module management daemon follows the Common Management Interface Specification ([Rev 3.0](http://www.qsfp-dd.com/wp-content/uploads/2018/09/QSFP-DD-mgmt-rev3p0-final-9-18-18-Default-clean.pdf), [Rev 4.0](http://www.qsfp-dd.com/wp-content/uploads/2019/05/QSFP-DD-CMIS-rev4p0.pdf), [Rev 5.0](http://www.qsfp-dd.com/wp-content/uploads/2021/05/CMIS5p0.pdf)). 

### Requirements
1. The daemon should detect the CMIS-compliant module by either GPIO pin interrupts or periodically polling. It works as the source-of-the-truth for the module presence. For a CMIS-compliant module already plugged into the system, we also want to differentiate its work states. For example, when a module is under firmware upgrade or debug mode, it may not be able to report identification or telemetry data. The management daemon should report the module status as present but unusable.
2. All CMIS-compliant modules should make their static identification information, e.g. identifier, vendor name, part number and vendor revision, readable through the TWI. Some active modules (mostly optics) utilize Digital Signal Processing technology and complex signal modulation schemes to achieve high data rate and longer transmission range. Those modules usually carry a micro-controller and are able to dynamically report telemetry data, e.g. temperature, tx/rx power, loss-of-signal (LoS), loss-of-lock (LoL), etc. The proposed daemon should collect both the static and dynamic telemetry data (if available), and expose them on the D-Bus for other usages.
3. The daemon should be able conduct firmware upgrade through Command Data Block based approach, which is defined in [CMIS-4.0](http://www.qsfp-dd.com/wp-content/uploads/2019/05/QSFP-DD-CMIS-rev4p0.pdf) (Section 7.2.2) and above, on modules that support this feature.

### Other references:
SFF Module Management Reference Code Tables ([Rev 4.9](https://www.snia.org/technology-communities/sff/specifications))

## Proposed Design

### CMIS module interface
A new dbus interface `xyz/openbmc_project/Inventory/Item/CmisModule.interface.yaml` will be introduced to model the CMIS-compliant module. Note that the CMIS module carries some properties quite different from other system components in the OpenBMC repository, e.g. vendor OUI and vendor revision. Creating a new interface is much easier than concatenating multiple other existing interfaces. An example looks like this:

```
/xyz/openbmc_project/Inventory/Item/CmisModule/Module.interface.yaml

description:
  This defines a CMIS-compliant module to be exposed for system management. Its implementation should describe the module properties and operational state.

properties:
    - name: Identifier
      default: ""
      type: int
      description: Identifier type of the module.
    - name: VendorOUI
      default: ""
      type: string
      description: The module vendor IEEE company ID.
    - name: VendorName
      default: ""
      type: string
      description: The module vendor name.
    - name: PartNumber
      default: ""
      type: string
      description: The module part number provided by the vendor.
    - name: SerialNumber
      default: ""
      type: string
      description: The module vendor serial number.
    - name: VendorRevision
      default: ""
      type: string
      description: The module revision level provided by the vendor.
    - name: State
      type: enum[self.ModuleState]
      default: 'Unknown'
      description: Current State of the module.    

enumerations:
    - name: ModuleState
      description: >
      values:
        - name: Unknown
          description: The module status is unknown. This indicates some errors on the module or gBMC side.
        - name: Uninitialized
          description: The module was plugged in but has not finished initialization yet.
        - name: Ready
          description: The module has been initialized successfully and is ready for data traffic.
        - name: UpgradingFirmware
          description: The module is upgrading the firmware (MCU/DSP/firmware_revision). Note that this state is unavailable for passive modules which do not carry any firmware.
        - name: UnderDebug
          description: The module is being managed by other debugging tools, not the management daemon. Note that this state is unavailable for passive modules which do not support debugging features.
```

Note that the module/cable presence should be exposed through the existing `xyz/openbmc_project/State/Decorator/Availability.interface.yaml interface`. The ModuleState enumeration indicates the plugged module’s readiness of data reporting and various features.

### Telemetry reporting (optics module only)
A new daemon will be introduced as the module detector and reactor. This daemon will periodically collect telemetry data from the module and publish on the dbus. In OpenBMC D-Bus producer-consumer module, this daemon should be the producer, and other daemon like bmcweb and phosphor-pid-control would be the consumer. Here are some examples:

| Name              | Type   | Description                                                                                                       |
|-------------------|--------|-------------------------------------------------------------------------------------------------------------------|
| Temperature       | DOUBLE | The measured module temperature in degrees celsius.                                                               |
| Supply Voltage    | DOUBLE | The measured voltage supplied to the module in volts.                                                             |
| Tx Power          | DOUBLE | The measured Tx output optical power in dBm.                                                                      |
| Rx Power          | DOUBLE | The measured Rx input optical power in dBm.                                                                       |
| Tx Loss-of-Signal | BYTE   | Multi-bit value with 1 bit per lane. True if the 'Loss of Signal' flag is raised on the corresponding Tx lane.    |
| Tx Loss-of-Lock   | BYTE   | Multi-bit value with 1 bit per lane. True if the 'CDR Loss of Lock’ flag is raised on the corresponding Tx lane.  |
| Rx Loss-of-Signal | BYTE   | Multi-bit value with 1 bit per lane. True if the 'Loss of Signal' flag is raised on the corresponding Rx lane.    |
| Rx Loss-of-Lock   | BYTE   | Multi-bit value with 1 bit per lane. True if the 'CDR Loss of Lock’ flag is raised on the corresponding Rx lane.  |


Among the above data, the management daemon will expose the temperature and supply voltage data through existing `xyz/openbmc_project/Sensor/Value.interface.yaml`, and create a new interface for the rest. The new interface may look like this:

```
xyz/openbmc_project/Inventory/Item/CmisModule/Telemetry.interface.yaml

description: This defines the interface for CMIS-compliant module telemetry data.

properties:
    - name: TxPower
      default: 0
      type: double
      description: The measured Tx output optical power in dBm.
    - name: RxPower
      default: 0
      type: double
      description: The measured Rx output optical power in dBm.
    - name: TxLossOfSignal
      default: 0
      type: byte
      description: Multi-bit value and 1 bit per lane. True if the 'Loss of Signal' flag is raised on the corresponding Tx lane. CMIS supports 8 lanes per module at most.
    - name: RxLossOfSignal
      default: 0
      type: byte
      description: Multi-bit value and 1 bit per lane. True if the 'Loss of Signal' flag is raised on the corresponding Rx lane. CMIS supports 8 lanes per module at most.
    - name: TxLossOfLock
      default: 0
      type: byte
      description: Multi-bit value and 1 bit per lane. True if the 'Loss of Lock' flag is raised on the corresponding Tx lane. CMIS supports 8 lanes per module at most.
    - name: RxLossOfLock
      default: 0
      type: byte
      description: Multi-bit value and 1 bit per lane. True if the 'Loss of Lock' flag is raised on the corresponding Rx lane. CMIS supports 8 lanes per module at most.
```

### Firmware upgrading (optics module only)
[CMIS-4.0](http://www.qsfp-dd.com/wp-content/uploads/2019/05/QSFP-DD-CMIS-rev4p0.pdf) has already defined the Command Data Block based firmware upgrade approach (Section 7.2.2 Firmware Upgrade Commands in CDB). This management daemon follows this public specification and accepts a series of firmware images as the input.

### Module initialization & Configuration
The CMIS-compliant module is usually plugged in some slots on the electrical board or tray. Each slot has its own bus id/address, and allows a single module at a time. Depending on system design or logistics considerations, one electrical board may connect with modules produced by different vendors, and even with different media types (copper/optics). 

When a module is plugged in the slot, the management daemon will take it out of reset and load corresponding configuration according to the module’s type and identification data (vendor name, part number, etc).

## Alternatives considered
We may also implement a kernel driver for the CMIS-compliant module, but the driver would be hard to support features like firmware upgrade and debugging. Those features require the raw i2c access, thus the kernel driver has to maintain a lock to gate the module access. Considering that the kernel space is usually more difficult to debug and unit test, the user space daemon is an easier approach and serves the same purpose.

## Testing
Testing can be done by monitoring the module identification and telemetry data from the dbus and comparing with other sensors reading.


