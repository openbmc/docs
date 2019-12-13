# OpenBMC interfaces

Purpose: This introduces the BMC's primary interfaces.  The information here is
intended to provide a shared understanding suitable for a wide audience:
 - Engineers provide domain expertise in specific areas and learn about use
   cases and threats their interfaces poses.
 - BMC administrators and system integrators need to understand the entire
   system at this level of abstraction.  For example, to understand which
   interfaces can be disabled.
 - Management and security folks need everything to work and play together
   nicely.  For example, to understand the BMC's attack surfaces.

Diagrams are included to help visualize relationships.  The diagrams show
management agents on the left side, the BMC in the center, and host elements on
the right side.  The diagrams are simplified and are not intended be complete.

The document begins with the BMC's physical interfaces and moves toward
abstractions such as networking services the BMC provides.  The intent is to
show the relevant interfaces essential to or provided by the OpenBMC project as
a framework to reason about which interfaces are present, how they are related,
which can be disabled, how they are secured, etc.

NOTE: This focuses on the BMC's management network; it lacks details about
host-facing interfaces.  TODO: Please contribute information about interfaces
are services which are common to the OpenBMC implementation.  The intent is for
this document to be used as a reference for customized BMC implementations,
including specific BMC and host hardware and projects which are built on
OpenBMC.

This document references OpenBMC systemd unit names to help link concepts to
the source code.  The reader is assumed to be familiar with [systemd
concepts][].  The templated units ("unit@.service") may be omitted for clarity.
Relevant details from the unit file may be shown, such as the program which
implements a service.

OpenBMC uses the [Service Management][] interface to work with systemd services.

[systemd concepts]: https://www.freedesktop.org/software/systemd/man/systemd.html#Concepts
[Service Management]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Control/Service/README.md

The appendix has more details about each interface and service shown here.

## Introduction to the interfaces and services

### Physical interfaces

This shows the BMC's physical connections which OpenBMC uses, including
network, USB, UART serial, and connections to the host.  It is intended
primarily as a reference for the other diagrams.

```
        +----------------+        +----------------+
        | BMC            |        | Host           |
        |                |        |                |
        | Network       -+--------+- USB           |
       -+- eth0         -+--------+- KVM           |
       -+- eth1         -+--------+- (media)       |
        |  lo           -+- MCTP -+-               |
        |               -+--------+- serial        |
        | USB           -+- IPMI -+-               |
       -+- usb0          |        |                |
        |                |        |                |
        | Serial         |        |                |
       -+- tty0          |        |                |
        |                |        |                |
        +----------------+        +----------------+
```

### Network services

The BMC provides services to agents on its management network, listed here by
port number.

```
        +------------------------------+
        | BMC                          |
        |                              |
       -+-+ Network services           |
        | |                            |
        | +-+ TCP ports                |
        | | +- 22 ssh - shell          |
        | | +- 80 HTTP (no connection) |
        | | +- 443 HTTPS               |
        | | +- 2200 ssh - host console |
        | | +- 5355 Avahi              |
        | |                            |
        | +-+ UDP ports                |
        |   +- 427 SLP                 |
        |   +- 623 RCMP+ IPMI          |
        |   +- 5355 Service discovery  |
        |                              |
        +------------------------------+
```

This shows how the ports are connected to `systemd` sockets, services, and
details how that service provided.

TCP ports:
 - 22 = `dropbear.socket` -- `dropbear@.service` -- `dropbear` -- (BMC shell)
 - 80 (no connection, see alternative configuration)
 - 443 = `bmcweb.socket` -- `bmcweb.service` -- `bmcweb`
 - 2200 = `obmc-console-ssh.socket` -- `obmc-console-ssh@.service` --
          `dropbear` -- `obmc-console-client`
 - 5355 is for Avahi Zeroconf DNS service discovery

UDP ports:
 - 427 = `slpd.service` -- `slpd`
 - 623 = `phosphor-ipmi-net@${RMCPP_IFACE}.socket` --
         `phosphor-ipmi-net@${RMCPP_IFACE}.service`
 - 5355 is for service discovery


### Host console

The BMC extends access to its host's serial console to the BMC's management
network.

```
                +---------------------------+    +-----------------+
                | BMC                       |    | Host            |
 ipmitool sol   |                           |    |                 |
 activate       |                           |    |                 |
 UDP port 623 .... netipmid ------------}   |    |                 |
                |                       }   |    |                 |
 ssh -p 2200   ... obmc-console-client -}-+ |    |                 |
 TCP port 2200  |                       } | |    |                 |
                |  hostlogger ----------} | |    |                 |
                |                         | |    |                 |
                |      +------------------+ |    |                 |
                |      |                    |    |                 |
                |  socket "obmc-console"    |    |                 |
                |      |                    |    |                 |
                |  obmc-console-server      |    |                 |
                |      |                    |    |                 |
                |  /dev/${HOST_TTY} --------------- serial console |
                |                           |    |                 |
                +---------------------------+    +-----------------+
```

The `obmc-console` service provides an abstraction of the host console as the
Unix domain socket named "obmc-console" which other BMC services can use.

This shows how the services are connected:

 - The `obmc-console` service binds the host console to the `obmc-console`
   socket.
   `/dev/ttyS0` -- `obmc-console@ttyS0.service` -- `obmc-console-server` --
       `socket="obmc-console"`
 - Multiple services consume the host serial console:
    - TCP port 2200 (via `SSH -P 2200`) -- `obmc-console-ssh.socket` client --
          `obmc-console-ssh@.service` -- `dropbear` -- `obmc-console-client` --
          `socket="obmc-console"`
    - UDP port 623 (IPMI) -- `phosphor-ipmi-net@.socket` -- `netipmid` --
          `socket="obmc-console"`
    - (no network connection) -- `hostlogger.service` -- `hostlogger` --
          `socket="obmc-console"`
          Hostlogger writes host console messages to BMC files in the
          `/var/lib/obmc/hostlogs/` directory on the BMC's flash.


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
BMCWeb APIs, some of which set up Websockets which give the network agent
direct communication with the desired interface (not via BMCWeb).

Note that [BMCWeb is configurable][] at compile time.  This section describes
the default configuration (which serves the HTTP application protocol over the
HTTPS transport protocol on TCP port 443).

[BMCWeb is configurable]: https://github.com/openbmc/bmcweb/blob/master/CMakeLists.txt

Services provided:
 - Web application (phosphor-webui) and other static content
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

### D-Bus interfaces

Various OpenBMC applications communicate via D-Bus interfaces.

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

Read the [Phosphor D-Bus interface docs][] and search for README files in
various subdirectories under the xyz/openbmc_project path.

[Phosphor D-Bus interface docs]: https://github.com/openbmc/phosphor-dbus-interfaces


## Interfaces and services

This section lists each interface and service shown in this document.  The
intent is to help you understand the relevance of each item and how to locate
details in the source code.

### BMC network

The BMC may be connected to a network used to manage the BMC.  This is dubbed
the "management network" to distinguish it from the payload network the host
system is connected to.  These are typically separate networks.
```
             +-----------+      +----------------+
             | BMC       |      | Host           |
management   |           |      |                |
network   ---+- Network  |      |       Network -+- payload
             |           |      |                |  network
             +-----------+      +----------------+

```

The BMC and host may share network hardware yet maintain logically separate
network access.
```
             +-----------+      +----------------+
             | BMC       |      | Host           |
             |           |      |                |
           +-+- Network  |      |       Network -+-+
           | |           |      |                | |
           | +-----------+      +----------------+ |
           |                                       |
           +-----------------------------+         |
                                         |         |
             +----------------------+    |         |
             | Network multiplexer  |    |         |
management   |                      |    |         |
network  ----+----------------------+----+         |
             |                      |              |
payload      |                      |              |
network  ----+----------------------+--------------+
             |                      |
             +----------------------+

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

### BMC serial

This gives access to the BMC's console which provides such function as
controlling the BMC's U-Boot and then providing access to the BMC's shell.
Contrast with the host console access.

### BMC/host bus connections

TODO

### Network interfaces

This refers to the standard NIC and Linux network services on the BMC.

### Secure Shell (SSH)

This refers to the SSH protocol which provides both secure shell (ssh) and
secure copy (scp) access to the BMC.  OpenBMC uses the Dropbear SSH
implementation.  Note that port 22 connects to the BMC's shell, while port 2200
connects to the host console.

### HTTP application protocol

Provided by the web server.

### HTTP transport protocol

OpenBMC does not support the HTTP transport protocol by default.  The webserver
has a build-time option to provide this support (which is disabled by default).
Contrast with the HTTPS transport protocol and the HTTP application protocol
which are provided by default.

### HTTPS transport protocol

Provided by the web server.

### Host serial console

Refers to the BMC's access to its host's system console (typically via the
host's serial connection).  See also `obmc-console-server` which provides host
serial access to various internal BMC services.  Contrast with access to the
BMC's serial connectiom which provides access to the BMC's console.

### Avahi Zeroconf mDNS service discovery

Refers to the multicast discovery service.  For example, you can find the BMC
via the `avahi-browse -rt _obmc_rest._tcp` command.

### Service Location Protocol (SLP)

Refers to the unicast service discovery protocol provided by `slpd`.  For
example, you can find the BMC via the `slptool -u ${ip} findsrvtypes or
findsrvs` command.

### RCMP+ IPMI

Refers to the RCMP+ protocol and IPMI implementation provided by `netipmid`
with source here: `https://github.com/openbmc/phosphor-net-ipmid` and some
details provided by [IPMI Session management][].  network IPMI provides access
to many resources including host IPMI access, SOL (access to the host console),
and more.  See also ipmitool.  Also known as out of band IPMI.  Contrast with
host-IPMI.

[IPMI Session management]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Ipmi/SESSION_README.md

### ipmitool

Refers to the command used by agents on the management network (or equivalent
access via the host) to reach the BMC's network IPMI service.  Hosted by
https://github.com/ipmitool/ipmitool

### Host IPMI

Refers to the host-facing IPMI service provided by the `ipmid` program with
source here: `https://github.com/openbmc/phosphor-host-ipmid`.  The systemd
service is `phosphor-ipmi-host` implemented by the `ipmid` program.  Also known
as in-band IPMI.  Contrast with network IPMI and with PLDM.

### BMC shell

This refers to the BMC's command line interface which defaults to the `bash`
program provided via the `/bin/sh` path on the BMC's file system.  Note that
the shell (together with its utility programs) provides access to many of the
BMC's internal and external interfaces.

### hostlogger

Refers to the BMC service provided by the `hostlogger` program here:
https://github.com/openbmc/phosphor-hostlogger which listens to the
`obmc-console-server` and logs host console messages into the BMC's file
system.

### obmc-console-client

Refers to the BMC service provided by the `obmc-console-client` program with
source here: https://github.com/openbmc/obmc-console which is suitable to
connect to an SSH session.

### BMCWeb webserver

Refers to the custom HTTP/Web server with source here:
https://github.com/openbmc/bmcweb Note that BMCWeb is configurable per
https://github.com/openbmc/bmcweb/blob/master/CMakeLists.txt with build-time
options to control which interfaces it provides.  For example, there are
configurations options to:
 - enable downloading firmware images from a TFTP server
 - enable the HTTP transport protocol instead of HTTPS
 - others

The webserver also sets up Secure Websockets for services such as KVM-IP,
Virtual-USB, and more.  Note that HTTP refers to the application protocol which
can be served over the HTTP, HTTPS, or other transport protocols.

### Redfish

Refers to the set of Redfish REST APIs served by the BMCWeb web server.  See
details here: https://github.com/openbmc/bmcweb/blob/master/Redfish.md with
docs here: https://github.com/openbmc/docs/blob/master/REDFISH-cheatsheet.md

### phosphor-dbus-rest

Refers to the legacy REST APIs optionally served by the BMCWeb server.
Docs: https://github.com/openbmc/docs/blob/master/REST-cheatsheet.md

### KVM-IP

Refers to the OpenBMC implementation of the Remote Frame Buffer (RFB, aka VNC)
protocol which lets you operate the host system's keyboard, video, and mouse
(KVM) remotely.  See https://github.com/openbmc/obmc-ikvm/blob/master/README.md
Also known as IPKvm.  Do not confuse with Kernel Virtual Machine (the other
KVM).

### Virtual media

Also known as: remote media and USB-over-IP.  Design:
https://github.com/openbmc/docs/blob/f6ae5ef8e87bed0b904ad9a01bea5c3a641c80a5/designs/VirtualMedia.md

### Virtual USB

Also known as USB-over-IP, and helps implement virtual media.  Contrast with
the BMC and host physical USB ports.
