# ocp-vdm-stack

Author: Gilbert Chen

Created: Created: November 22, 2024

## Problem Description

Enable the OpenBMC application to read GPU sensor data using the OCP-VDM binding
protocol per the MCTP OEM VDM in the
[OCP GPU & Accelerator Management Interfaces Specification](https://www.opencompute.org/documents/ocp-gpu-accelerator-management-interfaces-v0-9-pdf).
A reusable libgpumgmt library is designed for applications to encode and decode
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

- The libgpumgmt library is designed for the OpenBMC app that requires
  communication with a GPU. It provides the necessary API for encoding and
  decoding OCP VDM messages.
- The libgpumgmt library might be possible to be used by projects other than
  OpenBMC.
- To demonstrate the use of libgpumgmt in the OpenBMC framework. Develop a D-Bus
  Sensor app using the libgpumgmt API and the in-kernel AF_MCTP stack to get GPU
  sensor readings and expose them to the xyz.openbmc_project.sensor.value D-Bus
  interface.

## Proposed Design

- Develop a libgpumgmt library within OpenBMC to encode and decode OCP-VDM
  messages, enabling OpenBMC applications to communicate with GPUs. The
  libgpumgmt will provide essential APIs in the initial release to obtain
  temperature and power sensor data via OCP VDM messages.

  - encode_get_temperature_reading_req()
  - decode_get_temperature_reading_resp()
  - encode_get_current_power_draw_req()
  - decode_get_current_power_draw_resp()

- Develop an OpenBMC D-Bus sensor application that utilizes the libgpumgmt
  library to expose GPU temperature and power sensors to D-Bus.

```text
         ┌──────────────────────┐          ┌────────────────┐
         │ D-Bus GPU sensor App │   link   │   libgpumgmt   │
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

An alternative design suggests using a monolithic OCP-GPU service to manage all
GPUs. Other OpenBMC services would use D-Bus IPC to interact with this service
for obtaining additional OEM telemetry via OCP VDM message. As a result, the
encode/decode API would not be needed as a library.

## Impacts

New repo will be created within the organization. New recipe will be added to
OpenBMC.

### Organizational

- Does this repository require a new repository? Yes
- Who will be the initial maintainer(s) of this repository? TBD
- Which repositories are expected to be modified to execute this design?
  - openbmc/openbmc
  - openbmc/libgpumgmt
  - openbmc/dbus-sensors
- Make a list, and add listed repository maintainers to the Gerrit review.
  - Ed Tanous(etanous@nvidia.com)
  - Deepak Kodihalli(dkodihalli@nvidia.com)
  - Gilbert Chen(gilbertc@nvidia.com)
  - Harshit Aghera(haghera@nvidia.com)

## Testing

- Unit tests to verify basic functionality.
- An D-Bus sensor application using libgpumgmt will communicate with the GPU to
  verify the end-to-end path.
