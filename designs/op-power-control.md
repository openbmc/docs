### OpenPower Power Control Refactoring

Author:
  Matt Spinler [mspinler]
Primary assignee:
  None
Other contributors:
  None
Created:
  10/10/2018

#### Problem Description

Code is needed to power on and off OpenPower systems.  This entails setting
one or more GPIOs on some sort of power sequencing device to start the power
on, and then monitoring a master PGOOD signal, also wired to a GPIO, for
completion.  In addition, that PGOOD needs to be monitored while
the system is on to be able to recognize when it drops unexpectedly.

OpenPower systems commonly have other GPIOs used for chip resets that
need to be set to a certain value after PGOOD is reached, and sometimes even
after the platform boot has progressed to a certain point.

While there is nothing necessarily unique to OpenPower systems being done here,
this design won't presume to know how other system architectures want to
power on.  If it turns out there is a lot commonality with other architectures,
maybe this can be made more common.

Any analysis of failures powering on or other PGOOD faults is left
to other applications.

#### Background and References

This will replace the power\_control.exe and pgood\_wait applications that
reside in skeleton/op-pwrctl.  The former monitored the PGOOD by polling,
and provided a setPowerState method to power on/off the system.  The latter
would simply wait until the PGOOD GPIO matchewd the value passed into it as
an argument and then return, in order to hold off the boot process.

#### Requirements

1) Monitor a master PGOOD GPIO
  * When it changes state, other applications may need to know, so make
    the PGOOD status available on D-Bus.

2) Power on and off the chassis.
  * This is done with GPIOs.  The GPIO wiring can be different on each system,
    so the code will need to use some sort of system specific data.
  * The power on/off targets need to be held off until the desired power
    operation is complete, so the design should be able to hold off other
    systemd units.
  * If the power doesn't turn on in a user defined amount of time, a nonzero
    needs to be returned so the service it runs in fails.
  * GPIOs used for resets may need to be set after PGOOD is reached, and
    then set to their opposite state when PGOOD is dropped.
  * Resets may also need to be set when the value of the BootProgress sensor
    that is set by hostboot changes state.
  * Other applications may need to know the state of the virtual power switch
    separately than the PGOOD, in order to catch error conditions,
    so make this available on D-Bus in the form of a switch property.

#### Proposed Design

This design creates 2 new applications - one to do the power on/off, and one
to monitor the PGOOD and handle any resets that need to be done around it.

##### openpower-pgood-monitor

This will be an always running application that monitors the PGOOD GPIO, sets
a property when it changes, and handles any resets that need to be set based
on the PGOOD state.  It will look up any GPIOs it needs in a system specific
JSON file.

It will not poll the PGOOD, but rather use the linux evdev library along with
sd\_event (using sdeventplus) to be told when the GPIO changes state.
When it first starts, it will read the PGOOD to know the initial state.

It will provide the following object on D-Bus:
`/xyz/openbmc_project/control/power0`

and implement the interface `xyz.openbmc_project.control.Power` with the
following properties:

| Property | Type | Description |
| -------- | ---- | ----------- |
| PGOOD | bool | Reflects PGOOD state (true = on) |
| Switch| bool | Reflects the state of the virtual power switch (true = on) |

Note that the D-BUS namespace is still in `xyz.openbmc_project` as there is
nothing OpenPower specific about the D-Bus details.

##### openpower-power-control

This will power on or off the system using GPIOs defined in a system specific
JSON file, and it won't return until either PGOOD is in the desired state or
there is a timeout waiting for it.

It will set the Switch property on `/xyz/openbmc_project/control/power0` to
reflect the state of the virtual power switch right after it sets the GPIOs
to change the power state.

##### GPIO Data

The data required is already defined per system, like [here][1].  It should
probably be moved moved out of the skeleton directory though.

#### Alternatives Considered

##### Use D-Bus methods to start power on/off

An always running daemon could provide methods to turn on and off the power.
These would either need to be long running D-Bus calls (several seconds),
or would need to do what the current skeleton code does and provide a new
executable just to do the waiting.  As they are only ever called from a
service file, I feel starting an application is more appropriate and also
guards against the case of preventing a power op when D-Bus or something
like the ObjectMapper stops working.

##### Provide a phosphor-power-control app and use a plugin design

One could put all of the system architecture specific code into a plugin
library and the main part could be generic to all architectures.  However,
there is so little code involved here pretty much everything would then
just reside in the plugins.

#### Impacts

There are only minor changes required from other applications from the
previous implementation.

* No more PowerGood/PowerLost signals - applications can just watch the PGOOD
  property instead.
* The service files to switch on/off power will call the application directly.
* D-Bus namespaces moving from `org.openbmc` to `xyz.openbmc\_project`.

#### Testing

Standard power operations and power fault injection can be used for testing.

[1]: https://github.com/openbmc/meta-ibm/blob/master/meta-romulus/recipes-phosphor/skeleton/obmc-libobmc-intf/gpio_defs.json
