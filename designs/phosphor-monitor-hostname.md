# Generate a self-signed HTTPS certificate on BMC hostname assignment

Author: Alan Kuo

Primary assignee: Alan Kuo

Created: 2020-11-13

## Problem Description
In [OpenBMC/bmcweb](https://github.com/openbmc/bmcweb), a default HTTPS certificate is generated at startup, which enables 
TLS on the server however the certificate provides limited verifiable information. When
servers are first placed into the data center, we'd want the BMC to provide a certificate
that can be verified of its identity. 

## Background and References
#### References:
- Self-signed certificate: https://en.wikipedia.org/wiki/Self-signed_certificate

## Requirements
None.

## Proposed Design
Instead of the bmcweb default generated certificate, we are proposing a daemon that will
create a self-signed HTTPS certificate with the appropriate subject (e.g. `CN=$hostname`) when the BMC gets its hostname assigned via IPMI. 

## Alternatives Considered
We have considered the option to upload a self-signed certificate to the BMC with proper
configurations via Redfish, however there are two potential issues:
- A unique certificate will need to be generated for each BMC.
- There is no security validation of the target BMC.

## Impacts
None.

## Testing
To test and verify the service is functionally working correctly, we can use `openssl`
and `ipmitool` to execute the following commands:

#### Assign BMC hostname
```bash
ipmitool -H $IP -I lanplus -U root -P 0penBmc -C 17 dcmi set_mc_id_string $hostname
```

#### Get BMC server certificate infomation
- Run command below and check the server certificate details
    ```bash
    echo quit | openssl s_client -showcerts -servername $IP -connect $IP:443
    ```