# IPMB based SDR Design

Author:
  Jayashree D (Jayashree), [jayashree-d@hcl](mailto:jayashree-d@hcl.com)
  Velumani T (Velu),  [velumanit@hcl](mailto:velumanit@hcl.com)

Other contributors: None

Created: July 12, 2022

## Problem Description

SDR (Sensor Data Records) is a data record that provides platform management
sensor type, locations, event generation and access information. A data
records that contain information about the type and the number of sensors in
the platform, sensor threshold support, event generation capabilities and
information on what type of readings the sensor provides.

The primary purpose of the Sensor Data Records is to describe the sensor
configuration of the platform management subsystem to system software. Sensor
Data Records also include records describing the number and type of devices
that are connected to the system’s IPMB, records that describe the location
and type of FRU Devices (devices that contain field replaceable unit
information).

The current version of OpenBMC does not have support for SDR design. This
design will provide the support on how SDR information in each IPMB device
is received using IPMB support.

## Background and References

This design is completely based on the IPMI (Intelligent Platform Management
Interface) Specification Second Generation v2.0.

Entity manager supports SDR sensor statically by configuring the sensor details
in json file. This implementation supports SDR dynamically by detecting the
number of IPMB FRU present.

## Requirements

 - Entity manager configuration to support the number of IPMB devices.

 - Dbus-sensors (IpmbSensor) daemon to support SDR sensor information.

## Proposed Design

The below diagram represents the overview for proposed SDR Design.

```

      ENTITY-MANAGER               DBUS-SENSORS
                                   (IpmbSensor)

                               +-------------------+
                               | +---------------+ |  IPMB 1  +---------------+
                               | | SDR Repository| |--------->| IPMB Device 1 |
   +------------------+        | |  Information  | |          +---------------+
   |                  |        | +---------------+ |
   |   Configuration  |        |                   |  IPMB 2  +---------------+
   |      file to     |        | +---------------+ |--------->| IPMB Device 2 |
   |   identify the   |------->| |  Reserve SDR  | |          +---------------+
   |     number of    |        | |  Repository   | |             ....
   |   IPMB Devices   |        | +---------------+ |             ....
   |                  |        |                   |             ....
   +------------------+        | +---------------+ |  IPMB N  +---------------+
                               | |  Get SDR Data | |--------->| IPMB Device N |
                               | +---------------+ |          +---------------+
                               +-------------------+

```

This document proposes a design for IPMB based SDR implementation.

Following modules will be updated for this implementation.

 - Entity-Manager

 - Dbus-Sensors

**Entity-Manager**

 - A configuration file is created with the information required to implement the
   code in dbus-sensors daemon. ***Bus*** will represent the IPMB channel in which
   SDR information will be read. Other parameters like ***Name, Class, Address, Type***
   represents the basic information for SDR.

***Example Configuration***

```
        {
            "Address": "xxx",
            "Bus": "$bus",
            "Class": "IpmbDevice",
            "PowerState": "Always",
            "Name": "$bus + 1 IpmbDevice",
            "Type": "IPMBDevice"
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
              | |-/xyz/openbmc_project/inventory/system/board/<ProbeName>/1_IpmbDevice
                               ............
                               ............
              |-/xyz/openbmc_project/inventory/system/board/<ProbeName>
              | |-/xyz/openbmc_project/inventory/system/board/<ProbeName>/N_IpmbDevice

```

**Dbus-Sensors (IpmbSensor)**

 - After the interface is created for IPMBDevice in entity-manager, dbus-signal
   will be notified to IpmbSensor. For each config, based on the bus number for
   each IPMB device, SDR Repository information will be received using IPMB.

 - This information will determine the sensor population records and the
   capabilities. The Reserve SDR Repository command is provided to prevent
   receiving incorrect data when doing incremental reads.

 - IpmbSDRDevice will retrieve the full set of SDR Records starting with 0000h
   as the Record ID to get the first record. The Next Record ID is extracted
   from the response and this is then used as the Record ID in a Get SDR
   request to get the next record. This is repeated until the record count
   value is matched.

 - Once all the sensor information like sensor name, sensor type, sensor unit,
   threshold values and sensor ID are read, each data will be processed and
   stored in a map for each IPMB device.

 - For each interface (xyz.openbmc_project.Configuration.IpmbDevice) detected for
   IPMB device, all the sensors related to that IPMB device will be accessed from
   the map which are stored. Each sensor's name, device address, threshold values
   and unit are updated and displayed one at a time by invoking IpmbSensor
   constructor under IpmbSensor service.

 - Added a new IpmbType for SDR sensor to frame the data packet for each sensor
   based on the sensor's ID received. Sensor value will be read based on the
   polling rate and it will be updated in dbus property.

***Example***

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

This design does not have any impacts on the existing changes as the code is
implemented based on the IPMB Devices detected in entity-manager.

## Testing
The proposed design is tested in a IPMB based Twinlake platform.
