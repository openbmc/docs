# OpenBMC health statistics design guide

Author: Piotr Matuszczak <piotr.matuszczak@intel.com>

## 1. Requirements
Monitoring of BMC utilization statistics along with BIOS POST codes and host
power events. The following metrics shall be supported:

* BMC CPU usage
* BMC RAM usage
* BMC flash usage
* BIOS POST codes
* Host power events

The metrics shall be available over the Redfish API and it shall be possible to
use them in the metric reports. The first three metrics will be referred to as
BMC health metrics, while latter two are host metrics.

## 2. BMC health metrics
The BMC health metrics can be obtained using standard Linux mechanisms. Those
metrics can be collected by single application and exposed as D-Bus sensors.
This way these metrics will be easily consumable by the [Monitoring Service][1],
which is a part of the OpenBMC telemetry and bmcweb, which implements OpenBMC
web server and Redfish interface.

### 2.1 CPU usage statistics
In order to obtain CPU usage statistics the Linux procfs shall be used. Reading
the /proc/stat file provides statistical data about all CPUs in the system. 

Typical /proc/stat output for single processor system:

```ascii
cat /proc/stat

cpu  15175 0 10166 1372706 2493 0 336 0 0 0
cpu0 15175 0 10166 1372706 2493 0 336 0 0 0
intr 2126992 0 19450 0 0 0 0 0 52 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1400876 0 0 0 0 0 0 0 0 0 555862 0 0 0 0 0 0 0 0 0 0 0 100502 0 0 50250 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
ctxt 2064932
btime 1590052729
processes 84635
procs_running 1
procs_blocked 0
softirq 2048717 0 1400876 1 0 0 0 554926 0 0 92914
```

The ```cpu``` line provides aggregated statistics for all CPUs, while
```cpuX```lines provide statistics for each CPU in the system. The meaning of
particular columns is the time spent by the CPU running different kind of
workload. Time is calculated in USER_HZ units or jiffies. The meaning of
particular columns is shown below.

| column | data from the example | workload                                 |
|--------|-----------------------|------------------------------------------|
| 1st    | 15175                 | normal processes executing in user mode  |
| 2nd    | 0                     | niced processes executing in user mode   |
| 3rd    | 10166                 | processes executing in kernel mode       |
| 4th    | 1372706               | idle mode                                |
| 5th    | 2493                  | waiting for I/O to complete              |
| 6th    | 0                     | servicing interrupts                     |
| 7th    | 336                   | servicing softirqs                       |
| 8th    | 0                     | time stolen by other OSes running in virtual environment |
| 9th    | 0                     | time spent for running a virtual CPU for guest OS under the control of the kernel |
| 10th   | 0                     | time spent running a niced virtual CPU for guest OS under the control of the kernel |

Having these data, CPU utilization can be easily calculated. To calculate
CPU utilization just divide the sum of all columns except 4th by the sum of all
columns. In the following example it would be:

```ascii
[(15175 + 0 + 10166 + 2493 + 0 + 336 + 0 + 0 + 0) / (15175 + 0 + 10166 + 1372706 + 2493 + 0 + 336 + 0 + 0 + 0)] * 100% = 2.01%
```

### 2.2 Memory usage statistics
To obtain memory usage /proc/meminfo file shall be read. The typical output is shown below.

```ascii
cat /proc/meminfo

MemTotal:         331392 kB
MemFree:          152376 kB
Buffers:           33852 kB
Cached:            57512 kB
SwapCached:            0 kB
Active:            71356 kB
Inactive:          59788 kB
Active(anon):      39864 kB
Inactive(anon):     1088 kB
Active(file):      31492 kB
Inactive(file):    58700 kB
Unevictable:           0 kB
Mlocked:               0 kB
HighTotal:             0 kB
HighFree:              0 kB
LowTotal:         331392 kB
LowFree:          152376 kB
SwapTotal:             0 kB
SwapFree:              0 kB
Dirty:                 0 kB
Writeback:             0 kB
AnonPages:         39792 kB
Mapped:            18960 kB
Shmem:              1176 kB
Slab:              43152 kB
SReclaimable:      39708 kB
SUnreclaim:         3444 kB
KernelStack:        1248 kB
PageTables:         1440 kB
NFS_Unstable:          0 kB
Bounce:                0 kB
WritebackTmp:          0 kB
CommitLimit:      165696 kB
Committed_AS:     222864 kB
VmallocTotal:     565248 kB
VmallocUsed:       11556 kB
VmallocChunk:     304092 kB
```

In order to return memory usage in percent, simple calculation shall be performed:

```ascii
MemoryUsage[%] = ((MemTotal - MemFree) / MemTotal) * 100[%] = ((331392 - 142376) / 331392) * 100 = 57.04 %
```

### 2.3 Disk usage statistics
The easiest way to obtain disk usage statistics is to use the df program, which
is a part of coreutils. The df returns data about storage usage, which can be
redirected to other program. The typical df output is shown below.

```ascii
df 

Filesystem     1K-blocks    Used Available Use% Mounted on
rootfs           7430440 2591504   4461544  37% /
/dev/root        7430440 2591504   4461544  37% /
devtmpfs          165612       0    165612   0% /dev
tmpfs              33140     156     32984   1% /run
tmpfs               5120       0      5120   0% /run/lock
tmpfs              66260       0     66260   0% /run/shm
/dev/mmcblk0p1     16334    8626      7708  53% /boot
```

This program directly returns usage in percent along with used and available
disk space. To expose the disk usage data as metric, another application is
required that will run df periodically capturing its output and exposing the
disk usage data as D-Bus sensors. 

## 3. Platform metrics
Platform metrics are the BIOS POST codes and host power events. 

### 3.1 BIOS POST codes
The BIOS POST codes are collected by the OpenBMC and exposed over the D-Bus
by the [POST code manager application][5]. 
The [D-Bus interface for POST codes][6] is described below.

| Name | Type | Signature | Result/Value | Flags |
|------|------|-----------|--------------|-------|
|```xyz.openbmc_project.State.Boot.PostCode``` | interface | - | - | - |
|```.GetPostCodes```                           | method | u | au | - |
|```.GetPostCodesWithTimeStamp```              | method | u | a(uu) | - |
|```.CurrentBootCycleCount```                  | property | u | - | - |
|```.MaxBootCycleNum```                        | property | u | 100 | - |

Both methods, the ```.GetPostCodes``` and ```.GetPostCodesWithTimeStamp``` take
the index of boot cycle as an argument. Both return list of BIOS POST codes,
but the ```.GetPostCodesWithTimeStamp``` returns list of them with timestamps
in microseconds since epoch (UNIX time). 

There is already [design that exposes BIOS POST codes in the Redfish tree][7],
but it is done in event-like form. This would be the recommended way, because
OpenBMC does not support discrete sensors. The additional difficulty is that
the  ```xyz.openbmc_project.State.Boot.PostCode``` D-Bus interface returns
stored POST codes in a form of array and does not produce readings as normal
sensors.

### 3.2 Host power events
Host power state is exposed on D-Bus by the [x86-power-control application][7].
This reflects the current host power state and requested transitions. The
[D-Bus interface for host state][8] is described below. 

| Name | Type | Signature | Result/Value | Flags |
|------|------|-----------|--------------|-------|
|```xyz.openbmc_project.State.Host``` | interface | - | - | - |
|```.CurrentHostState```              | property | s | "Off" - Host firmware is not running<br>"Running" - Host firmware is running<br>"Quiesced" - Host firmware is quiesced<br>"DiagnosticMode" - Host firmware is capturing debug information. | - |
|```.RequestedHostTransition```       | property | s | "Off" - Host firmware should be off<br>"On" - Host firmware should be on<br>"Reboot" - Host firmware should be rebooted. Chassis power will be cycled from off to on during this reboot<br>"GracefulWarmReboot" - Host firmware be will notified to shutdown and once complete, the host firmware will be rebooted. Chassis power will remain on throughout the reboot<br>"ForceWarmReboot" - Host firmware will be rebooted without notification and chassis power will remain on throughout the reboot | - |
|```.RestartCause```                  | property | s | "Unknown" - Reason Unknown<br>"RemoteCommand" - Remote command issued<br>"ResetButton" - Reset button pressed<br>"PowerButton" - Power button pressed<br>"WatchdogTimer" - Watchdog Timer expired<br>"PowerPolicyAlwaysOn" - Power Policy Host Always on<br>"PowerPolicyPreviousState" - Power Policy Previous State of Host<br>"SoftReset" - Soft reset of Host | - |

## 4. Architecture
To unify access to these metrics and make them available over the Redfish and
the Telemetry Service, they shall be exposed on D-Bus using [D-Bus sensors API][2].
Note, that D-Bus sensors do support only the following sensor types described in
the [sensor interface][3].

| Sensor type | correlated unit |
|-------------|-----------------|
| temperature | DegreesC        |
| fan_tach    | RPMS            |
| voltage     | Volts           |
| altitude    | Meters          |
| current     | Amperes         |
| power       | Watts           |
| energy      | Joules          |


Those types influences the sensor D-Bus path 
(for example: ```/xyz/openbmc_project/sensors/temperature/ambient```) and on
the metric units. Since BMC health metrics return utilization, a new type
shall be introduced, called ```utilization```. The proposal is that this
sensor type shall have unit ```Percentage``` and sensors may be exposed on
D-Bus using the following paths:

| Sensor          |  D-Bus path                                                |
|-----------------|------------------------------------------------------------|
| BMC CPU usage   | ```/xyz/openbmc_project/sensors/utilization/bmc_cpu```     |
| BMC RAM usage   | ```/xyz/openbmc_project/sensors/utilization/bmc_ram```     |
| BMC flash usage | ```/xyz/openbmc_project/sensors/utilization/bmc_storage``` |

For host power events and BIOS POST codes design is TBD.

The sensors architecture is shown on the diagram below. All sensors are exposed
on D-Bus using uniform [D-Bus sensors API][2]. In such form, they can be
consumed by the Monitoring Service, which is used for metric reports and
triggers management and by the bmcweb, to expose them as metrics in the
Redfish tree.

```ascii
 +-------------+ +---------------+ +-------------+
 |             | |               | |             |
 | /proc/stat  | | /proc/meminfo | |     df      |
 |             | |               | |             |
 +------+------+ +-------+-------+ +------+------+
        |                |                |
        |                |                |
        v                v                v
 +------+------+ +-------+-------+ +------+------+ +-----------+ +------------+
 |             | |               | |             | |           | |            |
 |   BMC CPU   | |  BMC memory   | | BMC storage | | BIOS POST | | Host power |
 | utilization | |  utilization  | | utilization | |   codes   | |   event    |
 |   sensor    | |    sensor     | |   sensor    | |   sensor  | |   sensor   |
 |             | |               | |             | |           | |            |
 +------+------+ +-------+-------+ +------+------+ +-----+-----+ +-----+------+
        |                |                |              |             |
        |                |                |              |             |
        v                v                v              v             v
<-------+-------+--------+-------D-Bus----+--------------++------------+------->
                ^                                         ^
                |                                         |
                v                                         |
 +--------------+---------------+       +-----------------|-------------------+
 |                              |       |bmcweb           v                   |
 |                              |       |        +--------+--------+          |
 |                              |       |        |                 |          |
 |      Monitoring Service      |       |        |     Redfish     |          |
 |                              |       |        |                 |          |
 |                              |       |        +-----------------+          |
 |                              |       |                                     |
 +------------------------------+       +-------------------------------------+
```

Those metrics shall be exposed in Redfish tree in order to be able to add them
to the metric reports. The BMC health metrics may be placed in the Manager
schema.

```json
{
    "@odata.type": "#Manager.v1_7_0.Manager",
    "Id": "BMC",
    "Name": "Manager",
    "ManagerType": "BMC",
    "Description": "Contoso BMC",
    "ServiceEntryPointUUID": "92384634-2938-2342-8820-489239905423",
    "UUID": "58893887-8974-2487-2389-841168418919",
    "Model": "Joo Janta 200",
    "FirmwareVersion": "4.4.6521",
    "DateTime": "2015-03-13T04:14:33+06:00",
    "DateTimeLocalOffset": "+06:00",
    "Status": {
        "State": "Enabled",
        "Health": "OK"
    },
    "PowerState": "On",
    "GraphicalConsole": {
        "ServiceEnabled": true,
        "MaxConcurrentSessions": 2,
        "ConnectTypesSupported": [
            "KVMIP"
        ]
    },
    "SerialConsole": {
        "ServiceEnabled": true,
        "MaxConcurrentSessions": 1,
        "ConnectTypesSupported": [
            "Telnet",
            "SSH",
            "IPMI"
        ]
    },
    "CommandShell": {
        "ServiceEnabled": true,
        "MaxConcurrentSessions": 4,
        "ConnectTypesSupported": [
            "Telnet",
            "SSH"
        ]
    },
    "HostInterfaces": {
        "@odata.id": "/redfish/v1/Managers/9/HostInterfaces"
    },
    "NetworkProtocol": {
        "@odata.id": "/redfish/v1/Managers/BMC/NetworkProtocol"
    },
    "EthernetInterfaces": {
        "@odata.id": "/redfish/v1/Managers/BMC/NICs"
    },
    "SerialInterfaces": {
        "@odata.id": "/redfish/v1/Managers/BMC/SerialInterfaces"
    },
    "LogServices": {
        "@odata.id": "/redfish/v1/Managers/BMC/LogServices"
    },
    "VirtualMedia": {
        "@odata.id": "/redfish/v1/Managers/BMC/VirtualMedia"
    },
    "Links": {
        "ManagerForServers": [
            {
                "@odata.id": "/redfish/v1/Systems/437XR1138R2"
            }
        ],
        "ManagerForChassis": [
            {
                "@odata.id": "/redfish/v1/Chassis/1U"
            }
        ],
        "ManagerInChassis": {
            "@odata.id": "/redfish/v1/Chassis/1U"
        },
        "Oem": {}
    },
    "Actions": {
        "#Manager.Reset": {
            "target": "/redfish/v1/Managers/BMC/Actions/Manager.Reset",
            "ResetType@Redfish.AllowableValues": [
                "ForceRestart",
                "GracefulRestart"
            ]
        },
        "Oem": {}
    },
    "Oem": {
        "Health metrics": {
            "BMC": {
                "CPU usage": 2.01,
                "Memory usage": 57.04,
                "Storage usage": 37.0
            }
        }
    },
    "@odata.id": "/redfish/v1/Managers/BMC"
}
```

The BMC metrics extension is proposed in the schema as OEM extension. There is
possibility to make this as a part of standard Redfish schema, but this requires
DMTF approval. 

The Redfish interface for accessing the BIOS POST codes are described in the
["Logging BIOS POST Codes Through Redfish"][4] document. 

[1]: https://github.com/openbmc/docs/blob/master/designs/telemetry.md
[2]: https://github.com/openbmc/docs/blob/master/architecture/sensor-architecture.md
[3]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Sensor/Value.interface.yaml
[4]: https://github.com/openbmc/docs/blob/master/designs/redfish-postcodes.md
[5]: https://github.com/openbmc/phosphor-post-code-manager
[6]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/State/Boot/PostCode.interface.yaml
[7]: https://github.com/openbmc/x86-power-control
[8]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/State/Host.interface.yaml