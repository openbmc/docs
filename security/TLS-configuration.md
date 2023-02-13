# How to configure the server TLS certificates for authentication

Author: Zbigniew Kurzynski <zbigniew.kurzynski@intel.com>

Created: May 8, 2020

Related documents:

- [Redfish TLS User Authentication](https://github.com/openbmc/docs/blob/master/designs/redfish-tls-user-authentication.md)

## Introduction

With help of this guidebook you should be able to create both client and server
certificates signed by a CA that can be used to authenticate user requests to an
OpenBMC server. You will also learn how to enable and test the OpenBMC TLS
authentication.

## Certificates

For a certificate to be marked as valid, it (and every certificate in the chain)
has to meet these conditions:

- `KeyUsage` contains required purpose `digitalSignature` and `keyAgreement`
  (see rfc 3280 4.2.1.3)
- `ExtendedKeyUsage` contains required purpose `clientAuth` for client
  certificate and `serverAuth` for server certificate (see rfc 3280 4.2.1.13)
- public key meets minimal bit length requirement
- certificate has to be in its validity period
- `notBefore` and `notAfter` fields have to contain valid time
- has to be properly signed by certificate authority
- certificate is well-formed according to X.509
- issuer name has to match CA's subject name for client certificate
- issuer name has to match the fully qualified domain name of your OpenBMC host

If you already have certificates you can skip to
[Enable TLS authentication ](#Enable-TLS-authentication) or go to
[Verify certificates](#Verify-certificates) and check if they meet the above
requirements.

### Prepare configuration files

To generate certificates with required parameters some modification must be made
to the default openssl configuration file.

First create a new folder named `ca` and create a configuration file using the
default configuration as a template (we do not want to change the original one).
The location of the configuration file may vary depending on the operating
system. For Ubuntu it is usually `/usr/lib/ssl/openssl.cnf`, but can also can be
at `/etc/ssl/openssl.cnf`. For Cygwin it might be
`/etc/defaults/etc/pki/tls/openssl.cnf` or `/etc/pki/tls/openssl.cnf`.

```
mkdir ~/ca
cd ~/ca
cp /usr/lib/ssl/openssl.cnf openssl-client.cnf
```

Then open the client `~/ca/openssl-client.cnf` file in your favorite editor, for
example `vi`.

```
vi ~/ca/openssl-client.cnf
```

Find the sections listed below and add or choose the presented values.

```
[ req ]
req_extensions = v3_req

[ usr_cert ]
extendedKeyUsage = clientAuth

[ v3_req ]
extendedKeyUsage = clientAuth
keyUsage = digitalSignature, keyAgreement
```

Now create a server configuration `openssl-server.cnf` by copying the client
file

```
cp ~/ca/openssl-client.cnf openssl-server.cnf
```

and changing values presented in the sections listed below.

```
[ usr_cert ]
extendedKeyUsage = serverAuth

[ v3_req ]
extendedKeyUsage = serverAuth
```

Create two additional configuration files `myext-client.cnf` and
`myext-server.cnf` for the client and server certificates respectively. Without
these files no extensions are added to the certificate.

```
cat << END > myext-client.cnf
[ my_ext_section ]
keyUsage = digitalSignature, keyAgreement
extendedKeyUsage = clientAuth
authorityKeyIdentifier = keyid
END
```

```
cat << END > myext-server.cnf
[ my_ext_section ]
keyUsage = digitalSignature, keyAgreement
extendedKeyUsage = serverAuth
authorityKeyIdentifier = keyid
END
```

### Create a new CA certificate

First we need to create a private key to sign the CA certificate.

```
openssl genrsa -out CA-key.pem 2048
```

Now we can create a CA certificate, using the previously generated key. You will
be prompted for information which will be incorporated into the certificate,
such as Country, City, Company Name, etc.

```
openssl req -new -config openssl-client.cnf -key CA-key.pem -x509 -days 1000 -out CA-cert.pem
```

### Create client certificate signed by given CA certificate

To create a client certificate, a signing request must be created first. For
this another private key will be needed.

Generate a new key that will be used to sign the certificate signing request:

```
openssl genrsa -out client-key.pem 2048
```

Generate a certificate signing request.

You will be prompted for the same information as during CA generation, but
provide **the OpenBMC system user name** for the `CommonName` attribute of this
certificate. In this example, use **root**.

```
openssl req -new -config openssl-client.cnf -key client-key.pem -out signingReqClient.csr
```

Sign the certificate using your `CA-cert.pem` certificate with following
command:

```
openssl x509 -req -extensions my_ext_section -extfile myext-client.cnf -days 365 -in signingReqClient.csr -CA CA-cert.pem -CAkey CA-key.pem -CAcreateserial -out client-cert.pem
```

The file `client-cert.pem` now contains a signed client certificate.

### Create server certificate signed by given CA certificate

For convenience we will use the same CA generated in paragraph
[Create a new CA certificate](#Create-a-new-CA-certificate), although a
different one could be used.

Generate a new key that will be used to sign the server certificate signing
request:

```
openssl genrsa -out server-key.pem 2048
```

Generate a certificate signing request. You will be prompted for the same
information as during CA generation, but provide **the fully qualified domain
name of your OpenBMC server** for the `CommonName` attribute of this
certificate. In this example it will be `bmc.example.com`. A wildcard can be
used to protect multiple host, for example a certificate configured for
`*.example.com` will secure www.example.com, as well as mail.example.com,
blog.example.com, and others.

```
openssl req -new -config openssl-server.cnf -key server-key.pem -out signingReqServer.csr
```

Sign the certificate using your `CA-cert.pem` certificate with following
command:

```
openssl x509 -req -extensions my_ext_section -extfile myext-server.cnf -days 365 -in signingReqServer.csr -CA CA-cert.pem -CAkey CA-key.pem -CAcreateserial -out server-cert.pem
```

The file `server-cert.pem` now contains a signed server certificate.

### Verify certificates

To verify the signing request and both certificates you can use following
commands.

```
openssl x509 -in CA-cert.pem -text -noout
openssl x509 -in client-cert.pem -text -noout
openssl x509 -in server-cert.pem -text -noout
openssl req -in signingReqClient.csr -noout -text
openssl req -in signingReqServer.csr -noout -text
```

Below are example listings that you can compare with your results. Pay special
attention to attributes like:

- Validity in both certificates,
- `Issuer` in `client-cert.pem`, it must match to `Subject` in `CA-cert.pem`,
- Section _X509v3 extensions_ in `client-cert.pem` it should contain proper
  values,
- `Public-Key` length, it cannot be less than 2048 bits.
- `Subject` CN in `client-cert.pem`, it should match existing OpemBMC user name.
  In this example it is **root**.
- `Subject` CN in `server-cert.pem`, it should match OpemBMC host name. In this
  example it is **bmc.example.com **. (see rfc 3280 4.2.1.11 for name
  constraints)

Below are fragments of generated certificates that you can compare with.

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
server-cert.pem
    Data:
        Version: 3 (0x2)
        Serial Number: 10622848005881387807 (0x936beffaa586db1f)
    Signature Algorithm: sha256WithRSAEncryption
        Issuer: C=US, ST=z, L=z, O=z, OU=z, CN=bmc.example.com
        Validity
            Not Before: May 22 13:46:02 2020 GMT
            Not After : May 22 13:46:02 2021 GMT
        Subject: C=US, ST=z, L=z, O=z, OU=z, CN=bmc.example.com
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
                Public-Key: (2048 bit)
                Modulus:
                    00:d9:34:9c:da:83:c6:eb:af:8f:e8:11:56:2a:59:
                    ...
                    92:60:09:fc:f9:66:82:d0:27:03:44:2f:9d:6d:c0:
                    a5:6d
                Exponent: 65537 (0x10001)
        X509v3 extensions:
            X509v3 Key Usage:
                Digital Signature, Key Agreement
            X509v3 Extended Key Usage:
                TLS Web Server Authentication
            X509v3 Authority Key Identifier:
                keyid:5B:1D:0E:76:CC:54:B8:BF:AE:46:10:43:6F:79:0B:CA:14:5C:E0:90

    Signature Algorithm: sha256WithRSAEncryption
         bf:41:e2:2f:87:44:25:d8:54:9c:4e:dc:cc:b3:f9:af:5a:a3:
         ...
         ef:0f:90:a6

```

## Installing CA certificate on OpenBMC

The CA certificate can be installed via Redfish Service. The file `CA-cert.pem`
can not be uploaded directly but must be sent embedded in a valid JSON string,
which requires `\`, `"`, and control characters must be escaped. This means all
content is placed in a single string on a single line by encoding the line
endings as `\n`. The command below prepares a whole POST body and puts it into a
file named: `install_ca.json`.

```
cat << END > install_ca.json
{
  "CertificateString":"$(cat CA-cert.pem | sed -n -e '1h;1!H;${x;s/\n/\\n/g;p;}')",
  "CertificateType": "PEM"
}
END
```

To install the CA certificate on the OpenBMC server post the content of
`install_ca.json` with this command:

Where `${bmc}` should be `bmc.example.com`. It is convenient to export it as an
environment variable.

```
curl --user root:0penBmc -d @install_ca.json -k -H "Content-Type: application/json" -X POST https://${bmc}/redfish/v1/Managers/bmc/Truststore/Certificates

```

Credentials `root:0penBmc` can be replaced with any system user name and
password of your choice but with proper access rights to resources used here.

After successful certificate installation you should get positive HTTP response
and a new certificate should be available under this resource collection.

```
curl --user root:0penBmc -k https://${bmc}/redfish/v1/Managers/bmc/Truststore/Certificates

```

An auto-generated self-signed server certificate is already present on OpenBMC
by default. To use the certificate signed by our CA it must be replaced.
Additionally we must upload to OpenBMC the private key that was used to sign the
server certificate. A proper message mody can be prepared the with this command:

```
cat << END > replace_cert.json
{
  "CertificateString":"$(cat server-key.pem server-cert.pem | sed -n -e '1h;1!H;${x;s/\n/\\n/g;p;}')",
   "CertificateUri":
   {
      "@odata.id": "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/1"
   },
  "CertificateType": "PEM"
}
END
```

To replace the server certificate on the OpenBMC server post the content of
`replace_cert.json` with this command:

```
curl --user root:0penBmc -d @replace_cert.json -k -H "Content-Type: application/json" -X POST https://${bmc}/redfish/v1/CertificateService/Actions/CertificateService.ReplaceCertificate/

```

## Enable TLS authentication

To check current state of the TLS authentication method use this command:

```
curl --user root:0penBmc -k https://${bmc}/redfish/v1/AccountService
```

and verify that the attribute `Oem->OpenBMC->AuthMethods->TLS` is set to true.

To enable TLS authentication use this command:

```
curl --user root:0penBmc  -k -X PATCH -H "Content-Type: application/json" --data '{"Oem": {"OpenBMC": {"AuthMethods": { "TLS": true} } } }' https://${bmc}/redfish/v1/AccountService
```

To disable TLS authentication use this command:

```
curl --user root:0penBmc  -k -X PATCH -H "Content-Type: application/json" --data '{"Oem": {"OpenBMC": {"AuthMethods": { "TLS": false} } } }' https://${bmc}/redfish/v1/AccountService
```

Other authentication methods like basic authentication can be enabled or
disabled as well using the same mechanism. All supported authentication methods
are available under attribute `Oem->OpenBMC->AuthMethods` of the
`/redfish/v1/AccountService` resource.

## Using TLS to access OpenBMC resources

If TLS is enabled, valid CA certificate was uploaded and the server certificate
was replaced it should be possible to execute curl requests using only client
certificate, key, and CA like below.

```
curl --cert client-cert.pem --key client-key.pem -vvv --cacert CA-cert.pem https://${bmc}/redfish/v1/SessionService/Sessions
```

## Common mistakes during TLS configuration

- Invalid date and time on OpenBMC,

- Testing Redfish resources, like `https://${bmc}/redfish/v1` which are always
  available without any authentication will always result with success, even
  when TLS is disabled or certificates are invalid.

- Certificates do not meet the requirements. See paragraphs
  [Verify certificates](#Verify-certificates).

- Attempting to load the same certificate twice will end up with an error.

- Not having phosphor-bmcweb-cert-config in the build.
