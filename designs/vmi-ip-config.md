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
1. BMC will provide an interface to allow the admin to get and set the VMI network configuration.
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
which intends to get/configure the host interface details.

#### Persistency
The VMI configuration data will be persistent across the BMC reboot.

#### Get the Host Interfaces

Request:
```
curl -k -H "X-Auth-Token: $bmc_token" -X GET
https://${bmc}/ibm/v1/VMInterface/network/enumerate
```
Response:

The response will contain:
1. IPv4 address details
2. IPv6 address details
3. DHCP configuration
4. The physical location code of the network port


```
{
  "data": {
    "/ibm/v1/VMInterface/network/config":{
      "IPv4DefaultGateway":"",
    },
    "/ibm/v1/VMInterface/network/<Id>": {
      "DHCPEnabled": true/false,
      "PhysicalLocationCode": ""
    },
    "/ibm/v1/VMInterface/network/<Id>/ipv4/<objectId>": {
      "Address": "",
      "Origin": static/dynamic,
      "PrefixLength": <>,
      "Gateway": "",
      "Type": "IPv4"
    },
  "/ibm/v1/VMInterface/network/<Id>/ipv6/<objectId>": {
      "Address": "",
      "Origin": static/dynamic,
      "PrefixLength": <>
      "Gateway": "",
      "Type": "IPv6"
    }
  },
  "message": "200 OK",
  "status": "ok"
}
```

#### Configure the VMI Static IP
Below REST command should be used to configure the static VMI IP address. The parameters are as per the YAML specification:
The IP address object specification is published at
```
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Network/IP.interface.yaml
```
Request:
```
curl -c cjar -b cjar -k -H "Content-Type: application/json"
-H "X-Auth-Token: $bmc_token -X PUT https://${bmc}/ibm/v1/VMInterface/network/<Id>/IP
-d '{"data": ["IPv4", "address", "prefixlength", "gateway", "subnetmask"]}'
```
Response:

- Success – The JSON response with 200 OK status. The data field will contain the object which is created at the BMC to hold the details of this IP address configuration.
```
{
  "data": "ibm/v1/VMInterface/network/<Id>/ipv4/<objectID>",
  "message": "200 OK",
  "status": "ok"
}
```
- Failure – The JSON response with 4xx or 5xx status. The message field will contain the failure reason description. For example:
```
{
  "data": {
    "description": "No JSON object could be decoded"
  },
  "message": "400 Bad Request",
  "status": "error"
}
```

#### Configure the VMI Dynamic IP
The Dynamic VM IP will be assigned by the DHCP server connected to the BMC.
The DHCP server can be the HMC itself or any other external DHCP server running in the BMC's subnet.

DHCP for the VMI interface can be enabled using the below command.

```
curl -c cjar -b cjar -k -H "Content-Type: application/json"
-H "X-Auth-Token: $bmc_token" -X PUT
-d "{\"data\": 1}" https://${bmc}/ibm/v1/VMInterface/network/<Id>/attr/DHCPEnabled
```

#### Delete the VMI IP
The HMC can remove the IP address configuration by sending the DELETE request to the IP object Id which needs to be deleted.

Request:
```
curl -c cjar -b cjar -k -H "Content-Type: application/json"
-H "X-Auth-Token: $bmc_token" -X DELETE
https://${bmc}/ibm/v1/VMInterface/network/<Id>/ipv4/<objectId>
```
Response:

- Success – The JSON response with 200 OK status
```
{
  "data": null,
  "message": "200 OK",
  "status": "ok"
}
```
- Failure – The JSON response with 4xx or 5xx status. The message field will contain the failure reason description.

## D-Bus objects for the VMI IP configurations
At BMC standby, before the power on of the CEC, the VMI Interface IP parameters will be initialized to all zeros.

When the system is powered on initially, HMC should setup the VMI interface to PHYP via BMC , provided the DHCP is turned off. Its recommended to configure single IP address to the VMI since PHYP has the limitation to work with only single IP for its interface.

If VMI is configured with multiple IP addresses, the first IP in the list will be the active IP.

YAML specification for the IP address:
```
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Network/IP.interface.yaml
```
The below example shows the D-Bus object holding the Host Network details.
```
{
  "data": {
    "/ibm/v1/VMInterface/network/config":{
      "IPv4DefaultGateway":"",
    },
    "/ibm/v1/VMInterface/network/<Id>": {
      "DHCPEnabled": true/false,
      "PhysicalLocationCode": ""
    },
    "/ibm/v1/VMInterface/network/<Id>/ipv4/<objectId>": {
      "Address": "",
      "Gateway": "",
      "Origin": "",
      "PrefixLength": <>
      "Type": "IPv4"
    },
  "/ibm/v1/VMInterface/network/<Id>/ipv6/<objectId>": {
      "Address": "",
      "Gateway": "",
      "Origin": "",
      "PrefixLength": <>
      "Type": "IPv6"
    }
  },
  "message": "200 OK",
  "status": "ok"
}
```

## Alternatives Considered
1. Redfish Interface commands to GET and PATCH the host interface parameters.
   - This option is dropped since this is a IBM only usecase for configuring the interface for the VMI, provided for the HMC user only.

## Impacts
None

## Testing
1. Verify the interface commands from a REST client/HMC and verify if the network settings are working as expected.
2. Verify the network file is updated when the IP address is configured/modified.
4. Verify the persistence of the IP address configuration by rebooting the BMC.
5. Verify the IP address gets updated with the DHCP provided IP address when the VMI is enabled for the DHCP.



