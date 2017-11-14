OpenBMC D-Bus API
================

***WARNING*** **DEPRECATED API** The /org/openbmc API is deprecated and will *stop working* in a future OpenBMC release.

Please **note** that the v1.99.xx and greater code now has all of its
interfaces defined in [this](https://github.com/openbmc/phosphor-dbus-interfaces)
repository.  The below information in this document is for all v1.0.xx code.

This document aims to specify the OpenBMC D-Bus API.

The Phosphor distribution provides sample applications that implement off all
the interfaces and objects listed below.  Alternative or more feature complete
applications are possible by implementing parts of this D-Bus API.

OpenBMC typically adheres to D-Bus best practices and usage models; however, one
deviation is that OpenBMC places no requirements on well-known service names.
This allows developers to structure their object implementations in whatever
processes they choose.  In the standard D-Bus programming model, applications
connect to a service with a well-known name.  The well-known name is
associated with a fixed schema.  In OpenBMC, without any standardization of
well-known names, applications lose the knowledge of what applications provide
which objects.  To address this, the Phosphor distribution provides the
objectmapper service.  See the `org.openbmc.objectmapper.ObjectMapper`
interface below for more information.

The OpenBMC D-Bus API is still alpha.  If you have questions or suggestions
please [let the community know](https://lists.ozlabs.org/listinfo/openbmc).

`org.openbmc.HostIpmi`
----------------------
The HostIpmi interface allows applications to interact with host processor
firmware using IPMI.  Typically applications should use the higher level APIs
provided by `org.openbmc.HostServices` to interact with the host processor
firmware.

### methods
| name           | in signature | out signature | description                  |
| ------------   | ------------ | ------------- | ---------------------------- |
| `sendMessage`  | `yyyyyay`    | `x`           | **Send an IPMI message to the host processor firmware.**|
|                | `y`          |               | IPMI seq.                    |
|                | `y`          |               | IPMI netfn.                  |
|                | `y`          |               | IPMI lun.                    |
|                | `y`          |               | IPMI cmd.                    |
|                | `y`          |               | IPMI cc.                     |
|                | `ay`         |               | IPMI msg.                    |
|                |              | `x`           | The result status.           |
| `setAttention` | `void`       | `x`           | **Raise an SMS attention event.**|
|                |              | `x`           | The result status.           |

### signals
| name          | signature | description                                      |
| ------------- | --------- | ------------------------------------------------ |
| `ReceivedMessage` | `yyyyay`  | **An IPMI message was received from the host processor firmware.**|
|               | `y`       | IPMI seq.                                        |
|               | `y`       | IPMI netfn.                                      |
|               | `y`       | IPMI lun.                                        |
|               | `y`       | IPMI cmd.                                        |
|               | `ay`      | IPMI message.                                    |

### namespace
| path          | required    | description                                    |
| ------------- | ----------- | ---------------------------------------------- |
| `/org/openbmc/HostIpmi/<n>` | Yes on systems with the host-ipmi machine feature, otherwise no. | n: IPMI interface number.  If implemented, at least one IPMI interface must exist. |

`org.openbmc.HostServices`
------------------------
The HostServices interface allows applications to interact with the host
processor firmware.  Applications should use the high-level APIs provided here
in favor of `org.openbmc.HostIpmi` where applicable.

### methods
| name           | in signature | out signature | description                  |
| -------------- | ------------ | ------------- | ---------------------------- |
| `SoftPowerOff` | `void`       | `x`           | **Prepare the host processor firmware for an orderly shutdown.**|
|                |              | `x`           | The result status.           |

### namespace
| path                        | required                         | description |
| --------------------------- | -------------------------------- | ----------- |
| `/org/openbmc/HostServices` | Yes on systems with the host-ipmi machine feature, otherwise no. | ? |

`org.openbmc.InventoryItem`
---------------------------
The InventoryItem interface allows applications to examine and manipulate
information that relates to inventory items.

### methods
| name         | in signature | out signature | description                   |
| ------------ | ------------ | ------------- | ----------------------------- |
| `setPresent` | `s`          | `void`        | **Indicate the item is present.**|
|              | `s`          |               | true or false                 |
| `setFault`   | `s`          | `void`        | **Indicate the item is faulted.**|
|              | `s`          |               | true or false                 |
| `update`     | `a{sv}`      | `void`        | **Update the item attributes.**|
|              | `a{sv}`      |               | Key/value pair dictionary.    |

### properties
| name       | signature | description                             |
| ---------- | --------- | --------------------------------------- |
| `is_fru`   | `b`       | **The item is field replaceable.**      |
| `fru_type` | `s`       | **The type of the item.**               |
| `present`  | `s`       | **The item is physically present.**     |
| `fault`    | `s`       | **Whether or not the item is faulted.** |

### namespace
| path                            | required | description                     |
| ------------------------------- | -------- | ------------------------------- |
| `/org/openbmc/inventory/<item>` | No       | Inventory items must be instantiated in the inventory namespace.|

`org.openbmc.NetworkManager`
----------------------------
The NetworkManager interface allows applications to examine and manipulate
network settings.

### methods
| name             | in signature | out signature | description                |
| ---------------- | ------------ | ------------- | -------------------------- |
| `GetAddressType` | `s`          | `s`           | **Query the IP address type.**|
|                  | `s`          |               | The network device to query.|
|                  |              | `s`           | The address type for the network device.|
| `GetHwAddress`   | `s`          | `s`           | **Query the hardware address.**|
|                  | `s`          |               | The network device to query.|
|                  |              | `s`           | The hardware address for the network device.|
| `SetAddress4`    | `ssss`       | `x`           | **Set the IPV4 address.**  |
|                  | `s`          |               | The network device on which to set the ipv4 address.|
|                  | `s`          |               | The ipv4 address to set.   |
|                  | `s`          |               | The ipv4 mask to set.      |
|                  | `s`          |               | The ipv4 gateway to set    |
|                  |              | `x`           | The result status.         |
| `EnableDHCP`     | `s`          | `x`           | **Enable DHCP.**           |
|                  | `s`          |               | The network device on which to enable DHCP.|
|                  |              | `x`           | The result status.         |
| `SetHwAddress`   | `ss`         | `i`           | **Set the hardware address.**|
|                  | `s`          |               | The network device on which to set the hardware address.|
|                  | `s`          |               | The hardware address to set.|
|                  |              | `i`           | The result status.         |
| `GetAddress4`    | `s`          | `iyss`        | **Query the IPV4 address.**|
|                  | `s`          |               | The network device to query.|
|                  |              | `i`           | ?                          |
|                  |              | `y`           | ?                          |
|                  |              | `s`           | ?                          |
|                  |              | `s`           | ?                          |

### namespace
| path                                    | required             | description |
| --------------------------------------- | -------------------- | ----------- |
| `/org/openbmc/NetworkManager/Interface` | Yes on systems with the network machine feature, otherwise no. | ? |

`org.openbmc.Sensors`
---------------------
The Sensors interface allows applications to register a sensor instance with a
sensor management application.

### methods
| name       | in signature | out signature | description                      |
| ---------- | ------------ | ------------- | -------------------------------- |
| `register` | `ss`         | `void`        | **Register a sensor instance of type class.**|
|            | `s`          |               | The class name of the sensor.    |
|            | `s`          |               | The object path of the sensor.   |
| `delete`   | `s`          | `void`        | **Deregister a sensor instance.**|
|            | `s`          |               | The object path of the sensor.   |

### namespace
| path                   | required | description                              |
| ---------------------- | -------- | ---------------------------------------- |
| `/org/openbmc/sensors` | Yes      | The sensor manager must be instantiated here.|

`org.openbmc.HwmonSensor`
-------------------------
The HwmonSensor interface allows applications to query and manipulate hwmon
based sensors.

### methods
| name        | in signature | out signature | description |
| ----------- | ------------ | ------------- | ----------- |
| `setByPoll` | `v`          | `(bv)`        | **?**       |
|             | `v`          |               | ?           |
|             |              | `(bv)`        | ?           |

### properties
| name       | signature | description |
| ---------- | --------- | ----------- |
| `scale`    | `?`       | **?**       |
| `offset`   | `?`       | **?**       |
| `filename` | `?`       | **?**       |

### namespace
| path                                    | required | description             |
| --------------------------------------- | -------- | ----------------------- |
| `/org/openbmc/sensors/<class>/<sensor>` | No       | Any sensor instances must be instantiated in the sensors namespace.|

`org.openbmc.SensorThresholds`
------------------------------
The SensorThreshold interface allows applications to query and manipulate
sensors thresholds.

### methods
| name                  | in signature | out signature | description |
| --------------------- | ------------ | ------------- | ----------- |
| `resetThresholdState` | `void`       | `void`        | **?**       |

### signals
| name        | signature | description |
| ----------- | --------- | ----------- |
| `Emergency` | `void`    | **?**       |

### properties
| name                    | signature | description |
| ----------------------- | --------- | ----------- |
| `thresholds_enabled`    | `?`       | **?**       |
| `emergency_enabled`     | `?`       | **?**       |
| `warning_upper`         | `?`       | **?**       |
| `warning_lower`         | `?`       | **?**       |
| `critical_upper`        | `?`       | **?**       |
| `critical_lower`        | `?`       | **?**       |
| `threshold_state`       | `?`       | **?**       |
| `worst_threshold_state` | `?`       | **?**       |

### namespace
| path                                    | required | description             |
| --------------------------------------- | -------- | ----------------------- |
| `/org/openbmc/sensors/<class>/<sensor>` | No       | Any sensor instances must be instantiated in the sensors namespace.|

`org.openbmc.SensorValue`
-------------------------
The SensorValue interface allows applications to query and manipulate sensors
that only provide a value.

### methods
| name       | in signature | out signature | description               |
| ---------- | ------------ | ------------- | ------------------------- |
| `setValue` | `v`          | `void`        | **Set the sensor value.** |
|            | `v`          |               | The value to set.         |
| `getValue` | `void`       | `v`           | **Get the sensor value.** |
|            |              | `v`           | The sensor value.         |

### properties
| name    | signature | description                                            |
| ------- | --------- | ------------------------------------------------------ |
| `units` | `s`       | **The units associated with the sensor value.**        |
| `error` | `?`       | **?**                                                  |

### namespace
| path                                    | required | description             |
| --------------------------------------- | -------- | ----------------------- |
| `/org/openbmc/sensors/<class>/<sensor>` | No       | Any sensor instances must be instantiated in the sensors namespace.|

`org.openbmc.Button`
--------------------
The Button interface allows applications to query the state of buttons.

### methods
| name           | in signature | out signature | description                  |
| -------------- | ------------ | ------------- | ---------------------------- |
| `isOn`         | `void`       | `b`           | **Query the button state.**  |
|                |              | `b`           | The button state.            |
| `simPress`     | `void`       | `void`        | **Simulate a button press.** |
| `simLongPress` | `void`       | `void`        | **Simulate a long button press.**|

### signals
| name          | signature | description                          |
| ------------- | --------- | ------------------------------------ |
| `Released`    | `void`    | **The button was released.**         |
| `Pressed`     | `void`    | **The button was pressed.**          |
| `PressedLong` | `void`    | **The button was pressed and held.** |

### properties
| name    | signature | description                   |
| ------- | --------- | ----------------------------- |
| `state` | `b`       | **The current button state.** |

### namespace
| path                            | required | description                     |
| ------------------------------- | -------- | ------------------------------- |
| `/org/openbmc/buttons/<button>` | No       | Any button instances must be instantiated in the buttons namespace.|

`org.openbmc.control.Bmc`
-------------------------
The control.Bmc interface allows applications control the BMC.

### methods
| name        | in signature | out signature | description        |
| ----------- | ------------ | ------------- | ------------------ |
| `warmReset` | `void`       | `void`        | **Reset the BMC.** |

### namespace
| path                                 | required | description                |
| ------------------------------------ | -------- | -------------------------- |
| `/org/openbmc/control/bmc<instance>` | Yes      | Any BMC control instances must be instantiated in the control namespace.|

`org.openbmc.managers.Download`
-------------------------------
The managers.Download interface allows applications to receive file download
notifications.

### signals
| name               | signature | description                                 |
| ------------------ | --------- | ------------------------------------------- |
| `DownloadComplete` | `ss`      | **The file was downloaded successfully.**   |
|                    | `s`       | The file path in the local filesystem.      |
|                    | `s`       | The name of the file that was requested.    |
| `DownloadError`    | `s`       | **An error occurred downloading the file.** |
|                    | `s`       | The name of the file that was requested.    |

### namespace
| path                             | required | description |
| -------------------------------- | -------- | ----------- |
| `/org/openbmc/managers/Download` | ?        | ?           |

`org.openbmc.control.BmcFlash`
------------------------------
The control.BmcFlash interface allows applications update the BMC firmware.

### methods
| name                | in signature | out signature | description                 |
| ------------------- | ------------ | ------------- | --------------------------- |
| `updateViaTftp`     | `ss`         | `void`        | **Perform a BMC firmware update using a TFTP server.**|
|                     | `s`          |               | The ipv4 address of the TFTP server hosting the firmware image file.|
|                     | `s`          |               | The name of the file containing the BMC firmware image.|
| `update`            | `s`          | `void`        | **Perform a BMC firmware update with a file already on the BMC.**|
|                     | `s`          |               | The name of the file containing the BMC firmware image.|
| `PrepareForUpdate`  | `void`       | `void`        | **Reboot BMC with Flash content cached in RAM.**|
| `Abort`             | `void`       | `void`        | **Abort any pending, broken, or in-progress flash update.**|
| `Apply`             | `void`       | `void`        | **Initiate writing image to flash.**|
| `GetUpdateProgress` | `void`       | `s`           | **Display progress log `Apply` phase.**|
|                     |              | `s`           | The `status` and log output from `Apply`|

### signals
| name           | signature | description                              |
| -------------- | --------- | ---------------------------------------- |
| `TftpDownload` | `ss`      | **A request to download a file using TFTP occurred.**|
|                | `s`       | The ipv4 address of the TFTP server hosting the firmware image file.|
|                | `s`       | The name of the file containing the BMC firmware image.|

### properties
| name                           | signature | description                     |
| ------------------------------ | --------- | ------------------------------- |
| `status`                       | `s`       | **Description of the phase of the update.**        |
| `filename`                     | `s`       | **The name of the file containing the BMC firmware image.**|
| `preserve_network_settings`    | `b`       | **Perform a factory reset.**    |
| `restore_application_defaults` | `b`       | **Clear modified files in read-write filesystem.**    |
| `update_kernel_and_apps`       | `b`       | **Do not update bootloader (requires image pieces).**    |
| `clear_persistent_files`       | `b`       | **Also remove persistent files when updating read-write filesystem.**    |
| `auto_apply`                   | `b`       | **Attempt to apply image after unpacking (cleared if image verification fails).**    |

### namespace
| path                             | required | description |
| -------------------------------- | -------- | ----------- |
| `/org/openbmc/control/flash/bmc` | ?        | ?           |

`org.openbmc.control.Chassis`
-----------------------------
The control.Chassis interface allows applications to query and manipulate the
state of a Chassis.

### methods
| name            | in signature | out signature | description                 |
| --------------- | ------------ | ------------- | --------------------------- |
| `setIdentity`   | `void`       | `void`        | **Turn on the identification indicator.**|
| `clearIdentity` | `void`       | `void`        | **Turn off the identification indicator.**|
| `powerOn`       | `void`       | `void`        | **Power the chassis on.**   |
| `powerOff`      | `void`       | `void`        | **Power the chassis off immediately.**|
| `softPowerOff`  | `void`       | `void`        | **Perform a graceful shutdown of the chassis.**|
| `reboot`        | `void`       | `void`        | **Reboot the chassis immediately.**|
| `softReboot`    | `void`       | `void`        | **Perform a graceful reboot of the chassis.**|

### properties
| name   | signature | description           |
| ------ | --------- | --------------------- |
| `uuid` | `s`       | **The chassis UUID.** |

### namespace
| path                                     | required | description            |
| ---------------------------------------- | -------- | ---------------------- |
| `/org/openbmc/control/chassis<instance>` | ?        | Any chassis control instances must be instantiated in the control namespace.|

`org.openbmc.Flash`
-------------------
The Flash interface allows applications update the host firmware.

### methods
| name            | in signature | out signature | description                 |
| --------------- | ------------ | ------------- | --------------------------- |
| `update`        | `s`          | `void`        | **Update the host firmware.**|
|                 | `s`          |               | The file containing the host firmware image.|
| `error`         | `s`          | `void`        | **?**                       |
|                 | `s`          |               | The error message.          |
| `done`          | `void`       | `void`        | **?**                       |
| `init`          | `void`       | `void`        | **?**                       |
| `updateViaTftp` | `ss`         | `void`        | **Update the host firmware using a TFTP server.**|
|                 | `s`          |               | The TFTP server url.        |
|                 | `s`          |               | The file containing the host firmware image.|

### signals
| name       | signature | description                                  |
| ---------- | --------- | -------------------------------------------- |
| `Updated`  | `void`    | **?**                                        |
| `Download` | `ss`      | **?**                                        |
|            | `s`       | The TFTP server url.                         |
|            | `s`       | The file containing the host firmware image. |

### properties
| name               | signature | description     |
| ------------------ | --------- | --------------- |
| `filename`         | `s`       | **?**           |
| `flasher_path`     | `s`       | **?**           |
| `flasher_name`     | `s`       | **?**           |
| `flasher_instance` | `s`       | **?**           |
| `status`           | `s`       | **?**           |

### namespace
| path | required | description |
| ---- | -------- | ----------- |
| ?    | ?        | ?           |

`org.openbmc.SharedResource`
----------------------------
Insert description of the SharedResource interface here.

### methods
| name       | in signature | out signature | description                      |
| ---------- | ------------ | ------------- | -------------------------------- |
| `lock`     | `s`          | `void`        | **Lock the shared resource.**    |
|            | `s`          |               | The shared resource name.        |
| `unlock`   | `void`       | `void`        | **Unlock the shared resource.**  |
| `isLocked` | `void`       | `bs`          | **Get the lock state of the resource.**|
|            |              | `b`           | The lock state.                  |
|            |              | `s`           | The shared resource name.        |

### properties
| name   | signature | description                   |
| ------ | --------- | ----------------------------- |
| `lock` | `b`       | **The lock state.**           |
| `name` | `s`       | **The shared resource name.** |

`org.openbmc.control.Host`
--------------------------
The control.Host interface allows applications to manipulate the host processor
firmware.

### methods
| name       | in signature | out signature | description                      |
| ---------- | ------------ | ------------- | -------------------------------- |
| `boot`     | `void`       | `void`        | **Start the host processor firmware.**|
| `shutdown` | `void`       | `void`        | **Stop the host processor firmware.**|
| `reboot`   | `void`       | `void`        | **Restart the host processor firmware.**|

### signals
| name     | signature | description                              |
| -------- | --------- | ---------------------------------------- |
| `Booted` | `void`    | **The host processor firmware was started.** |

### properties
| name         | signature | description     |
| ------------ | --------- | --------------- |
| `debug_mode` | `i`       | **?**           |
| `flash_side` | `s`       | **?**           |

### namespace
| path                                  | required | description               |
| ------------------------------------- | -------- | ------------------------- |
| `/org/openbmc/control/host<instance>` | No       | Any host control instances must be instantiated in the control namespace.|

`org.openbmc.control.Power`
---------------------------
Insert a description of the control.Power interface here.

### methods
| name            | in signature | out signature | description                 |
| --------------- | ------------ | ------------- | --------------------------- |
| `setPowerState` | `i`          | `void`        | **Set the power state.**    |
|                 | `i`          |               | The state to enter.         |
| `getPowerState` | `void`       | `i`           | **Query the current power state.**|
|                 |              | `i`           | The current power state.    |

### signals
| name        | signature | description           |
| ----------- | --------- | --------------------- |
| `PowerGood` | `void`    | **The power is on.**  |
| `PowerLost` | `void`    | **The power is off.** |

### properties
| name            | signature | description     |
| --------------- | --------- | --------------- |
| `pgood`         | `i`     | **?**           |
| `state`         | `i`     | **?**           |
| `pgood_timeout` | `i`     | **?**           |

### namespace
| path                                   | required | description              |
| -------------------------------------- | -------- | ------------------------ |
| `/org/openbmc/control/power<instance>` | ?        | Any power control instances must be instantiated in the control namespace.|

`org.openbmc.Led`
---------------
Insert a description of the Led interface here.

### methods
| name           | in signature | out signature | description                |
| -------------- | ------------ | ------------- | -------------------------- |
| `setOn`        | `void`       | `void`        | **Turn the LED on.**       |
| `SetOff`       | `void`       | `void`        | **Turn the LED off.**      |
| `setBlinkSlow` | `void`       | `void`        | **Blink the LED slowly.**  |
| `setBlinkFast` | `void`       | `void`        | **Blink the LED quickly.** |

### properties
| name       | signature | description                |
| ---------- | --------- | -------------------------- |
| `color`    | `i`       | **The color of the LED.**  |
| `function` | `s`       | **?**                      |
| `state`    | `s`       | **The current LED state.** |

### namespace
| path                             | required | description                    |
| -------------------------------- | -------- | ------------------------------ |
| `/org/openbmc/control/led/<led>` | No       | Any LED instances must be instantiated in the control/led namespace.|

`org.openbmc.objectmapper.ObjectMapper`
---------------------------------------
The ObjectMapper interface enables applications to discover the D-Bus unique
connection name(s) for a given object path.

### methods
| name              | in signature | out signature | description               |
| ----------------- | ------------ | ------------- | ------------------------- |
| `GetObject`       | `s`          | `a{sas}`      | **Determine the D-Bus unique connection name(s) implementing a single object and the interfaces implemented by those services.**|
|                   | `s`          |               | The path of the object to query.|
|                   |              | `a{sas}`      | A dictionary with D-Bus unique connection names as keys, and interfaces as values.|
| `GetAncestors`    | `s`          | `a{sa{sas}}`  | **Determine the D-Bus unique connection name(s) implementing any ancestor objects and the interfaces implemented by those services.**|
|                   | `s`          |               | The point in the namespace from which to provide results.|
|                   |              | `a{sa{sas}}`  | A dictionary of dictionaries, with object paths as outer keys, D-Bus unique connection names as inner keys, and implemented interfaces as values.|
| `GetSubTree`      | `si`         | `a{sa{sas}}`  | **Determine the D-Bus unique connection name(s) implementing an entire subtree of objects in the D-Bus namespace.**|
|                   | `s`          |               | The point in the namespace from which to provide results.|
|                   | `i`          |               | The number of path elements to descend.|
|                   |              | `a{sa{sas}}`  | A dictionary of dictionaries, with object paths as outer keys, D-Bus unique connection names as inner keys, and interfaces implemented by those services as values.|
| `GetSubTreePaths` | `si`         | `as`          | **List all known D-Bus objects.**|
|                   | `s`          |               | The point in the namespace from which to provide results.|
|                   | `i`          |               | The number of path elements to descend.|
|                   |              | `as`          | An array of object paths. |

### namespace
| path                                     | required | description            |
| ---------------------------------------- | -------- | ---------------------- |
| `/org/openbmc/objectmapper/objectmapper` | Yes      | The object mapper must be instantiated here.|

`org.openbmc.recordlog`
-----------------------
Insert a description of the record log interface here.

### methods
| name                | in signature | out signature | description             |
| ------------------- | ------------ | ------------- | ----------------------- |
| `acceptHostMessage` | `sssay`      | `q`           | **Accept a message from the host processor firmware.**|
|                     | `s`          |               | The message content.    |
|                     | `s`          |               | The message severity.   |
|                     | `s`          |               | An association between the message and another entity.|
|                     | `ay`         |               | Development data associated with the message.|
|                     |              | `q`           | The created record ID.  |
| `clear`             | `void`       | `q`           | **Remove all record instances.**|
|                     |              | `q`           | ?                       |

### namespace
| path                           | required | description                      |
| ------------------------------ | -------- | -------------------------------- |
| `/org/openbmc/records/<class>` | No       | Any recordlog instances must be instantiated in the records namespace. |

`org.openbmc.record`
--------------------
Insert a description of the record interface here.

### properties
| name          | signature | description                                      |
| ------------- | --------- | ------------------------------------------------ |
| `message`     | `s`       | **A free from message.**                         |
| `severity`    | `s`       | **The record severity.**                         |
| `reported_by` | `s`       | **The originating entity of the record.**        |
| `time`        | `s`       | **The timestamp associated with the record.**    |
| `debug_data`  | `ay`      | **Development data associated with the record.** |

### namespace
| path                                    | required | description             |
| --------------------------------------- | -------- | ----------------------- |
| `/org/openbmc/records/<class>/<record>` | No       | Records must be instantiated in the records namespace.|

`org.openbmc.Object.Delete`
---------------------------
Applications that create objects that can be removed for any reason must
implement this interface.  Some common examples of this could be an event log
instance or a user account instance.

### methods
| name     | in signature | out signature | description                        |
| -------- | ------------ | ------------- | ---------------------------------- |
| `delete` | `void`       | `void`        | **Remove the object from the D-Bus namespace.**|

`org.openbmc.Associations`
--------------------------
Applications wishing to create an association between two or more objects
implement can this interface.  Associations exist to provide a stable but
extendable D-Bus API.

### properties
| name           | signature | description                              |
| -------------- | --------- | ---------------------------------------- |
| `associations` | `a(sss)`  | **An array of forward, reverse, endpoint tuples.**|
|                | `s`       | The type of association to create.       |
|                | `s`       | The type of association to create for the endpoint.|
|                | `s`       | The object path of the endpoint.         |

For example, given an object /org/openbmc/events/1 that implements
`org.openbmc.Associations` and then sets the associations property to:

```json
"associations": [
  ["events", "frus", "/org/openbmc/piece_of_hardware"],
  ["events", "times", "/org/openbmc/timestamps/1"]
]
```

would result in the following associations:

```shell
/org/openbmc/events/1/frus
/org/openbmc/events/1/times
/org/openbmc/piece_of_hardware/events
/org/openbmc/timestamps/1/events
```

`org.openbmc.Association`
-------------------------
Applications use this interface to inject associations into the D-Bus namespace.

### properties
| name        | signature | description                            |
| ----------- | --------- | -------------------------------------- |
| `endpoints` | `as`      | **An array of association endpoints.** |

For example, given:

```json
"/org/openbmc/events/1/frus": {
    "endpoints": [
        "/org/openbmc/hardware/cpu0",
        "/org/openbmc/hardware/cpu1",
    ]
}
```

Denotes the following:

```shell
/org/openbmc/events/1 => fru => /org/openbmc/hardware/cpu0
/org/openbmc/events/1 => fru => /org/openbmc/hardware/cpu1
```

`org.openbmc.settings.Host`
---------------------------
The settings.Host interface provides a basic settings repository for host
processor firmware settings.

### methods
Host settings are accessed using the standard
[`org.freedesktop.DBus.Properties`](
https://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-properties)
interface.

### signals
Applications are notified of host setting changes using the standard
[`org.freedesktop.DBus.ObjectManager`](
https://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-objectmanager)
interface.

### properties
Settings are accessed using the standard [`org.freedesktop.DBus.Properties`](
https://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-properties)
interface.

### namespace
| path                                   | required | description             |
| -------------------------------------- | -------- | ----------------------- |
| `/org/openbmc/settings/host<instance>` | No       | Any host settings instances must be instantiated in the settings namespace. |

`org.openbmc.Watchdog`
----------------------
The Watchdog interface enables health monitoring applications to offload timer
bookkeeping to another application.

### methods
| name    | in signature | out signature | description                    |
| ------- | ------------ | ------------- | ------------------------------ |
| `start` | `void`       | `void`        | **Start the countdown timer.** |
| `poke`  | `void`       | `void`        | **Ping the watchdog.**         |
| `stop`  | `void`       | `void`        | **Stop the countdown timer.**  |
| `set`   | `i`          | `void`        | **Set the timer interval.**    |
|         | `i`          |               | The timer interval.            |

### signals
| name            | signature | description                              |
| --------------- | --------- | ---------------------------------------- |
| `WatchdogError` | `void`    | *The watchdog was not pinged before the timer expired.**|

### namespace
| path                               | required | description                  |
| ---------------------------------- | -------- | ---------------------------------------- |
| `/org/openbmc/watchdog/<watchdog>` | No       | Any watchdog instances must be instantiated in the watchdog namespace. |
