# libocpami

Author: Gilbert Chen

Other contributors: Deepak Kodihalli

Created: Created: November 22, 2024

## Problem Description

Enable OpenBMC applications, such as d-bus sensor apps, to read GPU sensors
using an OCP MCTP VDM protocol
[OCP GPU & Accelerator Management Interfaces Specification](https://www.opencompute.org/documents/ocp-gpu-accelerator-management-interfaces-v0-9-pdf).
A reusable libocpami library is proposed for applications to encode and decode
OCP VDM messages.

## Background and References

According to the MCTP OEM VDM section of
[OCP GPU & Accelerator Management Interfaces Specification](https://www.opencompute.org/documents/ocp-gpu-accelerator-management-interfaces-v0-9-pdf)
, the device vendors retrieve telemetry data over MCTP OEM VDM because the
additional OEM telemetry that will be harvested by the management controller
doesn't meet the existing definition of the PLDM Type 2 ecosystem.
Therefore, OpenBMC must obtain the additional OEM telemetry via the OCP MCTP OEM
VDM protocol.

## Requirements

There is motivation to separate out a library part from the user apps for the
OCP VDM message and:
- There is OCP/industry interest to use the library on BMC and device firmware
stacks.
- There is OCP/industry interest to use the library on OpenBMC as well as
non-OpenBMC BMC stacks.
- Given the most common usage of the library is on OpenBMC-based BMCs, house the
library repo in OpenBMC (similar to libcper, libpldm).

Given the motivation above, following are the requirements for this library:
  - The library should be implemented in C.
  - The library should not have any dependency other than C standard library.
  - The library should support architectures with any endianness.
  - The library should not output any form of logs.
  - The library should not maintain any state.
  - The library should only provide encoding and decoding of messages using
    local buffers only and should not provide any mechanism for sending the
    message payloads to device endpoints.

## Proposed Design

- Develop a libocpami library within OpenBMC to encode and decode OCP-VDM
  messages, enabling OpenBMC applications to communicate with GPUs. The
  libocpami will provide essential APIs in the initial release to obtain common
  telemetry such as temperature and power sensors via OCP VDM messages.The APIs
  listed below are examples:

  - ocpami_encode_get_temperature_reading_req()
  - ocpami_decode_get_temperature_reading_req()
  - ocpami_encode_get_temperature_reading_resp()
  - ocpami_decode_get_temperature_reading_resp()
  - ocpami_encode_get_current_power_draw_req()
  - ocpami_decode_get_current_power_draw_req()
  - ocpami_eecode_get_current_power_draw_resp()
  - ocpami_decode_get_current_power_draw_resp()

- To demonstrate the use of libocpami in the OpenBMC framework, develop a D-Bus
  Sensor app using the libocpami API and the in-kernel AF_MCTP stack to get GPU
  sensor readings and expose them to the xyz.openbmc_project.sensor.value D-Bus
  interface.

```text
         ┌──────────────────────┐          ┌────────────────┐
         │ D-Bus GPU sensor App │   link   │   libocpami    │
         │                      │  ◄────── │                │
         └────────┬─────────────┘          └────────────────┘
                  │   ▲
                  │   │
                  ▼   │
         ┌────────────┴─────────┐
         │  In-kernel AF_MCTP   │
         │                      │
         └────────┬─────────────┘
                  │   ▲
                  │   │
                  ▼   │
              ┌───────┴───┐
              │    GPU    │
              │           │
              └───────────┘
```

## Alternatives Considered

An alternative design is to have the GPU management application and library in
the same repository. This approach simplifies source code management but can
make it difficult for others to reuse the library API on non-OpenBMC and device
firmware stacks.

## Impacts

New repo will be created within the organization. New recipe will be added to
OpenBMC.

### Organizational

- Does this repository require a new repository? Yes
- Who will be the initial maintainer(s) of this repository? TBD
- Which repositories are expected to be modified to execute this design?
  - openbmc/openbmc
  - openbmc/libocpami
  - openbmc/dbus-sensors
- Make a list, and add listed repository maintainers to the Gerrit review.
  - maintainers
    - Ed Tanous(etanous@nvidia.com)

## Testing

- Unit tests to verify basic functionality.
- An D-Bus sensor application using libocpami will communicate with the GPU to
  verify the end-to-end path.
