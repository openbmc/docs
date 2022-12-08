# OpenBMC Release Notes

The OpenBMC project now has a regular release cycle with stable branches
starting with the 2.6 release. Prior release tag notes are also listed here for
completeness.

Read more about the release process here:
https://github.com/openbmc/docs/blob/master/release/release-process.md

For information on how to checkout a particular branch or tag, see:
https://github.com/openbmc/openbmc/wiki/Releases

## OpenBMC Releases

### 2.9 January, 2021

#### Features:

- Yocto refresh to "Gatesgarth" 3.2
- Partial Redfish support for properties from the 2020.1, 2020.2, and 2020.3
  Schemas
- Redfish support for: Dump, Multiple Firmware Image Support
- Added webui-vue, a web-based user interface built of Vue.js
  - A replacement for phosphor-webui
  - Uses Redfish
  - Ability to easily theme to meet brand guidelines
  - Language translation-ready
  - Improved user experience

#### Fixes and Known Issues/Limitations:

#### Security audit results:

### 2.8 June, 2020

#### Features:

- Yocto refresh to "Dunfell" version 3.1
- Redfish support for:
  - full certificate management
  - complete LDAP management
  - full sensor support
  - event service schema
  - task schema
- Move to Redfish Specification 1.9.0
- Redfish support for 2020.1 Schemas
- GUI enhancements:
  - LDAP
  - certificate management
- mTLS HTTPS authentication
- Partial PLDM Support
- Partial MCTP Support

#### Fixes and Known Issues/Limitations:

- Enabling and disabling DHCP via Redfish is not working (openbmc/bmcweb #127)
- LDAP login will fail for users belonging to the redfish group
- Unable to configure IP address on VLAN interface (openbmc/openbmc #3668)
- Unable create VLAN via IPMI (openbmc/phosphor-net-ipmid #12)

#### Security audit results:

- IPMI RMCP+ cipher suite 3 was removed, leaving only cipher suite 17
  (https://github.com/openbmc/phosphor-net-ipmid/commit/4c494398a36d9f1bdc4f256f937487c7ebcc4e95)
- The fix for CVE-2020-14156 is present. This affects images that have IPMI
  users (https://github.com/openbmc/openbmc/issues/3670)

### 2.7 Aug 5, 2019

#### Features:

- Yocto refresh to "Warrior" version 2.7
- Removal of Python for footprint reduction: python is no longer required for
  the meta-phosphor layer and its defaults
- Finished up KVM over IP: adds the infrastructure to allow KVM sessions through
  the webui
- NVMe-MI over SMBus
- ECC logging for BMC: service to monitor EDAC driver and BMC log
- Partial PLDM Support
- Partial MCTP Support
- Redfish support for: partial Certificates, local user management, partial
  LDAP, network, event logging, DateTime, boot devices, firmware update,
  inventory and sensors
- phosphor-ipmi-flash support: sending firmware images over IPMI, pci-aspeed,
  and other mechanisms for host-driven updates
- GUI enhancements: multiple user support, virtual media, KVM

#### Fixes and Known Issues/Limitations:

- Yocto Warrior subtree refresh applied (24230)

### 2.6 Feb 4, 2019

**_First Release as Linux Foundation Project_**

#### Features:

- Yocto refresh to "Thud" version 2.6
- GUI enhancements: SNMP and Date/Time
- Serial over Lan
- IPMI 2.0 support
- Partial Redfish support
- Kernel updated to 4.19 LTS

#### Known Issues/Limitations:

- Support dropped for the ipmitool nameless option

## Release tags with notes prior to release branching

### v1.0.5 Aug 23, 2016

#### Updates:

- Cache all inventory properties and preserve inventory during BMC firmware
  update (#487)
- Update the power button behavior on Barreleye to:
  - Short press: Only power on. Remove the power off action
  - Long press (>3 seconds): Hard power off

### v1.0.4 Aug 8, 2016

#### Changes:

- Kernel version update:
  - Stable release 4.4.16
  - Power button debounce fix for Barreleye
  - Fix for an NCSI race condition that caused the device to not come up
    (openbmc/phosphor-networkd#18)
- Load inventory from cache (#487)
- Start host watchdog timer after magic sequence (openbmc/skeleton#127)
- Restart REST server when network configuration changes
  (openbmc/phosphor-rest-server#24)

### v1.0.3 Jul 18, 2016

#### Updates:

- Fix issue in Host IPMI inventory due to versioned shared libraries (#423)

### v1.0.2 Jul 5, 2016

#### Updates:

- Add ability to perform BMC code update at runtime and get the BMC code update
  progress through REST
- Fix event log directory duplication during BMC code update
- Fix hwmon attribute not being polled after failure

### v1.0.1 Jun 27, 2016

#### Release Notes:

- Fix encode firmware version in BCD format
- Handle floating point sensor values
- Performance improvements to prevent services from failing to start
- Extend the mapper service startup timeout to ensure it starts up
- Enable DNS resolution from DHCP

### v1.0 Jun 20, 2016

#### Features:

- Enable one-time vs permanent host boot option
- Enable handling of host checkstop gpio
- Handle endianness in IPMI eSEL function
- Improve IPMI error handling
- Add IPMI Travis CI
- Fix host hanging due to inventory upload
- Fix i2c-tools SRC url syntax
- Fix sensor attribute reading
- Fix limit of number of event logs
- Add adm1278 driver into the Linux kernel by default
- Automatically restart phosphor service processes
- Enable out-of-tree kernel device trees
- Update pflash version
- Update u-boot version
  - Fix memory corruption
- Update kernel version
  - Stable 4.4.12 release
  - Pick up i2c completion fix
  - Add power button debounce function for Barreleye
- Remove development tool rest-dbus from Barreleye image
- BMC performance improvements

### v0.8 May 20, 2016

#### Updates:

- Update pflash version
- Host console support for local tty mirroring
- Cap number of event logs at 128
- BMC boot performance improvements
- Linux updates:
  - Update to 4.4.10 release
  - Fix network cable link detection
  - Support for VOUT sampling to the adm1275 driver
  - Add eeprom to barreleye device tree
  - Fix OCC sensor activation

### v0.7 May 5, 2016

#### Updates:

- Update to kernel 4.4
- Update to yocto 2.0.1
- Use upstream pflash

#### Features:

- Device tree
- New host console.
  - Documentation: https://github.com/openbmc/docs/blob/master/console.md
- REST association support
- Dmidecode support
- New REST interface to query network type (dhcp/static) under
  NetworkManager->GetAddressType

#### Fix:

- Add CPU1 fru data to inventory

### v0.6.1 Mar 22, 2016

#### Fixes:

- JFFS2 corruption
- random segmentation faults
- keep event logs from filling up flash. They are limited to 200K
- OCC 12core fix
- PCIe slot presence detect
- SCU fix for networking issue

### v0.6 Mar 7, 2016

#### New features:

- Immediate MAC address set via IPMI and REST; no reboot necessary
- full user setup via REST
- Boot to RAM filesystem so SOCFlash and pflash can be used from host
- ADM1278 support
- Custom LED blink rate
- inARP support

#### Fixes:

- ipmid memory leak
- SBE reboot issue

### v0.5 Feb 17, 2016

#### New Features:

- Automatically run fsck to avoid file system corrupt
- Full LAN get/set support via REST and ipmitool
- User setup via REST support
- NCSI driver enhancements
- Virtual UART; improved hconsole
- Persistent event logs
- Persistent UUID based on /etc/machine-id
- Full LED support

### v0.4 Feb 4, 2016

#### New Features:

- Persistent file system
- network configuration is persistent across boots and flashing
- LAN set functions
- Selectable flash update (u-boot, initramfs, kernel, settings)
- fw utils for update u-boot environment variables
- power capping and measurements
- power restore policy

#### Notes/Limitations:

- Currently using ext4 for R/W file system. Need to migrate to JFFS which is
  designed for flashes and more resilient to AC power loss. Please 'poweroff'
  BMC before pulling AC power for now

#### Missing Functions:

- Persistent event logs
- LAN get functions
- User set/get functions
- Proper LED representation

### v0.2 Dec 4, 2015

#### Release Notes:

- Added CPU thermal sensors (/org/openbmc/sensors/temperature/cpu0 and 1)
- Added full soft power off and reset support from host or BMC
- Reboot issue fixed
- Power button fixed
- Network crash issue fixed
- Added delete event log from REST
- Cleaned up inventory
- consistent states
- io_board fru is populated from eeprom

#### Known issues:

- OCC driver unloading and loading prints harmless error messages to console
- About 1 out of every 5 boots you will see the ftgmac driver print out a trace
  stack. This appears to be harmless, but we are investigating

#### Not supported:

- Sensor thresholds
- Setting boot options through REST
