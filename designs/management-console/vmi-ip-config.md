# Virtual Management Interface IP Address Configuration

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
When the System initially powered on, the VMI interface would not have any network
configurations. The HMC can configure and manage the VMI network.

This design will describe how the HMC can manage the VMI network.

## Glossary
- BMC    - Baseboard Management Controller : A service processor which monitors and controls the management services of the servers.
- HMC    - Hardware Management Console : IBM Enterprise system which provides management functionalities to the server.
- PHYP   - Power Hypervisor : This orchestrates and manages system virtualization.
- VMI    - Virtual Management Interface : The interface facilitating communications between HMC and PHYP's embedded linux virtual machine.


## Background and References
This is part of the HMC-BMC interface design.

## Requirements
1. BMC will provide an interface to allow the admin to set the VMI network configuration.
   - Support static & dynamic IPv4/IPv6
   - Support changing the VMI IP address at system standby and runtime
2. BMC will provide an interface for HMC to get the VMI network configuration(VMI discovery)
3. Any state required across a reboot must be provided by firmware
   - IP address configuration

## Proposed Design
The HMC can configure the VMI network interface via BMC. This can be a Static
Configuration or Dynamic Configuration via DHCP.  The network can be IPv4/IPv6.

The configuration of new IP address can be done when the BMC is at minimum standby state.

The BMC's REST interface will provide the GET and PATCH interfaces to the client
which intends to get/configure the VMI interface details.

#### Design Flow - Static IP Configuration
```ascii
    +------------+        +--------+            +--------+             +--------+
    |    HMC     |        | BMCweb |            |  PLDM  |             |  VMI   |
    |  (client)  |        |        |            |        |             |        |
    +-----+------+        +----+---+            +---+----+             +---+----+
          |                    |                    |                    |
          |                    | standby            |                    |
          +------------------->+                    |			    	 |
          | VMI IP Configure   |                    |					 |
          +<-------------------|           			|					 |
          | 202 Accepted       |                    |					 |
          |                    +------------------->+	                 |
          |                    |   Dbus update      |                    |
          |                    | 					+------------------->|
          |                    | 					|  pldm call to host |
          |                    |                    |                    | Apply the IP
          |					   |                    |                    | address
          |                    |                    |                    |
          |                    |                    +<-------------------|
          |                    |                    |  IP applied        |
          |                    |                    |     successfully   |
          |                    +<-------------------|                    |
          |                    |   Trigger Event    |                    |
          +<-------------------|                    |                    |
          |  Send Event to HMC |                    |                    |
          |                    |                    |                    |
          +                    +                    +                    +

```
BMC will accept the valid IP configuration request from the HMC and it will respond 202 back, letting the HMC know that the configuration is accepted. At this moment, the VMI partition is yet to consume this new configuration. The BMCWEB will save this configuration values in the Dbus objects.
The PLDM will forward the configuration to the VMI partition.
Once the VMI partition applies the new IP address configurations, it notifies the BMC that the VMI is active with new configuration.
This will be forwarded to the HMC as an event.

#### Design Flow - Dynamic IP Configuration
When the VMI is connected to either DHCPv4 or DHCPv6 server, it receives the IP address from the server and needs to push the renewed network details to the BMC.
These network changes will be sent out to the HMC as events, and monitoring which the HMC should use the latest network configuration which is active for VMI.


#### Persistency
The VMI configuration data will be persistent across the BMC reboot.

#### Get the VMI network details

The below command will route to the network details of the VMI.

Request:
```
curl -k -H "X-Auth-Token: $bmc_token" -X GET
https://${bmc}/redfish/v1/Systems
```
Response:
```
{
    ...
  "Members": [
    ...
    {
      "@odata.id": "/redfish/v1/Systems/hypervisor"
    }
  ],
    ...
}
```
The "hypervisor" system will model the Power Hypervisor. This resource will follow the DMTF Redfish standard resource - ComputerSystems.

Request:
```
curl -k -H "X-Auth-Token: $bmc_token" -X GET
https://${bmc}/redfish/v1/Systems/hypervisor
```
Response:
```
{
  "@odata.id": "/redfish/v1/Systems/hypervisor",
  "@odata.type": "#ComputerSystem.v1_6_0.ComputerSystem",
  "Description": "Power Hypervisor",
  "EthernetInterfaces": {
    "@odata.id": "/redfish/v1/Systems/hypervisor/EthernetInterfaces"
  },
  "Id": "hypervisor",
  "Links": {
    "ManagedBy": [
      {
        "@odata.id": "/redfish/v1/Managers/bmc"
      }
    ]
  },
  "Name": "Power Hypervisor"
}
```
The "hypervisor" system will be the model the DMTF Redfish standard's "EthernetInterfaceCollection" resource for the collection of VMI network interfaces. 

GET operation on this collection resource will give the resource path for the individual VMI network resource.

Request:
```
curl -k -H "X-Auth-Token: $bmc_token" -X GET
https://${bmc}/redfish/v1/Systems/hypervisor/EthernetInterfaces
```
Response:
```
{
  "@odata.id": "/redfish/v1/Systems/hypervisor/EthernetInterfaces",
  "@odata.type": "#EthernetInterfaceCollection.EthernetInterfaceCollection",
  "Description": "Collection of Virtual Management Interfaces for the power hypervisor",
  "Members": [
    {
      "@odata.id": "/redfish/v1/Systems/hypervisor/EthernetInterfaces/eth0"
    },
    {
      "@odata.id": "/redfish/v1/Systems/hypervisor/EthernetInterfaces/eth1"
    }
  ],
  "Members@odata.count": 2,
  "Name": "Virtual Management Ethernet Network Interface Collection"
}
```
Further GET on these members will give the details on the interfaces.
This response will follow the DMTF Redfish standard resource EthernetInterface.
```
https://redfish.dmtf.org/schemas/v1/EthernetInterface.v1_5_1.json
```
Request:
```
curl -k -H "X-Auth-Token: $bmc_token" -X GET
https://${bmc}/redfish/v1/Systems/hypervisor/EthernetInterfaces/eth0
```
Response:

```
{
  "@odata.id": "/redfish/v1/Systems/hypervisor/EthernetInterfaces/eth0",
  "@odata.type": "#EthernetInterface.v1_5_1.EthernetInterface",
  "Description": "Virtual Interface Management Network Interface",
  "HostName": "",
  "IPv4Addresses": [
    {
      "Address": "",
      "AddressOrigin": "Static/DHCP/IPv4LinkLocal",
      "Gateway": "0.0.0.0",
      "SubnetMask": ""
    },
  ],
  "IPv4StaticAddresses": [
    {
      "Address": "",
      "AddressOrigin": "Static",
      "Gateway": "0.0.0.0",
      "SubnetMask": ""
    }
  ],
  "IPv6Addresses": [
    {
      "Address": "",
      "AddressOrigin": "LinkLocal/DHCPv6/SLAAC",
      "PrefixLength": <>
    },
    ...
    ...
  ],
  "IPv6DefaultGateway": "",
  "IPv6StaticAddresses": [
    {
      "Address": "",
      "AddressOrigin": "Static",
      "PrefixLength": <>
    },
  ],
  "Id": "eth0",
  "Name": "Virtual Management Ethernet Interface",
  "DHCPv4": {
    "DHCPEnabled": true/false,
    "UseDNSServers": true/false,
    "UseDomainName": true/false,
    "UseNTPServers": true/false
  },
  "DHCPv6": {
    "OperatingMode": "Enabled/Disabled",
    "UseDNSServers": true/false,
    "UseDomainName": true/false,
    "UseNTPServers": true/false
  },
  "MACAddress":"",
  "InterfaceEnabled": true/false
}
```

The eth1 resource of the VMI network will be same as the eth0 interface.

#### Configure the VMI Static IP
Below REST command should be used to configure the static VMI IP address.

Request:
```
curl -k -H "Content-Type: application/json"
-H "X-Auth-Token: $bmc_token -X PATCH -d '{"IPv4StaticAddresses": [{"Address": "","SubnetMask": "","Gateway":""}]}' https://${bmc}/redfish/v1/Systems/hypervisor/EthernetInterfaces/eth0
```
Response:

- Success – The JSON response with 202 OK status.
- Failure – The JSON response with 4xx or 5xx status. The message field will contain the failure reason description.

The property IPv4StaticAddresses is an array of IPv4Address objects. To modify the existing IP address, user need to send new values for that specific array element. To skip the current list of objects and to add new IP address to this list, the empty {} shall be used.

#### Configure the VMI Dynamic IP
The Dynamic VM IP will be assigned by the DHCP server connected to the host.
The DHCP server can be the HMC itself or any other external DHCP server running in the systems subnet.

DHCP for the VMI interface can be enabled using the below command.

```
curl -k -H "X-Auth-Token: $bmc_token" -X PATCH -D patch.txt -d '{"DHCPv4": {"DHCPEnabled": true}}' https://${bmc}/redfish/v1/Systems/hypervisor/EthernetInterfaces/eth0
```
Response:

- Success – The JSON response with 200 Accepted status
- Failure – The JSON response with 4xx or 5xx status. The message field will contain the failure reason description.

#### Delete the VMI IP
The HMC can remove the IP address configuration by sending the PATCH request with null value to the corresponding object in the IPv4StaticAddress list.

Request: To delete the first IP address if the interface contains three addresses configured.
```
curl -k -H "Content-Type: application/json"
-H "X-Auth-Token: $bmc_token" -X PATCH -d '{"IPv4StaticAddresses": [ null]}'
https://${bmc}/redfish/v1/Systems/hypervisor/EthernetInterfaces/eth0
```
Response:

- Success – The JSON response with 202 Accepted status
- Failure – The JSON response with 4xx or 5xx status. The message field will contain the failure reason description.

## D-Bus objects for the VMI IP configurations
When the system is powered on initially, HMC should setup the VMI interface to PHYP via BMC , provided the DHCP is turned off. Since the Power Hypervisor only supports a single IP address for an interface, it is recommended to only configure one IP address.

If VMI is configured with multiple IP addresses, the latest IP configured in the list will be the applied to the VMI.

The below are the D-Bus object holding the VMI Network details.
```
    /xyz/openbmc_project/network/vmi
        -Hostname
    /xyz/openbmc_project/network/vmi/intf0
        -MACAddress
        -DefaultGateway
        -DefaultGateway6
    /xyz/openbmc_project/network/vmi/intf0/ipv4/addr0
        -Address
        -Origin <IP::AddressOrigin::Static>
        -PrefixLength
        -Type <IP::Protocol::IPv4>
    /xyz/openbmc_project/network/vmi/intf0/ipv4/addr1
        -Address
        -Origin <IP::AddressOrigin::LinkLocal>
        -PrefixLength
        -Type <IP::Protocol::IPv4>
    /xyz/openbmc_project/network/vmi/intf0/ipv4/addr2
        -Address
        -Origin <IP::AddressOrigin::DHCP>
        -PrefixLength
        -Type <IP::Protocol::IPv4>
    /xyz/openbmc_project/network/vmi/intf0/ipv6/addr0
        -Address
        -Origin <IP::AddressOrigin::LinkLocal>
        -PrefixLength
        -Type <IP::Protocol::IPv6>
    /xyz/openbmc_project/network/vmi/intf0/ipv6/addr1
        -Address
        -Origin <IP::AddressOrigin::Static/IP::AddressOrigin::DHCP>
        -PrefixLength
        -Type <IP::Protocol::IPv6>
    /xyz/openbmc_project/network/vmi/intf0/ipv6/addr2
        -Address
        -Origin <IP::AddressOrigin::SLAAC>
        -PrefixLength
        -Type <IP::Protocol::IPv6>

   PS: The intf1 will contain similar objects as intf0

```

## Alternatives Considered
1. Modeling the hypervisor interface using the HostInterface property of the Manager schema. This is dropped because this property models the local host communication interface; and VMI is Not used for the local host communication. 
2. Use the /ibm/v1 path for the VMI. This is dropeed since we have a way to model this interface using the Redfish. Thus the GUI and the HMC can use the similar APIs for configuring the BMC and VMI ethernet interfaces.

## Impacts
None

## Testing
1. Verify the interface commands from the client/HMC and verify if the network settings are working as expected.
2. Verify the persistence of the IP address configuration by rebooting the BMC.
3. Verify the IP address gets updated with the DHCP provided IP address when the VMI is enabled for the DHCP.
4. Verify the notification is sent to the client once the configured IP address is applied at the VMI via the PHYP.
