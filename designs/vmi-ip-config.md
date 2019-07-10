# VMI IP Address Configuration

Author:
  Sunitha Harish

Primary assignee:
  Sunitha Harish

Other contributors:
  None

Created:
  07/10/2019

## Problem Description
The Hardware Management Console can directly talk to the PHYP using the
Virtual Management Interface.
When the CEC initially powered on, the VMI interface would not have any network
configurations. The HMC can configure and manage the VMI network. This design
will describe how the HMC can manage the VMI network.

## Background and References
This is part of the HMC-BMC interface design.

## Requirements
1.BMC will provide an interface to allow the admin to get and set the VMI
network configuration.
  1.1 Support static/dynamic IPv4/IPv6
2.BMC will provide an interface for HMC to get the VMI network configuration
 (VMI discovery)
3.Any state required across a reboot must be provided by firmware
  3.1 IP address configuration

## Proposed Design
The HMC can configure the VMI network interface via BMC. This can be a Static
Configuration or Dynamic Configuration via DHCP.  The network can be IPv4/IPv6.
The BMCs Redfish interface will provide the GET and PATCH interfaces to the client
which intends to get/configure the host interface details.

### Persistency
The VMI configuration data will be persistent across the BMC reboot.

### Get the Host Interfaces
The Redfish BMC Manager resource will hold the HostInterfaces parent resource.

Request:
curl -k -H "X-Auth-Token: $bmc_token" -X GET
https://${bmc}/redfish/v1/Managers/bmc/HostInterfaces

Response:
This will return the Host Interface Collection. As per the redfish DMTF schema
http://redfish.dmtf.org/schemas/v1/HostInterfaceCollection.json#/definitions/HostInterfaceCollection

Further GET on the listed host interface IDs will give the interface details.

Request:
curl -k -H "X-Auth-Token: $bmc_token" -X GET
https://${bmc}/redfish/v1/Managers/bmc/HostInterfaces/<Id>

This will return the Host Interface details of the specific interface. As per the
redfish DMTF schema:
http://redfish.dmtf.org/schemas/v1/HostInterface.json#/definitions/HostInterface

### Get the Ethernet Interfaces supported on the specific HostInterface
This HostInterface property gives a reference to the collection of Ethernet interfaces
associated with this host.Further GET on this HostEthernetInterfaces property will provide
the Interface details for this host.

Request:
curl -k -H "X-Auth-Token: $bmc_token" -X GET
https://${bmc}/redfish/v1/Managers/bmc/HostInterfaces/<Id>/EthernetInterfaceCollection

Further GET on the listed Ethernet Interface members will give the details of that
Interface. This will be the PHYPs Virtual Machine Interface (VMI).

The response will be as per the redfish DMTF schema
http://redfish.dmtf.org/schemas/v1/EthernetInterfaceCollection.json#/definitions/EthernetInterfaceCollection
http://redfish.dmtf.org/schemas/v1/EthernetInterface.json#/definitions/EthernetInterface

### Get/Discover VMI IP
The below Redfish command will fetch the host interface network configuration -the VMI IP configuration.

Request:
curl -k -H "X-Auth-Token: $bmc_token" -X GET
https://${bmc}/redfish/v1/Managers/bmc/HostInterfaces/<Id>/EthernetInterfaceCollection/<Id>

The reponse will contain:
1. IPv4 address
2. IPv6 address
3. DHCP configurations
4. MAC address
and so on.

### Configure/Update the VMI Static IP
The below Redfish command should be used to configure the VMI IP IPv4 address.
curl -k -H "X-Auth-Token: $bmc_token" -X PATCH
https://${bmc}/redfish/v1/Managers/bmc/HostInterfaces/<Id>/EthernetInterfaceCollection/<Id>
-d '{"IPv4StaticAddresses": [{"Address": <>,"AddressOrigin":<>,"SubnetMask":<>,"Gateway":<>}]}'

The data will be the list of IPv4 address. However the PHYP can support only single VMI IP.
Thus the last updated IPv4 address will be the functional IP.

Similarly the command to configure the IPv6 address is below.
curl -k -H "X-Auth-Token: $bmc_token" -X PATCH
https://${bmc}/redfish/v1/Managers/bmc/HostInterfaces/<Id>/EthernetInterfaceCollection/<Id>
-d '{"IPv6StaticAddresses": [{"Address": <>,"PrefixLength":<>}]}'

Response:
  Success – The json response with 200 OK status
  Failure – The json response with 4xx or 5xx status. The message field will contain the
            failure reason description

### Configure the VMI Dynamic IP
The Dynamic VM IP will be assigned by the DHCP server connected to the BMC.
The server can be the HMC or any other external DHCP server.

HMC can enable the DHCP for the VMI interface using the below command.

curl -k -H "X-Auth-Token: $bmc_token" -X PATCH
https://${bmc}/redfish/v1/Managers/bmc/HostInterfaces/<Id>/EthernetInterfaceCollection/<Id>
-d '{"DHCPv4Configuration" : {"DHCPEnabled":true}}'

### Delete the VMI IP
The HMC can remove the IP address configuration by sending all zeros for the network parameters
in the below PATCH request.

### Dbus objects for the VMI IP configurations
The HostInterface, host0 will be created by default with all zeros assigned for the
default parameters.At BMC standby, before the first power on of the CEC, the VMI Interface
IP parameters will be set to all zeros.
When the system is powered on initially, HMC should setup the VMI
interface to PHYP via BMC. HMC should configure single IP address to the VMI since the
PHYP has the limitation to work with only single IP for its interface.

YAML specification:
The IP address object specification is published at
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Network/IP.interface.yaml

The below example shows the dbus object holding the Host Network details.
{
  "data": {
    "/xyz/openbmc_project/network/host0/eth0": {
      "MACAddress": 00:00:00:00:00:00,
      "DHCPEnabled": true/false,
      …
      …
    },
    "/xyz/openbmc_project/network/host0/eth0/ipv4/<objectId>": {
      "Address": 0.0.0.0,
      "Gateway": 0.0.0.0,
      "Origin": Static/DHCP,
      "Type": "xyz.openbmc_project.Network.IP.Protocol.IPv4"
    },
    "/xyz/openbmc_project/network/host0/eth0/ipv6/<objectId>": {
      "Address": <>,
      "PrefixLength":<>
      "Type": "xyz.openbmc_project.Network.IP.Protocol.IPv6"
    }
  },
  "message": "200 OK",
  "status": "ok"
}

Similarly the second VMI interfaces will be listed under the
/xyz/openbmc_project/network/host0/eth1 and so on based on the number of interfaces
supported by the CEC.

## Alternatives Considered
1. REST Interface comands to GET and PATCH the host interface parameters.
This option is dropped since we have a Redfish schema for the HostInterface configuration.

2. Since this VMI IP setup is specific functionality involving PHYP-BMC and HMC, we could have
considered moving it under HMC "specific" URI, like : "/xyz/openbmc_project/HMC/VMInterface"
This idea is dropped since we already had the "/xyz/openbmc_project/network/host0/" object at BMC.

## Impacts
The exisitng "/xyz/openbmc_project/network/host0/" object would need modification to remove the
"intf/addr" . New objects are to be created to hold the Ipv4 and IPv6 objects.

## Testing
Test the interface commands from a redfish client/HMC and verify if the network settings
are working as expected.
Test the responses with the Redfish schema validator.
Test the network file is updated when the IP address is configured/modified.
Test the persistency of the IP address configuration by rebooting the BMC.
Test the IP address gets updated with enabled for the DHCP.

##Acronyms
VMI    - Virtual Management Interface
BMC    - Baseboard Management System
HMC    - Hardware Management System
