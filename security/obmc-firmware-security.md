# OpenBMC firmware security topics

TO DO: This is a work in progress intended to be merged into the
openbmc/docs repo in its present incomplete state.  The introduction
and basic architecture are ready; the OpenBMC firmware elements are
stubs only.


This document outlines security concerns for code that runs on the
BMC.  It outlines the basic architecture spanning from device files to
web interfaces.  (TODO: It then identifies elements within that
framework and addresses their security concerns.)  This complements
the security documentation for the BMC hardware in the [OpenBMC Server
Security Architecture](./obmc-securityarchitecture.md).

The reader is assumed to be familiar with the Yocto and Phosphor
layers in the [OpenBMC repository](https://github.com/openbmc/openbmc/)
which bring together elements of the OpenBMC firmware image.

This document describes the architecture, common elements, and common
configurations.  It does not attempt to describe every possible
element or configuration.  Specifically, this document describes
elements that may be omitted from OpenBMC firmware images.

Guidance for downstream development groups: Customize this document
for your specific OpenBMC application, detailing the elements you use
and how they are configured.  Typical applications vary in terms of:
 - Specific BMC hardware and host board
 - Specialized applications to monitor and control the host system
 - Selection of external BMC controls such as IPMI and web servers


## Basic architecture

The primary element of an OpenBMC firmware image the Linux operating
system which serves as a platform for all other elements.  Within the
Linux image, the architecture is characterized by four interface
layers: device files, D-Bus objects, URIs, and external interfaces.

 - Device files directly represent hardware devices such as fans and
   temperature sensors; they provide a way to monitor and control the
   hardware the BMC uses as part of its operation.

 - D-Bus objects provide access to all the resources the BMC monitors
   and controls.  Some of the objects are direct mappings of devices,
   others are for resources stored on the BMC such as firmware images.

 - URIs (uniform resource identifiers) are provided by REST API
   servers.  These typically serve as a translation layer for D-Bus
   objects and provide access via HTTP.  An example use case is to
   map D-Bus objects onto the RedFish specification.

 - The external interfaces include network servers (such as web
   applications, REST APIs, and IPMI) and access from the host (for
   example: error logging, or host-initiated reboot).  Other OpenBMC
   elements are characterized in terms of these interfaces.

These interfaces cover all significant information and control flows.
The following sections:
 - describe each interface in more detail
 - state how each interface gets populated
 - describe access controls such as authentication and authorization

TODO: Move most of this section (and its subsections) to the regular
docs repository, and change this section to reference significant
architectural features.

TODO: Populate the subsections below with more security considerations.

### Linux

OpenBMC is a Linux distribution based on Yocto and customized with
BMC-specific elements including daemon services and application
programs to interact with the host system, and with web and other
servers to interact with external agents.

Security considerations for Yocto are described in
https://wiki.yoctoproject.org/wiki/Security.

Downstream development teams are expected to fork the OpenBMC project
and customize it, for example, customizing applications and
configuring which external interfaces the BMC provides, and build for
a specific set of hardware (consisting of the BMC plus its host
system) via the TEMPLATECONF environment variable.

When built, an OpenBMC firmware image is complete and requires no
other software.  Indeed, installing new software, even in the form of
shell scripts, represents a security risk.  The image may require
configuration after it is installed, for example, to set up a static
network address.

The firmware image gets onto the BMC in various ways such as from
programming a flash chip, programming the BMC with a specialized
device, or as part of an OpenBMC software update function.

### Device file interfaces

OpenBMC uses standard device drivers and techniques to populate device
files such as /dev and /sys/devices.  These files provide direct
control of ethernet controllers, devices on I2C busses, and hardware
the host board provides.  OpenBMC uses device tree source (DTS) files
to help populate the device files.

The device files are accessible to anyone with root access to the BMC
(capabilities CAP_DAC_OVERRIDE or CAP_SYS_RAWIO), which includes many
phosphor applications, including those that populate D-Bus objects.

### D-Bus interfaces

D-Bus providdes the primary interface for all the resources the BMC
monitors and controls.

Specifically, OpenBMC uses D-Bus's system bus, with services and
objects that contain `openbmc_project` as part of their name.  There
is no access control, so any code running unprivileged on the BMC has
full access.

OpenBMC uses systemd to start many of the applications that translate
between device files and D-Bus objects.  These applications create and
manage the D-Bus objects.

### URIs and REST API interfaces

OpenBMC provides URIs via REST API servers.  Applications such as in
repo phosphor-rest-server map all D-Bus objects directly to URIs,
while the application in repo bmcweb maps D-Bus objects and other
resources onto the Redfish specification.

Security aspects of the Redfish specification can be found
[here](https://www.opencompute.org/wiki/Hardware_Management/SpecsAndDesigns).

HTTP or web servers are typically used to serve URIs.  Their security
aspects are well-known.  On OpenBMC, these servers are either used
internally (access is restricted by a firewall) or go through a
secure reverse proxy server for use outside the BMC.

The REST APIs require HTTP authentication techniques.  OpenBMC offers
basic Linux user authentication and is planning a [user management
service](https://github.com/openbmc/docs/blob/master/user_management.md).

### External interfaces

External interfaces include intended and unintended communication
channels with agents outside the BMC such as network communication and
communication with the host system.  This section focuses on
communication that happens through those channels, not the device used
to perform the communication.

The [OpenBMC Server Security Architecture](./obmc-securityarchitecture.md)
document lists all of the BMC's communication channels.  Because the
channels vary by hardware model, this section focuses on the common
aspects.  Guidance for downstream development teams: Repeat this
section for your specific application.

Host-facing interfaces include:
 - Board elements such as fans and temperature sensors that have a
   trivial interface.
 - Field replaceable unit (FRU) devices
 - Host-firmware facing elements such as host firmware updates, host
   IPMI commands
 - Mailbox (mbox) communication
 - Host console access via SOL and UART
 - Host video communication
 - RCMP+
 - KVM
 - Media redirection
 - Other host-facing elements???  TODO: help!

Communication through these channels is handled by the BMC's device
drivers.

Security aspects of communication with host-facing elements is
partially addressed by the Server Security Architecture document.

Outward-facing interfaces include:
 - IPMI
 - Avahi discovery services
 - SSH
 - REST APIs (if configured to go through the firewall)
 - Web application servers
 - Reverse proxy web server
 - TODO: More?  USB, serial communications, video

The security aspects of Linux network services are well-known.

Note that the default configuration is for the REST APIs and web
servers to send traffic through a reverse proxy server.  When used,
the the reverse proxy server is the only http-based communication
allowed through the firewall and is provided by a relatively more
secure server such as nginx.


## OpenBMC firmware elements

This section identifies the major elements of OpenBMC firmware images
and how they relate to each other and the interfaces identified above.
The elements include the applications that create and use D-Bus objects, web
servers, etc.

TODO: Once again, explain that variations in the list will occur and
provide guidance for downstream teams.

TODO: List OpenBMC applications by collecting them from systemd config
files.  Some of them will be from OpenBMC repos, and some will be
built into Linux.  Partition the list into sections: system services,
applications that provide D-Bus objects, applications that consume and
control D-bus objects, and the external interfaces.
