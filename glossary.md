# Glossary

Guidelines:

- Include terms specific to the OpenBMC project, OpenBMC reference platforms
  (like OpenPower or ASpeed), BMCs, or platform management.
- Include terms needed to disambiguate. For example, "image" may refer to a
  visual (e.g., JPEG) image or a firmware (e.g. UBIFS) image.
- Treat acronyms the same as terms.

BMC - Baseboard management controller. A device designed to enable remote out of
band management access to a host, generally a computer system.

D-Bus - Provides the primary mechanisms for inter-process communication with an
OpenBMC system. OpenBMC D-Bus APIs are documented in
`https://github.com/openbmc/phosphor-dbus-interfaces`. For example, see the tree
under `/xyz/openbmc_project/Led`.

IPMI - Intelligent Platform Management Interface. OpenBMC implements a subset of
the IPMI spec.

ODM - Original design manufacturer. In OpenBMC, ODM generally refers to the
manufacturer of the host system.

OEM - Original equipment manufacturer. In OpenBMC, OEM generally refers to the
manufacturer of the host system.

Phosphor - Informally designates OpenBMC software or APIs designed for use
across many systems, as distinct from platform or vendor-specific elements.

Redfish - The Distributed Management Task Force (DTMF) Redfish specification.
OpenBMC provides Redfish REST APIs for platform management.

REST - Representational State Transfer. OpenBMC provides REST APIs.

SDR - IPMI Sensor Data Record.

SEL - IMPI System Event Log. The BMC stores SEL events.

Server - A computing device, generally the one served by the BMC.
