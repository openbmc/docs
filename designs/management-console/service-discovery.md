# Service processor discovery through Avahi

Author: Ratan Gupta

Other contributors: Asmitha KR

Created: 2019-07-12

# Background and References

[1] https://www.engineersgarage.com/Articles/IoT-Service-Discovery-Protocols [2]
http://www.zeroconf.org/ZeroconfAndUPnP.html : comparison of mDns v/s UPnP. [3]
https://www.avahi.org/ [4]
https://github.com/lathiat/avahi/blob/master/avahi-daemon/example.service

Apple’s Bonjour uses mDNS and DNS-SD. Linux’s Avahi uses IPv4LL, mDNS, and
DNS-SD. Linux’s systemd uses resolve hostnames on a network via DNS, mDNS, and
LMMNR.

## Problem Description

In a network where there are thousands of system, Management console should be
able to discover the server. We have a requirement where the management console
wants to discover the vendor-specific server.

eg: Management console wants to discover the low-end server of manufacturer XYZ.

## Proposed Design

Currently in openBMC, Avahi is being used as service discovery. Avahi is a
system which facilitates service discovery on a local network via the
mDNS/DNS-SD protocol suite. BMC publishes the hostname and interface details to
the network using the Avahi service.

The services that are being published by Avahi have various fields like -
service name, type, port, hostname, address, port, and a text record. To solve
the above-listed problem, we are proposing a solution in which the
vendor-specific information is included in the text record field of the Avahi
service file.

To do so, currently, in OpenBMC we have the infrastructure where the
service-specific data is passed through a systemd service specific bb file.
Depending on the distro feature(Avahi) enabled or not, it generates the Avahi
service file with the given data. We are enhancing this infrastructure to add
the vendor-specific information in the avahi service file(under txt-record).

Following commits implements the behaviour.

https://gerrit.openbmc.org/c/openbmc/meta-phosphor/+/22950
https://gerrit.openbmc.org/c/openbmc/meta-ibm/+/22951

As part of avahi discovery response, the client receives the following:

1. Hostname
2. IP address
3. Service Port
4. Service Type
5. Additional data

### How to do discovery through avahi

The following command may be used to discover the BMC systems in the network.
Avahi-browse -rt <service type> e.g. To discover service type :
\_obmc_rest.\_tcp avahi-browse -rt \_obmc_rest.\_tcp Output of the above command
is as follows avahi-browse -rt \_obmc_rest.\_tcp

- eth0 IPv6 obmc_rest \_obmc_rest.\_tcp local
- eth0 IPv4 obmc_rest \_obmc_rest.\_tcp local = eth0 IPv6 obmc_rest
  \_obmc_rest.\_tcp local hostname =
  [witherspoon-cdcb785972fb41f082c7ca747e287fa6.local] address =
  [fe80::72e2:84ff:fe14:2390] port = [443] txt = ["Manufacturer=XYZ"] txt =
  ["Type=abc"] = eth0 IPv4 obmc_rest \_obmc_rest.\_tcp local hostname =
  [witherspoon-cdcb785972fb41f082c7ca747e287fa6.local] address = [9.5.180.233]
  port = [443] txt = ["Manufacturer=XYZ"] txt = ["Type=abc"]

## Alternatives Considered

openSLP

## Impacts

None.

## Testing

The path /etc/avahi/services contain all the services that avahi publishes on
startup.
