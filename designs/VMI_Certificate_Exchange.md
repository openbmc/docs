# VMI Certificate Exchange

Author:
  Raviteja Bailapudi

Primary assignee:
  Raviteja Bailapudi

Other contributors:
  Ratan Gupta

Created:
  07/10/2019

## Problem Description
Management Console can directly talk to the hypervisor using the
Virtualization Management Interface.
Management console requires client key, client.crt, and CA.crt to establish
secure connection to VMI.
Management console needs an API through which it can send the CSR to VMI(CA)
and gets the signed certificate and the CA certificate from VMI.
This design will describe how certificates get exchanged between management
console and VMI

## Glossary
- HMC    - Hardware Management Console : IBM Enterprise system which provides
           management functionalities to the server.
- PHYP   - Power Hypervisor : This orchestrates and manages system
           virtualization.
- VMI    - Virtual Management Interface : The interface facilitating
           communications between HMC and PHYP embedded linux virtual machine.

## Background and References
This is part of the HMC-BMC interface design.

## Requirements
BMC will provide an interface for management console to exchange certificate
information

## Proposed Design
The management console can send CSR string to VMI(CA) and get signed certificate
and Root CA certificate via proposed BMC interface.

### Design Flow
```ascii
    +------------+        +--------+            +--------+
    |    HMC     |        |  BMC   |            |  VMI   |
    |  (client)  |        |        |            |  (CA)  |
    +-----+------+        +----+---+            +---+----+
          |                    |                    |
          |                    |                    |
          +------------------->+                    |
          | VMI Network info   |                    |
          +<-------------------+                    |
          |                    |                    |
client.key|                    |                    |
client.csr     SignCSR()       | pldm call to host  |
          +------------------->+------------------->|
          |                    |                    |  Sign CSR
          | SignCSR() response | pldm response from host
          +<-------------------+<-------------------|
          |                    |                    |
  Client.crt                   |                    |
  CA.crt                       |                    |
          |                    |                    |
          |                    |                    |
          |                    |                    |
          |                    |                    |
          +                    +                    +

```
### VMI certificate exchange
Management console should use the below REST command to exchange certificates
with VMI

```bash
curl -c cjar -k -X POST -H "Content-Type: application/json" -d
  '{"data":[CsrString:"CSR string"]  }' http://{BMC_IP}/ibm/v1/VMInterface/SignCSR
```

## Response:
This will return the certificate string which contains both signed client
certificate and root CA certificate as well

```
{
 "data": [
   “Certificate String”
 ],
 "message": "200 OK",
 "status": "ok"
}
```

## Alternatives considered:

Have gone through existing BMC certificate management infrastructure if we can
extend for this use case.

### Current flow for generating and installing Certificates(CSR Based):

* Certificate Signing Request CSR is a message sent from an applicant to a
  certificate authority in order to apply for a digital identity certificate.
* The user calls CSR interface BMC creates new private key and CSR Certificate
  File
* CSR certificate is passed onto the CA to sign the certificate and then upload
  CSR signed certificate and install the certificate.

### Note

* Our existing BMC certificate manager/service have interfaces to generate CSR,
  upload certificates and other interfaces to manage certificates(replace,delete..etc).
* In VMI certificate exchange, requirement for BMC is to provide an interface for
  management console to get  CSR certificate signed by VMI(CA).
* We don’t have  any existing certificate manager interface to forward CSR
  request to CA to get signed by CA.
* Here proposal is to have SignCSR() interface which accepts CSR string and
  return signed certificate and Root CA certificate.
* This requirement is out of scope for existing certificate manager.so proposing
  SignCSR interface as management console specific interface.

### Alternate Design
```ascii
    +------------+        +--------+            +--------+
    |    HMC     |        |  BMC   |            |  VMI   |
    |  (client)  |        |        |            |  PHYP  |
    +-----+------+        +----+---+            +---+----+
          |                    |                    |
          |                    |                    |
          +------------------->+                    |
          | VMI Network info   |                    |
          +<-------------------+                    |
          |                                         |
          |                SSL tunnel               |
          +---------------------------------------->|
          |              Verify Password            |Nets
          +---------------------------------------->|
          |                                         |
          |                  pldm                   |pldm call to authenticate
          +<-------------------+<-------------------|
          |                    |                    |
          |                   pam                   |
          |              authentication             |
          |                    +------------------->|
          |                                         |
          |        session established              |
          |<--------------------------------------->|

```
* In this alternate design, Mangement console establishes connection to VMI and
  sends Verify Password command to authenticate user to establish secure session.
* VMI does not have authentication method,so VMI needs to use BMC authentication method
  over PLDM.
* There are security concerns if raw password is getting sent over PLDM in clear text
  over LPC, so this design ruled out.

## Impacts
Create new interface SignCSR which takes CSR string as input and returns certificate
string.This method invokes pldm commands to forward CSR request to VMI and get signed
certificates from VMI.
PLDM command needs to be defined.
## Testing
Test the interface command from a management console and verify if certificate
exchange worked as expected and verify if management console able to establish
secure connection to VMI.
