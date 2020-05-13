# How to configure the server TLS certificates for authentication
Author:
  Zbigniew Kurzynski <zbigniew.kurzynski@intel.com>

Created:
  May 8, 2020

Related documents:
* [Redfish TLS User Authentication](https://github.com/openbmc/docs/blob/master/designs/redfish-tls-user-authentication.md)

---
With help of this guidebook you should be able to create CA and client
certificates that can be used to verify the OpenBMC TLS authentication.
You will also learn how to enable and test the OpenBMC TLS authentication.

## Certificates
For certificate to be marked as valid, it (and every certificate in chain) has
to meet these conditions:

* KeyUsage contains required data ("digitalSignature" and "keyAgreement")
* ExtendedKeyUsage contains required data (contains "clientAuth")
* public key meets minimal bit length requirement
* certificate has to be in its validity period
* notBefore and notAfter fields have to contain valid time
* has to be properly signed by certificate authority
* certificate cannot be currently revoked
* certificate is well-formed according to X.509
* certificate cannot be self-signed
* issuer name has to match CA's subject name

If you already have certificates you can skip to
[Installing CA certificate in OpenBMC](##-Installing-CA-certificate-in-OpenBMC)
or go to [Verify certificates](###-Verify-certificates) and check if they meet
above requirements.

## CA and client certificate generation

### Prepare the configuration file

To generate certificates with required parameters some modification must be done
in default openssl configuration file.

First create a new folder named *ca* and create a copy of the default
*openssl.cnf* file (we do not want to change the original one).

> Location of *openssl.cnf* might vary depending on system. For Ubuntu it is
`/usr/lib/ssl/openssl.cnf`, but also can be `/etc/ssl/openssl.cnf`.
For Cygwin it might be `/etc/defaults/etc/pki/tls/openssl.cnf` or
`/etc/pki/tls/openssl.cnf`.

```
mkdir ~/ca
cd ~/ca
cp /usr/lib/ssl/openssl.cnf .
```

Then open the `~/ca/openssl.cnf` file in your favorite editor, for example vi.

```
vi ~/ca/openssl.cnf
```

Find listed below sections and add or change presented values.

```
[ req ]
req_extensions = v3_req

[ usr_cert ]
extendedKeyUsage=clientAuth

[ v3_req ]
extendedKeyUsage = clientAuth
keyUsage = digitalSignature, keyAgreement

[ v3_ca ]
keyUsage = digitalSignature, keyAgreement
```

Create an additional file named `myext.cnf` with section *my_ext_section* and
keyUsage, extendedKeyUsage, authorityKeyIdentifier attributes.
It should contain below certificate extensions and you can create it with
following command:

```
cat << END > myext.cnf
[ my_ext_section ]
keyUsage = digitalSignature, keyAgreement
extendedKeyUsage = clientAuth
authorityKeyIdentifier = keyid
END
```

### Create a new CA certificate
First we need to create a private key to sign the CA certificate.
```
openssl genrsa -out CA-key.pem 2048
```
> You might be prompted for passphrase. A passphrase is a word or phrase that
protects private key files. It prevents unauthorized users from encrypting them.

Now we can create CA certificate using previously generated key.
```
openssl req -new -key CA-key.pem -x509 -days 1000 -out CA-cert.pem
```

> You will be prompted for information which will be incorporated into the
certificate, such as Country, City, Company Name, etc.

### Create client certificate signed by given CA certificate
To create a client certificate a signing request must be created first.
For this another private key will be needed.

Generate a new key that will be used to sign the certificate signing request:
```
openssl genrsa -out client-key.pem 2048
```
Generate a certificate signing request.
> You will be prompted for the same information as during CA generation, but in
this case as a **CommonName** provide OpenBMC system user name (for example
**root**).

```
openssl req -new -config openssl.cnf -key client-key.pem -out signingReq.csr
```

Sign the certificate using your `CA-cert.pem` certificate with following command:
```
openssl x509 -req -extensions my_ext_section -extfile myext.cnf -days 365 -in signingReq.csr -CA CA-cert.pem -CAkey CA-key.pem -CAcreateserial -out client-cert.pem
```
Now a client certificate is created and signed in file `client-cert.pem`.


### Verify certificates
To verify the signing request and both certificates you can use following
commands.

```
openssl x509 -in CA-cert.pem -text -noout
openssl x509 -in client-cert.pem -text -noout
openssl req -in signingReq.csr -noout -text
```

Below are example listings that you can compare with your results.

>Please pay special attention to attributes like:
> * Validity in both certificates,
> * Issuer in `client-cert.pem`, it should match to Subject in `CA-cert.pem`,
> * Section *X509v3 extensions* in `client-cert.pem` it should contain proper
values,
> * Public-Key length, it cannot be less than 2048 bits.
> * Subject CN in `client-cert.pem`, it should match existing OpemBMC user name
(in this example it is **root**).

```
CA-cert.pem
    Data:
        Version: 3 (0x2)
        Serial Number: 16242916899984461675 (0xe16a6edca3c34f6b)
    Signature Algorithm: sha256WithRSAEncryption
        Issuer: C=US, ST=California, L=San Francisco, O=Intel, CN=Test CA
        Validity
            Not Before: May 11 11:40:48 2020 GMT
            Not After : Feb  5 11:40:48 2023 GMT
        Subject: C=US, ST=California, L=San Francisco, O=Intel, CN=Test CA
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
                Public-Key: (2048 bit)
                Modulus:
                    00:d4:24:c1:1d:ac:85:8c:5b:42:e4:f8:a8:d8:7c:
                    ...
                    55:83:8b:aa:ac:ac:6e:e3:01:2b:ce:f7:ee:87:21:
                    f9:2b
                Exponent: 65537 (0x10001)
        X509v3 extensions:
            X509v3 Subject Key Identifier:
                ED:FF:80:A7:F8:DA:99:7F:94:35:95:F0:92:74:1A:55:CD:DF:BA:FE
            X509v3 Authority Key Identifier:
                keyid:ED:FF:80:A7:F8:DA:99:7F:94:35:95:F0:92:74:1A:55:CD:DF:BA:FE

            X509v3 Basic Constraints:
                CA:TRUE
    Signature Algorithm: sha256WithRSAEncryption
         cc:8b:61:6a:55:60:2b:26:55:9f:a6:0c:42:b0:47:d4:ec:e0:
         ...
         45:47:91:62:10:bd:3e:a8:da:98:33:65:cc:11:23:95:06:1b:
         ee:d3:78:84
```
```
client-cert.pem
    Data:
        Version: 3 (0x2)
        Serial Number: 10150871893861973895 (0x8cdf2434b223bf87)
    Signature Algorithm: sha256WithRSAEncryption
        Issuer: C=US, ST=California, L=San Francisco, O=Intel, CN=Test CA
        Validity
            Not Before: May 11 11:42:58 2020 GMT
            Not After : May 11 11:42:58 2021 GMT
        Subject: C=US, ST=California, L=San Francisco, O=Intel, CN=root
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
                Public-Key: (2048 bit)
                Modulus:
                    00:cf:d6:d0:a2:09:62:df:e9:a9:b1:e1:3d:7f:2f:
                    ...
                    30:7b:48:dc:c5:2c:3f:a9:c0:d1:b6:04:d4:1a:c8:
                    8a:51
                Exponent: 65537 (0x10001)
        X509v3 extensions:
            X509v3 Key Usage:
                Digital Signature, Key Agreement
            X509v3 Extended Key Usage:
                TLS Web Client Authentication
            X509v3 Authority Key Identifier:
                keyid:ED:FF:80:A7:F8:DA:99:7F:94:35:95:F0:92:74:1A:55:CD:DF:BA:FE

    Signature Algorithm: sha256WithRSAEncryption
         7f:a4:57:f5:97:48:2a:c4:8e:d3:ef:d8:a1:c9:65:1b:20:fd:
         ...
         25:cb:5e:0a:37:fb:a1:ab:b0:c4:62:fe:51:d3:1c:1b:fb:11:
         56:57:4c:6a
```
```
signingReq.csr
    Data:
        Version: 0 (0x0)
        Subject: C=US, ST=California, L=San Francisco, O=Intel, CN=root
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
                Public-Key: (2048 bit)
                Modulus:
                    00:cf:d6:d0:a2:09:62:df:e9:a9:b1:e1:3d:7f:2f:
                    ...
                    30:7b:48:dc:c5:2c:3f:a9:c0:d1:b6:04:d4:1a:c8:
                    8a:51
                Exponent: 65537 (0x10001)
        Attributes:
        Requested Extensions:
            X509v3 Basic Constraints:
                CA:FALSE
            X509v3 Extended Key Usage:
                TLS Web Client Authentication
            X509v3 Key Usage:
                Digital Signature, Key Agreement
    Signature Algorithm: sha256WithRSAEncryption
         57:53:1f:99:62:4d:ec:49:50:e1:6c:ea:50:d5:3e:da:f3:48:
         ...
         fc:a0:2f:e3:cf:78:78:4b:12:96:49:c0:d5:76:39:e4:1f:ca:
         f6:a2:2a:7f
```

## Installing CA certificate on OpenBMC

CA certificate can be installed via redfish API. The file `CA-cert.pem` cannot
be send via redfish as is, its content must be transformed into a single line
where all EOL are replaced by *\n*.

The command below prepares a whole POST body and puts it into a file named:
`message_body`.

```
cat << END > message_body
{
  "CertificateString":"$(cat CA-cert.pem | sed -E ':a;N;$!ba;s/\r{0,1}\n/\\n/g')",
  "CertificateType": "PEM"
}
END
```

To install the CA certificate in OpenBMC post the content of `message_body`
with this command:


```
curl --user root:0penBmc -d @message_body -k POST https://${BMC}/redfish/v1/Managers/bmc/Truststore/Certificates

```
> Where ${BMC} is your OpenBMC instance IP. It is convenient to export it as
environment variable.

>Credentials root:0penBmc can be replaced with any system user name and password
of your choice but with proper access rights to resources used here.


After successful certificate installation you should get positive HTTP response
and a new certificate should be available under this resource collection.
```
curl --user root:0penBmc -k https://${BMC}/redfish/v1/Managers/bmc/Truststore/Certificates

```

## Enable TLS authentication
To enable/disable the TLS authentication use this command:

```
curl --user root:0penBmc  -k --request PATCH -H "ContentType:application/json" --data '{"Oem": {"OpenBMC": {"AuthMethods": { "TLS": true} } } }' https://${BMC}/redfish/v1/AccountService
```

To check current state of the TLS authentication method use this command:

```
curl --user root:0penBmc -k https://${BMC}/redfish/v1/AccountService
```
and verify if attribute *Oem->OpenBMC->AuthMethods->TLS* is set to true.


## Using TLS to access OpenBMC resources

If TLS is enabled and valid CA certificate was uploaded it should be possible
to execute curl requests using only client certificate and key, like below.

```
curl --cert client-cert.pem --key client-key.pem -vvv -k https://${BMC}/redfish/v1/SessionService/Sessions
```
## Common mistakes during TLS testing

* Invalid date and time on OpenBMC,

* Using redfish resources, like `https://${BMC}/redfish/v1` which are always
available without any authentication.

* Certificates do not meet the requirements. See paragraphs
[Certificates](##-Certificates) and [Verify certificates](###_Verify-certificates).
