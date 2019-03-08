____
### NVMe-MI over SMBus

Author:
  Tony Lee <tony.lee@quantatw.com>
Primary assignee:
  Tony Lee <tony.lee@quantatw.com>
Created:
  3-8-2019

#### Problem Description

Currently, it didn't support the NVMe drives information in OpenBMC. NVMe-MI
specification defines a command that can read the NVMe drive information via
SMBus directly. The NVMe drive can provide its information or status, like
vendor ID, temperature… etc. The aim of this proposal is desire to let users can
monitor NVMe drives so that user can take corresponding actions.

#### Background and References

NVMe-MI specification defines a command called
`NVM Express Basic Management Command` that can read the NVMe drives
information via SMBus directly. [1]. This command via SMBus Block Read protocol
specified by the SMBus specification. [2].

For our purpose is retrieve NVMe drives information, therefore, using NVM
Express Basic Management Command where describe in NVMe-MI specification to
communicate with NVMe drives. According to different platforms, temperature
sensor, present status, LED and power sequence will be customized.

[1] NVM Express Management Interface Revision 1.0a April 8, 2017 in Appendix A.
[2] System Management Bus (SMBus) Specification Version 3.0 20 Dec 2014

#### Requirements

The implementation should:

- Provide a daemon to monitor NVMe drives. Parameters to be monitored are
  Status Flags, SMART Warnings, Temperature, Percentage Drive Life Used, Vendor
  ID, and Serial Number.
- Provide a d-Bus interface to allow other function to access data.
- Capability of communication over hardware channel I2C to NVMe drives.
- Ability to turn the fault LED on/off for each drive by SmartWarnings if the
  object path of fault LED is defined in the configuration file.

#### Proposed Design

First it will create d-bus named "xyz.openbmc_project.nvme.manager", second,
there were multiple object paths: "/xyz/openbmc_project/nvme/1",
"/xyz/openbmc_project/nvme/2", and so on where under this d-bus.
There is a configuration file in json to configure drive index, bus ID, slave
address and the object path of fault LED for each drive.
For example,
{
  "NVMeDriveIndex":1,
  "NVMeDriveBusID":14,
  "NVMeDriveSlaveAddress":106,
  "NVMeDriveFaultLED":"/xyz/openbmc_project/led/groups/LED_U2_0_FAULT"
},
{
  "NVMeDriveIndex":2,
  "NVMeDriveBusID":15,
  "NVMeDriveSlaveAddress":106,
  "NVMeDriveFaultLED":"/xyz/openbmc_project/led/groups/LED_U2_1_FAULT"
}
The maximum number of drives is 16.

Structure like:
/xyz
  └─/xyz/openbmc_project
    └─/xyz/openbmc_project/nvme
      ├─/xyz/openbmc_project/nvme/1
      ├─/xyz/openbmc_project/nvme/2
      ├─/xyz/openbmc_project/nvme/3
      ├─/xyz/openbmc_project/nvme/4
      ├─/xyz/openbmc_project/nvme/5
      ├─/xyz/openbmc_project/nvme/6
      ├─/xyz/openbmc_project/nvme/7
      └─/xyz/openbmc_project/nvme/8

As for the interface which under object path, it will have four interfaces:

- interface   : `xyz.openbmc_project.Sensor.Value`
- interface   : `xyz.openbmc_project.Nvme.Information`
- interface   : `xyz.openbmc_project.Inventory.Item`
- interface   : `xyz.openbmc_project.Inventory.Decorator.Asset`

Interface `xyz.openbmc_project.Sensor.Value`, it's for hwmon to monitor
temperature and with the following properties:

| Property | Type | Description |
| -------- | ---- | ----------- |
| MaxValue | int64 | Sensor maximum value |
| MinValue | int64 | Sensor minimum value |
| Scale | int64 | Sensor value scale |
| Unit | string | Sensor unit |
| Value | int64 | Sensor value |

Interface `xyz.openbmc_project.Nvme.Information` with the following properties:
| Property | Type | Description |
| -------- | ---- | ----------- |
| SmartWarnings| string | Indicates smart warnings for the state |
| StatusFlags | string | Indicates the status of the drives |
| DriveLifeUsed | string | A vendor specific estimate of the percentage |
| TemperatureFault| bool | If warning type about temperature is happened |
| BackupdrivesFault | bool | If warning type about backup drives is happened |
| CapacityFault| bool | If warning type about capacity is happened |
| DegradesFault| bool | If warning type about degrades is happened |
| MediaFault| bool | If warning type about media is happened |

Interface `xyz.openbmc_project.Inventory.Item` with the following properties:
| Property | Type | Description |
| -------- | ---- | ----------- |
| PrettyName| string | The human readable name of the item |
| Present | bool | Whether or not the item is present |

Interface `xyz.openbmc_project.Inventory.Decorator.Asset` with the following
properties:
| Property | Type | Description |
| -------- | ---- | ----------- |
| PartNumber| string | The item part number, typically a stocking number |
| SerialNumber | string | The item serial number |
| Manufacturer | string | The item manufacturer |
| BuildDate| bool | The date of item manufacture in YYYYMMDD format |
| Model | bool | The model of the item |

##### xyz.openbmc_project.nvme.manager.service

This service has several steps:

1. It will register a d-bus called `xyz.openbmc_project.nvme.manager`
   description above.
2. This service retrieves the data of NVMe drives. The way to get data of NVMe
   drives is to send a NVMe-MI command via SMBus Block Read protocol. Parameters
   from NVMe drives are "Status Flags", "SMART Warnings", "Temperature",
   "Percentage Drive Life Used", "Vendor ID", and "Serial Number".
3. The parameters in the previous step will be set to the properties in d-bus
   description above.

This service will run automatically and look up NVMe drives every second.

##### Fault LED

When the value obtained from the command corresponds to one of the warning
types, it will trigger the fault LED of corresponding device and issue events.

##### Add SEL related to NVMe

The events `TemperatureFault`, `BackupdrivesFault`,
`CapacityFault`, `DegradesFault` and `MediaFault` will be generated for the
NVMe errors.

- Temperature Fault log : when the property `TemperatureFault` set to true
- Backupdrives Fault log : when the property `BackupdrivesFault` set to true
- Capacity Fault log : when the property `CapacityFault` set to true
- Degrades Fault log : when the property `DegradesFault` set to true
- Media Fault log : when the property `MediaFault` set to true

#### Alternatives Considered

NVMe-MI specification defines multiple commands that can communicate with
NVMe drives over MCTP protocol. The NVMe-MI over MCTP has the following key
capabilities:

- Discover drives that are present and learn capabilities of each drives.
- Store data about the host environment enabling a Management Controller to
  query the data later.
- A standard format for VPD and defined mechanisms to read/write VPD contents.
- Inventorying, configuring and monitoring.

For monitoring NVMe drives, using NVM Express Basic Management Command over
SMBus directly is much simpler than NVMe-MI over MCTP protocol.

#### Impacts

This application is monitoring NVMe drives via SMbus and set values to d-bus.
The impacts should be small in the system.

#### Testing

This implementation is to use NVMe-MI-Basic command over SMBus and then set the
reponsed data to d-bus.
Testing will send SMBus command to the drives to get the information and compare
with the properties in d-Bus to make sure they are the same.
The testing can be performed on different NVMe drives by different manufactures.
For example: Intel P4500/P4600 and Micron 9200 Max/Pro.

Unit tests will test by function:

- It tests the length of responded data is as same as design in the function
of getting NVMe information.
- It tests the function of setting values to d-bus is as same as design.
- It tests the function of turn the corresponding LED ON/OFF by different
Smartwarnings values.