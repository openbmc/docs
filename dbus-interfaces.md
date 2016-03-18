
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

### `/org/openbmc/sensors/host/PowerCap`
This object is used to set Host PowerCap. In turn a OCC "Set User PowerCap" command is sent to OCC.

### `/org/openbmc/sensors/host/OccStatus`
This object can set OCC state to either: "Enabled" or "Disabled".

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

# Event Logs

## `/org/openbmc/records/events`

methods:

  *  `Clear()`

## `/org/openbmc/records/events/<logid>`

methods:

  *  `delete()`

properties:

  *  `message`

  *  `association`

  *  `reported_by`

  *  `severity`

  *  `debug_data`

  *  `time`

# Associations
OpenBMC uses associations to extend the DBUS API without impacting existing objects and interfaces.
## `org.openbmc.Associations`
A object wishing to create an association implements this interface.

methods:
* None

properties:
* `associations signature=a(sss):` An array of forward, reverse, endpoint tuples where:
 * fowrard - the type of the association to create
 * reverse - the type of the association to create for the endpoint
 * endpoint - the endpoint of the association

For example, given an object: `/org/openbmc/events/1`
that implements `org.openbmc.Associations` and sets the `associations` property to:

`associations: [`

`    ["events", "frus", "/org/openbmc/piece_of_hardware"],`

`    ["events", "times", "/org/openbmc/timestamps/1"]`

`]`

would result in the following associations:

* /org/openbmc/events/1/frus
* /org/openbmc/events/1/times
* /org/openbmc/piece_of_hardware/events
* /org/openbmc/timestamps/1/events

It is up to the specific OpenBMC implementation to decide, what, if anything to do with these.
For example, the Phosphor mapper application looks for objects that implement this interface
and creates objects in its /org/openbmc DBUS namespace.

## `org.openbmc.Association`
OpenBMC implementations use this interface to insert associations into the DBUS namespace.

methods:
* None

properties:
* `endpoints signature=as:` An array of association endpoints.  
		   
For example, given:

/org/openbmc/events/1/frus:

`endpoints: [`

`    ["/org/openbmc/hardware/cpu0"],`

`    ["/org/openbmc/hardware/cpu1"],`

`]`
   
Denotes the following:
* /org/openbmc/events/1 => fru => /org/openbmc/hardware/cpu0
* /org/openbmc/events/1 => fru => /org/openbmc/hardware/cpu1
