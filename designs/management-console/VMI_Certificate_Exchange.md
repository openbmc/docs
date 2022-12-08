# VMI Certificate Exchange

Author: Raviteja Bailapudi

Other contributors: Ratan Gupta

Created: 07/10/2019

## Glossary

- HMC - Hardware Management Console : Management console for IBM enterprise
  servers.
- PHYP - Power Hypervisor : This orchestrates and manages system virtualization.
- VMI - Virtual Management Interface : The interface facilitating communications
  between HMC and PHYP embedded linux virtual machine.
- KVM - Kernel Virtual Machine : Open source virtualization software

## Problem Description

On enterprise POWER systems, the Hardware management console (HMC) needs to
establish a secure connection to the Virtualization management interface (VMI)
for virtualization management.

VMI is an embedded Linux VM created and run on PHYP which provides the
virtualization function.

HMC requires client key, client.crt, and CA.crt to establish secure connection
to VMI.

BMC needs to provide certificate exchange functionality to management console
due to following reasons:

- Host firmware (PHYP) does not have authentication mechanism.
- VMI trusts that BMC has authenticated and verified the authenticity of any
  client connected as there is a secure authenticated connection already exists
  between HMC and BMC.

Management console needs an API through which it can send the CSR to VMI (CA)
and gets the signed certificate and the CA certificate from VMI. This design
will describe how certificates get exchanged between management console and VMI

IBM systems can run both IBM specific host-firmware (PHYP) and Linux KVM. This
API would be used only for the PHYP based machines.

Enable and disable of this API would be controlled by the build time
configurable variable.

## Background and References

- VMI will be created and run on PHYP that will provide the virtualization
  function.
- When the VMI is powered on it generates a public-private key pair and a
  self-signed root certificate is created using this key pair.
- VMI acts as root CA only for VMI endpoints, its not an official CA and uses
  its self-signed certificate to sign CSR from client.
- HMC needs to establish secure connection to VMI to perform virtualization
  management.

## Requirements

BMC will provide an interface for management console to exchange certificate
information from VMI so that HMC can establish secure connection to VMI.

## Proposed Design

The management console can send CSR string to VMI (CA) and get signed
certificate and Root CA certificate via proposed BMC interface.

In this interface perspective, the HTTP error code could be 4XX/5XX. It would be
mapped depending on the PLDM error response.

HMC can query BMC state and use this API to initiate certificate exchange.If HMC
runs this command before PHYP boots, PLDM command returns error If PLDM command
throws an error, that would be mapped to Internal server Error (500).

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

Management console should use the below REST commands to exchange certificates
with VMI

#### Get Signed certificate:

REST command to get signed client certificate from VMI

Request:

```bash
curl -k -H "X-Auth-Token:  <token>" -X POST "Content-Type: application/json" -d
  '{"CsrString":"<CSR string>"}' https://{BMC_IP}/ibm/v1/Host/Actions/SignCSR
```

Response: This will return the certificate string which contains signed client
certificate

```
 {
   “Certificate”: "<certificate string>"
 }

```

#### Get Root certificate:

REST command to get VMI root certificate

Request:

```bash
curl -k -H "X-Auth-Token:  <token>" -X GET http://{BMC_IP}/ibm/v1/Host/Certificate/root
```

Response: This will return the certificate string which contains and root CA
certificate.

```
 {
   “Certificate”: "<certificate string>"
 }

```

This interface returns HTTP error codes 5XX/4XX in failure cases

## Alternatives considered:

Have gone through existing BMC certificate management infrastructure if we can
extend for this use case.

### Current flow for generating and installing Certificates (CSR Based):

- Certificate Signing Request CSR is a message sent from an applicant to a
  certificate authority in order to apply for a digital identity certificate.
- The user calls CSR interface BMC creates new private key and CSR Certificate
  File
- CSR certificate is passed onto the CA to sign the certificate and then upload
  CSR signed certificate and install the certificate.

### Note

- Our existing BMC certificate manager/service have interfaces to generate CSR,
  upload certificates and other interfaces to manage
  certificates(replace,delete..etc).
- In VMI certificate exchange, requirement for BMC is to provide an interface
  for management console to get CSR certificate signed by VMI (CA).
- We don’t have any existing certificate manager interface to forward CSR
  request to CA to get signed by CA.
- Here proposal is to have SignCSR() interface which accepts CSR string and
  return signed certificate and Root CA certificate.
- This requirement is out of scope for existing certificate manager so proposing
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

- In this alternate design, Management console establishes connection to VMI and
  sends Verify Password command to authenticate user to establish secure
  session.
- VMI does not have authentication method, so VMI needs to use BMC
  authentication method over PLDM.
- There are security concerns if raw password is getting sent over PLDM in clear
  text over LPC, so this design ruled out.

## Impacts

- Create new interface GetRootCertificate in webserver which reads root
  certificate from '/var/lib/bmcweb/RootCert' file.This API can handle muptiple
  requests at the sametime.
- PLDM gets root certificate as soon as VMI boots and it writes to
  '/var/lib/bmcweb/RootCert'.
- Implement D-Bus interface to create dbus object for each signCSR so that
  multiple requests can work at the sametime. D-bus service:
  xyz.openbmc_project.Certs.ca.authority.Manager Object path :
  /xyz/openbmc_project/certs/ca Interface : xyz.openbmc_project.Certs.Authority
  Method : SignCSR
- Dbus object contains CSR,ClientCertificate and Status properties.
- PLDM looks for interface added signal for each object created and reads CSR
  property for CSR string and forwards this CSR string to VMI for signing this
  CSR.
- Once PLDM on BMC gets the client certificate from VMI, it updates the
  ClientCertificate D-bus property and updates the Status property to Complete
  in the Dbus object.
- Create new interface SignCSR in webserver which takes CSR string as input and
  returns certificate string.This interface calls SignCSR dbus method and looks
  for Status property changed signal to verify status.Reads ClientCertificate
  property content and return certificate string.
- On completion of serving the sign CSR request, respective dbus object will be
  deleted before returning certificate string to client.
- BMC is passthrough which allows certificate exchange between VMI and HMC. BMC
  does not store or parse these certificates.
- Build time configure variable defined to control enable and disable of this
  API in webserver. It is required only for IBM systems with IBM specific
  host-firmware (PHYP)

## Testing

- Test the interface command from a management console and verify if certificate
  exchange worked as expected and verify if management console able to establish
  secure connection to VMI.

- Certificate exchange fails in the following scenarios

  - If PHYP is not up
  - If PHYP throws error for certificate validation. This interface returns
    appropriate HTTP error code (4XX/5XX) based on type of error.

- If there are issues like certificate expiry, revocation, incorrect date/time
  and incorrect certificates, then HMC fails to establish connection to VMI.
