# Boot initiator mailbox

Author:
  Alexander Amelkin, [a.amelkin@yadro.com](mailto:a.amelkin@yadro.com)

Primary assignee:
  Alexander Amelkin, [a.amelkin@yadro.com](mailto:a.amelkin@yadro.com)

Other contributors:
  Alexander Amelkin, [a.amelkin@yadro.com](mailto:a.amelkin@yadro.com)
  Ivan Mikhaylov, [i.mikhaylov@yadro.com](mailto:i.mikhaylov@yadro.com)

Created:
  17.12.2019

## Problem Description

OpenBMC users may need a way to change the boot parameters of the host
systems beyond the standard boot source settings. Namely, they may want to
set an exact UUID of a specific disk partition they would like the host
to boot from, or change other parameters that the host firmware is able
to process.

Passing such specific parameters during the boot stage grants the flexibility
to the end user to setup a system without reflashing NVRAM on the host.
This yields a significant decrease in time spent on system maintenance.

## Background and References

The IPMI 2.0 specification describes a so called 'boot initiator mailbox'
as a way for the BMC to pass OEM-specific information that 'directs the
operation of the OS loader or service partition software'.

Boot initiator mailbox is described inside IPMI spec v2.0 Table 28-14
'Boot Option Parameters' parameter 7 'Boot initiator mailbox'.

The information passed via boot initiator mailbox is specific to the
vendor of the host firmware and/or the host OS loader.

[Need a protocol to specify boot partition UUID from BMC](https://github.com/open-power/petitboot/issues/45)
[IPMI: Initiating Better Overrides](https://sthbrx.github.io/blog/2018/12/19/ipmi-initiating-better-overrides/)
[discover/platform-powerpc: read bootdev config from IPMI boot mailbox](https://github.com/open-power/petitboot/commit/78c3a044d2302bacf27ac2d9ef179bc35824af4c)
[chassis: Add boot initiator mailbox support](https://github.com/ipmitool/ipmitool/commit/62a04390e10f8e62ce16b7bc95bf6ced419b80eb)

## Requirements

Provide a way to set up boot parameters for startup and post-boot in the host.

## Proposed Design

The new interface `/xyz/openbmc_project/Control/Boot/Mailbox` is a part of
`phosphor-dbus-interfaces`. The interface consists of a constant integer
`IANAEnterprise` property that have to be set by `phosphor-settings-manager`
once at startup from machine-dependent data, and of an array of bytes of
machine-specific size. The data array is user-modifiable via IPMI.

The `Supported` boolean property is to indicate whether or not the particular
machine supports the boot mailbox.

In 'Get System Boot Options' handler for IPMI, a new parameter for mailbox
should be implemented and processed.

As an example of usage with ipmitool:

    ipmitool -H host -I lanplus -P 0penBmc chassis bootmbox set text 2 "petitboot,bootdevs=disk uuid:59616472-6f21-626f-6f74-302150415254 uuid:59616472-6f21-626f-6f74-312150415251"
    ipmitool -H host -I lanplus -P 0penBmc chassis bootmbox get text

## Alternatives Considered

None.

## Impacts

Ability to set the boot parameters for petitboot on host system via IPMI,
REST, and local dbus interfaces.

## Testing
TBD

