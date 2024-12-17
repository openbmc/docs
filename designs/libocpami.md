# libocpami

Author: Gilbert Chen

Created: Created: November 22, 2024

## Problem Description

Enable the OpenBMC application to read GPU sensor data using the OCP-VDM binding
protocol per the MCTP OEM VDM in the
[OCP GPU & Accelerator Management Interfaces Specification](https://www.opencompute.org/documents/ocp-gpu-accelerator-management-interfaces-v0-9-pdf).
A reusable libocpami library is designed for applications to encode and decode
OCP VDM messages.

## Background and References

According to the MCTP OEM VDM section of
[OCP GPU & Accelerator Management Interfaces Specification](https://www.opencompute.org/documents/ocp-gpu-accelerator-management-interfaces-v0-9-pdf)
, the device vendors expand telemetry data over MCTP OEM VDM because the
additional OEM telemetry that will be harvested by the management controller
that doesn't meet the existing definition of the PLDM Type 2 ecosystem.
Therefore, OpenBMC must obtain the additional OEM telemetry via the MCTP OEM VDM
binding protocol.

## Requirements

- The libocpami library is designed for the OpenBMC app that requires
  communication with a GPU. It provides the necessary API for encoding and
  decoding OCP VDM messages.
  - The library should be completely written in C.
  - The library should not have any dependency other than C standard library.
  - The library should support architectures with any endianness.
  - The library should not output any form of logs.
  - The library should not maintain any state.
  - The library should only provide encoding and decoding of messages using local
    buffers only and should not provide any mechanism for sending the message
    payloads to device endpoints.
- The libocpami library might be possible to be used by projects other than
  OpenBMC. The device firmware can use the library for encoding and decoding
  OCP VDM message.

## Proposed Design

- Develop a libocpami library within OpenBMC to encode and decode OCP-VDM
  messages, enabling OpenBMC applications to communicate with GPUs. The
  libocpami will provide essential APIs in the initial release to obtain
  temperature and power sensor data via OCP VDM messages. The APIs listed
  below are examples that will be included in the initial release, with
  additional APIs to be added in future releases.

  - ocpami_encode_get_temperature_reading_req()
  - ocpami_decode_get_temperature_reading_req()
  - ocpami_encode_get_temperature_reading_resp()
  - ocpami_decode_get_temperature_reading_resp()
  - ocpami_encode_get_current_power_draw_req()
  - ocpami_decode_get_current_power_draw_req()
  - ocpami_eecode_get_current_power_draw_resp()
  - ocpami_decode_get_current_power_draw_resp()

- To demonstrate the use of libocpami in the OpenBMC framework. Develop a D-Bus
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
make it difficult for others to reuse the library API.

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
