# HostBootTimeMonitor

Author: Michael Shen <gpgpgp@google.com>

Other contributors: Medicine Yeh <medicineyeh@google.com>

Created: June 25, 2022

## Problem Description

In the server area boot time is one of the key factors of the performance.
Having a detailed reboot time measurement provides us a clear data of the time
cost of each reboot [stage](#definition-of-each-stage).

This design is meant to breakdown the host reboot process into individual chunks
and provide a mechanism to measure its duration.

## Background and References

### phosphor-state-manager

phosphor-state-manager is an OpenBMC Daemon which tracks and controls the state
of different objects including the BMC, Chassis, Host, and Hypervisor. Some of
the interface provides useful properties that can be leveraged in reboot time
measurement. For example
[CurrentHostState](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/Host.interface.yaml)
provides tells us current host firmware state; And
[BootProgress](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/Boot/Progress.interface.yaml)
tells us current host boots to which process.

References:

1.  https://github.com/openbmc/phosphor-state-manager
2.  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/Host.interface.yaml
3.  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/Boot/Progress.interface.yaml

## Requirements

HostBootTimeMonitor should provide

1.  Breakdown the reboot process into individual **stages**.
2.  Each stage should be seamless which means $Sum(stages) =
    total\_reboot\_wall\_clock\_time$.
3.  **Unexpected reboot** during the boot up process should be handled properly.
    Which means we should expect that the boot up process may start over.
4.  Capable of restoring the real boot up process even an unexpected reboot
    happens. For example, let's say the normal process is `A->B->C->D`. We
    encountered unexpected reboot during `A` and counter again during `B`. So
    the real process is `A->A->B->A->B->C->D`.
5.  Flexibility to omit some stages if that server doesn't have it (or doesn't
    want to report it) . Use the same example above. If a server doesn't have
    stage `B`, then `A->C->D` is also eligible.
6.  Allow users to save extra (arbitrary) durations.
7.  Last host reboot information needs to be persisted until the next host
    reboot.
8.  Should support multi-host. Each host should have its own data.

## Proposed Design

```
            power cycle start                                                          power cycle ends
                    |                                                                          |
                    |                                                                          |
                    |                                                                          |
                    | Userspace |      |     |          |        |        |        |           |
Stage       Serving | Shutdown  | ToS5 | Off | Firmware | Loader | Kernel | InitRD | Userspace | Serving
           ---------+------------------+-----+----------+--------+--------+--------+-----------+----------
                    |           |      |     |          |        |        |        |           |
                    |           |      |     |          |        |        |        |           |
                    |           |      |     |          |        |        |        |           |
                    |  T_last_down_end |    T_s0        |  T_loader_end   |   T_initrd_end     |
                    |                  |                |                 |                    |
Timestamp           |                  |                |                 |                    |
                    |                  |                |                 |                    |
               T_down_start           T_s5            T_fw_end        T_kernel_end          T_user_end

```

In the following section, I will use `T_<timestamp>` to represent a certain
timestamp and use `D_<duration>` to represent a certain duration.

The main function for HostBootTimeMonitor is to report the duration for each
stage. All the stages are listed in the diagram above.

### Definition of each stage

| Stage              | Duration name  | Definition                                                                           |
| ------------------ | -------------- | ------------------------------------------------------------------------------------ |
| Userspace Shutdown | `D_user_down`  | Host userspace shutdown process.                                                     |
| ToS5               | `D_to_s5`      | Time frame from last shutdown notification to S5.                                    |
| Off                | `D_off`        | Same as the S5 state. Host is off while BMC has the power.                           |
| Firmware           | `D_fw`         | Host firmware initialization stage. For most of the cases firmware is equal to BIOS. |
| Loader             | `D_loader`     | Host loader initialization stage. For example linuxboot/nerf.                        |
| Kernel             | `D_kernel`     | Host kernel initialization stage.                                                    |
| InitRD             | `D_initrd`     | Host initRD initialization stage. For example systemd.                               |
| Userspace          | `D_user`       | Host userspace initialization stage.                                                 |
| Unmeasured         | `D_unmeasured` | Whenever host is being through an expected reboot, the duration will belong to here. |
| *Serving           | N/A            | Host completes the reboot and starts providing service.                              |

\* We don't monitor the `serving` stage because that stage means the reboot is
completed.

### How to calculate the duration of each stage?

We ensure that each duration will be clamped by two timestamps in this design.
So duration can be calculated by subtracting the former timestamp from later
timestamp. For example, $D\_off=T\_s0-T\_s5$

Another example for the case that has stage(s) skipped. Let's say we received
`T_initrd_end` right after `T_fw_end`, then the
$D\_initrd=T\_initrd\_end-T\_fw\_end$

Each timestamp will be used as the end of the former duration and the start of
the later duration at the same time. This will guarantee that it's seamless
between stages. Also the $Sum(stages) = total\_reboot\_wall\_clock\_time$.

### How to obtain the timestamps?

BMC has to know the exact time point for tagging a timestamp on it. So an
explicit symptoms must appear when we tag the timestamp.

There are 2 different symptoms in current design, which are

-   Host state changing (from DBus)
-   The moment of receiving specific DBus command (from host)

The timestamps shown in the diagram and its corresponding symptoms are listed
below.

| Timestamp name    | Symptom                                                                    |
| ----------------- | -------------------------------------------------------------------------- |
| `T_down_start`    | Notified by host through DBus                                              |
| `T_last_down_end` | Notified by host through DBus                                              |
| `T_s5`            | DBus, `xyz.openbmc_project.State.Host.CurrentHostState "Running" -> "Off"` |
| `T_s0`            | DBus, `xyz.openbmc_project.State.Host.CurrentHostState "Off" -> "Running"` |
| `T_fw_end`        | Notified by host through DBus                                              |
| `T_loader_end`    | Notified by host through DBus                                              |
| `T_kernel_end`    | Notified by host through DBus                                              |
| `T_initrd_end`    | Notified by host through DBus                                              |
| `T_user_end`      | Notified by host through DBus                                              |

### Extra durations

Besides the durations we listed above. We also allow users to save any extra
durations. The extra duration will be saved separated from the durations above
because it's optional.

### Reboot type

In this design we prefer to report the Reboot type as well. Based on the
definition of [Runlevel](https://en.wikipedia.org/wiki/Runlevel), a shutdown
could be `halt` or `reboot`. Currently this information needs to be informed by
the Host.

### State transition table

In this design, we will use a FSM (Finite-state machine) to describe what's the
allow transition. The `state` of this FSM are the stages defined above plus one
initial state. And the `input` are the symptoms from DBus/CurrentHostState.

| Current State (stage) | Allowed Input (symptom) | Next State (stage) | Note                                                                                        |
| --------------------- | ----------------------- | ------------------ | ------------------------------------------------------------------------------------------- |
| Userspace Shutdown    | `T_last_down_end`       | ToS5               |                                                                                             |
| ..                    | `T_s5`                  | Off                | Unexpected reboot will cause this transition. Time collapse here belongs to `D_unmeasured`. |
| ToS5                  | `T_s5`                  | Off                |                                                                                             |
| Off                   | `T_s0`                  | Firmware           | **Host is off in this state (S5)**                                                          |
| Firmware              | `T_s5`                  | Off                | Unexpected reboot will cause this transition                                                |
| ..                    | `T_fw_end`              | Loader             |                                                                                             |
| ..                    | `T_loader_end`          | Kernel             | `Firmware` time measurement is skipped.                                                     |
| ..                    | `T_kernel_end`          | InitRD             | `Firmware`/`Loader` time measurements are skipped.                                          |
| ..                    | `T_initrd_end`          | Userspace          | `Firmware`/`Loader`/`Kernel` time measurements are skipped.                                 |
| ..                    | `T_user_end`            | Serving            | `Firmware`/`Loader`/`Kernel`/`InitRD` time measurement are skipped.                         |
| Loader                | `T_s5`                  | Off                | Unexpected reboot will cause this transition. Time collapse here belongs to `D_unmeasured`. |
| ..                    | `T_loader_end`          | Kernel             |                                                                                             |
| ..                    | `T_kernel_end`          | InitRD             | `Loader` time measurements are skipped.                                                     |
| ..                    | `T_initrd_end`          | Userspace          | `Loader`/`Kernel` time measurements are skipped.                                            |
| ..                    | `T_user_end`            | Serving            | `Loader`/`Kernel`/`InitRD` time measurements are skipped.                                   |
| Kernel                | `T_s5`                  | Off                | Unexpected reboot will cause this transition. Time collapse here belongs to `D_unmeasured`. |
| ..                    | `T_kernel_end`          | InitRD             |                                                                                             |
| ..                    | `T_initrd_end`          | Userspace          | `Kernel` time measurements are skipped.                                                     |
| ..                    | `T_user_end`            | Serving            | `Kernel`/`InitRD` time measurements are skipped.                                            |
| InitRD                | `T_s5`                  | Off                | Unexpected reboot will cause this transition. Time collapse here belongs to `D_unmeasured`. |
| ..                    | `T_initrd_end`          | Userspace          |                                                                                             |
| ..                    | `T_user_end`            | Serving            | `InitRD` time measurements are skipped.                                                     |
| Userspace             | `T_s5`                  | Off                | Unexpected reboot will cause this transition. Time collapse here belongs to `D_unmeasured`. |
| ..                    | `T_user_end`            | Serving            |                                                                                             |
| Serving               | `T_down_start`          | Userspace Shutdown | Happens on a graceful reboot                                                                |
| ..                    | `T_s5`                  | Off                | Happens on a non-graceful reboot                                                            |

Here we will have the conclusions of this state transition table since it's a
big table.

-   Tray power-cycle process:
    `Initial->Firmware->Loader->Kernel->InitRD->Userspace->Serving`
-   Non-graceful reboot process:
    `Serving->Off->Firmware->Loader->Kernel->InitRD->Userspace->Serving`
-   Graceful reboot process: `Serving->Userspace
    Shutdown->ToS5->Off->Firmware->Loader->Kernel->InitRD->Userspace->Serving`
-   All the states are able to receive `T_s5` and transition to `Off` state.
    (Unless host is off already)
-   These 4 stages `Firmware`/`Loader`/`Kernel`/`InitRD` are not necessary and
    can be skipped.

## Implementation

### Date structure

```c++
enum class Notification : uint8_t {
    kDownStart = 0,
    kUserDownEnd,
    kS5,
    kS0,
    kFWEnd ,
    kLoaderEnd,
    kKernelEnd,
    kInitRDEnd,
    kUserEnd,
};
enum class Stage : uint8_t {
    kUserDown = 0,
    kToS5,
    kOff,
    kFW,
    kLoader,
    kKernel,
    kInitRD,
    kUser,
    kServing,
    kUnmeasured,
};

// Each host has its own boot info
class HostBootTimeInfo {
//              monotonic timestamp -------
//                                        |
//                                        v
    std::vector<std::pair<Notification, double>> notificationTimestamp;
    std::vector<std::pair<Stage, double>> stageDuration;
//                                 ^
//                                 |
//                  duration -------
};
```

Whenever a notification is received from hostX, we will push that `Notification`
along with the monotonic timestamp into hostX's `notificationTimestamp`. Then
put calculated duration into `stageDuration`. So for a specific host, the
received notification will be put in the vector in order. Also this can
guarantee that each host is individual and will not be affected by others.

### Interfaces

#### DBus

DBus will be used as the interface to receive notifications/durations and export
the boot flow and total duration for each stages.
The DBus interfaces should provide:

-   Notify
-   SetDuration
-   SetRebootTime
-   GetRebootFlow
-   GetAllDurations

#### IPMI OEM commands

Here we introduce 3 additional IPMI OEM commands that allow the host to send the
duration or notification to BMC since IPMI is one of the most common interface
between the host and the BMC. The format is shown in the example below. (Google
OEM group will be used in the example)

| Command                                   | Bytes                                                                                                                                                             |
| ----------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Notify (To notify BMC to tag a timestamp) | <netfn+cmd+oem (5 bytes)> + <subcommand for notification (1 byte)> + <timestamp code (1 byte)>                                                                    |
|                                           | e.g. 0x2e 0x32 0x79 0x2b 0x00 0x20 0x00 (Notification=0x00)                                                                                                       |
| Set duration                              | <netfn+cmd+oem (5 bytes)>* + <subcommand for set duration (1 byte)> + <duration in ms (8 bytes)> + <length of duration name (1 byte)> + <duration name (n bytes)> |
|                                           | e.g. 0x2e 0x32 0x79 0x2b 0x00 0x21 0xa0 0xbb 0x0d 0x00 0x00 0x00 0x00 0x00 0x04 0x44 0x48 0x43 0x50 (Duration = 15 mins, Name = "DHCP" in this example)           |
| Set reboot type                           | <netfn+cmd+oem (5 bytes)>* + <subcommand for set duration (1 byte)> + <duration in ms (8 bytes)> + <length of duration name (1 byte)> + <duration name (n bytes)> |
|                                           | e.g. 0x2e 0x32 0x79 0x2b 0x00 0x22 0x00 (Reboot type=0x00)                                                                                                        |

#### IPMI blob

The IPMI blob is mainly for export boot flow and duration for each stage to
host. The raw data will be similar to [HostBootTimeInfo](#date-structure). (one
vector for the pair of notification and timestamp, another vector for the pair
of stages and its duration.) Then we will serialize that raw data and put it
into a blob.

## Alternatives Considered

### Measure the reboot time from current service directly

In current OpenBMC we have
[phosphor-state-manager](https://github.com/openbmc/phosphor-state-manager) and
[x86-power-control](https://github.com/openbmc/x86-power-control) that provides
the `BootProgress` and `CurrentHostState`. We could measure the time delta of a
certain state transition to know how long it takes to transition from a stage to
another stage. However we would like the measurement to be more detailed and
more flexible. So we propose this design instead.

## Impacts

-   Need new IPMI OEM commands
    -   Notify
    -   Set duration
    -   Set reboot type
-   Need new DBus interfaces `xyz/openbmc_project/Time/Boot`

### Organizational

No new repository is required. Phosphor-state-manager will be modified to
implement this design.

Repository maintainer list:

-   phosphor-state-manager
    -   geissonator@yahoo.com

## Testing

What we will test in CI:

-   Unittest for each individual functions
-   Mock the state change of `CurrentHostState` to ensure an unexpected reboot
    can be handled correctly.
-   Mock the receiving IPMI command to ensure the state-transition is expected.
