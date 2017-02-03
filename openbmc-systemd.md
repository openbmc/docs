OpenBMC & Systemd
===================
OpenBMC uses [systemd](https://www.freedesktop.org/wiki/Software/systemd/) for
all processes control.  It has its own set of target and service units to
control which process's are started.  There is a lot of documentation on systemd
and to do OpenBMC state work, you're going to have to read some of it.  Here's
the highlights:

[Unit](https://www.freedesktop.org/software/systemd/man/systemd.unit.html#) -
Units are the basic framework of all systemd work.
[Service](https://www.freedesktop.org/software/systemd/man/systemd.service.html) -
Services are a type of unit, that define the process's to be run and execute.
[Target](https://www.freedesktop.org/software/systemd/man/systemd.target.html) -
Targets are another type of unit, they have two purposes:

1. Define synchronization points among services.
2. Define the services that get run for a given target.

On an OpenBMC system, you can go to /lib/systemd/system/ and see all of the
systemd units on the system.  You can easily cat these files to start looking at
the relationships among them.  Service files can also be found in
/etc/systemd/system and /run/systemd/system as well.

[systemctl](https://www.freedesktop.org/software/systemd/man/systemctl.html) is
the main tool you use to interact with systemd and its units.

----------
## Initial Power
When an OpenBMC system first has power applied, it starts the "default.target"
unless an alternate target is specified on the kernel command line.  In
Phosphor OpenBMC, there is a link from default.target to multi-user.target.
Within Phosphor OpenBMC, obmc-standby.target is wanted by multi-user.target and
is where you'll find all Phosphor services associated.

## Server Power On
When OpenBMC is used within a server, the [obmc-chassis-start@.target](https://github.com/openbmc/openbmc/blob/171031d20c7ed03900739d51ba53ad0001f98fa5/meta-phosphor/common/recipes-core/systemd/obmc-targets/obmc-chassis-start%40.target)
is what drives the boot of the system.

If you dig into its .wants relationship, you'll see the following in the file
system

```
ls -1 /lib/systemd/system/obmc-chassis-start@0.target.wants/
obmc-power-chassis-on@0.target
obmc-start-host@0.service
...
```

You can see we have another target in here, obmc-power-chassis-on@0.target,
along with some services that will all be started by systemd when you do a
"systemctl start obmc-chassis-start@0.target".

The target has corresponding services associated with it:
```
ls -1 /lib/systemd/system/obmc-power-chassis-on\@0.target.wants/
op-power-start@0.service
op-wait-power-on@0.service
```
So basically, if you run `systemctl start obmc-chassis-start@0.target` then
systemd will start execution of all associated services.

The services have dependencies within them that control the execution of each
service (for example, the op-power-start.service will run prior to the
op-wait-power-on.service).  These dependencies are set using targets and the
Wants,Before,After keywords.

## Server Power Off
The server power off function is encapsulated in the obmc-chassis-stop@0.target.
You can do a similar exercise with the power on to see its dependent services.

## Systemd Control in OpenBMC
There are a collection of services within OpenBMC that interact with systemd and
its unit files, providing somewhat of an abstraction layer from the user of the
OpenBMC system and systemd.  See the [state](https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/xyz/openbmc_project/State)
interfaces for more information on this function.

For example, if you wanted to execute the server power on function, you would do
the following:
 
> busctl set-property xyz.openbmc_project.State.Host /xyz/openbmc_project/state/host0
xyz.openbmc_project.State.Host RequestedHostTransition s
xyz.openbmc_project.State.Host.Transition.On

Underneath the covers, this is calling systemd with obmc-chassis-start@0.target

## Error Handling of Systemd
With great numbers of targets and services, come great chances for failures.
To make OpenBMC a robust and productive system, it needs to be sure to have an
error handling policy for when services and their targets fail.

When a fail happens, the OpenBMC software needs to notify the users of the
system and provide mechanisms for either the system to automatically retry the
failed operation (i.e. reboot the system) or to stay in a quiesced state so that
error data can be collected and the fail can be investigated.

There are 2 main fail scenarios when it comes to OpenBMC and systemd usage:

1. A service within a target fails
- If the service is a "oneshot" type, and the service is required
(not wanted) by the target then the target will fail if the service
fails
    - Define a behavior for when the target fails using the
    "OnFailure" option (i.e. go to a new failure target if any required
    service fails)
- If the service is not a "oneshot", then it can not fail the target
(the target only knows that it started successfully)
    - Define a behavior for when the service fails (OnFailure)
    option.
    - The service can not have "RemainAfterExit=yes" otherwise, the OnFailure
    action does not occur until the service is stopped (instead of when it
    fails)
        - *See more information below on [RemainAfterExit](#RemainAfterExit)

2. A failure outside of a normal systemd target/service (host watchdog expires,
host checkstop detected)
- The service which detects this failure is responsible for logging the
appropriate error, and instructing systemd to go to the appropriate target

Within OpenBMC, there is a host quiesce target.  This is the target that other
host related targets should go to when they hit a failure. Other software within
OpenBMC can then monitor for the entry into this quiesce target and will handle
the halt vs. automatic reboot functionality.

Targets which are not host related, will need special thought in regards to
their error handling.  For example, the target responsible for applying chassis
power, obmc-power-chassis-on@0.target, will have a
"OnFailure=obmc-power-chassis-off@%i.target" error path.  That is, if the
chassis power on target fails then power off the chassis.

The above info sets up some general **guidelines** for our host related
targets and services:

- All targets should have an "OnFailure=obmc-quiesce-host@.target"
- All services which are required for a target to achieve its function should
be RequiredBy that target (not WantedBy)
- All services should first try to be "Type=oneshot" so that we can just rely on
the target fail path
- If a service can not be "Type=oneshot", then it needs to have a
"OnFailure=obmc-quiesce-host@.target" and ideally set "RemainAfterExit=no"
(but see caveats on this below)
- If a service can not be any of these then it's up to the service application
to call systemd with the obmc-quiesce-host@.target on failures

### RemainAfterExit
This is set to "yes" for most OpenBMC services to handle the situation where
someone starts the same target twice.   If the associated service with that
target is not running (i.e. RemainAfterExit=no), then the service will be
executed again.  Think about someone accidentally running the
obmc-chassis-start@.target twice.  If you execute it when the operating system
is up and running, and the service which toggles the pgood pin is re-executed,
you're going to crash your system.  Given this info, the goal should always be
to write "oneshot" services that have RemainAfterExit set to yes.
