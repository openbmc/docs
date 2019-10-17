# OpenBMC Features

Follow links to learn more about OpenBMC features:

* [BMCWeb][] HTTP/Web server
* [Phosphor WebUI][] web application
* REST Management: [BMCWeb Redfish][], [Phosphor REST APIs][] includes
  [Host management REST APIs][]
* [D-Bus interfaces][] describes internal interfaces
* [D-Bus Object Mapper][]
* [Remote KVM][]
* [IPMI in band][] and [IPMI out of band][]
* Full IPMI 2.0 Compliance with DCMI
* SSH based SOL: [How to use][SOL How to use]
* Power and Cooling Management: [Phosphor Fan Control][]
* [Logging][Phosphor Logging] and [Callouts][Logging Callouts]
* Zeroconf discoverable through `systemd-networkd`
* [Sensors][]
* Inventory: [Entity manager][], [Phosphor inventory manager][] and its [MSL application][]
* [LEDs][]: see also [LED Groups][]
* Host Watchdog: [Phosphor Watchdog Implementation][]
* [Power State management] and [Chassis Power control][]
* [Network management][]
* [Factory reset][]
* [User management][Phosphor User Management]
* Time (time of day clock) management
* [Certificate management][]: [Phosphor Certificate Manager][]
* [Simulation][] via QEMU
* [Firmware update support][]
* [Automated Testing][]
* LDAP
* Remote syslog

## OpenPOWER Features

* [POWER OCC Support][POWER OCC Implementation] (On Chip Controller)
* [Hardware Diagnostics][] for POWER Systems fatal hardware errors.

[Automated Testing]: https://github.com/openbmc/openbmc-test-automation/blob/master/README.md
[BMCWeb]: https://github.com/openbmc/bmcweb/blob/master/README.md
[BMCWeb Redfish]: https://github.com/openbmc/bmcweb/blob/master/DEVELOPING.md#redfish
[Certificate management]: https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/xyz/openbmc_project/Certs/README.md
[Chassis Power control]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Chassis/README.md
[D-Bus interfaces]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/README.md
[D-Bus Object Mapper]: https://github.com/openbmc/docs/blob/master/architecture/object-mapper.md
[Entity manager]: https://github.com/openbmc/entity-manager/blob/master/README.md
[Factory reset]: https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/xyz/openbmc_project/Common/FactoryReset/README.md
[Firmware update support]: https://github.com/openbmc/docs/blob/master/code-update/code-update.md
[Hardware Diagnostics]: https://github.com/openbmc/openpower-hw-diags/blob/master/README.md
[Host management]: https://github.com/openbmc/docs/blob/master/host-management.md
[Host management REST APIs]: https://github.com/openbmc/docs/blob/master/host-management.md
[IPMI in band]: https://github.com/openbmc/docs/blob/master/architecture/ipmi-architecture.md
[IPMI out of band]: https://github.com/openbmc/ipmitool/blob/master/README
[LED Groups]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Led/README.md
[LEDs]: https://github.com/openbmc/docs/blob/master/architecture/LED-architecture.md
[Logging Callouts]: https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/xyz/openbmc_project/Common/Callout/README.md
[MSL application]: https://github.com/openbmc/phosphor-dbus-monitor/blob/master/mslverify/README.md
[Network management]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Network/README.md
[Phosphor Certificate Manager]: https://github.com/openbmc/phosphor-certificate-manager/blob/master/README.md
[Phosphor Fan Control]: https://github.com/openbmc/phosphor-fan-presence/blob/master/README.md
[Phosphor inventory manager]: https://github.com/openbmc/phosphor-inventory-manager/blob/master/README.md
[Phosphor Logging]: https://github.com/openbmc/phosphor-logging/blob/master/README.md
[Phosphor REST APIs]: https://github.com/openbmc/docs/blob/master/rest-api.md
[Phosphor User Management]: https://github.com/openbmc/docs/blob/master/architecture/user_management.md
[Phosphor Watchdog Implementation]: https://github.com/openbmc/phosphor-watchdog
[Phosphor WebUI]: https://github.com/openbmc/phosphor-webui/blob/master/README.md
[Power OCC Implementation]: https://github.com/openbmc/openpower-occ-control
[Remote KVM]: https://github.com/openbmc/obmc-ikvm/blob/master/README.md
[Sensors]: https://github.com/openbmc/docs/blob/master/architecture/sensor-architecture.md
[Simulation]: https://github.com/openbmc/docs/blob/master/development/dev-environment.md
[Power State management]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/State/README.md
[SOL How to use]: https://github.com/openbmc/docs/blob/master/console.md
