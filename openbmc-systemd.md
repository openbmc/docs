OpenBMC & Systemd
===================
OpenBMC uses [systemd](https://www.freedesktop.org/wiki/Software/systemd/) for all process control.  It has its own set of target and service units to control which process's are started.  There is a lot of documentation on systemd and to do openbmc state work, you're going to have to read some of it.  Here's the highlights:

[Unit](https://www.freedesktop.org/software/systemd/man/systemd.unit.html#) - Units are the basic framework of all systemd work.
[Service](https://www.freedesktop.org/software/systemd/man/systemd.service.html) - Services are a type of unit, that define the process's to be run and execute.
[Target](https://www.freedesktop.org/software/systemd/man/systemd.target.html) - Targets are another type of unit, they have two purposes:

1. Define synchronization points among services.
2. Define the services that get run for a given target.

On an openbmc system, you can go to /lib/systemd/system/ and see all of the systemd units on the system.  You can easily cat these files to start looking at the relationships among them.

systemctl is the main tool you use to interact with systemd and its units.

----------
## Initial Power
When an openbmc system first has power applied, it starts the default obmc-standby.target.  This target (and its dependency targets) will activate all required services.

## Server Power On
When openbmc is used within a server, the [obmc-chassis-start@.target](https://github.com/openbmc/openbmc/blob/171031d20c7ed03900739d51ba53ad0001f98fa5/meta-phosphor/common/recipes-core/systemd/obmc-targets/obmc-chassis-start%40.target) is what drives the boot of the system.

If you dig into its .wants relationship, you'll see the following in the file system

```
ls -1 /lib/systemd/system/obmc-chassis-start@0.target.wants/
obmc-power-chassis-on@0.target
obmc-start-host@0.service
...
```

You can see we have another target in here, obmc-power-chassis-on@0.target, along with some services that will all be started by systemd when you do a "systemctl start obmc-chassis-start@0.target".

The target has corresponding services associated with it:
```
ls -1 /lib/systemd/system/obmc-power-chassis-on\@0.target.wants/
op-power-start@0.service
op-wait-power-on@0.service
```
So basically, if you run `systemctl start obmc-chassis-start@0.target` then systemd will start execution of all associated services.

The services have dependencies within them that control the execution of each service (for example, the op-power-start.service will run prior to the op-wait-power-on.service).  These dependencies are set using targets and the Wants,Before,After keywords.

## Server Power Off
The server power off function is encapsulated in the obmc-chassis-stop@0.target.  You can do a similar exercise with the power on to see its dependent services.

## Systemd Control in OpenBMC
There are a collection of services within openbmc that interact with systemd and its unit files, providing somewhat of an abstraction layer from the user of the openbmc system and systemd.  See the [state](https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/xyz/openbmc_project/State) interfaces for more information on this function.

For example, if you wanted to execute the server power on function, you would do the following:

> busctl set-property xyz.openbmc_project.State.Host /xyz/openbmc_project/state/host0 xyz.openbmc_project.State.Host RequestedHostTransition s xyz.openbmc_project.State.Host.Transition.On

Underneath the covers, this is calling systemd with obmc-chassis-start@0.target
