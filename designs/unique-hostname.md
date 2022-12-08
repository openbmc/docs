# Setting a unique hostname to the BMC machine on first boot

Author: Asmitha KR

Other contributors: None

Created: 2019-05-07

## Problem Description

In OpenBMC, the hostname discovery is done by the avahi Dbus service at the
startup. In a network where there are multiple OpenBMC machines, avahi keeps
getting the hostname conflict and the service name conflict. Hence, the problem
is to find a solution that resolves these conflicts.

## Background and References

The detailed issue regarding the hostname and service name conflicts is
described in the following links.

https://github.com/openbmc/openbmc/issues/1741.
https://lists.freedesktop.org/archives/avahi/2018-January/002492.html
https://github.com/lathiat/avahi/issues/117

## Requirements

None.

## Proposed Design

To solve this, we are proposing a service which assigns a unique hostname to the
BMC and runs on the very first boot. one of the ways to generate the unique
hostname is to append the Serial Number retrieved from Inventory Manager to the
existing default hostname.

## Alternatives Considered

None.

## Impacts

None.

## Testing

None.
