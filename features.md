# OpenBMC Features

Follow links to learn more about OpenBMC features:

- [BMCWeb][] HTTP/Web server
- [WebUI Vue][] web application
- REST Management: [BMCWeb Redfish][], [Phosphor REST APIs][] includes [Host
  management REST APIs][]
- [D-Bus interfaces][] describes internal interfaces
- [D-Bus Object Mapper][]
- [Remote KVM][]
- [IPMI in band][] and [IPMI out of band][]
- Full IPMI 2.0 Compliance with DCMI
- SSH based SOL: [How to use][sol how to use]
- Power and Cooling Management: [Phosphor Fan Control][]
- [Logging][phosphor logging] and [Callouts][logging callouts]
- Zeroconf discoverable through `systemd-networkd`
- [Sensors][]
- Inventory: [Entity manager][], [Phosphor inventory manager][] and its [MSL
  application][]
- [LEDs][]: see also [LED Groups][]
- Host Watchdog: [Phosphor Watchdog Implementation][]
- [Power State management] and [Chassis Power control][]
- [Network management][]
- [Factory reset][]
- [User management][phosphor user management]
- Time (time of day clock) management
- [Certificate management][]: [Phosphor Certificate Manager][]
- [Simulation][] via QEMU
- [Firmware update support][]
- [Automated Testing][]
- LDAP
- Remote syslog

## OpenPOWER Features

- [POWER OCC Support][power occ implementation] (On Chip Controller)
- [Hardware Diagnostics][] for POWER Systems fatal hardware errors.

[automated testing]:
  https://github.com/openbmc/openbmc-test-automation/blob/master/README.md
[bmcweb]: https://github.com/openbmc/bmcweb/blob/master/README.md
[bmcweb redfish]:
  https://github.com/openbmc/bmcweb/blob/master/DEVELOPING.md#redfish
[certificate management]:
  https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/yaml/xyz/openbmc_project/Certs/README.md
[chassis power control]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Chassis/README.md
[d-bus interfaces]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/README.md
[d-bus object mapper]:
  https://github.com/openbmc/docs/blob/master/architecture/object-mapper.md
[entity manager]:
  https://github.com/openbmc/entity-manager/blob/master/README.md
[factory reset]:
  https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/yaml/xyz/openbmc_project/Common/FactoryReset/README.md
[firmware update support]:
  https://github.com/openbmc/docs/blob/master/architecture/code-update/code-update.md
[hardware diagnostics]:
  https://github.com/openbmc/openpower-hw-diags/blob/master/README.md
[host management]:
  https://github.com/openbmc/docs/blob/master/host-management.md
[host management rest apis]:
  https://github.com/openbmc/docs/blob/master/host-management.md
[ipmi in band]:
  https://github.com/openbmc/docs/blob/master/architecture/ipmi-architecture.md
[ipmi out of band]: https://github.com/openbmc/ipmitool/blob/master/README
[led groups]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Led/README.md
[leds]:
  https://github.com/openbmc/docs/blob/master/architecture/LED-architecture.md
[logging callouts]:
  https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/yaml/xyz/openbmc_project/Common/Callout/README.md
[msl application]:
  https://github.com/openbmc/phosphor-dbus-monitor/blob/master/mslverify/README.md
[network management]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Network/README.md
[phosphor certificate manager]:
  https://github.com/openbmc/phosphor-certificate-manager/blob/master/README.md
[phosphor fan control]:
  https://github.com/openbmc/phosphor-fan-presence/blob/master/README.md
[phosphor inventory manager]:
  https://github.com/openbmc/phosphor-inventory-manager/blob/master/README.md
[phosphor logging]:
  https://github.com/openbmc/phosphor-logging/blob/master/README.md
[phosphor rest apis]: https://github.com/openbmc/docs/blob/master/rest-api.md
[phosphor user management]:
  https://github.com/openbmc/docs/blob/master/architecture/user-management.md
[phosphor watchdog implementation]: https://github.com/openbmc/phosphor-watchdog
[power occ implementation]: https://github.com/openbmc/openpower-occ-control
[remote kvm]: https://github.com/openbmc/obmc-ikvm/blob/master/README.md
[sensors]:
  https://github.com/openbmc/docs/blob/master/architecture/sensor-architecture.md
[simulation]:
  https://github.com/openbmc/docs/blob/master/development/dev-environment.md
[power state management]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/README.md
[sol how to use]: https://github.com/openbmc/docs/blob/master/console.md
[webui vue]: https://github.com/openbmc/webui-vue/blob/master/README.md
