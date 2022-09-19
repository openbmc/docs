# Network Configuration Design

Author:
  Raviteja Bailapudi <rbailapu@in.ibm.com>

Other contributors:
  Sunitha Harish <sunharis@in.ibm.com>
  Ashmitha <asmitk01@in.ibm.com>

Created: 13/09/2022

## Problem Description
Any composed system should be managable via an external management client and
there should be a way to remotely connect to the system.
On the composite systems, this network configuration scope can be at composed
system level or limited to satellite BMC level. Therefore, there should be an
orchestration mechanism on the composite systems to coordinate between different
BMCs to configure complete system network.
This configuration includes commands to list, modify and delete network
parameter values

## Glossary

## Background and References

#### Scope:

## Requirements
- One network manager service per BMC, and one among them will be the SCA
  network manager
- All BMCs are required to notify SCA network manager when its network settings
  changes are complete
- SCA needs to aggregate network configuration from all satellite BMCs.
- All satellite BMCs should know IP configuration of all other BMCs in the
  composed system
- Any manual configuraions operation on satellite BMCs will be forwarded by the
  SCA to respective BMCs.
- Support all network configurations (Static IPv4/IPv6 configurations and DHCPv4
  and DHCPv6)
- Allow VLAN configuration on BMCs via SCA
- Support Static route configuration on SCA
- Allow HostName configuration on SCA
- Allow static and dynamic nameserver configurations
- Management client should be given only the externally connectable BMC physical
  interface configuration details. This should be the SCA network
- All satellite BMC network information is contained within the composed system
- Static route configurations should be supported to allow or route to the
  required BMC endpoint when all BMCs are in same subnet

## Assumptions
- SSDP protocal takes care of peer discovery of BMCs
- SCA BMC may not terminate external ethernet interfaces always

## Proposed Design
Current systemd-networkd and phosphor-networkd network management for IPv4 and
IPv6 types is directly consumable as-is

Aggregating the satellite BMC network interface details will be part of the
redfish aggregation design.

Static route configuration proposal is at
https://github.com/DMTF/Redfish/issues/5210

## Alternatives Considered
TBD
## Impacts
TBD
## Testing
TBD

