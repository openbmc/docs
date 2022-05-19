# Design Proposal for NVMe Out-Of-Band Management

Author: Hao Jiang (Haoj)

Other contributors: Ed Tanous (edtanous), Jeremy Kerr

Created: Feb 10, 2022

## Problem Description

Typically the NVMe storage devices are accessed and controlled by the
driver/software on Host machines, meaning both IO and administration commands
are sent via inband PCIe links. With the introduction of BMC, the control domain
can be shifted from inband HOST to out-of-band BMC. The same trend happens on
NVMe device.

The administrative task for out-of-band NVMe management will include but not
limited to:

*   Enumeration of the NVMe devices and their substructures
    (Subsystem/Controller/Namespace/Endurance Group/etc).
*   Properties and capabilities of the device and substructures, such as
    drive/namespace capacity, supported format, SRIOV capability, etc.
*   Runtime telemetries such as SMART log, device self test report, Sanitize
    status, vendor defined telemetries, etc.
*   Creation and deletion of the Namespaces
*   Format/Sanitize the drive
*   Security operations such as lock/unlock the drive
*   Firmware update

The importance of out-of-band management grows for the VM and Baremetal
customers in case of the PCIe access is fully deployed to tenant. However, we
are lacking of a daemon to provide a out-of-band Swiss tool to replace the
inband management, providing the **full stack** NVMe administrative
functionilty.

## Background and References

1.  [NVM Express Base Specification][nvme-base]
2.  [NVM Express Management Interface Specification][nvme-mi]
3.  [Management Component Transport Protocol (MCTP) Base Specification][dmtf-pmci-dsp0236]
4.  [Management Component Transport Protocol (MCTP) SMBus/I2C Transport Binding
    Specification][dmtf-pmci-dsp0237]
5.  [Swordfish Scalable Storage Management API Specification][snia-swordfish]
6.  [Technical Note on Basic Management Command][nvme-mi-basic-technical-note]
7.  [SNIA Swordfish Mapping Guidance][snia-swordfish-mapping]

[nvme-base]:
  https://nvmexpress.org/wp-content/uploads/NVM-Express-Base-Specification-2.0c-2022.10.04-Ratified.pdf
[nvme-mi]:
  https://nvmexpress.org/wp-content/uploads/NVM-Express-Management-Interface-Specification-1.2c-2022.10.06-Ratified.pdf
[dmtf-pmci-dsp0236]:
  https://www.dmtf.org/sites/default/files/standards/documents/DSP0236_1.3.1.pdf
[dmtf-pmci-dsp0237]:
  https://www.dmtf.org/sites/default/files/standards/documents/DSP0237_1.2.0.pdf
[dmtf-pmci-dsp0238]:
  <https://www.dmtf.org/sites/default/files/standards/documents/DSP0238_1.0.1.pdf>
[snia-swordfish]:
  <https://www.snia.org/sites/default/files/technical-work/swordfish/release/v1.2.5a/pdf/Swordfish_v1.2.5a_Specification.pdf>
[nvme-mi-basic-technical-note]:
  <https://nvmexpress.org/wp-content/uploads/NVMe_Management_-_Technical_Note_on_Basic_Management_Command_1.0a.pdf>
[snia-swordfish-mapping]: https://www.snia.org/sites/default/files/technical-work/swordfish/draft/v1.2.2/pdf/Swordfish_v1.2.2_NVMeMappingGuide.pdf

The [NVM Express Base Specification][nvme-base] defines the architecture for
NVMe devices upon which includes the administrative controller as well as a set
of cmd to interact with the controller for administrative job, called Admin
Command Set.

In addition, the NVM Express Management IF Workgroup also published the
[NVM Express Management Interface Specification][nvme-mi] for out-of-band (and
inband tunneling which is out of scope for this doc) management. The Management
Interface(MI) can also get access to the Admin Command Set same as the inband
path. The data link layer for MI is based on [MCTP][dmtf-pmci-dsp0236] with
physical layer of [SMBus][dmtf-pmci-dsp0237] or [PCIe VDM][dmtf-pmci-dsp0238].

On the north bound, [Swordfish][snia-swordfish], which is an extension of DMTF
Redfish specification, providing a RESTful API for NVMe storage device as well
as the mapping guidance between the standard NVMe definition and RESTful schema.

There are couple of OpenBMC services striking the NVMe area, which are:

*   [phosphor-nvme](https://github.com/openbmc/phosphor-nvme)
*   [nvmesensor](https://github.com/openbmc/dbus-sensors/commit/b669b6b54b1bfa499380e518bf508b672dd9a125)

Both of the daemon is providing the thermal services based the [NVMe Basic
protocol]. No furthur NVMe-MI support is included in either of the daemon yet. A
[discussion for nvmesensor split](https://gerrit.openbmc.org/c/openbmc/docs/+/67015)
works pretty well as a background story teller that can help to distinguish
between the existing daemons.

Other useful repositories that are already merged into OpenBMC are:

*   [libmctpd](https://github.com/openbmc/libmctp): An implementation for
    Management Component Transport Protocol(MCTP).
*   [mtcpd](https://github.com/CodeConstruct/mctp): Userspace tools for MCTP
    stack management.
*   [libnvme-mi](https://github.com/CodeConstruct/libnvme): library to implement
    NVMe-MI functions over MCTP message.
*   [BMCWeb](https://github.com/openbmc/bmcweb)

The latter three repos serve as the underlay driver to transmit NVMe messages
between BMC and the target device. BMCWeb is working as a proxy service to
bridge the backend service to the frontend clients.

## Requirements

The proposed daemon will remain and extend the thermal service in the same way
that the MI spec is extending the capability of the Basic protocol. The daemon
will provide all services for the NVMe management stack with the same
functionilty as the inband software.

The proposed daemon will utilize the DBus as the interface to interact with
other services on openBMC. The daemon will:

*   Presenting the enumeration result as DBus objects
*   Attaching the appropriate method to interact with NVMe objects
*   Polling and updating the attributes for NVMe objects
*   Managing the lifetime of NVMe objects and syncing to the NVMe device

The BMCWeb with Redfish/Swordfish will be served as the first class client under
the guidance of the [Redfish/Swordfish mapping][snia-swordfish-mapping].

NVMe-MI over MCTP over SMBus works as the backend protocol to communicate with
the NVMe device. The existing MCTP/I2C libraries, daemons, and kernel drivers
will be utilize to establish the communicate, meaning the proposed daemon will
sololy work on top of the MCTP endpoint, limiting its functionilty to the scope
within NVMe subsystem and MCTP endpoint management.

## Proposed Design

The prospose daemon will be called `nvmed` which extended and replace the
original `nvmesensor`. The largeset reason choosing `nvmesensor` over
`phosphor-nvme` is because the the NVMe device requires a FRU device on I2C bus
which is quite compatitive to the dynamic enumeration with the help of
[Entity-Manager](https://github.com/openbmc/entity-manager).

### NVMe Subsystem Enumeration

The enumeration for NVMe subsystem will reuse the current reactor model of NVMe
sensor. Configuration interface will be preserved as
`xyz.openbmc_project.Configuration.NVME1000`. And we will introduce a new enum
field to the configuration, named `Protocol`.

**Value**       | **Description(NVMe MI specification rev1.2)**
--------------- | ---------------------------------------------
Basic (default) | NVMe basic management commands (Appendix A)
MiI2c           | NVMe MI commands over MCTP over I2C (Chapter 2.2)
MiVdm           | NVMe MI commands over MCTP over PCIe Vendor Defined Messages(VDMs) (Chapter 2.1)

An example for NVMe configuration:

```json
{
  "Exposes": [
    {
      "Type": "NVMe1000",
      "Name": "ExampleNVMe",
      "Protocol": "mi_i2c",
      "Bus": "$bus",
      "Address": "0x1d",
      "Thresholds": [
        {
            "Direction": "greater than",
            "Name": "upper critical",
            "Severity": 1,
            "Value": 115
        },
        {
            "Direction": "greater than",
            "Name": "upper non critical",
            "Severity": 0,
            "Value": 110
        },
        {
            "Direction": "less than",
            "Name": "lower non critical",
            "Severity": 0,
            "Value": 5
        },
        {
            "Direction": "less than",
            "Name": "lower critical",
            "Severity": 1,
            "Value": 0
        }
      ]
     }
  ],
  "Probe": "xyz.openbmc_project.FruDevice({'BOARD_PRODUCT_NAME': 'ExampleProduct'})",
  "xyz.openbmc_project.Inventory.Decorator.Asset": {
        "Manufacturer": "$PRODUCT_MANUFACTURER",
        "Model": "$PRODUCT_PRODUCT_NAME",
        "PartNumber": "$PRODUCT_PART_NUMBER",
        "SerialNumber": "$PRODUCT_SERIAL_NUMBER"
  }
}
```

The proposal will focus on `mi-i2c` protocol. The `basic` has been supported by
current NVMe sensor.

### NVMe Dbus Structure

A simple SSD should expose the following Dbus tree via `nvmed`.

This Dbus structure should be similar to the NVMe architecture as well as
Swordfish Storage resource model. The exposed interfaces are also mappable to
Redfish resources. Associations are created corresponding to redfish links.
Redfish server such as BMCWeb should be able to reconstruct the Storage
recources by enumerating the corresponding interfaces out of `nvmed`.

```
/xyz/openbmc_project/inventory/system/board/IronFist_1
   └─/xyz/openbmc_project/inventory/system/board/Chassis_1/NVMe_1
      │     │ │ ├─/xyz/openbmc_project/inventory/system/board/Chassis_1/NVMe_1/controllers
      │     │ │ │ └─/xyz/openbmc_project/inventory/system/board/Chassis_1/NVMe_1/controllers/0
      │     │ │ └─/xyz/openbmc_project/inventory/system/board/Chassis_1/NVMe_1/volumes
      │     │ │   └─/xyz/openbmc_project/inventory/system/board/Chassis_1/NVMe_1/volumes/1
      └─/xyz/openbmc_project/sensors
         └─/xyz/openbmc_project/sensors/temperature
         ├─/xyz/openbmc_project/sensors/temperature/Chassis_1_NVMe_1
```

#### Chassis

The Chassis information doesn't read from NVMe subsystem and should not be
provided by `nvmed`. Instead, EntityManager should expose the chassis
information via other probing service such as FruDevice. The enumeration process
has been stated in [Subsystem Enumeration](#nvme-subsystem-enumeration)

#### Subsystem

A NVMe subsystem will be created under a Dbus path of
`/xyz/openbmc_project/inventory/system/board/Chassis_1/NVMe_1`.

The subsystem path will expose the following interfaces:

Dbus interface                             | Swordfish Resource
------------------------------------------ | ------------------
xyz.openbmc_project.Inventory.Item.Drive   | Drive
xyz.openbmc_project.Inventory.Item.Storage | Storage

##### thermal sensor

At least one thermal sensor is created to reflect the NVMe subsystem Composite
Temperature, similar to the temperature reading from NVMe Basic.

```
/xyz/openbmc_project/sensors/temperature/Chassis_1_NVMe_1
```

The thresholds can be read via CCTEMP(Critical Composite Temperature Threshold)
and WCTEMP(Warning CompositeTemperature Threshold) from device or over-written
by E-M config file.

##### Associations

The following Associations will be created:

Path                                                         | Forward | Backward    | Forward Path
------------------------------------------------------------ | ------- | ----------- | ------------
/xyz/openbmc_project/inventory/system/board/Chassis_1/NVMe_1 | chassis | drive       | /xyz/openbmc_project/inventory/system/board/Chassis_1
/xyz/openbmc_project/inventory/system/board/Chassis_1/NVMe_1 | chassis | storage     | /xyz/openbmc_project/inventory/system/board/Chassis_1
/xyz/openbmc_project/inventory/system/board/Chassis_1/NVMe_1 | drive   | storage     | /xyz/openbmc_project/inventory/system/board/Chassis_1/NVMe_1
/xyz/openbmc_project/sensors/temperature/Chassis_1_NVMe_1    | chassis | all_sensors | /xyz/openbmc_project/inventory/system/board/Chassis_1

#### Controller

A healthy NVMe subsystem will expose one or more controllers as subdirectories.

```
/xyz/openbmc_project/inventory/system/board/Chassis_1/NVMe_1/controllers/0
```

A NVMe controller will expose the following interfaces:

Dbus interface                                       | Swordfish Resource
---------------------------------------------------- | ---------------------
xyz.openbmc_project.Inventory.Item.StorageController | StorageController
xyz.openbmc_project.NVMe.NVMeAdmin                   | OEM StorageController

`xyz.openbmc_project.NVMe.NVMeAdmin` is a NVMe Specific interface which will be
addressed in the following chapter.

##### Associations

All NVMe controllers will expose the association to the belonging subsystem.

Path                                                                       | Forward | Backward           | Forward Path
-------------------------------------------------------------------------- | ------- | ------------------ | ------------
/xyz/openbmc_project/inventory/system/board/Chassis_1/NVMe_1/controllers/0 | storage | storage_controller | /xyz/openbmc_project/inventory/system/board/Chassis_1/NVMe_1

##### Primary Controller (PC)/Secondary Controller(SC)

If Virtualization is enabled for NVMe subsystem, then the SC would be created by
the PC. And an association will be created to reflect such relationship. Thus
for each PC, there will be an additional association:

Path                                                                        | Forward   | Backward | Forward Path
--------------------------------------------------------------------------- | --------- | -------- | ------------
/xyz/openbmc_project/inventory/system/board/Chassis_1/NVMe_1/controllers/0* | secondary | primary  | /xyz/openbmc_project/inventory/system/board/Chassis_1/NVMe_1/controllers/1**

\* Primary Controller path \
\*\* Secondary Controller path

Note: A SC with offline status will not be presented on the Dbus until it has
been transit into online/active via Virtualization configuration.

Such PC/SC relation needs to be exposed because the client is only permitted to
perform privileged operations (such as NS attachment) via PC.

##### NVMeAdmin Interface

We are introducing a `xyz.openbmc_project.NVMe.NVMeAdmin` interface for NVMe
controllers. The definition is shown as the ymal file:

```ymal
description: >
  NVMe MI Admin inteface for NVMe commands

methods:
  - name: Identify
    description: >
      Send Identify command to NVMe device
    parameters:
      - name: CNS
        type: byte
        description: >
          Controller or Namespace Structure
      - name: NSID
        type: uint32
        description: >
          Namespace Identifier
      - name: CNTID
        type: uint16
        description: >
          Controller Identifier
    returns:
      - name: Data
        type: unixfd
        description: >
          Identify Data
    errors:
      - xyz.openbmc_project.Common.File.Error.Open
  - name: GetLogPage
    description: >
      Send GetLogPage command to NVMe device
    parameters:
      - name: LID
        type: byte
        description: >
          Log Page Identifier
      - name: NSID
        type: uint32
        description: >
          Namespace Identifier
      - name: LSP
        type: byte
        description: >
          Log Specific Field
      - name: LSI
        type: uint16
        description: >
          Log Specific Identifier
    returns:
      - name: Log
        type: unixfd
        description: >
          Returned Log Page
    errors:
      - xyz.openbmc_project.Common.File.Error.Open
      - xyz.openbmc_project.Common.Error.InvalidArgument

```

The interface means to read all NVMe telemetries but in NVMe specification
fashion. The reason to have such an interface is because the SNIA Swordfish
falls behind the definition in NVMe specification. For example, There are more
than 20 Log Pages defined in the NVMe spec but merely the SMART log is being
supported in the recent SNIA Swordfish release. The interface is designed to
fill in the gap before SNIA Swordfish property and metrics can provide enough
information to monitoring the NVMe telemetries. It provide the similar way as
the inband path to the client software, and cover the same functionality.

During the transition period, more NVMe telemetries will merge into the
Swordfish oriented DBus properties.

<p style="text-align: center;"> NVMe Log Definition </p>

LID | Log Name                                                       | NVMe Base Spec Chapter
--- | -------------------------------------------------------------- | ----------------------
00h | Controller Supported Log Pages                                 | 5.16.1.1
01h | Controller Error Information                                   | 5.16.1.2
02h | Controller 1 SMART / Health Information                        | 5.16.1.3
03h | Domain / NVM subsystem 6 Firmware Slot Information             | 5.16.1.4
04h | Controller Changed Namespace List                              | 5.16.1.5
05h | Controller Commands Supported and Effects                      | 5.16.1.6
06h | Device Self-test                                               | 5 5.16.1.7
07h | Vendor Specific Telemetry Host-Initiated 5                     | 5.16.1.8
08h | Vendor Specific Telemetry Controller-Initiated 5               | 5.16.1.9
09h | Endurance Group Information                                    | 5.16.1.10
0Ah | Predictable Latency Per NVM Set                                | 5.16.1.11
0Bh | Predictable Latency Event Aggregate                            | 5.16.1.12
0Ch | Controller Asymmetric Namespace Access                         | 5.16.1.13
0Dh | NVM subsystem Persistent Event Log 5                           | 5.16.1.14
0Fh | Domain / NVM subsystem 6 Endurance Group Event Aggregate       | 5.16.1.15
10h | Domain / NVM subsystem 5, 6 Media Unit Status                  | 5.16.1.16
11h | Domain / NVM subsystem 6 Supported Capacity Configuration List | 5.16.1.17
12h | Controller Feature Identifiers Supported and Effects           | 5.16.1.18
13h | Controller NVMe-MI Commands Supported and Effects              | 5.16.1.19
14h | NVM subsystem Command and Feature Lockdown 5                   | 5.16.1.20
15h | Controller Boot Partition                                      | 5.16.1.21
16h | Endurance Group Rotational Media Information                   | 5.16.1.22
70h | Discovery                                                      | 5.16.1.23
80h | Controller Reservation Notification                            | 5.16.1.24
81h | NVM subsystem Sanitize Status                                  |

<p style="text-align: center;"> SNIA Swordfish  1.2.5a Release Note on 20 June 2023</p>

> New functionality includes metrics for volumes, drives, and storage
> controllers, as well as support for NVMe SMART Metrics, enhanced NVMe-oF
> discovery controller capabilities managing NVMe-oF centralized discovery
> controllers
>
> #### Namespace

If namepaces exist within a subsystem, then the following objects will be
created on Dbus:

```
/xyz/openbmc_project/inventory/system/board/Chassis_1/NVMe_1/volumes/1
```

A NVMe namespace will expose the following Dbus interface:

Dbus interface                            | Swordfish Resource
----------------------------------------- | ------------------
xyz.openbmc_project.Inventory.Item.Volume | Volume

##### Associations

All NVMe Namespaces will expose the following associations:

Path                                                                   | Forward   | Backward   | Forward Path
---------------------------------------------------------------------- | --------- | ---------- | ------------
/xyz/openbmc_project/inventory/system/board/Chassis_1/NVMe_1/volumes/1 | contained | containing | /xyz/openbmc_project/inventory/system/board/Chassis_1/NVMe_1
/xyz/openbmc_project/inventory/system/board/Chassis_1/NVMe_1/volumes/1 | attached  | attaching  | /xyz/openbmc_project/inventory/system/board/Chassis_1/NVMe_1/controllers/0

## Alternatives Considered

The NVMe daemon can expose the protocol API on DBus and work as a NVMe device
proxy. The daemon manages the transaction across multiple services to the remote
device. The client, such as BMCWeb, takes the responsiblity for data parsing and
resource construction.

This solution has been discussed on the
[gerrit](https://gerrit.openbmc.org/c/openbmc/docs/+/53809/2). The conclusion is
that the standard DBus storage interfaces are preferred to be constructed with
the NVMe daemon, which providing the structured data as passing-through to
Redfish.

## Impacts

### Organizational

The new daemon is suppose to swarm many tens of patches to a single nvmesensor
within dbus-sensors which may break the maintenance balance of that repository.
A [proposal](https://gerrit.openbmc.org/c/openbmc/docs/+/67015) to separate to
the nvmesensor from dbus-sensors has been discussed. Then a repository will be
introduced to OpenBMC but the daemon for NVMe management remains two.

## Security

The new proposed NVMeAdmin interface will transfer the unparsed NVMe data chuck.
However the client can only read the data out of the NVMe device without any
physical connectivity knowledge nor manipulating the connectivity. All other
privileged operations out of `nvmed` will be protected by Security layer of NVMe
such as TCG operations. The security level should remains not lower than the
original inband path.

## Testing

*   A Mock libnvme will be created for functional test.
*   The performance will be evaluated from the following aspects:

    *   I2C load when transferring large message, e.g. Persistent Log.
    *   Interface from/toward inband IO.
    *   Delays across multiple queries, e.g. delays on thermal sensor while a
        large log transfer.

    describes the NVMe Basic Management Command and is included here for
    informational purposes only. The NVMe Basic Management Command is not
    formally a part of this specification and its features are not tested by the
    NVMe Compliance program." host software", Page 107, NVM Express Management
    Interface Specification, Rev 1.2
