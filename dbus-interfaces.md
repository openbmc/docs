
This document aims to specify the dbus interfaces exported by OpenBMC objects.

# Host IPMI

## `/org/openbmc/HostIpmi`

signals:

  *  `ReceivedMessage(seq : byte, netfn : byte, lun : byte, cmd : byte, data: array[byte])`

methods:

  *  `sendMessage(seq : byte, netfn : byte, lun : byte, cmd : byte, cc : byte, data : array[byte])`
   
  *  `setAttention()`

# Chassis Control

## `/org/openbmc/control/chassis0`

signals:


methods:

  *  `(state : int) = getPowerState()`

  *  `powerOff()`

  *  `powerOn()`

  *  `softPowerOff()`

  *  `reboot()`
  
  *  `softReboot()`

  *  `setIdentify()`

  *  `clearIdentify()`

  *  `setPowerPolicy(policy : int)`

  *  `setDebugMode()`

properties:

  *  `reboot`


# Sensors

## `/org/openbmc/sensors/<type>/<name hierarchy>`

signals:

  *  `Warning(value : variant)`

  *  `Critical(value : variant)`

methods:

  *  `(value : variant) = getValue()`

  *  `setValue(value : variant)`

  *  `resetThresholdState()`

properties:

  *  `error`

  *  `units`

  *  `value`

  *  `critical_upper`

  *  `critical_lower`

  *  `warning_upper`

  *  `warning_lower`

  *  `threshold_state`

  *  `worst_threshold_state`

# Inventory

## `/org/openbmc/inventory/<item hierarchy>`

signals:


methods:

  *  `setPresent(present : string)`

  *  `setFault(fault : string)`

  *  `update(dict ( key : string, data : variant ) )`

properties:

  *  `fault`

  *  `fru_type`

  *  `is_fru`

  *  `present1`

  *  `version`


# Buttons

## `/org/openbmc/buttons/<button_name>`

signals:

  *  `Pressed`

  *  `PressedLong`

  *  `Released`

methods:

  *  `simPress`

# LEDs

## `/org/openbmc/control/leds/<led_name>`

methods:

  *  `setOn()`

  *  `setOff()`

  *  `setBlinkSlow()`

  *  `setBlinkFast()`

properties:

  *  `color`

  *  `state`

# BMC

## `/org/openbmc/control/bmc/bmc0`

methods:

  *  `cold_reset()`

