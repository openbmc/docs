# HPE Host to BMC Channel Interface Service

Author: 
Drew Neyland (drewn)

Other contributors: 
Dan Lewis (thepaladan)
Charles Kearney (charkear)

Created: January 16, 2024

Version: 1.0

## Problem Description 

OpenBMC needs to support HPE Channel Interface (CHIF) Service, a hardware 
based 2-way communication channel, in order to enable HPE ProLiant 
platforms. Without this feature, HPE platforms won't be able to boot.

## Background

The GXP ASIC in HPE ProLiant systems utilizes a dedicated hardware messaging 
bus (CHIF) defined by memory mapped registers and a doorbell interrupt 
mechanism for the GXP ASIC and host processor to communicate. The system ROM 
and operating systems running on the host processor utilize this hardware 
interface to communicate with the BMC. The CHIF bus hardware utilizes memory 
mapped registers and doorbell interrupts to represent 32 different channels. 
The CHIF driver is used to send SMIF (Shared Memory Interface) messages over 
this hardware bus. Because of this dependency on the hardware communication 
bus present in the GXP ASIC and the dependence on it in all HPE System ROMs, 
this solution is not possible on a non GXP ASIC based system.

The CHIF hardware communication system is used to carry data messages called 
SMIF packets. The CHIF service running in HPE BMC software stack decodes 
SMIF message packets, performs the appropriate actions, and puts any response 
back on the same channel the packet came in on. HPE System ROMs utilizes the 
SMIF packet format and CHIF interface during initialization of the host 
processor. HPE System ROMs rely on this to retrieve system information from 
the BMC (i2c topology, etc), and to send information about the current system 
state (ex: boot process location, network info, product ID, PCI devices, BIOS 
Environment Variables, etc) to the BMC. HPE OpenBMC implementations will need 
to have the CHIF service running on the GXP ASIC to ensure that required SMIF 
packet responses and actions are taken for the System ROM to operate properly. 

Currently a version of the CHIF hardware interface driver exists in the Linux 
kernel for Host Operating System access to the CHIF messaging bus. This driver 
is located at:
https://github.com/torvalds/linux/blob/master/drivers/misc/hpilo.c 

## Requirements

On HPE ProLiant servers outside of OpenBMC, a CHIF driver and a service 
written in C is already used and supported, this project will be a port of the 
service to C++. The goal of the port is to get the project hosted in a 
dedicated repo on the OpenBMC GitHub, with the development and maintenance 
responsibilities held by HPE. Based on this, the service will fit into the 
'Developer opt-in features' option class (per architecture/optionality.md)

The CHIF service must respond to System ROM requests for:
- Network Information (provide network address to ROM)
- System Information (serial number, FW version, security state etc)
- NIC Config
- License Information
- BIOS Authorization
- HPE System ROM Environmental Variables
- SMBIOS Records

## Proposed Design

As stated earlier, the approach to enabling the necessary communication 
between the System ROM and BMC is two pronged. A driver is used to collect 
data/requests in the kernel and relay the information on the CHIF bus. The 
CHIF service will run on the BMC to handle SMIF and ROM requests on the bus. 
The service will need to access to the information referred to in Requirements 
(see above).

- CHIF Driver: Driver interfaces with CHIF hardware to send/receive packets on
 shared memory bus

- CHIF Service:
    - read/decode packet from CHIF bus

    - perform appropriate actions within the BMC

    - write response back on CHIF bus 

## Alternatives Considered


### MMBI
Memory-Mapped BMC Interface (MMBI) Specification: 
https://www.dmtf.org/sites/default/files/standards/documents/DSP0282_1.0.0.pdf

#### Pros:
- Will accomplish the same goals as CHIF
#### Cons:
- Not out yet
- Could not be used on the existing generation of HPE hardware


## Impacts

Because CHIF would only run on HPE servers, it would not have a significant 
impact on the broader OpenBMC community. Because of the dependency this 
project has on HPE proprietary technology (CHIF bus, SMIF packets), this 
would not be expected to be adopted by other platforms. The responsibility for
maintenance and documentation for it would be on HPE.

## Organizational

- Does this repository require a new repository? (Yes, No)
    - Yes
- Who will be the initial maintainer(s) of this repository?
   - Jean-Marie
   - Dan Lewis
   - Drew Neyland
   - Charles Kearney
   - Grant OConnor
   - Chris Sides
- Which repositories are expected to be modified to execute this design?
   - Components within the Meta-HPE layer of the OpenBMC repo are the only 
   ones expected to need to be modified.
- Make a list, and add listed repository maintainers to the Gerrit review.
    - Meta-HPE
        - verdun@hpe.com
        - nhawkins48@gmail.com
        - matt.fischer@hpe.com
        - charles.kearney@hpe.com

## Testing

If installed and running properly, CHIF will appear as an active service on 
the BMC. Investigation is still being conducted into how to automate testing 
for this, but at the moment, automated testing cannot be conducted at all on 
HPE systems with the current CI tools. Getting CHIF hosted is one of the 
requirements to help support and integrate HPE servers into the OpenBMC eco 
system.
