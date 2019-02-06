# MCTP over SMbus

Author:
  Ed Tanous <edtanous>
Primary assignee:
  Ed Tanous <edtanous>
Other contributors:
  Nilan Naidoo
Created:
  2-1-2019

## Problem Description

A large number of non-BMC devices in a common server support the MCTP over SMBus
protocol, and expose a number of interfaces.  There is a desire to make a number
of these parameters available to a user through the BMC for a number of use
cases.

## Background and References

MCTP is a protocol defined by the DMTF organization for out-of-band management
of in-system devices.  A number of non-OpenBMC servers implement MCTP interfaces
today, to varying degrees of completeness.

[MCTP standard](https://www.dmtf.org/sites/default/files/standards/documents/DSP0237_1.1.0.pdf)

Previous work to enable a linux user space interface to received i2c commands is
here: [Patch](https://lore.kernel.org/patchwork/patch/934565/) And is currently
used to a very similar effect here in the
[IPMB daemon](https://github.com/openbmc/ipmbbridge)

MCTP over I2C flows that need to query a device follows the following flow:

1. BMC commits one or more i2c write transactions to a given device (for example
   NVMe devices default to address 0x1D).  Each individual payload includes
   headers and trailers that are used to chunk large data payloads over i2c
   devices.  Part of this payload contains the address that the device is
   expected to reply to.
2. Some time after the completion of the request, the device then triggers one
   or more i2c writes back to the BMC, containing the requested data.

Kernel documentation of the i2c slave interface:
<https://www.kernel.org/doc/Documentation/i2c/slave-interface>

## Requirements

1. MCTP protocol must open a communication channel to MCTP devices
2. MCTP implementation provide a DBus interface, to allow applications to access
   MCTP resources
3. MCTP implementation must allow access to devices behind i2c muxes
4. MCTP implementation must support multi-packet payloads
5. MCTP must support scanning for MCTP devices dynamically
6. Because PCIe cards may support IPMB or MCTP on the same bus, MCTP
   implementation must not conflict with the IPMB implementation
7. Implementation should not conflict with HWMon devices, such as TMP75 devices
   existing on the same bus
8. MCTP implementation should have an understanding of host power states, and
   support interaction with devices that might not be available if a given host
   processor is shutdown.

## Use cases

1. Management of NVMe-MI capable devices
   a. MCTP should allow publishing of inventory information
   (Model/Serial/Capabilities) to DBus, using the existing DBus interfaces
   b. MCTP should allow exposing a xyz.openbmc_project.Sensor.Value
   interface, to allow managing drive thermals.
   c. MCTP should allow uploading of new drive firmware over MCTP
   d. MCTP should allow provisioning of drive cryptography interfaces

  <https://nvmexpress.org/wp-content/uploads/NVM_Express_Management_Interface_1_0_gold.pdf>
2. Management of PCIe add-in-cards that support MCTP
   a. MCTP should allow publishing of inventory information about a given card
   card
   b. MCTP should allow detection of MCTP capable devices on a given i2c lane
   c. MCTP should allow monitoring card thermals

## Proposed Design

The proposed design, in practice, largely mirrors the existing ipmb interfaces
exposed above, despite the protocols being very different, the flows are
similar. The large work items involved include:

1. Implementation of a receive driver in the kernel, and user space interface.
   Currently the slave-mqueue device has not been accepted into upstream linux,
   but intent is that the design-intent of that would be largely unchanged, and
   would drive for a user space interface for receiving arbitrary i2c write
   payloads.
2. Development of an outbound DBus API for interacting with MCTP devices.
3. Implementation of a user space MCTP receive daemon.  In practice, this would
   look very similar to the existing ipmb daemon, and would be capable of
   receiving an MCTP payload from DBus, translating and chunking said payload,
   writing the payload to the driver, then waiting on receive of a response from
   the device.
4. Implementation of at minimum 1 use-case specific daemon to prove capability.
   First example will likely be an NVMe-MI capable daemon.

## Alternatives Considered

1. Implementing MCTP directly as a kernel abstraction, and remove the need for a
   user space layer: This is a relative non-starter, as on OpenBMC, a majority
   of the resources that are needed to be accessed are on DBus, which the kernel
   level abstractions do not have access to.  Inventory, thermals, firmware
   updates, and others are currently modeled on DBus.  Modeling them as both a
   user space file abstraction and a DBus abstraction would take considerable
   time and effort, for very little benefit.  Also, having a kernel level
   abstraction would require an MCTP command table to be checked into the kernel
   driver.  Given that this command table is not fixed, and includes OEM
   extensions that might differ per-platform, maintaining this abstraction in
   the kernel becomes very difficult.

2. Implementing the entirety of MCTP over i2c in user space: Given that there is
   no kernel abstraction for receiving of arbitrary i2c writes as payloads, this
   is a nonstarter.  The existing kernel interfaces have the ability to emulate
   particular types of devices, like EEPROMs as shown in the link below, but
   doesn't include an abstraction for arbitrary devices.
   <https://elixir.bootlin.com/linux/latest/source/drivers/i2c/i2c-slave-eeprom.c>

## Impacts

1. DBus APIs will be enhanced with MCTP abstractions (similar to the IPMI
   abstractions that exist today)
2. Security will need to be analyzed.  The primary source of concern here is the
   implementation of MCTP bridging, which is currently outside the scope of this
   proposed change.
3. Documentation will need to be written for creating MCTP devices and daemons.
4. Performance will need to be analysed, both as a function of MCTP bandwidth,
   and impact to other BMC services.  Given the similarities with IPMI, there is
   no expected impact at this time.

## Testing

The various use cases will be tested as they are implemented, for example, an
NVMe drive installed in a chassis should report inventory and thermal
information to Redfish under the various schemas (Storage/Drives/ect)

Stress testing will be performed on the BMC via simple scripting to determine
the limits of the MCTP bandwidth, and to ensure that rare cases (like bus
collisions) are handled and recovered correctly.  In practice, this will
generally be implemented by repeating a single command as fast as the interface
allows.

System cycling testing will be performed to ensure that MCTP devices are
detected properly each time, and do not contain any low probability race
conditions or collisions.

Because MCTP is implemented over a physical interface, systems under test will
likely be verified with a protocol analyser in the early stages.  Common
analyzers that might be used are
[Beagle](https://www.totalphase.com/products/beagle-i2cspi/) and [Bus Pirate](http://dangerousprototypes.com/docs/Bus_Pirate)