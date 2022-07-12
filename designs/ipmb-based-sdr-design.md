# IPMB based SDR Design

Author: Jayashree D (Jayashree), [jayashree-d@hcl](mailto:jayashree-d@hcl.com)
Velumani T (Velu), [velumanit@hcl](mailto:velumanit@hcl.com)

Other contributors: None

Created: July 12, 2022

## Problem Description

SDR (Sensor Data Records) is a data record that provides platform management
sensor type, locations, event generation and access information. Data records
contain information about the type and the number of sensors in the platform,
sensor threshold support, event generation capabilities and information on what
type of readings the sensor provides.

The primary purpose of the Sensor Data Records is to describe the sensor
configuration of the platform management subsystem to system software. Sensor
Data Records also include records describing the number and type of devices that
are connected to the systems IPMB, records that describe the location and type
of FRU Devices (devices that contain field replaceable unit information).

This design will provide the support on how SDR information in each IPMB device
will be read using IPMB support and displayed in dbus paths.

## Background and References

This design is completely based on the IPMI (Intelligent Platform Management
Interface) Specification Second Generation v2.0.

The current version of OpenBMC does have support for SDR design through IPMI.
Entity manager supports sensor statically by configuring the sensor details in
json file. The proposed implementation supports SDR dynamically by detecting
IPMB FRU and getting SDR through IPMB commands.

## Requirements

- A json configuration file needs to be created in entity-manager daemon to
  detect the number of IPMB devices which supports SDR information by probing
  IPMB FRU interface.

- Dbus-sensors (IpmbSensor) daemon to support SDR sensor information such as
  sensor's name, type, thresholds, units and values.

## Proposed Design

The below diagram represents the overview for proposed SDR Design.

```

     ENTITY-MANAGER                     DBUS-SENSORS
                                        (IpmbSensor)

                               +------------------------------+
                               | +--------------------------+ |
                               | | 1. SDR Repository Info   | |
   +---------------+           | | 2. Reserve SDR Repository| |  IPMB 1  +---------------+
   |               | Config 1  | | 3. Get SDR Data          | |--------->| IPMB Device 1 |
   | Configuration |---------->| +--------------------------+ |          +---------------+
   |   file to     |  .....    |            .....             |
   | identify the  |  .....    |            .....             |              .......
   |   number of   |           |            .....             |              .......
   | IPMB Devices  | Config N  | +--------------------------+ |              .......
   |               |---------->| | 1. SDR Repository Info   | |
   +---------------+           | | 2. Reserve SDR Repository| |  IPMB N  +---------------+
                               | | 3. Get SDR Data          | |--------->| IPMB Device N |
                               | +--------------------------+ |          +---------------+
                               +------------------------------+

```

This document proposes a design for IPMB based SDR implementation.

Following modules will be updated for this implementation.

- Entity-Manager

- Dbus-Sensors

**Entity-Manager**

- A configuration file is created with the information required to implement the
  code in dbus-sensors daemon. **_Bus_** will represent the IPMB channel in
  which SDR information will be read. Other parameters like **_Name, Class,
  Address, Type_** represents the basic information for SDR.

**_Example Configuration_**

```
        {
            "Address": "xxx",
            "Bus": "$ipmbindex",
            "Class": "IpmbDevice",
            "PowerState": "Always",
            "Name": "$ipmbindex + 1 Twinlake Board",
            "Type": "IpmbDevice"
        }
```

- This configuration will probe the IPMB FRU interface. IPMB FRU will provide
  the FRU information for each IPMB devices. If the IPMB FRU interface
  (xyz.openbmc_project.Ipmb.FruDevice) is detected, then SDR config will be
  created for each IPMB device.

```

  "Probe": "xyz.openbmc_project.Ipmb.FruDevice({'PRODUCT_PRODUCT_NAME': 'Twin Lake '})"

```

- Based on the number of IPMB devices detected, dbus path and IPMBDevice
  interface for SDR configuration will be displayed under entity-manager.

```
   SERVICE        xyz.openbmc_project.EntityManager

   INTERFACE      xyz.openbmc_project.Configuration.IpmbDevice

   OBJECT PATH

     busctl tree xyz.openbmc_project.EntityManager
     `-/xyz
       `-/xyz/openbmc_project
         |-/xyz/openbmc_project/EntityManager
         `-/xyz/openbmc_project/inventory
          `-/xyz/openbmc_project/inventory/system
            `-/xyz/openbmc_project/inventory/system/board
              |-/xyz/openbmc_project/inventory/system/board/<ProbeName>
              | |-/xyz/openbmc_project/inventory/system/board/<ProbeName>/1_Twinlake_Board
                               ............
                               ............
              |-/xyz/openbmc_project/inventory/system/board/<ProbeName>
              | |-/xyz/openbmc_project/inventory/system/board/<ProbeName>/N_Twinlake_Board

```

**Dbus-Sensors (IpmbSensor)**

- After the interface is created for IPMBDevice in entity-manager, dbus-signal
  will be notified to IpmbSensor. Each SDR config will have its own object
  created based on the bus number for each IPMB device.

- SDR Repository information will be received using IPMB. This information will
  determine the sensor population records and the capabilities. The Reserve SDR
  Repository command is provided to prevent receiving incorrect data when doing
  incremental reads.

- IpmbSDRDevice will retrieve the full set of SDR Records starting with 0000h as
  the Record ID to get the first record. The Next Record ID is extracted from
  the response and this is then used as the Record ID in a Get SDR request to
  get the next record. This is repeated until the record count value is matched.

- All the sensor information like sensor name, sensor type, sensor unit,
  threshold values and sensor number are read for each sensor using Get SDR
  commands. After that, sensor information will be processed and stored in a map
  for each IPMB device.

- For each interface (xyz.openbmc_project.Configuration.IpmbDevice) detected for
  IPMB device, all the sensors related to that IPMB device will be accessed from
  the map which are stored. Each sensor's name, device address, threshold values
  and unit are updated. dbus objects and interfaces are created by invoking
  IpmbSensor constructor under IpmbSensor service.

- A new IpmbType for SDR sensor based on the entity configuration **_Class_** to
  define the IPMB send Request commands for each sensor based on the sensor's
  number received.

- Polling Rate is now set as default value as 1 second. Sensor value will be
  read based on the polling rate and it will be updated in dbus property.

**_Example_**

```
     busctl tree xyz.openbmc_project.IpmbSensor
     `-/xyz
       `-/xyz/openbmc_project
         `-/xyz/openbmc_project/sensors
           `-/xyz/openbmc_project/sensors/current
             `-/xyz/openbmc_project/sensors/current/sensor1
                         ............
                         ............
           `-/xyz/openbmc_project/sensors/power
             `-/xyz/openbmc_project/sensors/power/sensor1
                         ............
                         ............
           `-/xyz/openbmc_project/sensors/temperature
             `-/xyz/openbmc_project/sensors/temperature/sensor1
                         ............
                         ............
           `-/xyz/openbmc_project/sensors/voltage
             `-/xyz/openbmc_project/sensors/voltage/sensor1
```

## Impacts

The proposed design have no external API impact, since it uses existing dbus
objects. There is no performance impact in IpmbSensor, since the process is
handled by polling rate for each sensor.

This is a new documentation for SDR information in IpmbSensor (Dbus-sensors),
therefore, there is no documentation impact.

## Testing

The proposed design is tested in a IPMB based Twinlake platform.

**_Manual Test_** - This sensors will be tested using busctl commands in the
hardware.

**_Redfish Test_** - The sensors will be displayed under /redfish/v1/Chassis for
each Twinlake board using curl commands.

**_Robot Test_** - These sensors will be tested under robotframework using REST
API in the hardware sensors suite.
