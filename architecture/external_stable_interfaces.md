# OpenBMC external and stable interfaces

Purpose: This introduces a simplified view of the BMC's external interfaces.

Intended Audience:

- Upstream Developers need to understand which parts of the OpenBMC firmware are
  stable and which parts can be changed for any reason at all.

- Users need to understand which interfaces provided by OpenBMC are stable and
  can be used to reliably 'build on top' of OpenBMC in terms of e.g.
  infrastructure automation.

- Downstream developers building on top of OpenBMC need to understand which
  parts are safe to 'build on top' of and which customizations are unstable,
  requiring review and testing when pulling newer versions of OpenBMC.

## Definitions

- interface: Any protocol, wire format, API, or externally observable behavior.
- external: Reachable from outside the BMC.
- internal: Not reachable from outside the BMC.
- stable: Governed by an external specification or project-level stability
  commitment.
- unstable: Subject to change at any time, for any reason.

## External stable network interfaces

These interfaces are reachable over the network and are intended for management,
automation, and integration with external systems.

- Redfish
  - HTTP(S)-based management API defined by the DMTF Redfish specification.
  - Primary long-term management interface for OpenBMC.
  - Stability governed by the Redfish specification.

- IPMI (network)
  - IPMI over RMCP+ as defined by the IPMI specification.
  - Includes command semantics defined by the IPMI standard.
  - Legacy but explicitly supported for compatibility.

- IPMI FRU Information
  - FRU data format as defined by the IPMI FRU Information Storage Definition.
  - Stability applies to the FRU binary layout and field semantics, not to how
    the data is produced internally.

- PLDM over MCTP (networked transports)
  - PLDM message formats and semantics as defined by DMTF PLDM specifications.
  - Applies where PLDM is exposed over a network-capable MCTP transport.

## External stable interfaces to the host

These interfaces are exposed by the BMC _to the managed host_ and are intended
to be consumed by host firmware or the host operating system.

- PLDM (host-facing)
  - PLDM message formats and semantics as defined by DMTF PLDM specifications.
  - Includes firmware update (FWUP) and other standardized PLDM types.
  - Transport details (e.g. MCTP binding) must conform to the relevant spec.

- PLDM firmware update packages
  - Firmware package format as defined by the PLDM FWUP specification.
  - Stability applies to package structure and semantics, not to internal
    handling.

- MCTP (host-facing)
  - Message transport behavior as defined by DMTF MCTP specifications.
  - Stability applies to protocol behavior, not to device topology or routing.

## List of unstable interfaces

The list is provided primarily for contrast with the above.

- Any command executed from a BMC shell
- Any interaction via an interactive SSH session
- Any file paths, configuration files, or on-disk formats on the BMC
- All D-Bus interfaces (including object paths, interfaces, properties, and
  signals)
- Any REST APIs not defined by the Redfish specification
- Any WebSocket or vendor-specific HTTP endpoints
