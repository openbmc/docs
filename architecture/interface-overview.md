# OpenBMC interfaces

Purpose: This introduces a simplified view of the BMC's primary interfaces. It
is intended to provide a reference suitable for a wide audience:

- Engineers provide domain expertise in specific areas and learn about use cases
  and threats their interfaces poses.
- Give BMC administrators and system integrators a simplified view of the BMC's
  system interfaces. For example, to understand which interfaces can be
  disabled.
- Management and security folks need everything to work and play together
  nicely. For example, to understand the BMC's attack surfaces.

## Introduction to the interfaces and services

This section shows the BMC's primary interfaces and how they are related. It
begins with the BMC's physical interfaces and moves toward abstractions such as
network services. The intent is to show the interfaces essential to the OpenBMC
project in a framework to reason about which interfaces are present, how they
are related. This provides a foundation to reason about which can be disabled,
how they are secured, etc. The appendix provides details about each interface
and service shown.

OpenBMC's services and the interfaces they provide are controlled by `systemd`.
This document references OpenBMC `systemd` unit names to help link concepts to
the source code. The reader is assumed to be familiar with [systemd concepts][].
The templated units ("unit@.service") may be omitted for clarity. Relevant
details from the unit file may be shown, such as the program which implements a
service.

The OpenBMC [Service Management][] interface can control `systemd` services. For
example, disabling a BMC service will disable the corresponding external
interface.

[systemd concepts]:
  https://www.freedesktop.org/software/systemd/man/systemd.html#Concepts
[service management]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Control/Service/README.md

Diagrams are included to help visualize relationships. The diagrams show
management agents on the left side, the BMC in the center, and host elements on
the right side. The diagrams are simplified and are not intended to be complete.

### Physical interfaces

This shows the BMC's physical connections including network, USB, UART serial,
and connections to its host platform. This uses a simplified view of the host
which shows only the host interfaces that connect directly to the BMC. A typical
host would have additional connections for console, network, etc.

Interfaces between the BMC and its host platform vary considerably based on BMC
and host platform implementation. The information presented in this section and
its subsections is intended to illustrate common elements, not to represent any
particular system. This section is intended to be referenced by additional
documentation which gives details for specific BMC and host implementations.

```
        +----------------+         +----------------+
        | BMC            |         | Host           |
        |                |         |                |
        | Network       -+- LPC ---+-               |
       -+- eth0         -+--PCIe --+-               |
       -+- eth1         -+--UART --+-               |
        |  lo           -+- I2C ---+-               |
        |               -+--I3C ---+-               |
        | USB           -+- SPI ---+-               |
       -+- usb0         -+- PECI --+-               |
        |               -+- GPIOs -+-               |
        | Serial        -+- UTMI --+-               |
       -+- tty0          |         |                |
        |                |         |                |
        +----------------+         +----------------+
```

#### Host-BMC physical interface transport protocols

This lists protocols that operate over the BMC-host physical interfaces:

- Host IPMI.
- [MCTP][]. OpenBMC offers MCTP over LPC, PCIe, UART.
- Custom OEM solution.
- SMBus.

[mctp]:
  https://www.dmtf.org/sites/default/files/standards/documents/DSP0236_1.3.0.pdf

#### Host-BMC data models

This lists specifications for the data which flows over the BMC-host transport
protocols:

- Host IPMI.
- PLDM (DMTF document DSP0240).
- Custom OEM solution.

### Network services provided

OpenBMC provides services via its management network. The default services are
listed here by port number. More information about each service is given in
sections below or in the appendix.

```
        +----------------------------------+
        | BMC                              |
        |                                  |
       -+-+ Network services               |
        | |                                |
        | +-+ TCP ports                    |
        | | +- 22 ssh - shell              |
        | | +- 80 HTTP (no connection)     |
        | | +- 443 HTTPS                   |
        | | +- 2200 ssh - host console     |
        | | +- 5355 mDNS service discovery |
        | |                                |
        | +-+ UDP ports                    |
        |   +- 427 SLP                     |
        |   +- 623 RMCP+ IPMI              |
        |   +- 5355 mDNS service discovery |
        |                                  |
        +----------------------------------+
```

Services provided to connected clients may use ports for:

- Active SSH sessions.
- Active KVM-IP sessions.
- Active virtual media sessions.

### Network services consumed

This section lists network services used by OpenBMC systems. OpenBMC uses the
typical services in the usual way, such as NTP, DNS, and DHCP. In addition,
OpenBMC uses:

- TFTP (disabled by default, when invoked by BMC operator) - Trivial FTP client
  to fetch firmware images for [code update][].
- SNMP manager to catch [SNMP traps][] (when enabled).

[code update]:
  https://github.com/openbmc/docs/blob/master/code-update/code-update.md
[snmp traps]:
  https://github.com/openbmc/phosphor-snmp/blob/master/docs/snmp-configuration.md

### Host console

OpenBMC provides access to its host's serial console in various ways:

- Client access via network IPMI.
- Client access via ssh port 2200.
- The hostlogger facility.

```
                +---------------------------+    +-----------------+
                | BMC                       |    | Host            |
 ipmitool sol   |                           |    |                 |
 activate       |                           |    |                 |
 UDP port 623 .... netipmid ------------}   |    |                 |
                |                       }   |    |                 |
 ssh -p 2200   ... obmc-console-client -}---+----+- serial UART    |
 TCP port 2200  |                       }   |    |  console        |
                |  hostlogger ----------}   |    |                 |
                |                           |    |                 |
                +---------------------------+    +-----------------+
```

The [obmc-console][] details how the host UART connection is abstracted within
the BMC as a Unix domain socket.

[obmc-console]: https://github.com/openbmc/obmc-console/blob/master/README.md

### Web services

OpenBMC provides a custom HTTP/Web server called BMCWeb.

```
        +--------------------------------------------------+
        | BMC                                              |
        |                                                  |
       -+-+ Network services                               |
        | ++ TCP                                           |
        |  +- 443 HTTPS - BMCWeb -> { static content       |
        |  |                        {   Web app (webui)    |
        |  +- (other ports) <---+   {   Redfish schema     |
        |       |               |   { /login               |
        |       V               |   { Redfish REST APIs    |
       -+- Websockets -+        |   { Phosphor REST APIs   |
        |              |        +<--{-- can set up:        |
        |              |            {     KVM-IP, USB-IP,  |
        |           various         {     Virtual Media    |
        |                                                  |
        +--------------------------------------------------+
```

In the diagram, the arrowheads represent the flow of control from web agents to
BMCWeb APIs, some of which set up Websockets which give the network agent direct
communication with the desired interface (not via BMCWeb).

Note that [BMCWeb is configurable][] at compile time. This section describes the
default configuration (which serves the HTTP application protocol over the HTTPS
transport protocol on TCP port 443).

[bmcweb is configurable]: https://github.com/openbmc/bmcweb#configuration

Services provided:

- Web application (webui-vue) and other static content
- REST APIs including custom phosphor-rest and Redfish APIs
- KVM-IP (Keyboard, Video, Mouse over IP)
- Virtual media via USB-IP (Universal Serial Bus over IP)
- others

### Host IPMI services

OpenBMC provides a host IPMI service.

```
    +---------------+    +-----------------+
    | BMC           |    | Host            |
    |               |    |                 |
    |        ipmid -+----+-                |
    |               |    |                 |
    +---------------+    +-----------------+
```

The IPMI firmware firewall (which aims to control which host commands and
channels can be used) is not implemented in OpenBMC. There is support for a
[Phosphor host IPMI whitelist][] scheme.

[phosphor host ipmi whitelist]:
  https://github.com/openbmc/openbmc/blob/master/meta-phosphor/classes/phosphor-ipmi-host-whitelist.bbclass

### D-Bus interfaces

OpenBMC uses D-Bus interfaces as the primary way to communicate (inter-process
communication) between OpenBMC applications. Note that other methods are used,
for example Unix domain sockets.

```
        +--------------------------------------------------+
        | BMC                                              |
        |                                                  |
        | +-------+                                        |
        | | D-Bus |                                        |
        | |      -+- bmcweb                                |
        | |      -+- ipmid                                 |
        | |      -+- ...                                   |
        | |      -+- many more (not shown here)            |
        | |      -+- ...                                   |
        | |       |                                        |
        | +-------+                                        |
        |                                                  |
        +--------------------------------------------------+
```

To learn more, read the [Phosphor D-Bus interface docs][] and search for README
files in various subdirectories under the xyz/openbmc_project path.

[phosphor d-bus interface docs]:
  https://github.com/openbmc/phosphor-dbus-interfaces

## Interfaces and services

This section lists each interface and service shown in this document. The intent
is to give the relevance of each item and how to locate details in the source
code.

### BMC network

This sections shows variations in the operational environment of the BMC's
management network.

The BMC may be connected to a network used to manage the BMC. This is dubbed the
"management network" to distinguish it from the payload network the host system
is connected to. These are typically separate networks.

```
             +-----------+      +----------------+
             | BMC       |      | Host           |
management   |           |      |                |
network   ---+- Network  |      |       Network -+- payload
             |           |      |                |  network
             +-----------+      +----------------+
```

The BMC may be served by a Network Controller Sideband Interface (NC-SI) which
maintains a logically separate network from the host, as shown in this diagram:

```
             +-----------+      +----------------+
             | BMC       |      | Host           |
management   |           |      |                |
network    +-+- Network  |      |       Network -+-+
           | |           |      |                | |
           | +-----------+      +----------------+ |
           |                                       |
           |      +------------------+             |
           |      | NIC              |             |
           |      |.........+       -+-------------+
           +------+- side-  :        |
management -------+- band   :       -+- payload
network           |.........+        |  network
                  +------------------+
```

The BMC's management network may be provided by its host system and have no
direct connection external to the host, as shown in this diagram:

```
             +-----------+      +----------------+
             | BMC       |      | Host           |
             |           |      |                |
          +--+- Network  |      |       Network -+- payload
          |  |           |      |                |  network
          |  |           |   +--+- management    |
          |  |           |   |  |  network       |
          |  +-----------+   |  +----------------+
          |                  |
          +------------------+
```

The BMC's management network may be connected to USB (LAN over USB):

```
             +-----------+      +----------------+
             | BMC       |      | Host           |
        +-+  |           |      |                |
   USB --+---+- Network  |      |       Network -+- payload
        +-+  |           |      |                |  network
             |           |      |                |
             +-----------+      +----------------+
```

### BMC serial

This gives access to the BMC's console which provides such function as
controlling the BMC's U-Boot and then providing access to the BMC's shell.
Contrast with the host serial console access.

### Network interfaces

This refers to the standard NIC and Linux network services on the BMC.

### Secure Shell (SSH)

This refers to the SSH protocol which provides both secure shell (ssh) and
secure copy (scp) access to the BMC. OpenBMC uses the Dropbear SSH
implementation. Note that port 22 connects to the BMC's shell, while port 2200
connects to the host console.

### HTTP and HTTPS

OpenBMC supports the HTTP application protocol over HTTPS, both handled by the
BMCWeb server. The "http" URI scheme is disabled by default but can be enabled
at compile time by BMCWeb configuration options.

### Host serial console

Refers to the BMC's access to its host's serial connection which typically
accesses the host system's console. See also `obmc-console-server` which
provides host serial access to various internal BMC services. Contrast with
access to the BMC's serial connection which provides access to the BMC's
console.

### Service discovery

Refers to the multicast discovery service (mDNS). For example, you can find the
BMC via the `avahi-browse -rt _obmc_rest._tcp` command.

### Service Location Protocol (SLP)

Refers to the unicast service discovery protocol provided by `slpd`. For
example, you can find the BMC via the
`slptool -u ${ip} findsrvtypes or findsrvs` command.

### RMCP+, IPMI, and ipmitool

Refers to the RMCP+ protocol and IPMI implementation provided by `netipmid` with
source here: `https://github.com/openbmc/phosphor-net-ipmid` and some details
provided by [IPMI Session management][]. Network IPMI provides access to many
resources including host IPMI access, SOL (access to the host console), and
more. Also known as out of band IPMI. Contrast with host-IPMI which interacts
with the host and with Redfish which provides alternate function.

The BMC's RMCP+ IPMI interface is designed to be operated by the `[ipmitool][]`
external command.

[ipmi session management]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Ipmi/SESSION_README.md
[ipmitool]: https://github.com/ipmitool/ipmitool

### Host IPMI

Refers to the host-facing IPMI service provided by the `ipmid` program with
source here: `https://github.com/openbmc/phosphor-host-ipmid`. The systemd
service is `phosphor-ipmi-host` implemented by the `ipmid` program. Also known
as in-band IPMI. Contrast with RMCP+ which faces the network and with PLDM which
provides alternate function.

### BMC shell

This refers to the BMC's command line interface which defaults to the `bash`
program provided via the `/bin/sh` path on the BMC's file system. Note that the
shell (together with its utility programs) provides access to many of the BMC's
internal and external interfaces.

### obmc-console

This refers to support for multiple independent consoles in
https://github.com/openbmc/obmc-console and two applications:

- The `obmc-console-server` abstracts the host console (UART) connection as a
  Unix domain socket.
- The `obmc-console-client` can connect a console to an SSH session.

Other applications use the console server.

### hostlogger

Refers to the BMC service provided by the `hostlogger` program here:
https://github.com/openbmc/phosphor-hostlogger which listens to the
`obmc-console-server` and logs host console messages into the BMC's file system.

### BMCWeb web server

Refers to the custom HTTP/Web server with source here:
https://github.com/openbmc/bmcweb Note that BMCWeb is configurable per
https://github.com/openbmc/bmcweb#configuration with build-time options to
control which interfaces it provides. For example, there are configurations
options to:

- enable downloading firmware images from a TFTP server
- enable the "http" URI scheme
- others

The webserver also sets up Secure Websockets for services such as KVM-IP,
Virtual-USB, and more.

### Redfish

Refers to the set of Redfish REST APIs served by the BMCWeb web server. See
details here: https://github.com/openbmc/bmcweb/blob/master/Redfish.md with docs
here: https://github.com/openbmc/docs/blob/master/REDFISH-cheatsheet.md

### phosphor-dbus-rest

Refers to the legacy REST APIs optionally served by the BMCWeb server. Docs:
https://github.com/openbmc/docs/blob/master/REST-cheatsheet.md

### KVM-IP

Refers to the OpenBMC implementation of the Remote Frame Buffer (RFB, aka VNC)
protocol which lets you operate the host system's keyboard, video, and mouse
(KVM) remotely. See https://github.com/openbmc/obmc-ikvm/blob/master/README.md
Also known as IPKvm. Do not confuse with Kernel Virtual Machine (the other KVM).

### Virtual media

Also known as: remote media and USB-over-IP. Design:
https://github.com/openbmc/docs/blob/master/designs/VirtualMedia.md Contrast
with LAN-over-USB.

### Virtual USB

Also known as USB-over-IP, and helps implement virtual media. Contrast with the
BMC and host physical USB ports.
