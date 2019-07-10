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
This is part of the HMC-BMC interface design.The requirements are described at
the VMI HLDD - https://ibm.ent.box.com/file/298674992261. Section 3.1 and 3.5

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

### Persistency
The network file /etc/systemd/network/00-bmc-host0.network will keep all the VMI
network related data. This file will be persistent across the BMC reboot. The new 
VMI IP configuration from the HMC will be updated and persisted in this file.

### Get/Discover VMI IP
HMC should use the below REST command to fetch the host network configuration. 
This gives the complete details of the VMI IP configuration.

Request:
curl -c cjar -b cjar -k -H "Content-Type: application/json" -H "X-Auth-Token: $bmc_token"
-d "{\"data\": []}" -X GET https://${bmc}/xyz/openbmc_project/network/host0/enumerate

Response:
This will list all the host network configuration. The value “Origin” will indicate 
how the address is determined. For IPv4, this will be Static/DHCP.
For IPv6, this will be Global/LinkLocal address. 

{
  "data": {
    "/xyz/openbmc_project/network/host0/eth0": {
      "MACAddress": "",
      "DHCPEnabled": true/false,
      …
      …
    },
    "/xyz/openbmc_project/network/host0/eth0/ipv4/<objectId>": {
      "Address": "",
      "Gateway": "",
      "Origin": "",
      "PrefixLength":
      "Type": "xyz.openbmc_project.Network.IP.Protocol.IPv4"
    },
  "/xyz/openbmc_project/network/host0/eth0/ipv6/<objectId>": {
      "Address": "",
      "Gateway": "",
      "Origin": "",
      "PrefixLength":
      "Type": "xyz.openbmc_project.Network.IP.Protocol.IPv6"
    }
  },
  "message": "200 OK",
  "status": "ok"
}

Similarly the second VMI interfaces will be listed under the 
/xyz/openbmc_project/network/host0/eth1 and so on

### Get Specific VMI IP details
Below is the step by step GET of the host network configuration
curl -c cjar -b cjar -k -H "Content-Type: application/json" -H "X-Auth-Token: $bmc_token” 
-d "{\"data\": []}" -X GET https://${bmc}/xyz/openbmc_project/network/host0/eth0/

This will list the ipv4 and ipv6 addresses configured for the VMI as below.
{
  "data": [
    "/xyz/openbmc_project/network/host0/eth0/ipv4",
    "/xyz/openbmc_project/network/host0/eth0/ipv6"
  ],
  "message": "200 OK",
  "status": "ok"
}

Further GET on these objects will return the IPv4 and IPv6 object IDs at the BMC.

curl -c cjar -b cjar -k -H "Content-Type: application/json" -H "X-Auth-Token: $bmc_token” 
-d "{\"data\": []}" -X GET https://${bmc}/xyz/openbmc_project/network/host0/eth0/ipv4/
{
  "data": [
    "/xyz/openbmc_project/network/host0/eth0/ipv4/<objectId>",
  ],
  "message": "200 OK",
  "status": "ok"
}

curl -c cjar -b cjar -k -H "Content-Type: application/json" -H "X-Auth-Token: $bmc_token”
-d "{\"data\": []}" -X GET https://${bmc}/xyz/openbmc_project/network/host0/eth0/ipv6
{
  "data": [
    "/xyz/openbmc_project/network/host0/eth0/ipv6/<objectId>"
  ],
  "message": "200 OK",
  "status": "ok"
}

Further GET on these objectIDs will return the IPv4/IPv6 network details of the specific 
IP address.

curl -c cjar -b cjar -k -H "Content-Type: application/json" -H "X-Auth-Token: $bmc_token” 
-d "{\"data\": []}" -X GET https://${bmc}/xyz/openbmc_project/network/host0/eth0/ipv4/<objectId>
{
  "data": {
    "Address": "",
    "Gateway": "",
    "Origin": "",
    "PrefixLength":
    "Type": "xyz.openbmc_project.Network.IP.Protocol.IPv4"
  },
  "message": "200 OK",
  "status": "ok"
}

curl -c cjar -b cjar -k -H "Content-Type: application/json" -H "X-Auth-Token: $bmc_token” 
-d "{\"data\": []}" -X GET https://${bmc}/xyz/openbmc_project/network/host0/eth0/ipv6/<objectId>
{
  "data": {
    "Address": "",
    "Gateway": "",
    "Origin": "",
    "PrefixLength":
    "Type": "xyz.openbmc_project.Network.IP.Protocol.IPv6"
  },
  "message": "200 OK",
  "status": "ok"
}

### Configure/Update the VMI Static IP
At BMC standby, before the first power on of the CEC, the VMI Interface parameters will 
be set to all zeros.
When the system is powered on, initially HMC should setup the VMI interface to PHYP via BMC.
HMC can configure single IP address to the VMI since the PHYP has the limitation.

The below REST command should be used to configure the VMI IP address. This is the PATCH request
specifying either the IPv4 or the IPv6 address details as per the IP.interface.yaml
The IP address configured in this way will be the Static IP always.

YAML specification: 
The IP address object specification is published at 
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Network/IP.interface.yaml

Request:
curl -c cjar -b cjar -k -H "Content-Type: application/json" -H "X-Auth-Token: $bmc_token -X 
PATCH https://${bmc}/xyz/openbmc_project/network/host0/eth0/action/IP 
-d ‘{“data”: ["xyz.openbmc_project.Network.IP.Protocol.IPv4/IPv6", "address", "gateway","prefixlength"]}’

Response:
  Success – The json response with 200 OK status 
  {
    "data": {},
    "message": "200 OK",
    "status": "ok"
  }
  Failure – The json response with 4xx or 5xx status. The message field will contain the 
            failure reason description
 
### Configure the VMI Dynamic IP
The Dynamic VM IP will be assigned by the DHCP server connected to the BMC. 
The server can be the HMC or any other external DHCP server.

HMC can enable the DHCP for the VMI interface using the below command.

curl -c cjar -b cjar -k -H "Content-Type: application/json" -H "X-Auth-Token: $bmc_token"
-X PUT -d "{\"data\": 1}" https://${bmc}/xyz/openbmc_project/network/host0/eth0/attr/DHCPEnabled

### Delete the VMI IP
The HMC can remove the IP address configuration by sending all zeros for the network parameters
in the below PATCH request.

curl -c cjar -b cjar -k -H "Content-Type: application/json" -H "X-Auth-Token: $bmc_token -X
PATCH https://${bmc}/xyz/openbmc_project/network/host0/eth0/action/IP
-d ‘{“data”: ["xyz.openbmc_project.Network.IP.Protocol.IPv4/IPv6", "address", "gateway","prefixlength"]}’

## Alternatives Considered
Since this VMI IP setup is specific functionality involving PHYP-BMC and HMC, we could have
considered moving it under HMC "specific" URI, like : "/xyz/openbmc_project/HMC/VMInterface"
This idea is dropped since we already had the "/xyz/openbmc_project/network/host0/" object at BMC.

## Impacts
The exisitng "/xyz/openbmc_project/network/host0/" object would need modification to remove the 
"intf/addr" and move the IP address details under the new objects.

## Testing
Test the interface commands from a rest client/HMC and verify if the network settings
are working as expected.
Test the network file is updated when the IP address is configured/modified.
Test the persistency of the IP address configuration by rebooting the BMC.
Test the IP address gets updated with enabled for the DHCP.

