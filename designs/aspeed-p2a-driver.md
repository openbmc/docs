# ASPEED P2A Control Driver

Author: Patrick Venture venture@google.com
Primary assignee: Patrick Venture
Created: 2019-01-10

## Problem Description

The PCI-to-AHB bridge on the ASPEED AST2400 and AST2500, when available allows
for a high speed transfer mechanism of data to and from the BMC.  It does this
by allowing the BMC to expose regions of BMC memory to the host.

For security reasons, the regions and the bridge mechanism need to be disabled
by default on platforms that support this.  However, this bridge is useful for
transfers, such as a firmware update.  Therefore, the BMC needs a driver to
control enabling the bridge and more specifically enabling a specific region of
memory.

## Background and References

The P2A (PCI-to-AHB bridge) allows the host to specify a 64KiB window into the
BMC's memory.  Through a coordination between host and BMC, the host can stage
bytes of a firmware image in place for the BMC to copy out and piece together
the firmware image for an update.

The bridge itself is on by default on platforms where it's physically possible.
This includes systems where the BMC is attached as a PCI device (TODO: or only
if there's VGA too?).  There are control registers on the BMC that allow
disabling specific regions of the BMC's memory.  These registers are set to
allow any region by default.

A nefarious host can by default, configure the window into the BMC's memory
without BMC cooperation and read or write memory.  There are a series of
patches to the ASPEED BMC's u-boot source files that disable these types of
features.

There is an LPC mechanism that is very similar to this, and there is
correspondingly a driver
"[aspeed-lpc-ctrl](https://github.com/torvalds/linux/commit/f4d029098428da68b47eaba8094b5c0887a9edc8)"
that enables it on demand and allows a user-space application on the BMC to
configure the window via ioctl calls.  Therefore, it stands to reason that an
"aspeed-p2a-ctrl" driver could serve the same purpose.

## Requirements

The host needs to have access to the ASPEED P2A bridge and the BMC needs to
control this access, such that by default all access is disabled.  Only, when
an application on the BMC needs said bridge, will it be enabled and then the
host will have access.  Once the use is complete, the bridge will be disabled.
While enabled, only the smallest region possible via configuration will be
enabled, the rest will remain marked disabled.

The driver will be modeled after the "aspeed-lpc-ctrl" driver living in the
upstream linux source tree.  The BMC software will be updated, such that u-boot
disables all access, and the firmware update mechanism at run-time will request
the region its configured to use for updates when an update request is received
from the host.  Once this update request is serviced or aborted, it will close
the window.

Once enabled the P2A bridge grants read-write access to all memory regions,
however, those **regions marked disabled grants read-only access**.  Therefore,
once the P2A bridge is enabled, all regions are accessible for reading, until
it's disabled again.

PCI DMA access in general is beyond the scope of this driver.

## Proposed Design

### From the datasheet:

"
P2A is a one-way bus bridge providing a back door for host CPU to access all
the internal IP modules in ARM SOC sub-system. Since P2A is a one-way bridge,
ARM CPU cannot issue any PCI bus commands through the help of this bridge. In
a normal condition, this back door should be well locked. The two potential
usages of this bus bridge are:

1.  Updating flash memory through host CPU
1.  H/W or S/W debugging through host CPU
"


### Host Control

P2A only implements two sets of 32-bit registers to provide a protection
mechanism and specify the base address of the 64KB address re-mapping window.

The first register the host sets enables or disables the bridge.  The second
register controls the address used from P-Bus to AHB.

**Re-mapping base address**

"""
This register defines the address re-mapping scheme from P-Bus to AHB. Bit [31:16]
of AHB address is from the Bit [31:16] of this register, Bit [15:0] is directly from P-Bus
command address.

```
AHB Address = (Re-mapping base address[31:16]) + (P-bus address[15:0])

P2A will convert all the commands from P-bus with 64KB address range from:

 (MMIOBASE + 0x10000) to (MMIOBASE + 0x1FFFF).

Where MMIOBASE is the re-locatable memory-mapped
```

I/O base address defined in PCI configuration space. P2A supports byte, word or
double word type of access commands.
"""

The registers can be read by reading the offsets from BAR0.

### BMC Control

The following bit of the SCU 2Ch register controls the bridge.

<table>
  <tr>
   <td><strong>Bit</strong>
   </td>
   <td><strong>Description</strong>
   </td>
  </tr>
  <tr>
   <td>8
   </td>
   <td>Disable PCI bus to AHB bus bridge
<p>
0: Enable bridge (default)
<p>
1: Disable bridge
   </td>
  </tr>
</table>


The following bits of the SCU 2Ch register control the regions.

<table>
  <tr>
   <td><strong>Bit</strong>
   </td>
   <td><strong>Description</strong>
   </td>
  </tr>
  <tr>
   <td>25
   </td>
   <td>Disable DRAM address space write from P2A bridge
<p>
When disabled, P2A will mask all write command to address space:
<p>
0x80000000 - 0xFFFFFFFF
   </td>
  </tr>
  <tr>
   <td>24
   </td>
   <td>Disable LPC Host/Plus address space write from P2A bridge
<p>
When disabled, P2A will mask all write command to address space:
<p>
0x60000000 - 0x7FFFFFFF
   </td>
  </tr>
  <tr>
   <td>23
   </td>
   <td>Disable SOC address space write from P2A bridge
<p>
When disabled, P2A will mask all write command to address space:
<p>
0x10000000 - 0x1FFFFFFF
<p>
0x40000000 - 0x5FFFFFFF
   </td>
  </tr>
  <tr>
   <td>22
   </td>
   <td>Disable flash address space write from P2A bridge
<p>
When disabled, P2A will mask all write command to address space:
<p>
0x00000000 - 0x0FFFFFFF
<p>
0x20000000 - 0x3FFFFFFF
   </td>
  </tr>
</table>

### Enabling a Region (from the BMC)

1.  Open driver as file (only one user at-a-time allowed, enforced by driver
    during open), does not enable bridge
1.  Ioctl to driver to configure window, it enables the smallest region to
    accommodate your request and the bridge.
1.  Close driver file (disables all windows and disables bridge).


#### Ioctl Parameters

```
ASPEED_P2A_CTRL_IOCTL_SET_WINDOW
```

***TODO: define the value.***

Structure to pass:

```
struct aspeed_p2a_ctrl_mapping {
    __u32 addr;
};
```

`addr` - the address in BMC memory to set use.

The size is not specified because the access region is 64KiB (the host can map
only 64KiB at a time to any address within the region they choose that is
correspondingly opened by the BMC via the not-so-granular control bits.

The information used here must be provided back to the host.

#### Reading Data from and Writing Data to a Region (from the BMC)

The driver will expose an mmap interface that allows the MMU to map the kernel
region used to userspace.  This is similar to what's done in aspeed-lpc-ctrl,
to make the interfaces similar.

## Impacts

### Security Considerations

This driver is meant to address security considerations:

*   The host should not have arbitrary read-write access to the BMC's memory.
*   The host should require the BMC to open a specific window for access.

Even with this driver in place, during the period where the BMC has opened a
specific window for read-write access, all other regions (all of BMC memory) is
then opened for read-only access.

The bits controlling the range are few and the ranges are huge, even though the
P2A bridge window is only 64KiB.  Therefore, the region opened for read-write
is larger than the usable range.

The host can only set the window to one position at-a-time, therefore two host
controllers cannot reliably operate via this bridge without cooperation.

## Testing

***TODO: Formalize a bit***

1. Test that disabling the P2A in u-boot stays disabled and the host cannot
   override it.
1. Test that enabling the P2A and the region works as intended.
1. Test that re-disabling the P2A on "close" blocks host access (see test
   above).

