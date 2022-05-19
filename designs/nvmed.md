# Design Proposal for NVMe Out-Of-Band Management

Author: Hao Jiang (Haoj)

Other contributors: Ed Tanous (edtanous), Jeremy Kerr

Created: Feb 10, 2022

## Problem Description

Typically the NVMe storage devices are directly accessed and controlled by the
software on Host machines. However, when the control domain are shifting from
CPU to BMC, the NVMe management is transiting into Out-Of-Band instead of
inband, meaning the DC software will directly talk through BMC for all the NVMe
Management tasks, while letting the computation node doing pure I/O transaction.
The classic scenario of NVMe management tasks will include but not be limited
to:

*   Enumerating the NVMe subsystems attached to the BMC.
*   Reading and expose the information for each NVMe subsystem, such as VPD
    info, health status, etc.
*   Identify the controllers within each subsystem and expose their basic
    information including firmware info, supported nvm commands, capacity,
    self-test time, etc.
*   Collect logs from each controller, including but not limited to Errors,
    SMART, Telemetry Host Initiated, etc.

Other than reading out from the NVMe device listed above, the DC software also
requires some advanced operations on the NVMe device, including:

*   Performing secured operations on encrypted NVMe device via
    [TCG storage spec](https://trustedcomputinggroup.org/wp-content/uploads/TCG_Storage_Architecture_Core_Spec_v2.01_r1.00.pdf),
    such as lock/unlock the device/LBA band, read SED DataStore, etc
*   Config the NVMe features (HMB/Host Memory Buffer for the moment) before
    logging-in and cleaning up the namespaces and/or entire subsystem after
    log-out

There lists the detailed NVMe specification corresponding to the previous tasks.

*   GetLogPage
    *   LogPageId=0x02 SMART/health data, issue the command to NVM subsystem.
    *   LogPageId=0x02 SMART/health data, issue the command to a namespace.
    *   LogPageId=0x03 Firmware slot info
    *   LogPageId=0x04 Changed namespace list
    *   LogPageId=0x05 Commands supported and effects
    *   LogPageId=0x06 Device self test on NVM subsystem
    *   LogPageId=0x06 Device self test on controller
    *   LogPageId=0x07 Telemetry host initiated
    *   LogPageId=0x08 Telemetry controller initiated
    *   LogPageId=0x09 Endurance group info
    *   LogPageId=0x0A Predictable latency per NVM set
    *   LogPageId=0x0B Predictable latency event aggregate
    *   LogPageId=0x0C Asymmetric namespace access
    *   LogPageId=0x0D Persistent event log
    *   LogPageId=0x0E LBA status info
    *   LogPageId=0x0F Endurance group even aggregate
    *   LogPageId=0x80 Reservation notification
    *   LogPageId=0x81 Sanitize status
*   Identify_controller
    *   controller_id
    *   vendor_id
    *   subsystem_vendor_id
    *   serial_number
    *   model_number
    *   firmware_revision
    *   firmware_update_capabilites
    *   optional_admin_commands_supported
    *   optional_nvm_commands_supported
    *   critical_temperature_threshold
    *   warning_temperature_threshold
    *   total_capacity
    *   max_data_transfer_size
    *   number_of_namespaces
    *   max_error_log_page_entries
    *   sanitize_capabilities
    *   extended_device_self_test_time_minutes
    *   subsystem_nqn
    *   write_zeroes_nvm_command_supported
*   Identify_namespace
    *   capacity_bytes
    *   formatted_lba_size_bytes
    *   namespace_guid
    *   data_protection_type
    *   lba_format_index
    *   protection_information_location
    *   metadata_setting
*   SecuritySend/Recv
    *   Unstructured data defined by TCG
*   Get/SetFeature_HMB
    *   Memory Return
    *   Enable Host Memory
    *   Host Memory Buffer Size
    *   Host Memory Descriptor List Address
    *   Host Memory Descriptor List Entry Count

## Background and References

NVM Express Workgroup defines a command set and architecture for managing NVMe
storage in the specification of NVMe Management
Interface([NVMe-MI](https://nvmexpress.org/developers/nvme-mi-specification/)),
via which the Data Center could achieve the out-of-band(OOB) control to the NVMe
devices through BMC.

On the north bound interface,
[Swordfish](https://www.snia.org/forums/smi/swordfish), which is an extension of
DMTF Redfish specification, providing a RESTful API for NVMe storage device as
well as the mapping guidance between the standard NVMe definition and RESTful
schema.

Other useful repositories that are already merged into OpenBMC are:

*   [libmctpd](https://github.com/openbmc/libmctp): An implementation for
    Management Component Transport Protocol(MCTP).
*   [mtcpd](https://github.com/CodeConstruct/mctp): Userspace tools for MCTP
    stack management.
*   [libnvme-mi](https://github.com/CodeConstruct/libnvme): library to implement
    NVMe-MI functions over MCTP message.

They serve as the underlay driver to transmit NVMe messages between BMC and the
target device.

There are couple of OpenBMC services striking the NVMe area, which are:

*   [phosphor-nvme](https://github.com/openbmc/phosphor-nvme)
*   [NVMe Sensor](https://github.com/openbmc/dbus-sensors/commit/b669b6b54b1bfa499380e518bf508b672dd9a125)

Currently, both of the project create DBus sensor interface targeting thermal
reading. The transportation protocol is based on NVMe Basic Management Command
(Appendix A,
[NVMe MI specification](https://nvmexpress.org/wp-content/uploads/NVM-Express-Management-Interface-1.2-2021.06.02-Ratified.pdf)),
which returns SMART info via a SMBus block read.

phosphor-nvme statically enumerates the NVMe device via its own configuration,
while NVMe Sensor relies on the dynamic stack (A.K.A Entity-Manager) to expose
the configuration interface(`xyz.openbmc_project.Configuration.NVME1000`).

## Requirements

The proposed service daemon will explore and expose the NVMe architecture
(including NVMe Subsystem, Controller, and Namespace) to the Dbus. Components
within NVMe architecture will follow
[mapping guidance](https://www.snia.org/sites/default/files/technical-work/swordfish/draft/v1.2.2/pdf/Swordfish_v1.2.2_NVMeMappingGuide.pdf)
from SNIA, in order to minimize the gap between DBus and Redfish/Swordfish
interface definition.

The daemon will retrieve the information from NVMe device through NVMe-MI API/s,
i.e. the NVMe MI commands and NVME-MI Admin commands. The daemon will be built
upon the existing openBMC kernel patches and userspace library for protocol
parsing(libnvme), and message transfer(mctp kernel subsystem).

The daemon will poll the device and translate the data definitions between NVMe
specification and Redfish/Swordfish schema, exposing Dbus Interfaces by DBus
interfaces, including:

*   `xyz.openbmc_project.Inventory.Item.Drive`
*   `xyz.openbmc_project.Inventory.Item.Storage`
*   `xyz.openbmc_project.Inventory.Item.StorageController`
*   `xyz.openbmc_project.Inventory.Item.Volume`

The daemon will also define NVMe protocol interface, including `GetLogPage` and
`Identify`. They are targeting the attributes that are not yet scoped in
Redfish/Swordfish schemas and thus serve for non-Redfish services (e.g
/google/v1 domain in BMCWeb). The raw data interface will be deprecated when
Swordfish are aligned with NVMe Spec.

## Proposed Design

### NVMe Subsystem Enumeration

The enumeration for NVMe subsystem will reuse the current reactor model of NVMe
sensor. Configuration interface will be preserved as
`xyz.openbmc_project.Configuration.NVME1000`. And we will introduce a new enum
field to the configuration, named `Protocol`.

| **Value**       | **Description(NVMe MI specification rev1.2)**       |
| --------------- | --------------------------------------------------- |
| Basic (default) | NVMe basic management commands (Appendix A)         |
| MiI2c           | NVMe MI commands over MCTP over I2C (Chapter 2.2)   |
| MiVdm           | NVMe MI commands over MCTP over PCIe Vendor Defined |
:                 : Messages(VDMs) (Chapter 2.1)                        :

An example for NVMe configuration:

```json
{
  "Exposes": [
    {
      "Type": "NVMe1000",
      "Name": "ExampleNVMe",
      "Protocol": "mi_i2c",
      "Bus": "$bus",
      "Address": "0x1d",
      "Thresholds": [
        {
            "Direction": "greater than",
            "Name": "upper critical",
            "Severity": 1,
            "Value": 115
        },
        {
            "Direction": "greater than",
            "Name": "upper non critical",
            "Severity": 0,
            "Value": 110
        },
        {
            "Direction": "less than",
            "Name": "lower non critical",
            "Severity": 0,
            "Value": 5
        },
        {
            "Direction": "less than",
            "Name": "lower critical",
            "Severity": 1,
            "Value": 0
        }
      ]
     }
  ],
  "Probe": "xyz.openbmc_project.FruDevice({'BOARD_PRODUCT_NAME': 'ExampleProduct'})"
}
```

The proposal will focus on `mi-i2c` protocol. The `basic` has been supported by
current NVMe sensor.

### NVMe Subsystem Structure

The NVMe daemon will create the following components within a NVMe subsystem.
This portion of design majorly targets at Redfish/Swordfish. The exposed
interfaces are one-to-one mapping to Redfish resources. Associations are created
corresponding to redfish links, and properties are parsed. Redfish server such
as BMCWeb should be able to directly passthrough the DBus objects into Redfish
domain.

<table class="blueTable">
<caption>NVMe Subsystem Structure</caption>
  <tr>
   <td><strong>Code</strong>
   </td>
   <td><strong>NVMe Spec</strong>
   </td>
   <td><strong>DBus</strong>
   </td>
   <td><strong>Note </strong>
   </td>
  </tr>
  <tr>
   <td>NVMeContext
   </td>
   <td>MCTP Endpoint
   </td>
   <td>/xyz/openbmc_project/inventory/system/board/{prod}/{nvme}<ul>

<li>Association: {“MCTP”, “storage”, “/Path/to/MCTPd/EP”}</li></ul>

   </td>
   <td>A NVMe Dbus object under chassis and association interface to MCTP EP
   </td>
  </tr>
  <tr>
   <td>|-SubSystem
   </td>
   <td>NVMe Subsystem
   </td>
   <td>/xyz/openbmc_project/inventory/system/board/{prod}/{nvme}
   </td>
   <td>Each NVMe Subsystem should have one MCTP controller (Endpoint)
<p>
(1.4, NVMe-MI Spec rev 1.2)
   </td>
  </tr>
  <tr>
   <td>|  |-Storage
   </td>
   <td>HealthStatus
<p>
IdentifyControoller
   </td>
   <td>/xyz/openbmc_project/inventory/system/board/{prod}/{nvme}<ul>

<li>xyz.openbmc_project.Inventory.Item.Storage
<li>Association: {“chassis”, “storage”, “/openbmc_project/inventory/system/board/{prod}/”}</li></ul>

   </td>
   <td>Storage interface and its association to Chassis
<p>
Health status and identifier for NVMe subsystem
<p>
(6.3.2, Swordfish Mapping Guide)
   </td>
  </tr>
  <tr>
   <td>|  |-Drive
   </td>
   <td>VPDRead
<p>
IdentifyController
   </td>
   <td>/xyz/openbmc_project/inventory/system/board/{prod}/{nvme}<ul>

<li>xyz.openbmc_project.Inventory.Item.Drive</li></ul>

   </td>
   <td>Drive interface
<p>
(6.8.2, Swordfish Mapping Guide)
   </td>
  </tr>
  <tr>
   <td>|  |-Sensor
   </td>
   <td>HealthStatus (CTEMP)
   </td>
   <td>/xyz/openbmc_project/sensors/temperature/{prod}_{nvme}<ul>

<li>xyz.openbmc_project.Sensor.Value
<li>Association: {“storage”, “sensors”}, “/openbmc_project/inventory/system/board/{prod}/{nvme}”}
<li>Association: {“chassis”, “all_sensors”, “/openbmc_project/inventory/system/board/{prod}”}</li></ul>

   </td>
   <td>The sensor should be associated with both Chassis and Storage(Subsys).
<p>
NVMe devices would provide thermal info subsystem wide (via HealthStatusPoll) or controller level(via SMART log).
<p>
Swordfish should map the sensor under `/redfish/v1/Chassis/{ChassisId}/Sensors/{SensorId}` . And construct `RelatedItem (v1.2+) [ { } ]` property by exploring the DBus association relation.
<p>
(Figure 8, Swordfish Mapping Guide)
   </td>
  </tr>
  <tr>
   <td>|  |-Controller Array
   </td>
   <td>NVMe-MI Data (Controller List)
   </td>
   <td>/xyz/openbmc_project/inventory/system/board/{prod}/{nvme}/controllers
   </td>
   <td>
   </td>
  </tr>
  <tr>
   <td>|  |  |-Controller
   </td>
   <td>NVMe Controller
   </td>
   <td>/xyz/openbmc_project/inventory/system/board/{prod}/{nvme}/controllers/{CTRLID}<ul>

<li>xyz.openbmc_project.Inventory.Item.StorageController
<li>Association (primary only): {“secondary”, “primary”, “/xyz/openbmc_project/inventory/system/board/{prod}/{nvme}/controllers/{CTRLID}”}</li></ul>

   </td>
   <td>StorageController properties refer to
<p>
(6.4, Swordfish Mapping Guide)
<p>
A primary-secondary controller association is used for SR-IOV use case. The client (BMCWeb) may expose the PF only.
   </td>
  </tr>
  <tr>
   <td>|  |-Namespace Array
   </td>
   <td>Identify (Namespace Identification Descriptor List)
   </td>
   <td>	/xyz/openbmc_project/inventory/system/board/{prod}/{nvme}/namespaces
   </td>
   <td>
   </td>
  </tr>
  <tr>
   <td>|  |  |-Namespace
   </td>
   <td>Namespace
   </td>
   <td>	/xyz/openbmc_project/inventory/system/board/{prod}/{nvme}/namespaces/{nid}<ul>

<li>xyz.openbmc_project.Inventory.Item.Volume
<li>Association: {“controller”, “namespace”, “/xyz/openbmc_project/inventory/system/board/{prod}/{nvme}/controllers/{CTRLID}”}</li></ul>

   </td>
   <td>Volume properties refer to
<p>
(6.5.2, Swordfish Mapping Guide)
<p>
Namespaces can be attached to multiple controllers, so association is introduced instead of sub-path.
   </td>
  </tr>
</table>

<style>
    table.blueTable td:nth-child(1){
        white-space: nowrap;
    }
</style>

<style>
    table.blueTable td,
    table.blueTable th {
        border: 1px solid #AAAAAA;
        padding: 3px 2px;
        text-align: left;
    }
</style>

### NVMe Admin Interface

A NVMeAdmin interface will be attached to each controller. It will transfer the
unparsed data in/out to the NVMe device via NVMe Admin message. The reasons for
such a requirement are:

*   The Swordfish is still catching up the NVMe specification. The current DC
    software requests some telemetries outside the current scope of Swordfish.
    Refer to "NVMe OOB Requirements" table for details.
*   The existing inband NVMe Management software stack is build upon NVMe spec
    API/s. The protocol styled API over DBus will mitigate the transition from
    inband to OOB, until all infrastructure is ready for Swordfish API.

The client of NVMe Admin interface will be company(Google) domain of BMCWeb. The
raw bytes is imcapatible with Redfish domain with structured data.

The interface will be deprecated when both Client software and Swordfish get
matural.

<table class="blueTable">
  <tr>
   <td><strong>Controller</strong>
   </td>
   <td><strong>NVMe Spec</strong>
   </td>
   <td><strong>DBus</strong>
   </td>
   <td><strong>Note (NVMe Spec r1.4)</strong>
   </td>
  </tr>
  <tr>
   <td>NVMeAdmin
   </td>
   <td>NVMe Admin CMDs
   </td>
   <td>/xyz/openbmc_project/sensors/temperature/{prod}_{nvme}/controllers/{CTRLID}<ul>

<li>xyz.openbmc_project.Nvme.NVMeAdmin</li></ul>

   </td>
   <td>The interface will live in NVMe daemon repo instead of phosphor-dbus-interface
   </td>
  </tr>
  <tr>
   <td>|-GetLogPage()
   </td>
   <td>GetLogPage
   </td>
   <td>‘s’ GetLogPage(LID, NSID, LSP, LSI)
   </td>
   <td>LID: Log Page Identifier
<p>
NSID: Namespace Identifier
<p>
LSP: Log Specific Field
<p>
LSI: Log Specific Identifier
<p>
return: the path of file which stores the returned raw data from NVMe device
<p>
The whole log page will be read and returned to the caller. If the the log length is variable, multiple libnvme call may be made before return and the last libnvme function call will clear RAE.
   </td>
  </tr>
  <tr>
   <td>|-Identify()
   </td>
   <td>Identify
   </td>
   <td>‘s’ Identify(CNS, NSID, CNTID)
   </td>
   <td>CNS: Controller or Namespace Structure
<p>
NSID: Namespace Identifier
<p>
CNTID: Controller Identifier
<p>
return: the path of file which stores the returned raw data from NVMe device
<p>

If the Identify structure is variable-length, the DBus function will return the full length of the list. It may involve multiple libnvme transaction.
   </td>
  </tr>
  <tr>
   <td>Error
   </td>
   <td>Status Code
   </td>
   <td>xyz.openbmc_project.Nvme.NVMeAdmin.errors
   </td>
   <td>Refer to Figure 128, NVMe base spec
   </td>
  </tr>
</table>

## Alternatives Considered

The NVMe daemon can expose the protocol API on DBus and work as a NVMe device
proxy. The daemon manages the transaction across multiple services to the remote
device. The client, such as BMCWeb, takes the responsiblity for data parsing and
resource construction.

This solution has been discussed on the
[gerrit](https://gerrit.openbmc.org/c/openbmc/docs/+/53809/2). The conclusion is
that the standard DBus storage interfaces are preferred to be constructed with
the NVMe daemon, which providing the structured data as passing-through to
Redfish.

## Impacts

The proposed nvme daemon will serves as the backend of Swordfish API. And it
will makeup the DBus storage model and simplify the logic within The DCSW will
have a more standardized and smooth way to migrate from inband management to OOB
on NVMe device.

Two NVMe spec API are exposed on DBus. Given they are both focused on telemetry
reading, and none of them are security operation such as
format/santize/namespace deletion, there should be no security impact on current
BMC system.

### Organizational

The NVMe daemon can be the enhancement upon the current NVMe sensor and live
within the phosphor-dbus-sensor repository.

Given the complexity of the NVMe specification and the expanding enhancement of
Swordfish upon Redfish, it may deserve an independent repository and migrating
the current NVMe sensor(thermal) into the repo. It allows more storage focused
maintainer to work on a dedicated place comparing to the general sensor repo.

## Testing

*   A Mock libnvme will be created for functional test.
*   The performance will be evaluated from the following aspects:
    *   I2C load when transferring large message, e.g. Persistent Log.
    *   Interface from/toward inband IO.
    *   Delays across multiple queries, e.g. delays on thermal sensor while a
        large log transfer.

[^1]: In "Appendix A Technical Note of NVMe-MI spec", it says "This appendix
    describes the NVMe Basic Management Command and is included here for
    informational purposes only. The NVMe Basic Management Command is not
    formally a part of this specification and its features are not tested by
    the NVMe Compliance program."
[^2]: Figure 114, NVM Express Management Interface Specification, Rev 1.2
[^3]: "NVMe Admin Commands over the out-of-band mechanism may interfere with
    host software", Page 107, NVM Express Management Interface Specification,
    Rev 1.2
