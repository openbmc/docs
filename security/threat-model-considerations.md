BMC Threat Model Considerations

Purpose: Here are considerations for an OpenBMC threat model which
holds the BMC to be of value along with its access to its host and
management network.  It begins by introducing OpenBMC and the BMC's
operational environment.  It then outlines OpenBMC’s operations, some
threats to the BMC because of those operations, and steps OpenBMC
takes to address those threats.  It also describes BMC use cases.

This is only intended to be a guide; security is ultimately the
responsibility of projects which use OpenBMC.  If you find a security
vulnerability, please report it.  Here is [how to report a security
vulnerability][].

[Wikipedia Threat model](https://en.wikipedia.org/wiki/Threat_model)
[OWASP Application Threat Modeling](https://www.owasp.org/index.php/Application_Threat_Modeling).
[how to report a security vulnerability]: https://github.com/openbmc/docs/blob/master/security/how-to-report-a-security-vulnerability.md

NOTE TO REVIEWERS:
 - This is a work in progress.  Please help shape this document for
   your security assurance scheme.
 - This is an early draft, a starting point to identify assumptions
   about BMC architecture, and design and configurations.  Many of
   pieces are missing.  Most of the threats are missing too.  Please
   help by saying what you want this document to cover.


## BMC definition

NOTE TO REVIEWERS: This section aims to describe BMCs:
 - Without assuming prior BMC domain knowledge, so we can identify
   assumptions from the ground up.
 - In a universally applicable way, so we can include all foreseeable
   uses for OpenBMC.
 - Articulating required, alternate, and optional elements at the
   correct level of detail, in an attempt to systematically identify
   all security considerations.

NOTE TO REVIEWERS: Please help by asking questions that help define
what a BMC is and what OpenBMC is for.

OpenBMC is a source code distribution
- based on Yocto/OpenEmbedded Linux
- for a Baseboard Management Controller ([BMC][]) which
- is an [embedded system][] (typically a [system on a chip][]) that
- provides "platform management" services for machines like [Server Computers][]
- Using standards like [IPMI][], [MCTP][], and [Redfish][].

[BMC]: https://en.wikipedia.org/wiki/Baseboard_management_controller
[embedded system]: https://en.wikipedia.org/wiki/Embedded_system
[system on a chip]: https://en.wikipedia.org/wiki/System_on_a_chip
[Server Computers]: https://en.wikipedia.org/wiki/Server_(computing)
[Redfish]: https://en.wikipedia.org/wiki/Redfish_(specification)
[IPMI]: https://en.wikipedia.org/wiki/Intelligent_Platform_Management_Interface
[MCTP]: https://en.wikipedia.org/wiki/Management_Component_Transport_Protocol

The following subsections introduce significant aspects of BMCs and
OpenBMC:
 - The BMC’s operational environment.
 - Hardware OpenBMC runs on and machines the BMC is intended to service.
 - OpenBMC’s major functions and interfaces.
 - Basic trust and authorization models.

### Operational environment

The following diagram shows a simplified view of the BMC’s operational
environment.  It shows the BMC embedded in its host machine and
attached to the BMC’s management network.  The diagram omits all
sub-components, connections to the host, and host elements (including
the host’s network connection).  Subsequent diagrams will reference
this and show more details.

```
            +----------+
            | Host     |
            |          |
            +-----+    |
  Network --+ BMC |    |
            +-----+    |
            +----------+
```

### Hardware

No specific hardware technology (processor, memory, etc.) is assumed
for the BMC or its host machine.  The BMC must be capable of running
Linux.  The BMC may be supported by its host (for example, power) and
is directly connect to it via GPIOs and buses, with support for
protocols such as I2C, but no specific technology is assumed.

The OpenBMC project organizes hardware support under various
https://github.com/openbmc meta-* repositories as described in the
[OpenBMC project README][].  The TEMPLATECONF parameter references the
target machine (the BMC’s host machine) and each machine references
the BMC it uses.

[OpenBMC project README]: https://github.com/openbmc/openbmc/blob/master/README.md

Flash.  The BMC is assumed to have flash (or similar) persistent storage.
 1. No specific connection between the BMC and flash is assumed.
    Typical configurations are directly connected, but a TPM can mediate
    access.
 2. No specific flash layout is assumed.  Flash is typically
    partitioned into read only file systems for firmware images, and
    read/write file systems to store logs, configurations, etc.
 3. Without read/write storage some functions may not work or may work
    differently, for example, configuration changes may not persist.
 4. No specific storage technology is assumed.  Wearing out the flash
    may cause the BMC to fail.
 5. Specialized designs may omit flash storage and rely on netboot,
    remote logging, and the like.

Network.  The BMC is assumed to have a network adapter.  The host and
BMC may share an adapter, but the idea is the BMC has a network
connection which is independent from its host.

### OpenBMC external interfaces

OpenBMC provides functions via network interfaces in the form of:
- IPMI
- Redfish REST APIs
- Other REST APIs
- Web application
- Secure Shell
Along with a facility to provide the following over WebSockets:
- Host serial console
- Host keyboard, video, mouse (KVM)
- Remote media

This list is representative of BMC support.  BMC implementations may
support more or fewer interfaces.

Note that the BMC may offer unintended hardware or software
interfaces.  For example, BMC hardware debug ports.  Some
installations may provide Secure Shell (SSH) access but do not intend
for it to be used to operate the BMC.  These can be used to attack the
BMC, so they are included.

### BMC Functions

This shows OpenBMC's core functions. Any given implementation may
provide more or fewer functions.

OpenBMC has functions to support host elements including:
- Host KVM (keyboard, video, and mouse)
- Host serial interface (typically for the host console)
- Power and cooling management
- Sensors
- LEDs
- Host inventory (FRUs) See [entity manager documenrtation][].
- Host firmware update
- Handle host error logs (SELs)
- Host power on

Functions are documented with the OpenBMC [features documentation][].

[entity manager documentation]: https://github.com/openbmc/entity-manager/blob/master/README.md
[features documentation]: https://github.com/openbmc/docs/blob/master/features.md

### Trust boundaries

This section describes the BMC’s security domain and related trust
boundaries.  OpenBMC is designed for the BMC to be in its own trust
domain and not trust either its host or its management network.

The BMC’s security domain:
- Includes the BMC itself.
- Generally includes the BMC’s memory and flash.
- Generally includes and ends at the NIC, which may be shared with the host.
- Includes and ends with I/O like GPIO and buses.
- May interact with a device like a trusted platform module (TPM).

The BMC and its host have a special relationship:

1. The BMC is explicitly trusted to operate the host’s firmware-level
   components.  Specifically:
    1. To operate the hardware elements identified above.
    2. Host firmware creates SELs without the BMC’s involvement, and
    only invokes the BMC to store them.

2. This special relationship ends where the host function begins.
   Specifically:
    1. The host may manage its own power and thermal controls once it
       is running, which transitions control from the BMC to the host.
    2. The BMC should have no access to the host beyond the elements
       identified above.
    3. The BMC should have no access to configure, operate, or access
       host elements such as BIOS, hypervisor, memory, storage, and
       similar devices.  If these functions are performed by the BMC,
       they are outside the special relationship and constitute an
       additional level of management not provided by OpenBMC common
       function.

3. The BMC does not necessarily trust the host.
    1. Root access to the host or host firmware does not necessarily
       grant authority to operate the BMC beyond their special
       relationship.
    2. Host IPMI access does give the host access to the BMC.
    3. The host’s access to the BMC, if present, it intended to be
       limited to the BMC’s administrative controls, and not to its
       underlying implementation.  Examples: memory, storage, internal
       APIs.

The BMC is assumed to be connected to a network (termed the "BMC's
management network") from which it is normally operated, for example,
via Redfish REST APIs or the Web interface.  Network security
considerations are described (TODO: in review, proposed:
docs/security/network-security-considerations.md)
[here](https://gerrit.openbmc-project.xyz/c/openbmc/docs/+/22106) and
include authentication and authorization topics.


## Uses Cases

NOTE TO REVIEWERS: This section is little more than a collection of
ideas.  Please help identify, explain, and organize use cases.  A
sentence or two will help, saying how you use your BMC.

OpenBMC supports multiple use cases.  Taxonomy:
1. BMC management access:
    1. BMC management network attached a single computer
    2. BMC management network attached to a private network
    3. BMC management from its own host (e.g., unauthenticated inband
       IPMI), uses whitelisted IPMI commands or restriction mode

2. Trust domains: Separate agents for (a) network, (b) BMC, (c) host
   function.  For example,
    1. BMC and host in same trust domain.
    2. Host does not trust the BMC.  Block attacks from the BMC to the
       host.
    3. BMC does not trust the host.  Block attacks from the host to
       the BMC.
    4. BMC as pass through to host console or hypervisor.

3. Modes of operations
    1. TODO: Include use cases for development, provisioning,
    configuring, servicing, change of ownership, and disposal.
    2. See also
       https://gerrit.openbmc-project.xyz/c/openbmc/docs/+/21195
    3. Using or restricted by a TPM?
    4. Using Secure Boot?
    5. Technology like Linux Integrity management architecture (IMA)

4. Service strategy:  Refers to servicing the BMC itself.
    1. Keep the host running
    2. Move work to another node

5. Operating environment infrastructure:
    1. None beyond person using Web interface.
    2. Available LDAP or authentication server, logging server, NTP
       server, etc.
    3. Fully automated agents such as xCAT, with no Web usage.
