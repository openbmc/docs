### NVMe-MI over SMBus

Author: Tony Lee <tony.lee@quantatw.com>

Created: 3-8-2019

#### Problem Description

Currently, OpenBMC does not support NVMe drive information. NVMe-MI
specification defines a command that can read the NVMe drive information via
SMBus directly. The NVMe drive can provide its information or status, like
vendor ID, temperature, etc. The aim of this proposal is to allow users to
monitor NVMe drives so appropriate action can be taken.

#### Background and References

NVMe-MI specification defines a command called
`NVM Express Basic Management Command` that can read the NVMe drives information
via SMBus directly. [1]. This command uses SMBus Block Read protocol specified
by the SMBus specification. [2].

For our purpose is retrieve NVMe drives information, therefore, using NVM
Express Basic Management Command where describe in NVMe-MI specification to
communicate with NVMe drives. According to different platforms, temperature
sensor, present status, LED and power sequence will be customized.

[1] NVM Express Management Interface Revision 1.0a April 8, 2017 in Appendix A.
(https://nvmexpress.org/wp-content/uploads/NVM_Express_Management_Interface_1_0a_2017.04.08_-_gold.pdf)
[2] System Management Bus (SMBus) Specification Version 3.0 20 Dec 2014
(http://smbus.org/specs/SMBus_3_0_20141220.pdf)

#### Requirements

The implementation should:

- Provide a daemon to monitor NVMe drives. Parameters to be monitored are Status
  Flags, SMART Warnings, Temperature, Percentage Drive Life Used, Vendor ID, and
  Serial Number.
- Provide a D-bus interface to allow other services to access data.
- Capability of communication over hardware channel I2C to NVMe drives.
- Ability to turn the fault LED on/off for each drive by SmartWarnings if the
  object path of fault LED is defined in the configuration file.

#### Proposed Design

Create a D-bus service "xyz.openbmc_project.nvme.manager" with object paths for
each NVMe sensor: "/xyz/openbmc_project/sensors/temperature/nvme0",
"/xyz/openbmc_project/sensors/temperature/nvme1", etc. There is a JSON
configuration file for drive index, bus ID, and the fault LED object path for
each drive. For example,

```json
{
  "NvmeDriveIndex": 0,
  "NVMeDriveBusID": 16,
  "NVMeDriveFaultLEDGroupPath": "/xyz/openbmc_project/led/groups/led_u2_0_fault",
  "NVMeDrivePresentPin": 148,
  "NVMeDrivePwrGoodPin": 161
},
{
  "NvmeDriveIndex": 1,
  "NVMeDriveBusID": 17,
  "NVMeDriveFaultLEDGroupPath": "/xyz/openbmc_project/led/groups/led_u2_0_fault",
  "NVMeDrivePresentPin": 149,
  "NVMeDrivePwrGoodPin": 162
}
```

Structure like:

Under the D-bus named "xyz.openbmc_project.nvme.manager":

```
    /xyz/openbmc_project
    └─/xyz/openbmc_project/sensors
      └─/xyz/openbmc_project/sensors/temperature/nvme0
```

/xyz/openbmc_project/sensors/temperature/nvme0 Which implements:

- xyz.openbmc_project.Sensor.Value
- xyz.openbmc_project.Sensor.Threshold.Warning
- xyz.openbmc_project.Sensor.Threshold.Critical

Under the D-bus named "xyz.openbmc_project.Inventory.Manager":

```
/xyz/openbmc_project
    └─/xyz/openbmc_project/inventory
      └─/xyz/openbmc_project/inventory/system
        └─/xyz/openbmc_project/inventory/system/chassis
          └─/xyz/openbmc_project/inventory/system/chassis/motherboard
           └─/xyz/openbmc_project/inventory/system/chassis/motherboard/nvme0
```

/xyz/openbmc_project/inventory/system/chassis/motherboard/nvme0 Which
implements:

- xyz.openbmc_project.Inventory.Item
- xyz.openbmc_project.Inventory.Decorator.Asset
- xyz.openbmc_project.Nvme.Status

Interface `xyz.openbmc_project.Sensor.Value`, it's for hwmon to monitor
temperature and with the following properties:

| Property | Type   | Description          |
| -------- | ------ | -------------------- |
| MaxValue | int64  | Sensor maximum value |
| MinValue | int64  | Sensor minimum value |
| Scale    | int64  | Sensor value scale   |
| Unit     | string | Sensor unit          |
| Value    | int64  | Sensor value         |

Interface `xyz.openbmc_project.Nvme.Status` with the following properties:

| Property          | Type   | Description                                  |
| ----------------- | ------ | -------------------------------------------- |
| SmartWarnings     | string | Indicates smart warnings for the state       |
| StatusFlags       | string | Indicates the status of the drives           |
| DriveLifeUsed     | string | A vendor specific estimate of the percentage |
| TemperatureFault  | bool   | If warning type about temperature happened   |
| BackupdrivesFault | bool   | If warning type about backup drives happened |
| CapacityFault     | bool   | If warning type about capacity happened      |
| DegradesFault     | bool   | If warning type about degrades happened      |
| MediaFault        | bool   | If warning type about media happened         |

Interface `xyz.openbmc_project.Inventory.Item` with the following properties:

| Property   | Type   | Description                         |
| ---------- | ------ | ----------------------------------- |
| PrettyName | string | The human readable name of the item |
| Present    | bool   | Whether or not the item is present  |

Interface `xyz.openbmc_project.Inventory.Decorator.Asset` with the following
properties:

| Property     | Type   | Description                                       |
| ------------ | ------ | ------------------------------------------------- |
| PartNumber   | string | The item part number, typically a stocking number |
| SerialNumber | string | The item serial number                            |
| Manufacturer | string | The item manufacturer                             |
| BuildDate    | bool   | The date of item manufacture in YYYYMMDD format   |
| Model        | bool   | The model of the item                             |

##### xyz.openbmc_project.nvme.manager.service

This service has several steps:

1. It will register a D-bus called `xyz.openbmc_project.nvme.manager`
   description above.
2. Obtain the drive index, bus ID, GPIO present pin, power good pin and fault
   LED object path from the json file mentioned above.
3. Each cycle will do following steps:
   1. Check if the present pin of target drive is true, if true, means drive
      exists and go to next step. If not, means drive does not exists and remove
      object path from D-bus by drive index.
   2. Check if the power good pin of target drive is true, if true means drive
      is ready then create object path by drive index and go to next step. If
      not, means drive power abnormal, turn on fault LED and log in journal.
   3. Send a NVMe-MI command via SMBus Block Read protocol by bus ID of target
      drive to get data. Data get from NVMe drives are "Status Flags", "SMART
      Warnings", "Temperature", "Percentage Drive Life Used", "Vendor ID", and
      "Serial Number".
   4. The data will be set to the properties in D-bus.

This service will run automatically and look up NVMe drives every second.

##### Fault LED

When the value obtained from the command corresponds to one of the warning
types, it will trigger the fault LED of corresponding device and issue events.

##### Add SEL related to NVMe

The events `TemperatureFault`, `BackupdrivesFault`, `CapacityFault`,
`DegradesFault` and `MediaFault` will be generated for the NVMe errors.

- Temperature Fault log : when the property `TemperatureFault` set to true
- Backupdrives Fault log : when the property `BackupdrivesFault` set to true
- Capacity Fault log : when the property `CapacityFault` set to true
- Degrades Fault log : when the property `DegradesFault` set to true
- Media Fault log: when the property `MediaFault` set to true

#### Alternatives Considered

NVMe-MI specification defines multiple commands that can communicate with NVMe
drives over MCTP protocol. The NVMe-MI over MCTP has the following key
capabilities:

- Discover drives that are present and learn capabilities of each drives.
- Store data about the host environment enabling a Management Controller to
  query the data later.
- A standard format for VPD and defined mechanisms to read/write VPD contents.
- Inventorying, configuring and monitoring.

For monitoring NVMe drives, using NVM Express Basic Management Command over
SMBus directly is much simpler than NVMe-MI over MCTP protocol.

#### Impacts

This application is monitoring NVMe drives via SMbus and set values to D-bus.
The impacts should be small in the system.

#### Testing

This implementation is to use NVMe-MI-Basic command over SMBus and then set the
response data to D-bus. Testing will send SMBus command to the drives to get the
information and compare with the properties in D-bus to make sure they are the
same. The testing can be performed on different NVMe drives by different
manufacturers. For example: Intel P4500/P4600 and Micron 9200 Max/Pro.

Unit tests will test by function:

- It tests the length of responded data is as same as design in the function of
  getting NVMe information.
- It tests the function of setting values to D-bus is as same as design.
- It tests the function of turn the corresponding LED ON/OFF by different
  Smartwarnings values.
