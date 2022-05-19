# nvmed Design - a Daemon for NVMe Out-Of-Band Management

Author:
  Hao Jiang (Haoj)

Other contributors:
  'None'

Created:
  Feb 10, 2022
  
## Problem Description

NVMe daemon(nvmed) serves as an abstraction for NVMe subsystem, enumerating NVMe
controllers within the subsystem and providing NVMe-MI functions over DBus
interface. The client could acchieve out-of-band Management, such as
`GetLogPage`, or `Identity`, to these NVMe controllers.

## Background and References

NVM Express Workgroup defines a command set and architecture for managing NVMe
storage in the specification of NVMe Management Interface([NVMe-MI](https://nvmexpress.org/developers/nvme-mi-specification/)).
BMC could access the NVMe-MI via the out-of-band mechanism of MCTP over SMBus/I2C
or MCTP over PCIe Vendor Defined Messages(VPD).

The first version of nvmed will be working on the former physical channel and
built upon the following existing projects:

* [libmctpd](https://github.com/openbmc/libmctp): An implementation for
Management Component Transport Protocol(MCTP).
* [mtcpd](https://github.com/CodeConstruct/mctp): Userspace tools for MCTP stack
management.
* [libnvme-mi](https://github.com/CodeConstruct/libnvme): library to implement
NVMe-MI functions over MCTP message.

There is an existing project, [phosphor-nvme](https://github.com/openbmc/phosphor-nvme),
also targeting of NVMe devices. However phosphor-nvme serves a total different
purpose comparing to the proposed nvmed:

1. nvmed represents a pure NVMe subsystem with standard APIs according to
NVMe-MI spec, while phosphor-nvme works more like a NVMe device/enclosure, with
sensors, LEDs and GPIOs.
2. nvmed isoluted the physical implement to the standard MCTP layer, while
phosphor-nvme implements its own customized version of i2c driver.
3. nvmed servers as a reactor for NVMe subsystem enumeration, so that it can
work with VPD and VPD-less NVMe devices. While phosphor-nvme maintains its
own inventory configuration based on EEPROM.

## Requirements

nvmed provides standard hierachy and interfaces of a NVMe subsystem as defined
in NVMe(MI) spec, which includes:

1. Intiate the MCTP remote endpoints for the NVMe subsystems.
2. Enumerate the controllers within each NVMe subsystem.
3. NVMe-MI interface to require the basic information for the NVMe subsystem.
4. NVMe Admin interface for each controller.

The client of the nvmed could be other services on BMC which need to interact
with the NVMe device for OOB Management. The potential customers will be:

* BMCWeb to expose the Storage structure to Redfish interface.
* TCG daemon to send and receive secured messages to NVMe Device.
* Log daemon to expose the NVMe log pages.

## Proposed Design

### NVMe Subsystem Initialization

nvmed will initiate the NVMe subsystem by constantly monitoring the NVMe DBus
interface of `xyz.openbmc_project.Configuration.NVMe`, which is typically
created by Entity-Manager(E-M). On a NVMe device with Fru EEPROM and OOB
controller on I2C bus, such E-M configration will trigger nvmed enueration:

```json
{
  "Exposes": [
    {
      "Type": "NVMe",
      "Name": "ExampleName",
      "MCTPBus": "I2C",
      "Address": "0x1d"
     }
  ],
  "Probe": "xyz.openbmc_project.FruDevice({'BOARD_PRODUCT_NAME': 'ExampleProduct'})"
}
```

Similarly, E-M can probe other DBus services, such as SMBus ARP daemon, to
start the initialization of VPD-less NVMe device.

Once the NVMe initialization is triggered, the nvmed will create the MCTP
remote endpoint for the NVMe subsystem via MCTPd's DBus interface.

```text
# busctl introspect xyz.openbmc_project.MCTP /xyz/openbmc_project/mctp
NAME                                TYPE      SIGNATURE  RESULT/VALUE  FLAGS
au.com.CodeConstruct.MCTP           interface -          -             -
.AssignEndpoint                     method    say        yisb          -
.LearnEndpoint                      method    say        yisb          -
.SetupEndpoint                      method    say        yisb          -
```

Via versa, nvmed will remove the MCTP endpoint from MCTPd by:

```text
# busctl introspect xyz.openbmc_project.MCTP /xyz/openbmc_project/mctp/1/9
NAME                                TYPE      SIGNATURE RESULT/VALUE
au.com.CodeConstruct.MCTP.Endpoint  interface
.Remove                             method    -         -         
```

### Initialization Alternatives

Instead of `xyz.openbmc_project.Configuration.NVMe`, E-M will create
`xyz.openbmc_project.Configuration.MCTP`, on which the MCTPd will probe. And
nvmed will monitor the `xyz.openbmc_project.MCTP.Endpoint` with
`SupportedMessageTypes` has 04h(NVMe Type).

### NVMe Controller Enueration

nvmed will automatically enumerate the controllers within the subsystem and
post them on DBus. The topology of NVMEd DBus objects follows the same
architecture of NVMe subsystem architecture(Figure3 in NVMe [MI spec rev1.2](https://nvmexpress.org/wp-content/uploads/NVM-Express-Management-Interface-1.2-2021.06.02-Ratified.pdf)).

The corresponding DBus object will be:

```text
/xyz/openbmc_project/nvme/(subsys)1/(controller)0
                                   /(controller)1
                                   /(controller)2
```

In SR-IOV case(Figure 19, [NVMe Base Spec rev2.0a](https://nvmexpress.org/wp-content/uploads/NVMe-NVM-Express-2.0a-2021.07.26-Ratified.pdf)),
DBus object will follow the structure of:

```text
/xyz/openbmc_project/nvme/(subsys)1/(PF)0
                                   /(PF)0/(VF)1
                                         /(VF)2
```

Note nvmed will only cache the topology information while enumeration, it won’t
cache any runtime status other than that. All other information should be
achieved by directly polling.

### Enumeration Alternatives

Alternatively we can flat the PF/VF and rely on client to pull the relationship
via Identity.

### Interfaces

#### /xyz/openbmc_project/nvme/\${subsys}

| Interface                         | Property/Method     | Comment                                      |
| --------------------------------- | ------------------- | -------------------------------------------- |
| xyz.openbmc_project.NVME.MCTPInfo | Path                | Dbus object path of nvme endpoint from mctpd |
| xyz.openbmc_project.NVME.PortInfo | Type                | NVMe MI Spec 5.7                             |
|                                   | CIAPS               | NVMe MI Spec 5.7                             |
|                                   | MctpMTUMax          | NVMe MI Spec 5.7                             |
|                                   | MEB                 | NVMe MI Spec 5.7                             |
|                                   | VPDSMBAddr          | NVMe MI Spec 5.7                             |
|                                   | VPDFreq             | NVMe MI Spec 5.7                             |
|                                   | MctpAddr            | NVMe MI Spec 5.7                             |
|                                   | MctpFreq            | NVMe MI Spec 5.7                             |
|                                   | NvmeBasicManagement | NVMe MI Spec 5.7                             |
| xyz.openbmc_project.NVME.MI       | HealthStatusPoll()  | NVMe MI Spec 5.6                             |
|                                   | ReadData()          | NVMe MI Spec 5.7                             |

#### /xyz/openbmc_project/nvme/\${subsys}/\${cid}

| Interface                        | Property/Method | Comment             |
| -------------------------------- | --------------- | ------------------- |
| xyz.openbmc_project.NVME.MIAdmin | GetLogPage()    | NVMe Base Spec 5.16 |
|                                  | Identify()      | NVMe Base Spec 5.17 |
|                                  | SecuritySend()  | NVMe Base Spec 5.26 |
|                                  | SecurityRecv()  | NVMe Base Spec 5.25 |

## Alternatives Considered

Stated in the above chapter.

## Impacts

Given nvmed is exposing the standard NVMe interface, others should be easy to
build their own NVMe service on top of nvmed. For example a NVMe sensor daemon
can construct a standard thermal sensor by pulling SMART log. Another example
will be a NVMe log daemon can expose the log page from NVMe device and tunneling
the logs to Redfish/IPMI/journald. However, since these functionility is outside
the definition of NVMe subsystem, it is not within the scope of nvmed.

The physical transportation is also out of the scope of nvmed, which is purely
depending on the MCTP implementation. So it is going to be fairly easy to
support MCTP over PCI VDM if the underlay supports it.

### Organizational

The nvmed can live in paralell within phosphor-nvme, given the functionality of
these two daemon are complementary.

## Testing

* Unit test can be done with MOCK libnvme-mi.
* Regression test can be performed with supported SSD.
