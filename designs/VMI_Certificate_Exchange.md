# VMI Certificate Exchange

For HMC to start communication with VMI(Virtualization management Interface), it needs a certificate signed by VMI which acts as CA. through BMC.
Goal of this document is to design the interface through which HMC sends the CSR to VMI and gets the signed certificate and the CA certificate from VMI.

## Curl Command

```bash
curl -c cjar -k -X POST -H "Content-Type: application/json" -d '{"data":[CsrString:"CSR string"]  }' http://{BMC_IP}/xyz/openbmc_project/HMC/VMInterface/SignCSR
```

## Response:

{
 "data": [
   “Certificate String”
 ],
 "message": "200 OK",
 "status": "ok"
}

  Certificate string contains both client certificate and root CA certificate as well
   [CertificateString:”client Ceritificate”+"CaCertificateString"]

## Alternatives considered:

Have gone through existing BMC certificate management infrastructure if we can extend for this use case.

## Current flow for generating and installing Certificates(CSR Based):

* Certificate Signing Request CSR is a message sent from an applicant to a certificate authority in order to apply for a digital identity certificate.
* The user calls CSR interface BMC creates new private key and CSR Certificate File
* CSR certificate is passed onto the CA to sign the certificate and then upload CSR signed certificate and install the certificate.
  
## Note

* Our existing BMC certificate manager/service have interfaces to generate CSR, upload certificates and other interfaces to manage certificates(replace,delete..etc).
* In VMI certificate exchange, requirement for BMC is to provide an interface for  HMC to get HMC’s CSR certificate signed by VMI(CA).
* We don’t have  any existing certificate manager interface to forward CSR request to CA to get signed by CA.
* Here proposal is to have SignCSR() interface which accepts CSR string and return signed certificate and Root CA certificate.
* This requirement is out of scope for existing certificate manager.so  proposing SignCSR interface as HMC specific interface at URI /xyz/openbmc_project/HMC/VMInterface/SignCSR
